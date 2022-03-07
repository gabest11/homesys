#pragma once

#include "stdafx.h"
#include "GenericTuner.h"
#include "AnalogTuner.h"
#include "DVBTTuner.h"
#include "DVBSTuner.h"
#include "DVBCTuner.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "DSMSink.h"
#include "MpaDecFilter.h"
#include "ImageGrabber.h"
#include "NullRenderer.h"
#include "ColorConverter.h"

GenericTuner::GenericTuner()
	: m_state(TunerState::None)
	, m_running(false)
	, m_start(0)
	, m_timeout(0)
	, m_exit(TRUE)
{
	Close();
}

GenericTuner::~GenericTuner()
{
	Close();
}

bool GenericTuner::Open(const TunerDesc& desc, ISmartCardServer* scs)
{
	Close();

	m_desc = desc;
	m_scs = scs;

	CComPtr<IGraphBuilder2> graph;

	graph = new CFGManagerCustom(_T(""), NULL);

	if(!RenderTuner(graph))
	{
		return false;
	}

	if(m_src == NULL) m_src = graph;
	if(m_dst == NULL) m_dst = graph;

	m_exit.Reset();

	CAMThread::Create();

	// m_src->AddToROT();

	if(m_src != m_dst)
	{
		CComQIPtr<IMediaControl>(m_src)->Run();

		// m_dst->AddToROT();
	}

	return true;
}	

void GenericTuner::Close()
{
	HRESULT hr;

	if(m_dst)
	{
		CComQIPtr<IMediaControl> pMC(m_dst);

		hr = pMC->Stop();

		ASSERT(SUCCEEDED(hr));

		OAFilterState fs;

		hr = pMC->GetState(3000, &fs);

		ASSERT(SUCCEEDED(hr));

		m_dst->RemoveFromROT();
	}

	if(m_src)
	{
		CComQIPtr<IMediaControl> pMC(m_src);

		hr = pMC->Stop();

		ASSERT(SUCCEEDED(hr));

		OAFilterState fs;

		hr = pMC->GetState(3000, &fs);

		ASSERT(SUCCEEDED(hr));

		m_src->RemoveFromROT();
	}

	m_exit.Set();

	CAMThread::Close();

	ASSERT(m_hThread == NULL);

	m_src = NULL;
	m_dst = NULL;
	m_scs = NULL;

	m_mdsm.clear();

	m_running = false;
}

bool GenericTuner::Render(HWND hWnd)
{
	HRESULT hr;

	Stop();

	if(m_dst == NULL)
	{
		return false;
	}

	std::list<IPin*> video;
	std::list<IPin*> audio;
	IPin* vbi = NULL;

	if(GetOutput(video, audio, vbi))
	{
		if(hWnd != NULL)
		{
/**/
			CComPtr<IBaseFilter> pBF;

			if(SUCCEEDED(hr = pBF.CoCreateInstance(CLSID_VideoMixingRenderer9)))
			{
				m_dst->AddFilter(pBF, L"VMR9");
			}

			if(!video.empty())
			{
				hr = m_dst->Render(video.front());
			
				_tprintf(_T("Render: video => %08x %s\n"), hr, CMediaTypeEx(video.front()).ToString().c_str());
			}

			if(!audio.empty())
			{
				hr = m_dst->Render(audio.front());

				_tprintf(_T("Render: audio => %08x %s\n"), hr, CMediaTypeEx(audio.front()).ToString().c_str());
			}

			ForeachInterface<IVideoWindow>(m_dst, [&hWnd] (IBaseFilter* pBF, IVideoWindow* pVW) -> HRESULT
			{
				pVW->put_Owner((OAHWND)hWnd);
				pVW->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
				// pVW->put_MessageDrain((OAHWND)hWnd);

				for(HWND hWndChild = GetWindow(hWnd, GW_CHILD); hWndChild; hWndChild = GetNextWindow(hWndChild, GW_HWNDNEXT))
				{
					EnableWindow(hWndChild, FALSE);
				}

				CComQIPtr<IVMRAspectRatioControl> pAR = pBF;

				if(pAR != NULL)
				{
					pAR->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
				}

				CComQIPtr<IVMRAspectRatioControl9> pAR9 = pBF;

				if(pAR9 != NULL)
				{
					pAR9->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
				}


				CComQIPtr<IVMRFilterConfig9> pConfig = pBF;

				if(pConfig != NULL) 
				{
					pConfig->SetNumberOfStreams(1);
				}

				return S_CONTINUE;
			});

			if(vbi != NULL)
			{
				hr = m_dst->Render(vbi);

				_tprintf(_T("Render: vbi => %08x %s\n"), hr, CMediaTypeEx(vbi).ToString().c_str());
			}

			MoveVideoRect();
		}
		else
		{
			CComPtr<IBaseFilter> nvr = new CNullVideoRenderer(NULL, &hr);

			if(!video.empty())
			{
				hr = m_dst->AddFilter(nvr, L"Null Video Renderer");

				hr = m_dst->ConnectFilterDirect(video.front(), nvr, NULL);
			}

			CComPtr<IBaseFilter> nar = new CNullAudioRenderer(NULL, &hr);

			if(!audio.empty())
			{
				hr = m_dst->AddFilter(nar, L"Null Audio Renderer");

				hr = m_dst->ConnectFilterDirect(audio.front(), nar, NULL);
			}
		}
	}

	m_dst->Clean();
	m_dst->Dump();

	Run();

	m_state = TunerState::None;

	return true;
}

bool GenericTuner::RenderAsf(IWMWriterAdvanced2** ppWMWA2, int profile)
{
	HRESULT hr;

	Stop();

	std::list<IPin*> video;
	std::list<IPin*> audio;
	IPin* vbi = NULL;

	GetOutput(video, audio, vbi);

	CComQIPtr<IBaseFilter> pBF;

	if(FAILED(pBF.CoCreateInstance(CLSID_WMAsfWriter)))
	{
		return false;
	}

	if(FAILED(m_dst->AddFilter(pBF, L"WMAsfWriter")))
	{
		return false;
	}

	hr = m_dst->ConnectFilter(video.front(), pBF);

	if(FAILED(hr))
	{
		return false;
	}

	hr = m_dst->ConnectFilter(audio.front(), pBF);

	if(FAILED(hr)) 
	{
		return false;
	}

	if(vbi)
	{
		CComPtr<IBaseFilter> pBF = new CImageGrabberFilter(NULL, &hr);

		hr = m_dst->AddFilter(pBF, L"Image Grabber");

		hr = m_dst->ConnectDirect(vbi, DirectShow::GetFirstPin(pBF), NULL);
	}

	m_dst->Clean();
	m_dst->Dump();

	if(CComQIPtr<IFileSinkFilter2> pFSF2 = pBF)
	{
		hr = pFSF2->SetMode(AM_FILE_OVERWRITE);
		hr = pFSF2->SetFileName(L"c:\\temp.asf", NULL);
	}

	if(CComQIPtr<IConfigAsfWriter> pConfig = pBF)
	{
		CComPtr<IWMProfileManager> pPM;

		hr = WMCreateProfileManager(&pPM);

		if(FAILED(hr))
		{
			return false;
		}

		std::wstring profilepath;

		if(profile < 0)
		{
			profile = 0;
		}

		// if(profile >= 0)
		{
			wchar_t buff[MAX_PATH];

			GetModuleFileName(NULL, buff, MAX_PATH);
			PathRemoveFileSpec(buff);
			PathAppend(buff, L"Profiles");
			CreateDirectory(buff, NULL);
			PathAppend(buff, Util::Format(L"%d.prx", profile).c_str());

			profilepath = buff;
		}

		if(profilepath.empty() || !GetFileAttributes(profilepath.c_str()))
		{
			DWORD err = GetLastError();

			profilepath = L"c:\\bitklub-profile1.prx";
		}

		CComPtr<IWMProfile> pProfile;

		wchar_t* buff = NULL;

		if(FILE* fp = _wfopen(profilepath.c_str(), L"r"))
		{
			fseek(fp, 0, SEEK_END);
			size_t len = ftell(fp);
			buff = new wchar_t[len];
			fseek(fp, 0, SEEK_SET);
			fread(buff, 1, len, fp);
			fclose(fp);
		}
		else
		{
			_tprintf(_T("Cannot find profile: %s\n"), profilepath.c_str());
		}

		if(buff)
		{
			hr = pPM->LoadProfileByData(buff, &pProfile);

			delete [] buff;
		}
		else
		{
			hr = E_FAIL;
		}

		if(FAILED(hr))
		{
			_tprintf(_T("LoadProfileByData FAILED (%08x)\n"), hr);

			hr = pPM->LoadProfileByID(WMProfile_V80_384PALVideo, &pProfile);

			_tprintf(_T("Loading profile WMProfile_V80_384PALVideo ... (%08x)\n"), hr);

			profilepath = _T("WMProfile_V80_384PALVideo");
		}

		if(FAILED(hr))
		{
			_tprintf(_T("LoadProfileByData FAILED (%08x)\n"), hr);

			hr = pPM->LoadProfileByID(WMProfile_V40_512Video, &pProfile);

			_tprintf(_T("Loading profile WMProfile_V40_512Video ... (%08x)\n"), hr);

			profilepath = _T("WMProfile_V40_512Video");
		}
		
		hr = pConfig->ConfigureFilterUsingProfile(pProfile);

		if(FAILED(hr))
		{
			_tprintf(_T("ConfigureFilterUsingProfile FAILED (%08x)\n"), hr);
			
			return false;
		}

		_tprintf(_T("Profile loaded (%s)\n"), profilepath.c_str());
	}

	if(CComQIPtr<IServiceProvider> pProvider = pBF)
	{
		CComPtr<IWMWriterAdvanced2> pWMWA2;

		hr = pProvider->QueryService(IID_IWMWriterAdvanced2, IID_IWMWriterAdvanced2, (void**)&pWMWA2);

		if(FAILED(hr))
		{
			return false;
		}

		pWMWA2->SetLiveSource(TRUE);

		DWORD count = 0;
		
		pWMWA2->GetSinkCount(&count);

		for(DWORD i = 0; i < count; i++)
		{
			CComPtr<IWMWriterSink> pSink;

			hr = pWMWA2->GetSink(i, &pSink);
			hr = pWMWA2->RemoveSink(pSink);
		}

		(*ppWMWA2 = pWMWA2.Detach());

		return true;
	}

	return false;
}

bool GenericTuner::RenderMDSM(LPCWSTR path, LPCWSTR id)
{
	HRESULT hr;

	m_mdsm.clear();

	if(m_dst == NULL)
	{
		return false;
	}

	std::list<IPin*> video;
	std::list<IPin*> audio;
	IPin* vbi = NULL;

	if(!GetOutput(video, audio, vbi))
	{
		return false;
	}

	CComPtr<IBaseFilter> sink = new CDSMSinkFilter(NULL, &hr);

	hr = m_dst->AddFilter(sink, L"DSM Sink");

	if(FAILED(hr)) 
	{
		return false;
	}

	for(auto i = video.begin(); i != video.end(); i++)
	{
		hr = m_dst->ConnectFilterDirect(*i, sink, NULL);

		if(FAILED(hr)) 
		{
			return false;
		}
	}

	for(auto i = audio.begin(); i != audio.end(); i++)
	{
		hr = m_dst->ConnectFilterDirect(*i, sink, NULL);

		if(FAILED(hr))
		{
			return false;
		}
	}

	if(vbi != NULL)
	{
		hr = m_dst->ConnectFilterDirect(vbi, sink, NULL);

		if(FAILED(hr))
		{
			return false;
		}
	}

	hr = E_FAIL;

	if(CComQIPtr<IFileSinkFilter> pFSF = sink)
	{
		for(int i = 0; i < 3; i++)
		{
			std::wstring s = Util::Format(L"%s%s.%d.mdsm", path, id, i);

			DeleteFile(s.c_str());

			if(FAILED(hr))
			{
				hr = pFSF->SetFileName(s.c_str(), NULL);

				if(SUCCEEDED(hr))
				{
					m_mdsm = s;
				}
			}
		}
	}

	m_dst->Clean();
	m_dst->Dump();

	return SUCCEEDED(hr);
}

bool GenericTuner::Stream(int port, int preset)
{
	CComPtr<IWMWriterAdvanced2> pWMWA2;

	if(!RenderAsf(&pWMWA2, preset))
	{
		return false;
	}

	HRESULT hr = E_FAIL;

	CComPtr<IWMWriterNetworkSink> pSink;

	hr = WMCreateWriterNetworkSink(&pSink);

	if(FAILED(hr))
	{
		return false;
	}

	DWORD dwPort = port;

	hr = pSink->Open(&dwPort);
	
	if(FAILED(hr))
	{
		return false;
	}

	hr = pWMWA2->AddSink(pSink);

	if(FAILED(hr))
	{
		return false;
	}

	Run();

	m_state = TunerState::Streaming;

	return SUCCEEDED(hr);
}

bool GenericTuner::Record(LPCWSTR path, int preset, int timeout)
{
	CComPtr<IWMWriterAdvanced2> pWMWA2;

	if(!RenderAsf(&pWMWA2, preset))
	{
		return false;
	}

	HRESULT hr = E_FAIL;

	CComPtr<IWMWriterFileSink> pSink;

	hr = WMCreateWriterFileSink(&pSink);

	if(FAILED(hr))
	{
		return false;
	}

	hr = pSink->Open(path);

	if(FAILED(hr))
	{
		return false;
	}

	hr = pWMWA2->AddSink(pSink);

	if(FAILED(hr))
	{
		return false;
	}

	m_start = clock();
	m_timeout = timeout;

	Run();

	m_state = TunerState::Recording;

	return SUCCEEDED(hr);
}

bool GenericTuner::Timeshift(LPCWSTR path, LPCWSTR id)
{
	Stop();

	if(!RenderMDSM(path, id))
	{
		return false;
	}

	Run();

	m_state = TunerState::Timeshifting;

	return true;
}

bool GenericTuner::Timeshift(LPCWSTR path, LPCWSTR id, const GenericPresetParam* p)
{
	Tune(p);

	return Timeshift(path, id);
}

bool GenericTuner::StartRecording(LPCWSTR path)
{
	HRESULT hr;

	hr = ForeachInterface<IDSMSinkFilter>(m_dst, [&] (IBaseFilter* pBF, IDSMSinkFilter* sink) -> HRESULT
	{
		return sink->StartRecording(path);
	});

	return hr == S_OK;
}

void GenericTuner::StopRecording()
{
	ForeachInterface<IDSMSinkFilter>(m_dst, [&] (IBaseFilter* pBF, IDSMSinkFilter* sink) -> HRESULT
	{
		sink->StopRecording();

		return S_CONTINUE;
	});
}

void GenericTuner::Run()
{
	if(m_dst)
	{
		m_dst->SetDefaultSyncSource();

		ForeachInterface<IReferenceClock>(m_dst, [this] (IBaseFilter* pBF, IReferenceClock* pClock) -> HRESULT
		{
			if(DirectShow::GetCLSID(pBF) != CLSID_MPEG2Demultiplexer)
			{
				CComQIPtr<IMediaFilter>(m_dst)->SetSyncSource(pClock);

				return S_OK;
			}

			return S_CONTINUE;
		});

		CComQIPtr<IMediaControl>(m_dst)->Run();

		m_running = true;
	}
}

void GenericTuner::Stop()
{
	if(m_dst == NULL)
	{
		return;
	}

	CComQIPtr<IMediaControl>(m_dst)->Stop();

	ForeachInterface<IServiceProvider>(m_dst, [] (IBaseFilter* pBF, IServiceProvider* pSP) -> HRESULT
	{
		HRESULT hr;

		CComPtr<IWMWriterAdvanced2> pWMWA2;

		if(SUCCEEDED(pSP->QueryService(IID_IWMWriterAdvanced2, IID_IWMWriterAdvanced2, (void**)&pWMWA2)))
		{
			DWORD count = 0;

			pWMWA2->GetSinkCount(&count);

			for(DWORD i = 0; i < count; i++)
			{
				CComPtr<IWMWriterSink> pSink;
				
				hr = pWMWA2->GetSink(i, &pSink);
				
				CComQIPtr<IWMWriterNetworkSink> pWNS = pSink;

				if(pWNS != NULL)
				{
					hr = pWNS->Disconnect();
					hr = pWNS->Close();
				}

				hr = pWMWA2->RemoveSink(pSink);
			}
		}

		return S_CONTINUE;
	});

	m_start = 0;
	m_timeout = 0;

	m_state = TunerState::None;
	m_running = false;

	m_mdsm.clear();
}

void GenericTuner::OnIdle()
{
	if(m_timeout > 0 && m_state == TunerState::Recording)
	{
		if(m_start + m_timeout <= clock())
		{
			Stop();
		}
	}
}

void GenericTuner::MoveVideoRect()
{
	ForeachInterface<IVideoWindow>(m_dst, [] (IBaseFilter* pBF, IVideoWindow* pVW) -> HRESULT
	{
		OAHWND hWnd;

		if(SUCCEEDED(pVW->get_Owner(&hWnd)) && hWnd)
		{
			RECT r;
			GetClientRect((HWND)hWnd, &r);
			OffsetRect(&r, -r.left, -r.top);
			pVW->SetWindowPosition(r.left, r.top, r.right, r.bottom);
		}

		return S_CONTINUE;
	});
}

DWORD GenericTuner::ThreadProc()
{
	HRESULT hr;

	CComQIPtr<IMediaEvent> me[2];

	me[0] = CComQIPtr<IMediaEvent>(m_src);
	me[1] = CComQIPtr<IMediaEvent>(m_dst);

	HANDLE handles[3] = {0, 0, m_exit};
	
	me[0]->GetEventHandle((OAEVENT*)&handles[0]);
	me[1]->GetEventHandle((OAEVENT*)&handles[1]);

	while(m_hThread != NULL)
	{
		long lEventCode;
		LONG_PTR lParam[2];

		switch(DWORD i = WaitForMultipleObjects(countof(handles), handles, FALSE, 100))
		{
		case WAIT_OBJECT_0 + 0: 
		case WAIT_OBJECT_0 + 1: 

			lParam[0] = lParam[1] = 0;

			hr = me[i - WAIT_OBJECT_0]->GetEvent(&lEventCode, &lParam[0], &lParam[1], 100);

			if(SUCCEEDED(hr))
			{
				OnEvent(lEventCode, lParam);

				hr = me[i - WAIT_OBJECT_0]->FreeEventParams(lEventCode, lParam[0], lParam[1]);
			}

			break;

		case WAIT_TIMEOUT:
			OnIdle();
			break;

		default: 
			m_hThread = NULL;
			break;
		}
	}				

	return 0;
}

struct EnumTunerParam
{
	bool digital;
	bool compressed;
	TunerDesc desc;
	std::list<TunerDesc::Type> types;
};

static DWORD WINAPI EnumTunerThreadProc(void* param)
{
	CoInitialize(0);

	EnumTunerParam* p = (EnumTunerParam*)param;

	if(p->digital)
	{
		if(DVBTTuner().Open(p->desc, NULL))
		{
			p->types.push_back(TunerDesc::DVBT);
		}
		
		if(DVBCTuner().Open(p->desc, NULL))
		{
			p->types.push_back(TunerDesc::DVBC);
		}

		if(p->types.empty())
		{
			if(DVBSTuner().Open(p->desc, NULL))
			{
				p->types.push_back(TunerDesc::DVBS);
			}
		}
	}
	else
	{
		if(AnalogTuner(p->compressed).Open(p->desc, NULL))
		{
			p->types.push_back(TunerDesc::Analog);
		}
	}

	CoUninitialize();

	return 0;
}

void GenericTuner::EnumTuners(std::list<TunerDesc>& tds, bool compressed, bool tsreader)
{
	std::vector<HANDLE> threads;
	std::list<EnumTunerParam*> params;

	static struct {const CLSID* clsid; bool digital;} s_cats[] = 
	{
		{&CLSID_VideoInputDeviceCategory, false},
		{&KSCATEGORY_BDA_NETWORK_TUNER, true},
	};

	for(int i = 0; i < countof(s_cats); i++)
	{
		bool digital = s_cats[i].digital;

		Foreach(*s_cats[i].clsid, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
		{
			EnumTunerParam* p = new EnumTunerParam();

			p->digital = digital;
			p->compressed = compressed;
			p->desc.id = id;
			p->desc.name = name;
			p->desc.type = TunerDesc::None;

			params.push_back(p);

			DWORD tid = 0;
	
			threads.push_back(CreateThread(NULL, 0, EnumTunerThreadProc, p, 0, &tid));

			return S_CONTINUE;
		});
	}

	if(!threads.empty()) 
	{
		WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);
	}

	for(auto i = params.begin(); i != params.end(); i++)
	{
		EnumTunerParam* p = *i;

		int index = 0;

		for(auto j = p->types.begin(); j != p->types.end(); j++, index++)
		{
			TunerDesc desc = p->desc;

			std::wstring s = desc.name;

			switch(*j)
			{
			case TunerDesc::Analog: s += L" (Analog)"; break;
			case TunerDesc::DVBT: s += L" (DVB-T)"; break;
			case TunerDesc::DVBS: s += L" (DVB-S)"; break;
			case TunerDesc::DVBC: s += L" (DVB-C)"; break;
			case TunerDesc::DVBF: s += L" (DVB-F)"; break;
			default: s += L" (???)"; break;
			}

			desc.type = *j;

			if(p->types.size() > 1)
			{
				desc.id += Util::Format(L"$homesys$%d", index);
				desc.name = s;
			}

			wprintf(L"%s\n", s.c_str());

			tds.push_back(desc);
		}
	}

	if(tsreader)
	{
		TunerDesc td;

		td.type = TunerDesc::DVBF;
		td.name = L"Transport Stream Reader";
		td.id = td.name;

		tds.push_back(td);
	}
/*
	static struct {const CLSID* clsid; bool digital;} s_cats[] = 
	{
		{&CLSID_VideoInputDeviceCategory, false},
		{&KSCATEGORY_BDA_NETWORK_TUNER, true},
	};

	for(int i = 0; i < countof(s_cats); i++)
	{
		bool digital = s_cats[i].digital;

		Foreach(*s_cats[i].clsid, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
		{
			TunerDesc td;

			td.id = id;
			td.name = name;

			_tprintf(_T("Tuner: %s\n"), td.name.c_str());

			if(digital)
			{
				if(DVBTTuner().Open(td, NULL))
				{
					td.type = TunerDesc::DVBT;
					
					_tprintf(_T("DVB-T\n"));
				}
				else if(DVBSTuner().Open(td, NULL))
				{
					td.type = TunerDesc::DVBS;

					_tprintf(_T("DVB-S\n"));
				}
				else if(DVBCTuner().Open(td, NULL))
				{
					td.type = TunerDesc::DVBC;

					_tprintf(_T("DVB-C\n"));
				}
			}
			else
			{
				if(AnalogTuner(compressed).Open(td, NULL))
				{
					td.type = TunerDesc::Analog;

					_tprintf(_T("Analog\n"));
				}
			}

			if(td.type != TunerDesc::None)
			{
				tds.push_back(td);
			}
			else
			{
				_tprintf(_T("???\n"));
			}

			return S_CONTINUE;
		});
	}

	if(tsreader)
	{
		TunerDesc td;

		td.type = TunerDesc::DVBF;
		td.name = L"Transport Stream Reader";
		td.id = td.name;

		tds.push_back(td);
	}
*/
}

void GenericTuner::EnumTunersNoType(std::list<TunerDesc>& tds)
{
	std::vector<HANDLE> threads;
	std::list<EnumTunerParam*> params;

	static struct {const CLSID* clsid; bool digital;} s_cats[] = 
	{
		{&CLSID_VideoInputDeviceCategory, false},
		{&KSCATEGORY_BDA_NETWORK_TUNER, true},
	};

	for(int i = 0; i < countof(s_cats); i++)
	{
		Foreach(*s_cats[i].clsid, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
		{
			TunerDesc td;

			td.type = TunerDesc::None;
			td.name = name;
			td.id = id;

			tds.push_back(td);

			return S_CONTINUE;
		});
	}
}

bool GenericTuner::IsTunerConnectorType(PhysicalConnectorType type)
{
	return type == PhysConn_Video_Tuner;
}

std::wstring GenericTuner::ToString(PhysicalConnectorType type)
{
	std::wstring name = Util::Format(L"Unknown %d", type);

	switch(type)
	{
	case PhysConn_Video_Tuner: name = L"Tuner"; break;
	case PhysConn_Video_Composite: name = L"Composite"; break;
	case PhysConn_Video_SVideo: name = L"SVideo"; break;
	case PhysConn_Video_RGB: name = L"RGB"; break;
	case PhysConn_Video_YRYBY: name = L"YRYBY"; break;
	case PhysConn_Video_SerialDigital: name = L"SerialDigital"; break;
	case PhysConn_Video_ParallelDigital: name = L"ParallelDigital"; break;
	case PhysConn_Video_SCSI: name = L"SCSI"; break;
	case PhysConn_Video_AUX: name = L"AUX"; break;
	case PhysConn_Video_1394: name = L"1394"; break;
	case PhysConn_Video_USB: name = L"USB"; break;
	case PhysConn_Video_VideoDecoder: name = L"VideoDecoder"; break;
	case PhysConn_Video_VideoEncoder: name = L"VideoEncoder"; break;
	case PhysConn_Video_SCART: name = L"SCART"; break;
	}

	return name;
}
