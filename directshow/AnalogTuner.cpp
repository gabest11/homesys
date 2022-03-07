#pragma once

#include "stdafx.h"
#include "AnalogTuner.h"
#include "DirectShow.h"
#include "ColorConverter.h"
#include "MPEG2Encoder.h"
#include "TeletextFilter.h"

AnalogTuner::AnalogTuner(bool compressed) 
	: m_compressed(compressed)
{
	Close();

	m_tuningspace = GenUniqueTuningSpace();
}

AnalogTuner::~AnalogTuner()
{
	Close();
}

void AnalogTuner::Close()
{
	m_video = NULL;
	m_audio = NULL;
	m_xbar = NULL;
	m_avdec = NULL;
	m_tvtuner = NULL;
	m_tvaudio = NULL;
	m_pin.video = NULL;
	m_pin.audio = NULL;
	m_pin.vbi = NULL;

	m_tvformats = 0;
	m_standard = 0;
	m_connector = -1;
	m_connectorType = -1;
	m_channel = -1;

	__super::Close();
}

bool AnalogTuner::CreateOutput(IGraphBuilder2* pGB, IBaseFilter* capture)
{
	m_pin.video = NULL;
	m_pin.audio = NULL;
	m_pin.vbi = NULL;

	CComPtr<IPin> video, audio;

	if(CreateMPEG2PSOutput(pGB, capture, &video, &audio))
	{
		m_pin.video = video;
		m_pin.audio = audio;

		CreateVBIOutput(pGB, capture, &m_pin.vbi);

		return true;
	}

	video = NULL;
	audio = NULL;

	pGB->NukeDownstream(capture);

	if(CreateMPEG2ESOutput(pGB, capture, &video, &audio))
	{
		m_pin.video = video;
		m_pin.audio = audio;

		CreateVBIOutput(pGB, capture, &m_pin.vbi);

		return true;
	}

	video = NULL;
	audio = NULL;

	pGB->NukeDownstream(capture);

	if(CreateUncompressedOutput(pGB, capture, &video, &audio))
	{
		if(!m_compressed || AppendMPEG2Encoder(pGB, video, audio))
		{
			m_pin.video = video;
			m_pin.audio = audio;

			CreateVBIOutput(pGB, capture, &m_pin.vbi);

			return true;
		}
	}

	video = NULL;
	audio = NULL;

	pGB->NukeDownstream(capture);

	return false;
}

bool AnalogTuner::CreateMPEG2PSOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** video, IPin** audio)
{
	HRESULT hr;

	if(!video || !audio) 
	{
		return false;
	}

	pGB->NukeDownstream(capture);

	CComPtr<IPin> pes;

	Foreach(KSCATEGORY_ENCODER, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
	{
		HRESULT hr;

		CComPtr<IBaseFilter> enc;

		hr = pGB->AddSourceFilterForMoniker(m, 0, name, &enc);

		if(SUCCEEDED(hr)) 
		{
			hr = pGB->ConnectFilterDirect(capture, enc, NULL);

			if(SUCCEEDED(hr))
			{
				Foreach(enc, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
				{
					Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
					{
						if(pmt->subtype == MEDIASUBTYPE_MPEG2_PROGRAM)
						{
							pes = pin;
						}

						return pes != NULL ? S_OK : S_CONTINUE;
					});

					return pes != NULL ? S_OK : S_CONTINUE;
				});
			}

			if(pes == NULL) 
			{
				hr = pGB->RemoveFilter(enc);
			}
		}

		return pes != NULL ? S_OK : S_CONTINUE;
	});

	if(pes == NULL) 
	{
		return false;
	}
/*
	if(CComQIPtr<IEncoderAPI> pEnc = DirectShow::GetFilter(pes))
	{
		{
			CComVariant vmin, vmax, step;

			hr = pEnc->GetParameterRange(&ENCAPIPARAM_PEAK_BITRATE, &vmin, &vmax, &step);
			hr = pEnc->GetValue(&ENCAPIPARAM_PEAK_BITRATE, &vmin);
			hr = pEnc->SetValue(&ENCAPIPARAM_PEAK_BITRATE, &vmax);
		}

		{
			CComVariant vmin, vmax, step;

			hr = pEnc->GetParameterRange(&ENCAPIPARAM_BITRATE, &vmin, &vmax, &step);
			hr = pEnc->GetValue(&ENCAPIPARAM_BITRATE, &vmin);
			hr = pEnc->SetValue(&ENCAPIPARAM_BITRATE, &vmax);
		}

		{
			CComVariant vmin, vmax, step;

			vmax = ConstantBitRate;
			hr = pEnc->GetValue(&ENCAPIPARAM_BITRATE_MODE, &vmin);
			hr = pEnc->SetValue(&ENCAPIPARAM_BITRATE_MODE, &vmax);
		}
	}
*/
	// TODO: append PSDemux, run graph, detect streams, stop graph, find output pins (video, audio)

	CComPtr<IBaseFilter> pBF;

	hr = pBF.CoCreateInstance(CLSID_MPEG2Demultiplexer);
	
	if(FAILED(hr))
	{
		return false;
	}

	hr = pGB->AddFilter(pBF, L"MPEG-2 Demultiplexer");

	if(FAILED(hr))
	{
		return false;
	}

	hr = pGB->ConnectFilterDirect(pes, pBF, NULL);

	if(FAILED(hr))
	{
		return false;
	}

	CComQIPtr<IMpeg2Demultiplexer> demux = pBF;

	if(!demux) 
	{
		return false;
	}

	CMediaType mt;

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
	mt.formattype = FORMAT_MPEG2Video;

	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = 0;

	MPEG2VIDEOINFO* vf = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(sizeof(MPEG2VIDEOINFO));

	memset(vf, 0, sizeof(*vf));

	vf->hdr.bmiHeader.biSize = sizeof(vf->hdr.bmiHeader);
	vf->hdr.bmiHeader.biWidth = 720; 
	vf->hdr.bmiHeader.biHeight = 576;
	vf->hdr.rcSource.right = vf->hdr.rcTarget.right = vf->hdr.bmiHeader.biWidth;
	vf->hdr.rcSource.bottom = vf->hdr.rcTarget.bottom = vf->hdr.bmiHeader.biHeight;
	vf->hdr.dwBitRate = 4000000;
	vf->hdr.AvgTimePerFrame = 400000; // 25fps
	vf->hdr.dwPictAspectRatioX = 4;
	vf->hdr.dwPictAspectRatioY = 3;
	vf->dwStartTimeCode = 0x0006f498; // huh?
	vf->cbSequenceHeader = 0;
	vf->dwProfile = 2;
	vf->dwLevel = 2;
	vf->dwFlags = 0;

	hr = demux->CreateOutputPin(&mt, L"Video", video);

	if(FAILED(hr)) 
	{
		return false;
	}

	CComQIPtr<IMPEG2StreamIdMap> pIVideoPIDMap = *video;

	if(pIVideoPIDMap == NULL) 
	{
		return false;
	}

	hr = pIVideoPIDMap->MapStreamId(0xe0, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0);

	if(FAILED(hr)) 
	{
		return false;
	}

	mt.InitMediaType();

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_MPEG2_AUDIO;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = 0;
	mt.formattype = FORMAT_WaveFormatEx;

	MPEG1WAVEFORMAT* af = (MPEG1WAVEFORMAT*)mt.AllocFormatBuffer(sizeof(MPEG1WAVEFORMAT));
	
	memset(af, 0, sizeof(*af));
	
	af->wfx.wFormatTag = WAVE_FORMAT_MPEG;
	af->wfx.nChannels = 2;
	af->wfx.nSamplesPerSec = 48000;
	af->wfx.nAvgBytesPerSec = 32000;
	af->wfx.nBlockAlign = 768;
	af->wfx.cbSize = sizeof(*af) - sizeof(af->wfx);
	af->fwHeadLayer = ACM_MPEG_LAYER2;
	af->fwHeadMode = ACM_MPEG_STEREO;
	af->wHeadEmphasis = 1;

	hr = demux->CreateOutputPin(&mt, L"Audio", audio);

	if(FAILED(hr))
	{
		return false;
	}

	CComQIPtr<IMPEG2StreamIdMap> pIAudioPIDMap = *audio;

	if(pIAudioPIDMap == NULL) 
	{
		return false;
	}
	
	hr = pIAudioPIDMap->MapStreamId(0xc0, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0);

	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

bool AnalogTuner::CreateMPEG2ESOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** video, IPin** audio)
{
	HRESULT hr;

	if(!video || !audio)
	{
		return false;
	}

	pGB->NukeDownstream(capture);

	Foreach(capture, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		Foreach(KSCATEGORY_ENCODER, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
		{
			CComPtr<IBaseFilter> enc;

			hr = pGB->AddSourceFilterForMoniker(m, 0, name, &enc);

			if(SUCCEEDED(hr)) 
			{
				bool found = false;

				hr = pGB->ConnectFilterDirect(pin, enc, NULL);

				if(SUCCEEDED(hr))
				{
					Foreach(enc, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
					{
						Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
						{
							if(pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO && *video == NULL)
							{
								(*video = pin)->AddRef();

								found = true;
							}
							else if(pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO && *audio == NULL)
							{
								(*audio = pin)->AddRef();

								found = true;
							}

							return found ? S_OK : S_CONTINUE;
						});

						return found ? S_OK : S_CONTINUE;
					});
				}

				if(!found)
				{
					hr = pGB->RemoveFilter(enc);
				}
			}

			return S_CONTINUE;
		});

		return S_CONTINUE;
	});

	return *video != NULL && *audio != NULL;
}

bool AnalogTuner::CreateUncompressedOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** video, IPin** audio)
{
	Foreach(capture, PINDIR_OUTPUT, [&video, &audio, this] (IPin* pin) -> HRESULT
	{
		HRESULT hr;

		CComQIPtr<IAMStreamConfig> config = pin;

		if(config == NULL)
		{
			return S_CONTINUE;
		}

		int count = 0, size = 0;

		hr = config->GetNumberOfCapabilities(&count, &size);

		if(FAILED(hr) || sizeof(AnalogTunerCaps) < size) 
		{
			return S_CONTINUE;
		}

		for(int i = 0; i < count; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;

			AnalogTunerCaps caps;

			hr = config->GetStreamCaps(i, &pmt, (BYTE*)&caps);

			if(FAILED(hr)) 
			{
				continue;
			}

			if(*video == NULL && pmt->formattype == FORMAT_VideoInfo && (caps.video.VideoStandard & AnalogVideoMask_MCE_PAL))
			{
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;

				vih->AvgTimePerFrame = max(caps.video.MinFrameInterval, 10000000 / 25);
				vih->bmiHeader.biWidth = min(max(640, caps.video.MinOutputSize.cx), caps.video.MaxOutputSize.cx);
				vih->bmiHeader.biHeight = min(max(480, caps.video.MinOutputSize.cy), caps.video.MaxOutputSize.cy);
				vih->bmiHeader.biBitCount = 12;
				vih->bmiHeader.biCompression = '024I';
				vih->bmiHeader.biPlanes = 1;
				vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * vih->bmiHeader.biBitCount >> 3;
				pmt->subtype.Data1 = vih->bmiHeader.biCompression;

				hr = config->SetFormat(pmt);

				if(FAILED(hr))
				{
					vih->bmiHeader.biCompression = pmt->subtype.Data1 = 'VUYI';

					hr = config->SetFormat(pmt);
				}

				if(FAILED(hr))
				{
					vih->bmiHeader.biCompression = pmt->subtype.Data1 = '21VY';

					hr = config->SetFormat(pmt);
				}

				if(FAILED(hr))
				{
					vih->bmiHeader.biBitCount = 16;
					vih->bmiHeader.biCompression = pmt->subtype.Data1 = '2YUY';
					vih->bmiHeader.biPlanes = 1;
					vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * vih->bmiHeader.biBitCount >> 3;

					hr = config->SetFormat(pmt);
				}

				if(FAILED(hr))
				{
					vih->bmiHeader.biBitCount = 16;
					vih->bmiHeader.biCompression = pmt->subtype.Data1 = 'UYVY';
					vih->bmiHeader.biPlanes = 1;
					vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * vih->bmiHeader.biBitCount >> 3;

					hr = config->SetFormat(pmt);
				}

				if(FAILED(hr))
				{
					vih->bmiHeader.biBitCount = 16;
					vih->bmiHeader.biCompression = pmt->subtype.Data1 = 'YVYU';
					vih->bmiHeader.biPlanes = 1;
					vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * vih->bmiHeader.biBitCount >> 3;

					hr = config->SetFormat(pmt);
				}

				if(SUCCEEDED(hr))
				{
					(*video = pin)->AddRef();
				}
			}
			else if(*audio == NULL && pmt->formattype == FORMAT_WaveFormatEx)
			{
				WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->pbFormat;

				wfe->wFormatTag = WAVE_FORMAT_PCM;
				wfe->nChannels = (WORD)min(caps.audio.MaximumChannels, 2);
				wfe->wBitsPerSample = 16;
				wfe->nSamplesPerSec = max(caps.audio.MinimumSampleFrequency, min(caps.audio.MaximumSampleFrequency, 44100));
				wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample >> 3;
				wfe->nAvgBytesPerSec = wfe->nBlockAlign * wfe->nSamplesPerSec;

				hr = config->SetFormat(pmt);

				if(SUCCEEDED(hr))
				{
					(*audio = pin)->AddRef();
				}
			}

			DeleteMediaType(pmt);
		}

		return S_CONTINUE;
	});

	return *video != NULL && *audio != NULL;
}

bool AnalogTuner::CreateVBIOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** vbi)
{
	return false;

	Foreach(capture, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
		{
			if(pmt->majortype != MEDIATYPE_VBI)
			{
				return S_CONTINUE;
			}

			/*
			HRESULT hr;

			CComPtr<IBaseFilter> pTF = new CTeletextFilter(NULL, &hr);

			hr = pGB->AddFilter(pTF, L"Teletext Filter");

			hr = pGB->ConnectFilterDirect(pin, pTF, NULL);

			if(SUCCEEDED(hr))
			{
				(*vbi = DirectShow::GetFirstPin(pTF, PINDIR_OUTPUT))->AddRef();
			}
			else
			{
				hr = pGB->RemoveFilter(pTF);
			}
			*/
			const GUID* guids[] = {&AM_KSCATEGORY_VBICODEC_MI, &AM_KSCATEGORY_VBICODEC};

			for(int i = 0; i < sizeof(guids) / sizeof(guids[0]); i++)
			{
				HRESULT hr = Foreach(*guids[i], [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
				{
					HRESULT hr;

					CComPtr<IBaseFilter> pBF;

					hr = pGB->AddSourceFilterForMoniker(m, 0, name, &pBF);

					if(FAILED(hr)) 
					{
						return S_CONTINUE;
					}

					hr = pGB->ConnectFilterDirect(pin, pBF, NULL);

					if(SUCCEEDED(hr))
					{
						Foreach(pBF, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
						{
							Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
							{
								if(pmt->majortype == MEDIATYPE_VBI && pmt->subtype == MEDIASUBTYPE_TELETEXT)
								{
									(*vbi = pin)->AddRef();

									return S_OK;
								}

								return S_CONTINUE;
							});

							return S_CONTINUE;
						});

						return S_OK;
					}

					hr = pGB->RemoveFilter(pBF);

					ASSERT(SUCCEEDED(hr));

					return S_CONTINUE;
				});

				if(S_OK == hr) 
				{
					break;
				}
			}

			return *vbi != NULL ? S_OK : S_CONTINUE;
		});

		return *vbi != NULL ? S_OK : S_CONTINUE;
	});

	return *vbi != NULL;
}

bool AnalogTuner::AppendMPEG2Encoder(IGraphBuilder2* pGB, CComPtr<IPin>& video, CComPtr<IPin>& audio)
{
	HRESULT hr;

	// audio

	CComPtr<IBaseFilter> aenc = new CMpeg2EncoderFilter();

	hr = pGB->AddFilter(aenc, L"MPEG-2 Audio Encoder");

	if(FAILED(hr))
	{
		return false;
	}

	hr = pGB->ConnectFilterDirect(audio, aenc, NULL);

	if(FAILED(hr)) 
	{
		pGB->RemoveFilter(aenc); 

		return false;
	}

	// video

	CComPtr<IBaseFilter> venc = new CMpeg2EncoderFilter();

	hr = pGB->AddFilter(venc, L"MPEG-2 Video Encoder");

	if(FAILED(hr)) 
	{
		return false;
	}

	hr = pGB->ConnectFilterDirect(video, venc, NULL);

	if(FAILED(hr)) 
	{
		CComPtr<IBaseFilter> cc = new CColorConverterFilter();

		hr = pGB->AddFilter(cc, L"Color Converter");

		if(FAILED(hr)) 
		{
			pGB->RemoveFilter(venc); 
			
			return false;
		}

		hr = pGB->ConnectFilterDirect(video, cc, NULL);

		if(FAILED(hr)) 
		{
			pGB->RemoveFilter(venc); 
			pGB->RemoveFilter(cc); 
			
			return false;
		}

		hr = pGB->ConnectFilterDirect(cc, venc, NULL);

		if(FAILED(hr)) 
		{
			pGB->RemoveFilter(venc); 
			pGB->RemoveFilter(cc); 
			
			return false;
		}
	}

	video = DirectShow::GetFirstPin(venc, PINDIR_OUTPUT);
	audio = DirectShow::GetFirstPin(aenc, PINDIR_OUTPUT);

	return true;
}

bool AnalogTuner::RenderTuner(IGraphBuilder2* pGB)
{
	HRESULT hr;

	CComPtr<IBaseFilter> pBF;

	std::wstring id = m_desc.id;

	std::wstring::size_type pos = id.find(L"$homesys$");

	if(pos != std::wstring::npos)
	{
		id = id.substr(0, pos);
	}

	hr = pGB->AddFilterForDisplayName(id.c_str(), &pBF);

	if(FAILED(hr)) 
	{
		return false;
	}

	CComPtr<ICaptureGraphBuilder2> pCGB;

	if(FAILED(pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2))) 
	{
		return false;
	}

	if(FAILED(pCGB->SetFiltergraph(pGB))) 
	{
		return false;
	}

	hr = pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pBF, IID_IAMStreamConfig, (void**)&m_video);
	
	if(FAILED(hr)) 
	{
		return false;
	}

	hr = pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pBF, IID_IAMStreamConfig, (void**)&m_audio);
	
	if(FAILED(hr)) 
	{
		// return false;
	}

	hr = pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pBF, IID_IAMCrossbar, (void**)&m_xbar);
	
	if(FAILED(hr)) 
	{
		return false;
	}

	hr = pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pBF, IID_IAMTVTuner, (void**)&m_tvtuner);

	if(FAILED(hr)) 
	{
		return false;
	}

	hr = pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pBF, IID_IAMTVAudio, (void**)&m_tvaudio);
	
	if(FAILED(hr))
	{
		return false;
	}

	hr = pCGB->FindInterface(NULL, NULL, pBF, IID_IAMAnalogVideoDecoder, (void**)&m_avdec);
	
	if(FAILED(hr)) 
	{
		return false;
	}

	// connect pins that were left disconnected between the xbar and the capture filter

	hr = pGB->ConnectFilterDirect(CComQIPtr<IBaseFilter>(m_xbar), pBF, NULL);

	//

	if(CComQIPtr<IAMAnalogVideoDecoder> pAVD = pBF)
	{
		hr = pAVD->get_AvailableTVFormats((long*)&m_tvformats);

		if(FAILED(hr)) 
		{
			return false;
		}

		hr = pAVD->put_TVFormat(AnalogVideo_PAL_B);
		
		if(FAILED(hr)) 
		{
			return false;
		}
	}

	hr = m_tvtuner->put_CountryCode(49);

	if(FAILED(hr)) 
	{
		return false;
	}

	hr = m_tvtuner->put_InputType(0, TunerInputCable);

	if(FAILED(hr)) 
	{
		return false;
	}

	hr = m_tvtuner->put_TuningSpace(m_tuningspace);

	if(FAILED(hr)) 
	{
		return false;
	}

	return CreateOutput(pGB, pBF);
}

void AnalogTuner::GetConnectorTypes(std::list<PhysicalConnectorType>& types)
{
	types.clear();

	if(m_xbar != NULL)
	{
		long InputPinCount = 0, OutputPinCount;

		if(SUCCEEDED(m_xbar->get_PinCounts(&OutputPinCount, &InputPinCount)))
		{
			for(int i = 0; i < InputPinCount; i++)
			{
				long PinIndexRelated, PhysicalType;

				if(SUCCEEDED(m_xbar->get_CrossbarPinInfo(TRUE, i, &PinIndexRelated, &PhysicalType)))
				{
					if(PhysicalType < PhysConn_Audio_Tuner)
					{
						types.push_back((PhysicalConnectorType)PhysicalType);
					}
				}
			}
		}
	}
}

void AnalogTuner::Stop()
{
	HRESULT hr;

	if(m_dst != NULL)
	{
		if(m_pin.vbi != NULL)
		{
			hr = DirectShow::GetFilter(m_pin.vbi)->Stop(); // stop vbi codec first, or else it may crash
		}
	}

	__super::Stop();

	if(m_dst != NULL)
	{
		m_dst->NukeDownstream(m_pin.video);
		m_dst->NukeDownstream(m_pin.audio);
		m_dst->NukeDownstream(m_pin.vbi);

		m_dst->Clean();
	}
}

int AnalogTuner::GetStandard()
{
	long lAnalogVideoStandard = 0;

	if(m_avdec)
	{
		m_avdec->get_TVFormat(&lAnalogVideoStandard);
	}

	return (int)lAnalogVideoStandard;
}

int AnalogTuner::GetConnector()
{
	if(m_xbar)
	{
		long InputPinCount = 0, OutputPinCount, InputPinIndex = 0;

		if(SUCCEEDED(m_xbar->get_PinCounts(&OutputPinCount, &InputPinCount))
		&& SUCCEEDED(m_xbar->get_IsRoutedTo(0, &InputPinIndex)))
		{
			for(int i = 0, j = 0; i < InputPinCount; i++)
			{
				long PinIndexRelated, PhysicalType;

				if(FAILED(m_xbar->get_CrossbarPinInfo(TRUE, i, &PinIndexRelated, &PhysicalType)))
					continue;

				if(PhysicalType < PhysConn_Audio_Tuner)
				{
					if(i == InputPinIndex)
					{
						return j;
					}

					j++;
				}
			}
		}
	}

	return -1;
}

int AnalogTuner::GetFreq()
{
	long lFreq = 0;

	if(m_tvtuner)
	{
		m_tvtuner->get_VideoFrequency(&lFreq);
	}

	return (int)lFreq / 1000;
}

void AnalogTuner::GetSignal(TunerSignal& signal)
{
	signal.Reset();

	long stength = 0;

	if(m_tvtuner)
	{
		if(SUCCEEDED(m_tvtuner->SignalPresent(&stength)))
		{
			signal.locked = stength > 0 ? 1 : 0;
			signal.present = stength > 0 ? 1 : 0;
			signal.strength = stength;
		}
	}
}

void AnalogTuner::SetStandard(int standard)
{
	if(m_avdec && m_standard != standard)
	{
		int formats = m_tvformats & standard;

		for(int i = 0; i < 32; i++)
		{
			if(int j = (formats & (1 << i)))
			{
				if(SUCCEEDED(m_avdec->put_TVFormat(j)))
				{
					m_standard = standard;

					break;
				}
			}
		}
	}
}

void AnalogTuner::RouteConnector(int num)
{
	if(m_xbar && m_connector != num)
	{
		long PinIndexRelated, PhysicalType;
		long OutputPinCount, InputPinCount;
		
		if(SUCCEEDED(m_xbar->get_CrossbarPinInfo(TRUE, num, &PinIndexRelated, &PhysicalType))
		&& SUCCEEDED(m_xbar->get_PinCounts(&OutputPinCount, &InputPinCount)))
		{
			for(int i = 0; i < OutputPinCount; i++)
			{
				if(S_OK == m_xbar->CanRoute(i, num))
				{
					m_xbar->Route(i, num);
					break;
				}
			}

			if(PinIndexRelated >= 0)
			{
				for(int i = 0; i < OutputPinCount; i++)
				{
					if(S_OK == m_xbar->CanRoute(i, PinIndexRelated))
					{
						m_xbar->Route(i, PinIndexRelated);
						break;
					}
				}
			}

			m_connector = num;
			m_connectorType = PhysicalType;
		}
	}
}

void AnalogTuner::SetConnector(int num)
{
	if(m_xbar)
	{
		long InputPinCount = 0, OutputPinCount;

		if(SUCCEEDED(m_xbar->get_PinCounts(&OutputPinCount, &InputPinCount)))
		{
			for(int i = 0; i < InputPinCount; i++)
			{
				long PinIndexRelated, PhysicalType;

				if(FAILED(m_xbar->get_CrossbarPinInfo(TRUE, i, &PinIndexRelated, &PhysicalType)))
					continue;

				if(PhysicalType < PhysConn_Audio_Tuner)
				{
					if(--num < 0)
					{
						RouteConnector(i);
						break;
					}
				}
			}
		}
	}
}

#define INSTANCEDATA_OF_PROPERTY_PTR(x) ((PKSPROPERTY((x))) + 1)
#define INSTANCEDATA_OF_PROPERTY_SIZE(x) (sizeof((x)) - sizeof(KSPROPERTY))

void AnalogTuner::SetFrequency(int frequency)
{
	if(!m_tvtuner) return;

	frequency *= 1000;
/*
	HRESULT hr;

	CComPtr<IKsPropertySet> pKSProp;

	if(SUCCEEDED(m_tvtuner->QueryInterface(IID_IKsPropertySet, (void**)&pKSProp)))
	{
		KSPROPERTY_TUNER_MODE_CAPS_S c;
		KSPROPERTY_TUNER_FREQUENCY_S f;
		
		memset(&c, 0, sizeof(KSPROPERTY_TUNER_MODE_CAPS_S));
		memset(&f, 0, sizeof(KSPROPERTY_TUNER_FREQUENCY_S));
		
		DWORD dwSupported;

		if(SUCCEEDED(pKSProp->QuerySupported(PROPSETID_TUNER, KSPROPERTY_TUNER_MODE_CAPS, &dwSupported))
		&& (dwSupported & KSPROPERTY_SUPPORT_GET))
		{
			DWORD cbBytes = 0;

			if(SUCCEEDED(hr = pKSProp->Get(PROPSETID_TUNER, KSPROPERTY_TUNER_MODE_CAPS, INSTANCEDATA_OF_PROPERTY_PTR(&c), INSTANCEDATA_OF_PROPERTY_SIZE(c), &c, sizeof(c), &cbBytes)))
			{
				f.Frequency = (ULONG)frequency;
				f.TuningFlags = c.Strategy == KS_TUNER_STRATEGY_DRIVER_TUNES ? KS_TUNER_TUNING_FINE : KS_TUNER_TUNING_EXACT;

				if(frequency >= c.MinFrequency && frequency <= c.MaxFrequency)
				{
					if(SUCCEEDED(hr = pKSProp->Set(PROPSETID_TUNER, KSPROPERTY_TUNER_FREQUENCY, INSTANCEDATA_OF_PROPERTY_PTR(&f), INSTANCEDATA_OF_PROPERTY_SIZE(f), &f, sizeof(f))))
					{
						return;
					}
				}
			}
		}
	}
*/
	int channel = (frequency - ANALOG_FREQUENCY_MIN + 125000) / 250000 + 1;

	if(channel < 1) return;

	HRESULT hr = S_OK;

	if(m_channel != channel && m_connectorType == PhysConn_Video_Tuner) 
	{
		int i = channel / 1000;
		int j = channel % 1000;

		hr = m_tvtuner->put_Mode(AMTUNER_MODE_TV);

		long tuningspace = -1;

		if(FAILED(m_tvtuner->get_TuningSpace(&tuningspace)) || tuningspace < 0 || tuningspace != m_tuningspace + i)
		{
			hr = m_tvtuner->put_TuningSpace(m_tuningspace + i);
		}
		
		hr = m_tvtuner->put_InputType(0, TunerInputCable);

		if(SUCCEEDED(hr = m_tvtuner->put_Channel(j, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT)))
		{
			m_channel = channel;
		}
	}
}

bool AnalogTuner::Tune(const GenericPresetParam* p)
{
	if(const AnalogPresetParam* pp = dynamic_cast<const AnalogPresetParam*>(p))
	{
		SetStandard(pp->Standard);
		SetConnector(pp->Connector);
		SetFrequency(pp->Frequency);

		return true;
	}

	return false;
}

bool AnalogTuner::TuneMayChangeFormat()
{
	return false;
}

void AnalogTuner::GetPrograms(std::list<TunerProgram>& programs)
{
	programs.clear();

	TunerSignal signal;

	GetSignal(signal);

	if(signal.present > 0)
	{
		TunerProgram p;

		// p.provider = L"";
		p.name = Util::Format(L"%d KHz", GetFreq());

		// TODO: streams

		programs.push_back(p);
	}
}

bool AnalogTuner::GetOutput(std::list<IPin*>& video, std::list<IPin*>& audio, IPin*& vbi)
{
	video.push_back(m_pin.video);
	audio.push_back(m_pin.audio);
	vbi = m_pin.vbi;

	return true;
}

int AnalogTuner::GenUniqueTuningSpace()
{
	CRegKey key;

	if(ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\TV System Services\\TVAutoTune")))
	{
		return -1;
	}

	static int index = 5000;

	wchar_t buff[256];
	DWORD len = countof(buff);

	for(int i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len, NULL); i++, len = countof(buff))
	{
		std::wstring s = buff;

		if(s.size() > 2 && s.substr(0, 2) == L"TS")
		{
			if(_wtoi(s.substr(2).c_str()) == index)
			{
				return index;
			}
		}
	}

	InitTuningSpace(index);

	return index;
}

bool AnalogTuner::InitTuningSpace(int tuningspace)
{
	CRegKey key;

	if(ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\TV System Services\\TVAutoTune"))
	{
		return false;
	}

	CRegKey subkey;

	int offset = -1;

	for(int i = 0, freq = ANALOG_FREQUENCY_MIN; freq < ANALOG_FREQUENCY_MAX; i++, freq += 250000)
	{
		int j = i / 1000;
		int channel = i % 1000;

		if(offset != j)
		{
			offset = j;

			std::wstring s = Util::Format(L"TS%d-1", tuningspace + offset);

			key.DeleteSubKey(s.c_str());

			if(ERROR_SUCCESS != subkey.Create(key, s.c_str()))
			{
				return false;
			}
		}

		subkey.SetDWORDValue(Util::Format(L"%d", channel).c_str(), freq);
	}

	return true;
}
