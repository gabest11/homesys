#pragma once

using namespace System;
using namespace System::Threading;

namespace Homesys
{
	public ref class WaitableTimer
	{
		array<WaitHandle^>^ m_handles;
		DateTime m_dueTime;
		Thread^ m_thread;

		void DoWait();
		void BreakWait();

	public:
		WaitableTimer();
		~WaitableTimer();
		!WaitableTimer();

		void Set(DateTime dueTime);
		void Reset();
	};
}