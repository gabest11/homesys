#pragma once

#include "GenericTuner.h"
#include "TSDemux.h"
#include "TSSink.h"
#include "WinTVCI.h"
#include "../3rdparty/anysee/anyseeNIMEx.h"

class DVBPresetParam : public GenericPresetParam
{
public:
	int sid;

	// ILocator

	long CarrierFrequency; 
	FECMethod InnerFEC;
	BinaryConvolutionCodeRate InnerFECRate;
	FECMethod OuterFEC; 
	BinaryConvolutionCodeRate OuterFECRate;
	ModulationType Modulation;
	long SymbolRate;

	// IDVBTuningSpace2

	long NetworkID;

	DVBPresetParam()
	{
		sid = 0;

		CarrierFrequency = BDA_FREQUENCY_NOT_SET;
		InnerFEC = BDA_FEC_METHOD_NOT_SET;
		InnerFECRate = BDA_BCC_RATE_NOT_SET;
		OuterFEC = BDA_FEC_METHOD_NOT_SET; 
		OuterFECRate = BDA_BCC_RATE_NOT_SET;
		Modulation = BDA_MOD_NOT_SET;
		SymbolRate = BDA_BCC_RATE_NOT_SET;

		NetworkID = -1;
	}

	std::wstring ToString() const
	{
		return __super::ToString() + Util::Format(
			L", sid=%d, CarrierFrequency=%d, InnerFEC=%d, InnerFECRate=%d, OuterFEC=%d, OuterFECRate=%d, Modulation=%d, SymbolRate=%d, NetworkID=%d",
			sid, CarrierFrequency, InnerFEC, InnerFECRate, OuterFEC, OuterFECRate, Modulation, SymbolRate, NetworkID
			);
	}
};

class DVBTuner : public GenericTuner
{
	class AnyseeTuner
	{
		CanyseeNIMEx* m_nim;

		static DWORD WINAPI ThreadProc(void* p);

	public:
		AnyseeTuner();
		virtual ~AnyseeTuner();

		bool Open();
		void Close();

		bool SetMode(ModulationType modulation, BinaryConvolutionCodeRate ifecrate);
		void SelectLNB(BYTE DiSEqC10Flag = 0, bool bDiSEqC1v1 = false);
		int SendDiSEqCData(PBYTE pData, DWORD dwLength, EnumDiSEqCToneBurst ToneBurst = No_DiSEqCToneBurst);
	};

	class TechnotrendTuner
	{
		HANDLE m_device;

	public:
		TechnotrendTuner();
		virtual ~TechnotrendTuner();

		static bool IsCompatible(LPCWSTR name);

		bool Open(LPCWSTR name);
		void Close();

		bool Tune(const DVBPresetParam* p);
	};

protected:
	struct {CLSID networkprovider, tuningspace, locator; DVBSystemType type;} m_dvb;
	CComPtr<IBaseFilter> m_networkprovider;
	CComPtr<IBaseFilter> m_tuner;
	CComPtr<IBaseFilter> m_source;
	CComPtr<IBaseFilter> m_writer;
	CComPtr<IBaseFilter> m_demux;
	CComPtr<IKsPropertySet> m_realtek;
	CComPtr<IKsPropertySet> m_firesat;
	AnyseeTuner* m_anysee;
	TechnotrendTuner* m_technotrend;
	CComPtr<IWinTVCI> m_wintvci;
	CComPtr<IGeniaCI> m_geniaci;
	int m_sid;
	int m_freq;
	bool m_dummy;

	bool RenderTuner(IGraphBuilder2* pGB);
	bool WaitSignal();

	virtual bool CreateNetworkProvider(IBaseFilter** ppNP);
	virtual bool GetTuningSpace(ITuningSpace** ppTS);
	virtual HRESULT TuneDVB(const DVBPresetParam* p);

public:
	DVBTuner();
	virtual ~DVBTuner();

	void Close();

	bool Record(LPCWSTR path, int profile, int timeout);
	bool Timeshift(LPCWSTR path, LPCWSTR id, const GenericPresetParam* p);
	bool IsTimeshiftDummy() const {return m_dummy;}

	bool StartRecording(LPCWSTR path);
	void StopRecording();

	void Stop();

	int GetFreq();
	void GetSignal(TunerSignal& signal);
	__int64 GetReceivedBytes();
	bool IsScrambled();
	void GetConnectorTypes(std::list<PhysicalConnectorType>& types);
	bool GetOutput(std::list<IPin*>& video, std::list<IPin*>& audio, IPin*& vbi);

	bool Tune(const GenericPresetParam* p);
	bool TuneMayChangeFormat();

	void GetPrograms(std::list<TunerProgram>& programs);
	WORD GetPort();
};
