/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "Texture.h"
#include <d3d10.h>

namespace DXUI
{
	class Texture10 : public Texture
	{
		CComPtr<ID3D10Device> m_dev;
		CComPtr<ID3D10Texture2D> m_texture;
		D3D10_TEXTURE2D_DESC m_desc;
		CComPtr<ID3D10ShaderResourceView> m_srv;
		CComPtr<ID3D10RenderTargetView> m_rtv;
		CComPtr<ID3D10DepthStencilView> m_dsv;

	public:
		explicit Texture10(ID3D10Texture2D* texture);

		bool Update(const Vector4i& r, const void* data, int pitch);
		bool Map(MemMap& m, const Vector4i* r);
		void Unmap();
		bool Save(LPCWSTR fn, bool dds = false);

		operator ID3D10Texture2D*();
		operator ID3D10ShaderResourceView*();
		operator ID3D10RenderTargetView*();
		operator ID3D10DepthStencilView*();
	};
}