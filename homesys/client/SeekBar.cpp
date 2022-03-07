#include "stdafx.h"
#include "SeekBar.h"
#include "client.h"

namespace DXUI
{
	SeekBar::SeekBar()
	{
		Position.Get([] (float& pos) {pos = g_env->PositionF;});
		Stop.Get([] (float& pos) {pos = g_env->PositionF;});
		Text.Get([] (std::wstring& str) 
		{
			str = g_env->PositionText;

			if(g_env->DurationRT > 0)
			{
				str += L" / ";
				str += g_env->DurationText;
			}
		});

		Position.ChangedEvent.Add([] (Control* c, const float& pos) -> bool 
		{
			printf("async %f\n", pos);
			return g_env->SeekAsyncEvent(c, pos);
		});

		DragReleasedEvent.Add([] (Control* c, const float& pos) -> bool 
		{
			printf("sync %f\n", pos);
			return g_env->SeekSyncEvent(c, pos);
		});
	}
}