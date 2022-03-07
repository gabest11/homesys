#pragma once

#include "GenericTuner.h"

#define ANALOG_FREQUENCY_MIN   45750000
#define ANALOG_FREQUENCY_MAX 1000000000

class AnalogPresetParam : public GenericPresetParam
{
public:
	AnalogVideoStandard Standard;
	int Connector;
	int Frequency;

	AnalogPresetParam()
	{
		Standard = (AnalogVideoStandard)AnalogVideoMask_MCE_PAL;
		Connector = 0;
		Frequency = 0;
	}
};

union AnalogTunerCaps
{
	VIDEO_STREAM_CONFIG_CAPS video;
	AUDIO_STREAM_CONFIG_CAPS audio;
};

class AnalogTuner : public GenericTuner
{
	bool m_compressed;

	CComPtr<IAMStreamConfig> m_video;
	CComPtr<IAMStreamConfig> m_audio;
	CComPtr<IAMAnalogVideoDecoder> m_avdec;
	CComPtr<IAMCrossbar> m_xbar;
	CComPtr<IAMTVTuner> m_tvtuner;
	CComPtr<IAMTVAudio> m_tvaudio;
	struct {CComPtr<IPin> video, audio, vbi;} m_pin;

	int m_tuningspace;
	int m_tvformats;
	int m_standard;
	int m_connector;
	int m_connectorType;
	int m_channel;

	bool CreateOutput(IGraphBuilder2* pGB, IBaseFilter* capture);
	bool CreateMPEG2PSOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** video, IPin** audio);
	bool CreateMPEG2ESOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** video, IPin** audio);
	bool CreateUncompressedOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** video, IPin** audio);
	bool CreateVBIOutput(IGraphBuilder2* pGB, IBaseFilter* capture, IPin** vbi);
	bool AppendMPEG2Encoder(IGraphBuilder2* pGB, CComPtr<IPin>& video, CComPtr<IPin>& audio);
	
	void RouteConnector(int num);

	int GenUniqueTuningSpace();
	bool InitTuningSpace(int tuningspace);

protected:
	bool RenderTuner(IGraphBuilder2* pGB);

public:
	AnalogTuner(bool compressed);
	virtual ~AnalogTuner();

	void Close();

	void GetConnectorTypes(std::list<PhysicalConnectorType>& types);

	void Stop();

	int GetStandard();
	int GetConnector();
	int GetFreq();
	void GetSignal(TunerSignal& signal);

	void SetStandard(int standard);
	void SetConnector(int num);
	void SetFrequency(int frequency);
	void SetCable(bool cable);

	bool Tune(const GenericPresetParam* p);
	bool TuneMayChangeFormat();

	void GetPrograms(std::list<TunerProgram>& programs);

	bool GetOutput(std::list<IPin*>& video, std::list<IPin*>& audio, IPin*& vbi);
};
