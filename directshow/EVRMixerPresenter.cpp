#include "stdafx.h"
#include <initguid.h>
#include "EVRMixerPresenter.h"
#include "DirectShow.h"
#include <d3dx9.h>
#include <dxva.h>
#include <mfapi.h>
#include <mferror.h>

CEVRMixerPresenter::CEVRMixerPresenter(const VideoPresenterParam& param, HRESULT& hr)
	: CUnknown(L"CEVRMixerPresenter", NULL)
	, m_param(param)
	, m_state(RENDER_STATE_SHUTDOWN)
	, m_error(0)
	, m_streaming(false)
	, m_stepping(false)
	, m_late(0)
	, m_eos(false)
	, m_prerolled(false)
	, m_log(NULL)
{
	memset(&m_dst, 0, sizeof(m_dst));

	hr = E_FAIL;

	if(m_param.dev == NULL || m_param.wnd == NULL)
	{
		return;
	}

	OSVERSIONINFO osver;

	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	
	if(!GetVersionEx(&osver) || osver.dwPlatformId != VER_PLATFORM_WIN32_NT || osver.dwMajorVersion < 6)
	{
		return;
	}

	HMODULE hModule = LoadLibrary(L"dxva2.dll");

	if(hModule == NULL)
	{
		return;
	}

	FreeLibrary(hModule);

	hr = DXVA2CreateDirect3DDeviceManager9(&m_token, &m_manager);

	if(FAILED(hr)) 
	{
		return;
	}

	hr = m_manager->ResetDevice(m_param.dev, m_token);

	if(FAILED(hr)) 
	{
		return;
	}

	CComPtr<IDirectXVideoDecoderService> decoder;

	HANDLE hDevice = NULL;

	hr = m_manager->OpenDeviceHandle(&hDevice);

	if(SUCCEEDED(hr)) 
	{
		hr = m_manager->GetVideoService(hDevice, __uuidof(IDirectXVideoDecoderService), (void**)&decoder);
		
		hr = m_manager->CloseDeviceHandle(hDevice);
	}

	if(decoder == NULL)
	{
		hr = E_FAIL;

		return;
	}

	hr = S_OK;

	// m_log = _wfopen(Util::Format(L"c:\\evr%p.log", this).c_str(), L"w");
}

CEVRMixerPresenter::~CEVRMixerPresenter()
{
	FreeSamples();

	if(m_log) fclose(m_log);
}

STDMETHODIMP CEVRMixerPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IMFClockStateSink)
		QI(IMFVideoPresenter)
		QI(IMFGetService)
		QI(IMFTopologyServiceLookupClient)
		QI(IMFVideoDeviceID)
		QI(IMFVideoDisplayControl)
		QI(IEVRTrustedVideoPlugin)
		QI(IVideoPresenter)
		riid == __uuidof(IDirect3DDeviceManager9) ? m_manager->QueryInterface(riid, ppv) :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CEVRMixerPresenter::CreateInstance(const VideoPresenterParam& param, IBaseFilter** ppBF, IVideoPresenter** ppVP)
{
	CheckPointer(ppBF, E_POINTER);
	CheckPointer(ppVP, E_POINTER);

	if(*ppBF) return E_INVALIDARG;
	if(*ppVP) return E_INVALIDARG;

	HRESULT hr;

	CComPtr<IBaseFilter> pBF;

	if(FAILED(hr = pBF.CoCreateInstance(CLSID_EnhancedVideoRenderer)))
	{
		return E_FAIL;
	}

	CComQIPtr<IMFGetService> pMFGS = pBF;

	if(pMFGS == NULL) 
	{
		return E_FAIL;
	}

	CComPtr<IMFVideoRenderer> pMFVR;

	hr = pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_IMFVideoRenderer, (void**)&pMFVR);

	if(FAILED(hr) || pMFVR == NULL)
	{
		return E_FAIL;
	}

	CComPtr<IMFVideoPresenter> pVP = new CEVRMixerPresenter(param, hr);

	if(FAILED(hr))
	{
		return E_FAIL;
	}

	hr = pMFVR->InitializeRenderer(NULL, pVP);

	if(FAILED(hr))
	{
		return E_FAIL;
	}

	*ppBF = pBF.Detach();
	*ppVP = CComQIPtr<IVideoPresenter>(pVP).Detach();

	return S_OK;
}

HRESULT CEVRMixerPresenter::GetTexture(IMFSample* sample, IDirect3DTexture9** ppt)
{
	CheckPointer(sample, E_POINTER);
	CheckPointer(ppt, E_POINTER);

	CComPtr<IMFMediaBuffer> buff;

	sample->GetBufferByIndex(0, &buff);

	if(CComQIPtr<IMFGetService> service = buff)
	{
		CComPtr<IDirect3DSurface9> surface;

		service->GetService(MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&surface);

		if(surface != NULL)
		{
			surface->GetContainer(__uuidof(IDirect3DTexture9), (void**)ppt);
		}
	}

	return *ppt ? S_OK : E_FAIL;
}


typedef HRESULT (__stdcall * fpMFCreateVideoMediaType)(
    __in const MFVIDEOFORMAT* pVideoFormat,
    __out IMFVideoMediaType** ppIVideoMediaType
    );

HRESULT CEVRMixerPresenter::RenegotiateMediaType()
{
	CAutoLock cAutoLock(this);

	CheckPointer(m_mixer, E_UNEXPECTED);

	fpMFCreateVideoMediaType myMFCreateVideoMediaType = NULL;

	HMODULE hDll = LoadLibrary(L"evr.dll");

	if(hDll == NULL) return E_FAIL;

	myMFCreateVideoMediaType = (fpMFCreateVideoMediaType)GetProcAddress(hDll, "MFCreateVideoMediaType");

	if(myMFCreateVideoMediaType == NULL) {FreeLibrary(hDll); return E_FAIL;}

	if(m_streaming) FreeSamples();

	HRESULT hr;

	CComPtr<IMFMediaType> type;
	std::vector<IMFMediaType*> mts;

	for(DWORD index = 0; SUCCEEDED(m_mixer->GetOutputAvailableType(0, index, &type)); index++, type = NULL)
	{
		AM_MEDIA_TYPE* pmt = NULL;

		hr = type->GetRepresentation(FORMAT_MFVideoFormat, (void**)&pmt);

		if(FAILED(hr))
		{
			continue;
		}

		MFVIDEOFORMAT* fmt = (MFVIDEOFORMAT*)pmt->pbFormat;

		CComPtr<IMFVideoMediaType> mt;

		hr = myMFCreateVideoMediaType(fmt, &mt);
/*
		{
			UINT32 iii = 0;

			while(1)
			{
				GUID guid;
				PROPVARIANT prop;

				if(S_OK != mt->GetItemByIndex(iii++, &guid, &prop)) break;

				MF_ATTRIBUTE_TYPE type;

				mt->GetItemType(guid, &type);

				std::wstring s;

				Util::ToString(guid, s);

				wprintf(L"%s\n", s.c_str());

				PropVariantClear(&prop);
			}
		}
*/
		/*
		CComPtr<IMFMediaType> mt;

		hr = MFCreateMediaType(&mt);

		if(FAILED(hr))
		{
			continue;
		}
		*/

		/*
		MF_MT_MAJOR_TYPE MFMediaType_Video
		MF_MT_SUBTYPE fmt->guidFormat
		MF_MT_FRAME_SIZE {UINT64 (HI32(Width),LO32(Height))}
		MF_MT_GEOMETRIC_APERTURE U1 {BLOB (MFVideoArea)} [16](0,0,0,0,0,0,0,0,208 'Ð',2 '',0,0,64 '@',2 '',0,0)
		MF_MT_MINIMUM_DISPLAY_APERTURE *
		MF_MT_PIXEL_ASPECT_RATIO {UINT64 (HI32(Numerator),LO32(Denominator))} 0x0000009d00000090 fmt->videoInfo.PixelAspectRatio
		MF_MT_VIDEO_PRIMARIES UINT32 oneof MFVideoPrimaries MFVideoPrimaries_BT709
		MF_MT_INTERLACE_MODE UINT32 oneof MFVideoInterlaceMode MFVideoInterlace_Progressive
		MF_MT_VIDEO_NOMINAL_RANGE UINT32 oneof MFNominalRange MFNominalRange_Wide
		MF_MT_TRANSFER_FUNCTION UINT32 oneof MFVideoTransferFunction MFVideoTransFunc_709
		MF_MT_YUV_MATRIX  UINT32 oneof MFVideoTransferMatrix MFVideoTransferMatrix_BT709
		MF_MT_VIDEO_LIGHTING UINT32 oneof MFVideoLighting MFVideoLighting_dim
		MF_MT_DEFAULT_STRIDE            {UINT32 (INT32)} // in bytes b40 w*4
		MF_MT_ALL_SAMPLES_INDEPENDENT   {UINT32 (BOOL)} 1
		MF_MT_FIXED_SIZE_SAMPLES        {UINT32 (BOOL)} 1
		MF_MT_SAMPLE_SIZE               {UINT32} w*h*4
		MF_MT_FRAME_RATE                {UINT64 (HI32(Numerator),LO32(Denominator))}
		*/
		/*
		LARGE_INTEGER wh;
		
		wh.LowPart = fmt->videoInfo.dwHeight;
		wh.HighPart = fmt->videoInfo.dwWidth;

		LARGE_INTEGER ar;
		
		ar.LowPart = fmt->videoInfo.PixelAspectRatio.Denominator;
		ar.HighPart = fmt->videoInfo.PixelAspectRatio.Numerator;

		LARGE_INTEGER fps;
		
		fps.LowPart = fmt->videoInfo.FramesPerSecond.Denominator;
		fps.HighPart = fmt->videoInfo.FramesPerSecond.Numerator;

		MFVideoArea area;
		memset(&area, 0, sizeof(area));
		area.Area.cx = fmt->videoInfo.dwWidth;
		area.Area.cy = fmt->videoInfo.dwHeight;

		mt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		mt->SetGUID(MF_MT_SUBTYPE, fmt->guidFormat);
		mt->SetUINT64(MF_MT_FRAME_SIZE, *(UINT64*)&wh);
		mt->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&area, sizeof(area));
		mt->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&area, sizeof(area));
		mt->SetUINT64(MF_MT_PIXEL_ASPECT_RATIO, *(UINT64*)&ar);
		mt->SetUINT32(MF_MT_VIDEO_PRIMARIES, fmt->videoInfo.ColorPrimaries);
		mt->SetUINT32(MF_MT_INTERLACE_MODE, fmt->videoInfo.InterlaceMode);
		mt->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, fmt->videoInfo.NominalRange);
		mt->SetUINT32(MF_MT_TRANSFER_FUNCTION, fmt->videoInfo.TransferFunction);
		mt->SetUINT32(MF_MT_YUV_MATRIX, fmt->videoInfo.TransferMatrix);
		mt->SetUINT32(MF_MT_VIDEO_LIGHTING, fmt->videoInfo.SourceLighting);
		mt->SetUINT32(MF_MT_DEFAULT_STRIDE, fmt->videoInfo.dwWidth * 4); // TODO: check non-mod8
		mt->SetUINT32(MF_MT_SAMPLE_SIZE, fmt->videoInfo.dwWidth * fmt->videoInfo.dwHeight * 4);
		mt->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1);
		mt->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, 1);
		mt->SetBlob(MF_MT_FRAME_RATE, (UINT8*)&fps, sizeof(fps));
		*/
		UINT32 merit = 100;

		switch(fmt->surfaceInfo.Format)
		{
			case FCC('NV12'): merit = 900; break;
			case FCC('YV12'): merit = 800; break;
			case FCC('YUY2'): merit = 700; break;
			case FCC('UYVY'): merit = 600; break;
			case D3DFMT_X8R8G8B8:
			case D3DFMT_A8R8G8B8: 
			case D3DFMT_R8G8B8: 
			case D3DFMT_R5G6B5: merit = 0; break;
		}

		mt->SetUINT32(__uuidof(CEVRMixerPresenter), merit);
		mt->SetUINT32(MF_MT_PAN_SCAN_ENABLED, 0);
		mt->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255);
		mt->SetUINT32(MF_MT_PAD_CONTROL_FLAGS, MFVideoPadFlag_PAD_TO_None);
		
		type->FreeRepresentation(FORMAT_MFVideoFormat, pmt);

		hr = m_mixer->SetOutputType(0, CComPtr<IMFMediaType>(mt), MFT_SET_TYPE_TEST_ONLY);

		if(FAILED(hr))
		{
			continue;
		}

		mts.push_back(mt.Detach());
	}

	std::sort(mts.begin(), mts.end(), [] (IMFMediaType* a, IMFMediaType* b) -> bool
	{
		UINT32 ma, mb;

		a->GetUINT32(__uuidof(CEVRMixerPresenter), &ma);
		b->GetUINT32(__uuidof(CEVRMixerPresenter), &mb);

		return ma > mb;
	});

	for(auto i = mts.begin(); i != mts.end(); i++)
	{
		IMFMediaType* mt = *i;
		
		hr = SetMediaType(mt);

		if(SUCCEEDED(hr))
		{
			hr = m_mixer->SetOutputType(0, mt, 0);

			if(SUCCEEDED(hr))
			{
				break;
			}

			SetMediaType(NULL);
		}
	}

	for(auto i = mts.begin(); i != mts.end(); i++)
	{
		(*i)->Release();
	}

	if(m_streaming) AllocSamples();

	FreeLibrary(hDll);

	return S_OK;
}

HRESULT CEVRMixerPresenter::SetMediaType(IMFMediaType* mt)
{
	HRESULT hr;

	FreeSamples();

	m_mt = mt;

	if(mt != NULL)
	{
		UINT32 w = 0, h = 0;
		UINT32 arx = 1, ary = 1;

		hr = MFGetAttributeSize(mt, MF_MT_FRAME_SIZE, &w, &h);
		hr = MFGetAttributeSize(mt, MF_MT_PIXEL_ASPECT_RATIO, &arx, &ary);

		arx = w * arx / ary;
		ary = h;

		wprintf(L"%d x %d (%d:%d)\n", w, h, arx, ary);

		m_size = Vector2i(w, h);
		m_ar = Vector2i(arx, ary);
	}

	return S_OK;
}

HRESULT CEVRMixerPresenter::BeginStreaming()
{
	CAutoLock cAutoLock(this);

	if(m_log) fwprintf(m_log, L"BeginStreaming\n");

	m_streaming = true;
	m_stepping = false;
	m_eos = false;
	m_prerolled = false;
	m_late = 0;
	m_first = true;

	AllocSamples();

	CAMThread::Create();

	return S_OK;
}

HRESULT CEVRMixerPresenter::EndStreaming()
{
	// must not lock before closing the thread

	CAMThread::CallWorker(0);
	CAMThread::Close();

	CAutoLock cAutoLock(this);

	m_streaming = false;
	m_stepping = false;

	FreeSamples();

	if(m_log) fwprintf(m_log, L"EndStreaming\n");

	return S_OK;
}

HRESULT CEVRMixerPresenter::ProcessInputNotify()
{
	CAutoLock cAutoLock(this);

	if(m_mt == NULL || m_mixer == NULL) 
	{
		return E_UNEXPECTED;
	}

    HRESULT hr = S_OK;

    while(hr == S_OK && m_samples.size() > 1)
    {
		if(m_prerolled)
		{
			if(m_state != RENDER_STATE_STARTED || m_stepping && m_steps <= 0)
			{
				break;
			}
		}

		IMFSample* sample = m_samples.front();

		LONGLONG start = 0, now = 0, system = 0;

		if(m_clock)
		{
			m_clock->GetCorrelatedTime(0, &start, &system);
		}

		MFT_OUTPUT_DATA_BUFFER buff;

		memset(&buff, 0, sizeof(buff));

	    buff.pSample = sample;

	    DWORD status = 0;

		hr = m_mixer->ProcessOutput(0, 1, &buff, &status);

		// 
		if(m_log) fwprintf(m_log, L"[o] system = %I64d now = %I64d (hr = %08x, status = %d %d) (%d %d %d)\n", system / 10000, start / 10000, hr, status, buff.dwStatus, m_samples.size(), m_scheduled.size(), m_current != NULL ? 1 : 0);

		if(SUCCEEDED(hr))
		{
	        if(m_clock)
	        {
	            m_clock->GetCorrelatedTime(0, &now, &system);

	            LONGLONG latency = now - start;

				m_sink->Notify(EC_PROCESSING_LATENCY, (LONG_PTR)&latency, 0);

				start = 0;

				if(SUCCEEDED(sample->GetSampleTime(&start)))
				{
					if(m_log) fwprintf(m_log, L"[i] system = %I64d now = %I64d start = %I64d (latency = %I64d) (%d %d %d)\n", system / 10000, now / 10000, start / 10000, latency / 10000, m_samples.size(), m_scheduled.size(), m_current != NULL ? 1 : 0);
					// printf("[i] now = %I64d start = %I64d (%d %d %d)\n", now / 10000, start / 10000, m_samples.size(), m_scheduled.size(), m_current != NULL ? 1 : 0);
				}
	        }

			m_samples.pop_front();

			m_scheduled.push_back(sample);

			m_prerolled = true;

			if(m_stepping && --m_steps <= 0)
			{
				m_sink->Notify(EC_STEP_COMPLETE, (LONG_PTR)FALSE, 0);
			}
		}
		else
		{
	        if(hr == MF_E_TRANSFORM_TYPE_NOT_SET)
	        {
	            hr = RenegotiateMediaType();
	        }
	        else if(hr == MF_E_TRANSFORM_STREAM_CHANGE)
	        {
	            SetMediaType(NULL);
	        }
	        else if(hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
	        {
	        }
		}
    }

    if(hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
		CheckEndOfStream();
    }

	return hr;
}

HRESULT CEVRMixerPresenter::Flush()
{
	CAutoLock cAutoLock(this);

	if(m_log) fwprintf(m_log, L"Flush\n");

	if(!m_scheduled.empty())
	{
		m_samples.insert(m_samples.end(), m_scheduled.begin(), m_scheduled.end());
	
		m_scheduled.clear();
	}

	m_eos = false;
	m_prerolled = false;
	m_late = 0;

	return S_OK;
}

HRESULT CEVRMixerPresenter::CheckEndOfStream()
{
	CAutoLock cAutoLock(this);

    if(m_eos && m_scheduled.empty())
    {
		m_sink->Notify(EC_COMPLETE, (LONG_PTR)S_OK, 0);

		m_eos = false;
	}

	return S_OK;
}

HRESULT CEVRMixerPresenter::Step(int steps)
{
	CAutoLock cAutoLock(this);

	m_stepping = true;
	m_steps = steps;

	return S_OK;
}

HRESULT CEVRMixerPresenter::CancelStep()
{
	CAutoLock cAutoLock(this);

	if(m_stepping) 
	{
		m_sink->Notify(EC_STEP_COMPLETE, (LONG_PTR)TRUE, 0);

		m_stepping = false;
		m_steps = 0;
	}

	return S_OK;
}

HRESULT CEVRMixerPresenter::AllocSamples()
{
	HRESULT hr;

	for(int i = 0; i < 3; i++)
	{
		CComPtr<IDirect3DTexture9> texture;

		hr = m_param.dev->CreateTexture(m_size.x, m_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);

		if(FAILED(hr))
		{
			FreeSamples();

			return E_FAIL;
		}

		CComPtr<IDirect3DSurface9> surface;

		texture->GetSurfaceLevel(0, &surface);

		m_param.dev->ColorFill(surface, NULL, 0);

		CComPtr<IMFSample> sample;

		hr = MFCreateVideoSampleFromSurface(surface, &sample);

		if(FAILED(hr))
		{
			FreeSamples();

			return E_FAIL;
		}

		m_samples.push_back(sample.Detach());
	}

	return S_OK;
}

void CEVRMixerPresenter::FreeSamples()
{
	Flush();

	if(m_current != NULL)
	{
		m_samples.push_back(m_current);

		m_current = NULL;
	}

	for(auto i = m_samples.begin(); i != m_samples.end(); i++)
	{
		if(*i != NULL) (*i)->Release();
	}

	m_samples.clear();
}

DWORD CEVRMixerPresenter::ThreadProc()
{
	HRESULT hr;

	SetThreadPriority(m_hThread, THREAD_PRIORITY_HIGHEST);

	while(!CheckRequest(NULL))
	{
		Sleep(1);

		ProcessInputNotify();

		CAutoLock cAutoLock(this);

		while(!m_scheduled.empty())
		{
			IMFSample* sample = m_scheduled.front();

			bool present = true;

			LONGLONG start = 0;

			hr = sample->GetSampleTime(&start);

			LONGLONG now = 0, system = 0;

			if(SUCCEEDED(hr) && m_clock != NULL)
			{
				m_clock->GetCorrelatedTime(0, &now, &system);

				DWORD cc = 0;

				hr = m_clock->GetClockCharacteristics(&cc);
				
				if((cc & MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ) == 0) 
				{
					MFCLOCK_PROPERTIES props; 
					
					if(m_clock->GetProperties(&props) == S_OK)
					{
						now = (now * 10000000) / props.qwClockFrequency;
					}
				}

				LONGLONG dt = now - system;

				if(abs(dt - m_error) < 400000)
				{
					now = system + m_error;
				}
				else
				{
					m_error = now - system;
				}

				present = now >= start - 10000;

				if(present && m_late == 0)
				{
					if(now >= start + 200000)
					{
						m_late = now < 30000000 ? 100 : /*now >= start + 5000000 ? 25 :*/ now >= start + 1000000 ? 2 : 1;
					
						m_late++;
					}
				}
			}

			if(!present)
			{
				m_late = 0;

				break;
			}

			if(m_log) fwprintf(m_log, L"[p] system = %I64d now = %I64d start = %I64d (late = %I64d / %d) (%d %d %d)\n", system / 10000, now / 10000, start / 10000, (start - now) / 10000, m_late, m_samples.size(), m_scheduled.size(), m_current != NULL ? 1 : 0);

			if(0)
			{
				CComPtr<IDirect3DTexture9> texture;

				if(SUCCEEDED(GetTexture(sample, &texture)))
				{
					static int i = 0;

					::D3DXSaveTextureToFile(Util::Format(L"c:\\%016I64d_%d.jpg", start / 10000, i++).c_str(), D3DXIFF_JPG, texture, NULL);
				}
			}

			m_scheduled.pop_front();

			if(m_late > 0)
			{
				m_late--;
			}

			if(m_late > 0 && !m_first)
			{
				// printf("late = %d (%I64d %d)\n", m_late, (now - start) / 10000, clock());

				m_samples.push_front(sample);
			}
			else
			{
				m_first = false;

				if(m_current != NULL)
				{
					m_samples.push_back(m_current);
				
					m_current = NULL;
				}

				m_current = sample;

				SetEvent(m_param.flip);
			}

			// Sleep(1); // yield
		}
	}

	Reply(0);

	return 0;
}

// IMFClockStateSink

STDMETHODIMP CEVRMixerPresenter::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
	m_state = RENDER_STATE_STARTED;

	m_error = 0;

	if(m_log) fwprintf(m_log, L"[OnClockStart] system = %I64d offset = %I64d\n", hnsSystemTime / 10000, llClockStartOffset / 10000);

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::OnClockStop(MFTIME hnsSystemTime)
{
	m_state = RENDER_STATE_STOPPED;

	m_error = 0;

	if(m_log) fwprintf(m_log, L"[OnClockStop] system = %I64d\n", hnsSystemTime / 10000);

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::OnClockPause(MFTIME hnsSystemTime)
{
	m_state = RENDER_STATE_PAUSED;

	m_error = 0;

	if(m_log) fwprintf(m_log, L"[OnClockPause] system = %I64d\n", hnsSystemTime / 10000);

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
	m_state = RENDER_STATE_STARTED;

	m_error = 0;

	if(m_log) fwprintf(m_log, L"[OnClockRestart] system = %I64d\n", hnsSystemTime / 10000);

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
	m_error = 0;

	if(m_log) fwprintf(m_log, L"[OnClockSetRate] system = %I64d\n", hnsSystemTime / 10000);

	return E_NOTIMPL;
}

// IMFVideoPresenter

STDMETHODIMP CEVRMixerPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	HRESULT hr = S_OK;

	switch(eMessage)
	{
	case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
		hr = RenegotiateMediaType();
		break;

	case MFVP_MESSAGE_BEGINSTREAMING: // => paused
		hr = BeginStreaming();
		break;

	case MFVP_MESSAGE_ENDSTREAMING: // => stopped
		hr = EndStreaming();
		break;

	case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
		if(m_log) fwprintf(m_log, L"ProcessInputNotify\n");
		// hr = ProcessInputNotify();
		break;

	case MFVP_MESSAGE_FLUSH:
		hr = Flush();
		break;

	case MFVP_MESSAGE_STEP:
		hr = Step((int)ulParam);
		break;

	case MFVP_MESSAGE_CANCELSTEP:
		hr = CancelStep();
		break;

	case MFVP_MESSAGE_ENDOFSTREAM:
		m_eos = true;
		hr = CheckEndOfStream();
		break;

	default:
		ASSERT(0);
		hr = E_NOTIMPL;
		break;
	}

	return hr;
}

STDMETHODIMP CEVRMixerPresenter::GetCurrentMediaType(IMFVideoMediaType** ppMediaType)
{
	CAutoLock cAutoLock(this);

	CheckPointer(ppMediaType, E_POINTER);

	if(m_mt == NULL)
	{
		return MF_E_NOT_INITIALIZED;
	}

	return m_mt->QueryInterface(__uuidof(IMFVideoMediaType), (void**)ppMediaType);
}

//

namespace FAKE
{
	interface IDirect3DDeviceManager9;

    typedef struct IDirect3DDeviceManager9Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirect3DDeviceManager9 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirect3DDeviceManager9 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirect3DDeviceManager9 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResetDevice )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][in] */ 
            __in  IDirect3DDevice9 *pDevice,
            /* [annotation][in] */ 
            __in  UINT resetToken);
        
        HRESULT ( STDMETHODCALLTYPE *OpenDeviceHandle )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][out] */ 
            __out  HANDLE *phDevice);
        
        HRESULT ( STDMETHODCALLTYPE *CloseDeviceHandle )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][in] */ 
            __in  HANDLE hDevice);
        
        HRESULT ( STDMETHODCALLTYPE *TestDevice )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][in] */ 
            __in  HANDLE hDevice);
        
        HRESULT ( STDMETHODCALLTYPE *LockDevice )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][in] */ 
            __in  HANDLE hDevice,
            /* [annotation][out] */ 
            __deref_out  IDirect3DDevice9 **ppDevice,
            /* [annotation][in] */ 
            __in  BOOL fBlock);
        
        HRESULT ( STDMETHODCALLTYPE *UnlockDevice )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][in] */ 
            __in  HANDLE hDevice,
            /* [annotation][in] */ 
            __in  BOOL fSaveState);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoService )( 
            IDirect3DDeviceManager9 * This,
            /* [annotation][in] */ 
            __in  HANDLE hDevice,
            /* [annotation][in] */ 
            __in  REFIID riid,
            /* [annotation][out] */ 
            __deref_out  void **ppService);
        
        END_INTERFACE
    } IDirect3DDeviceManager9Vtbl;

    interface IDirect3DDeviceManager9
    {
        CONST_VTBL struct IDirect3DDeviceManager9Vtbl *lpVtbl;
    };

	interface IDirectXVideoDecoderService;

    typedef struct IDirectXVideoDecoderServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirectXVideoDecoderService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirectXVideoDecoderService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirectXVideoDecoderService * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDirectXVideoDecoderService * This,
            /* [annotation][in] */ 
            __in  UINT Width,
            /* [annotation][in] */ 
            __in  UINT Height,
            /* [annotation][in] */ 
            __in  UINT BackBuffers,
            /* [annotation][in] */ 
            __in  D3DFORMAT Format,
            /* [annotation][in] */ 
            __in  D3DPOOL Pool,
            /* [annotation][in] */ 
            __in  DWORD Usage,
            /* [annotation][in] */ 
            __in  DWORD DxvaType,
            /* [annotation][size_is][out] */ 
            __out_ecount(BackBuffers+1)  IDirect3DSurface9 **ppSurface,
            /* [annotation][out][in] */ 
            __inout_opt  HANDLE *pSharedHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecoderDeviceGuids )( 
            IDirectXVideoDecoderService * This,
            /* [annotation][out] */ 
            __out  UINT *pCount,
            /* [annotation][size_is][unique][out] */ 
            __deref_out_ecount_opt(*pCount)  GUID **pGuids);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecoderRenderTargets )( 
            IDirectXVideoDecoderService * This,
            /* [annotation][in] */ 
            __in  REFGUID Guid,
            /* [annotation][out] */ 
            __out  UINT *pCount,
            /* [annotation][size_is][unique][out] */ 
            __deref_out_ecount_opt(*pCount)  D3DFORMAT **pFormats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecoderConfigurations )( 
            IDirectXVideoDecoderService * This,
            /* [annotation][in] */ 
            __in  REFGUID Guid,
            /* [annotation][in] */ 
            __in  const DXVA2_VideoDesc *pVideoDesc,
            /* [annotation][in] */ 
            __reserved  void *pReserved,
            /* [annotation][out] */ 
            __out  UINT *pCount,
            /* [annotation][size_is][unique][out] */ 
            __deref_out_ecount_opt(*pCount)  DXVA2_ConfigPictureDecode **ppConfigs);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoDecoder )( 
            IDirectXVideoDecoderService * This,
            /* [annotation][in] */ 
            __in  REFGUID Guid,
            /* [annotation][in] */ 
            __in  const DXVA2_VideoDesc *pVideoDesc,
            /* [annotation][in] */ 
            __in  const DXVA2_ConfigPictureDecode *pConfig,
            /* [annotation][size_is][in] */ 
            __in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets,
            /* [annotation][in] */ 
            __in  UINT NumRenderTargets,
            /* [annotation][out] */ 
            __deref_out  IDirectXVideoDecoder **ppDecode);
        
        END_INTERFACE
    } IDirectXVideoDecoderServiceVtbl;

    interface IDirectXVideoDecoderService
    {
        CONST_VTBL struct IDirectXVideoDecoderServiceVtbl *lpVtbl;
    };

	interface IDirectXVideoDecoder;

    typedef struct IDirectXVideoDecoderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirectXVideoDecoder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirectXVideoDecoder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirectXVideoDecoder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoDecoderService )( 
            IDirectXVideoDecoder * This,
            /* [annotation][out] */ 
            __deref_out  IDirectXVideoDecoderService **ppService);
        
        HRESULT ( STDMETHODCALLTYPE *GetCreationParameters )( 
            IDirectXVideoDecoder * This,
            /* [annotation][out] */ 
            __out_opt  GUID *pDeviceGuid,
            /* [annotation][out] */ 
            __out_opt  DXVA2_VideoDesc *pVideoDesc,
            /* [annotation][out] */ 
            __out_opt  DXVA2_ConfigPictureDecode *pConfig,
            /* [annotation][size_is][unique][out] */ 
            __out_ecount(*pNumSurfaces)  IDirect3DSurface9 ***pDecoderRenderTargets,
            /* [annotation][out] */ 
            __out_opt  UINT *pNumSurfaces);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDirectXVideoDecoder * This,
            /* [annotation][in] */ 
            __in  UINT BufferType,
            /* [annotation][out] */ 
            __out  void **ppBuffer,
            /* [annotation][out] */ 
            __out  UINT *pBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseBuffer )( 
            IDirectXVideoDecoder * This,
            /* [annotation][in] */ 
            __in  UINT BufferType);
        
        HRESULT ( STDMETHODCALLTYPE *BeginFrame )( 
            IDirectXVideoDecoder * This,
            /* [annotation][in] */ 
            __in  IDirect3DSurface9 *pRenderTarget,
            /* [annotation][in] */ 
            __in_opt  void *pvPVPData);
        
        HRESULT ( STDMETHODCALLTYPE *EndFrame )( 
            IDirectXVideoDecoder * This,
            /* [annotation][out] */ 
            __inout_opt  HANDLE *pHandleComplete);
        
        HRESULT ( STDMETHODCALLTYPE *Execute )( 
            IDirectXVideoDecoder * This,
            /* [annotation][in] */ 
            __in  const DXVA2_DecodeExecuteParams *pExecuteParams);
        
        END_INTERFACE
    } IDirectXVideoDecoderVtbl;

    interface IDirectXVideoDecoder
    {
        CONST_VTBL struct IDirectXVideoDecoderVtbl *lpVtbl;
    };

	static struct {void* buff; UINT size;} s_buffers[16];

	static IDirect3DDeviceManager9Vtbl s_man_org;
	static IDirectXVideoDecoderServiceVtbl s_decsvc_org;
	static IDirectXVideoDecoderVtbl s_dec_org;
	static int s_counter = 0;

	static STDMETHODIMP GetBuffer( 
            IDirectXVideoDecoder * This,
            /* [annotation][in] */ 
            __in  UINT BufferType,
            /* [annotation][out] */ 
            __out  void **ppBuffer,
            /* [annotation][out] */ 
            __out  UINT *pBufferSize)
	{
		HRESULT hr = s_dec_org.GetBuffer(This, BufferType, ppBuffer, pBufferSize);

		s_buffers[BufferType].buff = *ppBuffer;
		s_buffers[BufferType].size = *pBufferSize;

		memset(s_buffers[BufferType].buff, 0, s_buffers[BufferType].size);

		return hr;
	}
        
	static STDMETHODIMP ReleaseBuffer( 
            IDirectXVideoDecoder * This,
            /* [annotation][in] */ 
            __in  UINT BufferType)
	{
		/*
		std::string fn = Util::Format("c:\\temp\\2_%04d_%d.bin", s_counter++, BufferType);
		
		if(FILE* fp = fopen(fn.c_str(), "wb"))
		{
			BYTE* p = (BYTE*)s_buffers[BufferType].buff;
			int size = s_buffers[BufferType].size;
			while(size > 0 && p[size - 1] == 0) size--;
			fwrite(p, size, 1, fp);
			fclose(fp);
		}
		*/
		s_buffers[BufferType].buff = NULL;
		s_buffers[BufferType].size = 0;

		HRESULT hr = s_dec_org.ReleaseBuffer(This, BufferType);

		return hr;
	}

    static STDMETHODIMP CreateVideoDecoder( 
        IDirectXVideoDecoderService * This,
        /* [annotation][in] */ 
        __in  REFGUID Guid,
        /* [annotation][in] */ 
        __in  const DXVA2_VideoDesc *pVideoDesc,
        /* [annotation][in] */ 
        __in  const DXVA2_ConfigPictureDecode *pConfig,
        /* [annotation][size_is][in] */ 
        __in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets,
        /* [annotation][in] */ 
        __in  UINT NumRenderTargets,
        /* [annotation][out] */ 
        __deref_out  ::IDirectXVideoDecoder **ppDecode)
	{
		HRESULT hr = s_decsvc_org.CreateVideoDecoder(This, Guid, pVideoDesc, pConfig, ppDecoderRenderTargets, NumRenderTargets, ppDecode);
		
		if(SUCCEEDED(hr))
		{
			FAKE::IDirectXVideoDecoder* d = *(FAKE::IDirectXVideoDecoder**)ppDecode;

			if(d->lpVtbl->GetBuffer != GetBuffer)
			{
				memset(&s_buffers, 0, sizeof(s_buffers));

				FAKE::s_dec_org = *d->lpVtbl;

				DWORD dw1 = PAGE_READWRITE, dw2 = 0;
				VirtualProtect(d->lpVtbl, sizeof(*d->lpVtbl), dw1, &dw2);
				d->lpVtbl->GetBuffer = GetBuffer;
				d->lpVtbl->ReleaseBuffer = ReleaseBuffer;
				VirtualProtect(d->lpVtbl, sizeof(*d->lpVtbl), dw2, &dw1);
			}
		}

		return hr;
	}

	static STDMETHODIMP GetDecoderConfigurations( 
        IDirectXVideoDecoderService * This,
        /* [annotation][in] */ 
        __in  REFGUID Guid,
        /* [annotation][in] */ 
        __in  const DXVA2_VideoDesc *pVideoDesc,
        /* [annotation][in] */ 
        __reserved  void *pReserved,
        /* [annotation][out] */ 
        __out  UINT *pCount,
        /* [annotation][size_is][unique][out] */ 
        __deref_out_ecount_opt(*pCount)  DXVA2_ConfigPictureDecode **ppConfigs)
	{
		HRESULT hr = s_decsvc_org.GetDecoderConfigurations(This, Guid, pVideoDesc, pReserved, pCount, ppConfigs);

		return hr;
	}

	static STDMETHODIMP GetVideoService( 
	    IDirect3DDeviceManager9 * This,
	    /* [annotation][in] */ 
	    __in  HANDLE hDevice,
	    /* [annotation][in] */ 
	    __in  REFIID riid,
	    /* [annotation][out] */ 
	    __deref_out  void **ppService)
	{
		HRESULT hr = s_man_org.GetVideoService(This, hDevice, riid, ppService);

		if(SUCCEEDED(hr) && riid == __uuidof(::IDirectXVideoDecoderService))
		{
			FAKE::IDirectXVideoDecoderService* d = *(FAKE::IDirectXVideoDecoderService**)ppService;

			if(d->lpVtbl->CreateVideoDecoder != CreateVideoDecoder)
			{
				FAKE::s_decsvc_org = *d->lpVtbl;

				DWORD dw1 = PAGE_READWRITE, dw2 = 0;
				VirtualProtect(d->lpVtbl, sizeof(*d->lpVtbl), dw1, &dw2);
				d->lpVtbl->CreateVideoDecoder = CreateVideoDecoder;
				d->lpVtbl->GetDecoderConfigurations = GetDecoderConfigurations;
				VirtualProtect(d->lpVtbl, sizeof(*d->lpVtbl), dw2, &dw1);
			}
		}

		return hr;
	}
}

// IMFGetService

STDMETHODIMP CEVRMixerPresenter::GetService(REFGUID guidService, REFIID riid, LPVOID* ppvObject)
{
	*ppvObject = NULL;

	if(guidService == MR_VIDEO_RENDER_SERVICE)
	{
		return NonDelegatingQueryInterface(riid, ppvObject);
	}
	else if(guidService == MR_VIDEO_ACCELERATION_SERVICE)
	{
		HRESULT hr = m_manager->QueryInterface(__uuidof(IDirect3DDeviceManager9), ppvObject);
		/*
		if(SUCCEEDED(hr))
		{
			FAKE::IDirect3DDeviceManager9* m = *(FAKE::IDirect3DDeviceManager9**)ppvObject;

			if(m->lpVtbl->GetVideoService != FAKE::GetVideoService)
			{
				FAKE::s_man_org = *m->lpVtbl;

				DWORD dw1 = PAGE_READWRITE, dw2 = 0;
				VirtualProtect(m->lpVtbl, sizeof(*m->lpVtbl), dw1, &dw2);
				m->lpVtbl->GetVideoService = FAKE::GetVideoService;
				VirtualProtect(m->lpVtbl, sizeof(*m->lpVtbl), dw2, &dw1);
			}
		}
		*/
		return hr;
	}

	return E_NOINTERFACE;
}

// IMFTopologyServiceLookupClient

STDMETHODIMP CEVRMixerPresenter::InitServicePointers(IMFTopologyServiceLookup* pLookup)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	DWORD n = 1;

	m_mixer = NULL;
	m_sink = NULL;
	m_clock = NULL;

	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE, __uuidof(IMFTransform), (void**)&m_mixer, &n);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, __uuidof(IMediaEventSink), (void**)&m_sink, &n);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, __uuidof(IMFClock), (void**)&m_clock, &n);

	m_state = RENDER_STATE_STOPPED;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::ReleaseServicePointers()
{
	CAutoLock cAutoLock(this);

	m_mixer = NULL;
	m_sink = NULL;
	m_clock = NULL;

	m_state = RENDER_STATE_SHUTDOWN;

	return S_OK;
}

// IMFVideoDeviceID

STDMETHODIMP CEVRMixerPresenter::GetDeviceID(IID* pDeviceID)
{
	CheckPointer(pDeviceID, E_POINTER);

	*pDeviceID = __uuidof(IDirect3DDevice9);

	return S_OK;
}

// IMFVideoDisplayControl

STDMETHODIMP CEVRMixerPresenter::GetNativeVideoSize(SIZE* pszVideo, SIZE* pszARVideo)
{
	if(pszVideo)
	{
		pszVideo->cx = m_size.x;
		pszVideo->cy = m_size.y;
	}

	if(pszARVideo)
	{
		pszARVideo->cx = m_ar.x;
		pszARVideo->cy = m_ar.y;
	}

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::GetIdealVideoSize(SIZE* pszMin, SIZE* pszMax)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::SetVideoPosition(const MFVideoNormalizedRect* pnrcSource, const LPRECT prcDest)
{
	CAutoLock cAutoLock(this);

	if(prcDest) m_dst = *prcDest;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::GetVideoPosition(MFVideoNormalizedRect* pnrcSource, LPRECT prcDest)
{
	CAutoLock cAutoLock(this);

	if(pnrcSource)
	{
		pnrcSource->left = 0.0f;
		pnrcSource->top = 0.0f;
		pnrcSource->right = 1.0f;
		pnrcSource->bottom = 1.0f;
	}

	if(prcDest)
	{
		*prcDest = m_dst;
	}

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::GetAspectRatioMode(DWORD* pdwAspectRatioMode)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::SetVideoWindow(HWND hwndVideo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::GetVideoWindow(HWND* phwndVideo)
{
	*phwndVideo = m_param.wnd;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::RepaintVideo()
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::GetCurrentImage(BITMAPINFOHEADER* pBih, BYTE** pDib, DWORD* pcbDib, LONGLONG* pTimeStamp)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::SetBorderColor(COLORREF Clr)
{
	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::GetBorderColor(COLORREF* pClr)
{
	if(pClr) *pClr = 0;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::SetRenderingPrefs(DWORD dwRenderFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::GetRenderingPrefs(DWORD* pdwRenderFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::SetFullscreen(BOOL fFullscreen)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRMixerPresenter::GetFullscreen(BOOL* pfFullscreen)
{
	return E_NOTIMPL;
}

// IEVRTrustedVideoPlugin

STDMETHODIMP CEVRMixerPresenter::IsInTrustedVideoMode(BOOL* pYes)
{
	CheckPointer(pYes, E_POINTER);

	*pYes = TRUE;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::CanConstrict(BOOL* pYes)
{
	CheckPointer(pYes, E_POINTER);

	*pYes = TRUE;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::SetConstriction(DWORD dwKPix)
{
	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::DisableImageExport(BOOL bDisable)
{
	return S_OK;
}

// IVideoPresenter

STDMETHODIMP CEVRMixerPresenter::GetVideoTexture(IDirect3DTexture9** ppt, Vector2i* ar, Vector4i* src)
{
	CAutoLock cAutoLock(this);

	CheckPointer(ppt, E_POINTER);

	HRESULT hr;

	hr = GetTexture(m_current, ppt);

	if(FAILED(hr))
	{
		return E_FAIL;
	}

	*ar = m_ar;
	*src = Vector4i(0, 0, m_size.x, m_size.y);

	if(m_ar.x == 0 || m_ar.y == 0)
	{
		*ar = m_size;
	}
	
	if(m_log && m_clock)
	{
		static LONGLONG prev = 0;

		LONGLONG start, now, system;

		m_current->GetSampleTime(&start);
		m_clock->GetCorrelatedTime(0, &now, &system);

		// 
		if(m_log) fwprintf(m_log, L"[t] system = %I64d now = %I64d start = %I64d (%I64d %I64d) (%d %d %d)\n", system / 10000, now / 10000, start / 10000, (start - now) / 10000, (prev - now) / 10000, m_samples.size(), m_scheduled.size(), m_current != NULL ? 1 : 0);

		prev = now;
	}
	
	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::SetFlipEvent(HANDLE hFlipEvent)
{
	CAutoLock cAutoLock(this);

	m_param.flip = hFlipEvent;

	return S_OK;
}

STDMETHODIMP CEVRMixerPresenter::GetFPS(double& fps)
{
	fps = m_fps;

	return S_OK;
}
