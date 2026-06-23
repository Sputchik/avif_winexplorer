#pragma once
#include <windows.h>
#include <thumbcache.h>
#include <vector>

// Unique GUID for your specific AVIF Thumbnailer
// {B7A41C69-7788-4660-84E3-8E50C0A1B2C3}
const CLSID CLSID_AvifThumbnailProvider = 
{ 0xB7A41C69, 0x7788, 0x4660, { 0x84, 0xE3, 0x8E, 0x50, 0xC0, 0xA1, 0xB2, 0xC3 } };

class AvifThumbnailProvider : public IInitializeWithStream, public IThumbnailProvider {
public:
    AvifThumbnailProvider() : m_cRef(1) {}
    virtual ~AvifThumbnailProvider() {}

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IInitializeWithStream
    IFACEMETHODIMP Initialize(IStream *pStream, DWORD grfMode);

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

private:
    long m_cRef;
    std::vector<uint8_t> m_buffer;
};