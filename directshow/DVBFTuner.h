#pragma once

#include "DVBTuner.h"
#include "../Util/Socket.h"

class DVBFPresetParam : public DVBPresetParam
{
public:
	std::wstring path;

	DVBFPresetParam()
	{
	}
	std::wstring ToString() const
	{
		return __super::ToString() + Util::Format(
			L", path=%d",
			path.c_str()
			);
	}
};

class DVBFTuner : public DVBTuner
{
	bool m_loaded;

protected:
	bool CreateNetworkProvider(IBaseFilter** ppNP);
	bool GetTuningSpace(ITuningSpace** ppTS);
	HRESULT TuneDVB(const DVBPresetParam* p);

public:
	DVBFTuner();
	virtual ~DVBFTuner();

	int GetSignalStrength();
};

[uuid("E08A4FCE-4B6A-4CCB-BDB3-FDACB1D7652F")]
class CDVBFTunerNetworkProvider
	: public CSource
	, public IFileSourceFilter
	, public IAMFilterMiscFlags
{
	std::wstring m_path;

public:
	CDVBFTunerNetworkProvider(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CDVBFTunerNetworkProvider();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IAMFilterMiscFlags

	STDMETHODIMP_(ULONG) GetMiscFlags();
};

class CDVBFTunerStream : public CSourceStream
{
	Util::Socket m_socket;

	FILE* m_fp;
	__int64 m_fs;

	HRESULT OnThreadCreate();
	HRESULT OnThreadDestroy();
	HRESULT OnThreadStartPlay();

public:
    CDVBFTunerStream(CDVBFTunerNetworkProvider* pParent, HRESULT* phr);
	virtual ~CDVBFTunerStream();

	HRESULT Load(LPCWSTR path);

    HRESULT FillBuffer(IMediaSample* pSample);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT CheckMediaType(const CMediaType* pMediaType);

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return E_NOTIMPL;}
};