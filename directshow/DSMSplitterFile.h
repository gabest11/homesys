#pragma once

#include "BaseSplitter.h"
#include "DSMPropertyBag.h"
#include "DSM.h"

class CDSMSplitterFile : public CBaseSplitterFile
{
	DSMKey* m_key;

	HRESULT Init(IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap);

public:
	CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr, IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap);
	virtual ~CDSMSplitterFile();

	std::map<BYTE, CMediaType> m_mts;
	struct {REFERENCE_TIME first, dur;} m_time;

	struct SyncPoint {REFERENCE_TIME m_rt; __int64 m_fp;};
	std::vector<SyncPoint> m_sps;

	typedef std::map<std::wstring, std::wstring> StreamInfoMap;
	
	StreamInfoMap m_fim;
	std::map<BYTE, StreamInfoMap> m_sim;

	bool Sync(dsmp_t& type, UINT64& len, __int64 limit = 65536);
	bool Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, __int64 limit = 65536);
	bool Read(__int64 len, BYTE& id, CMediaType& mt);
	bool Read(__int64 len, CPacket* p, bool fData = true);
	bool Read(__int64 len, std::vector<SyncPoint>& sps);
	bool Read(__int64 len, StreamInfoMap& im);
	bool Read(__int64 len, IDSMResourceBagImpl& res);
	bool Read(__int64 len, IDSMChapterBagImpl& chap);
	__int64 Read(__int64 len, std::wstring& str);
	
	__int64 FindSyncPoint(REFERENCE_TIME rt);
};
