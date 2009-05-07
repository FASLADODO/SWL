// WinViewTestView.cpp : implementation of the CWinViewTestView class
//

#include "stdafx.h"
#include "WinViewTest.h"

#include "WinViewTestDoc.h"
#include "WinViewTestView.h"

#include "ViewStateMachine.h"
#include "ViewEventHandler.h"
#include "swl/winview/GdiContext.h"
#include "swl/winview/GdiBitmapBufferedContext.h"
#include "swl/winview/GdiplusContext.h"
#include "swl/winview/GdiplusBitmapBufferedContext.h"
#include "swl/view/MouseEvent.h"
#include "swl/view/KeyEvent.h"
#include "swl/view/ViewCamera2.h"
#include "swl/winutil/WinTimer.h"
#include <gdiplus.h>
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWinViewTestView

IMPLEMENT_DYNCREATE(CWinViewTestView, CView)

BEGIN_MESSAGE_MAP(CWinViewTestView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_COMMAND(ID_VIEWSTATE_PAN, &CWinViewTestView::OnViewstatePan)
	ON_COMMAND(ID_VIEWSTATE_ROTATE, &CWinViewTestView::OnViewstateRotate)
	ON_COMMAND(ID_VIEWSTATE_ZOOMREGION, &CWinViewTestView::OnViewstateZoomregion)
	ON_COMMAND(ID_VIEWSTATE_ZOOMALL, &CWinViewTestView::OnViewstateZoomall)
	ON_COMMAND(ID_VIEWSTATE_ZOOMIN, &CWinViewTestView::OnViewstateZoomin)
	ON_COMMAND(ID_VIEWSTATE_ZOOMOUT, &CWinViewTestView::OnViewstateZoomout)
	ON_UPDATE_COMMAND_UI(ID_VIEWSTATE_PAN, &CWinViewTestView::OnUpdateViewstatePan)
	ON_UPDATE_COMMAND_UI(ID_VIEWSTATE_ROTATE, &CWinViewTestView::OnUpdateViewstateRotate)
	ON_UPDATE_COMMAND_UI(ID_VIEWSTATE_ZOOMREGION, &CWinViewTestView::OnUpdateViewstateZoomregion)
	ON_UPDATE_COMMAND_UI(ID_VIEWSTATE_ZOOMALL, &CWinViewTestView::OnUpdateViewstateZoomall)
	ON_UPDATE_COMMAND_UI(ID_VIEWSTATE_ZOOMIN, &CWinViewTestView::OnUpdateViewstateZoomin)
	ON_UPDATE_COMMAND_UI(ID_VIEWSTATE_ZOOMOUT, &CWinViewTestView::OnUpdateViewstateZoomout)
END_MESSAGE_MAP()

// CWinViewTestView construction/destruction

CWinViewTestView::CWinViewTestView()
: viewStateFsm_()
{
}

CWinViewTestView::~CWinViewTestView()
{
}

BOOL CWinViewTestView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CWinViewTestView drawing

void CWinViewTestView::OnDraw(CDC* pDC)
{
	CWinViewTestDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: basic routine

	if (pDC && pDC->IsPrinting())
	{
		// FIXME [add] >>
	}
	else
	{
		const boost::shared_ptr<camera_type> &camera = topCamera();
		if (!camera) return;

		// using a locally-created context
		if (useLocallyCreatedContext_)
		{
			CRect rect;
			GetClientRect(&rect);

			boost::scoped_ptr<context_type> context;
			if (1 == drawMode_)
				context.reset(new swl::GdiContext(GetSafeHwnd()));
			else if (2 == drawMode_)
				context.reset(new swl::GdiBitmapBufferedContext(GetSafeHwnd(), rect));
			else if (3 == drawMode_)
				context.reset(new swl::GdiplusContext(GetSafeHwnd()));
			else if (4 == drawMode_)
				context.reset(new swl::GdiplusBitmapBufferedContext(GetSafeHwnd(), rect));

			if (context.get() && context->isActivated())
			{
				initializeView();
				camera->setViewport(0, 0, rect.Width(), rect.Height());
				renderScene(*context, *camera);
			}
		}
		else
		{
			const boost::shared_ptr<context_type> &context = topContext();
			if (context.get() && context->isActivated())
				renderScene(*context, *camera);
		}
	}
}


// CWinViewTestView printing

BOOL CWinViewTestView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CWinViewTestView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CWinViewTestView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CWinViewTestView diagnostics

#ifdef _DEBUG
void CWinViewTestView::AssertValid() const
{
	CView::AssertValid();
}

void CWinViewTestView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWinViewTestDoc* CWinViewTestView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWinViewTestDoc)));
	return (CWinViewTestDoc*)m_pDocument;
}
#endif //_DEBUG


// CWinViewTestView message handlers

void CWinViewTestView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	//
	idx_ = 0;
	timeInterval_ = 50;
	SetTimer(1, timeInterval_, NULL);

	for (int i = 0; i < 5000; ++i)
	{
		const double x = (double)i * timeInterval_ / 10000.0;
		const double y = std::sin(x) * 100.0 + 100.0;
		data1_.push_back(std::make_pair(i, (int)std::floor(y + 0.5)));
	}

	drawMode_ = 4;  // [1, 4]
	useLocallyCreatedContext_ = false;

	CRect rect;
	GetClientRect(&rect);

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
/*
	viewController_.addMousePressHandler(swl::MousePressHandler());
	viewController_.addMouseReleaseHandler(swl::MouseReleaseHandler());
	viewController_.addMouseMoveHandler(swl::MouseMoveHandler());
	viewController_.addMouseWheelHandler(swl::MouseWheelHandler());
	viewController_.addMouseClickHandler(swl::MouseClickHandler());
	viewController_.addMouseDoubleClickHandler(swl::MouseDoubleClickHandler());
	viewController_.addKeyPressHandler(swl::KeyPressHandler());
	viewController_.addKeyReleaseHandler(swl::KeyReleaseHandler());
	viewController_.addKeyHitHandler(swl::KeyHitHandler());
*/
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: basic routine
	
	// create a context
	if (1 == drawMode_)
		pushContext(boost::shared_ptr<context_type>(new swl::GdiContext(GetSafeHwnd(), false)));
	else if (2 == drawMode_)
		pushContext(boost::shared_ptr<context_type>(new swl::GdiBitmapBufferedContext(GetSafeHwnd(), rect, false)));
	else if (3 == drawMode_)
		pushContext(boost::shared_ptr<context_type>(new swl::GdiplusContext(GetSafeHwnd(), false)));
	else if (4 == drawMode_)
		pushContext(boost::shared_ptr<context_type>(new swl::GdiplusBitmapBufferedContext(GetSafeHwnd(), rect, false)));

	// create a camera
	pushCamera(boost::shared_ptr<camera_type>(new swl::ViewCamera2()));

	const boost::shared_ptr<context_type> &viewContext = topContext();
	const boost::shared_ptr<camera_type> &viewCamera = topCamera();

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state

	if (!useLocallyCreatedContext_ && NULL == viewStateFsm_.get() && viewContext.get() && viewCamera.get())
	{
		viewStateFsm_.reset(new swl::ViewStateMachine(*this, *viewContext, *viewCamera));
		if (viewStateFsm_.get()) viewStateFsm_->initiate();
	}

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: basic routine

	// initialize a view
	if (viewContext.get())
	{
		// activate the context
		viewContext->activate();

		// set the view
		initializeView();

		// set the camera
		if (viewCamera.get())
		{
			// TODO [check] >>
			viewCamera->setViewBound(rect.left - 100, rect.top - 100, rect.right + 100, rect.bottom + 100);
			viewCamera->setViewport(0, 0, rect.Width(), rect.Height());
		}

		raiseDrawEvent(false);

		// de-activate the context
		viewContext->deactivate();
	}

	// using a locally-created context
	if (useLocallyCreatedContext_)
		popContext();
}

void CWinViewTestView::OnDestroy()
{
	CView::OnDestroy();

	popContext();
	popCamera();
}

void CWinViewTestView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: basic routine

	// using a locally-created context
	if (useLocallyCreatedContext_)
		raiseDrawEvent(false);
	else
	{
		const boost::shared_ptr<context_type> &context = topContext();
		if (context.get())
		{
			if (context->isOffScreenUsed())
			{
				//context->activate();
				context->swapBuffer();
				//context->deactivate();
			}
			else raiseDrawEvent(true);
		}
	}

	// Do not call CView::OnPaint() for painting messages
}

void CWinViewTestView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: basic routine

	if (cx <= 0 || cy <= 0) return;
	resizeView(0, 0, cx, cy);
}

void CWinViewTestView::OnTimer(UINT_PTR nIDEvent)
{
	const double x = (double)idx_ * timeInterval_ / 1000.0;
	const double y = std::cos(x) * 100.0 + 100.0;
	data2_.push_back(std::make_pair(idx_, (int)std::floor(y + 0.5)));

	++idx_;

	raiseDrawEvent(useLocallyCreatedContext_ ? false : true);

	CView::OnTimer(nIDEvent);
}

//-------------------------------------------------------------------------
// This code is required for SWL.WinView: basic routine

bool CWinViewTestView::raiseDrawEvent(const bool isContextActivated)
{
	if (isContextActivated)
	{
		const boost::shared_ptr<context_type> &context = topContext();
		if (!context.get() || context->isDrawing())
			return false;

		context->activate();
		OnDraw(0L);
		context->deactivate();
	}
	else OnDraw(0L);

	return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WinView: basic routine

bool CWinViewTestView::initializeView()
{
	return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WinView: basic routine

bool CWinViewTestView::resizeView(const int x1, const int y1, const int x2, const int y2)
{
	const boost::shared_ptr<context_type> &viewContext = topContext();
	if (viewContext.get() && viewContext->resize(x1, y1, x2, y2))
	{
		viewContext->activate();
		initializeView();
		const boost::shared_ptr<camera_type> &viewCamera = topCamera();
		if (viewCamera.get()) viewCamera->setViewport(x1, y1, x2, y2);	
		raiseDrawEvent(false);
		viewContext->deactivate();

		return true;
	}
	else return false;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WinView: basic routine

bool CWinViewTestView::doPrepareRendering(const context_type &context, const camera_type &camera)
{

    return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WinView: basic routine

bool CWinViewTestView::doRenderStockScene(const context_type &context, const camera_type &camera)
{
    return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WinView: basic routine

bool CWinViewTestView::doRenderScene(const context_type &context, const camera_type &camera)
{
	CRect rect;
	GetClientRect(&rect);

	const int lineWidth1 = 6;
	const int lineWidth2 = 4;
	const int lineWidth3 = 2;

	try
	{
		const HDC *dc = boost::any_cast<const HDC *>(context.getNativeContext());
		if (dc)
		{
			CDC *pDC = CDC::FromHandle(*dc);

			// clear the background
			//pDC->SetBkColor(RGB(192, 192, 0));  // not working ???
			pDC->FillRect(rect, &CBrush(RGB(240, 240, 240)));

			// draw contents
			int vx, vy;

			{
				CPen pen(PS_SOLID, lineWidth1, RGB(255, 0, 255));
				CPen *oldPen = pDC->SelectObject(&pen);
				const swl::Region2<double> bound = camera.getViewBound();
				int vx0, vy0;
				camera.mapCanvasToWindow(bound.left, bound.bottom, vx0, vy0);
				camera.mapCanvasToWindow(bound.right, bound.top, vx, vy);
				pDC->Rectangle(vx0, vy0, vx, vy);
				pDC->SelectObject(oldPen);
			}

			{
				CPen pen(PS_SOLID, lineWidth1, RGB(255, 0, 0));
				CPen *oldPen = pDC->SelectObject(&pen);
				camera.mapCanvasToWindow(100, 100, vx, vy);
				pDC->MoveTo(vx, vy);
				camera.mapCanvasToWindow(300, 300, vx, vy);
				pDC->LineTo(vx, vy);
				pDC->SelectObject(oldPen);
			}

			if (data1_.size() > 1)
			{
				CPen pen(PS_SOLID, lineWidth2, RGB(0, 255, 0));
				CPen *oldPen = pDC->SelectObject(&pen);
				data_type::iterator it = data1_.begin();
				camera.mapCanvasToWindow(it->first, it->second, vx, vy);
				pDC->MoveTo(vx, vy);
				for (++it; it != data1_.end(); ++it)
				{
					camera.mapCanvasToWindow(it->first, it->second, vx, vy);
					pDC->LineTo(vx, vy);
				}
				pDC->SelectObject(oldPen);
			}

			if (data2_.size() > 1)
			{
				CPen pen(PS_SOLID, lineWidth3, RGB(0, 0, 255));
				CPen *oldPen = pDC->SelectObject(&pen);
				data_type::iterator it = data2_.begin();
				camera.mapCanvasToWindow(it->first, it->second, vx, vy);
				pDC->MoveTo(vx, vy);
				for (++it; it != data2_.end(); ++it)
				{
					camera.mapCanvasToWindow(it->first, it->second, vx, vy);
					pDC->LineTo(vx, vy);
				}
				pDC->SelectObject(oldPen);
			}
		}
	}
	catch (const boost::bad_any_cast &)
	{
	}

	try
	{
		Gdiplus::Graphics *graphics = boost::any_cast<Gdiplus::Graphics *>(context.getNativeContext());
		if (graphics)
		{
			// clear the background
			graphics->Clear(Gdiplus::Color(255, 240, 240, 240));

			// draw contents
			int vx1, vy1, vx2, vy2;

			{
				Gdiplus::Pen pen(Gdiplus::Color(255, 255, 0, 255), lineWidth1);
				const swl::Region2<double> bound = camera.getViewBound();
				camera.mapCanvasToWindow(bound.left, bound.bottom, vx1, vy1);
				camera.mapCanvasToWindow(bound.right, bound.top, vx2, vy2);
				graphics->DrawRectangle(&pen, vx1, vy1, vx2 - vx1, vy2 - vy1);
			}

			{
				Gdiplus::Pen pen(Gdiplus::Color(255, 255, 0, 0), lineWidth1);
				camera.mapCanvasToWindow(100, 300, vx1, vy1);
				camera.mapCanvasToWindow(300, 500, vx2, vy2);
				graphics->DrawLine(&pen, vx1, vy1, vx2, vy2);
			}

			if (data1_.size() > 1)
			{
				Gdiplus::Pen pen(Gdiplus::Color(255, 0, 255, 0), lineWidth2);
				data_type::iterator prevIt = data1_.begin();
				data_type::iterator it = data1_.begin();
				for (++it; it != data1_.end(); ++prevIt, ++it)
				{
					camera.mapCanvasToWindow(prevIt->first, prevIt->second, vx1, vy1);
					camera.mapCanvasToWindow(it->first, it->second, vx2, vy2);
					graphics->DrawLine(&pen, vx1, vy1, vx2, vy2);
				}
			}

			if (data2_.size() > 1)
			{
				Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255), lineWidth3);
				data_type::iterator prevIt = data2_.begin();
				data_type::iterator it = data2_.begin();
				for (++it; it != data2_.end(); ++prevIt, ++it)
				{
					camera.mapCanvasToWindow(prevIt->first, prevIt->second, vx1, vy1);
					camera.mapCanvasToWindow(it->first, it->second, vx2, vy2);
					graphics->DrawLine(&pen, vx1, vy1, vx2, vy2);
				}
			}
		}
	}
	catch (const boost::bad_any_cast &)
	{
	}

    return true;
}

void CWinViewTestView::OnLButtonDown(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	SetCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));

	CView::OnLButtonDown(nFlags, point);
}

void CWinViewTestView::OnLButtonUp(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	ReleaseCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));

	CView::OnLButtonUp(nFlags, point);
}

void CWinViewTestView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));

	CView::OnLButtonDblClk(nFlags, point);
}

void CWinViewTestView::OnMButtonDown(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	SetCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));

	CView::OnMButtonDown(nFlags, point);
}

void CWinViewTestView::OnMButtonUp(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	ReleaseCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));

	CView::OnMButtonUp(nFlags, point);
}

void CWinViewTestView::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));

	CView::OnMButtonDblClk(nFlags, point);
}

void CWinViewTestView::OnRButtonDown(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	SetCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));

	CView::OnRButtonDown(nFlags, point);
}

void CWinViewTestView::OnRButtonUp(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	ReleaseCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));

	CView::OnRButtonUp(nFlags, point);
}

void CWinViewTestView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));

	CView::OnRButtonDblClk(nFlags, point);
}

void CWinViewTestView::OnMouseMove(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	const swl::MouseEvent::EButton btn = (swl::MouseEvent::EButton)(
		((nFlags & MK_LBUTTON) == MK_LBUTTON ? swl::MouseEvent::BT_LEFT : swl::MouseEvent::BT_NONE) |
		((nFlags & MK_MBUTTON) == MK_MBUTTON ? swl::MouseEvent::BT_MIDDLE : swl::MouseEvent::BT_NONE) |
		((nFlags & MK_RBUTTON) == MK_RBUTTON ? swl::MouseEvent::BT_RIGHT : swl::MouseEvent::BT_NONE)
	);
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.moveMouse(swl::MouseEvent(point.x, point.y, btn, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->moveMouse(swl::MouseEvent(point.x, point.y, btn, ckey));

	CView::OnMouseMove(nFlags, point);
}

BOOL CWinViewTestView::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	const swl::MouseEvent::EButton btn = (swl::MouseEvent::EButton)(
		((nFlags & MK_LBUTTON) == MK_LBUTTON ? swl::MouseEvent::BT_LEFT : swl::MouseEvent::BT_NONE) |
		((nFlags & MK_MBUTTON) == MK_MBUTTON ? swl::MouseEvent::BT_MIDDLE : swl::MouseEvent::BT_NONE) |
		((nFlags & MK_RBUTTON) == MK_RBUTTON ? swl::MouseEvent::BT_RIGHT : swl::MouseEvent::BT_NONE)
	);
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.wheelMouse(swl::MouseEvent(point.x, point.y, btn, ckey, swl::MouseEvent::SC_VERTICAL, zDelta / WHEEL_DELTA));
	if (viewStateFsm_.get()) viewStateFsm_->wheelMouse(swl::MouseEvent(point.x, point.y, btn, ckey, swl::MouseEvent::SC_VERTICAL, zDelta / WHEEL_DELTA));

	return CView::OnMouseWheel(nFlags, zDelta, point);
}

void CWinViewTestView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	//viewController_.pressKey(swl::KeyEvent(nChar, nRepCnt));
	if (viewStateFsm_.get()) viewStateFsm_->pressKey(swl::KeyEvent(nChar, nRepCnt));

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CWinViewTestView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	//viewController_.releaseKey(swl::KeyEvent(nChar, nRepCnt));
	if (viewStateFsm_.get()) viewStateFsm_->releaseKey(swl::KeyEvent(nChar, nRepCnt));

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CWinViewTestView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: event handling
	// TODO [check] >>
	const swl::KeyEvent::EControlKey ckey = ((nFlags >> 28) & 0x01) == 0x01 ? swl::KeyEvent::CK_ALT : swl::KeyEvent::CK_NONE;
	//viewController_.releaseKey(swl::KeyEvent(nChar, nRepCnt, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseKey(swl::KeyEvent(nChar, nRepCnt, ckey));

	CView::OnChar(nChar, nRepCnt, nFlags);
}

void CWinViewTestView::OnViewstatePan()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtPan());
}

void CWinViewTestView::OnViewstateRotate()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtRotate());
}

void CWinViewTestView::OnViewstateZoomregion()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomRegion());
}

void CWinViewTestView::OnViewstateZoomall()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomAll());
}

void CWinViewTestView::OnViewstateZoomin()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomIn());
}

void CWinViewTestView::OnViewstateZoomout()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomOut());
}

void CWinViewTestView::OnUpdateViewstatePan(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::PanState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWinViewTestView::OnUpdateViewstateRotate(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	pCmdUI->Enable(FALSE);

	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::RotateState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWinViewTestView::OnUpdateViewstateZoomregion(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomRegionState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWinViewTestView::OnUpdateViewstateZoomall(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomAllState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWinViewTestView::OnUpdateViewstateZoomin(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomInState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWinViewTestView::OnUpdateViewstateZoomout(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomOutState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}
