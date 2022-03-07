#include "stdafx.h"
#include "Control.h"
#include "Localizer.h"

namespace DXUI
{
	CCritSec Control::m_lock;

	int Control::m_zorder_min = 0;
	int Control::m_zorder_max = 0;

	Property<DeviceContext*> Control::DC;
	
	Event<> Control::UserActionEvent;

	// Control

	Control::Control()
		: m_focused(NULL)
		, m_zorder(0)
		, m_scroll(NULL) 
		, m_flags(0)
	{
		Visible.Get(&Control::GetVisible, this);
		Visible.Set(&Control::SetVisible, this);
		Enabled.Get(&Control::GetEnabled, this);
		Enabled.Set(&Control::SetEnabled, this);
		Focused.Get(&Control::GetFocused, this);
		Focused.Set(&Control::SetFocused, this);
		FocusedPath.Get(&Control::GetFocusedPath, this);
		Captured.Get(&Control::GetCaptured, this);
		Captured.Set(&Control::SetCaptured, this);
		Alpha.Set(&Control::SetAlpha, this);
		Left.Get(&Control::GetLeft, this);
		Left.Set(&Control::SetLeft, this);
		Top.Get(&Control::GetTop, this);
		Top.Set(&Control::SetTop, this);
		Right.Get(&Control::GetRight, this);
		Right.Set(&Control::SetRight, this);
		Bottom.Get(&Control::GetBottom, this);
		Bottom.Set(&Control::SetBottom, this);
		Width.Get(&Control::GetWidth, this);
		Width.Set(&Control::SetWidth, this);
		Height.Get(&Control::GetHeight, this);
		Height.Set(&Control::SetHeight, this);
		Size.Get(&Control::GetSize, this);
		Size.Set(&Control::SetSize, this);
		ClientRect.Get(&Control::GetClientRect, this);
		WindowRect.Get(&Control::GetWindowRect, this);
		TextHeight.Get(&Control::GetTextHeight, this);
		TextBold.Get(&Control::GetTextBold, this);
		TextBold.Set(&Control::SetTextBold, this);
		TextItalic.Get(&Control::GetTextItalic, this);
		TextItalic.Set(&Control::SetTextItalic, this);
		TextUnderline.Get(&Control::GetTextUnderline, this);
		TextUnderline.Set(&Control::SetTextUnderline, this);
		TextOutline.Get(&Control::GetTextOutline, this);
		TextOutline.Set(&Control::SetTextOutline, this);
		TextShadow.Get(&Control::GetTextShadow, this);
		TextShadow.Set(&Control::SetTextShadow, this);
		TextEscaped.Get(&Control::GetTextEscaped, this);
		TextEscaped.Set(&Control::SetTextEscaped, this);
		TextWrap.Get(&Control::GetTextWrap, this);
		TextWrap.Set(&Control::SetTextWrap, this);
		TextMarginLeft.Get([&] (int& value) {value = TextMargin->left;});
		TextMarginLeft.Set(&Control::SetTextMarginLeft, this);
		TextMarginTop.Get([&] (int& value) {value = TextMargin->top;});
		TextMarginTop.Set(&Control::SetTextMarginTop, this);
		TextMarginRight.Get([&] (int& value) {value = TextMargin->right;});
		TextMarginRight.Set(&Control::SetTextMarginRight, this);
		TextMarginBottom.Get([&] (int& value) {value = TextMargin->bottom;});
		TextMarginBottom.Set(&Control::SetTextMarginBottom, this);
		BackgroundMarginLeft.Get([&] (int& value) {value = BackgroundMargin->left;});
		BackgroundMarginLeft.Set(&Control::SetBackgroundMarginLeft, this);
		BackgroundMarginTop.Get([&] (int& value) {value = BackgroundMargin->top;});
		BackgroundMarginTop.Set(&Control::SetBackgroundMarginTop, this);
		BackgroundMarginRight.Get([&] (int& value) {value = BackgroundMargin->right;});
		BackgroundMarginRight.Set(&Control::SetBackgroundMarginRight, this);
		BackgroundMarginBottom.Get([&] (int& value) {value = BackgroundMargin->bottom;});
		BackgroundMarginBottom.Set(&Control::SetBackgroundMarginBottom, this);
		HoverMarginLeft.Get([&] (int& value) {value = HoverMargin->left;});
		HoverMarginLeft.Set(&Control::SetHoverMarginLeft, this);
		HoverMarginTop.Get([&] (int& value) {value = HoverMargin->top;});
		HoverMarginTop.Set(&Control::SetHoverMarginTop, this);
		HoverMarginRight.Get([&] (int& value) {value = HoverMargin->right;});
		HoverMarginRight.Set(&Control::SetHoverMarginRight, this);
		HoverMarginBottom.Get([&] (int& value) {value = HoverMargin->bottom;});
		HoverMarginBottom.Set(&Control::SetHoverMarginBottom, this);
		HoverMarginAll.Set(&Control::SetHoverMarginAll, this);
		HoverRect.Get(&Control::GetHoverRect, this);
		AnchorRect.Set(&Control::SetAnchorRect, this);
		Parent.ChangingEvent.Add(&Control::OnParentChanging, this);
		Parent.ChangedEvent.Add(&Control::OnParentChanged, this);
		PaintBackgroundEvent.Add(&Control::OnPaintBackground, this);
		PaintForegroundEvent.Add(&Control::OnPaintForeground, this);

		OriginalRect = Vector4i::zero();
		Rect = Vector4i::zero();
		Size = Vector2i(0, 0);
		ClientRect = Vector4i::zero();
		WindowRect = Vector4i::zero();
		HoverMargin = Vector4i::zero();
		HoverRect = Vector4i::zero();
		AnchorRect = Vector4i::zero();
		Visible = true;
		Enabled = true;
		Alpha = 1.0f;
		TextAlign = Align::Left | Align::Middle;
		TextColor = Color::Text1;
		TextFace = FontType::Normal;
		TextMargin = Vector4i::zero();
		BackgroundColor = 0;
		BackgroundStyle = BackgroundStyleType::Stretch;
		BackgroundMargin = Vector4i::zero();
		TabLeft = true;
		TabTop = true;
		TabRight = true;
		TabBottom = true;
	}

	Control::~Control()
	{
		Focused.ChangingEvent.clear();
		Focused.ChangedEvent.clear();

		Parent = NULL;
	}

	bool Control::Create(const Vector4i& rect, Control* parent)
	{
		OriginalRect = rect;
		Rect = rect;
		Parent = parent;
		AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Left, Anchor::Top);
		m_zorder = ++m_zorder_max;

		return true;
	}

	void Control::Activate(int dir)
	{
		ActivatingEvent(this);
		ActivatedEvent(this, dir);

		for(auto i = m_children.begin(); i != m_children.end(); i++)
		{
			(*i)->Activate(dir);
		}

		Activated = true;
	}

	void Control::Deactivate(int dir)
	{
		for(auto i = m_children.begin(); i != m_children.end(); i++)
		{
			(*i)->Deactivate(dir);
		}

		if(Captured)
		{
			Captured = false;
		}

		DeactivatingEvent(this);
		DeactivatedEvent(this, dir);

		Activated = false;
	}

	void Control::Close()
	{
		if(!GetCapture())
		{
			ClosedEvent(this);
		}
	}

	void Control::Paint(DeviceContext* dc)
	{
		ASSERT(Parent == NULL); // must be the root

		bool hidden = !Visible || Alpha < 1.0f / 255;

		auto cmp = [] (const Control* a, const Control* b) -> bool {return a->m_zorder < b->m_zorder;};

		std::list<Control*> controls;

		GetDescendants(controls, true, false, cmp);

		for(auto i = controls.begin(); i != controls.end(); i++)
		{
			Control* c = *i;

			c->PaintBeginEvent(c, dc);

			if(!hidden)
			{
				dc->TextAlign = c->TextAlign;
				dc->TextColor = c->TextColor;
				dc->TextFace = c->TextFace;
				dc->TextHeight = c->TextHeight;
				dc->TextStyle = c->TextStyle;
			
				c->PaintBackgroundEvent(c, dc);
				c->PaintForegroundEvent(c, dc);
			}

			c->PaintEndEvent(c, dc);
		}
	}

	void Control::GetSiblings(std::list<Control*>& controls)
	{
		controls.clear();

		Control* parent = Parent;

		if(parent != NULL)
		{
			for(auto i = parent->m_children.begin(); i != parent->m_children.end(); i++)
			{
				Control* c = *i;
				
				if(c != this)
				{
					controls.push_back(c);
				}
			}
		}
	}

	void Control::GetDescendants(std::list<Control*>& controls, bool visibleOnly, bool enabledOnly, SortFunct sort)
	{
		if(visibleOnly && !Visible || enabledOnly && !Enabled) 
		{
			return;
		}

		bool empty = controls.empty();

		controls.push_back(this);

		for(auto i = m_children.begin(); i != m_children.end(); i++)
		{
			(*i)->GetDescendants(controls, visibleOnly, enabledOnly);
		}

		if(empty && sort)
		{
			std::vector<Control*> tmp(controls.begin(), controls.end());

			std::sort(tmp.begin(), tmp.end(), sort);

			controls.assign(tmp.begin(), tmp.end());
		}
	}

	Control* Control::GetCheckedByGroup(int group)
	{
		for(auto i = m_children.begin(); i != m_children.end(); i++)
		{
			Control* c = *i;

			if(c->Group == group && c->Checked)
			{
				return c;
			}
		}

		return NULL;
	}

	void Control::FindFocus(std::list<Control*>& controls)
	{
		for(auto i = m_children.begin(); i != m_children.end(); i++)
		{
			Control* c = *i;

			if(c->CanFocus && c->Visible && c->Enabled)
			{
				controls.push_back(c);
			}

			c->FindFocus(controls);
		}
	}

	Control* Control::GetRoot()
	{
		Control* c = this;

		while(c->Parent)
		{
			c = c->Parent;
		}

		return c;
	}

	Control* Control::GetFocus()
	{
		if(m_focused)
		{
			return m_focused->GetFocus();
		}

		Control* c = Parent;
		
		if(c == NULL || c->m_focused == this)
		{
			return this;
		}

		return NULL;
	}

	Control* Control::GetCapture()
	{
		Control* c = GetRoot();

		return !c->m_captured.empty() ? c->m_captured.back() : NULL;
	}

	Vector2i Control::C2W(const Vector2i& p)
	{
		Vector4i v(p);

		Control* c = this;

		while(c != NULL)
		{
			v += c->Rect;

			c = c->Parent;
		}

		return v.tl;
	}

	Vector2i Control::W2C(const Vector2i& p)
	{
		Vector4i v(p);

		Control* c = this;

		while(c != NULL)
		{
			v -= c->Rect;

			c = c->Parent;
		}

		return v.tl;
	}

	Vector4i Control::C2W(const Vector4i& r)
	{
		Vector4i v;

		v.tl = C2W(r.tl);
		v.br = C2W(r.br);

		return v;
	}

	Vector4i Control::W2C(const Vector4i& r)
	{
		Vector4i v;

		v.tl = W2C(r.tl);
		v.br = W2C(r.br);

		return v;
	}

	Vector2i Control::Map(Control* to, const Vector2i& p)
	{
		return to->W2C(C2W(p));
	}

	Vector4i Control::Map(Control* to, const Vector4i& r)
	{
		return to->W2C(C2W(r));
	}

	Vector4i Control::MapClientRect(Control* control)
	{
		return control->Map(this, control->ClientRect);
	}

	void Control::Move(const Vector4i& rect, bool anchor)
	{
		Vector4i r = Rect;

		Rect = rect;

		if(anchor)
		{
			AnchorRect = AnchorRect;
		}

		if(!r.eq(rect))
		{
			for(auto i = m_children.begin(); i != m_children.end(); i++)
			{
				Control* c = *i;

				int side[4];

				for(int i = 0; i < 4; i++)
				{
					double percent = 0;
					int margin = 0;

					switch(c->m_anchor[i].t)
					{
					case Anchor::Percent:
						if(c->m_anchor[i].s > 0) percent = (float)(c->m_anchor[i].p - 0) / c->m_anchor[i].s;
						margin = 0;
						break;
					case Anchor::Center:
						if(c->m_anchor[i].s > 0) percent = (float)(c->m_anchor[i].c - 0) / c->m_anchor[i].s;
						margin = c->m_anchor[i].p - c->m_anchor[i].c;
						break;
					case Anchor::Middle:
						margin = c->m_anchor[i].p - c->m_anchor[i].s / 2;
						percent = 0.5;
						break;
					case Anchor::Left: 
						percent = 0;
						margin = c->m_anchor[i].p - 0;
						break;
					case Anchor::Right:
						percent = 1;
						margin = c->m_anchor[i].p - c->m_anchor[i].s;
						break;
					default:
						ASSERT(0);
						break;
					}

					side[i] = (int)(percent * (!(i & 1) ? Width : Height) + margin);
				}

				c->Move(Vector4i(side[0], side[1], side[2], side[3]), false);
			}
		}
	}

	void Control::ToFront()
	{
		if(Control* parent = Parent)
		{
			parent->m_children.remove(this);
			parent->m_children.push_back(this);
		}

		std::list<Control*> controls;

		GetDescendants(controls, false, false);

		for(auto i = controls.begin(); i != controls.end(); i++)
		{
			(*i)->m_zorder = ++m_zorder_max;
		}
	}

	void Control::ToBack()
	{
		if(Control* parent = Parent)
		{
			parent->m_children.remove(this);
			parent->m_children.push_front(this);
		}

		m_zorder = --m_zorder_min;
	}

	void Control::DrawScrollText(DeviceContext* dc, LPCWSTR str, const Vector4i& r)
	{
		Vector2i offset(0, 0);

		if(Focused || ScrollNoFocus)
		{
			Vector4i bbox;

			TextEntry* te = dc->Draw(str, Vector4i(0, 0, INT_MAX, r.height()), true);

			if(te == NULL)
			{
				return;
			}

			int w1 = te->bbox.width();
			int w2 = r.width();

			if(w1 > w2)
			{
				int dx = w1 - w2 + 5;

				if(m_scroll == NULL)
				{
					m_scroll = new Animation(true);
				}

				if(m_scroll->GetCount() == 0)
				{
					m_scroll->Set(0, 0, 5000);
					m_scroll->Add(1, dx * 40);
					m_scroll->Add(1, 1000);
					m_scroll->Add(0, dx * 40);
				}
				else
				{
					m_scroll->SetSegmentDuration(1, dx * 40);
					m_scroll->SetSegmentDuration(3, dx * 40);
				}

				offset.x = (int)(*m_scroll * dx + 0.5f);
			}
		}

		dc->Draw(str, r, offset);
	}

	void Control::ResetScrollText()
	{
		if(m_scroll != NULL)
		{
			delete m_scroll;

			m_scroll = NULL;
		}
	}

	// properties

	void Control::GetVisible(bool& value)
	{
		Control* parent = Parent;

		value = (m_flags & ControlFlag::Visible) != 0 && (parent == NULL || parent->Visible);
	}

	void Control::SetVisible(bool& value)
	{
		if(value)
		{
			m_flags |= ControlFlag::Visible;
		}
		else
		{
			m_flags &= ~ControlFlag::Visible;
		}

		if(FocusedPath)
		{
			GetRoot()->GetFocus()->Focused = false;
		}
	}

	void Control::GetEnabled(bool& value)
	{
		Control* parent = Parent;

		value = (m_flags & ControlFlag::Enabled) != 0 && (parent == NULL  || parent->Enabled);
	}

	void Control::SetEnabled(bool& value)
	{
		if(value)
		{
			m_flags |= ControlFlag::Enabled;
		}
		else
		{
			m_flags &= ~ControlFlag::Enabled;
		}
	}

	void Control::GetFocused(bool& value)
	{
		bool b = value;

		GetFocusedPath(value);

		value = m_focused == NULL && b;
	}

	void Control::SetFocused(bool& value)
	{
		Control* focus = GetRoot()->GetFocus();
		Control* parent = Parent;

		if(value)
		{
			Control* captured = GetCapture();

			if(captured != NULL)
			{
				Control* c = this;

				while(c != NULL)
				{
					if(c == captured)
					{
						break;
					}

					c = c->Parent;
				}

				if(c != captured)
				{
					value = false;

					printf("WARNING: SetFocus failed, another control is captured currently\n");

					return;
				}
			}

			if(!CanFocus)
			{
				printf("WARNING: Control cannot be focused\n");

				std::list<Control*> l;

				FindFocus(l);

				if(!l.empty()) 
				{
					l.front()->Focused = true;
				}
				else
				{
					// ASSERT(0);

					printf("WARNING: There is no other control to be auto-focused\n");
				}

				value = false;

				return;
			}

			if(focus != this)
			{
				focus->Focused = false;

				Control* child = this;

				while(parent)
				{
					parent->m_focused = child;
					child = parent;
					parent = parent->Parent;
				}

				m_focused = NULL;

				FocusSetEvent(this); // TODO
			}
		}
		else
		{
			if(focus == this)
			{
				while(parent)
				{
					parent->m_focused = NULL;
					parent = parent->Parent;
				}

				FocusLostEvent(this); // TODO
			}
		}
	}

	void Control::GetFocusedPath(bool& value)
	{
		Control* parent = Parent;
		Control* child = this;

		while(parent)
		{
			if(parent->m_focused != child)
			{
				value = false;

				return;
			}

			child = parent;
			parent = parent->Parent;
		}

		value = true;
	}

	void Control::GetCaptured(bool& value)
	{
		Control* c = GetRoot();

		value = !c->m_captured.empty() && c->m_captured.back() == this;
	}

	void Control::SetCaptured(bool& value)
	{
		Control* root = GetRoot();
		Control* captured = NULL;

		if(!root->m_captured.empty())
		{
			captured = root->m_captured.back();
		}

		if(value && captured != this)
		{
			root->m_captured.push_back(this);
		}
		else if(!value && captured == this) 
		{
			root->m_captured.pop_back();
		}
		else 
		{
			ASSERT(0);
		}
	}

	void Control::SetAlpha(float& value)
	{
		value = std::min<float>(std::max<float>(value, 0.0f), 1.0f);
	}

	void Control::GetLeft(int& value)
	{
		value = Rect->left;
	}

	void Control::SetLeft(int& value)
	{
		Vector4i r = Rect;
		r.left = value;
		Rect = r;
	}

	void Control::GetTop(int& value)
	{
		value = Rect->top;
	}

	void Control::SetTop(int& value)
	{
		Vector4i r = Rect;
		r.top = value;
		Rect = r;
	}

	void Control::GetRight(int& value)
	{
		value = Rect->right;
	}

	void Control::SetRight(int& value)
	{
		Vector4i r = Rect;
		r.right = value;
		Rect = r;
	}

	void Control::GetBottom(int& value)
	{
		value = Rect->bottom;
	}

	void Control::SetBottom(int& value)
	{
		Vector4i r = Rect;
		r.bottom = value;
		Rect = r;
	}

	void Control::GetWidth(int& value)
	{
		value = Rect->width();
	}

	void Control::SetWidth(int& value)
	{
		Vector4i r = Rect;
		r.right = r.left + value;
		Rect = r;
	}

	void Control::GetHeight(int& value)
	{
		value = Rect->height();
	}

	void Control::SetHeight(int& value)
	{
		Vector4i r = Rect;
		r.bottom = r.top + value;
		Rect = r;
	}

	void Control::GetSize(Vector2i& value)
	{
		value = Rect->rsize().br;
	}

	void Control::SetSize(Vector2i& value)
	{
		Vector4i r = Rect;
		r.right = r.left + value.x;
		r.bottom = r.top + value.y;
		Rect = r;
	}

	void Control::GetClientRect(Vector4i& value)
	{
		value = Rect->rsize();
	}

	void Control::GetWindowRect(Vector4i& value)
	{
		value = C2W(Rect->rsize());
	}

	void Control::GetTextHeight(int& value)
	{
		if(value == 0)
		{
			value = Height;
		}
	}

	void Control::GetTextBold(bool& value)
	{
		value = !!(TextStyle & TextStyles::Bold);
	}

	void Control::SetTextBold(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Bold : TextStyle & ~TextStyles::Bold;
	}

	void Control::GetTextItalic(bool& value)
	{
		value = !!(TextStyle & TextStyles::Italic);
	}

	void Control::SetTextItalic(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Italic : TextStyle & ~TextStyles::Italic;
	}

	void Control::GetTextUnderline(bool& value)
	{
		value = !!(TextStyle & TextStyles::Underline);
	}

	void Control::SetTextUnderline(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Underline : TextStyle & ~TextStyles::Underline;
	}

	void Control::GetTextOutline(bool& value)
	{
		value = !!(TextStyle & TextStyles::Outline);
	}

	void Control::SetTextOutline(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Outline : TextStyle & ~TextStyles::Outline;
	}

	void Control::GetTextShadow(bool& value)
	{
		value = !!(TextStyle & TextStyles::Shadow);
	}

	void Control::SetTextShadow(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Shadow : TextStyle & ~TextStyles::Shadow;
	}

	void Control::GetTextEscaped(bool& value)
	{
		value = !!(TextStyle & TextStyles::Escaped);
	}

	void Control::SetTextEscaped(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Escaped : TextStyle & ~TextStyles::Escaped;
	}

	void Control::GetTextWrap(bool& value)
	{
		value = !!(TextStyle & TextStyles::Wrap);
	}

	void Control::SetTextWrap(bool& value)
	{
		TextStyle = value ? TextStyle | TextStyles::Wrap : TextStyle & ~TextStyles::Wrap;
	}

	void Control::SetTextMarginLeft(int& value)
	{
		Vector4i r = TextMargin;
		r.left = value;
		TextMargin = r;
	}

	void Control::SetTextMarginTop(int& value)
	{
		Vector4i r = TextMargin;
		r.top = value;
		TextMargin = r;
	}

	void Control::SetTextMarginRight(int& value)
	{
		Vector4i r = TextMargin;
		r.right = value;
		TextMargin = r;
	}

	void Control::SetTextMarginBottom(int& value)
	{
		Vector4i r = TextMargin;
		r.bottom = value;
		TextMargin = r;
	}

	void Control::SetBackgroundMarginLeft(int& value)
	{
		Vector4i r = BackgroundMargin;
		r.left = value;
		BackgroundMargin = r;
	}

	void Control::SetBackgroundMarginTop(int& value)
	{
		Vector4i r = BackgroundMargin;
		r.top = value;
		BackgroundMargin = r;
	}

	void Control::SetBackgroundMarginRight(int& value)
	{
		Vector4i r = BackgroundMargin;
		r.right = value;
		BackgroundMargin = r;
	}

	void Control::SetBackgroundMarginBottom(int& value)
	{
		Vector4i r = BackgroundMargin;
		r.bottom = value;
		BackgroundMargin = r;
	}

	void Control::SetHoverMarginLeft(int& value)
	{
		Vector4i r = HoverMargin;
		r.left = value;
		HoverMargin = r;
	}

	void Control::SetHoverMarginTop(int& value)
	{
		Vector4i r = HoverMargin;
		r.top = value;
		HoverMargin = r;
	}

	void Control::SetHoverMarginRight(int& value)
	{
		Vector4i r = HoverMargin;
		r.right = value;
		HoverMargin = r;
	}

	void Control::SetHoverMarginBottom(int& value)
	{
		Vector4i r = HoverMargin;
		r.bottom = value;
		HoverMargin = r;
	}

	void Control::SetHoverMarginAll(int& value)
	{
		HoverMargin = Vector4i(value).xxxx();
	}

	void Control::GetHoverRect(Vector4i& value)
	{
		value = ClientRect->deflate(HoverMargin);
	}

	void Control::SetAnchorRect(Vector4i& value)
	{
		Control* parent = Parent;

		if(parent != NULL)
		{
			Vector4i r = Rect;
			Vector2i s = parent->Size;

			m_anchor[0].t = value.left;
			m_anchor[0].p = r.left;
			m_anchor[0].c = (r.x + r.z) / 2;
			m_anchor[0].s = s.x;

			m_anchor[1].t = value.top;
			m_anchor[1].p = r.top;
			m_anchor[1].c = (r.y + r.w) / 2;
			m_anchor[1].s = s.y;

			m_anchor[2].t = value.right;
			m_anchor[2].p = r.right;
			m_anchor[2].c = m_anchor[0].c;
			m_anchor[2].s = s.x;

			m_anchor[3].t = value.bottom;
			m_anchor[3].p = r.bottom;
			m_anchor[3].c = m_anchor[1].c;
			m_anchor[3].s = s.y;
		}
	}

	// event handlers

	bool Control::OnParentChanging(Control* c, Control* value)
	{
		Control* parent = Parent;

		if(parent != value)
		{
			if(parent)
			{
				parent->m_children.remove(this);

				if(parent->m_focused == this)
				{
					GetRoot()->GetFocus()->Focused = false;
				}
			}
		}

		return true;
	}

	bool Control::OnParentChanged(Control* c, Control* value)
	{
		if(value && std::find(value->m_children.begin(), value->m_children.end(), value) == value->m_children.end())
		{
			value->m_children.push_back(this);
		}

		return true;
	}

	bool Control::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		if(BackgroundColor & 0xff000000)
		{
			dc->Draw(WindowRect, BackgroundColor);
		}

		Vector4i src, dst;
		
		Texture* t = GetBackground(dc, src, dst);

		if(t != NULL)
		{
			switch(BackgroundStyle)
			{
			case BackgroundStyleType::Fit:
				dst = dc->FitRect(src, dst.rsize()); 
				break;
			case BackgroundStyleType::FitOutside:
				dst = dc->FitRect(src, dst.rsize(), false); 
				break;
			case BackgroundStyleType::Center: 
				dst = dc->CenterRect(src, dst.rsize()); 
				break;
			case BackgroundStyleType::TopLeft: 
				dst = dc->FitRect(src, dst.rsize()).rsize(); 
				break;
			}

			Vector4i margin = BackgroundMargin;

			if((margin > Vector4i::zero()).anytrue())
			{
				if(BackgroundMarginInflate)
				{
					dst = dst.inflate(margin);
				}

				dc->Draw9x9(t, C2W(dst), margin, Alpha);
			}
			else
			{
				dc->Draw(t, C2W(dst), src, Alpha);
			}
		}

		return true;
	}
	
	bool Control::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(!Text->empty())
		{
			Vector4i r = WindowRect->deflate(TextMargin);

			if(Scroll || ScrollNoFocus) 
			{
				DrawScrollText(dc, Text->c_str(), r);
			}
			else 
			{
				dc->Draw(Text->c_str(), r);
			}
		}

		return true;
	}

	// 

	bool Control::Message(const KeyEventParam& p)
	{
		if(!Visible || !Enabled)
		{
			return false;
		}

		if(m_focused && m_focused->Message(p))
		{
			return true;
		}

		bool handled = false;

		switch(p.cmd.remote)
		{
		case RemoteKey::Red: handled = RedEvent(this); break;
		case RemoteKey::Green: handled = GreenEvent(this); break;
		case RemoteKey::Yellow: handled = YellowEvent(this); break;
		case RemoteKey::Blue: handled = BlueEvent(this); break;
		default: handled = KeyEvent(this, p); break;
		}

		if(handled)
		{
			return true;
		}

		if(!Parent)
		{
			Control* f = GetFocus();

			int dir = 0;

			if(p.cmd.key == VK_TAB && p.mods == KeyEventParam::Shift
			|| p.cmd.key == VK_UP && p.mods == 0 && (f == NULL || f->TabTop)
			|| p.cmd.key == VK_LEFT && p.mods == 0 && (f == NULL || f->TabLeft))
			{
				dir = -1;
			}
			else if(p.cmd.key == VK_TAB && p.mods == 0
			|| p.cmd.key == VK_RIGHT && p.mods == 0 && (f == NULL || f->TabRight)
			|| p.cmd.key == VK_DOWN && p.mods == 0 && (f == NULL || f->TabBottom))
			{
				dir = +1;
			}

			if(dir != 0)
			{
				Control* capture = GetCapture();

				std::list<Control*> l;

				if(capture != NULL)
				{
					capture->FindFocus(l);
				}
				else 
				{
					FindFocus(l);
				}

				if(!l.empty())
				{
					if(dir < 0)
					{
						l.reverse();
					}

					auto i = l.begin();
					auto j = l.end();

					while(i != j && !(*i)->Focused)
					{
						i++;
					}

					if(i != j)
					{
						i++;
					}

					if(i == j)
					{
						i = l.begin();
					}

					(*i)->Focused = true;

					return true;
				}
			}
		}

		if(p.cmd.key == VK_RETURN && p.mods == 0)
		{
			Control* c = Default;

			if(c != NULL && c->Message(p))
			{
				return true;
			}
		}

		return false;
	}

	bool Control::Message(const MouseEventParam& p)
	{
		if(!Visible || !Enabled)
		{
			return false;
		}

		auto cmp = [] (const Control* a, const Control* b) -> bool {return a->m_zorder < b->m_zorder;};

		std::list<Control*> controls;

		GetDescendants(controls, true, true, cmp);

		if(p.cmd == MouseEventParam::Move)
		{
			for(auto i = controls.begin(); i != controls.end(); i++)
			{
				Control* c = *i;

				c->Hovering = c->C2W(c->HoverRect).contains(p.point);
			}
		}

		Control* captured = GetCapture();

		if(captured)
		{
			controls.clear();

			captured->GetDescendants(controls, true, true, cmp);
		}

		for(auto i = controls.rbegin(); i != controls.rend(); i++)
		{
			Control* c = *i;

			if(c->Hovering)
			{
				if(c->CanFocus == 1 && p.cmd == MouseEventParam::Move 
				|| c->CanFocus == 2 && p.cmd == MouseEventParam::Down)
				{
					c->Focused = true;

					break;
				}
			}
		}

		Control* c = NULL;

		if(p.cmd == MouseEventParam::Wheel)
		{
			c = GetFocus();
		}
		else
		{
			for(auto i = controls.rbegin(); i != controls.rend(); i++)
			{
				c = *i;
				
				if(c->Hovering)
				{
					break;
				}

				c = NULL;
			}

			if(c == NULL) 
			{
				c = captured;
			}
		}

		while(c != NULL)
		{
			MouseEventParam p2 = p;

			p2.point = c->W2C(p2.point);
			
			if(c->MouseEvent(c, p2)) 
			{
				return true;
			}

			if(c == captured)
			{
				break;
			}

			c = c->Parent;
		}

		return false;
	}

	Texture* Control::GetBackground(DeviceContext* dc, Vector4i& src, Vector4i& dst)
	{
		std::wstring id = 
			!Enabled && !DisabledImage->empty() ? DisabledImage :
			Hovering && !HoverImage->empty() ? HoverImage :
			Focused && !FocusedImage->empty() ? FocusedImage :
			BackgroundImage;

		if(!id.empty())
		{
			Texture* t = dc->GetTexture(id.c_str());

			if(t != NULL)
			{
				src = Vector4i(Vector2i(0, 0), t->GetSize());
				dst = ClientRect;

				return t;
			}
		}

		return NULL;
	}

	void Control::SetTextProperties(DeviceContext* dc)
	{
		dc->TextAlign = TextAlign;
		dc->TextColor = TextColor;
		dc->TextFace = TextFace;
		dc->TextHeight = TextHeight;
		dc->TextStyle = TextStyle;
	}

	static Localizer s_localizer;

	void Control::SetLanguage(LPCWSTR lang)
	{
		s_localizer.SetLanguage(lang);
	}

	void Control::SetString(LPCSTR id, LPCWSTR str, LPCWSTR lang)
	{
		s_localizer.SetString(id, str, lang);
	}

	LPCWSTR Control::GetString(LPCSTR id, LPCWSTR lang)
	{
		return s_localizer.GetString(id, lang);
	}
}