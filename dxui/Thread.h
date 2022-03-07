#pragma once

#include "Property.h"

namespace DXUI
{
	class Thread : public CCritSec
	{
		bool m_initcom;

		void SetTimerPeriod(UINT& value);
		void FireTimer();

	protected:
		HANDLE m_hEventQueueReady;
		HANDLE m_hThread;
		HANDLE m_hThreadOld;
		DWORD m_ThreadId;
		UINT m_nTimer;
		UINT64 m_nTimerEvent;
		HANDLE m_hExit;
		static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
		DWORD ThreadProc();

	public:
		Thread(bool initcom = true);
		virtual ~Thread();

		HANDLE Create();
		BOOL Post(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
		BOOL PostTimer();
		UINT Peek();
		void Join(bool wait = true);

		DWORD GetThreadId() const {return m_ThreadId;}

		HANDLE GetJoinEvent() const {return m_hExit;}

		operator HANDLE() {return m_hThread;}

		Property<UINT> TimerPeriod;
		Property<UINT> TimerStartAfter;

		Event<> InitEvent;
		Event<> ExitEvent;
		Event<UINT64> TimerEvent;
		Event<const MSG&> MessageEvent;
	};
}
