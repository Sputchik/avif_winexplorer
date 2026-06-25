#include "ThumbnailProvider.h"
#include <avif/avif.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

// --- IUnknown Implementation ---
IFACEMETHODIMP AvifThumbnailProvider::QueryInterface(REFIID riid, void **ppv) {
    static const QITAB qit[] = {
        QITABENT(AvifThumbnailProvider, IInitializeWithStream),
        QITABENT(AvifThumbnailProvider, IThumbnailProvider),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) AvifThumbnailProvider::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) AvifThumbnailProvider::Release() {
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

// --- IInitializeWithStream Implementation ---
IFACEMETHODIMP AvifThumbnailProvider::Initialize(IStream *pStream, DWORD grfMode) {
    STATSTG stat;
    HRESULT hr = pStream->Stat(&stat, STATFLAG_NONAME);
    if (FAILED(hr)) return hr;

    // Read the file from Windows Explorer into std::vector buffer
    m_buffer.resize(stat.cbSize.LowPart);
    ULONG bytesRead = 0;
    hr = pStream->Read(m_buffer.data(), stat.cbSize.LowPart, &bytesRead);
    return hr;
}

// --- IThumbnailProvider Implementation ---
IFACEMETHODIMP AvifThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
    if (m_buffer.empty()) return E_FAIL;

    avifDecoder* decoder = avifDecoderCreate();
    decoder->codecChoice = AVIF_CODEC_CHOICE_DAV1D;
    decoder->maxThreads = 2;
    decoder->strictFlags = AVIF_STRICT_DISABLED;

    avifResult result = avifDecoderSetIOMemory(decoder, m_buffer.data(), m_buffer.size());

    if (result != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    if (avifDecoderParse(decoder) == AVIF_RESULT_OK) {
        result = avifDecoderNextImage(decoder);
    }

    if (result != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    // 1. Setup the RGB properties, but DO NOT allocate memory yet
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.depth = 8; 
    rgb.format = AVIF_RGB_FORMAT_BGRA;

    // 2. Setup Windows Bitmap properties
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rgb.width;
    bmi.bmiHeader.biHeight = -static_cast<LONG>(rgb.height); 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    
    if (hbmp && pBits) {
        // 3. Wire libavif directly to Windows memory
        rgb.pixels = static_cast<uint8_t*>(pBits);
        rgb.rowBytes = rgb.width * 4; // 32-bit BGRA is exactly 4 bytes per pixel

        // 4. Decode YUV straight into the Windows HBITMAP. Zero copies..
        avifImageYUVToRGB(decoder->image, &rgb);
        
        *phbmp = hbmp;
        *pdwAlpha = WTSAT_ARGB; 
    } else {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    avifDecoderDestroy(decoder);
    return S_OK;
}