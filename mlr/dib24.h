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

#if !defined(DIB24_H)
#define DIB24_H

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>
#include <olectl.h>

//------------------------------------------------------------------------
// 24-bit BGR Windows DIB class.
//
// Supports the loading of BMP, EMF, GIF, ICO, JPG, and WMF files using
// only native Windows DLLs. No more linking to 3rd party DLLs.
//
// Support is also provided for capturing a screen shot of the current
// Windows desktop and loading that as an image into the DIB24 object.
//
// This class stores the DIB in a top-down orientation.
// The pixel data stored in 24-bit Windows DIBs is ordered BGR.
//
// All Windows DIBs are aligned to 4-byte (DWORD) memory boundaries.
// This means that each scanline is padded with extra bytes to ensure
// that the next scanline starts on a 4-byte memory boundary.
//
// To get a copy of the DIB that is BYTE (1-byte) aligned with all the
// extra padding bytes removed, use the DIB24::copyByteAligned() method.
//------------------------------------------------------------------------

class DIB24
{
public:
    HDC dc;
    HBITMAP hBitmap;
    int width;
    int height;
    int pitch;
    BITMAPINFO info;

    DIB24();
    ~DIB24();

    BYTE *operator[](int row) {return &m_pBits[pitch * row];}
    const BYTE *operator[](int row) const {return &m_pBits[pitch * row];}

    bool create(int widthPixels, int heightPixels);
    void destroy();

    bool loadDesktop();
    bool loadBitmap(LPCTSTR filename);
    bool loadPicture(LPCTSTR filename);

    void selectObject();
    void deselectObject();
    
    void copyByteAligned(BYTE *pDest);

private:
    static const int HIMETRIC_INCH;
    static int m_logpixelsx;
    static int m_logpixelsy;

    HGDIOBJ m_hPrevObj;
    BYTE *m_pBits;
};

#endif