#include "stdafx.h"
#include "MenuSignup.h"
#include "client.h"

namespace DXUI
{
	MenuSignup::MenuSignup() 
	{
		ActivatedEvent.Add(&MenuSignup::OnActivated, this);
		DeactivatedEvent.Add(&MenuSignup::OnDeactivated, this);
		m_next.ClickedEvent.Add0(&MenuSignup::OnNext, this);
		DoneEvent.Add0([] (Control* c) -> bool {g_env->UpdateServiceState(); return true;});
	}

	bool MenuSignup::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_username.Text = L"";
			m_password.Text = L"";
			m_password2.Text = L"";
			m_email.Text = L"";
		}

		return true;
	}

	bool MenuSignup::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuSignup::OnNext(Control* c)
	{
		std::wstring username = m_username.Text;
		std::wstring password = m_password.Text;
		std::wstring password2 = m_password2.Text;
		std::wstring email = m_email.Text;

		std::wstring str;

		if(password != password2)
		{
			str = Control::GetString("REGISTER_PASSWORD_MISMATCH");
		}
		else if(g_env->svc.LoginExists(username.c_str(), password.c_str()))
		{
			if(g_env->svc.SetUsername(username) && g_env->svc.SetPassword(password))
			{
				g_env->svc.GoOnline();

				DoneEvent(this);
			}
			else
			{
				str = Control::GetString("REGISTER_PLEASE_FILL");
			}
		}
		else
		{
			if(username.empty() || password.empty() || email.empty())
			{
				str = Control::GetString("REGISTER_PLEASE_FILL");
			}
			else if(g_env->svc.UserExists(username.c_str()))
			{
				str = Control::GetString("REGISTER_USER_EXITS");
			}

			if(str.empty())
			{
				if(g_env->svc.RegisterUser(username.c_str(), password.c_str(), email.c_str()))
				{
					DoneEvent(this);
				}
				else
				{
					str = Control::GetString("REGISTER_FAILED");
				}
			}
		}

		if(!str.empty())
		{
			m_dialog.m_message.Text = str;
			m_dialog.Show();
		}

		return true;
	}
}