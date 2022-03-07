#pragma once

#include <time.h>
#include <list>

namespace DXUI
{
	class Animation
	{
		bool m_loop;

		clock_t m_duration;
		clock_t m_start;
		float m_src;
		struct Span {clock_t duration; float dst;};
		std::vector<Span> m_spans;

	public:
		Animation(bool loop = false);
		void RemoveAll();
		int GetCount();
		void Set(float src, float dst, clock_t duration);
		void Add(float dst, clock_t duration);
		void SetSegment(int i);
		void SetSegment(int i, clock_t offset);
		int GetSegment();
		void SetSegmentDuration(int i, clock_t duration);
		operator float();
		float operator [] (int i) {return i == 0 ? m_src : m_spans[i - 1].dst;}
	};
}
