#include "stdafx.h"
#include "EPGProgram.h"
#include "client.h"

namespace DXUI
{
	EPGProgram::EPGProgram()
	{
		CurrentProgram.ChangedEvent.Add(&EPGProgram::OnCurrentProgramChanged, this);
		PaintForegroundEvent.Add(&EPGProgram::OnPaintForeground, this);
		m_schedule.PaintForegroundEvent.Add(&EPGProgram::OnPaintScheduleForeground, this);
	}

	bool EPGProgram::OnCurrentProgramChanged(Control* c, Homesys::Program* p)
	{
		m_title.Text = L"";
		m_time.Text = L"";
		m_desc.Text = L"";

		if(p != NULL && p->id)
		{
			m_title.Text = p->episode.movie.title;

			if(!p->episode.title.empty())
			{
				std::wstring s = m_title.Text;

				m_title.Text = s + L" - " + p->episode.title;
			}

			if(p->start < p->stop)
			{
				std::wstring start = p->start.Format(L"%H:%M");
				std::wstring stop = p->stop.Format(L"%H:%M");

				m_time.Text = start + L" - " + stop;
			}

			if(MoreDescription)
			{
				m_desc.TextEscaped = true;

				std::wstring font = L"[{font.face: '" + FontType::Bold + L"'}] {";

				std::list<std::wstring> desc;

				//

				std::wstring br = Util::Format(L"[{font.size: %d}] {\\n\\n}", m_desc.TextHeight / 2);

				//

				std::wstring str;

				std::list<std::wstring> sl;

				if(!p->episode.movie.movieTypeShort.empty())
				{
					sl.push_back(p->episode.movie.movieTypeShort);
				}

				if(p->episode.episodeNumber > 0)
				{
					if(p->episode.movie.episodeCount > 0)
					{
						sl.push_back(Util::Format(L"%d/%d. rész", p->episode.movie.episodeCount, p->episode.episodeNumber));
					}
					else
					{
						sl.push_back(Util::Format(L"%d. rész", p->episode.episodeNumber));
					}
				}

				if(p->episode.movie.year > 0)
				{
					sl.push_back(Util::Format(L"%d", p->episode.movie.year));
				}

				if(p->stop > p->start)
				{
					sl.push_back(Util::Format(L"%d perc", ((p->stop - p->start).GetTimeSpan() + 59) / 60));
				}

				if(!sl.empty())
				{
					str = L"(" + Util::Implode(sl, '|') + L")";
					Util::Replace(str, L"|", L", ");
					str = font + DeviceContext::Escape(str.c_str()) + L"}";
					desc.push_back(str);
				}

				//

				{
					Homesys::Channel channel;

					if(g_env->svc.GetChannel(p->channelId, channel))
					{
						str = p->start.Format(L"%Y.%m.%d");
						str = channel.name + L", " + str;
						str = font + DeviceContext::Escape(str.c_str()) + L"}";
						desc.push_back(str);
					}
				}

				//

				if(p->episode.movie.rating > 0)
				{
					str = Util::Format(L"%d éven aluliaknak nem ajálott", p->episode.movie.rating);
					str = font + DeviceContext::Escape(str.c_str()) + L"}";
					desc.push_back(str);
				}

				//

				if(!p->episode.desc.empty())
				{
					desc.push_back(DeviceContext::Escape(p->episode.desc.c_str()));
				}
				else if(!p->episode.movie.desc.empty())
				{
					desc.push_back(DeviceContext::Escape(p->episode.movie.desc.c_str()));
				}

				//

				if(!p->episode.directors.empty())
				{
					sl.clear();

					for(auto i = p->episode.directors.begin(); i != p->episode.directors.end(); i++)
					{
						sl.push_back(i->name);
					}

					str = Util::Implode(sl, '|');

					Util::Replace(str, L"|", L", ");

					str = font + DeviceContext::Escape(sl.size() > 1 ? L"Rendező: " : L"Rendezők: ") + L"} " + DeviceContext::Escape(str.c_str());

					desc.push_back(str); 
				}

				if(!p->episode.actors.empty())
				{
					sl.clear();

					for(auto i = p->episode.actors.begin(); i != p->episode.actors.end(); i++)
					{
						const Homesys::Cast& cast = *i;

						str = cast.name;

						if(!cast.movieName.empty())
						{
							str += L" (" + cast.movieName + L")";
						}

						sl.push_back(str);
					}

					str = Util::Implode(sl, '|');

					Util::Replace(str, L"|", L", ");

					str = font + DeviceContext::Escape(sl.size() > 1 ? L"Szereplő: " : L"Szereplők: ") + L"} " + DeviceContext::Escape(str.c_str());

					desc.push_back(str); 
				}

				desc.push_back(font + DeviceContext::Escape(L"Forrás: PORT.hu") + L"}");

				str = Util::Implode(desc, '|');

				Util::Replace(str, L"|", br.c_str());

				m_desc.Text = str;
			}
			else
			{
				m_desc.TextEscaped = false;

				std::wstring desc = p->episode.desc;

				if(desc.empty()) desc = p->episode.movie.desc;

				m_desc.Text = desc;
			}
		}

		return true;
	}

	bool EPGProgram::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Homesys::Program* p = CurrentProgram;

		if(p != NULL)
		{
			Vector4i r = MapClientRect(&m_container); // ClientRect;

			// r.top = 35;

			r.left = 0;
			r.right = Width;

			if(ShowPicture)
			{
				std::wstring s = g_env->FormatMoviePictureUrl(p->episode.movie.id);

				Texture* t = dc->GetTexture(s.c_str());

				if(t != NULL)
				{
					Vector4i r2 = DeviceContext::FitRect(t->GetSize(), r);

					int w = r2.width();

					r2.left = 0;
					r2.right = w;
					r.left = r2.right + 10;

					//r2.left = r.right - w;
					//r2.right = r.right;
					//r.right = r2.left - 10;

					dc->Draw(t, C2W(r2));
				}
			}

			m_container.Move(r);
		}

		return true;
	}

	bool EPGProgram::OnPaintScheduleForeground(Control* c, DeviceContext* dc)
	{
		Homesys::Program* p = CurrentProgram;

		if(p != NULL)
		{
			Texture* t = m_rstf.GetTexture(p, dc);

			if(t != NULL)
			{
				Vector4i r = c->WindowRect;

				r.left = r.right - t->GetSize().x;

				r = DeviceContext::FitRect(t->GetSize(), r);

				dc->Draw(t, r);
			}
		}

		return true;
	}

}

