#pragma once

#include "BaseMuxer.h"
#include "DSM.h"

[uuid("9F7F3697-E2F3-47E5-8898-91861A71783E")]
interface IDSMMuxerFilter : public IUnknown
{
	STDMETHOD(SetKey) (const DSMKey* key) = 0;
	STDMETHOD(GetKeyPos) (__int64* pos, int* len) = 0;
};

[uuid("C6590B76-587E-4082-9125-680D0693A97B")]
class CDSMMuxerFilter : public CBaseMuxerFilter, public IDSMMuxerFilter
{
	bool m_fAutoChap, m_fAutoRes;

	struct SyncPoint {BYTE id; REFERENCE_TIME rtStart, rtStop; __int64 fp;};
	struct IndexedSyncPoint {BYTE id; REFERENCE_TIME rt, rtfp; __int64 fp;};
	std::list<SyncPoint> m_sps;
	std::list<IndexedSyncPoint> m_isps;
	REFERENCE_TIME m_rtPrevSyncPoint;
	void IndexSyncPoint(const CMuxerPacket* p, __int64 fp);

	void MuxPacketHeader(IBitStream* pBS, dsmp_t type, UINT64 len);
	void MuxFileInfo(IBitStream* pBS);
	void MuxStreamInfo(IBitStream* pBS, CBaseMuxerInputPin* pin);

	DSMKey* m_key;
	std::vector<BYTE> m_data;
	__int64 m_keypos;
	int m_keylen;

protected:
	void MuxInit();
	void MuxHeader(IBitStream* pBS);
	void MuxPacket(IBitStream* pBS, const CMuxerPacket* pPacket);
	void MuxFooter(IBitStream* pBS);

public:
	CDSMMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, bool fAutoChap = true, bool fAutoRes = true);
	virtual ~CDSMMuxerFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	STDMETHODIMP SetKey(const DSMKey* key);
	STDMETHODIMP GetKeyPos(__int64* pos, int* len);
};
