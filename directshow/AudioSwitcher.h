// Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

#pragma once

#include "StreamSwitcher.h"
#include "fft.h"

[uuid("CEDB2890-53AE-4231-91A3-B0AAFCD1DBDE")]
interface IAudioSwitcherFilter : public IUnknown
{
	STDMETHOD(GetInputSpeakerConfig)(DWORD* mask) = 0;
    STDMETHOD(GetSpeakerConfig)(bool* enabled, DWORD speakers[18][18]) = 0;
    STDMETHOD(SetSpeakerConfig)(bool enabled, DWORD speakers[18][18]) = 0;
    STDMETHOD_(int, GetNumberOfInputChannels)() = 0;
	STDMETHOD_(REFERENCE_TIME, GetAudioTimeShift)() = 0;
	STDMETHOD(SetAudioTimeShift)(REFERENCE_TIME shift) = 0;
	STDMETHOD(GetNormalizeBoost)(bool& normalize, bool& recover, float& boost) = 0;
	STDMETHOD(SetNormalizeBoost)(bool normalize, bool recover, float boost) = 0;
	STDMETHOD(GetRecentSamples)(float* buff, int& count) = 0;
	STDMETHOD(GetRecentFrequencies)(float* buff, int freqs, float& volume) = 0;
	STDMETHOD(SaveConfig)() = 0;
};

class CAudioTimeToFreqTransformer
{
    int m_input;
    int m_output;
    int* m_bitrevtable;
	struct CosSin {float c, s;}* m_cossintable;
    float* m_envelope;
    float* m_equalize;
    float* m_temp1;
    float* m_temp2;
	std::vector<float> m_wavedata;
	float* m_spectraldata;

public:
	CAudioTimeToFreqTransformer(int input, int output, bool equalize = true, float envelope = 1.0f);
	virtual ~CAudioTimeToFreqTransformer();
	void Add(float sample);
	void GetOutput(float* buff);
};

[uuid("18C16B08-6497-420e-AD14-22D21C2CEAB7")]
class CAudioSwitcherFilter : public CStreamSwitcherFilter, public IAudioSwitcherFilter
{
	struct ChannelMap {DWORD speaker, channel;};

	std::vector<ChannelMap> m_channels[18];
	bool m_fCustomChannelMapping;
	DWORD m_speakers[18][18];
	REFERENCE_TIME m_shift;
	double m_sample_max;
	bool m_normalize;
	bool m_recover;
	float m_boost;

	std::vector<double> m_fft;
	int m_fft_pos;
	CCritSec m_fft_lock;
	VDRealFFT m_fft_tr;

	struct {REFERENCE_TIME start, stop;} m_next;

public:
	CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	CMediaType CreateNewOutputMediaType(CMediaType mt, long& cbBuffer);
	void OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut);

	HRESULT DeliverEndFlush();
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	// IAudioSwitcherFilter

	STDMETHODIMP GetInputSpeakerConfig(DWORD* mask);
    STDMETHODIMP GetSpeakerConfig(bool* enabled, DWORD speakers[18][18]);
    STDMETHODIMP SetSpeakerConfig(bool enabled, DWORD speakers[18][18]);
    STDMETHODIMP_(int) GetNumberOfInputChannels();
	STDMETHODIMP_(REFERENCE_TIME) GetAudioTimeShift();
	STDMETHODIMP SetAudioTimeShift(REFERENCE_TIME shift);
	STDMETHODIMP GetNormalizeBoost(bool& normalize, bool& recover, float& boost);
	STDMETHODIMP SetNormalizeBoost(bool normalize, bool recover, float boost);
	STDMETHODIMP GetRecentSamples(float* buff, int& count);
	STDMETHODIMP GetRecentFrequencies(float* buff, int freqs, float& volume);
	STDMETHODIMP SaveConfig();

	// IAMStreamSelect

	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
};
