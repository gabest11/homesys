#pragma once

class LoaderThread 
	: public CCritSec
	, public DXUI::ResourceLoader
{
	struct WorkItem {std::wstring url, offline; int status; bool priority;};

	std::map<std::wstring, WorkItem> m_items;
	std::list<std::wstring> m_queue;

	DWORD ThreadProc();
	
	void QueueItem(const std::wstring hash, const WorkItem& item);

	// ResourceLoader

	bool GetResource(LPCWSTR id, std::vector<BYTE>& buff, bool priority = false, bool refresh = false);
	void IgnoreResource(LPCWSTR id);

	std::set<std::wstring> m_ignored;

	int LoadResource(LPCWSTR id, std::vector<BYTE>& buff, bool priority = false, bool refresh = false);

	std::wstring m_skin;

	class HelperThread : public CAMThread
	{
		LoaderThread* m_parent;

	public:
		HelperThread(LoaderThread* parent) : m_parent(parent) {}

		DWORD ThreadProc();
	}; 
	
	std::list<HelperThread*> m_threads;

public:
	LoaderThread();
	virtual ~LoaderThread();
};
