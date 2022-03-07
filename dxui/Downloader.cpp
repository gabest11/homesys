#include "stdafx.h"
#include "Downloader.h"
#include "Localizer.h"
#include "../util/String.h"
#include "../util/Socket.h"

namespace DXUI
{
	Downloader::Downloader()
		: m_position(0)
		, m_size(0)
		, m_status(None)
		, m_result(0)
	{
		Url.Set(&Downloader::SetUrl, this);
		Path.Get(&Downloader::GetPath, this);
		Position.Get(&Downloader::GetPosition, this);
		Size.Get(&Downloader::GetSize, this);
		ContentType.Get(&Downloader::GetContentType, this);
		Status.Get(&Downloader::GetStatus, this);
		Result.Get(&Downloader::GetResult, this);
		Progress.Get(&Downloader::GetProgress, this);
		ProgressText.Get(&Downloader::GetProgressText, this);

		m_thread.InitEvent.Add0(&Downloader::OnThreadInit, this, true);
	}

	void Downloader::SetUrl(std::wstring& value)
	{
		m_thread.Join();

		if(!m_path.empty())
		{
			DeleteFile(m_path.c_str());
		}

		m_url = value;
		m_path.clear();
		m_position = 0;
		m_size = 0;
		m_status = None;
		m_result = 0;

		if(!m_url.empty()) 
		{
			m_thread.Create();
		}
	}

	void Downloader::GetPath(std::wstring& value)
	{
		CAutoLock cAutoLock(&m_thread);

		value = m_path;
	}

	void Downloader::GetPosition(UINT64& value)
	{
		CAutoLock cAutoLock(&m_thread);

		value = m_position;
	}

	void Downloader::GetSize(UINT64& value)
	{
		CAutoLock cAutoLock(&m_thread);

		value = m_size;
	}

	void Downloader::GetContentType(std::wstring& value)
	{
		CAutoLock cAutoLock(&m_thread);

		value = m_type;
	}

	void Downloader::GetStatus(int& value)
	{
		CAutoLock cAutoLock(&m_thread);

		value = m_status;
	}

	void Downloader::GetResult(int& value)
	{
		CAutoLock cAutoLock(&m_thread);

		value = m_result;
	}

	void Downloader::GetProgress(float& value)
	{
		value = m_size > 0 && m_position <= m_size ? 1.0f * m_position / m_size : 0;
	}

	void Downloader::GetProgressText(std::wstring& value)
	{
		value.clear();

		switch(m_status)
		{
		case Downloader::InProgress:
			value = Util::Format(L"%d%%", m_size > 0 ? (int)(m_position * 100 / m_size) : 0);
			break;
		case Downloader::Success:
			value = L"100%";
			break;
		case Downloader::Error:
			value = Control::GetString("ERROR");
			break;
		}
	}

	bool Downloader::OnThreadInit(Control* c)
	{
		Util::Socket::Startup();

		m_status = Download();

		Util::Socket::Cleanup();

		DoneEvent(NULL, m_status);

		return false;
	}

	int Downloader::Download()
	{
		Util::Socket s;

		std::wstring url;
		std::wstring path;

		{
			CAutoLock cAutoLock(&m_thread);

			Util::Socket::Url u(m_url.c_str());

			if(u.protocol != "http" || u.host.empty() || u.request.empty())
			{
				return Error;
			}

			WCHAR buff[MAX_PATH];

			GetTempPath(MAX_PATH, buff);

			path = buff;

			GetTempFileName(path.c_str(), L"dl", 0, buff);

			m_path = buff;
			m_status = InProgress;

			url = m_url;
			path = m_path;
		}

		std::map<std::string, std::string> headers;

		if(!s.HttpGet(url.c_str(), headers))
		{
			return Error;
		}

		{
			CAutoLock cAutoLock(&m_thread);

			std::string ct = s.GetContentType();

			m_size = s.GetContentLength();
			m_type = std::wstring(ct.begin(), ct.end());
		}

		int len = -1;

		if(FILE* f = _wfopen(path.c_str(), L"wb"))
		{
			BYTE* buff = new BYTE[65536];

			while(m_thread && (len = s.Read(buff, 65536)) > 0)
			{
				fwrite(buff, len, 1, f);

				CAutoLock cAutoLock(&m_thread);

				m_position += len;
				m_status = InProgress;
			}

			fclose(f);

			delete [] buff;
		}

		return len == 0 ? Success : Error;
	}
}