#include "stdafx.h"
#include "Thread.h"

namespace DXUI
{
	Thread::Thread(bool initcom)
		: m_initcom(initcom)
		, m_hThread(0)
		, m_hThreadOld(0)
		, m_ThreadId(0)
		, m_hEventQueueReady(NULL)
		, m_nTimer(0)
		, m_nTimerEvent(0)
	{
		m_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);

		TimerPeriod.Set(&Thread::SetTimerPeriod, this);

		InitEvent.Add0([&] (Control* c) {return true;});
	}

	Thread::~Thread()
	{
		Join();

		CloseHandle(m_hExit);
	}

	HANDLE Thread::Create()
	{
		Join();

		ResetEvent(m_hExit);

		m_hEventQueueReady = CreateEvent(NULL, FALSE, FALSE, NULL);

		m_ThreadId = 0;
		m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);

		WaitForSingleObject(m_hEventQueueReady, 30000);
		CloseHandle(m_hEventQueueReady);
		m_hEventQueueReady = NULL;

		return m_hThread;
	}

	DWORD WINAPI Thread::StaticThreadProc(LPVOID lpParam)
	{
		Thread* t = (Thread*)lpParam;

		HRESULT hr;

		if(t->m_initcom) 
		{
			hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);
		}

		DWORD ret = ((Thread*)lpParam)->ThreadProc();
		
		if(t->m_initcom) 
		{
			if(SUCCEEDED(hr)) ::CoUninitialize();
		}

		return ret;
	}

	DWORD Thread::ThreadProc()
	{
		MSG msg;
		
		PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
		
		SetEvent(m_hEventQueueReady);

		if(InitEvent(NULL))
		{
			m_nTimerEvent = 0;

			if(TimerPeriod > 0)
			{
				FireTimer();

				m_nTimer = SetTimer(NULL, 0, TimerPeriod, NULL);
			}

			while((int)GetMessage(&msg, NULL, 0, 0) > 0)
			{
				if(msg.message == WM_TIMER && msg.wParam == m_nTimer)
				{
					FireTimer();
				}
				else
				{
					MessageEvent(NULL, msg);
				}

				DispatchMessage(&msg);
			}
		}

		ExitEvent(NULL);

		m_hThread = NULL;
		m_hThreadOld = NULL;

		return 0;
	}

	BOOL Thread::Post(UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		return ::PostThreadMessage(m_ThreadId, Msg, wParam, lParam);
	}

	BOOL Thread::PostTimer()
	{
		return ::PostThreadMessage(m_ThreadId, WM_TIMER, m_nTimer, 0);
	}

	UINT Thread::Peek()
	{
		MSG msg;
		
		return ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) ? msg.message : 0;			
	}

	void Thread::Join(bool wait)
	{
		SetEvent(m_hExit);
		
		if(m_hThread != NULL)
		{
			m_hThreadOld = m_hThread;
			m_hThread = NULL;
			
			Post(WM_QUIT);
		}

		if(m_hThreadOld != NULL && wait)
		{
			DWORD res = WaitForSingleObject(m_hThreadOld, 15000);
			
			if(res == WAIT_TIMEOUT) 
			{
				ASSERT(0); 
				
				TerminateThread(m_hThreadOld, (DWORD)-1);
		
				m_hThreadOld = NULL;
			}
		}
	}

	void Thread::SetTimerPeriod(UINT& value)
	{
		if(::GetCurrentThreadId() == m_ThreadId)
		{
			if(m_nTimer)
			{
				KillTimer(NULL, m_nTimer);

				m_nTimer = 0;
			}

			if(value > 0)
			{
				m_nTimer = SetTimer(NULL, 0, value, NULL);
			}
		}
	}
	
	void Thread::FireTimer()
	{
		if(TimerStartAfter == 0)
		{
			TimerEvent(NULL, m_nTimerEvent++);
		}
		else
		{
			TimerStartAfter = TimerStartAfter - 1;
		}
	}
}