#pragma once

#include <wmsdk.h>
#include <wmsysprf.h>
#include <dshowasf.h>
#include <tuner.h>
#include <time.h>
#include "../3rdparty/baseclasses/baseclasses.h"
#include "FGManager.h"
#include "SmartCard.h"

struct TunerDesc
{
	enum Type {None, Analog, DVBT, DVBS, DVBC, DVBF} type;
	std::wstring id;
	std::wstring name;

	struct TunerDesc() {type = None;}
};

struct TunerState 
{
	enum Type {None, Streaming, Recording, Timeshifting};
};

class TunerProgram
{
public:
	std::wstring name;
	std::wstring provider;
	int type, pid, sid, pcr;
	bool scrambled;

	TunerProgram()
	{
		type = pid = sid = pcr = 0;
		scrambled = false;
	}
};

class TunerSignal
{
public:
	int present;
	int locked;
	int strength;
	int quality;

	TunerSignal()
	{
		Reset();
	}

	void Reset()
	{
		present = locked = -1;
		strength = quality = 0;
	}
};

class GenericPresetParam
{
public:
	std::wstring name;

	virtual ~GenericPresetParam() {}

	virtual std::wstring ToString() const
	{
		return Util::Format(L"name=%s", name.c_str());
	}
};

class GenericTuner 
	: public CAMThread
	, public CCritSec
{
protected:
	CComPtr<IGraphBuilder2> m_src;
	CComPtr<IGraphBuilder2> m_dst;
	TunerDesc m_desc;
	CComPtr<ISmartCardServer> m_scs;
	TunerState::Type m_state;	
	bool m_running;
	clock_t m_start;
	int m_timeout;
	CAMEvent m_exit;
	std::wstring m_mdsm;

	DWORD ThreadProc();

	virtual bool RenderTuner(IGraphBuilder2* pGB) = 0;
	virtual void OnEvent(long code, LONG_PTR param[2]) {}
	virtual void OnIdle();

	bool RenderAsf(IWMWriterAdvanced2** ppWMWA2, int profile = -1);
	bool RenderMDSM(LPCWSTR path, LPCWSTR id);

public:
	GenericTuner();
	virtual ~GenericTuner();

	const TunerDesc& GetDesc() {return m_desc;}

	virtual bool Open(const TunerDesc& desc, ISmartCardServer* scs);
	virtual void Close();

	IGraphBuilder2* GetSrcGraph() {return m_src;}
	IGraphBuilder2* GetDstGraph() {return m_dst;}

	bool IsRunning() {return m_running;}
	TunerState::Type GetTunerState() {return m_state;}
	int GetSignalStrength() {TunerSignal signal; GetSignal(signal); return signal.strength;}
	LPCWSTR GetTimeshiftFile() const {return m_mdsm.c_str();}

	void MoveVideoRect();

	virtual bool Render(HWND hWnd);
	virtual bool Stream(int port, int profile);
	virtual bool Record(LPCWSTR path, int profile, int timeout);
	virtual bool Timeshift(LPCWSTR path, LPCWSTR id);
	virtual bool Timeshift(LPCWSTR path, LPCWSTR id, const GenericPresetParam* p);
	virtual bool IsTimeshiftDummy() const {return false;}

	virtual bool StartRecording(LPCWSTR path);
	virtual void StopRecording();

	virtual void Run();
	virtual void Stop();

	virtual int GetFreq() {return 0;}
	virtual void GetSignal(TunerSignal& signal) = 0;
	virtual __int64 GetReceivedBytes() {return 0;}
	virtual bool IsScrambled() {return false;}
	virtual void GetConnectorTypes(std::list<PhysicalConnectorType>& types) = 0;
	virtual bool GetOutput(std::list<IPin*>& video, std::list<IPin*>& audio, IPin*& vbi) = 0;

	virtual bool Tune(const GenericPresetParam* p) = 0;
	virtual bool TuneMayChangeFormat() = 0;

	virtual void GetPrograms(std::list<TunerProgram>& programs) = 0;

	static void EnumTuners(std::list<TunerDesc>& tds, bool compressed, bool tsreader);
	static void EnumTunersNoType(std::list<TunerDesc>& tds);

	static std::wstring ToString(PhysicalConnectorType type);
	static bool IsTunerConnectorType(PhysicalConnectorType type);
};
