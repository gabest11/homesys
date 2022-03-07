#include "stdafx.h"
#include "ImageGrabber.h"
#include "../../DirectShow/Filters.h"
#include "../../DirectShow/MediaTypeEx.h"
#include "../../DirectShow/BitBlt.h"

namespace DXUI
{
	[uuid("1DEE2E2A-8755-4988-A29D-5300548D6D19")]
	class CImageGrabberFilter : public CBaseFilter, public CCritSec
	{
		class CImageGrabberInputPin : public CBaseInputPin
		{
		public:
			CImageGrabberInputPin(CImageGrabberFilter* pFilter, HRESULT* phr) 
				: CBaseInputPin(L"", pFilter, pFilter, phr, L"Input") 
			{
				m_bCanReconnectWhenActive = TRUE;
			}
 
			HRESULT CheckMediaType(const CMediaType* pmt)
			{
				return pmt->majortype == MEDIATYPE_Video 
					&& (pmt->subtype == MEDIASUBTYPE_RGB32 
					|| pmt->subtype == MEDIASUBTYPE_ARGB32 
					// || pmt->subtype == MEDIASUBTYPE_RGB24 
					// || pmt->subtype == MEDIASUBTYPE_YUY2
					|| pmt->subtype == MEDIASUBTYPE_YV12)
					&& (pmt->formattype == FORMAT_VideoInfo
					|| pmt->formattype == FORMAT_VideoInfo2)
					? S_OK 
					: E_FAIL;
			}

			const CMediaType& CurrentMediaType()
			{
				return m_mt;
			}

			STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
			{
				if(m_Connected && m_Connected != pConnector)
				{
					return VFW_E_ALREADY_CONNECTED;
				}

				if(m_Connected) 
				{
					m_Connected->Release();
					m_Connected = NULL;
				}

				return __super::ReceiveConnection(pConnector, pmt);
			}

			STDMETHODIMP Receive(IMediaSample* pIn)
			{
				AM_MEDIA_TYPE* pmt = NULL;

				if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
				{
					CMediaType mt = *pmt;
					SetMediaType(&mt);
					DeleteMediaType(pmt);
				}

				return ((CImageGrabberFilter*)m_pFilter)->Receive(pIn);
			}
		};

		CImageGrabberInputPin* m_pInput;
		CAMEvent& m_evReceive;
		ImageGrabber::Bitmap* m_bm;

	public:
		CImageGrabberFilter(ImageGrabber::Bitmap* bm, CAMEvent& evReceive)
			: CBaseFilter(L"", NULL, this, __uuidof(this))
			, m_bm(bm)
			, m_evReceive(evReceive)
		{
			HRESULT hr;

			m_pInput = new CImageGrabberInputPin(this, &hr);
		}

		virtual ~CImageGrabberFilter()
		{
			delete m_pInput;
		}

		const ImageGrabber::Bitmap* GetBitmap()
		{
			return m_bm;
		}

		int GetPinCount() 
		{
			return 1;
		}		
		
		CBasePin* GetPin(int n)
		{
			return n == 0 ? m_pInput : NULL;
		}

		HRESULT Receive(IMediaSample* pIn)
		{
			if(S_OK == pIn->IsPreroll())
			{
				return S_OK;
			}

			BYTE* ptr = NULL;
			pIn->GetPointer(&ptr);

			if(!m_bm->bits && ptr)
			{
				CMediaTypeEx mt = m_pInput->CurrentMediaType();

				BITMAPINFOHEADER bih;
				
				if(mt.ExtractBIH(&bih))
				{
					m_bm->width = bih.biWidth;
					m_bm->height = abs(bih.biHeight);
					m_bm->bits = new DWORD[m_bm->width * m_bm->height];

					if(mt.subtype == MEDIASUBTYPE_RGB32 || mt.subtype == MEDIASUBTYPE_ARGB32)
					{
						int pitch = m_bm->width * 4;
						
						if(bih.biHeight > 0) 
						{
							ptr += pitch * (m_bm->height - 1); 
							pitch = -pitch;
						}
						
						Image::RGBToRGB(m_bm->width, m_bm->height, (BYTE*)m_bm->bits, m_bm->width * 4, 32, ptr, pitch, 32);
					}
					else if(mt.subtype == MEDIASUBTYPE_RGB24)
					{
						//pIn->GetSize();
						int pitch = (m_bm->width * 3 + 3) & ~3;
	
						if(bih.biHeight > 0)
						{
							ptr += pitch * (m_bm->height - 1); 
							pitch = -pitch;
						}

						Image::RGBToRGB(m_bm->width, m_bm->height, (BYTE*)m_bm->bits, m_bm->width * 4, 32, ptr, pitch, 24);
					}
					else if(mt.subtype == MEDIASUBTYPE_YUY2)
					{
						Image::YUY2ToRGB(m_bm->width, m_bm->height, (BYTE*)m_bm->bits, m_bm->width * 4, 32, ptr, m_bm->width*2);
					}
					else if(mt.subtype == MEDIASUBTYPE_YV12)
					{
						BYTE* y = ptr;
						BYTE* v = y + m_bm->width*m_bm->height;
						BYTE* u = v + (m_bm->width / 2) * (m_bm->height / 2);

						Image::I420ToRGB(m_bm->width, m_bm->height, (BYTE*)m_bm->bits, m_bm->width * 4, 32, y, u, v, m_bm->width);
					}

					// FIXME
					for(int i = 0, j = m_bm->width * m_bm->height; i < j; i++)
					{
						m_bm->bits[i] |= 0xff000000;
					}
				}
			}

			m_evReceive.Set();

			return S_OK;
		}
	};

	// ImageGrabber

	ImageGrabber::ImageGrabber()
	{
		m_pGB = new CFGManagerCustom(_T(""), NULL);
	}

	bool ImageGrabber::GetInfo(LPCWSTR path, float at, Bitmap* bm, std::map<std::wstring, std::wstring>* tags, REFERENCE_TIME* pdur, int timeout)
	{
		if(pdur != NULL)
		{
			*pdur = -1;
		}

		if(tags)
		{
			tags->clear();
		}

		if(bm)
		{
			std::wstring fn = path;

			if(PathIsDirectory(path))
			{
				if(wcscmp(path, L".") == 0 || wcscmp(path, L"..") == 0)
				{
					return false;
				}

				fn = Util::CombinePath(path, L"folder.jpg");

				if(!PathFileExists(fn.c_str()))
				{
					return false;
				}
			}

			if(LPCWSTR s = PathFindExtension(fn.c_str()))
			{
				std::wstring ext = Util::MakeLower(s);

				if(ext == L".jpg" || ext == L".jpeg" || ext == L".png" || ext == L".gif" || ext == L".bmp")
				{
					Util::ImageDecompressor id;
					
					if(id.Open(fn.c_str()))
					{
						bm->width = id.m_width;
						bm->height = id.m_height;
						bm->bits = new DWORD[bm->width * bm->height];
						
						Image::RGBToRGB(bm->width, bm->height, (BYTE*)bm->bits, bm->width * 4, 32, id.m_pixels, id.m_pitch, 32);
						
						for(int i = 0, j = bm->width * bm->height; i < j; i++) bm->bits[i] |= 0xff000000; // FIXME

						return true;
					}
				}
			}
		}

		if(bm || tags || pdur)
		{
			HRESULT hr;

			m_pGB->Reset();

			do
			{
				CComPtr<IBaseFilter> pBF;

				hr = m_pGB->HasRegisteredSourceFilter(path);

				if(S_OK != hr) {hr = E_FAIL; break;}

				hr = m_pGB->AddSourceFilter(path, NULL, &pBF);

				if(FAILED(hr)) break;

				if(tags)
				{
					Foreach(m_pGB, [&] (IBaseFilter* pBF) -> HRESULT
					{
						CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF;

						if(pAMMC != NULL)
						{
							CComBSTR bstr;
							if(SUCCEEDED(pAMMC->get_Title(&bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"TITL"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"AUTH"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pAMMC->get_Copyright(&bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"CPYR"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pAMMC->get_Rating(&bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"RTNG"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pAMMC->get_Description(&bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"DESC"] = bstr.m_str;
						}

						CComQIPtr<IDSMPropertyBag> pPB = pBF;

						if(pPB != NULL)
						{
							CComBSTR bstr;
							if(SUCCEEDED(pPB->GetProperty(L"TITL", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"TITL"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"AUTH", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"AUTH"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"CPYR", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"CPYR"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"RTNG", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"RTNG"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"DESC", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"DESC"] = bstr.m_str;					
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"CPRF", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"CPRF"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"HTNM", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"HTNM"] = bstr.m_str;
							bstr.Empty();
							if(SUCCEEDED(pPB->GetProperty(L"HPID", &bstr)) && wcslen(bstr.m_str) > 0) (*tags)[L"HPID"] = bstr.m_str;					
						}

						return S_CONTINUE;
					});
				}

				REFERENCE_TIME pos, dur = 0;

				CComQIPtr<IMediaSeeking> pMS = m_pGB;

				pMS->GetDuration(&dur);

				if(dur > 0 && pdur)
				{
					*pdur = dur;
				}

				if(bm)
				{
					CAMEvent evReceive;

					CImageGrabberFilter* pIGF = new CImageGrabberFilter(bm, evReceive);

					hr = m_pGB->AddFilter(pIGF, L"Image Grabber");

					if(FAILED(hr)) break;

					Foreach(pBF, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
					{
						if(SUCCEEDED(m_pGB->ConnectFilter(pin, pIGF)))
						{
							return S_OK;
						}

						return S_CONTINUE;
					});

					// TODO: render to null

					if(DirectShow::GetConnected(DirectShow::GetFirstPin(pIGF)) == NULL)
					{
						hr = E_FAIL; 

						Foreach(m_pGB, [&] (IBaseFilter* pBF) -> HRESULT
						{
							CComQIPtr<IDSMResourceBag> pRB = pBF;

							if(pRB != NULL)
							{
								DWORD count = pRB->ResGetCount();

								for(ULONG i = 0; i < count; i++)
								{
									CComBSTR mime;

									if(SUCCEEDED(pRB->ResGet(i, NULL, NULL, &mime, NULL, NULL, NULL)) && (mime == L"image/jpeg" || mime == L"image/png"))
									{
										BYTE* data = NULL;
										DWORD len = 0;

										if(SUCCEEDED(pRB->ResGet(i, NULL, NULL, &mime, &data, &len, NULL)) && data != NULL)
										{
											Util::ImageDecompressor id;
					
											if(id.Open(data, len))
											{
												bm->width = id.m_width;
												bm->height = id.m_height;
												bm->bits = new DWORD[bm->width * bm->height];
						
												Image::RGBToRGB(bm->width, bm->height, (BYTE*)bm->bits, bm->width * 4, 32, id.m_pixels, id.m_pitch, 32);
						
												for(int i = 0, j = bm->width * bm->height; i < j; i++) bm->bits[i] |= 0xff000000; // FIXME
											}

											CoTaskMemFree(data);

											if(bm->bits != NULL)
											{
												hr = S_OK;

												return S_OK;
											}
										}
									}
								}
							}

							return S_CONTINUE;
						});

						break;
					}

					CComQIPtr<IMediaControl> pMC = m_pGB;

					pMS->GetDuration(&dur);

					if(dur > 0)
					{
						if(pdur) *pdur = dur;

						pos = (REFERENCE_TIME)(at * dur);

						pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
					}

					pMC->Pause();

					if(!evReceive.Wait(timeout)) 
					{
						hr = E_FAIL; 
						
						break;
					}

					pMC->Stop();
				}
			}
			while(0);

			m_pGB->Reset();

			return SUCCEEDED(hr);
		}

		return true;
	}

	// ImageGrabber::Bitmap

	void ImageGrabber::Bitmap::Resize(int w, int h)
	{
		DWORD* src = bits;
		DWORD* dst = new DWORD[w * h];

		for(DWORD y = 0, yd = (height << 8) / h, j = 0; j < h; y += yd, j++)
		{
			DWORD yf = y & 0xff;
			DWORD yi0 = y >> 8;
			DWORD yi1 = std::min<DWORD>(yi0 + 1, height - 1);

			DWORD* s0 = src + yi0 * width;
			DWORD* s1 = src + yi1 * width;
			DWORD* d = dst + j * w;

			for(DWORD x = 0, xd = (width << 8) / w, i = 0; i < w; x += xd, i++)
			{
				DWORD xf = x & 0xff;
				DWORD xi0 = x >> 8;
				DWORD xi1 = std::min<DWORD>(xi0 + 1, width - 1);

				DWORD c0 = s0[xi0];
				DWORD c1 = s0[xi1];
				DWORD c2 = s1[xi0];
				DWORD c3 = s1[xi1];

				c0 = ((c0&0xff00ff) + ((((c1&0xff00ff) - (c0&0xff00ff)) * xf) >> 8)) & 0x00ff00ff
				  | ((((c0>>8)&0xff00ff) + (((((c1>>8)&0xff00ff) - ((c0>>8)&0xff00ff)) * xf) >> 8)) << 8) & 0xff00ff00;

				c2 = ((c2&0xff00ff) + ((((c3&0xff00ff) - (c2&0xff00ff)) * xf) >> 8)) & 0x00ff00ff
				  | ((((c2>>8)&0xff00ff) + (((((c3>>8)&0xff00ff) - ((c2>>8)&0xff00ff)) * xf) >> 8)) << 8) & 0xff00ff00;

				c0 = ((c0&0xff00ff) + ((((c2&0xff00ff) - (c0&0xff00ff)) * yf) >> 8)) & 0x00ff00ff
				  | ((((c0>>8)&0xff00ff) + (((((c2>>8)&0xff00ff) - ((c0>>8)&0xff00ff)) * yf) >> 8)) << 8) & 0xff00ff00;

				d[i] = c0;
			}
		}

		bits = dst;
		width = w;
		height = h;

		delete [] src;
	}
}