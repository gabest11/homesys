#pragma once

#include <atlsync.h>
#include "../3rdparty/baseclasses/baseclasses.h"

template<class T> class CThreadSafeQueue : public CCritSec
{
	std::list<T> m_queue;
	ATL::CSemaphore m_put;
	ATL::CSemaphore m_get;
	CAMEvent m_enqueue;
	CAMEvent m_dequeue;
	LONG m_count;
 
public:
	CThreadSafeQueue(LONG count) 
		: m_put(count, count)
		, m_get(0, count)
		, m_enqueue(TRUE)
		, m_dequeue(TRUE)
		, m_count(count)
	{
		m_dequeue.Set();
	}
 
	size_t GetCount()
	{
		CAutoLock cAutoLock(this);
 
		return m_queue.size();
	}
 
	size_t GetMaxCount()
	{
		CAutoLock cAutoLock(this);
 
		return (size_t)m_count;
	}
 
	CAMEvent& GetEnqueueEvent()
	{
		return m_enqueue;
	}
 
	CAMEvent& GetDequeueEvent()
	{
		return m_dequeue;
	}
 
	void Enqueue(T item)
	{
		WaitForSingleObject(m_put, INFINITE);
 
		{
			CAutoLock cAutoLock(this);
 
			m_queue.push_back(item);
 
			m_enqueue.Set();
			m_dequeue.Reset();
		}
 
		m_get.Release();
	}
 
	T Dequeue()
	{
		T item;
 
		WaitForSingleObject(m_get, INFINITE);
 
		{
			CAutoLock cAutoLock(this);
 
			item = m_queue.front();

			m_queue.pop_front();
 
			if(m_queue.empty())
			{
				m_enqueue.Reset();
				m_dequeue.Set();
			}
		}
 
		m_put.Release();
 
		return item;
	}

	T Peek() // lock on "this"
	{
		return m_queue.front();
	}
};

