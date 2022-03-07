#include "stdafx.h"
#include "GenericPlayer.h"
#include "MediaTypeEx.h"
#include "Filters.h"
#include "VMR9AllocatorPresenter.h"
#include "EVRMixerPresenter.h"
#include "TextSubProvider.h"
#include "VobSubProvider.h"
#include "moreuuids.h"
#include "../util/Config.h"

#define SUBPICS 3

//

GenericPlayer::GenericPlayer() 
	: CUnknown(L"GenericPlayer", NULL)
	, m_eof(false)
	, m_pos(0)
	, m_active(true)
	, m_lastactivity(0)
	, m_state(State_Stopped)
	, m_tmp_state(State_Stopped)
	, m_suspended(false)
	, m_hasvideo(true)
	, m_cfgkey(L"Homesys") // TODO
{
	m_sub.track = 0;
}

GenericPlayer::~GenericPlayer()
{
	m_sub.streams.clear();
	m_sub.provider = NULL;

	if(m_graph) 
	{
		m_graph->Reset();
	}
}

STDMETHODIMP GenericPlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IGenericPlayer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

DWORD GenericPlayer::GetSourceConfig()
{
	DWORD ret = -1;

	// TODO

	return ret;
}

DWORD GenericPlayer::GetDecoderConfig()
{
	DWORD ret = -1;

	ret &= ~TRA_MPEG1;

	Util::Config cfg(L"Homesys", L"Decoder");

	if(cfg.GetInt(L"H264", 1) == 0) ret &= ~TRA_H264;
	if(cfg.GetInt(L"DXVA", 1) == 0) ret &= ~TRA_DXVA;

	return ret;
}

bool GenericPlayer::IsSubtitleExt(LPCWSTR ext)
{
	static LPCWSTR s_subexts[] = {L".srt", L".sub", L".smi", L".psb", L".ssa", L".ass", L".idx", L".usf", L".xss", L".txt", L".ssf"};

	for(int i = 0; i < sizeof(s_subexts) / sizeof(s_subexts[0]); i++)
	{
		if(wcsicmp(ext, s_subexts[i]) == 0)
		{
			return true;
		}
	}

	return false;
}

HRESULT GenericPlayer::CreateRenderer()
{
	HRESULT hr;

	m_presenter = NULL;
	m_renderer = NULL;

	hr = CEVRMixerPresenter::CreateInstance(m_param, &m_renderer, &m_presenter);

	if(FAILED(hr))
	{
		m_presenter = NULL;
		m_renderer = NULL;

		hr = CVMR9AllocatorPresenter::CreateInstance(m_param, &m_renderer, &m_presenter);
	}

	return hr;
}

HRESULT GenericPlayer::GetOutputs(IBaseFilter* pBF)
{
	m_outputs.clear();

	Foreach(pBF, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		m_outputs.push_back(CComPtr<IPin>(pin));

		return S_CONTINUE;
	});

	return !m_outputs.empty() ? S_OK : E_FAIL;
}

HRESULT GenericPlayer::NukeOutputs()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		CComPtr<IPin> pin = *i;

		m_graph->NukeDownstream(pin);
	}

	return S_OK;
}

HRESULT GenericPlayer::Render(IPin* pin) 
{
	std::wstring name = DirectShow::GetName(pin);

	if(name[0] == '~') return E_FAIL;

	return m_graph->Render(pin);
}

IBaseFilter* GenericPlayer::GetAudioSwitcher()
{
	IBaseFilter* ret = NULL;

	Foreach(m_graph, [&] (IBaseFilter* pBF) -> HRESULT
	{
		if(DirectShow::GetCLSID(pBF) == __uuidof(CAudioSwitcherFilter))
		{
			ret = pBF;

			return S_OK;
		}

		return S_CONTINUE;
	});

	return ret;
}

void GenericPlayer::LoadSubtitles(LPCWSTR path)
{
	SetSubtitle(NULL);

	m_sub.streams.clear();

	if(wcsstr(path, L"://") != NULL) return;

	std::wstring base = Util::RemoveFileExt(path);
	std::wstring dir = Util::RemoveFileSpec(path);

	std::vector<std::wstring> files;

	WIN32_FIND_DATA fd = {0};

	HANDLE hFind = FindFirstFile((base + L".*").c_str(), &fd);

	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(LPCWSTR ext = PathFindExtension(fd.cFileName))
				{
					if(IsSubtitleExt(ext))
					{
						files.push_back(Util::CombinePath(dir.c_str(), fd.cFileName));
					}
				}
			}
		}
		while(FindNextFile(hFind, &fd));
		
		FindClose(hFind);
	}

	std::sort(files.begin(), files.end(), [] (const std::wstring& a, const std::wstring& b) -> bool
	{
		return wcsicmp(a.c_str(), b.c_str()) < 0;
	});

	for(auto i = files.begin(); i != files.end(); i++)
	{
		LoadSubtitle(i->c_str());
	}
}

bool GenericPlayer::LoadSubtitle(LPCWSTR fn)
{
	CComPtr<ISubStream> ss;
	
	if(ss == NULL && Util::MakeLower(fn).find(L".idx") != std::wstring::npos)
	{
		CComPtr<IVobSubStream> vss = new CVobSubProvider();

		if(SUCCEEDED(vss->OpenFile(fn)) && vss->GetStreamCount() > 0)
		{
			ss = vss;
		}
	}

	if(ss == NULL)
	{
		CComPtr<ITextSubStream> tss = new CTextSubProvider();

		if(SUCCEEDED(tss->OpenFile(fn)) && tss->GetStreamCount() > 0)
		{
			ss = tss;
		}
	}

	if(ss != NULL)
	{
		m_sub.streams.push_back(ss);

		return true;
	}

	return false;
}

void GenericPlayer::LoadSubtitleStreams()
{
	m_sub.streams.remove_if([] (CComPtr<ISubStream>& ss) -> bool 
	{
		return CComQIPtr<IBaseFilter>(ss) != NULL;
	});

	std::list<IPin*> pins;

	Foreach(m_graph, [&] (IBaseFilter* pBF) -> HRESULT
	{
		if(DirectShow::IsSplitter(pBF)) 
		{
			Foreach(pBF, [&] (IPin* pPin) -> HRESULT
			{
				CComPtr<IPin> pPinTo;

				AM_MEDIA_TYPE mt;
				
				if(SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo != NULL
				&& SUCCEEDED(pPin->ConnectionMediaType(&mt)) 
				&& (mt.majortype == MEDIATYPE_Text || mt.majortype == MEDIATYPE_Subtitle))
				{
					pins.push_back(pPin);
				}

				return S_CONTINUE;
			});
		}

		return S_CONTINUE;
	});

	for(auto i = pins.begin(); i != pins.end(); i++)
	{
		IPin* pPin = *i;

		CSubPassThruFilter* passthru = new CSubPassThruFilter(this);

		if(SUCCEEDED(m_graph->AddFilter(passthru, Util::Format(L"SubPassThruFilter%p", passthru).c_str())))
		{
			HRESULT hr;

			CComPtr<IPin> pPinTo;

			hr = pPin->ConnectedTo(&pPinTo);

			CLSID clsid = DirectShow::GetCLSID(pPinTo);
			std::wstring name = DirectShow::GetName(DirectShow::GetFilter(pPinTo));

			m_graph->Disconnect(pPin);
			m_graph->Disconnect(pPinTo);

			if(FAILED(hr = m_graph->ConnectDirect(pPin, DirectShow::GetFirstPin(passthru, PINDIR_INPUT), NULL))
			|| FAILED(hr = m_graph->ConnectDirect(DirectShow::GetFirstPin(passthru, PINDIR_OUTPUT), pPinTo, NULL)))
			{
				hr = m_graph->RemoveFilter(passthru);
				hr = m_graph->ConnectDirect(pPin, pPinTo, NULL);
			}
			else
			{
				CComQIPtr<ISubStream> ss((IBaseFilter*)passthru);
				
				if(ss != NULL)
				{
					m_sub.streams.push_back(ss);
				}
			}
		}
	}

	if(!m_sub.streams.empty())
	{
		m_sub.queue = new CDX9SubPicQueue(m_param.dev, Vector2i(2048, 1024), SUBPICS);
	}

	SetTrack(TrackType::Subtitle, 1);
}

void GenericPlayer::ReplaceSubtitle(ISubStream* ssold, ISubStream* ssnew)
{
	// TODO
}

void GenericPlayer::InvalidateSubtitle(REFERENCE_TIME start, ISubStream* ss)
{
	if(m_sub.queue)
	{
		if(CComQIPtr<ISubStream>(m_sub.provider) == ss)
		{
			m_sub.queue->Invalidate(start);
		}
	}
}

void GenericPlayer::SetSubtitle(ISubStream* ss)
{
	if(ss != NULL)
	{
		if(CComQIPtr<ITextSubStream> tss = ss)
		{
			Util::Config cfg(L"Homesys", L"Subtitle");

			std::wstring face = cfg.GetString(L"Face", L"Arial");
			int size = cfg.GetInt(L"Size", 26);
			bool bold = cfg.GetInt(L"Bold", 1) != 0;
			float outline = (float)cfg.GetInt(L"Outline", 3) / 2;
			float shadow = (float)cfg.GetInt(L"Shadow", 3) / 2;
			int bottom = cfg.GetInt(L"Position", 40);

			m_sub.bbox = cfg.GetInt(L"BBox", 0) != 0;

			tss->SetFont(face.c_str(), size, bold, false, false, false);
			tss->SetColor(0x00ffffff, 0x0000ffff, 0x00000000, 0x80000000);
			tss->SetBackgroundSize(outline);
			tss->SetShadowDepth(shadow);
			tss->SetMargin(2, 40, 40, 40, bottom);
			tss->SetDefaultStyle();
		}

		if(m_sub.queue == NULL)
		{
			m_sub.queue = new CDX9SubPicQueue(m_param.dev, Vector2i(2048, 1024), SUBPICS);
		}
	}

	m_sub.provider = CComQIPtr<ISubProvider>(ss);

	if(m_sub.queue != NULL)
	{
		m_sub.queue->SetSubProvider(m_sub.provider);
	}
}

// IGenericPlayer

STDMETHODIMP GenericPlayer::GetState(OAFilterState& fs)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	fs = m_state;

	return S_OK;

	// return CComQIPtr<IMediaControl>(m_graph)->GetState(INFINITE, &fs);
}

STDMETHODIMP GenericPlayer::GetCurrent(REFERENCE_TIME& rt)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr;

	hr = CComQIPtr<IMediaSeeking>(m_graph)->GetCurrentPosition(&rt);

	if(FAILED(hr))
	{
		return hr;
	}

	if(m_eof && rt == 0) 
	{
		rt = m_pos;

		return hr;
	}

	m_pos = rt;

	return hr;
}

STDMETHODIMP GenericPlayer::GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CheckPointer(m_graph, E_UNEXPECTED);
/*
	REFERNECE_TIME rt;

	hr = CComQIPtr<IMediaSeeking>(m_graph)->GetDuration(&rt);

	start = 0;
	stop = rt;
*/
	return CComQIPtr<IMediaSeeking>(m_graph)->GetAvailable(&start, &stop);
}

STDMETHODIMP GenericPlayer::Seek(REFERENCE_TIME rt)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	m_eof = false;
	m_pos = rt;

	return CComQIPtr<IMediaSeeking>(m_graph)->SetPositions(&rt, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

STDMETHODIMP GenericPlayer::Play()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr = CComQIPtr<IMediaControl>(m_graph)->Run();

	if(SUCCEEDED(hr))
	{
		m_state = State_Running;
		m_suspended = false;
	}

	return hr;
}

STDMETHODIMP GenericPlayer::Pause()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr = CComQIPtr<IMediaControl>(m_graph)->Pause();

	if(SUCCEEDED(hr))
	{
		m_state = State_Paused;
		m_suspended = false;
	}

	return hr;
}

STDMETHODIMP GenericPlayer::Stop()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	m_eof = false;

	HRESULT hr = CComQIPtr<IMediaControl>(m_graph)->Stop();

	if(SUCCEEDED(hr))
	{
		m_state = State_Stopped;
		m_suspended = false;
	}

	return hr;
}

STDMETHODIMP GenericPlayer::PlayPause()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr = E_FAIL;

	OAFilterState fs;

	if(SUCCEEDED(GetState(fs)))
	{
		if(fs == State_Stopped) hr = Play();
		else if(fs == State_Paused) hr = Play();
		else if(fs == State_Running) hr = Pause();
	}

	return hr;
}

STDMETHODIMP GenericPlayer::Suspend()
{
	HRESULT hr = S_OK;

	if(m_state != State_Paused) // == State_Running
	{
		hr = Pause();

		if(SUCCEEDED(hr))
		{
			m_suspended = true;
		}
	}

	return hr;
}

STDMETHODIMP GenericPlayer::Resume()
{
	HRESULT hr = S_OK;

	if(m_suspended)
	{
		hr = Play();
	}

	return hr;
}

STDMETHODIMP GenericPlayer::Prev()
{
	return E_NOTIMPL;
}

STDMETHODIMP GenericPlayer::Next()
{
	return E_NOTIMPL;
}

STDMETHODIMP GenericPlayer::FrameStep()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	return CComQIPtr<IVideoFrameStep>(m_graph)->Step(1, NULL);
}

STDMETHODIMP GenericPlayer::GetVolume(float& volume)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	return m_graph->GetVolume(volume);
}

STDMETHODIMP GenericPlayer::SetVolume(float volume)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	return m_graph->SetVolume(volume);
}

STDMETHODIMP GenericPlayer::GetTrackCount(int type, int& count)
{
	count = 0;

	if(type == TrackType::Video)
	{
		// TODO

		count = 1;

		return S_OK;
	}
	else if(type == TrackType::Audio)
	{
		if(CComQIPtr<IAMStreamSelect> pSS = GetAudioSwitcher())
		{
			DWORD cStreams = 0;

			if(SUCCEEDED(pSS->Count(&cStreams)))
			{
				count = (int)cStreams;
			}
		}

		return S_OK;
	}
	else if(type == TrackType::Subtitle)
	{
		count = 1;

		for(auto i = m_sub.streams.begin(); i != m_sub.streams.end(); i++)
		{
			count += (*i)->GetStreamCount();
		}

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP GenericPlayer::GetTrackInfo(int type, int index, TrackInfo& info)
{
	if(type == TrackType::Video)
	{
		if(index == 0)
		{
			if(CComPtr<IPin> pPin = DirectShow::GetUpStreamPin(DirectShow::GetUpStreamFilter(m_renderer)))
			{
				CComPtr<IPin> pPinTo;

				if(SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo) 
				{
					pPin = pPinTo;
				}

				pPin->ConnectionMediaType(&info.mt);
				
				if(info.mt.majortype == GUID_NULL)
				{
					pPinTo = NULL;
					
					info.mt.InitMediaType();
					
					if(SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo) 
					{
						pPin = pPinTo;
					}

					pPin->ConnectionMediaType(&info.mt);
				}
			}

			if(CComPtr<IPin> pPin = DirectShow::GetUpStreamPin(m_renderer))
			{
				CMediaTypeEx mt;

				pPin->ConnectionMediaType(&mt);
				
				BITMAPINFOHEADER bih;
				
				info.dxva = mt.ExtractBIH(&bih) && (bih.biCompression == 'avxd' || bih.biCompression == 'AVXD');
			}

			GUID broadcom;
			
			Util::ToCLSID(L"{2DE1D17E-46B1-42A8-9AEC-E20E80D9B1A9}", broadcom);

			Foreach(m_graph, [&] (IBaseFilter* pBF) -> HRESULT
			{
				if(DirectShow::GetCLSID(pBF) == broadcom)
				{
					info.broadcom = true;

					return S_OK;
				}

				return S_CONTINUE;
			});

			return S_OK;
		}
	}
	else if(type == TrackType::Audio)
	{
		if(CComQIPtr<IAMStreamSelect> pSS = GetAudioSwitcher())
		{
			DWORD cStreams = 0;
		
			if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0 && index < (int)cStreams)
			{
				WCHAR* pName = NULL;
				CComPtr<IUnknown> pUnk;

				if(SUCCEEDED(pSS->Info(index, NULL, NULL, NULL, NULL, &pName, NULL, &pUnk)))
				{
					info.name = pName;

					CComQIPtr<IPin> pPin = pUnk;

					if(pPin != NULL)
					{
						CComPtr<IBaseFilter> pBF = DirectShow::GetFilter(pPin);

						CComPtr<IPin> pDemuxOutputPin = DirectShow::GetUpStreamPin(DirectShow::GetUpStreamFilter(pBF, pPin));

						if(pDemuxOutputPin != NULL)
						{
							if(CComQIPtr<IDSMPropertyBag> pPB = pDemuxOutputPin)
							{
								CComBSTR bstr;

								if(SUCCEEDED(pPB->GetProperty(L"LANG", &bstr)))
								{
									info.iso6392 = Util::ISO6392ToLanguage(bstr.m_str);
								}
							}

							CComPtr<IPin> pPinTo;

							if(SUCCEEDED(pDemuxOutputPin->ConnectedTo(&pPinTo)) && pPinTo) 
							{
								pDemuxOutputPin = pPinTo; // use the decoder's input for media type, it may detect and set a new one
							}

							pDemuxOutputPin->ConnectionMediaType(&info.mt);

							if(info.mt.majortype == MEDIATYPE_Stream) // no decoder?
							{
								info.mt.InitMediaType();

								pPin->ConnectionMediaType(&info.mt);
							}
						}
					}

					CoTaskMemFree(pName);
				}
			}
		}

		return S_OK;
	}
	else if(type == TrackType::Subtitle)
	{
		if(index == 0)
		{
			info.name = L"";

			return S_OK;
		}
		else
		{
			for(auto i = m_sub.streams.begin(); i != m_sub.streams.end(); i++)
			{
				CComPtr<ISubStream> s = *i;

				for(int i = 0, n = s->GetStreamCount(); i < n; i++, index--)
				{
					if(index == 1)
					{
						WCHAR* name = NULL;
						LCID lcid = 0;

						if(SUCCEEDED(s->GetStreamInfo(i, &name, &lcid)))
						{
							info.name = name;

							CoTaskMemFree(name);

							return S_OK;
						}

						return E_FAIL;
					}
				}
			}
		}
	}

	return E_INVALIDARG;
}

STDMETHODIMP GenericPlayer::GetTrack(int type, int& index)
{
	if(type == TrackType::Video)
	{
		index = 0;

		return S_OK;
	}
	else if(type == TrackType::Audio)
	{
		index = -1;

		if(CComQIPtr<IAMStreamSelect> pSS = GetAudioSwitcher())
		{
			DWORD cStreams = 0;

			if(SUCCEEDED(pSS->Count(&cStreams)))
			{
				for(DWORD i = 0; i < cStreams; i++)
				{
					DWORD flags = 0;

					if(SUCCEEDED(pSS->Info(i, NULL, &flags, NULL, NULL, NULL, NULL, NULL)))
					{
						if(flags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE))
						{
							index = (int)i;

							break;
						}
					}
				}
			}
		}

		return S_OK;
	}
	else if(type == TrackType::Subtitle)
	{
		index = m_sub.track;

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP GenericPlayer::SetTrack(int type, int index)
{
	if(type == TrackType::Audio)
	{
		if(CComQIPtr<IAMStreamSelect> pSS = GetAudioSwitcher())
		{
			return pSS->Enable(index, AMSTREAMSELECTENABLE_ENABLE);
		}
	}
	else if(type == TrackType::Subtitle)
	{
		m_sub.track = index;

		if(index == 0)
		{
			SetSubtitle(NULL);

			return S_OK;
		}
		else
		{
			for(auto i = m_sub.streams.begin(); i != m_sub.streams.end(); i++)
			{
				CComPtr<ISubStream> s = *i;

				for(int i = 0, n = s->GetStreamCount(); i < n; i++, index--)
				{
					if(index == 1)
					{
						s->SetStream(i);
						
						SetSubtitle(s);

						return S_OK;
					}
				}
			}
		}

		SetSubtitle(NULL);

		m_sub.track = 0;
	}

	return E_INVALIDARG;
}

STDMETHODIMP GenericPlayer::SetEOF()
{
	m_eof = true;

	return S_OK;
}

STDMETHODIMP GenericPlayer::IsEOF()
{
	return m_eof ? S_OK : S_FALSE;
}

STDMETHODIMP GenericPlayer::Render(const VideoPresenterParam& param)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	m_param = param;

	HRESULT hr = CreateRenderer();

	return SUCCEEDED(hr) ? ReRender() : E_FAIL;
}

STDMETHODIMP GenericPlayer::ReRender()
{
	CheckPointer(m_graph, E_UNEXPECTED);
	CheckPointer(m_renderer, E_UNEXPECTED);		

	HRESULT hr;

	OAFilterState state = m_state;

	CComQIPtr<IMediaControl> pMC = m_graph;

	hr = pMC->Stop();

	hr = pMC->GetState(INFINITE, &m_state);

	long lVolume = -10000;

	hr = CComQIPtr<IBasicAudio>(m_graph)->get_Volume(&lVolume);

	bool hasVolume = SUCCEEDED(hr);

	NukeOutputs();

	m_hasvideo = true;

	hr = m_graph->AddFilter(m_renderer, L"Video Render");

	if(FAILED(hr)) 
	{
		return E_FAIL;
	}

	int succeeded = 0;

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		CComPtr<IPin> pin = *i;

		hr = Render(pin);
		
		/*
		if(CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_graph)
		{
			CAtlList<CStringW> path;
			CAtlList<CMediaType> mts;

			for(int i = 0; S_OK == pGBDE->GetDeadEnd(i, path, mts); i++)
			{
				_tprintf(_T("-------------\n"));
				_tprintf(_T("%s [%s]"), GetFilterName(GetFilterFromPin(pPin)), GetPinName(pPin));

				if(!path.IsEmpty())
				{
					POSITION pos = path.GetHeadPosition();

					while(pos)
					{
						_tprintf(_T(" -> %s"), path.GetNext(pos));
					}
				}

				_tprintf(_T(":\n"));

				POSITION pos = mts.GetHeadPosition();

				while(pos) 
				{
					CAtlList<CString> sl;

					CMediaTypeEx(mts.GetNext(pos)).Dump(sl);

					POSITION pos = sl.GetHeadPosition();

					while(pos) 
					{
						_tprintf(_T("%s\n"), sl.GetNext(pos));
					}
				}
			}
		}
		*/

		if(SUCCEEDED(hr)) 
		{
			succeeded++;
		}
	}

	m_graph->Clean();

	if(succeeded == 0)
	{
		NukeOutputs(); 
		
		return E_FAIL;
	}

	LoadSubtitleStreams();

	hr = CComQIPtr<IMediaEventEx>(m_graph)->SetNotifyWindow((OAHWND)m_param.wnd, m_param.msg, (LONG_PTR)(IGenericPlayer*)this);

	if(hasVolume)
	{
		hr = CComQIPtr<IBasicAudio>(m_graph)->put_Volume(lVolume);
	}

	if(state == State_Paused) 
	{
		hr = pMC->Pause();

		m_state = state;
	}
	else if(state == State_Running) 
	{
		hr = pMC->Run();

		m_state = state;
	}

	m_hasvideo = DirectShow::GetUpStreamFilter(m_renderer) != NULL;

	m_graph->Dump();

	return S_OK;
}

STDMETHODIMP GenericPlayer::ResetDevice(bool before)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	CComQIPtr<IMediaControl> pMC = m_graph;

	HRESULT hr;

	if(before)
	{
		if(m_renderer_pin == NULL && m_renderer != NULL)
		{
			m_renderer_pin = DirectShow::GetUpStreamPin(m_renderer);

			m_tmp_state = m_state;
			
			GetCurrent(m_tmp_pos);

			CComPtr<IBaseFilter> pBF = DirectShow::GetFilter(m_renderer_pin);

			hr = pMC->Stop();

			hr = m_graph->RemoveFilter(m_renderer);

			if(FAILED(hr))
			{
				while(1)
				{
					hr = pMC->GetState(100, &m_state);

					if(SUCCEEDED(hr) || hr == VFW_E_NOT_IN_GRAPH)
					{
						hr = m_graph->RemoveFilter(m_renderer);

						if(SUCCEEDED(hr))
						{
							break;
						}
					}
				}
			}
			
			// m_presenter = NULL; m_renderer = NULL; 
		}
	}
	else
	{
		if(m_renderer_pin != NULL)
		{
			HRESULT hr;

			// hr = CreateRenderer();

			hr = m_graph->AddFilter(m_renderer, L"Video Renderer");

			hr = m_graph->ConnectDirect(m_renderer_pin, DirectShow::GetFirstPin(m_renderer), NULL);

			m_renderer_pin = NULL;
			
			if(m_tmp_state == State_Paused)
			{
				hr = pMC->Pause();

				m_state = m_tmp_state;
			}
			else if(m_tmp_state == State_Running) 
			{
				hr = pMC->Run();

				m_state = m_tmp_state;
			}

			Seek(m_tmp_pos);
		}
	}

	return S_OK;
}

STDMETHODIMP GenericPlayer::GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src)
{
	CheckPointer(m_presenter, E_UNEXPECTED);

	m_lastactivity = clock();

	return m_presenter->GetVideoTexture(ppt, &ar, &src);
}

STDMETHODIMP GenericPlayer::SetVideoPosition(const Vector4i& r)
{
	CheckPointer(m_presenter, E_UNEXPECTED);

	Vector4i tmp = r;

	if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_presenter)
	{
		return pWC9->SetVideoPosition(NULL, tmp);
	}
	else if(CComQIPtr<IMFVideoDisplayControl> pVDC = m_presenter)
	{
		return pVDC->SetVideoPosition(NULL, tmp);
	}

	return E_FAIL;
}

STDMETHODIMP GenericPlayer::GetSubtitleTexture(IDirect3DTexture9** ppt, const Vector2i& s, Vector4i& r, bool& bbox)
{
	CheckPointer(m_sub.queue, E_FAIL);

	REFERENCE_TIME rt = 0;	
	GetCurrent(rt);
	m_sub.queue->SetTime(rt);

	double fps = 25.0f;
	m_presenter->GetFPS(fps);
	m_sub.queue->SetFPS(fps);

	m_sub.queue->SetTarget(s, r);

	CComPtr<ISubPic> sp;

	if(m_sub.queue->LookupSubPic(rt, &sp))
	{
		CComQIPtr<IDirect3DTexture9> texture = sp;

		if(texture != NULL)
		{
			(*ppt = texture)->AddRef();

			r = Vector4i(0, 0, s.x, s.y);

			sp->GetRect(r);

			bbox = m_sub.bbox;

			return S_OK;
		}
	}

	return E_FAIL;
}

STDMETHODIMP GenericPlayer::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return E_NOTIMPL;
}

STDMETHODIMP GenericPlayer::GetEvent(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2)
{
	CheckPointer(m_graph, E_UNEXPECTED);
	CheckPointer(plEventCode, E_POINTER);
	CheckPointer(plParam1, E_POINTER);
	CheckPointer(plParam2, E_POINTER);

	CComQIPtr<IMediaEventEx> pME = m_graph;

	if(pME && SUCCEEDED(pME->GetEvent(plEventCode, plParam1, plParam2, 0)))
	{
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP GenericPlayer::FreeEvent(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2)
{
	CheckPointer(m_graph, E_UNEXPECTED);
	
	CComQIPtr<IMediaEventEx> pME = m_graph;

	if(pME && SUCCEEDED(pME->FreeEventParams(lEventCode, lParam1, lParam2)))
	{
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP GenericPlayer::Activate(float volume)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr;

	hr = SetVolume(volume);

	if(m_presenter != NULL)
	{
		hr = m_presenter->SetFlipEvent(m_param.flip);
	}

	m_active = true;

	m_lastactivity = clock();

	return S_OK;
}

STDMETHODIMP GenericPlayer::Deactivate()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr;

	hr = SetVolume(0);

	if(m_presenter != NULL)
	{
		hr = m_presenter->SetFlipEvent(NULL);
	}

	m_active = false;

	return S_OK;
}

STDMETHODIMP GenericPlayer::GetFilterGraph(IFilterGraph** ppFG)
{
	if(m_graph == NULL) 
	{
		return E_UNEXPECTED;
	}

	(*ppFG = m_graph)->AddRef();

	return S_OK;
}

STDMETHODIMP GenericPlayer::HasVideo() 
{
	return m_hasvideo ? S_OK : S_FALSE;
}

STDMETHODIMP GenericPlayer::Poke()
{
	m_lastactivity = clock();

	return S_OK;
}

// FilePlayer

FilePlayer::FilePlayer(LPCWSTR path, LPCWSTR mime, HRESULT& hr)
{
	m_graph = new CFGManagerPlayer(L"CFGManagerPlayer", NULL, GetSourceConfig(), GetDecoderConfig());

	CComQIPtr<IBaseFilter> pBF;

	hr = m_graph->AddSourceFilterMime(path, path, mime, &pBF);
	
	if(FAILED(hr)) return;

	hr = GetOutputs(pBF);

	if(SUCCEEDED(hr))
	{
		m_fn = path;

		LoadSubtitles(path);
	}

	// TODO: ask the user

	m_hash = HashFile(path);

	if(m_hash)
	{
		std::wstring str = Util::Format(L"Software\\%s\\Profile\\File\\%08x", m_cfgkey.c_str(), m_hash);

		CRegKey key;

		if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, str.c_str(), KEY_READ))
		{
			DWORD pos = 0;

			if(ERROR_SUCCESS == key.QueryDWORDValue(L"LastPosition", pos) && pos > 0)
			{
				Seek(100000i64 * pos);
			}
		}
	}
}

FilePlayer::~FilePlayer()
{
	if(m_graph && m_hash)
	{
		OAFilterState fs = State_Stopped;

		if(SUCCEEDED(GetState(fs)))
		{
			REFERENCE_TIME current = 0;
			REFERENCE_TIME start = 0;
			REFERENCE_TIME stop = 0;

			if(SUCCEEDED(GetCurrent(current)) && SUCCEEDED(GetAvailable(start, stop)))
			{
				std::wstring str = Util::Format(L"Software\\%s\\Profile\\File\\%08x", m_cfgkey.c_str(), m_hash);

				CRegKey key;

				if(ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, str.c_str()))
				{
					DWORD dw = current < stop - 10000000i64 * 10 ? (DWORD)(current / 100000) : 0;

					key.SetDWORDValue(L"LastPosition", dw);
				}
			}
		}
	}
}

DWORD FilePlayer::HashFile(LPCWSTR path)
{
	DWORD hash = 0;

	if(FILE* f = _wfopen(path, L"rb"))
	{
		int size = 2048;

		BYTE* buff = new BYTE[size];

		memset(buff, 0, size);

		fread(buff, size, 1, f);

		for(int i = 0; i < size / 4; i++)
		{
			hash ^= ((DWORD*)buff)[i];
		}

		_fseeki64(f, 0, SEEK_END);

		__int64 len = _ftelli64(f);

		if(len > size)
		{
			_fseeki64(f, -size, SEEK_END);

			fread(buff, size, 1, f);

			for(int i = 0; i < size / 4; i++)
			{
				hash ^= ((DWORD*)buff)[i];
			}
		}

		hash ^= len;

		fclose(f);
	}

	return hash;
}

STDMETHODIMP FilePlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IFilePlayer)
		QI(ISubtitleFilePlayer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IGenericPlayer

STDMETHODIMP FilePlayer::Stop()
{
	HRESULT hr = __super::Stop();

	if(FAILED(hr)) return hr;

	Seek(0);

	return S_OK;
}

// IFilePlayer

STDMETHODIMP_(LPCWSTR) FilePlayer::GetFileName()
{
	return m_fn.c_str();
}

// ISubtitleFilePlayer

STDMETHODIMP FilePlayer::AddSubtitle(LPCWSTR fn)
{
	return LoadSubtitle(fn) ? S_OK : E_FAIL;
}

// DVDPlayer

DVDPlayer::DVDPlayer(LPCWSTR path, HRESULT& hr)
{
	m_graph = new CFGManagerDVD(L"CFGManagerDVD", NULL, GetSourceConfig(), GetDecoderConfig());

	CComQIPtr<IBaseFilter> nav;

	hr = m_graph->AddSourceFilter(path, NULL, &nav);
	
	if(FAILED(hr)) return;

	m_control = nav;
	m_info = nav;

	GetOutputs(nav);

	hr = m_control && m_info ? S_OK : E_FAIL;
}

DVDPlayer::~DVDPlayer()
{
	m_graph->RemoveFromROT();
}

STDMETHODIMP DVDPlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IDVDPlayer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT DVDPlayer::InTitle()
{
	CheckPointer(m_info, E_UNEXPECTED);

	HRESULT hr;

	DVD_DOMAIN domain;

	hr = m_info->GetCurrentDomain(&domain);
	
	if(FAILED(hr)) return hr;

	return domain == DVD_DOMAIN_Title ? S_OK : S_FALSE;
}

// IGenericPlayer

STDMETHODIMP DVDPlayer::GetCurrent(REFERENCE_TIME& rt)
{
	CheckPointer(m_info, E_UNEXPECTED);

	if(S_OK != InTitle())
	{
		return E_UNEXPECTED;
	}

	HRESULT hr;

	DVD_PLAYBACK_LOCATION2 Location;
	
	hr = m_info->GetCurrentLocation(&Location);
	
	if(FAILED(hr)) return hr;

	rt = DirectShow::HMSF2RT(Location.TimeCode);

	return S_OK;
}

STDMETHODIMP DVDPlayer::GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CheckPointer(m_info, E_UNEXPECTED);

	if(S_OK != InTitle())
	{
		return E_UNEXPECTED;
	}

	HRESULT hr;

	DVD_HMSF_TIMECODE TotalTime;
	
	hr = m_info->GetTotalTitleTime(&TotalTime, NULL);
	
	if(FAILED(hr)) 
	{
		return hr;
	}
	
	start = 0;
	stop = DirectShow::HMSF2RT(TotalTime);

	return S_OK;
}

STDMETHODIMP DVDPlayer::Seek(REFERENCE_TIME rt)
{
	CheckPointer(m_control, E_UNEXPECTED);

	if(S_OK != InTitle())
	{
		return E_UNEXPECTED;
	}

	DVD_HMSF_TIMECODE tc = DirectShow::RT2HMSF(rt);
	
	return m_control->PlayAtTime(&tc, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
}

STDMETHODIMP DVDPlayer::Play()
{
	CheckPointer(m_graph, E_UNEXPECTED);
	CheckPointer(m_control, E_UNEXPECTED);

	m_control->PlayForwards(1.0f, DVD_CMD_FLAG_Block, NULL);
	m_control->Pause(FALSE);

	if(InTitle() != S_OK)
	{
		m_control->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
	}

	return __super::Play();
}

STDMETHODIMP DVDPlayer::Pause()
{
	CheckPointer(m_graph, E_UNEXPECTED);

	if(S_OK != InTitle())
	{
		return E_UNEXPECTED;
	}

	return __super::Pause();
}

STDMETHODIMP DVDPlayer::Stop()
{
	CheckPointer(m_graph, E_UNEXPECTED);
	CheckPointer(m_control, E_UNEXPECTED);

	return InTitle() == S_OK ? Menu(DVD_MENU_Root) : m_control->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
}

STDMETHODIMP DVDPlayer::Prev()
{
	CheckPointer(m_control, E_UNEXPECTED);
	CheckPointer(m_info, E_UNEXPECTED);

	if(S_OK != InTitle())
	{
		return E_UNEXPECTED;
	}

	Play();

	HRESULT hr;

	ULONG ulNumOfVolumes;
	ULONG ulVolume;
	DVD_DISC_SIDE Side;
	ULONG ulNumOfTitles = 0;
	DVD_PLAYBACK_LOCATION2 Location;
	ULONG ulNumOfChapters = 0;
	
	hr = m_info->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);
	
	if(FAILED(hr)) return hr;
	
	hr = m_info->GetCurrentLocation(&Location);
	
	if(FAILED(hr)) return hr;
	
	hr = m_info->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);
	
	if(FAILED(hr)) return hr;

	if(Location.ChapterNum == 1 && Location.TitleNum > 1)
	{
		hr = m_info->GetNumberOfChapters(Location.TitleNum - 1, &ulNumOfChapters);

		if(FAILED(hr)) return hr;

		hr = m_control->PlayChapterInTitle(Location.TitleNum - 1, ulNumOfChapters, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

		if(FAILED(hr)) return hr;
	}
	else
	{
		hr = m_control->PlayPrevChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
		
		if(FAILED(hr)) return hr;
	}

	return S_OK;
}

STDMETHODIMP DVDPlayer::Next()
{
	CheckPointer(m_control, E_UNEXPECTED);
	CheckPointer(m_info, E_UNEXPECTED);

	if(S_OK != InTitle())
	{
		return E_UNEXPECTED;
	}

	Play();

	HRESULT hr;

	ULONG ulNumOfVolumes;
	ULONG ulVolume;
	DVD_DISC_SIDE Side;
	ULONG ulNumOfTitles = 0;
	DVD_PLAYBACK_LOCATION2 Location;
	ULONG ulNumOfChapters = 0;
	
	hr = m_info->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);
	
	if(FAILED(hr)) return hr;

	hr = m_info->GetCurrentLocation(&Location);
	
	if(FAILED(hr)) return hr;

	hr = m_info->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

	if(FAILED(hr)) return hr;

	if(Location.ChapterNum == ulNumOfChapters && Location.TitleNum < ulNumOfTitles)
	{
		hr = m_control->PlayChapterInTitle(Location.TitleNum+1, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

		if(FAILED(hr)) return hr;
	}
	else
	{
		hr = m_control->PlayNextChapter(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);

		if(FAILED(hr)) return hr;
	}

	return S_OK;
}

STDMETHODIMP DVDPlayer::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	CheckPointer(m_control, E_UNEXPECTED);

	Vector2i p(LOWORD(lParam), HIWORD(lParam));

	if(message == WM_MOUSEMOVE)
	{
		m_control->SelectAtPosition(*(POINT*)&p);

		return S_OK;
	}

	if(message == WM_LBUTTONDOWN)
	{
		m_control->ActivateAtPosition(*(POINT*)&p);

		return S_OK;
	}

	// TODO: WM_USER+i

	if(message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
	{
		switch(wParam)
		{
		case VK_LEFT: m_control->SelectRelativeButton(DVD_Relative_Left); return S_OK;
		case VK_RIGHT: m_control->SelectRelativeButton(DVD_Relative_Right); return S_OK;
		case VK_UP: m_control->SelectRelativeButton(DVD_Relative_Upper); return S_OK;
		case VK_DOWN: m_control->SelectRelativeButton(DVD_Relative_Lower); return S_OK;
		case VK_RETURN: m_control->ActivateButton(); return S_OK;
		case VK_BACK: m_control->ReturnFromSubmenu(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); return S_OK;
		case VK_SPACE: Stop(); return S_OK;
		}
	}

	if(message == WM_APPCOMMAND)
	{
		switch(GET_APPCOMMAND_LPARAM(lParam))
		{
		case APPCOMMAND_BROWSER_BACKWARD: m_control->ReturnFromSubmenu(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); return S_OK;
		}
	}

	return S_FALSE;
}

// IDVDPlayer

STDMETHODIMP DVDPlayer::Menu(DVD_MENU_ID id)
{
	CheckPointer(m_control, E_UNEXPECTED);

	Play();

	return m_control->ShowMenu(id, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
}

STDMETHODIMP DVDPlayer::GetDVDDirectory(std::wstring& dir)
{
	CheckPointer(m_info, E_UNEXPECTED);

	WCHAR path[MAX_PATH+1];
	ULONG len = 0;

	if(FAILED(m_info->GetDVDDirectory(path, countof(path), &len)))
	{
		return E_FAIL;
	}

	dir = path;

	return S_OK;
}

// PlaylistPlayer

PlaylistPlayer::PlaylistPlayer(LPCWSTR path, HRESULT& hr)
{
	m_pos = m_files.end();

	m_graph = new CFGManagerPlayer(L"CFGManagerPlayer", NULL, GetSourceConfig(), GetDecoderConfig());

	hr = E_FAIL;

	std::list<std::wstring> rows;

	std::wstring url(path);

	if(url.find(L"://") != std::wstring::npos)
	{
		if(0) // url.find(L"homesys") == std::wstring::npos) // HACK
		{
			Util::Socket s;

			std::map<std::string, std::string> headers;

			if(s.HttpGet(url.c_str(), headers))
			{
				if(s.GetContentLength() > 0 && s.GetContentLength() < 65536)
				{
					std::vector<BYTE> buff;
		
					buff.resize(s.GetContentLength());
					
					if(s.ReadAll(&buff[0], buff.size()))
					{
						std::string s((const char*)&buff[0], buff.size());

						Util::Explode(std::wstring(s.begin(), s.end()), rows, '\n');
					}
				}
			}
		}
	}
	else 
	{
		if(FILE* f = _wfopen(path, L"r"))
		{
			TCHAR buff[1024];

			while(!feof(f) && fgetws(buff, countof(buff) - 1, f))
			{
				std::wstring s(buff);

				if(s.find(L"File") != 0)
				{
					break;
				}

				rows.push_back(s);
			}

			fclose(f);
		}
	}

	for(auto i = rows.begin(); i != rows.end(); i++)
	{
		std::list<std::wstring> sl;

		Util::Explode(*i, sl, '=', 2);

		if(sl.front().find(L"File") == 0)
		{
			m_files.push_back(sl.back());
		}
	}

	m_pos = m_files.begin(); 

	hr = Open(0);
}

PlaylistPlayer::~PlaylistPlayer()
{
}

STDMETHODIMP PlaylistPlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IFilePlayer)
		QI(IPlaylistPlayer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT PlaylistPlayer::Open(int step)
{
	if(m_pos == m_files.end() || m_files.empty())
	{
		return E_FAIL;
	}

	if(step > 0)
	{
		while(step-- > 0)
		{
			m_pos++;

			if(m_pos == m_files.end())
			{
				m_pos = m_files.begin();
			}
		}
	}
	else if(step < 0)
	{
		while(step++ < 0)
		{
			if(m_pos == m_files.begin())
			{
				m_pos = m_files.end();
			}

			m_pos--;
		}
	}

	HRESULT hr;

	Stop();

	m_outputs.clear();

	m_graph->Reset();

	std::wstring path = *m_pos;

	CComQIPtr<IBaseFilter> pBF;

	hr = m_graph->AddSourceFilter(path.c_str(), L"Source", &pBF);
	
	if(FAILED(hr)) return hr;

	if(FAILED(GetOutputs(pBF)))
	{
		return E_FAIL;
	}

	LoadSubtitles(path.c_str());

	return S_OK;
}

// IGenericPlayer

STDMETHODIMP PlaylistPlayer::Prev()
{
	HRESULT hr;
	float volume = 0;
	GetVolume(volume);
	hr = Open(-1);
	if(FAILED(hr)) return hr;
	hr = ReRender();
	if(FAILED(hr)) return hr;
	Seek(0);
	SetVolume(volume);
	Play();
	return S_OK;
}

STDMETHODIMP PlaylistPlayer::Next()
{
	HRESULT hr;
	float volume = 0;
	GetVolume(volume);
	hr = Open(+1);
	if(FAILED(hr)) return hr;
	hr = ReRender();
	if(FAILED(hr)) return hr;
	Seek(0);
	SetVolume(volume);
	Play();
	return S_OK;
}

// IPlaylistPlayer

STDMETHODIMP_(LPCWSTR) PlaylistPlayer::GetFileName()
{
	if(m_pos == m_files.end() || m_files.empty())
	{
		return L"";
	}

	return m_pos->c_str();
}

