#include "stdafx.h"
#include "MenuDVDOSD.h"
#include "client.h"
#include "Player.h"
#include "../../directshow/DSMPropertyBag.h"
#include "../../directshow/MediaTypeEx.h"
#include <qnetwork.h>
#include <wmsdk.h>

namespace DXUI
{
	MenuDVDOSD::MenuDVDOSD() 
	{
		m_seekbar.Visible.Get([] (bool& visible) {visible = g_env->ps.stop > g_env->ps.start;});
		m_controls.Visible.Get([] (bool& visible) {visible = !g_env->players.empty();}); // TODO: && !CComQIPtr<IFlashPlayer>(g_env->players.front());});

		ActivatedEvent.Add(&MenuDVDOSD::OnActivated, this);
		EndFileEvent.Add0(&MenuDVDOSD::OnEndFile, this);
		InfoEvent.Add0(&MenuDVDOSD::OnInfoEvent, this);
		KeyEvent.Add(&MenuDVDOSD::OnKey, this);
		RedEvent.Add0(&MenuDVDOSD::OnRed, this);

		g_env->EndFileEvent.Chain(EndFileEvent);
		g_env->InfoEvent.Chain(InfoEvent);

		HandleEndOfFile = true;
	}

	MenuDVDOSD::~MenuDVDOSD() 
	{
		g_env->EndFileEvent.Unchain(EndFileEvent);
		g_env->InfoEvent.Unchain(InfoEvent);
	}

	bool MenuDVDOSD::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_info_desc_button.Visible = false;
			m_info_desc_more_container.Visible = false;
		}

		return true;
	}

	bool MenuDVDOSD::OnEndFile(Control* c)
	{
		if(Activated && HandleEndOfFile)
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			if(CComQIPtr<IPlaylistPlayer>(p) || CComQIPtr<IImagePlayer>(p))
			{
				// TODO: random mode

				g_env->NextEvent(this);
			}
			else if(CComQIPtr<IFilePlayer>(p))
			{
				g_env->CloseFileEvent(this);
				
				Close();
			}
		}

		return true;
	}

	bool MenuDVDOSD::OnInfoEvent(Control* c)
	{
		std::wstring str;

		m_info_picture.BackgroundImage = L"";
		m_info_video_icon.BackgroundImage = L"Video.png";
		m_info_audio_icon.BackgroundImage = L"Audio.png";
		m_info_subtitle_icon.BackgroundImage = L"Subtitle.png";
		m_info_title.Text = L"";
		m_info_desc.Text = L"";
		m_info_year.Text = L"";
		m_info_video.Text = L"";
		m_info_audio.Text = L"";
		m_info_subtitle.Text = L"";
		m_info_desc_more.Text = L"";
		m_info_desc_button.Visible = false;

		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> player = g_env->players.front();

			if(CComQIPtr<IDVDPlayer> dvd = player)
			{
				if(g_env->ps.dvd.error)
				{
					switch(g_env->ps.dvd.error)
					{
					case DVD_ERROR_Unexpected: default: str = L"DVD: Unexpected error"; break;
					case DVD_ERROR_CopyProtectFail: str = L"DVD: Copy-Protect Fail"; break;
					case DVD_ERROR_InvalidDVD1_0Disc: str = L"DVD: Invalid DVD 1.x Disc"; break;
					case DVD_ERROR_InvalidDiscRegion: str = L"DVD: Invalid Disc Region"; break;
					case DVD_ERROR_LowParentalLevel: str = L"DVD: Low Parental Level"; break;
					case DVD_ERROR_MacrovisionFail: str = L"DVD: Macrovision Fail"; break;
					case DVD_ERROR_IncompatibleSystemAndDecoderRegions: str = L"DVD: Incompatible System And Decoder Regions"; break;
					case DVD_ERROR_IncompatibleDiscAndDecoderRegions: str = L"DVD: Incompatible Disc And Decoder Regions"; break;
					}

					m_info_title.Text = str;
				}
				else
				{
					if(g_env->ps.dvd.domain == DVD_DOMAIN_Title)
					{
						str = Util::Format(L"%d. title", g_env->ps.dvd.title);

						m_info_title.Text = str;
						
						str = Util::Format(_T("%d. fejezet"), g_env->ps.dvd.chapter);

						m_info_desc.Text = str;

						m_seekbar.Visible = true;
					}
					else
					{
						switch(g_env->ps.dvd.domain)
						{
						case DVD_DOMAIN_FirstPlay: str = L"Bevezető"; break;
						case DVD_DOMAIN_VideoManagerMenu: str = L"Video Manager Menu"; break;
						case DVD_DOMAIN_VideoTitleSetMenu: str = L"Video Title Set Menu"; break;
						case DVD_DOMAIN_Title: str = L"Title"; break;
						case DVD_DOMAIN_Stop: str = L"Stop"; break;
						default: str = L"-"; break;
						}

						m_info_title.Text = str;

						m_seekbar.Visible = false;
					}

					if(g_env->ps.dvd.angle >= 1)
					{
						int index = -1;

						TrackInfo info;

						if(SUCCEEDED(player->GetTrack(TrackType::Video, index)) 
						&& SUCCEEDED(player->GetTrackInfo(TrackType::Video, index, info)))
						{
							// hm
						}

						str = Util::Format(L"%d. %s", g_env->ps.dvd.angle, L"kép"); // TODO: CMediaTypeEx(info.mt).ToString());

						m_info_video.Text = str;
					}

					if(g_env->ps.dvd.audio >= 0)
					{
						int index = -1;

						TrackInfo info;

						if(SUCCEEDED(player->GetTrack(TrackType::Audio, index)) 
						&& SUCCEEDED(player->GetTrackInfo(TrackType::Audio, index, info)))
						{
							// hm
						}

						str = Util::Format(L"%d. %s", g_env->ps.dvd.audio + 1, L"hang"); // TODO:, CMediaTypeEx(info.mt).ToString());

						m_info_audio.Text = str;
					}

					if(g_env->ps.dvd.subpicture >= 0)
					{
						str = Util::Format(L"%d. %s", g_env->ps.dvd.subpicture + 1, L"felirat");

						m_info_subtitle.Text = str;
					}
				}
			}
			else if(CComQIPtr<IFilePlayer> file = player)
			{
				std::wstring str = file->GetFileName();

				if(str.find(L"://") == std::wstring::npos)
				{
					wchar_t* tmp = new wchar_t[str.size() + 1];
					wcscpy(tmp, str.c_str());
					PathStripPath(tmp);
					str = tmp;
				}

				m_info_title.Text = str;

				{
					int index = -1;
					TrackInfo info;				
					int count;

					if(SUCCEEDED(player->GetTrack(TrackType::Video, index)) 
					&& SUCCEEDED(player->GetTrackInfo(TrackType::Video, index, info))
					&& SUCCEEDED(player->GetTrackCount(TrackType::Video, count)))
					{
						str = CMediaTypeEx(info.mt).ToString();	
						if(info.dxva) str += L" (DXVA)";
						else if(info.broadcom) str += L" (HW Decoding)";
						if(count > 1) str = L"\u00ab " + str + L" \u00bb";
						m_info_video.Text = str;
						
						BITMAPINFOHEADER bih;
						
						if(CMediaTypeEx(info.mt).ExtractBIH(&bih) && abs(bih.biHeight) >= 720)
						{
							m_info_video_icon.BackgroundImage = L"VideoHD.png";
						}
					}
				}

				{
					int index = -1;				
					TrackInfo info;
					int count;

					if(SUCCEEDED(player->GetTrack(TrackType::Audio, index)) 
					&& SUCCEEDED(player->GetTrackInfo(TrackType::Audio, index, info))
					&& SUCCEEDED(player->GetTrackCount(TrackType::Audio, count)))
					{
						str = CMediaTypeEx(info.mt).ToString();
						// if(!info.name.empty()) str += L"(" + info.name + L")";
						if(!info.iso6392.empty()) str += L" (" + info.iso6392 + L")";
						if(count > 1) str = Util::Format(L"\u00ab %d. %s \u00bb", index + 1, str.c_str());
						m_info_audio.Text = str;

						if(str.find(L"AC3") != std::wstring::npos)
						{
							m_info_audio_icon.BackgroundImage = L"AudioAC3.png";
						}
						else if(str.find(L"DTS") != std::wstring::npos)
						{
							m_info_audio_icon.BackgroundImage = L"AudioDTS.png";
						}
					}
				}

				{
					int index = -1;
					TrackInfo info;
					int count;

					if(SUCCEEDED(player->GetTrack(TrackType::Subtitle, index)) 
					&& SUCCEEDED(player->GetTrackInfo(TrackType::Subtitle, index, info))
					&& SUCCEEDED(player->GetTrackCount(TrackType::Subtitle, count))
					&& index > 0)
					{
						str = info.name;
						if(!info.iso6392.empty()) str += L" (" + info.iso6392 + L")";
						if(count > 1) str = Util::Format(L"\u00ab %d. %s \u00bb", index, str.c_str());

						m_info_subtitle.Text = str;
					}
				}

				CComPtr<IFilterGraph> pFG;

				if(SUCCEEDED(player->GetFilterGraph(&pFG)) && pFG)
				{
					Foreach(pFG, [&] (IBaseFilter* pBF) -> HRESULT
					{
						/*
						if(CComQIPtr<IDSMChapterBag> pCB = pBF)
						{
							REFERENCE_TIME rt = 0;
							
							if(control->GetPos(rt))
							{
								CComBSTR bstr;
								
								long i = pCB->ChapLookup(&rt, &bstr);
								
								if(i >= 0)
								{
									str.Format(_T("Fejezet: %d. %s"), i + 1, CString(bstr.m_str));

									m_info_chapter.Text = str; 
								}
							}
						}
						*/

						std::wstring title, author, copyright;

						CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF;

						if(pAMMC != NULL)
						{
							CComBSTR bstr;
							if(SUCCEEDED(pAMMC->get_Title(&bstr)) && wcslen(bstr.m_str) > 0) title = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && wcslen(bstr.m_str) > 0) author = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pAMMC->get_Copyright(&bstr)) && wcslen(bstr.m_str) > 0) copyright = bstr.m_str;
						}

						CComQIPtr<IDSMPropertyBag> pPB = pBF;

						if(pPB != NULL)
						{
							CComBSTR bstr;
							if(SUCCEEDED(pPB->GetProperty(L"TITL", &bstr)) && wcslen(bstr.m_str) > 0) title = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"AUTH", &bstr)) && wcslen(bstr.m_str) > 0) author = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"CPYR", &bstr)) && wcslen(bstr.m_str) > 0) copyright = bstr.m_str;
						}

						CComQIPtr<IWMHeaderInfo> pHI = pBF;

						if(pHI != NULL)
						{
							WORD wStreamNum = 0;
							WMT_ATTR_DATATYPE Type;
							WORD cbLength;

							if(SUCCEEDED(pHI->GetAttributeByName(&wStreamNum, L"Title", &Type, NULL, &cbLength)) && Type == WMT_TYPE_STRING && cbLength > 0)
							{
								WCHAR* pwszTitle = (WCHAR*)new BYTE[cbLength];

								if(SUCCEEDED(pHI->GetAttributeByName(&wStreamNum, L"Title", &Type, (BYTE*)pwszTitle, &cbLength)) && Type == WMT_TYPE_STRING && cbLength > 0)
								{
									title = pwszTitle;
								}

								delete [] pwszTitle;
							}
						}

						if(!title.empty())
						{
							m_info_title.Text = title;
						}

						if(!author.empty())
						{
							m_info_desc.Text = author;
						}

						if(int i = _wtoi(copyright.c_str()))
						{
							str = Util::Format(L"%d", i);

							m_info_year.Text = str;
						}

						return S_CONTINUE;
					});
				}
			}
			else if(CComQIPtr<IImagePlayer> image = player)
			{
				std::wstring str = image->GetFileName();

				if(str.find(L"://") == std::wstring::npos)
				{
					wchar_t* tmp = new wchar_t[str.size() + 1];
					wcscpy(tmp, str.c_str());
					PathStripPath(tmp);
					str = tmp;
				}

				m_info_title.Text = str;

				str = Util::Format(L"%dx%d", image->GetWidth(), image->GetHeight());

				m_info_video.Text = str;
			}
		}

		Vector4i r = m_info_text.Rect;

		r.left = 10;

		if(!m_info_picture.BackgroundImage->empty())
		{
			r.left += m_info_picture.Right;

			m_info_picture.Visible = true;
		}
		else
		{
			m_info_picture.Visible = false;
		}

		m_info_text.Move(r);

		return true;
	}

	bool MenuDVDOSD::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0 || p.mods == KeyEventParam::Shift)
		{
			if(p.cmd.key == VK_LEFT)
			{
				m_controls.m_rewind.Click();

				return true;
			}

			if(p.cmd.key == VK_RIGHT)
			{
				m_controls.m_forward.Click();

				return true;
			}
		}

		if(p.mods == KeyEventParam::Ctrl)
		{
			if(p.cmd.key == VK_LEFT)
			{
				m_controls.m_prev.Click();

				return true;
			}

			if(p.cmd.key == VK_RIGHT)
			{
				m_controls.m_next.Click();

				return true;
			}
		}

		if(p.mods == 0)
		{
			if(p.cmd.app == APPCOMMAND_MEDIA_PREVIOUSTRACK)
			{
				m_controls.m_prev.Click();
			
				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_NEXTTRACK)
			{
				m_controls.m_next.Click();
			
				return true;
			}

			if(p.cmd.remote == RemoteKey::DVDMenu)
			{
				m_controls.m_stop.Click();

				return true;
			}

			if(p.cmd.key == VK_SPACE)
			{
				m_controls.m_playpause.Click();

				return true;
			}

			if(p.cmd.key == 'V' || p.cmd.remote == RemoteKey::Angle)
			{
				g_env->VideoSwitchEvent(this);

				return true;
			}

			if(p.cmd.key == 'A' || p.cmd.remote == RemoteKey::Audio)
			{
				g_env->AudioSwitchEvent(this);

				return true;
			}

			if(p.cmd.key == 'S' || p.cmd.remote == RemoteKey::Subtitle)
			{
				g_env->SubtitleSwitchEvent(this);

				return true;
			}
		
			/*
			if(p.nChar == VK_F2 || p.nRemoteCmd == RemoteKey::Details)
			{
				if(!g_env->players.IsEmpty())
				{
					if(CComQIPtr<Player::IDVD> dvd = g_env->players.GetHead())
					{
						CString str;

						if(SUCCEEDED(dvd->GetDVDDirectory(str)))
						{
							g_env->FileInfoRequestEvent(this, str);
							return true;
						}
					}
					else if(CComQIPtr<Player::IFile> file = g_env->players.GetHead())
					{
						CString str = file->GetFileName();

						if(!str.IsEmpty())
						{
							g_env->FileInfoRequestEvent(this, str);
							return true;
						}
					}
				}
			}
			*/
		}

		return false;
	}

	bool MenuDVDOSD::OnRed(Control* c)
	{
		if(!m_info_desc_more.Text->empty()) 
		{
			m_info_desc_more_container.Visible = !m_info_desc_more_container.Visible;
		}
		else 
		{
			m_info_desc_more_container.Visible = false;
		}

		return true;
	}
}