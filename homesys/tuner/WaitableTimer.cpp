#include "StdAfx.h"
#include "WaitableTimer.h"

using namespace Microsoft::Win32::SafeHandles;

namespace Homesys
{
	WaitableTimer::WaitableTimer()
	{
		m_handles = gcnew array<WaitHandle^> {gcnew AutoResetEvent(false), gcnew AutoResetEvent(false)};
		m_handles[1]->SafeWaitHandle = gcnew SafeWaitHandle((IntPtr)CreateWaitableTimer(NULL, TRUE, NULL), true);
	}

	WaitableTimer::~WaitableTimer()
	{
		this->!WaitableTimer();
	}

	WaitableTimer::!WaitableTimer()
	{
		BreakWait();

		delete [] m_handles;
	}

	void WaitableTimer::Set(DateTime dueTime)
	{
		if(m_dueTime == dueTime) return;

		BreakWait();

		m_dueTime = dueTime;

		LARGE_INTEGER li;
		li.QuadPart = dueTime.ToFileTime();
		SetWaitableTimer((HANDLE)m_handles[1]->SafeWaitHandle->DangerousGetHandle(), &li, 0, NULL, NULL, TRUE);

		m_thread = gcnew Thread(gcnew ThreadStart(this, &WaitableTimer::DoWait));
		m_thread->Start();

		Console::WriteLine("Wakeup event scheduled at {0}", m_dueTime);
	}

	void WaitableTimer::Reset()
	{
		BreakWait();

		m_dueTime = DateTime::MinValue;
	}

	void WaitableTimer::DoWait()
	{
		int i = WaitHandle::WaitAny(m_handles);

		Console::WriteLine("Wakeup event [{0}] fired at {1}", i, DateTime::Now);

		m_dueTime = DateTime::Now;
	}

	void WaitableTimer::BreakWait()
	{
		if(m_thread)
		{
			AutoResetEvent^ e = (AutoResetEvent^)m_handles[0];

			e->Set();

			m_thread->Join();

			e->Reset();
		}
	}
}