/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "AudioSwitcher.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

#define FFT_BITS 13
#define FFT_SIZE (1 << (FFT_BITS + 1))

// CAudioSwitcherFilter

CAudioSwitcherFilter::CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CStreamSwitcherFilter(lpunk, phr, __uuidof(this))
	, m_fCustomChannelMapping(false)
	, m_shift(0)
	, m_normalize(false)
	, m_recover(true)
	, m_boost(1)
	, m_sample_max(0.1f)
	, m_fft_tr(FFT_BITS)
{
	memset(m_speakers, 0, sizeof(m_speakers));

	m_fft_pos = 0;

	m_next.start = 0;
	m_next.stop = 1;

	if(phr)
	{
		if(FAILED(*phr)) return;
		
		*phr = S_OK;
	}

	CRegKey key;

	if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Homesys\\Filters\\Audio Switcher", KEY_READ))
	{
		DWORD dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Normalize", dw)) m_normalize = !!dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Recover", dw)) m_recover = !!dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Boost", dw)) m_boost = *(float*)&dw;
		// TODO
	}
}

STDMETHODIMP CAudioSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAudioSwitcherFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAudioSwitcherFilter::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->formattype == FORMAT_WaveFormatEx)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->pbFormat;

		if(wfe->nChannels > 2 && wfe->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
		{
			return VFW_E_INVALIDMEDIATYPE; // stupid iviaudio tries to fool us
		}
	}

	if(pmt->majortype == MEDIATYPE_Audio && pmt->formattype == FORMAT_WaveFormatEx)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->pbFormat;

		if((wfe->wBitsPerSample == 8 
		 || wfe->wBitsPerSample == 16 
		 || wfe->wBitsPerSample == 24 
		 || wfe->wBitsPerSample == 32)
		&& (wfe->wFormatTag == WAVE_FORMAT_PCM 
		 || wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT 
		 || wfe->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF 
		 || wfe->wFormatTag == WAVE_FORMAT_WMASPDIF 
		 || wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE))
		{
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

template<class T, class U, int Umin, int Umax> 
void MixSamples(DWORD mask, int ch, int bps, BYTE* src, BYTE* dst)
{
	U sum = 0;

	for(int i = 0, j = min(18, ch); i < j; i++)
	{
		if(mask & (1 << i))
		{
			sum += *(T*)&src[bps * i];
		}
	}

	if(sum < Umin) sum = Umin;
	if(sum > Umax) sum = Umax;
	
	*(T*)dst = (T)sum;
}

template<> 
void MixSamples<int, INT64, (-1 << 24), (+1 << 24) - 1>(DWORD mask, int ch, int bps, BYTE* src, BYTE* dst)
{
	INT64 sum = 0;

	for(int i = 0, j = min(18, ch); i < j; i++)
	{
		if(mask & (1 << i))
		{
			int tmp;
			memcpy((BYTE*)&tmp + 1, &src[bps * i], 3);
			sum += tmp >> 8;
		}
	}

	sum = min(max(sum, (-1 << 24)), (+1 << 24) - 1);

	memcpy(dst, (BYTE*)&sum, 3);
}

template<class T> 
T ScaleSamples(double s, T smin, T smax)
{
	T t = (T)(s * smax);
	if(t < smin) t = smin;
	else if(t > smax) t = smax;
	return t;
}

HRESULT CAudioSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();

	if(pInPin == NULL || pOutPin == NULL)
	{
		return __super::Transform(pIn, pOut);
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;
	WAVEFORMATEX* wfeout = (WAVEFORMATEX*)pOutPin->CurrentMediaType().pbFormat;
	WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
	// WAVEFORMATEXTENSIBLE* wfexout = (WAVEFORMATEXTENSIBLE*)wfeout;

	int bps = wfe->wBitsPerSample >> 3;

	int len = pIn->GetActualDataLength() / (bps * wfe->nChannels);
	int lenout = len * wfeout->nSamplesPerSec / wfe->nSamplesPerSec;

	REFERENCE_TIME rtStart, rtStop;

	if(SUCCEEDED(pIn->GetTime(&rtStart, &rtStop)))
	{
		rtStart += m_shift;
		rtStop += m_shift;

		pOut->SetTime(&rtStart, &rtStop);

		m_next.start = rtStart;
		m_next.stop = rtStop;
	}
	else
	{
		pOut->SetTime(&m_next.start, &m_next.stop);
	}

	REFERENCE_TIME dur = 10000000i64 * len / wfe->nSamplesPerSec;

	m_next.start += dur;
	m_next.stop += dur;

	if(pIn->IsDiscontinuity() == S_OK)
	{
		m_sample_max = 0.1f;
	}

	WORD tag = wfe->wFormatTag;

	bool fPCM = tag == WAVE_FORMAT_PCM || tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;
	bool fFloat = tag == WAVE_FORMAT_IEEE_FLOAT || tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

	if(!fPCM && !fFloat) 
	{
		return __super::Transform(pIn, pOut);
	}

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	HRESULT hr;

	if(FAILED(hr = pIn->GetPointer(&pDataIn))) 
	{
		return hr;
	}

	if(FAILED(hr = pOut->GetPointer(&pDataOut))) 
	{
		return hr;
	}

	if(!pDataIn || !pDataOut || len <= 0 || lenout <= 0)
	{
		return S_FALSE;
	}

	memset(pDataOut, 0, pOut->GetSize());

	if(m_fCustomChannelMapping)
	{
		if(m_channels[wfe->nChannels - 1].size() > 0)
		{
			for(int i = 0; i < wfeout->nChannels; i++)
			{
				DWORD mask = m_channels[wfe->nChannels - 1][i].channel;

				BYTE* src = pDataIn;
				BYTE* dst = &pDataOut[bps * i];

				int srcstep = bps * wfe->nChannels;
				int dststep = bps * wfeout->nChannels;

				if(fPCM && wfe->wBitsPerSample == 8)
				{
					for(int k = 0; k < len; k++, src += srcstep, dst += dststep)
					{
						MixSamples<unsigned char, INT64, 0, UCHAR_MAX>(mask, wfe->nChannels, bps, src, dst);
					}
				}
				else if(fPCM && wfe->wBitsPerSample == 16)
				{
					for(int k = 0; k < len; k++, src += srcstep, dst += dststep)
					{
						MixSamples<short, INT64, SHRT_MIN, SHRT_MAX>(mask, wfe->nChannels, bps, src, dst);
					}
				}
				else if(fPCM && wfe->wBitsPerSample == 24)
				{
					for(int k = 0; k < len; k++, src += srcstep, dst += dststep)
					{
						MixSamples<int, INT64, (-1 << 24), (+1 << 24) - 1>(mask, wfe->nChannels, bps, src, dst);
					}
				}
				else if(fPCM && wfe->wBitsPerSample == 32)
				{
					for(int k = 0; k < len; k++, src += srcstep, dst += dststep)
					{
						MixSamples<int, __int64, INT_MIN, INT_MAX>(mask, wfe->nChannels, bps, src, dst);
					}
				}
				else if(fFloat && wfe->wBitsPerSample == 32)
				{
					for(int k = 0; k < len; k++, src += srcstep, dst += dststep)
					{
						MixSamples<float, double, -1, 1>(mask, wfe->nChannels, bps, src, dst);
					}
				}
				else if(fFloat && wfe->wBitsPerSample == 64)
				{
					for(int k = 0; k < len; k++, src += srcstep, dst += dststep)
					{
						MixSamples<double, double, -1, 1>(mask, wfe->nChannels, bps, src, dst);
					}
				}
			}
		}
		else
		{
			BYTE* pDataOut = NULL;

			if(FAILED(hr = pOut->GetPointer(&pDataOut)) || pDataOut == NULL)
			{
				return hr;
			}

			memset(pDataOut, 0, pOut->GetSize());
		}
	}
	else
	{
		if(S_OK != (hr = __super::Transform(pIn, pOut)))
		{
			return hr;
		}
	}

	//if(m_normalize || m_boost > 1)
	{
		int samples = lenout * wfeout->nChannels;

		double boost = log10(m_boost) + 1;

		double* buff = new double[samples];

		{
			WORD bits = wfe->wBitsPerSample;

			if(fPCM && bits == 8) 
			{
				for(int i = 0; i < samples; i++) 
				{
					buff[i] = (double)((BYTE*)pDataOut)[i] * (1.0 / UCHAR_MAX);
				}
			}
			else if(fPCM && bits == 16)
			{
				for(int i = 0; i < samples; i++) 
				{
					buff[i] = (double)((short*)pDataOut)[i] * (1.0 / SHRT_MAX);
				}
			}
			else if(fPCM && bits == 24) 
			{
				for(int i = 0; i < samples; i++)
				{
					int tmp; 
					
					memcpy(((BYTE*)&tmp)+1, &pDataOut[i*3], 3); 

					buff[i] = (float)(tmp >> 8) * (1.0 / ((1 << 23) - 1));
				}
			}
			else if(fPCM && bits == 32)
			{
				for(int i = 0; i < samples; i++) 
				{
					buff[i] = (double)((int*)pDataOut)[i] * (1.0 / INT_MAX);
				}
			}
			else if(fFloat && bits == 32) 
			{
				for(int i = 0; i < samples; i++) 
				{
					buff[i] = (double)((float*)pDataOut)[i];
				}
			}
			else if(fFloat && bits == 64) 
			{
				for(int i = 0; i < samples; i++) 
				{
					buff[i] = ((double*)pDataOut)[i];
				}
			}
		}

		double sample_mul = 1.0;

		if(m_normalize)
		{
			for(int i = 0; i < samples; i++)
			{
				double s = buff[i];

				if(s < 0) s = -s;
				if(s > 1) s = 1;

				if(m_sample_max < s)
				{
					m_sample_max = s;
				}
			}

			sample_mul = 1.0f / m_sample_max;

			if(m_recover)
			{
				m_sample_max -= 1.0 * dur / 200000000; // -5%/sec
			}

			if(m_sample_max < 0.1)
			{
				m_sample_max = 0.1;
			}
		}

		if(m_boost > 1)
		{
			sample_mul *= boost;
		}

		if(sample_mul != 1.0)
		{
			for(int i = 0; i < samples; i++)
			{
				double s = buff[i] * sample_mul;

				if(s < -1) s = -1;
				else if(s > +1) s = +1;

				buff[i] = s;
			}
		}

		{
			CAutoLock cAutoLock(&m_fft_lock);

			if(m_fft.empty())
			{
				m_fft.resize(FFT_SIZE);
			}

			int fft_pos = m_fft_pos;

			double ich = 1.0 / wfeout->nChannels;

			for(int i = 0, ch = wfeout->nChannels; i < samples; i += ch, fft_pos++)
			{
				double s = 0;

				for(int j = 0; j < ch; j++)
				{
					s += buff[i + j];
				}

				m_fft[fft_pos & (FFT_SIZE - 1)] = s * ich;
			}

			m_fft_pos = fft_pos & (FFT_SIZE - 1);
		}

		{
			WORD bits = wfe->wBitsPerSample;

			if(fPCM && bits == 8)
			{
				for(int i = 0; i < samples; i++)
				{
					((BYTE*)pDataOut)[i] = ScaleSamples<BYTE>(buff[i], 0, UCHAR_MAX); // (buff[i] + 1) / 2 ???
				}
			}
			else if(fPCM && bits == 16) 
			{
				for(int i = 0; i < samples; i++) 
				{
					((short*)pDataOut)[i] = ScaleSamples<short>(buff[i], SHRT_MIN, SHRT_MAX);
				}
			}
			else if(fPCM && bits == 24)
			{
				for(int i = 0; i < samples; i++)
				{
					int tmp = ScaleSamples<int>(buff[i], -1 << 23, (1 << 23) - 1); 

					memcpy(&pDataOut[i*3], &tmp, 3);
				}
			}
			else if(fPCM && bits == 32)
			{
				for(int i = 0; i < samples; i++)
				{
					((int*)pDataOut)[i] = ScaleSamples<int>(buff[i], INT_MIN, INT_MAX);
				}
			}
			else if(fFloat && bits == 32)
			{
				for(int i = 0; i < samples; i++)
				{
					((float*)pDataOut)[i] = (float)buff[i];
				}
			}
			else if(fFloat && bits == 64)
			{
				for(int i = 0; i < samples; i++)
				{
					((double*)pDataOut)[i] = buff[i];
				}
			}
		}

		delete [] buff;
	}

	pOut->SetActualDataLength(lenout * bps * wfeout->nChannels);

	return S_OK;
}

CMediaType CAudioSwitcherFilter::CreateNewOutputMediaType(CMediaType mt, long& cbBuffer)
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;

	if(pInPin == NULL || pOutPin == NULL || wfe->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF || wfe->wFormatTag == WAVE_FORMAT_WMASPDIF)
	{
		return __super::CreateNewOutputMediaType(mt, cbBuffer);
	}

	wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;

	if(m_fCustomChannelMapping)
	{
		std::vector<ChannelMap>& channel = m_channels[wfe->nChannels - 1];

		channel.clear();

		DWORD mask = (DWORD)((1i64 << wfe->nChannels) - 1);

		for(int i = 0; i < 18; i++)
		{
			if(m_speakers[wfe->nChannels - 1][i] & mask)
			{
				ChannelMap cm = {1 << i, m_speakers[wfe->nChannels - 1][i]};

				channel.push_back(cm);
			}
		}

		if(channel.size() > 0)
		{
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
			
			wfex->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			wfex->Samples.wValidBitsPerSample = wfe->wBitsPerSample;
			wfex->SubFormat = 
				wfe->wFormatTag == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM :
				wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT :
				wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? ((WAVEFORMATEXTENSIBLE*)wfe)->SubFormat :
				KSDATAFORMAT_SUBTYPE_PCM; // can't happen

			wfex->dwChannelMask = 0;
			
			std::for_each(channel.begin(), channel.end(), [&] (const ChannelMap& cm)
			{
				wfex->dwChannelMask |= cm.speaker;
			});

			wfex->Format.nChannels = (WORD)channel.size();
			wfex->Format.nBlockAlign = wfex->Format.nChannels*wfex->Format.wBitsPerSample >> 3;
			wfex->Format.nAvgBytesPerSec = wfex->Format.nBlockAlign*wfex->Format.nSamplesPerSec;
		}
	}

	WAVEFORMATEX* wfeout = (WAVEFORMATEX*)mt.pbFormat;

	int bps = wfe->wBitsPerSample >> 3;
	int len = cbBuffer / (bps * wfe->nChannels);
	int lenout = len * wfeout->nSamplesPerSec / wfe->nSamplesPerSec;

	cbBuffer = lenout * bps * wfeout->nChannels;

//	mt.lSampleSize = (ULONG)max(mt.lSampleSize, wfe->nAvgBytesPerSec * rtLen / 10000000i64);
//	mt.lSampleSize = (mt.lSampleSize + (wfe->nBlockAlign - 1)) & ~(wfe->nBlockAlign - 1);

	return mt;
}

void CAudioSwitcherFilter::OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut)
{
	m_sample_max = 0.1f;
}

HRESULT CAudioSwitcherFilter::DeliverEndFlush()
{
	m_sample_max = 0.1f;

	return __super::DeliverEndFlush();
}

HRESULT CAudioSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_sample_max = 0.1f;

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

// IAudioSwitcherFilter

STDMETHODIMP CAudioSwitcherFilter::GetInputSpeakerConfig(DWORD* mask)
{
	if(!mask) return E_POINTER;

	*mask = 0;

	CStreamSwitcherInputPin* pInPin = GetInputPin();

	if(pInPin == NULL || !pInPin->IsConnected())
	{
		return E_UNEXPECTED;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;

	*mask = wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? ((WAVEFORMATEXTENSIBLE*)wfe)->dwChannelMask : 0; /*wfe->nChannels == 1 ? 4 : wfe->nChannels == 2 ? 3 : 0*/

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetSpeakerConfig(bool* enabled, DWORD speakers[18][18])
{
	if(enabled) 
	{
		*enabled = m_fCustomChannelMapping;
	}

	memcpy(speakers, m_speakers, sizeof(m_speakers));

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetSpeakerConfig(bool enabled, DWORD speakers[18][18])
{
	if(m_State == State_Stopped || m_fCustomChannelMapping != enabled || memcmp(m_speakers, speakers, sizeof(m_speakers)) != 0)
	{
		PauseGraph;
		
		CStreamSwitcherInputPin* pInput = GetInputPin();

		SelectInput(NULL);

		m_fCustomChannelMapping = enabled;

		memcpy(m_speakers, speakers, sizeof(m_speakers));

		SelectInput(pInput);

		ResumeGraph;
	}

	return S_OK;
}

STDMETHODIMP_(int) CAudioSwitcherFilter::GetNumberOfInputChannels()
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();

	return pInPin ? ((WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat)->nChannels : 0;
}

STDMETHODIMP_(REFERENCE_TIME) CAudioSwitcherFilter::GetAudioTimeShift()
{
	return m_shift;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioTimeShift(REFERENCE_TIME shift)
{
	m_shift = shift;

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetNormalizeBoost(bool& normalize, bool& recover, float& boost)
{
	normalize = m_normalize;
	recover = m_recover;
	boost = m_boost;

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetNormalizeBoost(bool normalize, bool recover, float boost)
{
	if(m_normalize != normalize)
	{
		m_sample_max = 0.1f;
	}

	m_normalize = normalize;
	m_recover = recover;
	m_boost = boost;

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SaveConfig()
{
	CRegKey key;

	if(ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, L"Software\\Homesys\\Filters\\Audio Switcher"))
	{
		key.SetDWORDValue(L"Normalize", m_normalize);
		key.SetDWORDValue(L"Recover", m_recover);
		key.SetDWORDValue(L"Boost", *(DWORD*)&m_boost);
		// TODO
	}

	return S_OK;
}

#include "../util/Vector.h"

STDMETHODIMP CAudioSwitcherFilter::GetRecentSamples(float* buff, int& count)
{
	CAutoLock cAutoLock(&m_fft_lock);

	if(m_fft.empty())
	{
		return E_FAIL;
	}

	count = m_fft.size();

	if(buff != NULL)
	{
		for(int i = 0, j = m_fft_pos, n = count; i < n; i++, j++)
		{
			buff[i] = (float)m_fft[j & (FFT_SIZE - 1)];
		}
	}

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetRecentFrequencies(float* buff, int freqs, float& volume)
{
	CAutoLock cAutoLock(&m_fft_lock);

	if(m_fft.empty())
	{
		return E_FAIL;
	}

	int count = 1 << FFT_BITS;

	float* tmp = (float*)_aligned_malloc(sizeof(float) * count, 16);

	float f = 0;

	for(int i = 0, j = m_fft_pos, n = count; i < n; i++, j++)
	{
		float s = (float)m_fft[j & (FFT_SIZE - 1)];

		f += abs(s);

		tmp[i] = s;
	}

	volume = f / count;

	m_fft_tr.ComputeRealFFT(tmp);

	Vector4* RESTRICT src = (Vector4*)tmp;
	Vector4* RESTRICT dst = (Vector4*)tmp;
		
	for(int i = 0, n = count / 8; i < n; i++, src += 2, dst++)
	{
		Vector4 v0 = src[0];
		Vector4 v1 = src[1];

		Vector4 v2 = v0.xzxz(v1);
		Vector4 v3 = v0.ywyw(v1);

		*dst = (v2 * v2 + v3 * v3).sqrt();
	}

	count /= 2;

	int cmin = count / 256;
	int cmax = count - cmin;// / 2;
					
	for(int i = 0, j = 0, step = (cmax - cmin) / freqs; j < freqs; i += step, j++)
	{
		float d0 = (float)(i) / count;
		float d1 = (float)(std::min<int>(i + step, count)) / count;
							
		d0 = powf(d0, 2.4f);
		d1 = powf(d1, 2.4f);

		int i0 = cmin + (int)(d0 * (cmax - cmin) + 0.5f);
		int i1 = cmin + (int)(d1 * (cmax - cmin) + 0.5f);

		// printf("%d - %d (%d)\n", i0, i1, i1 - i0);

		float s = 0;

		while(i0 < i1)
		{
			float s2 = tmp[i0];

			s2 = s2 * (1.0f + powf((float)i0 * 2 / count, 3));

			i0++;

			s = std::max<float>(s2, s);
		}

		buff[j] = s;
	}

	_aligned_free(tmp);

	return S_OK;
}

// IAMStreamSelect

STDMETHODIMP CAudioSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	HRESULT hr;
	
	hr = __super::Enable(lIndex, dwFlags);

	if(S_OK == hr)
	{
		m_sample_max = 0.1f;
	}

	return hr;
}
