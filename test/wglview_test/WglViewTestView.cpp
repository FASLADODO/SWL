// WglViewTestView.cpp : implementation of the CWglViewTestView class
//

#include "stdafx.h"
#include "swl/Config.h"
#include "WglViewTest.h"

#include "WglViewTestDoc.h"
#include "WglViewTestView.h"

#include "ViewStateMachine.h"
#include "ViewEventHandler.h"
#include "swl/winview/WglDoubleBufferedContext.h"
#include "swl/winview/WglBitmapBufferedContext.h"
#include "swl/winview/WglPrintContext.h"
#include "swl/winview/WglViewPrintApi.h"
#include "swl/winview/WglViewCaptureApi.h"
#include "swl/oglview/OglCamera.h"
#include "swl/view/MouseEvent.h"
#include "swl/view/KeyEvent.h"
#include <boost/smart_ptr.hpp>
#include <boost/multi_array.hpp>
#include <GL/glut.h>
#include <limits>
#include <iostream>
#include <fstream>
#include <cassert>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if defined(max)
#undef max
#endif


namespace {

typedef boost::multi_array<float, 2> mesh_array_type;
boost::scoped_ptr<mesh_array_type> mesh;
int mesh_row = 0, mesh_col = 0;
float mesh_z_min = std::numeric_limits<float>::max(), mesh_z_max = 0.0f;
const float mesh_max_color_r = 1.0f, mesh_max_color_g = 1.0f, mesh_max_color_b = 0.0f;
const float mesh_min_color_r = 0.5f, mesh_min_color_g = 0.5f, mesh_min_color_b = 0.0f;

void loadMesh()
{
	const std::string filename("..\\data\\mesh.txt");

	std::ifstream stream(filename.c_str());
	stream >> mesh_row >> mesh_col;

	mesh.reset(new mesh_array_type(boost::extents[mesh_row][mesh_col]));

	float dat;
	for (int i = 0; i < mesh_row; ++i)
		for (int j = 0; j < mesh_col; ++j)
		{
			stream >> dat;
			(*mesh)[i][j] = dat;

			if (dat < mesh_z_min)
				mesh_z_min = dat;
			if (dat > mesh_z_max)
				mesh_z_max = dat;
		}
}

void calculateNormal(const float vx1, const float vy1, const float vz1, const float vx2, const float vy2, const float vz2, float &nx, float &ny, float &nz)
{
	nx = vy1 * vz2 - vz1 * vy2;
	ny = vz1 * vx2 - vx1 * vz2;
	nz = vx1 * vy2 - vy1 * vx2;

	const float norm = std::sqrt(nx*nx + ny*ny + nz*nz);
	nx /= norm;
	ny /= norm;
	nz /= norm;
}

void drawMesh()
{
	if (mesh.get())
	{
		const float factor = 50.0f;

		const GLfloat material_none[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		const GLfloat material_white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		const GLfloat shininess_none[] = { 0.0f };
		GLfloat material_diffuse[] = { 0.0f, 0.0f, 0.0f, 1.0f };

		float nx, ny, nz;
		float r, g, b;
		float ratio;

#if 0
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_none);

		float x1, y1, z1, x2, y2, z2, x3, y3, z3;
		glBegin(GL_TRIANGLES);
			for (int i = 0; i < mesh_row - 1; ++i)
				for (int j = 0; j < mesh_col - 1; ++j)
				{
					x1 = float(i) * factor;
					y1 = float(j) * factor;
					z1 = (*mesh)[i][j] * factor;
					x2 = float(i+1) * factor;
					y2 = float(j) * factor;
					z2 = (*mesh)[i+1][j] * factor;
					x3 = float(i) * factor;
					y3 = float(j+1) * factor;
					z3 = (*mesh)[i][j+1] * factor;
					calculateNormal(x2 - x1, y2 - y1, z2 - z1, x3 - x1, y3 - y1, z3 - z1, nx, ny, nz);

					//
					ratio = (z1 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x1, y1, z1);
					//
					ratio = (z2 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x2, y2, z2);
					//
					ratio = (z3 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x3, y3, z3);

					//
					x1 = float(i+1) * factor;
					y1 = float(j+1) * factor;
					z1 = (*mesh)[i+1][j+1] * factor;
					x2 = float(i) * factor;
					y2 = float(j+1) * factor;
					z2 = (*mesh)[i][j+1] * factor;
					x3 = float(i+1) * factor;
					y3 = float(j) * factor;
					z3 = (*mesh)[i+1][j] * factor;
					calculateNormal(x2 - x1, y2 - y1, z2 - z1, x3 - x1, y3 - y1, z3 - z1, nx, ny, nz);

					//
					ratio = (z1 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x1, y1, z1);
					//
					ratio = (z2 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x2, y2, z2);
					//
					ratio = (z3 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x3, y3, z3);
				}
		glEnd();
#elif 0
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_none);

		float x1, y1, z1, x2, y2, z2, x3, y3, z3;
		glBegin(GL_TRIANGLES);
			for (int i = 0; i < mesh_row - 1; ++i)
				for (int j = 0; j < mesh_col - 1; ++j)
				{
					x1 = float(i) * factor;
					y1 = float(j) * factor;
					z1 = (*mesh)[i][j] * factor;
					x2 = float(i+1) * factor;
					y2 = float(j) * factor;
					z2 = (*mesh)[i+1][j] * factor;
					x3 = float(i+1) * factor;
					y3 = float(j+1) * factor;
					z3 = (*mesh)[i+1][j+1] * factor;
					calculateNormal(x2 - x1, y2 - y1, z2 - z1, x3 - x1, y3 - y1, z3 - z1, nx, ny, nz);

					//
					ratio = (z1 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x1, y1, z1);
					//
					ratio = (z2 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x2, y2, z2);
					//
					ratio = (z3 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x3, y3, z3);

					//
					x1 = float(i) * factor;
					y1 = float(j) * factor;
					z1 = (*mesh)[i][j] * factor;
					x2 = float(i+1) * factor;
					y2 = float(j+1) * factor;
					z2 = (*mesh)[i+1][j+1] * factor;
					x3 = float(i) * factor;
					y3 = float(j+1) * factor;
					z3 = (*mesh)[i][j+1] * factor;
					calculateNormal(x2 - x1, y2 - y1, z2 - z1, x3 - x1, y3 - y1, z3 - z1, nx, ny, nz);

					//
					ratio = (z1 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x1, y1, z1);
					//
					ratio = (z2 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x2, y2, z2);
					//
					ratio = (z3 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x3, y3, z3);
				}
		glEnd();
#else
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess_none);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_none);

		float x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
		glBegin(GL_QUADS);
			for (int i = 0; i < mesh_row - 1; ++i)
				for (int j = 0; j < mesh_col - 1; ++j)
				{
					x1 = float(i) * factor;
					y1 = float(j) * factor;
					z1 = (*mesh)[i][j] * factor;
					x2 = float(i+1) * factor;
					y2 = float(j) * factor;
					z2 = (*mesh)[i+1][j] * factor;
					x3 = float(i+1) * factor;
					y3 = float(j+1) * factor;
					z3 = (*mesh)[i+1][j+1] * factor;
					x4 = float(i) * factor;
					y4 = float(j+1) * factor;
					z4 = (*mesh)[i][j+1] * factor;
					calculateNormal(x2 - x1, y2 - y1, z2 - z1, x3 - x1, y3 - y1, z3 - z1, nx, ny, nz);

					//
					ratio = (z1 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x1, y1, z1);
					//
					ratio = (z2 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x2, y2, z2);
					//
					ratio = (z3 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x3, y3, z3);
					//
					ratio = (z4 - mesh_z_min) / (mesh_z_max - mesh_z_min);
					r = mesh_min_color_r + (mesh_max_color_r - mesh_min_color_r) * ratio;
					g = mesh_min_color_g + (mesh_max_color_g - mesh_min_color_g) * ratio;
					b = mesh_min_color_b + (mesh_max_color_b - mesh_min_color_b) * ratio;
					glEdgeFlag(GL_TRUE);
					glColor3f(r, g, b);
					//material_diffuse[0] = r;  material_diffuse[1] = g;  material_diffuse[2] = b;
					//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
					glNormal3f(nx, ny, nz);
					glVertex3f(x4, y4, z4);
				}
		glEnd();
#endif
	}
}

}  // unnamed namespace

// CWglViewTestView

IMPLEMENT_DYNCREATE(CWglViewTestView, CView)

BEGIN_MESSAGE_MAP(CWglViewTestView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
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
	ON_COMMAND(ID_VIEWHANDLING_PAN, &CWglViewTestView::OnViewhandlingPan)
	ON_COMMAND(ID_VIEWHANDLING_ROTATE, &CWglViewTestView::OnViewhandlingRotate)
	ON_COMMAND(ID_VIEWHANDLING_ZOOMREGION, &CWglViewTestView::OnViewhandlingZoomregion)
	ON_COMMAND(ID_VIEWHANDLING_ZOOMALL, &CWglViewTestView::OnViewhandlingZoomall)
	ON_COMMAND(ID_VIEWHANDLING_ZOOMIN, &CWglViewTestView::OnViewhandlingZoomin)
	ON_COMMAND(ID_VIEWHANDLING_ZOOMOUT, &CWglViewTestView::OnViewhandlingZoomout)
	ON_UPDATE_COMMAND_UI(ID_VIEWHANDLING_PAN, &CWglViewTestView::OnUpdateViewhandlingPan)
	ON_UPDATE_COMMAND_UI(ID_VIEWHANDLING_ROTATE, &CWglViewTestView::OnUpdateViewhandlingRotate)
	ON_UPDATE_COMMAND_UI(ID_VIEWHANDLING_ZOOMREGION, &CWglViewTestView::OnUpdateViewhandlingZoomregion)
	ON_UPDATE_COMMAND_UI(ID_VIEWHANDLING_ZOOMALL, &CWglViewTestView::OnUpdateViewhandlingZoomall)
	ON_UPDATE_COMMAND_UI(ID_VIEWHANDLING_ZOOMIN, &CWglViewTestView::OnUpdateViewhandlingZoomin)
	ON_UPDATE_COMMAND_UI(ID_VIEWHANDLING_ZOOMOUT, &CWglViewTestView::OnUpdateViewhandlingZoomout)
	ON_COMMAND(ID_PRINTANDCAPTURE_PRINTVIEWUSINGGDI, &CWglViewTestView::OnPrintandcapturePrintviewusinggdi)
	ON_COMMAND(ID_PRINTANDCAPTURE_CAPTUREVIEWUSINGGDI, &CWglViewTestView::OnPrintandcaptureCaptureviewusinggdi)
	ON_COMMAND(ID_PRINTANDCAPTURE_CAPTUREVIEWUSINGGDIPLUS, &CWglViewTestView::OnPrintandcaptureCaptureviewusinggdiplus)
	ON_COMMAND(ID_PRINTANDCAPTURE_COPYTOCLIPBOARD, &CWglViewTestView::OnPrintandcaptureCopytoclipboard)
	ON_COMMAND(ID_EDIT_COPY, &CWglViewTestView::OnEditCopy)
END_MESSAGE_MAP()

// CWglViewTestView construction/destruction

CWglViewTestView::CWglViewTestView()
: viewStateFsm_()
{
	loadMesh();
}

CWglViewTestView::~CWglViewTestView()
{
}

BOOL CWglViewTestView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CWglViewTestView drawing

void CWglViewTestView::OnDraw(CDC* pDC)
{
	CWglViewTestDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: basic routine

	if (pDC && pDC->IsPrinting())
	{
		const boost::shared_ptr<camera_type> &camera = topCamera();
		if (!camera) return;

		const HCURSOR oldCursor = SetCursor(LoadCursor(0L, IDC_WAIT));
		const int oldMapMode = pDC->SetMapMode(MM_TEXT);

		const CRect rctPage(0, 0, pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES));

		swl::WglPrintContext printContext(pDC->GetSafeHdc(), rctPage);
		const std::auto_ptr<camera_type> printCamera(dynamic_cast<WglViewBase::camera_type *>(camera->cloneCamera()));
		if (printCamera.get() && printContext.isActivated())
		{
			initializeView();
			printCamera->setViewRegion(camera->getCurrentViewRegion());
			printCamera->setViewport(rctPage.left, rctPage.top, rctPage.right, rctPage.bottom);
			renderScene(printContext, *printCamera);
		}

		pDC->SetMapMode(oldMapMode);
		DeleteObject(SetCursor(oldCursor ? oldCursor : LoadCursor(0L, IDC_ARROW)));
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
				context.reset(new swl::WglDoubleBufferedContext(GetSafeHwnd(), rect));
			else if (2 == drawMode_)
				context.reset(new swl::WglBitmapBufferedContext(GetSafeHwnd(), rect));

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


// CWglViewTestView printing

BOOL CWglViewTestView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CWglViewTestView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CWglViewTestView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CWglViewTestView diagnostics

#ifdef _DEBUG
void CWglViewTestView::AssertValid() const
{
	CView::AssertValid();
}

void CWglViewTestView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWglViewTestDoc* CWglViewTestView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWglViewTestDoc)));
	return (CWglViewTestDoc*)m_pDocument;
}
#endif //_DEBUG


// CWglViewTestView message handlers

void CWglViewTestView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	//
	CRect rect;
	GetClientRect(&rect);

	drawMode_ = 1;  // [1, 2]
	useLocallyCreatedContext_ = false;

	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
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
	// This code is required for SWL.WglView: basic routine

	// create a context
	if (1 == drawMode_)
		pushContext(boost::shared_ptr<context_type>(new swl::WglDoubleBufferedContext(GetSafeHwnd(), rect, false)));
	else if (2 == drawMode_)
		pushContext(boost::shared_ptr<context_type>(new swl::WglBitmapBufferedContext(GetSafeHwnd(), rect, false)));

	// create a camera
	pushCamera(boost::shared_ptr<camera_type>(new swl::OglCamera()));

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
		// guard the context
		context_type::guard_type guard(*viewContext);

		// set the view
		initializeView();

		// set the camera
		if (viewCamera.get())
		{
			//viewCamera->setViewBound(-1600.0, -1100.0, 2400.0, 2900.0, 1.0, 20000.0);
			viewCamera->setViewBound(-1000.0, -1000.0, 1000.0, 1000.0, 4000.0, 12000.0);
			//viewCamera->setViewBound(-50.0, -50.0, 50.0, 50.0, 1.0, 2000.0);

			viewCamera->setViewport(0, 0, rect.Width(), rect.Height());
			viewCamera->setEyePosition(1000.0, 1000.0, 1000.0, false);
			viewCamera->setEyeDistance(8000.0, false);
			viewCamera->setObjectPosition(0.0, 0.0, 0.0);
			//viewCamera->setEyeDistance(1000.0, false);
			//viewCamera->setObjectPosition(110.0, 110.0, 150.0);

			viewCamera->setPerspective(true);
		}

		raiseDrawEvent(false);
	}

	// using a locally-created context
	if (useLocallyCreatedContext_)
		popContext();
}

void CWglViewTestView::OnDestroy()
{
	CView::OnDestroy();

	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: basic routine

	popContext();
	popCamera();
}

void CWglViewTestView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: basic routine

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
				//context_type::guard_type guard(*context);
				context->swapBuffer();
			}
			else raiseDrawEvent(true);
		}
	}

	// Do not call CView::OnPaint() for painting messages
}

void CWglViewTestView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: basic routine

	if (cx <= 0 || cy <= 0) return;
	resizeView(0, 0, cx, cy);
}

//-------------------------------------------------------------------------
// This code is required for SWL.WglView: basic routine

bool CWglViewTestView::raiseDrawEvent(const bool isContextActivated)
{
	if (isContextActivated)
	{
		const boost::shared_ptr<context_type> &context = topContext();
		if (!context.get() || context->isDrawing())
			return false;

		context_type::guard_type guard(*context);
		OnDraw(0L);
	}
	else OnDraw(0L);

	return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WglView: basic routine

bool CWglViewTestView::initializeView()
{
	// can we put this in the constructor?
	// specify black(0.0f, 0.0f, 0.0f, 0.0f) or white(1.0f, 1.0f, 1.0f, 1.0f) as clear color
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// specify the back of the buffer as clear depth
    glClearDepth(1.0f);
	// enable depth testing
    glEnable(GL_DEPTH_TEST);

	// lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	// create light components
	const GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	const GLfloat diffuseLight[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	const GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const GLfloat position0[] = { 0.2f, 0.2f, 1.0f, 0.0f };
	const GLfloat position1[] = { 0.2f, 0.2f, -1.0f, 0.0f };

	// assign created components to GL_LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);

	// polygon winding
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT_AND_BACK);

	// surface normal
	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);

	// shading model
	//glShadeModel(GL_FLAT);
	glShadeModel(GL_SMOOTH);

	// color tracking
	glEnable(GL_COLOR_MATERIAL);
	// set material properties which will be assigned by glColor
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WglView: basic routine

bool CWglViewTestView::resizeView(const int x1, const int y1, const int x2, const int y2)
{
	const boost::shared_ptr<context_type> &context = topContext();
	if (context.get() && context->resize(x1, y1, x2, y2))
	{
		context_type::guard_type guard(*context);
		initializeView();
		const boost::shared_ptr<camera_type> &camera = topCamera();
		if (camera.get()) camera->setViewport(x1, y1, x2, y2);
		raiseDrawEvent(false);

		return true;
	}
	else return false;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WglView: basic routine

bool CWglViewTestView::doPrepareRendering(const context_type &/*context*/, const camera_type &/*camera*/)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WglView: basic routine

bool CWglViewTestView::doRenderStockScene(const context_type &/*context*/, const camera_type &/*camera*/)
{
    return true;
}

//-------------------------------------------------------------------------
// This code is required for SWL.WglView: basic routine

bool CWglViewTestView::doRenderScene(const context_type &/*context*/, const camera_type &/*camera*/)
{
#if 1
	glPushMatrix();
		//glLoadIdentity();
		glTranslatef(-250.0f, 250.0f, -250.0f);
		glColor3f(1.0f, 0.0f, 0.0f);
		glutWireSphere(500.0, 20, 20);
		//glutSolidSphere(500.0, 20, 20);
	glPopMatrix();

	glPushMatrix();
		//glLoadIdentity();
		glTranslatef(250.0f, -250.0f, 250.0f);
		glColor3f(0.5f, 0.5f, 1.0f);
		//glutWireCube(500.0);
		glutSolidCube(500.0);
	glPopMatrix();
#endif

#if 0
	glPushMatrix();
		drawMesh();
	glPopMatrix();
#endif

    return true;
}

void CWglViewTestView::OnLButtonDown(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	SetCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));

	CView::OnLButtonDown(nFlags, point);
}

void CWglViewTestView::OnLButtonUp(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	ReleaseCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));

	CView::OnLButtonUp(nFlags, point);
}

void CWglViewTestView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_LEFT, ckey));

	CView::OnLButtonDblClk(nFlags, point);
}

void CWglViewTestView::OnMButtonDown(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	SetCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));

	CView::OnMButtonDown(nFlags, point);
}

void CWglViewTestView::OnMButtonUp(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	ReleaseCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));

	CView::OnMButtonUp(nFlags, point);
}

void CWglViewTestView::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_MIDDLE, ckey));

	CView::OnMButtonDblClk(nFlags, point);
}

void CWglViewTestView::OnRButtonDown(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	SetCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->pressMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));

	CView::OnRButtonDown(nFlags, point);
}

void CWglViewTestView::OnRButtonUp(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	ReleaseCapture();

	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));

	CView::OnRButtonUp(nFlags, point);
}

void CWglViewTestView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	const swl::MouseEvent::EControlKey ckey = (swl::MouseEvent::EControlKey)(
		((nFlags & MK_CONTROL) == MK_CONTROL ? swl::MouseEvent::CK_CTRL : swl::MouseEvent::CK_NONE) |
		((nFlags & MK_SHIFT) == MK_SHIFT ? swl::MouseEvent::CK_SHIFT : swl::MouseEvent::CK_NONE)
	);
	//viewController_.doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->doubleClickMouse(swl::MouseEvent(point.x, point.y, swl::MouseEvent::BT_RIGHT, ckey));

	CView::OnRButtonDblClk(nFlags, point);
}

void CWglViewTestView::OnMouseMove(UINT nFlags, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
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

BOOL CWglViewTestView::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
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

void CWglViewTestView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	//viewController_.pressKey(swl::KeyEvent(nChar, nRepCnt));
	if (viewStateFsm_.get()) viewStateFsm_->pressKey(swl::KeyEvent(nChar, nRepCnt));

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CWglViewTestView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	//viewController_.releaseKey(swl::KeyEvent(nChar, nRepCnt));
	if (viewStateFsm_.get()) viewStateFsm_->releaseKey(swl::KeyEvent(nChar, nRepCnt));

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CWglViewTestView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: event handling
	const swl::KeyEvent::EControlKey ckey = ((nFlags >> 28) & 0x01) == 0x01 ? swl::KeyEvent::CK_ALT : swl::KeyEvent::CK_NONE;
	//viewController_.releaseKey(swl::KeyEvent(nChar, nRepCnt, ckey));
	if (viewStateFsm_.get()) viewStateFsm_->releaseKey(swl::KeyEvent(nChar, nRepCnt, ckey));

	CView::OnChar(nChar, nRepCnt, nFlags);
}

void CWglViewTestView::OnViewhandlingPan()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtPan());
}

void CWglViewTestView::OnViewhandlingRotate()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtRotate());
}

void CWglViewTestView::OnViewhandlingZoomregion()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomRegion());
}

void CWglViewTestView::OnViewhandlingZoomall()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomAll());
}

void CWglViewTestView::OnViewhandlingZoomin()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomIn());
}

void CWglViewTestView::OnViewhandlingZoomout()
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WglView: view state
	if (viewStateFsm_.get()) viewStateFsm_->process_event(swl::EvtZoomOut());
}

void CWglViewTestView::OnUpdateViewhandlingPan(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::PanState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWglViewTestView::OnUpdateViewhandlingRotate(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::RotateState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWglViewTestView::OnUpdateViewhandlingZoomregion(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomRegionState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWglViewTestView::OnUpdateViewhandlingZoomall(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomAllState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWglViewTestView::OnUpdateViewhandlingZoomin(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomInState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWglViewTestView::OnUpdateViewhandlingZoomout(CCmdUI *pCmdUI)
{
	//-------------------------------------------------------------------------
	// This code is required for SWL.WinView: view state
	if (viewStateFsm_.get())
		pCmdUI->SetCheck(viewStateFsm_->state_cast<const swl::ZoomOutState *>() ? 1 : 0);
	else pCmdUI->SetCheck(0);
}

void CWglViewTestView::OnPrintandcapturePrintviewusinggdi()
{
	// initialize a PRINTDLG structure
	PRINTDLG pd;
	memset(&pd, 0, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = GetSafeHwnd();
	pd.Flags = PD_RETURNDC | PD_DISABLEPRINTTOFILE;
	pd.hInstance = NULL;
	if (!PrintDlg(&pd)) return;
	if (!pd.hDC) return;

	//
	const HCURSOR oldCursor = SetCursor(LoadCursor(0L, IDC_WAIT));

	// each logical unit is mapped to one device pixel. Positive x is to the right. positive y is down.
	SetMapMode(pd.hDC, MM_TEXT);

	DOCINFO di;
	di.cbSize = sizeof(DOCINFO);
	di.lpszDocName = _T("OpenGL Print");
	di.lpszOutput = NULL;

	// start the print job
	StartDoc(pd.hDC, &di);
	StartPage(pd.hDC);

	//
	if (!swl::printWglViewUsingGdi(*this, pd.hDC))
		AfxMessageBox(_T("fail to print a view"), MB_OK | MB_ICONSTOP);

	// end the print job
	EndPage(pd.hDC);
	EndDoc(pd.hDC);
	DeleteDC(pd.hDC);

	DeleteObject(SetCursor(oldCursor ? oldCursor : LoadCursor(0L, IDC_ARROW)));
}

void CWglViewTestView::OnPrintandcaptureCaptureviewusinggdi()
{
	CFileDialog dlg(FALSE, _T("bmp"), _T("*.bmp"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("BMP Files (*.bmp)|*.bmp||"), NULL);
	dlg.m_ofn.lpstrTitle = _T("Capture View As");
	if (dlg.DoModal() == IDOK)
	{
		const HCURSOR oldCursor = SetCursor(LoadCursor(0L, IDC_WAIT));

#if defined(_UNICODE) || defined(UNICODE)
		const std::wstring filePathName((wchar_t *)(LPCTSTR)dlg.GetPathName());
#else
		const std::string filePathName((char *)(LPCTSTR)dlg.GetPathName());
#endif
		if (!swl::captureWglViewUsingGdi(filePathName, *this, GetSafeHwnd()))
			AfxMessageBox(_T("fail to capture a view"), MB_OK | MB_ICONSTOP);

		DeleteObject(SetCursor(oldCursor ? oldCursor : LoadCursor(0L, IDC_ARROW)));
	}
}

void CWglViewTestView::OnPrintandcaptureCaptureviewusinggdiplus()
{
	// FIXME [add] >>
	AfxMessageBox(_T("not yet implemented"), MB_OK | MB_ICONSTOP);
}

void CWglViewTestView::OnPrintandcaptureCopytoclipboard()
{
	CClientDC dc(this);

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);

	CRect rect;
	GetWindowRect(&rect);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());

	CBitmap *oldBitmap = memDC.SelectObject(&bitmap);
	memDC.BitBlt(0, 0, rect.Width(), rect.Height(), &dc, 0, 0, SRCCOPY);

	// clipboard
	if (OpenClipboard())
	{
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, bitmap.GetSafeHandle());
		CloseClipboard();
	}

	memDC.SelectObject(oldBitmap);
	bitmap.Detach();
}

void CWglViewTestView::OnEditCopy()
{
	OnPrintandcaptureCopytoclipboard();
}
