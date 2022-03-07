

long __stdcall StringValidateDestA(const char* pszDest, size_t cchDest, const size_t cchMax)
{
    long hr = 0;

    if ((cchDest == 0) || (cchDest > cchMax))
    {
        hr = (long)0x80070057L;
    }

    return hr;
}

long __stdcall StringCopyWorkerA(char* pszDest, size_t cchDest, size_t* pcchNewDestLength, const char* pszSrc, size_t cchToCopy)
{
    long hr = 0;
    size_t cchNewDestLength = 0;
    
    // ASSERT(cchDest != 0);

    while (cchDest && cchToCopy && (*pszSrc != '\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        hr = 0x8007007AL;
    }

    *pszDest = '\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return hr;
}

extern "C" long __stdcall StringCbCopyA(char* pszDest, size_t cbDest, const char* pszSrc)
{
    long hr;
    size_t cchDest = cbDest / sizeof(char);

    hr = StringValidateDestA(pszDest, cchDest, 2147483647);

    if (hr > 0)
    {
        hr = StringCopyWorkerA(pszDest,
                               cchDest,
                               0,
                               pszSrc,
                               2147483647 - 1);
    }

    return hr;
}
