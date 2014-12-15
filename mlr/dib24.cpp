//------------------------------------------------------------------------ 
// Copyright (c) 2003 David Poon
//
// Permission is hereby granted, free of charge, to any person 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without 
// restriction, including without limitation the rights to use, 
// copy, modify, merge, publish, distribute, sublicense, and/or 
// sell copies of the Software, and to permit persons to whom 
// the Software is furnished to do so, subject to the following 
// conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE.
//------------------------------------------------------------------------ 

#include "stdafx.h"

#include <stdexcept>
#include <cstdio>
#include "dib24.h"

// HIMETRIC units per inch - taken from MFC source code for CDC class.
const int DIB24::HIMETRIC_INCH = 2540;

int DIB24::m_logpixelsx = 0;
int DIB24::m_logpixelsy = 0;

DIB24::DIB24()
{
    dc = 0;
    hBitmap = 0;
    width = 0;
    height = 0;
    pitch = 0;

    m_hPrevObj = 0;
    m_pBits = 0;

    memset(&info, 0, sizeof(BITMAPINFO));

    if (!m_logpixelsx && !m_logpixelsy)
    {
        HDC hScreenDC = CreateCompatibleDC(GetDC(0));
		
        if (!hScreenDC)
        {
printf("failed to init m_logpixels\n" );
exit(1);
//            throw std::runtime_error(
//                _T("failed to init m_logpixelsx and/or m_logpixelsy"));
        }

        m_logpixelsx = GetDeviceCaps(hScreenDC, LOGPIXELSX);
        m_logpixelsy = GetDeviceCaps(hScreenDC, LOGPIXELSY);

        DeleteDC(hScreenDC);
    }
}

DIB24::~DIB24()
{
    destroy();
}

bool DIB24::create(int widthPixels, int heightPixels)
{
    destroy();
        
    width = widthPixels;
    height = heightPixels;
    pitch = ((width * 24 + 31) & ~31) >> 3;
    dc = CreateCompatibleDC(0);
    
    if (!dc)
        return false;
    
    memset(&info, 0, sizeof(info));

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biPlanes = 1;

    hBitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, 
        reinterpret_cast<void**>(&m_pBits), 0, 0);

    if (!hBitmap)
    {
        destroy();
        return false;
    }

    GdiFlush();
    return true;
}

void DIB24::destroy()
{
    deselectObject();

    if (hBitmap)
    {
        DeleteObject(hBitmap);
        hBitmap = 0;
    }

    if (dc)
    {
        DeleteDC(dc);
        dc = 0;
    }
}

bool DIB24::loadDesktop()
{
    // Takes a screen capture of the current Windows desktop and stores
    // the image in the DIB24 object.

    HWND hDesktop = GetDesktopWindow();

    if (!hDesktop)
        return false;

    int desktopWidth = GetSystemMetrics(SM_CXSCREEN);
    int desktopHeight = GetSystemMetrics(SM_CYSCREEN);
    HDC hDesktopDC = GetDCEx(hDesktop, 0, DCX_CACHE | DCX_WINDOW);

    if (!hDesktopDC)
        return false;

    if (!create(desktopWidth, desktopHeight))
    {
        ReleaseDC(hDesktop, hDesktopDC);
        return false;
    }

    selectObject();

    if (!BitBlt(dc, 0, 0, width, height, hDesktopDC, 0, 0, SRCCOPY))
    {
        destroy();
        ReleaseDC(hDesktop, hDesktopDC);
        return false;
    }

    deselectObject();
    ReleaseDC(hDesktop, hDesktopDC);
    return true;
}

bool DIB24::loadBitmap(LPCTSTR filename)
{
    // Loads a BMP image and stores it in the DIB24 object.

    HANDLE hImage = LoadImage(GetModuleHandle(0), filename, IMAGE_BITMAP,
        0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    if (!hImage)
        return false;

    BITMAP bitmap = {0};

    if (!GetObject(hImage, sizeof(bitmap), &bitmap))
    {
        DeleteObject(hImage);
        return false;
    }

    HDC hImageDC = CreateCompatibleDC(0);

    if (!hImageDC)
    {
        DeleteObject(hImage);
        return false;
    }

    SelectObject(hImageDC, hImage);

    int h = (bitmap.bmHeight < 0) ? -bitmap.bmHeight : bitmap.bmHeight;

    if (create(bitmap.bmWidth, h))
    {
        selectObject();
        
        if (!BitBlt(dc, 0, 0, width, height, hImageDC, 0, 0, SRCCOPY))
        {
            destroy();
            DeleteDC(hImageDC);
            DeleteObject(hImage);
            return false;
        }

        deselectObject();
    }

    DeleteDC(hImageDC);
    DeleteObject(hImage);
    return true;
}

bool DIB24::loadPicture(LPCTSTR filename)
{
    // Loads an image using the IPicture COM interface and stores the
    // image in the DIB24 object.
    //
    // Supported image formats: BMP, EMF, GIF, ICO, JPG, WMF.

    bool status = false;
    FILE *fp = _tfopen(filename, _T("rb"));

    if (fp)
    {
        fseek(fp, 0, SEEK_END);

        long length = ftell(fp);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, length);

        if (hGlobal)
        {
            void *p = GlobalLock(hGlobal);

            if (!p)
            {
                fclose(fp);
            }
            else
            {
                rewind(fp);
                fread(p, length, 1, fp);
                GlobalUnlock(hGlobal);
                fclose(fp);

                IStream *pIStream = 0;
                HRESULT hr = 0;

                hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pIStream);

                if (SUCCEEDED(hr))
                {
                    IPicture *pIPicture = 0;
                    
                    hr = OleLoadPicture(pIStream, length, FALSE, 
                            IID_IPicture, reinterpret_cast<LPVOID*>(&pIPicture));

                    if (SUCCEEDED(hr))
                    {
                        LONG lWidth = 0;
                        LONG lHeight = 0;

                        pIPicture->get_Width(&lWidth);
                        pIPicture->get_Height(&lHeight);

                        //int w = MulDiv(lWidth, m_logpixelsx, HIMETRIC_INCH);
                        //int h = MulDiv(lHeight, m_logpixelsy, HIMETRIC_INCH);

                        int w = (lWidth * m_logpixelsx) / HIMETRIC_INCH;
                        int h = (lHeight * m_logpixelsy) / HIMETRIC_INCH;

                        if (create(w, h))
                        {
                            selectObject();

                            hr = pIPicture->Render(dc, 0, 0, width, height,
                                    0, lHeight, lWidth, -lHeight, 0);

                            if (SUCCEEDED(hr))
                                status = true;

                            deselectObject();
                        }

                        pIPicture->Release();
                    }

                    pIStream->Release();
                }
            }
        }

        GlobalFree(hGlobal);
    }

    return status;
}

void DIB24::selectObject()
{
    if (dc)
        m_hPrevObj = SelectObject(dc, hBitmap);
}

void DIB24::deselectObject()
{
    if (dc && m_hPrevObj)
    {
        SelectObject(dc, m_hPrevObj);
        m_hPrevObj = 0;
    }
}

void DIB24::copyByteAligned(BYTE *pDest)
{
    // 'pDest' must already point to a chunk of allocated memory of the
    // correct size (i.e., width pixels X height pixels X 24 bits).
    // 
    // The returned image will be byte aligned, and the pixel format
    // will be BGR (as per Windows DIB standards).

    if (pDest)
    {
        const int widthBytes = width * 3;

        for (int y = 0; y < height; ++y)
            memcpy(&pDest[widthBytes * y], &m_pBits[pitch * y], widthBytes);
    }
}