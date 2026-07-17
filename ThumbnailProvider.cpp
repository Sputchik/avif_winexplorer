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
    m_bufferSize = stat.cbSize.LowPart;

    // new uint8_t[] avoids the mandatory zero-initialization of std::vector
    m_buffer.reset(new uint8_t[m_bufferSize]);

    ULONG bytesRead = 0;
    hr = pStream->Read(m_buffer.get(), m_bufferSize, &bytesRead);
    return hr;
}
// --- IThumbnailProvider Implementation ---
IFACEMETHODIMP AvifThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
    if (!m_buffer) return E_FAIL;
    avifDecoder* decoder = avifDecoderCreate();
    if (!decoder) return E_FAIL;

    decoder->codecChoice = AVIF_CODEC_CHOICE_DAV1D;
    decoder->strictFlags = AVIF_STRICT_DISABLED;
    decoder->ignoreExif = AVIF_TRUE; // we never read EXIF, skip parsing/copying it
    decoder->ignoreXMP = AVIF_TRUE;  // same for XMP
    decoder->maxThreads = 1;         // refined below once we know the image size

    avifResult result = avifDecoderSetIOMemory(decoder, m_buffer.get(), m_bufferSize);
    if (result != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    // Parse only reads the container/headers - image dimensions are available here,
    // before the (expensive) actual AV1 frame decode.
    result = avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    // Large stills are usually tile-encoded, so a couple of decode threads help.
    // Small/typical images stay single-threaded - thread-pool spin-up cost would
    // otherwise dominate a decode that's already cheap.
    const uint64_t pixelCount = (uint64_t)decoder->image->width * (uint64_t)decoder->image->height;
    if (pixelCount > 8000000ULL) {
        decoder->maxThreads = 4;
    } else if (pixelCount > 2000000ULL) {
        decoder->maxThreads = 2;
    }

    result = avifDecoderNextImage(decoder);
    if (result != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    // Explorer only ever needs a cx-by-cx box. Downscale the YUV planes now (cheap:
    // libyuv SIMD, 1-1.5 bytes/pixel) instead of converting to full-res BGRA (4
    // bytes/pixel) and making Explorer scale the result down afterward. Only ever
    // scale down - upscaling here wastes time and adds nothing.
    if (cx > 0) {
        const uint32_t longSide = (decoder->image->width > decoder->image->height)
                                       ? decoder->image->width
                                       : decoder->image->height;
        if (longSide > cx) {
            const double scale = (double)cx / (double)longSide;
            uint32_t dstW = (uint32_t)(decoder->image->width * scale + 0.5);
            uint32_t dstH = (uint32_t)(decoder->image->height * scale + 0.5);
            if (dstW < 1) dstW = 1;
            if (dstH < 1) dstH = 1;

            avifDiagnostics diag;
            if (avifImageScale(decoder->image, dstW, dstH, &diag) != AVIF_RESULT_OK) {
                avifDecoderDestroy(decoder);
                return E_FAIL;
            }
        }
    }

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image); // must come after the scale above
    rgb.depth = 8;
    rgb.format = AVIF_RGB_FORMAT_BGRA;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rgb.width;
    bmi.bmiHeader.biHeight = -static_cast<LONG>(rgb.height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hbmp || !pBits) {
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    rgb.pixels = static_cast<uint8_t*>(pBits);
    rgb.rowBytes = rgb.width * 4;

    if (avifImageYUVToRGB(decoder->image, &rgb) != AVIF_RESULT_OK) {
        DeleteObject(hbmp);
        avifDecoderDestroy(decoder);
        return E_FAIL;
    }

    *phbmp = hbmp;
    *pdwAlpha = WTSAT_ARGB;
    avifDecoderDestroy(decoder);
    return S_OK;
}