#if !defined(__SWL_WIN_VIEW__WGL_BITMAP_BUFFERED_CONTEXT__H_ )
#define __SWL_WIN_VIEW__WGL_BITMAP_BUFFERED_CONTEXT__H_ 1


#include "swl/winview/WglContextBase.h"


namespace swl {

//-----------------------------------------------------------------------------------
//  Bitmap-buffered Context for OpenGL

class SWL_WIN_VIEW_API WglBitmapBufferedContext: public WglContextBase
{
public:
	typedef WglContextBase base_type;

public:
	WglBitmapBufferedContext(HWND hWnd, const Region2<int>& drawRegion, const bool isAutomaticallyActivated = true);
	WglBitmapBufferedContext(HWND hWnd, const RECT& drawRect, const bool isAutomaticallyActivated = true);
	virtual ~WglBitmapBufferedContext();

private:
	WglBitmapBufferedContext(const WglBitmapBufferedContext &);
	WglBitmapBufferedContext & operator=(const WglBitmapBufferedContext &);

public:
	/// swap buffers
	/*virtual*/ bool swapBuffer();
	/// resize the context
	/*virtual*/ bool resize(const int x1, const int y1, const int x2, const int y2);

	/// activate the context
	/*virtual*/ bool activate();
	/// de-activate the context
	/*virtual*/ bool deactivate();

	/// get the native context
	/*virtual*/ boost::any getNativeContext()  {  return isActivated() ? boost::any(&memDC_) : boost::any();  }
	/*virtual*/ const boost::any getNativeContext() const  {  return isActivated() ? boost::any(&memDC_) : boost::any();  }

    /// get the off-screen surface generated by the DIB section
	void * getOffScreen()  {  return dibBits_;  }
	const void * getOffScreen() const  {  return dibBits_;  }

protected :
	/// re-create an OpenGL display list
	/*virtual*/ bool doRecreateDisplayList()  {  return true;  }

private:
	bool createOffScreen();
	bool createOffScreenBitmap(const int colorBitCount, const int colorPlaneCount);
	void deleteOffScreen();

private:
	/// a window handle
	HWND hWnd_;
	/// a target context
	HDC hDC_;

	/// a buffered context
	HDC memDC_;
	/// a buffered bitmap
	HBITMAP memBmp_;
	/// an old bitmap
	HBITMAP oldBmp_;
    /// an off-screen surface generated by the DIB section
	void *dibBits_;
};

}  // namespace swl


#endif  // __SWL_WIN_VIEW__WGL_BITMAP_BUFFERED_CONTEXT__H_
