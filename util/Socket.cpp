#include "stdafx.h"
#include "Socket.h"
#include "String.h"

using namespace std;

namespace Util
{
	Socket::Socket()
		: m_socket(INVALID_SOCKET)
		, m_port(0)
		, m_connected(false)
		, m_canseek(false)
		, m_error(0)
	{
	}

	Socket::~Socket()
	{
		Close();
	}

	bool Socket::Open(LPCSTR host, int port)
	{
		Close();

		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if(m_socket == INVALID_SOCKET)
		{
			return false;
		}

		ULONG addr = 0;

		if(inet_addr(host) == INADDR_NONE)
		{
		    hostent* h = gethostbyname(host);

			if(h == NULL)
			{
				Close();

				return false;
			}

			addr = *(DWORD*)h->h_addr;
		}
		else
		{
		    addr = inet_addr(host);
		}

		sockaddr_in server;

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = addr;
		server.sin_port = htons(port);
		
		if(connect(m_socket, (sockaddr*)&server, sizeof(server)) != 0)
		{
			Close();

			return false;
		}

		m_port = port;

		m_connected = true;

		return true;
	}

	bool Socket::Open(const Url& url)
	{
		return Open(url.host.c_str(), url.port);
	}

	bool Socket::Listen(LPCSTR host, int port)
	{
		Close();

		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if(m_socket == INVALID_SOCKET)
		{
			return false;
		}

		u_long iMode = 1;

		if(ioctlsocket(m_socket, FIONBIO, &iMode) != 0)
		{
			Close();

			return false;
		}

		ULONG addr = INADDR_ANY;

		if(host != NULL)
		{
			if(inet_addr(host) == INADDR_NONE)
			{
				hostent* h = gethostbyname(host);

				if(h == NULL)
				{
					Close();

					return false;
				}

				addr = *(DWORD*)h->h_addr;
			}
			else
			{
				addr = inet_addr(host);
			}
		}

		sockaddr_in server;

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = addr;
		server.sin_port = htons(port);
		
		if(bind(m_socket, (sockaddr*)&server, sizeof(server)) != 0)
		{
			Close();

			return false;
		}

		if(port == 0)
		{
			int len = sizeof(server);

			if(getsockname(m_socket, (sockaddr*)&server, &len) == 0)
			{
				port = ntohs(((sockaddr_in*)&server)->sin_port);
			}
		}
		
		if(listen(m_socket, SOMAXCONN) != 0)
		{
			Close();

			return false;
		}

		m_port = port;

		m_connected = true;

		return true;
	}

	bool Socket::Accept(Socket& s)
	{
		s.Close();

		s.m_socket = accept(m_socket, NULL, NULL);

		if(s.m_socket == INVALID_SOCKET)
		{
			int error = WSAGetLastError();

			if(error != WSAEWOULDBLOCK)
			{
				Close();
			}

			return false;
		}

		u_long iMode = 0;

		if(ioctlsocket(s.m_socket, FIONBIO, &iMode) != 0)
		{
			s.Close();

			return false;
		}

		// TODO: s.m_port = ;

		s.m_connected = true;

		return true;
	}

	bool Socket::Write(BYTE b)
	{
		return Write(&b, 1);
	}

	bool Socket::Write(const void* buff, int len)
	{
		m_error = 0;

		for(int i = 0, sent; i < len; i += sent)
		{
			sent = send(m_socket, &((const char*)buff)[i], len, 0);

			if(sent == SOCKET_ERROR)
			{
				m_error = WSAGetLastError();

				printf("Socket write error = %d\n", m_error);

				if(m_error == WSAECONNRESET)
				{
					m_connected = false;
				}

				return false;
			}
		}

		return true;
	}

	void Socket::Close()
	{
		if(m_socket != INVALID_SOCKET)
		{
			closesocket(m_socket);

			m_socket = INVALID_SOCKET;
		}

		m_port = 0;

		m_connected = false;
	}

	int Socket::Read(void* buff, int len)
	{
		m_error = 0;

		int n = recv(m_socket, (char*)buff, len, 0);

		if(n <= 0)
		{
			if(n == 0)
			{
				m_connected = false;
			}
			else
			{
				m_error = WSAGetLastError();

				printf("Socket read error = %d\n", m_error);
			}
		}

		return n;
	}

	bool Socket::ReadAll(void* buff, int len)
	{
		for(int i = 0, n; i < len; i += n)
		{
			n = Read(&((BYTE*)buff)[i], len - i);

			if(n <= 0)
			{
				return false;
			}
		}

		return true;
	}

	bool Socket::ReadToEnd(std::vector<BYTE>& buff, int limit)
	{
		buff.clear();

		std::vector<BYTE> tmp(65536);

		while(limit > 0)
		{
			int n = Read(tmp.data(), tmp.size());

			if(n <= 0)
			{
				if(n < 0)
				{
					return false;
				}

				break;
			}

			n = std::min<int>(n, limit);

			buff.insert(buff.end(), tmp.begin(), tmp.begin() + n);

			limit -= n;
		}

		return true;
	}

	bool Socket::Write(const std::string& s)
	{
		return Write((const BYTE*)s.c_str(), s.size());
	}

	bool Socket::Read(std::string& s)
	{
		// TODO: m_connected = false;

		s.clear();

		while(1)
		{
			char c;
			int ret;

			if((ret = recv(m_socket, &c, 1, 0)) != 1)
			{
				return ret == 0;
			}

			if(c == '\r')
			{
				if((ret = recv(m_socket, &c, 1, MSG_PEEK)) != 1)
				{
					return ret == 0;
				}

				if(c == '\n')
				{
					if((ret = recv(m_socket, &c, 1, 0)) != 1)
					{
						return ret == 0;
					}
					
					return true;
				}
			}

			s += c;
		}
	}

	bool Socket::HttpGet(LPCSTR url, map<string, string>& headers, int major, int minor)
	{
		Url u(url);

		return HttpGet(u, headers, major, minor);
	}

	bool Socket::HttpGetXML(LPCSTR url, std::map<std::string, std::string>& headers, const std::list<std::string> types, std::string& xml, int major, int minor)
	{
		Url u(url);

		return HttpGetXML(u, headers, types, xml, major, minor);
	}

	bool Socket::HttpGet(const Url& url, map<string, string>& headers, int major, int minor)
	{
		if(url.protocol != "http" && url.protocol != "https") // TODO: https is not implemented, just hope it responds on port 80...
		{
			return false;
		}

		if(!Open(url.host.c_str(), url.port))
		{
			return false;
		}

		m_content.type.clear();
		m_content.length = 0;
		m_content.from = 0;
		m_content.to = 0;

		m_canseek = false;

		std::string request = url.request;
		
		Util::Replace(request, " ", "%20");

		Write("GET " + request + " HTTP/" + Util::Format("%d.%d", major, minor) + "\r\n");
		Write("Accept: */*\r\n");
		Write("Host: " + url.host + "\r\n");
		Write("Connection: close\r\n"); // TODO: keep-alive

		for(auto i = headers.begin(); i != headers.end(); i++)
		{
			Write(i->first + ": " + i->second + "\r\n");
		}

		Write("\r\n");

		string s;

		if(!Read(s) || s.find("HTTP") != 0 && s.find("ICY 200 OK") != 0) // TODO: check code
		{
			return false;
		}

		map<string, string> h;

		std::string::size_type i = s.find(' ');

		if(i != std::string::npos)
		{
			h["HTTP_RES"] = s.c_str() + i + 1;
		}

		while(Read(s))
		{
			if(s.empty())
			{
				break;
			}

			string::size_type i = s.find(':');

			if(i == string::npos)
			{
				continue;
			}

			string key = s.substr(0, i);
			string value = s.substr(i + 1);

			i = value.find_first_not_of(' ');

			if(i != string::npos)
			{
				value = value.substr(i);
			}
			else
			{
				value.clear();
			}

			if(stricmp(key.c_str(), "Location") == 0)
			{
				return HttpGet(value.c_str(), headers);
			}

			h[key] = value;

			if(stricmp(key.c_str(), "Content-Type") == 0)
			{
				m_content.type = value;

				i = value.find(';');

				if(i != string::npos)
				{
					m_content.type = value.substr(0, i);
					m_content.charset = Util::Trim(value.substr(i + 1));
				}
			}
			else if(stricmp(key.c_str(), "Content-Length") == 0)
			{
				m_content.length = m_content.from + _atoi64(value.c_str());
			}
			else if(stricmp(key.c_str(), "Content-Range") == 0)
			{
				__int64 from, to, length;

				if(sscanf(value.c_str(), "bytes %I64d-%I64d/%I64d", &from, &to, &length) == 3)
				{
					m_content.from = from;
					m_content.to = to;
					m_content.length = length;

					m_canseek = true;
				}
			}
		}

		headers = h;

		return true;
	}

	bool Socket::HttpGetXML(const Url& url, std::map<std::string, std::string>& headers, const std::list<std::string> types, std::string& xml, int major, int minor)
	{
		xml.clear();

		if(!HttpGet(url, headers, major, minor))
		{
			return false;
		}

		if(!types.empty())
		{
			std::string ct = GetContentType();

			bool found = false;

			for(auto i = types.begin(); i != types.end(); i++)
			{
				if(stricmp(i->c_str(), ct.c_str()) == 0)
				{
					found = true;
				}
			}

			if(!found) 
			{
				return false;
			}
		}

		int length = (int)GetContentLength();

		std::vector<BYTE> buff;

		if(!ReadToEnd(buff, length > 0 ? length : 10 << 20) || buff.empty())
		{
			return false;
		}

		if(buff.size() > 2 && buff[0] == 0xff && buff[1] == 0xfe)
		{
			xml = Util::UTF16To8(std::wstring((LPCWSTR)buff.data() + 1, buff.size() / 2 - 1).c_str());
		}
		else if(wcsstr((LPCWSTR)buff.data(), L"utf-16") != NULL || wcsstr((LPCWSTR)buff.data(), L"UTF-16") != NULL)
		{
			xml = Util::UTF16To8(std::wstring((LPCWSTR)buff.data(), buff.size() / 2).c_str());
		}
		else
		{
			int offset = 0;

			for(; offset < buff.size(); offset++)
			{
				if(buff[offset] == '<') break;
			}

			xml = std::string((LPCSTR)buff.data() + offset, buff.size() - offset);
		}

		return true;
	}

	Socket::Url::Url(LPCSTR url)
	{
		Parse(url);
	}

	Socket::Url::Url(LPCWSTR url)
	{
		std::string s = UTF16To8(url);

		Parse(s.c_str());
	}

	void Socket::Url::Parse(LPCSTR url)
	{
		value = url;

		string s = url;

		string::size_type i = s.find("://", 0);

		if (i == string::npos)
		{
			return;
		}

		protocol = Util::MakeLower(s.substr(0, i));

		s = s.substr(i + 3);

		if (s.empty())
		{
			return;
		}

		i = s.find_first_of(":/");

		if (i == string::npos)
		{
			i = s.size();
		}

		host = s.substr(0, i);

		s = s.substr(i);

		if (!s.empty() && s[0] == ':')
		{
			i = s.find('/');

			if (i == string::npos)
			{
				i = s.size();
			}

			port = atoi(s.c_str() + 1);

			s = s.substr(i);
		}
		else
		{
			if (protocol == "rtmp" || protocol == "bbc")
			{
				port = 1935;
			}
			else // if(protocol == "http")
			{
				port = 80;
			}
		}

		if (s.empty())
		{
			request = '/';
		}
		else
		{
			i = s.find('#');

			if (i == string::npos)
			{
				i = s.size();
			}

			request = s.substr(0, i);

			if (i < s.size() - 1)
			{
				extra = s.substr(i + 1);
			}
		}
	}
}