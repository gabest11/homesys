#pragma once

#include <Winsock.h>
#include <string>
#include <map>

namespace Util
{
	class Socket
	{
	protected:
		SOCKET m_socket;
		WORD m_port;
		bool m_connected;
		struct {std::string type, charset; __int64 length, from, to;} m_content;
		bool m_canseek;
		int m_error;

	public:
		class Url
		{
			void Parse(LPCSTR url);

		public:
			std::string value;
			std::string protocol;
			std::string host;
			int port;
			std::string request;
			std::string extra;

			Url(LPCSTR url);
			Url(LPCWSTR url);
		};

	public:
		Socket();
		virtual ~Socket();

		bool IsOpen() {return m_socket != INVALID_SOCKET && m_connected;}
		WORD GetPort() const {return m_port;}

		virtual bool Open(LPCSTR host, int port);
		virtual bool Open(const Url& url);
		virtual bool Listen(LPCSTR host, int port);
		virtual bool Accept(Socket& s);
		virtual void Close();

		virtual bool Write(const void* buff, int len);
		virtual int Read(void* buff, int len);

		bool Write(BYTE b);
		bool ReadAll(void* buff, int len);
		bool ReadToEnd(std::vector<BYTE>& buff, int limit);

		bool Write(const std::string& s);
		bool Read(std::string& s);

		bool HttpGet(LPCSTR url, std::map<std::string, std::string>& headers, int major = 1, int minor = 1);
		bool HttpGetXML(LPCSTR url, std::map<std::string, std::string>& headers, const std::list<std::string> types, std::string& xml, int major = 1, int minor = 1);
		bool HttpGet(const Url& url, std::map<std::string, std::string>& headers, int major = 1, int minor = 1);
		bool HttpGetXML(const Url& url, std::map<std::string, std::string>& headers, const std::list<std::string> types, std::string& xml, int major = 1, int minor = 1);

		const std::string GetContentType() const {return m_content.type.c_str();}
		__int64 GetContentLength() const {return m_content.length;}
		__int64 GetContentStart() const {return m_content.from;}
		__int64 GetContentEnd() const {return m_content.to;}

		bool CanSeek() const {return m_canseek;}

		static bool Startup() {WSADATA wsaData; return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;}
		static void Cleanup() {WSACleanup();}
	};
}