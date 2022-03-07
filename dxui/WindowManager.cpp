#include "stdafx.h"
#include "WindowManager.h"
#include <math.h>

namespace DXUI
{
	WindowManager::WindowManager() 
		: m_control(NULL)
		, m_rt(NULL)
		, m_rt_old(NULL)
		, m_rt_painted(false)
		, m_rt_changed(false)
	{
		m_control_rect = Vector4i::zero();

		m_anim.Set(0, 1, 750);
		m_anim_dim.Set(0.3f, 1, 500);
		m_anim_dim.Add(1, 60 * 1000);
		m_anim_dim.Add(0.3f, 500);

		Control::DC.ChangedEvent.Add(&WindowManager::OnDeviceContextChanged, this);
	}

	void WindowManager::ResetDim()
	{
		int i = m_anim_dim.GetSegment();
		
		m_anim_dim.SetSegment(i < 2 ? 1 : 0);
	}

	Control* WindowManager::GetControl()
	{
		return m_control;
	}

	void WindowManager::SetControl(Control* control)
	{
		if(m_control != control)
		{
			if(m_control)
			{
				if(m_rt_painted)
				{
					m_rt_rect = m_control_rect;
					m_rt_alpha = m_control->Alpha;
					m_rt_changed = true;
				}
			}

			m_control = control;
		}

		m_control_rect = Vector4i::zero();
		m_rt_painted = false;

		ResetDim();
	}

	void WindowManager::Open(Control* c, bool reset)
	{
		ASSERT(c);

		CAutoLock cAutoLock(&Control::m_lock);

		auto j = std::find(m_stack.begin(), m_stack.end(), c);

		if(reset)
		{
			if(j != m_stack.end())
			{
				m_stack.erase(j);
			}

			for(auto i = m_stack.rbegin(); i != m_stack.rend(); i++)
			{
				(*i)->Deactivate(-1);
			}

			m_stack.clear();

			m_stack.push_back(c);

			if(m_control != c)
			{
				SetControl(c);

				c->Activate(+1);
			}
		}
		else
		{
			if(j != m_stack.end())
			{
				while(c != m_stack.back())
				{
					m_stack.back()->Deactivate(-1);

					m_stack.pop_back();
				}

				if(m_control != c)
				{
					SetControl(c);

					c->Activate(-1);
				}
			}
			else
			{
				if(!m_stack.empty())
				{
					m_stack.back()->Deactivate(+1);
				}

				m_stack.push_back(c);

				SetControl(c);

				c->Activate(+1);
			}
		}
	}

	void WindowManager::Close()
	{
		CAutoLock cAutoLock(&Control::m_lock);
		
		if(m_stack.size() > 1)
		{
			Control* c = m_stack.back();

			c->Deactivate(-1);

			m_stack.pop_back();

			c = m_stack.back();

			SetControl(c);

			c->Activate(-1);
		}
	}

	bool WindowManager::Save(LPCWSTR fn)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		if(m_rt_painted && m_rt != NULL)
		{
			return m_rt->Save(fn);
		}

		return false;
	}

	bool WindowManager::OnInput(const KeyEventParam& p)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		ResetDim();

		return m_control ? m_control->Message(p) : false;
	}

	bool WindowManager::OnInput(const MouseEventParam& p)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		ResetDim();

		if(m_control)
		{
			MouseEventParam tmp = p;

			Vector4i r = m_control_rect;

			if(!r.rempty())
			{
				tmp.point.x = (p.point.x - r.left) * m_control->Width / r.width();
				tmp.point.y = (p.point.y - r.top) * m_control->Height / r.height();
			}

			return m_control->Message(tmp);
		}
	
		return false;
	}

	bool WindowManager::IsOpen(Control* c) 
	{
		return std::find(m_stack.begin(), m_stack.end(), c) != m_stack.end();
	}

	void WindowManager::OnPaint(const Vector4i& r, DeviceContext* dc, bool locked)
	{
		if(m_control == NULL || dc == NULL)
		{
			return;
		}

		Vector4i r1 = m_control->OriginalRect;
		Vector4i r2 = DeviceContext::FitRect(r, r1, false).rsize();

		if(r2.height() == r1.height() && r2.width() > r2.height() * 16 / 9)
		{
			r2.right = r1.left + r2.height() * 16 / 9;
		}

		if(r2.width() == r1.width() && r2.height() > r2.width() * 4 / 5)
		{
			r2.bottom = r1.top + r2.width() * 4 / 5;
		}

		if(r2.width() > 2048) 
		{
			r2.right = r2.left + 2048;
		}

		if(r2.height() > 2048) 
		{
			r2.bottom = r2.top + 2048;
		}

		m_control->Move(r2, false);

		m_control_rect = DeviceContext::FitRect(r2, r);

		float alpha = (float)m_control->Alpha;
		
		if(!m_control->Transparent)
		{
			alpha *= (float)m_anim_dim;
		}

		Device* dev = dc->GetDevice();

		if(m_rt_changed)
		{
			if(!locked) 
			{
				dc->Draw(m_rt, m_control_rect, m_rt_alpha); 
				
				return;
			}

			Vector2i size = m_rt->GetSize();

			delete m_rt_old;

			m_rt_old = dev->CreateRenderTarget(size.x, size.y, false);

			dc->SetTarget(m_rt_old);
			dc->Draw(m_rt, Vector4i(0, 0, size.x, size.y)); 
			dc->SetTarget(NULL);

			m_rt_changed = false;

			m_anim.SetSegment(0);
		}

		Vector2i r2size = r2.rsize().br;

		if(m_rt == NULL || m_rt->GetSize() != r2size)
		{
			delete m_rt;

			m_rt = dev->CreateRenderTarget(r2size.x, r2size.y, false);

			if(m_rt == NULL) return;

			dev->ClearRenderTarget(m_rt, 0);
		}

		if(locked)
		{
			dev->ClearRenderTarget(m_rt, 0);

			dc->SetTarget(m_rt);

			m_control->Paint(dc);

			dc->SetTarget(NULL);
		}

		//

		float f = m_anim;

		if(f >= 1.0f)
		{
			delete m_rt_old;

			m_rt_old = NULL;
		}

		if(m_rt_old != NULL)
		{
			Texture* rt;
			float a;
			Vector4i dst;
			Vector4 src;
			float fi;

			if(f < 0.5f)
			{
				rt = m_rt_old;
				a = m_rt_alpha;
				dst = m_rt_rect;
				src = Vector4(0, 0, 1, 1);
				fi = 1.0f - f;
			}
			else
			{
				rt = m_rt;
				a = alpha;
				dst = m_control_rect;
				src = Vector4(1, 0, 0, 1);
				fi = f;
			}

			Vector4 r(dst);
			Vector4 c = (r + r.zwxy()) / 2;

			r -= c;

			Vector4 p(0.0f, 1.0f);
			Vector2i color(0xffffff | (std::max<DWORD>(std::min<DWORD>((DWORD)(a * 0xff), 0xff), 0) << 24), 0);

			Vertex v[6] =
			{
				{r.xyxy(p), Vector2(src.x, src.y), color},
				{r.zyxy(p), Vector2(src.z, src.y), color},
				{r.xwxy(p), Vector2(src.x, src.w), color},
				{r.zwxy(p), Vector2(src.z, src.w), color},
			};

			float ca = (float)cos(f * M_PI);
			float sa = (float)sin(f * M_PI);

			for(int i = 0; i < 4; i++)
			{
				float x = ca * v[i].p.x * fi - sa * v[i].p.z * fi;
				float y = v[i].p.y * fi;
				float z = sa * v[i].p.x * fi + ca * v[i].p.z * fi;

				v[i].p.z = z / (r.right - r.left) + 0.5f;
				v[i].p.w = 1.0f / v[i].p.z;
				v[i].p.x = x / (v[i].p.z * 2) + c.x;
				v[i].p.y = y / (v[i].p.z * 2) + c.y;
			}

			v[4] = v[1];
			v[5] = v[2];

			dc->DrawTriangleList(v, 2, rt);
		}
		else
		{
			dc->Draw(m_rt, m_control_rect, alpha); 
		}

		m_rt_painted = true;
	}

	bool WindowManager::OnDeviceContextChanged(Control* c, DeviceContext* dc)
	{
		delete m_rt;

		m_rt = NULL;

		delete m_rt_old;

		m_rt_old = NULL;
		
		m_rt_painted = false;
		m_rt_changed = false;
		
		m_control_rect = Vector4i::zero();

		return true;
	}
}