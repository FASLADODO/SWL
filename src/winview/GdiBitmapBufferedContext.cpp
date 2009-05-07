#include "swl/winview/GdiBitmapBufferedContext.h"
#include <wingdi.h>

#if defined(WIN32) && defined(_DEBUG)
void* __cdecl operator new(size_t nSize, const char* lpszFileName, int nLine);
#define new new(__FILE__, __LINE__)
//#pragma comment(lib, "mfc80ud.lib")
#endif


namespace swl {

GdiBitmapBufferedContext::GdiBitmapBufferedContext(HWND hWnd, const Region2<int>& drawRegion, const bool isAutomaticallyActivated /*= true*/)
: base_type(drawRegion, true),
  hWnd_(hWnd), hDC_(NULL), memDC_(NULL), memBmp_(NULL), oldBmp_(NULL)
{
	if (createOffScreen() && isAutomaticallyActivated)
		activate();
}

GdiBitmapBufferedContext::GdiBitmapBufferedContext(HWND hWnd, const RECT& drawRect, const bool isAutomaticallyActivated /*= true*/)
: base_type(Region2<int>(drawRect.left, drawRect.top, drawRect.right, drawRect.bottom), true),
  hWnd_(hWnd), hDC_(NULL), memDC_(NULL), memBmp_(NULL), oldBmp_(NULL)
{
	if (createOffScreen() && isAutomaticallyActivated)
		activate();
}

GdiBitmapBufferedContext::~GdiBitmapBufferedContext()
{
	deactivate();

	// free-up the off-screen DC
	if (oldBmp_)
	{
		SelectObject(memDC_, oldBmp_);
		oldBmp_ = NULL;
	}
	if (memBmp_)
	{
		DeleteObject(memBmp_);
		memBmp_ = NULL;
	}

	if (memDC_)
	{
		DeleteDC(memDC_);
		memDC_ = NULL;
	}

	// release DC
	if (hDC_)
	{
		ReleaseDC(hWnd_, hDC_);
		hDC_ = NULL;
	}
}

bool GdiBitmapBufferedContext::swapBuffer()
{
	//if (!isActivated() || isDrawing()) return false;
	if (isDrawing()) return false;
	if (NULL == memBmp_ || NULL == memDC_ || NULL == hDC_) return false;
	setDrawing(true);

	// copy off-screen buffer to window's DC
	const bool ret = TRUE == BitBlt(
		hDC_,
		drawRegion_.left, drawRegion_.bottom, drawRegion_.getWidth(), drawRegion_.getHeight(), 
		memDC_,
		0, 0,  //drawRegion_.left, drawRegion_.bottom,
		SRCCOPY
	);
/*
	const bool ret = TRUE == StretchBlt(
		hDC_,
		drawRegion_.left, drawRegion_.bottom, drawRegion_.getWidth(), drawRegion_.getHeight(),
		memDC_,
		drawRegion_.left, drawRegion_.bottom, drawRegion_.getWidth(), drawRegion_.getHeight(),
		SRCCOPY
	);
	const bool ret = TRUE == StretchDIBits(
		hDC_,
		drawRegion_.left, drawRegion_.bottom, drawRegion_.getWidth(), drawRegion_.getHeight(),
		drawRegion_.left, drawRegion_.bottom, drawRegion_.getWidth(), drawRegion_.getHeight(),
		(void *)bits_,
		&bitsInfo_,
		DIB_RGB_COLORS, //DIB_PAL_COLORS
		SRCCOPY
	);
*/
	setDrawing(false);
	return ret;
}

bool GdiBitmapBufferedContext::resize(const int x1, const int y1, const int x2, const int y2)
{
	if (isActivated()) return false;
	drawRegion_ = Region2<int>(x1, y1, x2, y2);

	// free-up the off-screen DC
	if (oldBmp_)
	{
		SelectObject(memDC_, oldBmp_);
		oldBmp_ = NULL;
	}
	if (memBmp_)
	{
		DeleteObject(memBmp_);
		memBmp_ = NULL;
	}

	if (memDC_)
	{
		DeleteDC(memDC_);
		memDC_ = NULL;
	}

	// release DC
	if (hDC_)
	{
		ReleaseDC(hWnd_, hDC_);
		hDC_ = NULL;
	}

	return createOffScreen();
}

bool GdiBitmapBufferedContext::activate()
{
	if (isActivated()) return true;
	if (NULL == memBmp_ || NULL == memDC_ || NULL == hDC_) return false;

	setActivation(true);
	return true;

	// draw something into memDC_
}

bool GdiBitmapBufferedContext::deactivate()
{
	if (!isActivated()) return true;
	if (NULL == memBmp_ || NULL == memDC_ || NULL == hDC_) return false;

	setActivation(false);
	return true;
}

bool GdiBitmapBufferedContext::createOffScreen()
{
	if (NULL == hWnd_) return false;

	// get DC for window
	hDC_ = GetDC(hWnd_);
	if (NULL == hDC_) return false;

	// create an off-screen DC for double-buffering
	memDC_ = CreateCompatibleDC(hDC_);
	if (NULL == memDC_)
	{
		ReleaseDC(hWnd_, hDC_);
		hDC_ = NULL;
		return false;
	}

	return createOffScreenBitmap();
}

bool GdiBitmapBufferedContext::createOffScreenBitmap()
{
	// method #1
	memBmp_ = CreateCompatibleBitmap(hDC_, drawRegion_.getWidth(), drawRegion_.getHeight());

	// method #2
/*
    // create dib section
	BITMAPINFO bmiDIB;
	memset(&bmiDIB, 0, sizeof(BITMAPINFO));

	// when using 256 color
	//RGBQUAD rgb[255];

	// following routine aligns given value to 4 bytes boundary.
	// the current implementation of DIB rendering in Windows 95/98/NT seems to be free from this alignment
	// but old version compatibility is needed.
	const int width = ((drawRegion_.getWidth()+3)/4*4 > 0) ? drawRegion_.getWidth() : 4;
	const int height = (0 == drawRegion_.getHeight()) ? 1 : drawRegion_.getHeight();

	bmiDIB.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bmiDIB.bmiHeader.biWidth		= width;
	bmiDIB.bmiHeader.biHeight		= height;
	bmiDIB.bmiHeader.biPlanes		= 1;
	//bmiDIB.bmiHeader.biBitCount	= 32;
	bmiDIB.bmiHeader.biBitCount		= GetDeviceCaps(memDC_, BITSPIXEL);
	bmiDIB.bmiHeader.biCompression	= BI_RGB;
	bmiDIB.bmiHeader.biSizeImage	= 0;  //  for BI_RGB
	//bmiDIB.bmiHeader.biSizeImage	= width * height * 3;

	//// when using 256 color
	//PALETTEENTRY aPaletteEntry[256];
	//GetPaletteEntries(ms_hPalette, 0, 256, aPaletteEntry);
	//
	//for (int i = 0; i < 256; ++i)
	//{
	//	bmiDIB.bmiColors[i].rgbRed		= aPaletteEntry[i].peRed;
	//	bmiDIB.bmiColors[i].rgbGreen	= aPaletteEntry[i].peGreen;
	//	bmiDIB.bmiColors[i].rgbBlue		= aPaletteEntry[i].peBlue;
	//	bmiDIB.bmiColors[i].rgbReserved = 0;
	//}

    // offscreen surface generated by the dib section
    void* offScreen;
    HBITMAP memBmp_ = CreateDIBSection(memDC_, &bmiDIB, DIB_RGB_COLORS, &offScreen, 0L, 0);
*/
	if (NULL == memBmp_)
	{
		ReleaseDC(hWnd_, hDC_);
		hDC_ = NULL;
		return false;
	}
	oldBmp_ = (HBITMAP)SelectObject(memDC_, memBmp_);

	return true;
}

}  // namespace swl
