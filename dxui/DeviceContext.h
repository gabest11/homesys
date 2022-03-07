#pragma once 

#include "Device.h"
#include "Property.h"
#include "ResourceLoader.h"
#include "../util/AlignedClass.h"
#include <string>
#include <list>
#include <map>

namespace SSF
{
	class Renderer; // just a fwd decl, we don't need to include its headers here
}

namespace DXUI
{
	class TextureEntry : public AlignedClass<16>
	{
	public:
		int n; 
		Texture* t;

		TextureEntry() {n = 0; t = NULL;}
		virtual ~TextureEntry() {delete t;}
	};

	class TextEntry : public TextureEntry
	{
	public:
		std::wstring s;
		int align;
		DWORD color;
		std::wstring face;
		int height;
		int style;
		Vector4i rect;
		Vector4i bbox;
		Vector2i offset;
	};

	class PixelShaderEntry : public AlignedClass<16>
	{
	public:
		int n; 
		PixelShader* ps;

		PixelShaderEntry() {n = 0; ps = NULL;}
		virtual ~PixelShaderEntry() {delete ps;}
	};

	class DeviceContext : public AlignedClass<16>, public CCritSec
	{
		std::map<std::wstring, TextureEntry*> m_texture;
		std::map<std::wstring, TextEntry*> m_text;
		std::map<std::wstring, PixelShaderEntry*> m_pixelshader;

		void IncAge(int limit = 3);

	protected:
		Device* m_dev;
		ResourceLoader* m_loader;
		PixelShader* m_ps;
		Texture* m_rt;
		Vector4i m_scissor;
		SSF::Renderer* m_ssf;

		Vector4i* GetScissor();

		int GetBlendStateIndex(Texture* t);
		int GetShaderIndex(Texture* t);

	public:
		DeviceContext(Device* dev, ResourceLoader* loader);
		virtual ~DeviceContext();

		void Flip();
		
		Device* GetDevice() const {return m_dev;}
		Texture* GetTexture(LPCWSTR id, bool priority = false, bool refresh = false);
		PixelShader* GetPixelShader(LPCWSTR id);

		Texture* CreateTexture(const BYTE* p, int w, int h, int pitch, bool alpha = true);

		void FlushCache() {IncAge(0);}

		//

		virtual void DrawTriangleList(const Vertex* v, int n, Texture* t, PixelShader* ps = NULL) = 0;
		virtual void DrawLineList(const Vertex* v, int n) = 0;

		//

		PixelShader* SetPixelShader(PixelShader* ps);
		Texture* SetTarget(Texture* rt);
		void SetScissor(const Vector4i* r);

		void Draw(const Vector4i& r, DWORD c);

		void Draw(Texture* t, const Vector4i& r, float alpha = 1.0f);
		void Draw(Texture* t, const Vector4i& r, const Vector4i& src, float alpha = 1.0f);

		void Draw9x9(Texture* t, const Vector4i& r, const Vector2i& margin, float alpha = 1.0f);
		void Draw9x9(Texture* t, const Vector4i& r, const Vector4i& margin, float alpha = 1.0f);

		void Draw(const Vector2i& p0, const Vector2i& p1, DWORD c0, DWORD c1);
		void Draw(const Vector2i& p0, const Vector2i& p1, DWORD c);

		TextEntry* Draw(LPCWSTR str, const Vector4i& r, bool measure = false);
		TextEntry* Draw(LPCWSTR str, const Vector4i& r, Vector2i offset, bool measure = false);

		static std::wstring Escape(LPCWSTR str);

		// TODO: move these into Vector4i

		static Vector4i FitRect(const Vector4i& rect, const Vector2i& size, bool inside = true);
		static Vector4i FitRect(const Vector4i& rect, const Vector4i& frame, bool inside = true);
		static Vector4i FitRect(const Vector2i& rect, const Vector2i& size, bool inside = true);
		static Vector4i FitRect(const Vector2i& rect, const Vector4i& frame, bool inside = true);
		static Vector4i CenterRect(const Vector4i& rect, const Vector2i& size);
		static Vector4i CenterRect(const Vector4i& rect, const Vector4i& frame);

		//

		Property<DWORD> TextureColor0;
		Property<DWORD> TextureColor1;

		Property<int> TextAlign;
		Property<DWORD> TextColor;
		Property<std::wstring> TextFace;
		Property<int> TextHeight;
		Property<int> TextStyle;
	};
}