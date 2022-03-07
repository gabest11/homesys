//------------------------------------------------------------------------------
// File: SeekPT.h
//
// Desc: DirectShow base classes.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#pragma once

#include "combase.h"
#include "ctlutil.h"

class CSeekingPassThru : public ISeekingPassThru, public CUnknown
{
public:
    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    CSeekingPassThru(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    ~CSeekingPassThru();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP Init(BOOL bSupportRendering, IPin *pPin);

private:
    CPosPassThru              *m_pPosPassThru;
};
