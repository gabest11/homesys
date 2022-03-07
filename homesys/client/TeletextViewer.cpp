#include "stdafx.h"
#include "TeletextViewer.h"
#include "client.h"
#include "../../DirectShow/TeletextFilter.h"

namespace DXUI
{
	TeletextViewer::TeletextViewer()
	{
		ActivatedEvent.Add(&TeletextViewer::OnActivated, this);
		DeactivatedEvent.Add(&TeletextViewer::OnDeactivated, this);
		PaintForegroundEvent.Add(&TeletextViewer::OnPaintForeground, this);
		KeyEvent.Add(&TeletextViewer::OnKey, this);
		RedEvent.Add0([&] (Control* c) -> bool {return JumpToLink(0);});
		GreenEvent.Add0([&] (Control* c) -> bool {return JumpToLink(1);});
		YellowEvent.Add0([&] (Control* c) -> bool {return JumpToLink(2);});
		BlueEvent.Add0([&] (Control* c) -> bool {return JumpToLink(3);});
		
		m_teletext.tex = NULL;
		m_teletext.buff = NULL;

		Control::DC.ChangedEvent.Add0([&] (Control* c) -> bool {m_teletext.Free(); return true;});
	}

	TeletextViewer::~TeletextViewer()
	{
		m_teletext.Free();
	}

	bool TeletextViewer::JumpToLink(int index)
	{
		bool res = false;

		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			Vector4i r(0, 0, 520, 400);

			CComPtr<IFilterGraph> g;

			if(SUCCEEDED(p->GetFilterGraph(&g)))
			{
				ForeachInterface<ITeletextRenderer>(g, [&] (IBaseFilter* pBF, ITeletextRenderer* pTR) -> HRESULT
				{
					res = pTR->JumpToLink(index) == S_OK;

					return S_OK;
				});
			}
		}

		return res;
	}

	bool TeletextViewer::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
		}

		return true;
	}

	bool TeletextViewer::OnDeactivated(Control* c, int dir)
	{
		m_teletext.Free();

		if(dir <= 0)
		{
		}

		return true;
	}

	bool TeletextViewer::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			Vector4i r(0, 0, 520, 400);

			CComPtr<IFilterGraph> g;

			if(SUCCEEDED(p->GetFilterGraph(&g)))
			{
				ForeachInterface<ITeletextRenderer>(g, [&] (IBaseFilter* pBF, ITeletextRenderer* pTR) -> HRESULT
				{
					if(m_teletext.buff == NULL)
					{
						m_teletext.buff = new BYTE[520 * 400 * 4];
					}

					if(m_teletext.buff != NULL)
					{
						if(SUCCEEDED(pTR->GetImage(m_teletext.buff, 520 * 4)))
						{
							if(m_teletext.tex == NULL)
							{
								m_teletext.tex = dc->CreateTexture(m_teletext.buff, 520, 400, 520 * 4, false);
							}
							else
							{
								m_teletext.tex->Update(r, m_teletext.buff, 520 * 4);
							}
						}
					}

					return S_OK;
				});
			}

			if(m_teletext.tex != NULL)
			{
				dc->Draw(m_teletext.tex, WindowRect);
			}
		}

		return true;
	}

	bool TeletextViewer::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.chr >= '0' && p.cmd.chr <= '9' || p.cmd.chr == 246
			|| p.cmd.chr == '+' || p.cmd.chr == '-' 
			|| p.cmd.key == VK_UP || p.cmd.key == VK_DOWN
			|| p.cmd.key == VK_LEFT || p.cmd.key == VK_RIGHT)
			{
				CComPtr<ITeletextRenderer> tr;

				if(!g_env->players.empty())
				{
					CComPtr<IGenericPlayer> player = g_env->players.front();

					CComPtr<IFilterGraph> graph;

					if(SUCCEEDED(player->GetFilterGraph(&graph)))
					{
						ForeachInterface<ITeletextRenderer>(graph, [&] (IBaseFilter* pBF, ITeletextRenderer* pTR) -> HRESULT
						{
							tr = pTR;

							return S_OK;
						});
					}
				}

				if(tr != NULL)
				{
					DWORD page = 0;

					tr->GetPage(page);

					if(p.cmd.chr >= '0' && p.cmd.chr <= '9' || p.cmd.chr == 246)
					{
						wchar_t c = p.cmd.chr == 246 ? '0' : (wchar_t)p.cmd.chr;

						tr->EnterPage(c - '0');
					}
					else if(p.cmd.chr == '+' || p.cmd.key == VK_UP || p.cmd.key == VK_RIGHT)
					{
						page = page < 0x899 ? page + 1 : 0x100;

						while((page & 0x0f) > 0x09) page++;
						while((page & 0xf0) > 0x99) page++;
							
						tr->SetPage(page);
					}
					else if(p.cmd.chr == '-' || p.cmd.key == VK_DOWN || p.cmd.key == VK_LEFT)
					{
						page = page > 0x100 ? page - 1 : 0x899;

						while((page & 0xf0) > 0x99) page--;
						while((page & 0x0f) > 0x09) page--;

						tr->SetPage(page);
					}
				}
				
				return true;
			}
		}

		return false;
	}

	void TeletextViewer::Teletext::Free()
	{
		delete tex;

		tex = NULL;

		delete [] buff;

		buff = NULL;
	}
}
