#include "stdafx.h"
#include "SeekBar2.h"
#include "client.h"

namespace DXUI
{
	SeekBar2::SeekBar2()
	{
		FillRect.Get(&SeekBar2::GetFillRect, this);

		ActivatedEvent.Add(&SeekBar2::OnActivated, this);
		DeactivatedEvent.Add(&SeekBar2::OnDeactivated, this);
		PaintBackgroundEvent.Add(&SeekBar2::OnPaintBackground, this, true);
		PaintForegroundEvent.Add(&SeekBar2::OnPaintForeground, this, true);

		m_thread.TimerEvent.Add(&SeekBar2::OnThreadTimer, this);
		m_thread.TimerPeriod = 1000;
	}

	void SeekBar2::GetFillRect(Vector4i& value)
	{
		Vector4i r = ClientRect->deflate(BackgroundMargin);

		value = r.inflate(Vector4i(5, 3).xyxy());
	}

	bool SeekBar2::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
		}

		m_thread.Create();

		return true;
	}

	bool SeekBar2::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join(false);

		m_parts.clear();
		m_length = 0;

		return true;
	}

	bool SeekBar2::OnThreadTimer(Control* c, UINT64 n)
	{
		CComPtr<IHttpReader> p;

		{
			CAutoLock cAutoLock(&m_lock);

			if(!g_env->players.empty())
			{
				CComPtr<IFilterGraph> pFG;

				if(SUCCEEDED(g_env->players.front()->GetFilterGraph(&pFG)))
				{
					ForeachInterface<IHttpReader>(pFG, [&] (IBaseFilter* pBF, IHttpReader* pReader) -> HRESULT
					{
						p = pReader;

						return S_OK;
					});
				}
			}
		}

		{
			CAutoLock cAutoLock(&m_thread);

			m_parts.clear();
			m_length = 0;

			if(p != NULL)
			{
				p->GetAvailable(m_parts, m_length);
			}
		}

		return true;
	}

	bool SeekBar2::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		Texture* t;

		Vector4i r = FillRect;

		if(m_length > 0)
		{
			t = dc->GetTexture(L"CutExclude.png");

			if(t != NULL)
			{
				dc->Draw9x9(t, C2W(r), Vector2i(5, 1));
			}
		}

		t = dc->GetTexture(L"CutInclude.png");

		if(t != NULL)
		{
			if(m_length > 0)
			{
				CAutoLock cAutoLock(&m_thread);

				int w = r.width();
				int l = r.left;

				for(auto i = m_parts.begin(); i != m_parts.end(); i++)
				{
					const HttpFilePart& p = *i;

					r.left = l + p.pos * w / m_length;
					r.right = l + (p.pos + p.len) * w / m_length;

					r.left = std::max<int>(std::min<int>(r.left, l + w), 0);
					r.right = std::max<int>(std::min<int>(r.right, l + w), 0);

					dc->Draw9x9(t, C2W(r), Vector2i(5, 1));
				}
			}
			else
			{
				dc->Draw9x9(t, C2W(r), Vector2i(5, 1));
			}
		}

		t = dc->GetTexture(BackgroundImage->c_str());

		if(t != NULL)
		{
/*
				SolidVertex v1[3] =
				{
					{left, top, 0.5f, 2, color},
					{left + size, top, 0.5f, 2, color},
					{left, top + size, 0.5f, 2, color},
				};

				SolidVertex v2[3] =
				{
					{left, bottom, 0.5f, 2, color},
					{left + size, bottom, 0.5f, 2, color},
					{left, bottom - size, 0.5f, 2, color},
				};

				dc->DrawPoly(v1, 3);
*/
			dc->Draw9x9(t, WindowRect, BackgroundMargin);
		}

		return true;
	}

	bool SeekBar2::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(!Text->empty())
		{
			Vector4i r = WindowRect;

			r.left = r.right - 185;
			r.right = r.right - 25;
			r.top += 2;
			r.bottom -= 4;

			dc->TextFace = FontType::Bold;
			dc->TextHeight = r.height();
			dc->Draw(Text->c_str(), r);
		}

		if(Indicator)
		{
			Texture* t = dc->GetTexture(IndicatorImage->c_str());

			if(t != NULL)
			{
				Vector4i r = ClientRect->deflate(BackgroundMargin);

				int w = r.width();

				int x = (int)(r.left + Position * w + 0.5f);
				int y = Height / 2;

				Vector2i s = t->GetSize();

				r.left = x - s.x / 2;
				r.top = y - s.y / 2;
				r.right = r.left + s.x;
				r.bottom = r.top + s.y;

				r = r.inflate(3);

				dc->Draw(t, C2W(r), Alpha);
			}
		}

		return true;
	}
}