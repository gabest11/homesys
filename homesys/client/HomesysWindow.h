#pragma once

#include "LoaderThread.h"

#include "MenuMain.h"
#include "MenuTV.h"
#include "MenuTVOSD.h"
#include "MenuPresets.h"
#include "MenuPresetsOrder.h"
#include "MenuRecord.h"
#include "MenuRecordings.h"
#include "MenuRecordingModify.h"
#include "MenuWishlist.h"
#include "MenuEPG.h"
#include "MenuEPGChannel.h"
#include "MenuEPGSearch.h"
#include "MenuDVD.h"
#include "MenuAudioCD.h"
#include "MenuDVDOSD.h"
#include "MenuTuner.h"
#include "MenuSmartCard.h"
#include "MenuTuning.h"
#include "MenuEditText.h"
#include "MenuFileBrowser.h"
#include "MenuImageBrowser.h"
#include "MenuSettings.h"
#include "MenuSettingsPersonal.h"
#include "MenuVideoSettings.h"
#include "MenuAudioSettings.h"
#include "MenuSubtitleSettings.h"
#include "MenuHotLinkSettings.h"
#include "MenuSignup.h"
#include "MenuParental.h"
#include "MenuTeletext.h"

using namespace DXUI;

class HomesysWindow 
	: public PlayerWindow
	, public Homesys::ICallback
{
	class HomesysMenu
	{
	public:
		MenuMain main;
		MenuTV tv;
		MenuTVOSD tvosd;
		MenuTVOSD webtvosd;
		MenuPresets presets;
		MenuPresetsOrder presetsorder;
		MenuRecord record;
		MenuRecordings recordings;
		MenuRecordingModify recordingmodify;
		MenuWishlist wishlist;
		MenuEPG epg;
		MenuEPGChannel epgchannel;
		MenuEPGSearch epgsearch;
		MenuDVD dvd;
		MenuAudioCD audiocd;
		MenuTuner tuner;
		MenuSmartCard smartcard;
		MenuTuning tuning;
		MenuDVDOSD fileosd;
		MenuEditText edittext;
		MenuFileBrowser file;
		MenuImageBrowser image;
		MenuSettings settings;
		MenuSettingsPersonal personal;
		MenuVideoSettings video;
		MenuAudioSettings audio;
		MenuSubtitleSettings subtitle;
		MenuHotLinkSettings hotlink;
		MenuSignup signup;
		MenuParental parental;
		MenuTeletext teletext;
	};

	HomesysMenu* m_menu;
	bool m_fullrange;
	clock_t m_fullrange_changed;
	bool m_pip;
	int m_arpreset;
	clock_t m_arpreset_changed;
	UINT m_timer_fast;
	UINT m_timer_slow;
	Homesys::CallbackService* m_callback;
	struct {bool broadcast, streaming;} m_tv;
	Homesys::MediaRendererWrapper* m_mr;
	int m_debug;
	
	void PreCreate(WNDCLASS* wc, DWORD& style, DWORD& exStyle);

	HRESULT OpenFile(LPCTSTR path);
	HRESULT OpenTV(bool broadcast, bool streaming);
	void ClosePlayers();

	void PrintScreen(bool ext);

	struct
	{
		std::vector<float> fmax;
		std::vector<int> ttl;
	} m_fft;

	void PaintAudio(IGenericPlayer* player, DeviceContext* dc);

protected:
	LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	
	bool OnCreate();
	void OnDestroy();
	void OnTimer(UINT id);
	void OnPaint(DeviceContext* dc);
	void OnPaintBusy(DeviceContext* dc);
	bool OnInput(const KeyEventParam& p);
	bool OnInput(const MouseEventParam& p);
	void OnBeforeResetDevice();
	void OnAfterResetDevice();
	void OnDropFiles(const std::list<std::wstring>& files);

	void OnDeviceChange(WPARAM wParam);
	LRESULT OnPowerBroadcast(WPARAM wParam);
	LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
	LRESULT OnReactivateTuner(WPARAM wParam, LPARAM lParam);
	void OnUserInputMessage(Homesys::UserInputType type, int value);
	bool OnSysCommand(UINT source, UINT id, HANDLE handle);
	void OnUPnPMediaRenderer(WPARAM wParam, LPARAM lParam);

	bool OnFileInfo(Control* c);
	bool OnPlayFile(Control* c, std::wstring file);
	bool OnOpenFile(Control* c, std::wstring file);
	bool OnPlayerClose(Control* c);
	bool OnMute(Control* c);
	bool OnFullRange(Control* c);
	bool OnAspectRatio(Control* c);
	bool OnPIP(Control* c);
	bool OnVideoClicked(Control* c);
	bool OnRegisterUser(Control* c);
	bool OnRecordClicked(Control* c);
	bool OnChannelDetails(Control* c);
	bool OnModifyRecording(Control* c, GUID recordingId);
	bool OnTeletext(Control* c);
	bool OnEditText(Control* c);
	bool OnMenuMainClicked(Control* c, int id);
	bool OnMenuTVClicked(Control* c, int id);
	bool OnMenuTVActivating(Control* c);
	bool OnMenuDVDClicked(Control* c, int id);

	// ICallback

	void OnUserInput(Homesys::UserInputType type, int value);
	void OnCurrentPresetChanged(const GUID& tunerId);

public:
	HomesysWindow(LoaderThread* loader, DWORD flags);
	virtual ~HomesysWindow();
};

