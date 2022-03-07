#pragma once

#include <d3d9.h>
#include "../3rdparty/baseclasses/baseclasses.h"
#include "../util/Vector.h"

struct SubPicDesc
{
	int w, h;
	int pitch;
	BYTE* bits;

	REFERENCE_TIME start;
	REFERENCE_TIME stop;
	
	Vector2i size; // currently used area of (w, h), top-left at (0, 0)
	Vector4i rect; // video rect inside size
	Vector4i bbox; // subtitle rect inside size

	struct SubPicDesc() {w = h = pitch = 0; bits = NULL; start = stop = 0; rect = bbox = Vector4i::zero();}
};

// ISubPic

[uuid("449E11F3-52D1-4a27-AA61-E2733AC92CC0")]
interface ISubPic : public IUnknown
{
	STDMETHOD_(REFERENCE_TIME, GetStart) () PURE;
	STDMETHOD_(REFERENCE_TIME, GetStop) () PURE;
	STDMETHOD (GetRect) (Vector4i& r) PURE;
};

// CSubPic

class CSubPic : public AlignedClass<16>, public CUnknown, public ISubPic
{
	REFERENCE_TIME m_start;
	REFERENCE_TIME m_stop;
	Vector4 m_rect;

public:
	CSubPic(const SubPicDesc& desc);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	STDMETHODIMP_(REFERENCE_TIME) GetStart();
	STDMETHODIMP_(REFERENCE_TIME) GetStop();
	STDMETHODIMP GetRect(Vector4i& r);
};

// ISubProvider

[uuid("D62B9A1A-879A-42db-AB04-88AA8F243CFD")]
interface ISubProvider : public IUnknown
{
	STDMETHOD (Lock) () PURE;
	STDMETHOD (Unlock) () PURE;

	STDMETHOD_(void*, GetStartPosition) (REFERENCE_TIME rt, float fps) PURE;
	STDMETHOD_(void*, GetNext) (void* pos) PURE;
	STDMETHOD(GetTime) (void* pos, float fps, REFERENCE_TIME& start, REFERENCE_TIME& stop) PURE;
	STDMETHOD_(bool, IsAnimated) (void* pos) PURE;

	STDMETHOD (Render) (SubPicDesc& desc, float fps) PURE;
};

class CSubProvider : public CUnknown, public CCritSec, public ISubProvider
{
public:
	CSubProvider();
	virtual ~CSubProvider();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubProvider

	STDMETHODIMP Lock();
	STDMETHODIMP Unlock();
};

// ISubPicQueue

[uuid("C8334466-CD1E-4ad1-9D2D-8EE8519BD180")]
interface ISubPicQueue : public IUnknown
{
	STDMETHOD (SetSubProvider) (ISubProvider* provider) PURE;
	STDMETHOD (GetSubProvider) (ISubProvider** provider) PURE;

	STDMETHOD (SetTime) (REFERENCE_TIME now) PURE;
	STDMETHOD (SetFPS) (float fps) PURE;
	STDMETHOD (SetTarget) (const Vector2i& s, const Vector4i& r) PURE;

	STDMETHOD (Invalidate) (REFERENCE_TIME rt = -1) PURE;
	STDMETHOD_(bool, LookupSubPic) (REFERENCE_TIME now, ISubPic** ppsp) PURE;

	STDMETHOD (GetStats) (int& count, REFERENCE_TIME& now, REFERENCE_TIME& start, REFERENCE_TIME& stop) PURE;
	STDMETHOD (GetStats) (int index, REFERENCE_TIME& start, REFERENCE_TIME& stop) PURE;
};

class CSubPicQueue 
	: public AlignedClass<16>
	, public CUnknown
	, protected CCritSec
	, public ISubPicQueue
	, private CAMThread
{
	const int m_maxsubpic;

	REFERENCE_TIME m_start;
	REFERENCE_TIME m_invalidate;
	std::list<CAdapt<CComPtr<ISubPic>>> m_subpics;
	enum {CMD_EXIT, CMD_TIME}; // IMPORTANT: EXIT must be before TIME if we want to exit fast from the destructor
	HANDLE m_events[2];
	bool m_break;

	REFERENCE_TIME UpdateQueue();

    DWORD ThreadProc();

protected:
	CComPtr<ISubProvider> m_provider;
	REFERENCE_TIME m_now;
	float m_fps;
	struct {Vector2i size; Vector4i rect;} m_target;
	SubPicDesc m_desc;

	bool Render(ISubProvider* provider, const REFERENCE_TIME& start, const REFERENCE_TIME& stop, float fps, ISubPic** ppsp);

	virtual bool CreateSubPic(const SubPicDesc& desc, ISubPic** ppsp) = 0;

public:
	CSubPicQueue(const Vector2i& maxsize, int maxsubpic);
	virtual ~CSubPicQueue();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicQueue

	STDMETHODIMP SetSubProvider(ISubProvider* provider);
	STDMETHODIMP GetSubProvider(ISubProvider** provider);

	STDMETHODIMP SetTime(REFERENCE_TIME now);
	STDMETHODIMP SetFPS(float fps);
	STDMETHODIMP SetTarget(const Vector2i& s, const Vector4i& r);

	STDMETHODIMP Invalidate(REFERENCE_TIME rt = -1);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME now, ISubPic** ppsp);

	STDMETHODIMP GetStats(int& count, REFERENCE_TIME& now, REFERENCE_TIME& start, REFERENCE_TIME& stop);
	STDMETHODIMP GetStats(int index, REFERENCE_TIME& start, REFERENCE_TIME& stop);
};

// CDX9SubPicQueue

class CDX9SubPicQueue : public CSubPicQueue
{
	class CDX9SubPic : public CSubPic
	{
		CComPtr<IDirect3DTexture9> m_texture;

	public:
		CDX9SubPic(const SubPicDesc& desc, IDirect3DTexture9* texture);
		
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
	};

protected:
	CComPtr<IDirect3DDevice9> m_dev;

	bool CreateSubPic(const SubPicDesc& desc, ISubPic** ppsp);

public:
	CDX9SubPicQueue(IDirect3DDevice9* dev, const Vector2i& maxsize, int maxsubpic);
};

// ISubStream

[uuid("DE11E2FB-02D3-45e4-A174-6B7CE2783BDB")]
interface ISubStream : public IUnknown
{
	STDMETHOD_(int, GetStreamCount) () PURE;
	STDMETHOD (GetStreamInfo) (int i, WCHAR** ppName, LCID* pLCID) PURE;
	STDMETHOD_(int, GetStream) () PURE;
	STDMETHOD (SetStream) (int iStream) PURE;
};

