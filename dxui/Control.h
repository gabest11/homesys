#pragma once

#include "Common.h"
#include "Property.h"
#include "DeviceContext.h"
#include "Animation.h"
#include "../3rdparty/baseclasses/baseclasses.h"
#include "../util/Vector.h"
#include <list>
#include <string>
#include <functional>
#include <atltime.h>

namespace DXUI
{
	class Control : public AlignedClass<16>
	{
	public:
		static CCritSec m_lock;
		static int m_zorder_min;
		static int m_zorder_max;
	
	private:
		std::list<Control*> m_children;
		Control* m_focused;
		std::list<Control*> m_captured;
		int m_zorder;
		Anchor m_anchor[4];
		Animation* m_scroll;
		int m_flags;

		struct ControlFlag {enum {Visible = 1, Enabled = 2};};

		void GetVisible(bool& value);
		void SetVisible(bool& value);
		void GetEnabled(bool& value);
		void SetEnabled(bool& value);
		void GetFocused(bool& value);
		void SetFocused(bool& value);
		void GetFocusedPath(bool& value);
		void GetCaptured(bool& value);
		void SetCaptured(bool& value);
		void SetAlpha(float& value);
		void GetLeft(int& value);
		void SetLeft(int& value);
		void GetTop(int& value);
		void SetTop(int& value);
		void GetRight(int& value);
		void SetRight(int& value);
		void GetBottom(int& value);
		void SetBottom(int& value);
		void GetWidth(int& value);
		void SetWidth(int& value);
		void GetHeight(int& value);
		void SetHeight(int& value);
		void GetSize(Vector2i& value);
		void SetSize(Vector2i& value);
		void GetClientRect(Vector4i& rect);
		void GetWindowRect(Vector4i& rect);
		void GetTextHeight(int& value);
		void GetTextBold(bool& value);
		void SetTextBold(bool& value);
		void GetTextItalic(bool& value);
		void SetTextItalic(bool& value);
		void GetTextUnderline(bool& value);
		void SetTextUnderline(bool& value);
		void GetTextOutline(bool& value);
		void SetTextOutline(bool& value);
		void GetTextShadow(bool& value);
		void SetTextShadow(bool& value);
		void GetTextEscaped(bool& value);
		void SetTextEscaped(bool& value);
		void GetTextWrap(bool& value);
		void SetTextWrap(bool& value);
		void SetTextMarginLeft(int& value);
		void SetTextMarginTop(int& value);
		void SetTextMarginRight(int& value);
		void SetTextMarginBottom(int& value);
		void SetBackgroundMarginLeft(int& value);
		void SetBackgroundMarginTop(int& value);
		void SetBackgroundMarginRight(int& value);
		void SetBackgroundMarginBottom(int& value);
		void SetHoverMarginLeft(int& value);
		void SetHoverMarginTop(int& value);
		void SetHoverMarginRight(int& value);
		void SetHoverMarginBottom(int& value);
		void SetHoverMarginAll(int& value);
		void GetHoverRect(Vector4i& value);
		void SetAnchorRect(Vector4i& value);

		bool OnParentChanging(Control* c, Control* value);
		bool OnParentChanged(Control* c, Control* value);
		bool OnPaintBackground(Control* c, DeviceContext* dc);
		bool OnPaintForeground(Control* c, DeviceContext* dc);

	protected:
		virtual Texture* GetBackground(DeviceContext* dc, Vector4i& src, Vector4i& dst);

		void SetTextProperties(DeviceContext* dc);

	public:
		Control();
		virtual ~Control();

		virtual bool Create(const Vector4i& rect, Control* parent);

		virtual bool Message(const KeyEventParam& p);
		virtual bool Message(const MouseEventParam& p);

		void Activate(int dir);
		void Deactivate(int dir);
		void Close();
		void Paint(DeviceContext* dc);

		typedef std::tr1::function<bool (const Control*, const Control*)> SortFunct;

		void GetSiblings(std::list<Control*>& controls);
		void GetDescendants(std::list<Control*>& controls, bool visibleOnly, bool enabledOnly, SortFunct sort = SortFunct());
		
		Control* GetCheckedByGroup(int group);

		void FindFocus(std::list<Control*>& controls);

		Control* GetRoot();
		Control* GetFocus();
		Control* GetCapture();

		Vector2i C2W(const Vector2i& p);
		Vector2i W2C(const Vector2i& p);
		Vector4i C2W(const Vector4i& r);
		Vector4i W2C(const Vector4i& r);
		Vector2i Map(Control* to, const Vector2i& p);
		Vector4i Map(Control* to, const Vector4i& r);
		Vector4i MapClientRect(Control* control);

		void Move(const Vector4i& rect, bool anchor = true);

		void ToFront();
		void ToBack();

		void DrawScrollText(DeviceContext* dc, LPCWSTR str, const Vector4i& r);
		void ResetScrollText();

		static void SetLanguage(LPCWSTR lang);
		static void SetString(LPCSTR id, LPCWSTR str, LPCWSTR lang = NULL);
		static LPCWSTR GetString(LPCSTR id, LPCWSTR lang = NULL);

		Property<Control*> Parent;
		Property<Control*> Default;
		Property<bool> Visible;
		Property<bool> Enabled;
		Property<bool> Activated;
		Property<bool> Focused;
		Property<bool> FocusedPath;
		Property<int> CanFocus;
		Property<bool> Hovering;
		Property<bool> Captured;
		Property<bool> Checked;
		Property<int> Group;
		Property<UINT_PTR> UserData;
		Property<bool> Transparent;
		Property<bool> ClickThrough;
		Property<float> Alpha;
		Property<Vector4i> OriginalRect;
		Property<Vector4i> Rect;
		Property<int> Left;
		Property<int> Top;
		Property<int> Right;
		Property<int> Bottom;
		Property<int> Width;
		Property<int> Height;
		Property<Vector2i> Size;
		Property<Vector4i> ClientRect;
		Property<Vector4i> WindowRect;
		Property<std::wstring> Text;
		Property<int> TextAlign;
		Property<DWORD> TextColor;
		Property<std::wstring> TextFace;
		Property<int> TextHeight;
		Property<int> TextStyle;
		Property<bool> TextBold;
		Property<bool> TextItalic;
		Property<bool> TextUnderline;
		Property<bool> TextOutline;
		Property<bool> TextShadow;
		Property<bool> TextEscaped;
		Property<bool> TextWrap;
		Property<Vector4i> TextMargin;
		Property<int> TextMarginLeft;
		Property<int> TextMarginTop;
		Property<int> TextMarginRight;
		Property<int> TextMarginBottom;
		Property<DWORD> BackgroundColor;
		Property<int> BackgroundStyle;
		Property<Vector4i> BackgroundMargin;
		Property<int> BackgroundMarginLeft;
		Property<int> BackgroundMarginTop;
		Property<int> BackgroundMarginRight;
		Property<int> BackgroundMarginBottom;
		Property<bool> BackgroundMarginInflate;
		Property<Vector4i> HoverMargin;
		Property<int> HoverMarginLeft;
		Property<int> HoverMarginTop;
		Property<int> HoverMarginRight;
		Property<int> HoverMarginBottom;
		Property<int> HoverMarginAll;
		Property<Vector4i> HoverRect;
		Property<Vector4i> AnchorRect;
		Property<std::wstring> BackgroundImage;
		Property<std::wstring> HoverImage;
		Property<std::wstring> DisabledImage;
		Property<std::wstring> FocusedImage;
		Property<bool> TabLeft;
		Property<bool> TabTop;
		Property<bool> TabRight;
		Property<bool> TabBottom;
		Property<std::wstring> RedInfo;
		Property<std::wstring> GreenInfo;
		Property<std::wstring> YellowInfo;
		Property<std::wstring> BlueInfo;
		Property<bool> Scroll;
		Property<bool> ScrollNoFocus;

		static Property<DeviceContext*> DC;

		Event<DeviceContext*> PaintBeginEvent;
		Event<DeviceContext*> PaintBackgroundEvent;
		Event<DeviceContext*> PaintForegroundEvent;
		Event<DeviceContext*> PaintEndEvent;
		Event<const KeyEventParam&> KeyEvent;
		Event<const MouseEventParam&> MouseEvent;
		Event<> RedEvent;
		Event<> GreenEvent;
		Event<> YellowEvent;
		Event<> BlueEvent;
		Event<> ActivatingEvent;
		Event<int> ActivatedEvent;
		Event<> DeactivatingEvent;
		Event<int> DeactivatedEvent;
		Event<> ClosedEvent;
		Event<> FocusSetEvent;
		Event<> FocusLostEvent;

		static Event<> UserActionEvent;
	};
}
