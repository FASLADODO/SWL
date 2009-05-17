#include "swl/winview/GdiContextBase.h"
#include <iostream>

#if defined(WIN32) && defined(_DEBUG)
void* __cdecl operator new(size_t nSize, const char* lpszFileName, int nLine);
#define new new(__FILE__, __LINE__)
//#pragma comment(lib, "mfc80ud.lib")
#endif


namespace swl  {

/*static*/ HPALETTE GdiContextBase::shPalette_ = NULL;
/*static*/ size_t GdiContextBase::sUsedPaletteCount_ = 0;

GdiContextBase::GdiContextBase(const Region2<int> &drawRegion, const bool isOffScreenUsed)
: base_type(drawRegion, isOffScreenUsed),
  isPaletteUsed_(false)
{
}

GdiContextBase::GdiContextBase(const RECT &drawRect, const bool isOffScreenUsed)
: base_type(Region2<int>(drawRect.left, drawRect.top, drawRect.right, drawRect.bottom), isOffScreenUsed),
  isPaletteUsed_(false)
{
}

GdiContextBase::~GdiContextBase()
{
}

/*static*/ void GdiContextBase::createPalette(HDC hDC, const int colorBitCount)
{
	// FIXME [check] >> this implementation is not tested
	if (colorBitCount <= 8 && !shPalette_)
	{
		// following routines are originated from ogl2 sdk made by Sillicon Graphics and modified for glext
		const int paletteSize = 1 << colorBitCount;
		LOGPALETTE *logPalette = (LOGPALETTE *)new char [sizeof(LOGPALETTE) + paletteSize * sizeof(PALETTEENTRY)];
		
		if (logPalette)
		{
			logPalette->palVersion = 0x300;
			logPalette->palNumEntries = GetSystemPaletteEntries(hDC, 0, paletteSize, logPalette->palPalEntry);

			shPalette_ = CreatePalette(logPalette);

			delete [] (char *)logPalette;
			logPalette = NULL;
		}
	}

	if (shPalette_)
	{
		SelectPalette(hDC, shPalette_, FALSE);
		RealizePalette(hDC);
		++sUsedPaletteCount_;
	}
}

/*static*/ void GdiContextBase::deletePalette(HDC hDC)
{
	// delete palette
	SelectPalette(hDC, (HPALETTE)GetStockObject(DEFAULT_PALETTE), FALSE);
	if (shPalette_ && --sUsedPaletteCount_ <= 0)
	{
		DeleteObject(shPalette_);
		shPalette_ = NULL;
		sUsedPaletteCount_ = 0;
	}
}

}  // namespace swl
