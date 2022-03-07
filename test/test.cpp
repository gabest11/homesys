// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <atlbase.h>
#include <time.h>
#include <conio.h>
#include <map>
#include <string>
#include <evr.h>
#include "../DirectShow/FGManager.h"
#include "../DirectShow/Filters.h"
#include "../util/String.h"
#include "../util/Window.h"

using namespace std;
using namespace Util;

void DoModalLoop()
{
	MSG msg;

	BOOL bRet;

	while((bRet = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) != 0)
	{
		if(bRet != -1)
	    {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
	    }
	    else
	    {
			break;
	    } 
	}
}

void Play(LPCWSTR path)
{
	HRESULT hr;

	CComPtr<IGraphBuilder2> pGB = new CFGManagerCustom(_T("CFGManager"), NULL);

	CComPtr<IBaseFilter> pBF;

	hr = pGB->AddSourceFilter(path, L"Source", &pBF);

	hr = pGB->ConnectFilter(pBF, NULL);

	pGB->Dump();

	if(SUCCEEDED(hr))
	{
		//Sleep(1000);

		CComQIPtr<IMediaControl> pMC = pGB;
		CComQIPtr<IMediaSeeking> pMS = pGB;

		hr = pMC->Run();

		for(bool exit = false; !exit; )
		{
			REFERENCE_TIME cur, dur, start, stop;

			pMS->GetCurrentPosition(&cur);
			pMS->GetDuration(&dur);

			if(_kbhit())
			{
				switch(int ch = _getch())
				{
				case 'x': {exit = true; break;}
				case 's': {pMC->Stop(); break;}
				case 'p': {pMC->Pause(); break;}
				case 'r': {pMC->Run(); break;}
				case 'j': {start = max(0, cur - 50000000); stop = dur; pMS->SetPositions(&start, AM_SEEKING_AbsolutePositioning, &stop, AM_SEEKING_NoPositioning); break;}
				case 'l': {start = min(dur, cur + 50000000); stop = dur; pMS->SetPositions(&start, AM_SEEKING_AbsolutePositioning, &stop, AM_SEEKING_NoPositioning); break;}
				default: {if(ch != 0) printf("%x\n", ch); break;}
				}
			}

			// printf("%I64d %I64d\n", cur, dur);

			DoModalLoop();

			Sleep(500);
		}

		hr = pMC->Stop();
/*
		Sleep(3000);

		hr = pMC->Stop();

		Sleep(3000);

		hr = pMC->Run();

		Sleep(6000);
*/
	}
}

#include "../dxui/Device9.h"
#include "../dxui/DeviceContext9.h"
#include "../DirectShow/TextSubProvider.h"

void SubTest()
{
	Util::Window* wnd = new Util::Window();

	wnd->Create(L"Hello World!", 800, 400);

	DXUI::Device9* dev = new DXUI::Device9();

	if(!dev->Create((HWND)wnd->GetHandle(), false, false))
	{
		return;
	}

	DXUI::DeviceContext9* dc = new DXUI::DeviceContext9(dev, NULL);

	CComPtr<ISubPicQueue> queue = new CDX9SubPicQueue(*dev, Vector2i(800, 400), 0);

	CComPtr<ITextSubStream> ss = new CTextSubProvider();

	// if(FAILED(ss->OpenFile(L"../subtitle/doc/demo.ssa")))
	// if(FAILED(ss->OpenFile(L"Y:/uTorrent/test/sub/3.ssa")))
	// if(FAILED(ss->OpenFile(L"c:/Akikan! - 01-001.ass")))
	if(FAILED(ss->OpenFile(L"C:/Users/Gabest/Desktop/[umee]_Shiki_-_09_[B9414DF7].ass")))
	{
		return;
	}

	queue->SetSubProvider(CComQIPtr<ISubProvider>(ss));

	int frame = 0;

	wnd->Show();

	for(bool exit = false; !exit; )
	{
		if(_kbhit())
		{
			switch(int ch = _getch())
			{
			case 'x': {exit = true; break;}
			default: {if(ch != 0) printf("%x\n", ch); break;}
			}
		}

		DoModalLoop();

		bool canReset;

		if(dev->IsLost(canReset))
		{
			Sleep(100);

			if(canReset)
			{
				dev->Reset(DXUI::Device::DontCare);
			}
		}
		else
		{
			Vector4i cr = wnd->GetClientRect();

			DXUI::Texture* bb = dev->GetBackbuffer();

			if(bb != NULL && (bb->GetWidth() != cr.width() || bb->GetHeight() != cr.height()))
			{
				dev->Resize(cr.width(), cr.height());
				
				bb = dev->GetBackbuffer();
			}

			if(bb != NULL)
			{
				dev->ClearRenderTarget(bb, 0xff0000);

				Vector2i s = bb->GetSize();

				queue->SetTarget(s, Vector4i(0, 0, s.x, s.y));

				CComPtr<ISubPic> sp;

				REFERENCE_TIME rt = 400000i64 * frame; // (__int64)clock() * 10000;

				queue->SetTime(rt);

				wnd->SetWindowText(Util::Format(L"%d %I64d", frame, rt / 10000).c_str());

				if(queue->LookupSubPic(rt, &sp))
				{
					CComQIPtr<IDirect3DTexture9> texture = sp;

					if(texture != NULL)
					{
						Vector4i r = cr;
						
						sp->GetRect(r);

						DXUI::Texture9 t(texture);

						t.SetFlag(DXUI::Texture::AlphaComposed);

						// t.Save(L"c:\\1.dds", true);

						dc->Draw(&t, r);
					}
				}
			}

			dev->Flip(false);

			frame++;

			// if(frame >= 4500) break;

			Sleep(10);
		}
	}

	delete wnd;

	delete dev;
}

#include <regex>
#include <string>

using namespace std;

void Regex()
{
	string str = "<b>Hell</b>o<b> World!</b>";
	regex rx("(<b[^>]*>)(.+?)(<\\/b>)", regex::ECMAScript | regex::icase);
    cmatch res; 
	regex_search(str.c_str(), res, rx);
	cout << res[1] << "\n";
}

/*
#include "../DirectShow/VobSubProvider.h"

void Search(LPCWSTR dir)
{
	std::wstring pattern = Util::CombinePath(dir, L"*.*");

	WIN32_FIND_DATA fd;

	HANDLE h = FindFirstFile(pattern.c_str(), &fd);
	
	if(h != INVALID_HANDLE_VALUE)
	{
		do 
		{
			std::wstring path = Util::CombinePath(dir, fd.cFileName);

			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(fd.cFileName[0] != '.')
				{
					Search(path.c_str());
				}
			}
			else
			{
				if(path.find(L".idx") != std::wstring::npos)
				{
					CVobSubProvider* p = new CVobSubProvider();

					std::list<CVobSubProvider::SubStream*> sss;

					if(SUCCEEDED(p->OpenFile(path.c_str())))
					{
						for(int i = 0; i < sizeof(p->m_sss) / sizeof(p->m_sss[0]); i++)
						{
							if(p->m_sss[i].id == 0) break;

							for(auto j = p->m_sss[i].sps.begin(); j != p->m_sss[i].sps.end(); j++)
							{
								CVobSubProvider::SubPic* sp = *j;

								if(sp->forced)
								{
									sss.push_back(&p->m_sss[i]);

									break;
								}
							}
						}
					}

					if(!sss.empty())
					{
						wprintf(L"%s: ", path.c_str());

						for(auto i = sss.begin(); i != sss.end(); i++)
						{
							wprintf(L"%c%c ", (*i)->id >> 8, (*i)->id & 0xff);
						}

						wprintf(L"\n");
					}

					delete p;
				}
			}
		}
		while(FindNextFile(h, &fd));

		FindClose(h);
	}
}
*/

#include "../dxui/PlayerWindow.h"

using namespace DXUI;

class MotionSearchWindow 
	: public PlayerWindow
{
	void CopyImage(DeviceContext* dc, Texture* src, Texture* dst, int side)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		Vector4 param(side ? 0.5f : 0, 0.0f, 0.0f, 0.0f);

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/0.psh"), (float*)&param, 4);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void GrayScale(DeviceContext* dc, Texture* src, Texture* dst)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/1.psh"), NULL, 0);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void Diff(DeviceContext* dc, Texture* src, Texture* dst, int offset)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		Vector4 param(0.5f + (float)offset / src->GetWidth(), 0.0f, 0.0f, 0.0f);

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/2.psh"), (float*)&param, 4);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void Sum(DeviceContext* dc, Texture* src, Texture* dst, int dir)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(dir == 0 ? L"d:/temp/3x.psh" : L"d:/temp/3y.psh"), NULL, 0);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}
	
	void SumMin(DeviceContext* dc, Texture* src, Texture* dst_rt, Texture* dst_ds, float offset, Direct3DDepthStencilState9* dss)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(dss);
		dev->OMSetRenderTargets(dst_rt, dst_ds, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst_rt->GetWidth(), dst_rt->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		Vector4 param(offset, 0.0f, 0.0f, 0.0f);

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/4.psh"), (float*)&param, 4);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void Smooth(DeviceContext* dc, Texture* src, Texture* dst)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/5.psh"), NULL, 0);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}
	
	void Shift(DeviceContext* dc, Texture* src, Texture* src_shift, Texture* dst, int steps)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		Vector4 param((float)steps / src->GetWidth(), 0.0f, 0.0f, 0.0f);

		dev->PSSetSamplerState(&dev->m_convert.ln);
		dev->PSSetShaderResources(src, src_shift);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/6.psh"), (float*)&param, 4);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void Gradient(DeviceContext* dc, Texture* src, Texture* dst)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/of_gradient.psh"), NULL, 0);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void Blur(DeviceContext* dc, Texture* src, Texture* dst)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, NULL);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/of_blur.psh"), NULL, 0);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void Velocity(DeviceContext* dc, Texture* src, Texture* src_prev, Texture* dst)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetRenderTargets(dst, NULL, NULL);

		// ia

		Vector4 s(0, 0, 1, 1);
		Vector4 d(0, 0, dst->GetWidth(), dst->GetHeight());
		Vector4 p(0.5f, 1.0f);
		Vector2i c(0, 0);

		Vertex v[4] =
		{
			{d.xyxy(p), Vector2(s.x, s.y), c},
			{d.zyxy(p), Vector2(s.z, s.y), c},
			{d.xwxy(p), Vector2(s.x, s.w), c},
			{d.zwxy(p), Vector2(s.z, s.w), c},
		};

		dev->IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));

		// ps

		dev->PSSetSamplerState(&dev->m_convert.pt);
		dev->PSSetShaderResources(src, src_prev);
		dev->PSSetShader(*(PixelShader9*)dc->GetPixelShader(L"d:/temp/of_velocity.psh"), NULL, 0);

		//

		dev->DrawPrimitive();

		dev->EndScene();
	}

	void OnPaint(DeviceContext* dc)
	{
		Vector4i r = GetClientRect();

		Texture* t = dc->GetTexture(L"C:/ProgramData/Homesys/5.bmp");

		int w = t->GetWidth();
		int h = t->GetHeight() * 2 / 5;

		Device9* dev = (Device9*)m_dev;

		Texture* rt0 = dev->CreateRenderTarget(w, h, false, D3DFMT_L8);
		Texture* rt1 = dev->CreateRenderTarget(w / 2, h, false, D3DFMT_L8);
		Texture* rt2 = dev->CreateRenderTarget(w / 2, h, false, D3DFMT_L8);
		Texture* rt3 = dev->CreateRenderTarget(w / 2, h, false, D3DFMT_L8);
		Texture* ds3 = dev->CreateDepthStencil(w / 2, h, false, D3DFMT_D24X8);
		Texture* rt4 = dev->CreateRenderTarget(w / 2, h, false);

		Texture* gradient = dev->CreateRenderTarget(w / 2, h, false, D3DFMT_A16B16G16R16F);
		Texture* velocity0 = dev->CreateRenderTarget(w / 2, h, false, D3DFMT_A16B16G16R16F); // D3DFMT_G16R16F);
		Texture* velocity1 = dev->CreateRenderTarget(w / 2, h, false, D3DFMT_A16B16G16R16F); // D3DFMT_G16R16F);

		Direct3DDepthStencilState9 dss;
		
		dss.DepthEnable = true;
		dss.DepthFunc = D3DCMP_LESS;
		dss.DepthWriteMask = true;
		dss.StencilEnable = false;

		dev->OMSetBlendState(&dev->m_convert.bs[0], 0);
		dev->IASetInputLayout(dev->m_convert.il[0]); 
		dev->IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);
		dev->VSSetShader(dev->m_convert.vs[0], NULL, 0);

		CopyImage(dc, t, rt4, 0);

		// 
		rt4->Save(L"d:/temp/v0.bmp");

		CopyImage(dc, t, rt4, 1);

		//
		rt4->Save(L"d:/temp/v2.bmp");

CComPtr<IDirect3DQuery9> query;

(*dev)->CreateQuery(D3DQUERYTYPE_EVENT, &query);

		clock_t start = clock();

		dev->ClearRenderTarget(rt3, 0x80808080);
		dev->ClearDepth(ds3, 1.0f);

		int steps = 64;

		GrayScale(dc, t, rt0);

		// rt0->Save(L"d:/temp/0.bmp");

		dev->ClearRenderTarget(velocity0, 0);

		Gradient(dc, rt0, gradient);

		gradient->Save(L"d:/temp/gradient.bmp");

		for(int i = 0; i < 16; i++)
		{
			if(i > 0) Blur(dc, velocity1, velocity0);

			Velocity(dc, gradient, velocity0, velocity1);

			velocity1->Save(L"d:/temp/velocity.bmp");
		}

		delete gradient;
		delete velocity0;
		delete velocity1;

		printf("1 %d\n", clock() - start);

		for(int j = 0; j <= steps; j++)
		{
			int i = steps / 2;
			int k = j + 1;
			
			if(k & 1) i += k / 2;
			else i -= k / 2;

			Diff(dc, rt0, rt1, i - steps / 2);

			// rt1->Save(Util::Format(L"d:/temp/1_%02d.bmp", j).c_str());

			Sum(dc, rt1, rt2, 0);

			// rt2->Save(Util::Format(L"d:/temp/2_%02d.bmp", j).c_str());

			Sum(dc, rt2, rt1, 1);

			// rt1->Save(Util::Format(L"d:/temp/3_%02d.bmp", j).c_str());

			SumMin(dc, rt1, rt3, ds3, (float)i / steps, &dss);

			// 
			if(j == 64) rt3->Save(Util::Format(L"d:/temp/4_%02d.bmp", j).c_str());
		}
/*
		Smooth(dc, rt3, rt1);
		Smooth(dc, rt1, rt3);
*/
		Smooth(dc, rt3, rt1);
		Smooth(dc, rt1, rt3);

		// 
		rt3->Save(L"d:/temp/5.bmp");

		Shift(dc, t, rt3, rt4, steps);

query->Issue(D3DISSUE_END);
while((S_FALSE == query->GetData(NULL, 0, D3DGETDATA_FLUSH))) Sleep(1);

		rt4->Save(L"d:/temp/v1.bmp");

		printf("2 %d\n", clock() - start);

		delete rt0;
		delete rt1;
		delete rt2;
		delete rt3;
		delete ds3;
		delete rt4;
	}

public:
	MotionSearchWindow(ResourceLoader* loader, DWORD flags)
		: PlayerWindow(L"MotionSearchTest", loader, flags)
	{
	}

	virtual ~MotionSearchWindow()
	{
	}

	class Loader : public ResourceLoader
	{
	public:
		bool GetResource(LPCWSTR id, std::vector<BYTE>& buff, bool priority = false, bool refresh = false) 
		{
			return wcsstr(id, L":") != NULL ? __super::FileGetContents(id, buff) : false;
		}

		void IgnoreResource(LPCWSTR id) {}
	};
};

static void MotionSearchTest()
{
	ResourceLoader* loader = new MotionSearchWindow::Loader();

	MotionSearchWindow* wnd = new MotionSearchWindow(loader, 4);

	wnd->Create(L"MotionSearchTest", 640, 480);

	wnd->Show();

	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	delete wnd;

	delete loader;
}

#include "../3rdparty/libdvbcsa/dvbcsa.h"
#include "../3rdparty/ffdecsa/ffdecsa.h"

void CSATest()
{
	csa_key_t* keys[2];
	dvbcsa_key_s* keys2[2];

	for(int i = 0; i < 2; i++)
	{
		keys[i] = (csa_key_t*)_aligned_malloc(sizeof(csa_key_t), 16);
		keys2[i] = dvbcsa_key_alloc();
	}

	BYTE cws[16];

	for(int i = 0; i < 2; i++)
	{
		set_control_word(cws + i * 8, keys[i]);
		dvbcsa_key_set(cws + i * 8, keys2[i]);
	}

	BYTE* packet = new BYTE[188 * 8 * 2048];

	clock_t start = clock();

	BYTE* p = packet;
/*	
	for(int j = 0, b = 0; j < 2048 / 4; j++, p += 188 * 8 * 4)
	{
		BYTE* cluster[3] = {p, p + 188 * 8 * 4, NULL};

		for(int k = 0; k < 188 * 8 * 4; k += 188)
		{
			p[k + 3] = 0x80;

			for(int i = 4; i < 188; i++, b += 0x71)
			{
				p[k + i] = b;
			}
		}

		decrypt_packets(cluster, keys);
	}
*/
	for(int j = 0, b = 0; j < 2048 * 8; j++, p += 188)
	{
		BYTE* cluster[3] = {p, p + 188, NULL};

		for(int k = 0; k < 188; k += 188)
		{
			p[k + 3] = 0x80;

			for(int i = 4; i < 188; i++, b += 0x71)
			{
				p[k + i] = b;
			}
		}

		//decrypt_packets(cluster, keys);
		dvbcsa_decrypt(keys2[0], p + 4, 184);
	}

	printf("%d\n", clock() - start);

	delete [] packet;

	for(int i = 0; i < 2; i++)
	{
		_aligned_free(keys[i]);
		dvbcsa_key_free(keys2[i]);
	}
}

void SocketTest()
{
	Socket::Startup();

	Socket s;
	 
	if(s.Listen(NULL, 9993))
	{
		while(!_kbhit())
		{
			Socket s2;

			if(s.Accept(s2))
			{
				s2.Write("Hello World\r\n");
				s2.Close();
			}

			Sleep(1);
		}
	}

	Socket::Cleanup();
}

#include "..\3rdparty\anysee\anyseeIR.h"

void AnyseeIR()
{
	CanyseeIR ir;

	DWORD n = ir.GetNumberOfDevices();

	DEVICE_INFO info;

	int ret = ir.Open(1, &info);

	for(bool exit = false; !exit; )
	{
		DWORD key = 0;

		ret = ir.Read(key);

		if(_kbhit())
		{
			switch(int ch = _getch())
			{
			case 'x': {exit = true; break;}
			}
		}

		printf("%d\n", ret);

		Sleep(1000);
	}

	ir.Close(1);
}

#include "../3rdparty/rtmp/amf.h"

void AMFTest()
{
	std::vector<char> buff;

	if(FILE* fp = _wfopen(L"c:\\temp\\1.bin", L"rb"))
	{
		fseek(fp, 0, SEEK_END);
		buff.resize((size_t)_ftelli64(fp));
		fseek(fp, 0, SEEK_SET);
		fread(buff.data(), buff.size(), 1, fp);
		fclose(fp);
	}
	
	AMFObject obj;

	if(obj.Decode(buff.data(), buff.size()) < 0)
	{
		return;
	}

	std::string metastring = obj.GetProperty(0).GetString();

	int duration = 0;

	if(metastring == "onMetaData")
	{
		AMFObjectProperty prop;

		if(obj.FindFirstMatchingProperty("duration", prop))
		{
			duration = prop.GetNumber();
		}
	}

	printf("%d\n", duration);
}

#include "../directshow/SmartCard.h"

void SmartCardTest()
{
	wprintf(L"Waiting for anysee card reader, press any key to exit...\n\n");

	while(!kbhit())
	{
		SmartCardReader* r = new WinSCardReader(); // new AnyseeCardReader();

		r->Create(0);

		r->Connect();

		if(r->HasCard())
		{
			Sleep(1000);

			wprintf(L"%s\n%s\n%s %04x\n\n", 
				r->GetName(), 
				r->GetSerial().c_str(),
				r->GetSystemName().c_str(),
				r->GetSystemId());
		}

		delete r;

		Sleep(1000);
	}
	/*
	SmartCardServer* scs = new SmartCardServer();

	// scs->GetDevices(

	delete scs;
	*/
}

#include "../DirectShow/DSMMuxer.h"
#include "../DirectShow/MatroskaMuxer.h"
#include "../DirectShow/mp4mux/MuxFilter.h"
#include "../DirectShow/Monogram_x264.h"
#include "../DirectShow/Monogram_aac.h"
#include "../DirectShow/MediaTypeEx.h"

[uuid("1FB0F046-623C-40A7-B439-41E4BFCB8BAB")]
class MonogramX264 {};

[uuid("88F36DB6-D898-40B5-B409-466A0EECC26A")]
class MonogramAAC {};

static HRESULT AddSource(std::wstring src, IGraphBuilder2* graph, CComPtr<IBaseFilter>& source, REFERENCE_TIME* dur)
{
	HRESULT hr;

	if(dur != NULL)
	{
		*dur = 0;
	}

	if(src.find(L"://") != std::wstring::npos)
	{
		Socket::Startup();

		Socket s;

		std::string url = std::string(src.begin(), src.end());

		std::map<std::string, std::string> headers;

		std::list<std::string> types;

		types.push_back("video/x-ms-asf");

		std::string xml;

		if(s.HttpGetXML(url.c_str(), headers, types, xml))
		{
			TiXmlDocument doc;

			doc.Parse(xml.c_str(), NULL, TIXML_ENCODING_UTF8);

			if(TiXmlElement* asx = doc.FirstChildElement("asx"))
			{
				if(TiXmlElement* entry = asx->FirstChildElement("entry"))
				{
					for(TiXmlElement* n = entry->FirstChildElement("ref"); 
						n != NULL; 
						n = n->NextSiblingElement("ref"))
					{
						std::wstring href = Util::UTF8To16(n->Attribute("href")) + L"?MSWMExt=.asf";

						hr = graph->AddSourceFilterMime(href.c_str(), L"Source", L"video/x-ms-asf", &source);

						break;
					}
				}
			}
		}
		else if(s.GetContentType() == "video/x-ms-wvx")
		{
			std::string str;

			if(s.Read(str) && str == "[Reference]")
			{
				while(s.Read(str))
				{
					std::vector<std::string> sl;

					Explode(str, sl, '=', 2);

					if(sl.size() == 2 && sl[0].find("Ref") == 0)
					{
						std::wstring href = Util::UTF8To16(sl[1].c_str());

						hr = graph->AddSourceFilterMime(href.c_str(), L"Source", L"video/x-ms-asf", &source);

						break;
					}
				}
			}
		}

		s.Close();

		Socket::Cleanup();
	}

	if(source == NULL)
	{
		if(FAILED(hr = graph->AddSourceFilter(src.c_str(), L"Source", &source))) 
		{
			return hr;
		}
	}

	if(IPin* pin = DirectShow::GetFirstDisconnectedPin(source, PINDIR_OUTPUT))
	{
		bool stream = false;

		Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
		{
			if(pmt->majortype == MEDIATYPE_Stream)
			{
				stream = true;

				return S_OK;
			}

			return S_CONTINUE;
		});

		if(stream)
		{
			hr = graph->ConnectFilter(pin);

			if(FAILED(hr)) return hr;

			source = DirectShow::GetFilter(DirectShow::GetConnected(pin));
		}
	}

	if(dur != NULL)
	{
		if(CComQIPtr<IMediaSeeking> pMS = source)
		{
			REFERENCE_TIME start = 0;
			REFERENCE_TIME stop = 0;

			hr = pMS->GetAvailable(&start, &stop);

			if(SUCCEEDED(hr))
			{
				*dur = stop - start;
			}
		}
		else
		{
			Foreach(source, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
			{
				CComQIPtr<IMediaSeeking> pMS = pin;
				
				if(pMS != NULL)
				{
					REFERENCE_TIME start = 0;
					REFERENCE_TIME stop = 0;

					hr = pMS->GetAvailable(&start, &stop);

					if(SUCCEEDED(hr) && stop > start)
					{
						*dur = stop - start;

						return S_OK;
					}
				}

				return S_CONTINUE;
			});
		}
	}

	return hr;
}

static HRESULT Remux(const std::wstring& src, const std::wstring& dst, IBaseFilter* muxer)
{
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

	HRESULT hr;

	REFERENCE_TIME dur1 = 0;
	REFERENCE_TIME dur2 = 0;
	
	{
		CComPtr<IGraphBuilder2> graph = new CFGManagerCustom(L"CFGManagerCustom", NULL);

		CComPtr<IBaseFilter> source;

		hr = AddSource(src, graph, source, &dur1);

		if(FAILED(hr)) return hr;

		if(dur1 == 0) return E_FAIL;

		printf("%lld\n", dur1);

		if(FAILED(hr = graph->AddFilter(muxer, L"Muxer")))
		{
			return hr;
		}

		int n = 0;
		int m = 0;

		Foreach(source, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
		{
			n++;

			if(SUCCEEDED(graph->ConnectFilter(pin, muxer)))
			{
				m++;
			}
			else
			{
				const CLSID* encids[] = 
				{
					&CLSID_MonogramX264,
					&CLSID_MonogramAACEncoder
				};

				for(size_t i = 0; i < countof(encids); i++)
				{
					CComQIPtr<IBaseFilter> enc;
				
					enc.CoCreateInstance(*encids[i]);

					if(SUCCEEDED(graph->AddFilter(enc, L"Encoder")))
					{
						if(SUCCEEDED(graph->ConnectFilter(pin, enc))
						&& SUCCEEDED(graph->ConnectFilterDirect(enc, muxer, NULL)))
						{
							m++;

							CComQIPtr<IMonogramX264_2> x264_cfg = enc;

							if(x264_cfg != NULL)
							{
								MONOGRAM_X264_CONFIG2 cfg;

								x264_cfg->GetConfig(&cfg);

								MONOGRAM_X264_VIDEOPROPS props;

								x264_cfg->GetVideoProps(&props);

								int w = props.width;
								int h = props.height;
								int mb = ((w + 15) >> 4) * ((h + 15) >> 4);

								if(mb >= 3600)
								{
									cfg.level = MONO_AVC_LEVEL_4_1;
								}
								else
								{
									cfg.level = MONO_AVC_LEVEL_3_1;
								}

								if(h >= 1080)
								{
									cfg.bitrate_kbps = 2048;
								}
								else if(h >= 720)
								{
									cfg.bitrate_kbps = 2048;
								}
								else if(h >= 480)
								{
									cfg.bitrate_kbps = 1024;
								}
								else
								{
									cfg.bitrate_kbps = 768;
								}

								cfg.parallel = true;
								cfg.bframes = 3;
								cfg.adaptive_b_frames = MONO_AVC_BFRAMESADAPTIVE_OPTIMAL;
								cfg.bframes_pyramid = MONO_AVC_BPYRAMID_NORMAL;
								cfg.trellis = MONO_AVC_TRELLIS_ALLMODES;
								cfg.partitions_intra = MONO_AVC_INTRA_HIGH;   
								cfg.partitions_inter = MONO_AVC_INTER_HIGH; 
								cfg.reference_frames = 3;
								cfg.key_int_min = 25;
								cfg.key_int_max = 250;
								//cfg.tolerance_percent = 50;
								//cfg.vbv_maxrate_kbps = cfg.bitrate_kbps;
								/*
								cfg.rcmethod = MONO_AVC_RC_MODE_ABR;
								cfg.qp_min = 0;
								cfg.qp_max = 51;
								cfg.const_qp = 10;
								*/
								cfg.rcmethod = MONO_AVC_RC_MODE_CQP;
								cfg.qp_min = 0;
								cfg.qp_max = 51;
								cfg.const_qp = 27;								
								/*
								cfg.rcmethod = MONO_AVC_RC_MODE_CRF;
								cfg.qp_min = 0;
								cfg.qp_max = 51;
								cfg.const_qp = 22;
								*/
								//cfg.tolerance_percent = 0;

								x264_cfg->SetConfig(&cfg);
							}

							break;
						}
						else
						{
							graph->RemoveFilter(enc);
						}
					}
				}
			}

			return S_CONTINUE;
		});

		printf("pins connected: %d out of %d\n", n, m);

		graph->Dump();

		if(n == 0 || n != m) return E_FAIL;

		CComPtr<IBaseFilter> writer;

		if(FAILED(hr = writer.CoCreateInstance(CLSID_FileWriter)))
		{
			return hr;
		}

		if(FAILED(hr = graph->AddFilter(writer, L"Writer")))
		{
			return hr;
		}

		if(FAILED(hr = graph->ConnectFilterDirect(muxer, writer, NULL)))
		{
			return hr;
		}

		if(FAILED(hr = CComQIPtr<IFileSinkFilter2>(writer)->SetFileName(dst.c_str(), NULL)))
		{
			return hr;
		}

		CComQIPtr<IMediaControl>(graph)->Run();

		long code;
		LONG_PTR p1, p2;

		while(1)
		{
			hr = CComQIPtr<IMediaEvent>(graph)->GetEvent(&code, &p1, &p2, 1000);

			REFERENCE_TIME rt = 0;
		
			CComQIPtr<IMediaSeeking>(graph)->GetCurrentPosition(&rt);
		
			printf("pos = %I64d\n", rt / 10000);

			//if(::GetAsyncKeyState(VK_CONTROL) & 0x8000) break;

			if(hr == E_ABORT) continue;

			if(FAILED(hr)) break;

			CComQIPtr<IMediaEvent>(graph)->FreeEventParams(code, p1, p2);

			if(code == EC_COMPLETE || code == EC_ERRORABORT || code == EC_STATE_CHANGE && (FILTER_STATE)p1 != State_Running)
			{
				break;
			}
		}

		CComQIPtr<IMediaControl>(graph)->Stop();
	}

	if(SUCCEEDED(hr))
	{
		CComPtr<IGraphBuilder2> graph = new CFGManagerCustom(L"CFGManagerCustom", NULL);

		CComPtr<IBaseFilter> source;

		if(FAILED(hr = graph->AddSourceFilter(dst.c_str(), L"Source", &source))) 
		{
			return hr;
		}

		dur2 = 0;

		if(CComQIPtr<IMediaSeeking> pMS = source) 
		{
			pMS->GetDuration(&dur2);
		}

		int ms1 = (int)(dur1 / 10000);
		int ms2 = (int)(dur2 / 10000);
		int ratio = ms1 > 0 ? (int)(ms2 * 100 / ms1) : 0;

		printf("%d %d %d%%\n", ms2, ms1, ratio);
			
		if(ms1 == 0 || ms2 == 0 || ratio < 95)
		{
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT PrintDuration(const std::wstring& src)
{
	HRESULT hr;

	REFERENCE_TIME dur = 0;

	CComPtr<IGraphBuilder2> graph = new CFGManagerCustom(L"CFGManagerCustom", NULL);

	CComPtr<IBaseFilter> source;

	hr = AddSource(src, graph, source, &dur);

	if(FAILED(hr)) return hr;

	if(dur == 0) return E_FAIL;

	printf("%lld\n", dur / 10000);

	return S_OK;
}

HRESULT PrintStreams(const std::wstring& src)
{
	HRESULT hr;

	CComPtr<IGraphBuilder2> graph = new CFGManagerCustom(L"CFGManagerCustom", NULL);

	CComPtr<IBaseFilter> source;

	hr = AddSource(src, graph, source, NULL);

	if(FAILED(hr)) return hr;

	Foreach(source, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
		{
			if(pmt->majortype == MEDIATYPE_Video) printf("v");
			else if(pmt->majortype == MEDIATYPE_Audio) printf("a");
			else printf("u");

			return S_OK;
		});

		return S_CONTINUE;
	});

	printf("\n");

	return S_OK;
}

#include "..\directshow\GenericTuner.h"
#include <setupapi.h>
#include <Devpropdef.h>

#define MAX_DEV_LEN 1000

int _tmain(int argc, _TCHAR* argv[])
{
	int res = 0;

	CoInitialize(0);

/*
	if(argc >= 2)
	{
		Search(argv[1]);
	}
*/

/*
	std::list<TunerDesc> tds;

	GenericTuner::EnumTuners(tds, false, false);

	for(auto i = tds.begin(); i != tds.end(); i++)
	{
		_tprintf(_T("%s\n%s\n"), i->id.c_str(), i->name.c_str());
	}

	return 0;
*/
	Play(argv[1]);
	return 0;

	//clock_t start = clock();

	//SubTest();

	//printf("%d\n", clock() - start);
	
	// Regex();

	//MotionSearchTest();

	// CSATest();

	//SocketTest();

	//AnyseeIR();

	//AMFTest();

	// SmartCardTest();

	//SkypeTest();
	
	// remux

	if(1)
	{
		res = -1;

		std::wstring src, dst;
		CComPtr<IBaseFilter> muxer;

		for(int i = 1; i < argc; i++)
		{
			if(wcscmp(argv[i], L"/dsm") == 0 && i < argc - 2)
			{
				src = argv[++i];
				dst = argv[++i];

				HRESULT hr;
			
				muxer = new CDSMMuxerFilter(NULL, &hr);

				break;
			}
			else if(wcscmp(argv[i], L"/mkv") == 0 && i < argc - 2)
			{
				src = argv[++i];
				dst = argv[++i];

				HRESULT hr;
			
				muxer = new CMatroskaMuxerFilter(NULL, &hr);

				break;
			}
			else if(wcscmp(argv[i], L"/mp4") == 0 && i < argc - 2)
			{
				src = argv[++i];
				dst = argv[++i];

				HRESULT hr;
			
				muxer = new Mpeg4Mux(NULL, &hr);

				break;
			}
			else if(wcscmp(argv[i], L"/dur") == 0 && i < argc - 1)
			{
				src = argv[++i];

				res = PrintDuration(src);

				break;
			}
			else if(wcscmp(argv[i], L"/streams") == 0 && i < argc - 1)
			{
				src = argv[++i];

				res = PrintStreams(src);

				break;
			}
		}

		if(muxer)
		{
			res = Remux(src, dst, muxer);
		}
	}

	CoUninitialize();

#ifdef _DEBUG
	_kbhit();
#endif

	return res;
}

