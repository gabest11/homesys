#include "stdafx.h"
#include "EPGRowButton.h"

namespace DXUI
{
	EPGRowButton::EPGRowButton()
	{
		CurrentProgram.ChangedEvent.Add(&EPGRowButton::OnCurrentProgramChanged, this);
		PaintForegroundEvent.Add(&EPGRowButton::OnPaintForeground, this, true);
	}

	bool EPGRowButton::OnCurrentProgramChanged(Control* c, Homesys::Program* program)
	{
		if(program != NULL)
		{
			Text = program->episode.movie.title;
		}
		else
		{
			Text = L"";
		}

		return true;
	}

	bool EPGRowButton::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect->deflate(TextMargin);

		if(r.right <= r.left) return true;

		const Homesys::Program* program = CurrentProgram;

		if(program != NULL)
		{
			Vector4 p(WindowRect);
			Vector2 t(0, 0);
			Vector2i c(Color::White, 0);			
		
			int size = 10;
			
			if(program->start < HeaderStart)
			{
				Vertex v[6] = 
				{
					{Vector4(p.left, p.top), t, c},
					{Vector4(p.left + size, p.top), t, c},
					{Vector4(p.left, p.top + size), t, c},
					{Vector4(p.left, p.bottom), t, c},
					{Vector4(p.left + size, p.bottom), t, c},
					{Vector4(p.left, p.bottom - size), t, c},
				};

				dc->DrawTriangleList(v, 2, NULL);
			}

			if(program->stop > HeaderStop)
			{
				Vertex v[6] = 
				{
					{Vector4(p.right, p.top), t, c},
					{Vector4(p.right - size, p.top), t, c},
					{Vector4(p.right, p.top + size), t, c},
					{Vector4(p.right, p.bottom), t, c},
					{Vector4(p.right - size, p.bottom), t, c},
					{Vector4(p.right, p.bottom - size), t, c},
				};

				dc->DrawTriangleList(v, 2, NULL);
			}

			if(Texture* t = m_rstf.GetTexture(program, dc))
			{
				Vector4i r2 = r;

				r2.left = r2.right - 25;
				r2 = DeviceContext::FitRect(t->GetSize(), r2);
				r2.left = std::max<int>(r2.left, r.left);
				r.right = r2.left - 3;

				dc->Draw(t, r2);
			}
		}

		DrawScrollText(dc, Text->c_str(), r);

		return true;
	}
}
