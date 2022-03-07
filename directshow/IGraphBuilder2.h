/* 
 *	Copyright (C) 2003-2006 Gabest
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

#include "../3rdparty/baseclasses/baseclasses.h"

[uuid("165BE9D6-0929-4363-9BA3-580D735AA0F6")]
interface IGraphBuilder2 : public IFilterGraph2
{
	STDMETHOD(RenderFileMime)(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrMimeType) PURE;
	STDMETHOD(AddSourceFilterMime)(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, LPCWSTR lpcwstrMimeType, IBaseFilter** ppFilter) PURE;
	STDMETHOD(HasRegisteredSourceFilter)(LPCWSTR fn) PURE;
	STDMETHOD(AddFilterForDisplayName)(LPCWSTR dispname, IBaseFilter** ppBF) PURE;
	STDMETHOD(NukeDownstream)(IUnknown* pUnk) PURE;
	STDMETHOD(ConnectFilter)(IPin* pPinOut) PURE;
	STDMETHOD(ConnectFilter)(IBaseFilter* pBF, IPin* pPinIn) PURE;
	STDMETHOD(ConnectFilter)(IPin* pPinOut, IBaseFilter* pBF) PURE;
	STDMETHOD(ConnectFilterDirect)(IBaseFilter* pBFOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt) PURE;
	STDMETHOD(ConnectFilterDirect)(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt) PURE;
	STDMETHOD(FindInterface)(REFIID iid, void** ppv, bool remove) PURE;
	STDMETHOD(AddToROT)() PURE;
	STDMETHOD(RemoveFromROT)() PURE;
	STDMETHOD(Reset)() PURE;
	STDMETHOD(Clean)() PURE;
	STDMETHOD(Dump)() PURE;
	STDMETHOD(GetVolume)(float& volume) PURE;
	STDMETHOD(SetVolume)(float volume) PURE;
	STDMETHOD(GetBalance)(float& balance) PURE;
	STDMETHOD(SetBalance)(float balance) PURE;
};

// private use only

[uuid("43CDA93D-6A4E-4A07-BD3E-49D161073EE7")]
interface IGraphBuilderDeadEnd : public IUnknown
{
	STDMETHOD(ResetDeadEnd)() PURE;
	STDMETHOD_(int, GetCount)() PURE;
	STDMETHOD(GetDeadEnd) (int index, std::list<std::wstring>& path, std::list<CMediaType>& mts) PURE;
};
