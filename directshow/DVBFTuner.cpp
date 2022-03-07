#pragma once

#include "stdafx.h"
#include "DVBFTuner.h"
#include "DirectShow.h"

DVBFTuner::DVBFTuner()
	: m_loaded(false)
{
	m_dvb.type = (DVBSystemType)-1;
}

DVBFTuner::~DVBFTuner()
{
}

bool DVBFTuner::CreateNetworkProvider(IBaseFilter** ppNP)
{
	HRESULT hr;

	CComPtr<IBaseFilter> pNP = new CDVBFTunerNetworkProvider(NULL, &hr);

	if(FAILED(hr))
	{
		return false;
	}

	*ppNP = pNP.Detach();

	return true;
}

bool DVBFTuner::GetTuningSpace(ITuningSpace** ppTS)
{
	return false;
}

HRESULT DVBFTuner::TuneDVB(const DVBPresetParam* p)
{
	m_loaded = false;

	if(const DVBFPresetParam* pp = dynamic_cast<const DVBFPresetParam*>(p))
	{
		if(CComQIPtr<IFileSourceFilter> p = m_networkprovider)
		{
			if(SUCCEEDED(p->Load(pp->path.c_str(), NULL)))
			{
				m_loaded = true;
			}
		}
	}

	return __super::TuneDVB(p);
}

int DVBFTuner::GetSignalStrength()
{
	return m_loaded ? 1 : 0;
}

// CDVBFTunerNetworkProvider

CDVBFTunerNetworkProvider::CDVBFTunerNetworkProvider(LPUNKNOWN lpunk, HRESULT* phr)
	: CSource(NAME("CDVBFTunerNetworkProvider"), lpunk, __uuidof(this))
{
	new CDVBFTunerStream(this, phr);
}

CDVBFTunerNetworkProvider::~CDVBFTunerNetworkProvider()
{
}

STDMETHODIMP CDVBFTunerNetworkProvider::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IFileSourceFilter)
		QI(IAMFilterMiscFlags)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CDVBFTunerNetworkProvider::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt) 
{
	if(GetPinCount() == 0)
	{
		return E_UNEXPECTED;
	}

	m_path.clear();

	if(FAILED(((CDVBFTunerStream*)m_paStreams[0])->Load(pszFileName)))
	{
		return E_FAIL;
	}

	m_path = pszFileName;

	return S_OK;
}

STDMETHODIMP CDVBFTunerNetworkProvider::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_path.size() + 1) * sizeof(WCHAR));
	
	if(*ppszFileName == NULL)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName,  m_path.c_str());

	return S_OK;
}

// IAMFilterMiscFlags

ULONG CDVBFTunerNetworkProvider::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

// CDVBFTunerStream

CDVBFTunerStream::CDVBFTunerStream(CDVBFTunerNetworkProvider* pParent, HRESULT* phr)
	: CSourceStream(_T("Output"), phr, pParent, L"Output")
	, m_fp(NULL)
{
}

CDVBFTunerStream::~CDVBFTunerStream()
{
	if(m_fp) fclose(m_fp);
}

HRESULT CDVBFTunerStream::Load(LPCWSTR path)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

	HRESULT hr = E_FAIL;

	Util::Socket::Startup();

	if(m_fp != NULL)
	{
		fclose(m_fp);

		m_fp = NULL;
	}
	
	m_socket.Close();

	if(path == NULL || wcslen(path) == 0)
	{
		return E_FAIL;
	}

	if(wcsstr(path, L"://") != NULL)
	{
		std::map<std::string, std::string> headers;

		if(m_socket.HttpGet(path, headers))
		{
			m_fs = m_socket.GetContentLength();

			hr = S_OK;
		}
	}
	else
	{
		m_fp = _tfopen(path, _T("rb"));

		if(m_fp == NULL)
		{
			return E_FAIL;
		}

		_fseeki64(m_fp, 0, SEEK_END);
	
		m_fs = _ftelli64(m_fp);

		hr = S_OK;
	}

	Util::Socket::Cleanup();

	return hr;
}

HRESULT CDVBFTunerStream::OnThreadCreate()
{
	Util::Socket::Startup();

	return __super::OnThreadCreate();
}

HRESULT CDVBFTunerStream::OnThreadDestroy()
{
	Util::Socket::Cleanup();

	return __super::OnThreadDestroy();
}

HRESULT CDVBFTunerStream::OnThreadStartPlay()
{
	if(m_fp != NULL) 
	{
		fseek(m_fp, 0, SEEK_SET);
	}
	
	return __super::OnThreadStartPlay();
}

HRESULT CDVBFTunerStream::FillBuffer(IMediaSample* pSample)
{
	HRESULT hr;

	BYTE* pData = NULL;

	if(FAILED(hr = pSample->GetPointer(&pData)) || pData == NULL)
	{
		return S_FALSE;
	}

    CAutoLock cAutoLock(m_pFilter->pStateLock());

	if(m_fp != NULL)
	{
		if(feof(m_fp))
		{
			_fseeki64(m_fp, 0, SEEK_SET);

			DeliverNewSegment(0, m_fs, 0);
		}

		Sleep(5);

		int size = fread(pData, 1, 65536, m_fp);

		pSample->SetActualDataLength(size);
	}
	else if(m_socket.IsOpen())
	{
		int size = m_socket.Read(pData, 65536);

		if(size < 0) return E_FAIL;

		pSample->SetActualDataLength(size);
	}

	return S_OK;
}

HRESULT CDVBFTunerStream::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 65536;

    return NOERROR;
}

HRESULT CDVBFTunerStream::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

    pmt->SetType(&MEDIATYPE_Stream);
    pmt->SetSubtype(&MEDIASUBTYPE_MPEG2_TRANSPORT);
    pmt->SetFormatType(&GUID_NULL);

    return S_OK;
}

HRESULT CDVBFTunerStream::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_MPEG2_TRANSPORT 
		? S_OK
		: E_INVALIDARG;
}
