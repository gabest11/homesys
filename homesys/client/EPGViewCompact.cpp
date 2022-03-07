#include "stdafx.h"
#include "EPGViewCompact.h"
#include "client.h"

namespace DXUI
{
	EPGViewCompact::EPGViewCompact()
	{
		m_overlay.PaintForegroundEvent.Add(&EPGViewCompact::OnPaintOverlayForeground, this);

		PresetRows = 1;
		Compact = true;
	}

	bool EPGViewCompact::OnPaintOverlayForeground(Control* c, DeviceContext* dc)
	{
		CTime now = CTime::GetCurrentTime();
		CTime start = now - CTimeSpan((g_env->ps.stop - g_env->ps.start) / 10000000);
		CTime current = now - CTimeSpan((g_env->ps.stop - g_env->ps.GetCurrent()) / 10000000);

		PaintFlag((CTime)Start, dc, false);
		PaintFlag((CTime)Start + Duration, dc, false);
		PaintFlag(current, dc, true);

		return true;
	}

	void EPGViewCompact::PaintFlag(CTime time, DeviceContext* dc, bool indicator)
	{
		if(m_rows.empty())
		{
			return;
		}

		Vector4i cr = MapClientRect(&m_rows.front()->m_programbox);

		Vector4i r = cr;

		int x = cr.left + cr.width() * (time - Start).GetTimeSpan() / Duration->GetTimeSpan();

		if(x < cr.left || x > cr.right)
		{
			return;
		}

		Texture* t = dc->GetTexture(L"epg_zaszlo.png");

		if(t != NULL)
		{
			int y = r.top;
/*
			r.top = y - 20;
			r.bottom = y;
			r.left = x - 1;
			r.right = x + 1;

			if(indicator)
			{
				dc->Draw(C2W(r), Color::White);
			}

			r.top = y - 50;
			r.bottom = y - 10;
*/
			r.top = y - 42;
			r.bottom = y - 2;
			r.left = x - 40;
			r.right = r.left + 80;

			if(r.right > cr.right) {r.left = cr.right - r.width(); r.right = cr.right;}
			if(r.left < cr.left) {r.right = cr.left + r.width(); r.left = cr.left;}

			dc->Draw9x9(t, C2W(r), Vector4i(9));

			dc->TextAlign = Align::Center | Align::Middle;
			dc->TextColor = Color::White;
			dc->TextHeight = r.height() * 2 / 3;

			std::wstring s = std::wstring(time.Format(L"%#H:%M"));

			dc->Draw(s.c_str(), C2W(r));
		}
	}
}
