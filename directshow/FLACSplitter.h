#pragma once

#include "BaseSplitter.h"

[uuid("9279C68C-741E-43B9-A99D-41D984F46687")]
class CFLACSplitterFilter : public CBaseSplitterFilter
{
protected:
	struct SeekPoint {UINT64 pos, offset;};

	CBaseSplitterFile* m_file;
	CBaseSplitterFile::FLACStreamHeader m_stream;
	CBaseSplitterFile::FLACFrameHeader m_frame;
	std::vector<SeekPoint> m_index;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CFLACSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CFLACSplitterFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

[uuid("0A68C3B5-9164-4a54-AFBF-995B2FF0E0D4")]
class CFLACSourceFilter : public CFLACSplitterFilter
{
public:
	CFLACSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
