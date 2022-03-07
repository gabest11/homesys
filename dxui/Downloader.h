#pragma once

#include "Thread.h"

namespace DXUI
{
	class Downloader
	{
	public:
		enum {None, InProgress, Success, Error}; 

	private:
		std::wstring m_url;
		std::wstring m_path;
		UINT64 m_position;
		UINT64 m_size;
		std::wstring m_type;
		int m_status;
		int m_result;

		void SetUrl(std::wstring& value);
		void GetPath(std::wstring& value);
		void GetPosition(UINT64& value);
		void GetSize(UINT64& value);
		void GetContentType(std::wstring& value);
		void GetStatus(int& value);
		void GetResult(int& value);
		void GetProgress(float& value);
		void GetProgressText(std::wstring& value);

		Thread m_thread;

		bool OnThreadInit(Control* c);

		int Download();

	public:
		Downloader();

		Property<std::wstring> Url;
		Property<std::wstring> Path;
		Property<UINT64> Position;
		Property<UINT64> Size;
		Property<std::wstring> ContentType;
		Property<int> Status;
		Property<int> Result;
		Property<float> Progress;
		Property<std::wstring> ProgressText;

		Event<int> DoneEvent;
	};
}
