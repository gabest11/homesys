#include "stdafx.h"
#include "Animation.h"

namespace DXUI
{
	Animation::Animation(bool loop) 
		: m_loop(loop)
	{
		m_duration = 0;
		m_start = 0;
		m_src = 0;
	}

	void Animation::RemoveAll()
	{
		m_duration = 0;
		m_spans.clear();
	}

	int Animation::GetCount()
	{
		return m_spans.size();
	}

	void Animation::Set(float src, float dst, clock_t duration)
	{
		RemoveAll();

		Add(dst, duration);

		m_src = src;
	}

	void Animation::Add(float dst, clock_t duration)
	{
		if(m_spans.empty())
		{
			m_start = clock();
			m_src = 0;
		}

		m_duration += duration;

		Span s;

		s.duration = duration;
		s.dst = dst;

		m_spans.push_back(s);
	}

	void Animation::SetSegment(int i)
	{
		SetSegment(i, 0);
	}

	void Animation::SetSegment(int i, clock_t offset)
	{
		m_start = clock();

		for(auto j = m_spans.begin(); i-- > 0 && j != m_spans.end(); j++)
		{
			m_start -= j->duration;
		}

		m_start -= offset;
	}

	int Animation::GetSegment()
	{
		clock_t now = clock();
		clock_t start = m_start;

		if(m_loop)
		{
			clock_t c = now - start;

			if(m_duration > 0 && m_duration < c)
			{
				start = now - c % m_duration;
			}
		}

		int i = 0;

		for(auto j = m_spans.begin(); j != m_spans.end(); j++, i++)
		{
			const Span& s = *j;

			if(start <= now && now < start + s.duration)
			{
				return i;
			}

			start += s.duration;
		}

		return m_spans.size();
	}

	void Animation::SetSegmentDuration(int i, clock_t duration)
	{
		Span& s = m_spans[i];

		m_duration -= s.duration;
		s.duration = duration;
		m_duration += s.duration;
	}

	Animation::operator float()
	{
		clock_t now = clock();
		clock_t start = m_start;

		if(m_loop)
		{
			clock_t c = now - start;

			if(m_duration > 0 && m_duration < c)
			{
				start = now - c % m_duration;
			}
		}

		float f = m_src;

		for(auto j = m_spans.begin(); j != m_spans.end(); j++)
		{
			const Span& s = *j;

			clock_t stop = start + s.duration;

			if(start <= now && now <= stop)
			{
				return f + (s.dst - f) * (now - start) / (stop - start);
			}

			start = stop;

			f = s.dst;
		}

		return f;
	}
}