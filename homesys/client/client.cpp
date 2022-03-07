// client.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "client.h"
#include "HomesysWindow.h"
#include "../../DirectShow/DSMMuxer.h"
#include "../../DirectShow/MatroskaMuxer.h"
#include "../../DirectShow/mp4mux/MuxFilter.h"

Environment* g_env = NULL;

static void SendCommandLine(HWND hWnd, const std::list<std::wstring>& cmdln)
{
	if(!cmdln.empty())
	{
		int size = sizeof(DWORD);

		for(auto i = cmdln.begin(); i != cmdln.end(); i++)
		{
			size += (i->size() + 1) * sizeof(WCHAR);
		}

		std::vector<BYTE> buff(size);

		BYTE* p = buff.data();

		*(DWORD*)p = cmdln.size(); 

		p += sizeof(DWORD);

		for(auto i = cmdln.begin(); i != cmdln.end(); i++)
		{
			wcscpy((WCHAR*)p, i->c_str());

			p += (i->size() + 1) * sizeof(WCHAR);
		}

		COPYDATASTRUCT cds;

		cds.dwData = 0x6ABE51;
		cds.cbData = size;
		cds.lpData = (void*)(BYTE*)buff.data();

		SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	}
}

static void InitDXUI(ResourceLoader* loader)
{
}

static int Remux(const std::wstring& src, const std::wstring& dst, IBaseFilter* muxer)
{
	HRESULT hr;

	CComPtr<IGraphBuilder2> graph = new CFGManagerCustom(L"CFGManagerCustom", NULL);

	CComPtr<IBaseFilter> source;

	if(FAILED(hr = graph->AddSourceFilter(src.c_str(), L"Source", &source))) 
	{
		return hr;
	}

	if(FAILED(hr = graph->AddFilter(muxer, L"Muxer")))
	{
		return hr;
	}

	int n = 0;
	int m = 0;

	Foreach(source, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		n++;

		if(SUCCEEDED(graph->ConnectFilter(pin, muxer)))
		{
			m++;
		}

		return S_CONTINUE;
	});

	printf("pins connected: %d out of %d\n", n, m);

	CComPtr<IBaseFilter> writer;

	if(FAILED(hr = writer.CoCreateInstance(CLSID_FileWriter)))
	{
		return hr;
	}

	if(FAILED(hr = graph->AddFilter(writer, L"Writer")))
	{
		return hr;
	}

	if(FAILED(hr = graph->ConnectFilterDirect(muxer, writer, NULL)))
	{
		return hr;
	}

	if(FAILED(hr = CComQIPtr<IFileSinkFilter2>(writer)->SetFileName(dst.c_str(), NULL)))
	{
		return hr;
	}

	CComQIPtr<IMediaControl>(graph)->Run();

	long code;
	LONG_PTR p1, p2;

	while(1)
	{
		hr = CComQIPtr<IMediaEvent>(graph)->GetEvent(&code, &p1, &p2, 1000);

		REFERENCE_TIME rt = 0;
		
		CComQIPtr<IMediaSeeking>(graph)->GetCurrentPosition(&rt);
		
		printf("pos = %I64d\n", rt / 10000);

		if(::GetAsyncKeyState(VK_CONTROL) & 0x8000) break;

		if(hr == E_ABORT) continue;

		if(FAILED(hr)) break;

		CComQIPtr<IMediaEvent>(graph)->FreeEventParams(code, p1, p2);

		if(code == EC_COMPLETE || code == EC_ERRORABORT || code == EC_STATE_CHANGE && (FILTER_STATE)p1 != State_Running)
		{
			break;
		}
	}

	CComQIPtr<IMediaControl>(graph)->Stop();

	return S_OK;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	if(FAILED(OleInitialize(0))) // CoInitializeEx(0, COINIT_MULTITHREADED)))
	{
		MessageBox(NULL, L"OleInitialize failed!", L"Homesys", MB_OK);

		return -1;
	}

	CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_SECURE_REFS, 0);

	bool newinst = false;

	std::list<std::wstring> cmdln;

	for(int i = 1; i < __argc; i++)
	{
		std::wstring s = Util::Trim(__wargv[i], L" \"");

		if(!s.empty() && s[0] != '/' && s[0] != '-' && s.find(L":") == std::wstring::npos)
		{
			WCHAR buff[MAX_PATH];

			if(GetFullPathName(s.c_str(), MAX_PATH, buff, NULL) && PathFileExists(buff))
			{
				s = buff;
			}
		}
		else
		{
			if(s == L"/new")
			{
				newinst = true;
			}
		}

		cmdln.push_back(s);
	}

	static LPCWSTR HOMESYS_WND_CLASS = L"HomesysWndClass";

	HANDLE hMutexOneInstance = CreateMutex(NULL, TRUE, HOMESYS_WND_CLASS);

#ifdef NDEBUG

	if(!newinst && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if(HWND hWnd = FindWindow(HOMESYS_WND_CLASS, NULL))
		{
			SetForegroundWindow(hWnd);

			if(IsIconic(hWnd))
			{
				ShowWindow(hWnd, SW_RESTORE);
			}

			SendCommandLine(hWnd, cmdln);
		}

		return 0;
	}

#endif

	static CComModule _Module;

	_pAtlModule = &_Module;

	// gdiplus

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
   
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// console

	Util::Console* console = NULL;

	DWORD flags = 0;

	for(int i = 1; i < __argc; i++)
	{
		if(wcscmp(__wargv[i], L"/debug") == 0 && console == NULL)
		{
			console = new Util::Console(L"Homesys", true);
		}
		else if(wcscmp(__wargv[i], L"/perfhud") == 0)
		{
			flags |= 1;
		}
		else if(wcscmp(__wargv[i], L"/flipstat") == 0)
		{
			flags |= 2;
		}
		else if(wcscmp(__wargv[i], L"/tv") == 0)
		{
			flags |= 4;
		}
		else if(wcscmp(__wargv[i], L"/dsm") == 0 && i < __argc - 2)
		{
			std::wstring src = __wargv[++i];
			std::wstring dst = __wargv[++i];

			HRESULT hr;
			
			CComPtr<IBaseFilter> muxer;

			muxer = new CDSMMuxerFilter(NULL, &hr);

			Remux(src, dst, muxer);
		}
		else if(wcscmp(__wargv[i], L"/mkv") == 0 && i < __argc - 2)
		{
			std::wstring src = __wargv[++i];
			std::wstring dst = __wargv[++i];

			HRESULT hr;
			
			CComPtr<IBaseFilter> muxer;

			muxer = new CMatroskaMuxerFilter(NULL, &hr);

			Remux(src, dst, muxer);
		}
		else if(wcscmp(__wargv[i], L"/mp4") == 0 && i < __argc - 2)
		{
			std::wstring src = __wargv[++i];
			std::wstring dst = __wargv[++i];

			HRESULT hr;
			
			CComPtr<IBaseFilter> muxer;

			muxer = new Mpeg4Mux(NULL, &hr);

			Remux(src, dst, muxer);
		}
	}

	g_env = new Environment(hInstance);

	LoaderThread* loader = new LoaderThread();

	HomesysWindow* wnd = new HomesysWindow(loader, flags);

	wnd->Create(L"Homesys", 480 * 16 / 9, 480, HOMESYS_WND_CLASS);

	SendCommandLine((HWND)wnd->GetHandle(), cmdln);

	MSG msg;

	wnd->Show();

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	delete wnd;

	delete loader;

	delete g_env;

	delete console;

	Gdiplus::GdiplusShutdown(gdiplusToken);

	OleUninitialize(); // CoUninitialize();

	CloseHandle(hMutexOneInstance);

	return 0;
}
