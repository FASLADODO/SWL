
// swl_gps_aided_imu_filter_appDlg.cpp : implementation file
//

#include "stdafx.h"
#include "swl_gps_aided_imu_filter_app.h"
#include "swl_gps_aided_imu_filter_appDlg.h"
#include <fstream>
#include <cstdlib>
#include <ctime>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if defined(max)
#undef max
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cswl_gps_aided_imu_filter_appDlg dialog




Cswl_gps_aided_imu_filter_appDlg::Cswl_gps_aided_imu_filter_appDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cswl_gps_aided_imu_filter_appDlg::IDD, pParent),
#if defined(_UNICODE) || defined(UNICODE)
	  gpsPortName_(L"COM8"),
#else
	  gpsPortName_("COM8"),
#endif
	  gpsBaudRate_(9600),
	  initialGravity_(0.0, 0.0, 0.0), initialAngularVel_(0.0, 0.0, 0.0), initialGpsECEF_(0.0, 0.0, 0.0), initialGpsSpeed_(0.0),
	  prevGpsUtc_(0, 0, 0, 0), prevGpsECEF_(0.0, 0.0, 0.0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cswl_gps_aided_imu_filter_appDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cswl_gps_aided_imu_filter_appDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CHECK_IMU, &Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonCheckImu)
	ON_BN_CLICKED(IDC_BUTTON_CHECK_GPS, &Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonCheckGps)
	ON_BN_CLICKED(IDC_BUTTON_RUN_FILTER, &Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonRunFilter)
	ON_BN_CLICKED(IDC_BUTTON_INITIALIZE_IMU_AND_GPS, &Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonInitializeImuAndGps)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_RAW_DATA, &Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonSaveRawData)
END_MESSAGE_MAP()


// Cswl_gps_aided_imu_filter_appDlg message handlers

BOOL Cswl_gps_aided_imu_filter_appDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	std::srand((unsigned int)time(NULL));

	isSensorsInitialized_ = false;

	prevPerformanceCount_.LowPart = 0;
	prevPerformanceCount_.HighPart = 0;
	prevGpsUtc_.hour = prevGpsUtc_.min = prevGpsUtc_.sec = prevGpsUtc_.msec = 0;

	GetDlgItem(IDC_BUTTON_CHECK_IMU)->SetWindowText(_T("Start IMU"));
	GetDlgItem(IDC_BUTTON_CHECK_GPS)->SetWindowText(_T("Start GPS"));
	GetDlgItem(IDC_BUTTON_RUN_FILTER)->SetWindowText(_T("Start GPS-aided IMU Filter"));
	GetDlgItem(IDC_BUTTON_SAVE_RAW_DATA)->SetWindowText(_T("Start Saving Raw Data"));
	GetDlgItem(IDC_BUTTON_CHECK_IMU)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_CHECK_GPS)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RUN_FILTER)->EnableWindow(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Cswl_gps_aided_imu_filter_appDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Cswl_gps_aided_imu_filter_appDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Cswl_gps_aided_imu_filter_appDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonInitializeImuAndGps()
{
	isSensorsInitialized_ = false;
	GetDlgItem(IDC_BUTTON_CHECK_IMU)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_CHECK_GPS)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RUN_FILTER)->EnableWindow(FALSE);

	CString msg;

	if (initializeSensors())
	{
		msg.Format(_T("%f"), initialGravity_.x);
		GetDlgItem(IDC_EDIT_IMU_ACCEL_X)->SetWindowText(msg);
		msg.Format(_T("%f"), initialGravity_.y);
		GetDlgItem(IDC_EDIT_IMU_ACCEL_Y)->SetWindowText(msg);
		msg.Format(_T("%f"), initialGravity_.z);
		GetDlgItem(IDC_EDIT_IMU_ACCEL_Z)->SetWindowText(msg);
		msg.Format(_T("%f"), initialAngularVel_.x);
		GetDlgItem(IDC_EDIT_IMU_GYRO_X)->SetWindowText(msg);
		msg.Format(_T("%f"), initialAngularVel_.y);
		GetDlgItem(IDC_EDIT_IMU_GYRO_Y)->SetWindowText(msg);
		msg.Format(_T("%f"), initialAngularVel_.z);
		GetDlgItem(IDC_EDIT_IMU_GYRO_Z)->SetWindowText(msg);
		GetDlgItem(IDC_EDIT_IMU_ELAPSED_TIME)->SetWindowText(_T(""));

		GetDlgItem(IDC_EDIT_GPS_LAT)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_LON)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_ALT)->SetWindowText(_T(""));
		msg.Format(_T("%f"), initialGpsECEF_.x);
		GetDlgItem(IDC_EDIT_GPS_X)->SetWindowText(msg);
		msg.Format(_T("%f"), initialGpsECEF_.y);
		GetDlgItem(IDC_EDIT_GPS_Y)->SetWindowText(msg);
		msg.Format(_T("%f"), initialGpsECEF_.z);
		GetDlgItem(IDC_EDIT_GPS_Z)->SetWindowText(msg);
		GetDlgItem(IDC_EDIT_GPS_VEL_X)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_VEL_Y)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_VEL_Z)->SetWindowText(_T(""));
		msg.Format(_T("%f"), initialGpsSpeed_.val);
		GetDlgItem(IDC_EDIT_GPS_SPEED)->SetWindowText(msg);
		GetDlgItem(IDC_EDIT_GPS_ELAPSED_TIME)->SetWindowText(_T(""));

		//
		isSensorsInitialized_ = true;

		GetDlgItem(IDC_BUTTON_CHECK_IMU)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_CHECK_GPS)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RUN_FILTER)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_EDIT_IMU_ACCEL_X)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_IMU_ACCEL_Y)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_IMU_ACCEL_Z)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_IMU_GYRO_X)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_IMU_GYRO_Y)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_IMU_GYRO_Z)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_IMU_ELAPSED_TIME)->SetWindowText(_T(""));

		GetDlgItem(IDC_EDIT_GPS_LAT)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_LON)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_ALT)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_X)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_Y)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_Z)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_VEL_X)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_VEL_Y)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_VEL_Z)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_SPEED)->SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_GPS_ELAPSED_TIME)->SetWindowText(_T(""));

		AfxMessageBox(_T("fail to initialize an IMU & a GPS"), MB_ICONERROR | MB_OK);
	}
}

void Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonSaveRawData()
{
	static bool toggle = true;

	if (toggle)
	{
		// initialize ADIS16350
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize ADIS16350"));
		try
		{
			imu_.reset(new swl::Adis16350Interface());
			if (!imu_)
			{
				AfxMessageBox(_T("fail to create an IMU"), MB_ICONERROR | MB_OK);
				return;
			}
		}
		catch (const std::runtime_error &e)
		{
			AfxMessageBox(CString(_T("fail to create an IMU: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
			return;
		}
		// initialize GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize GPS"));
		try
		{
			gps_.reset(new swl::GpsInterface(gpsPortName_, gpsBaudRate_));
			if (!gps_)
			{
				AfxMessageBox(_T("fail to create a GPS"), MB_ICONERROR | MB_OK);
				return;
			}
		}
		catch (const std::runtime_error &e)
		{
			AfxMessageBox(CString(_T("fail to create an GPS: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
			return;
		}

		// connect GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("connect GPS"));
		if (!gps_->isConnected())
		{
			AfxMessageBox(_T("fail to connect a GPS"), MB_ICONERROR | MB_OK);
			return;
		}

		freq_.LowPart = 0;
		freq_.HighPart = 0;
		QueryPerformanceFrequency(&freq_);
		prevPerformanceCount_.LowPart = 0;
		prevPerformanceCount_.HighPart = 0;
		QueryPerformanceCounter(&prevPerformanceCount_);
		prevGpsUtc_.hour = prevGpsUtc_.min = prevGpsUtc_.sec = prevGpsUtc_.msec = 0;
		prevGpsECEF_.x = prevGpsECEF_.y = prevGpsECEF_.z = 0.0;

		imuTimeStamps_.clear();
		imuAccels_.clear();
		imuGyros_.clear();
		gpsTimeStamps_.clear();
		gpsGeodetics_.clear();
		gpsSpeeds_.clear();

		step_ = 0;

		//
		GetDlgItem(IDC_BUTTON_SAVE_RAW_DATA)->SetWindowText(_T("Stop Saving Raw Data"));
		SetTimer(SAVER_TIMER_ID, SAVER_SAMPLING_INTERVAL, NULL);

		toggle = false;
	}
	else
	{
		GetDlgItem(IDC_BUTTON_SAVE_RAW_DATA)->SetWindowText(_T("Start Saving Raw Data"));
		KillTimer(SAVER_TIMER_ID);

		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate ADIS16350 & GPS"));
		imu_.reset();
		gps_.reset();

		//
		const std::string raw_data_filename("..\\data\\adis16350_data_20100930\\mesaured_raw_data.txt");
		std::ofstream stream(raw_data_filename.c_str());
		if (stream)
		{
			if (imuTimeStamps_.size() != step_ || imuAccels_.size() != step_ || imuGyros_.size() != step_ ||
				gpsTimeStamps_.size() != step_ || gpsGeodetics_.size() != step_ || gpsSpeeds_.size() != step_)
			{
				AfxMessageBox(_T("the sizes of measured raw datasets are not matched"), MB_ICONERROR | MB_OK);
				return;
			}

			std::list<__int64>::iterator itImuTimeStamp = imuTimeStamps_.begin();
			std::list<swl::ImuData::Accel>::iterator itImuAccel = imuAccels_.begin();
			std::list<swl::ImuData::Gyro>::iterator itImuGyro = imuGyros_.begin();
			std::list<long>::iterator itGpsTimeStamp = gpsTimeStamps_.begin();
			std::list<swl::EarthData::Geodetic>::iterator itGpsGeodetic = gpsGeodetics_.begin();
			std::list<swl::EarthData::Speed>::iterator itGpsSpeed = gpsSpeeds_.begin();

			for (size_t i = 0; i < step_; ++i)
			{
				stream << *itImuTimeStamp << '\t' << itImuAccel->x << '\t' << itImuAccel->y << '\t' << itImuAccel->z << '\t' << itImuGyro->x << '\t' << itImuGyro->y << '\t' << itImuGyro->z << '\t'
					<< *itGpsTimeStamp << '\t' << itGpsGeodetic->lat << '\t' << itGpsGeodetic->lon << '\t' << itGpsGeodetic->alt << '\t' << itGpsSpeed->val << std::endl;

				++itImuTimeStamp;
				++itImuAccel;
				++itImuGyro;
				++itGpsTimeStamp;
				++itGpsGeodetic;
				++itGpsSpeed;
			}

			stream.flush();
			stream.close();

			imuTimeStamps_.clear();
			imuAccels_.clear();
			imuGyros_.clear();
			gpsTimeStamps_.clear();
			gpsGeodetics_.clear();
			gpsSpeeds_.clear();
		}

		toggle = true;
	}
}

void Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonCheckImu()
{
	static bool toggle = true;

	if (toggle)
	{
		const size_t Ninitial = 1000;

		// initialize ADIS16350
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize ADIS16350"));
		try
		{
			imu_.reset(new swl::Adis16350Interface());
			if (!imu_)
			{
				AfxMessageBox(_T("fail to create an IMU"), MB_ICONERROR | MB_OK);
				return;
			}
		}
		catch (const std::runtime_error &e)
		{
			AfxMessageBox(CString(_T("fail to create an IMU: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
			return;
		}

		// load calibration parameters
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("load calibration parameters of ADIS16350"));
		{
			const std::string calibration_param_filename("..\\data\\adis16350_data_20100801\\imu_calibration_result.txt");
			if (!imu_->loadCalibrationParam(calibration_param_filename))
			{
				AfxMessageBox(_T("fail to load a IMU's calibration parameters"), MB_ICONERROR | MB_OK);
				return;
			}
		}

#if 0
		// set the initial local gravity & the initial Earth's angular velocity
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("set the initial local gravity & the initial Earth's angular velocity"));
		initialGravity_.x = initialGravity_.y = initialGravity_.z = 0.0;
		initialAngularVel_.x = initialAngularVel_.y = initialAngularVel_.z = 0.0;
		if (!imu_->setInitialAttitude(Ninitial, initialGravity_, initialAngularVel_))
		{
			AfxMessageBox(_T("fail to set the initial local gravity & the initial Earth's angular velocity"), MB_ICONERROR | MB_OK);
			return;
		}
#else
		if (!isSensorsInitialized_)
		{
			AfxMessageBox(_T("fail to set the initial local gravity & the initial Earth's angular velocity"), MB_ICONERROR | MB_OK);
			return;
		}
#endif

		freq_.LowPart = 0;
		freq_.HighPart = 0;
		QueryPerformanceFrequency(&freq_);
		prevPerformanceCount_.LowPart = 0;
		prevPerformanceCount_.HighPart = 0;
		QueryPerformanceCounter(&prevPerformanceCount_);

		step_ = 0;

		//
		GetDlgItem(IDC_BUTTON_CHECK_IMU)->SetWindowText(_T("Stop IMU"));
		SetTimer(IMU_TIMER_ID, IMU_SAMPLING_INTERVAL, NULL);

		toggle = false;
	}
	else
	{
		GetDlgItem(IDC_BUTTON_CHECK_IMU)->SetWindowText(_T("Start IMU"));
		KillTimer(IMU_TIMER_ID);

		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate ADIS16350"));
		imu_.reset();

		toggle = true;
	}
}

void Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonCheckGps()
{
	static bool toggle = true;

	if (toggle)
	{
		const size_t Ninitial = 100;

		// initialize GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize GPS"));
		try
		{
			gps_.reset(new swl::GpsInterface(gpsPortName_, gpsBaudRate_));
			if (!gps_)
			{
				AfxMessageBox(_T("fail to create a GPS"), MB_ICONERROR | MB_OK);
				return;
			}
		}
		catch (const std::runtime_error &e)
		{
			AfxMessageBox(CString(_T("fail to create an GPS: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
			return;
		}

		// connect GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("connect GPS"));
		if (!gps_->isConnected())
		{
			AfxMessageBox(_T("fail to connect a GPS"), MB_ICONERROR | MB_OK);
			return;
		}

#if 0
		// set the initial position and speed of GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("set the initial position and speed of GPS"));
		if (!gps_->setInitialState(Ninitial, initialGpsECEF_, initialGpsSpeed_))
		{
			AfxMessageBox(_T("fail to set the initial position & speed of the GPS"), MB_ICONERROR | MB_OK);
			return;
		}
#else
		if (!isSensorsInitialized_)
		{
			AfxMessageBox(_T("fail to set the initial position & speed of the GPS"), MB_ICONERROR | MB_OK);
			return;
		}
#endif

		// FIXME [check] >>
		prevGpsUtc_.hour = prevGpsUtc_.min = prevGpsUtc_.sec = prevGpsUtc_.msec = 0;
		prevGpsECEF_.x = prevGpsECEF_.y = prevGpsECEF_.z = 0.0;

		step_ = 0;

		//
		GetDlgItem(IDC_BUTTON_CHECK_GPS)->SetWindowText(_T("Stop GPS"));
		SetTimer(GPS_TIMER_ID, GPS_SAMPLING_INTERVAL, NULL);

		toggle = false;
	}
	else
	{
		GetDlgItem(IDC_BUTTON_CHECK_GPS)->SetWindowText(_T("Start GPS"));
		KillTimer(GPS_TIMER_ID);

		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate GPS"));
		gps_.reset();

		toggle = true;
	}
}

void Cswl_gps_aided_imu_filter_appDlg::OnBnClickedButtonRunFilter()
{
	static bool toggle = true;

	if (toggle)
	{
		const size_t Nimu = 10000;
		const size_t Ngps = 100;

		// initialize ADIS16350
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize ADIS16350"));
		try
		{
			imu_.reset(new swl::Adis16350Interface());
			if (!imu_)
			{
				AfxMessageBox(_T("fail to create an IMU"), MB_ICONERROR | MB_OK);
				return;
			}
		}
		catch (const std::runtime_error &e)
		{
			AfxMessageBox(CString(_T("fail to create an IMU: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
			return;
		}
		// initialize GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize GPS"));
		try
		{
			gps_.reset(new swl::GpsInterface(gpsPortName_, gpsBaudRate_));
			if (!gps_)
			{
				AfxMessageBox(_T("fail to create a GPS"), MB_ICONERROR | MB_OK);
				return;
			}
		}
		catch (const std::runtime_error &e)
		{
			AfxMessageBox(CString(_T("fail to create an GPS: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
			return;
		}

		// connect GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("connect GPS"));
		if (!gps_->isConnected())
		{
			AfxMessageBox(_T("fail to connect a GPS"), MB_ICONERROR | MB_OK);
			return;
		}

		// load calibration parameters
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("load calibration parameters of ADIS16350"));
		{
			const std::string calibration_param_filename("..\\data\\adis16350_data_20100801\\imu_calibration_result.txt");
			if (!imu_->loadCalibrationParam(calibration_param_filename))
			{
				AfxMessageBox(_T("fail to load a IMU's calibration parameters"), MB_ICONERROR | MB_OK);
				return;
			}
		}

#if 0
		// set the initial local gravity & the initial Earth's angular velocity
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("set the initial local gravity & the initial Earth's angular velocity"));
		initialGravity_.x = initialGravity_.y = initialGravity_.z = 0.0;
		initialAngularVel_.x = initialAngularVel_.y = initialAngularVel_.z = 0.0;
		if (!imu_->setInitialAttitude(Nimu, initialGravity_, initialAngularVel_))
		{
			AfxMessageBox(_T("fail to set the initial local gravity & the initial Earth's angular velocity"), MB_ICONERROR | MB_OK);
			return;
		}

		// FIXME [modify] >>
		initialAngularVel_.x = initialAngularVel_.y = initialAngularVel_.z = 0.0;

		// set the initial position and speed of GPS
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("set the initial position and speed of GPS"));
		if (!gps_->setInitialState(Ngps, initialGpsECEF_, initialGpsSpeed_))
		{
			AfxMessageBox(_T("fail to set the initial position & speed of the GPS"), MB_ICONERROR | MB_OK);
			return;
		}

		// FIXME [modify] >>
		initialGpsSpeed_.val = 0.0;
#else
		if (!isSensorsInitialized_)
		{
			AfxMessageBox(_T("fail to set the initial parameters of the IMU & the GPS"), MB_ICONERROR | MB_OK);
			return;
		}
#endif

		// FIXME [check] >>
		freq_.LowPart = 0;
		freq_.HighPart = 0;
		QueryPerformanceFrequency(&freq_);
		prevPerformanceCount_.LowPart = 0;
		prevPerformanceCount_.HighPart = 0;
		QueryPerformanceCounter(&prevPerformanceCount_);
		prevGpsUtc_.hour = prevGpsUtc_.min = prevGpsUtc_.sec = prevGpsUtc_.msec = 0;
		prevGpsECEF_.x = prevGpsECEF_.y = prevGpsECEF_.z = 0.0;

		step_ = 0;

		//
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize a runner of GPS-aided IMU filter"));
		runner_.reset(new swl::GpsAidedImuFilterRunner(initialGravity_, initialAngularVel_));
		runner_->initialize();

		//
		GetDlgItem(IDC_BUTTON_RUN_FILTER)->SetWindowText(_T("Stop GPS-aided IMU Filter"));
		SetTimer(FILTER_TIMER_ID, FILTER_SAMPLING_INTERVAL, NULL);

		toggle = false;
	}
	else
	{
		GetDlgItem(IDC_BUTTON_RUN_FILTER)->SetWindowText(_T("Start GPS-aided IMU Filter"));
		KillTimer(FILTER_TIMER_ID);

		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate a runner of GPS-aided IMU filter"));
		runner_->finalize();
		runner_.reset();
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate ADIS16350"));
		imu_.reset();
		GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate GPS"));
		gps_.reset();

		toggle = true;
	}
}

void Cswl_gps_aided_imu_filter_appDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent)
	{
	case IMU_TIMER_ID:  // IMU
		checkImu();
		break;
	case GPS_TIMER_ID:  // GPS
		checkGps();
		break;
	case FILTER_TIMER_ID:  // GPS-aided IMU filter
		runFilter();
		break;
	case SAVER_TIMER_ID:
		saveRawData();
		break;
	}

	CDialog::OnTimer(nIDEvent);
}

bool Cswl_gps_aided_imu_filter_appDlg::initializeSensors()
{
	const size_t Nimu = 10000;
	const size_t Ngps = 100;

	// initialize ADIS16350
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize ADIS16350"));
	try
	{
		imu_.reset(new swl::Adis16350Interface());
		if (!imu_)
		{
			AfxMessageBox(_T("fail to create an IMU"), MB_ICONERROR | MB_OK);
			return false;
		}
	}
	catch (const std::runtime_error &e)
	{
		AfxMessageBox(CString(_T("fail to create an IMU: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
		return false;
	}
	// initialize GPS
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("initialize GPS"));
	try
	{
		gps_.reset(new swl::GpsInterface(gpsPortName_, gpsBaudRate_));
		if (!gps_)
		{
			AfxMessageBox(_T("fail to create a GPS"), MB_ICONERROR | MB_OK);
			return false;
		}
	}
	catch (const std::runtime_error &e)
	{
		AfxMessageBox(CString(_T("fail to create an GPS: ")) + CString(e.what()), MB_ICONERROR | MB_OK);
		return false;
	}

	// connect GPS
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("connect GPS"));
	if (!gps_->isConnected())
	{
		AfxMessageBox(_T("fail to connect a GPS"), MB_ICONERROR | MB_OK);
		return false;
	}

	// load calibration parameters
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("load calibration parameters of ADIS16350"));
	{
		const std::string calibration_param_filename("..\\data\\adis16350_data_20100801\\imu_calibration_result.txt");
		if (!imu_->loadCalibrationParam(calibration_param_filename))
		{
			AfxMessageBox(_T("fail to load a IMU's calibration parameters"), MB_ICONERROR | MB_OK);
			return false;
		}
	}

	// set the initial local gravity & the initial Earth's angular velocity
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("set the initial local gravity & the initial Earth's angular velocity"));
	initialGravity_.x = initialGravity_.y = initialGravity_.z = 0.0;
	initialAngularVel_.x = initialAngularVel_.y = initialAngularVel_.z = 0.0;
	if (!imu_->setInitialAttitude(Nimu, initialGravity_, initialAngularVel_))
	{
		AfxMessageBox(_T("fail to set the initial local gravity & the initial Earth's angular velocity"), MB_ICONERROR | MB_OK);
		return false;
	}

	// FIXME [modify] >>
	initialAngularVel_.x = initialAngularVel_.y = initialAngularVel_.z = 0.0;

	// set the initial position and speed of GPS
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("set the initial position and speed of GPS"));
	if (!gps_->setInitialState(Ngps, initialGpsECEF_, initialGpsSpeed_))
	{
		AfxMessageBox(_T("fail to set the initial position & speed of the GPS"), MB_ICONERROR | MB_OK);
		return false;
	}

	// FIXME [modify] >>
	initialGpsSpeed_.val = 0.0;

	//
	GetDlgItem(IDC_EDIT_MESSAGE)->SetWindowText(_T("terminate ADIS16350 & GPS"));
	imu_.reset();
	gps_.reset();

	return true;
}

void Cswl_gps_aided_imu_filter_appDlg::saveRawData()
{
	swl::ImuData::Accel measuredAccel(0.0, 0.0, 0.0);
	swl::ImuData::Gyro measuredAngularVel(0.0, 0.0, 0.0);
	LARGE_INTEGER performanceCount;
	swl::EarthData::Geodetic measuredGpsGeodetic(0.0, 0.0, 0.0);
	swl::EarthData::Speed measuredGpsSpeed(0.0);
	swl::EarthData::Time gpsUtc(0, 0, 0, 0);

	CString msg;

	// get measurements of IMU & GPS
	if (!imu_->readData(measuredAccel, measuredAngularVel, performanceCount) ||
		!gps_->readData(measuredGpsGeodetic, measuredGpsSpeed, gpsUtc))
		return;

	const __int64 imuTimeStamp = (0 == performanceCount.HighPart && 0 == performanceCount.LowPart) ? 0 : ((performanceCount.QuadPart) * 1000 / freq_.QuadPart);
	const long gpsTimeStamp = (gpsUtc.min * 60 + gpsUtc.sec) * 1000 + gpsUtc.msec;

	imuTimeStamps_.push_back(imuTimeStamp);
	imuAccels_.push_back(measuredAccel);
	imuGyros_.push_back(measuredAngularVel);
	gpsTimeStamps_.push_back(gpsTimeStamp);
	gpsGeodetics_.push_back(measuredGpsGeodetic);
	gpsSpeeds_.push_back(measuredGpsSpeed);

	//
	const __int64 imuElapsedTime = (0 == performanceCount.HighPart && 0 == performanceCount.LowPart) ? 0 : ((performanceCount.QuadPart - prevPerformanceCount_.QuadPart) * 1000 / freq_.QuadPart);
	const long gpsElapsedTime = ((gpsUtc.min - prevGpsUtc_.min) * 60 + (gpsUtc.sec - prevGpsUtc_.sec)) * 1000 + (gpsUtc.msec - prevGpsUtc_.msec);

	//
	prevPerformanceCount_ = performanceCount;
	prevGpsUtc_ = gpsUtc;

	++step_;

	//
	msg.Format(_T("%f"), measuredAccel.x);
	GetDlgItem(IDC_EDIT_IMU_ACCEL_X)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAccel.y);
	GetDlgItem(IDC_EDIT_IMU_ACCEL_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAccel.z);
	GetDlgItem(IDC_EDIT_IMU_ACCEL_Z)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAngularVel.x);
	GetDlgItem(IDC_EDIT_IMU_GYRO_X)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAngularVel.y);
	GetDlgItem(IDC_EDIT_IMU_GYRO_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAngularVel.z);
	GetDlgItem(IDC_EDIT_IMU_GYRO_Z)->SetWindowText(msg);
	msg.Format(_T("%d"), imuElapsedTime);
	GetDlgItem(IDC_EDIT_IMU_ELAPSED_TIME)->SetWindowText(msg);

	msg.Format(_T("%f"), measuredGpsGeodetic.lat);
	GetDlgItem(IDC_EDIT_GPS_LAT)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsGeodetic.lon);
	GetDlgItem(IDC_EDIT_GPS_LON)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsGeodetic.alt);
	GetDlgItem(IDC_EDIT_GPS_ALT)->SetWindowText(msg);
	GetDlgItem(IDC_EDIT_GPS_X)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDIT_GPS_Y)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDIT_GPS_Z)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDIT_GPS_VEL_X)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDIT_GPS_VEL_Y)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDIT_GPS_VEL_Z)->SetWindowText(_T(""));
	msg.Format(_T("%f"), measuredGpsSpeed.val);
	GetDlgItem(IDC_EDIT_GPS_SPEED)->SetWindowText(msg);
	msg.Format(_T("%d"), gpsElapsedTime);
	GetDlgItem(IDC_EDIT_GPS_ELAPSED_TIME)->SetWindowText(msg);

	msg.Format(_T("%d"), step_);
	GetDlgItem(IDC_EDIT_STEP)->SetWindowText(msg);
}

void Cswl_gps_aided_imu_filter_appDlg::checkImu()
{
	swl::ImuData::Accel measuredAccel(0.0, 0.0, 0.0), calibratedAccel(0.0, 0.0, 0.0);
	swl::ImuData::Gyro measuredAngularVel(0.0, 0.0, 0.0), calibratedAngularVel(0.0, 0.0, 0.0);
	LARGE_INTEGER performanceCount;

	CString msg;

	// get measurements of IMU
	if (!imu_->readData(measuredAccel, measuredAngularVel, performanceCount)) return;

	// elpased time [msec]
	const __int64 elapsedTime = (0 == performanceCount.HighPart && 0 == performanceCount.LowPart) ? 0 : ((performanceCount.QuadPart - prevPerformanceCount_.QuadPart) * 1000 / freq_.QuadPart);
	prevPerformanceCount_ = performanceCount;

	//
	imu_->calculateCalibratedAcceleration(measuredAccel, calibratedAccel);
	imu_->calculateCalibratedAngularRate(measuredAngularVel, calibratedAngularVel);

	//
	++step_;

	msg.Format(_T("%f"), measuredAccel.x);
	GetDlgItem(IDC_EDIT_IMU_ACCEL_X)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAccel.y);
	GetDlgItem(IDC_EDIT_IMU_ACCEL_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAccel.z);
	GetDlgItem(IDC_EDIT_IMU_ACCEL_Z)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAngularVel.x);
	GetDlgItem(IDC_EDIT_IMU_GYRO_X)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAngularVel.y);
	GetDlgItem(IDC_EDIT_IMU_GYRO_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredAngularVel.z);
	GetDlgItem(IDC_EDIT_IMU_GYRO_Z)->SetWindowText(msg);
	msg.Format(_T("%d"), elapsedTime);
	GetDlgItem(IDC_EDIT_IMU_ELAPSED_TIME)->SetWindowText(msg);

	msg.Format(_T("%d"), step_);
	GetDlgItem(IDC_EDIT_STEP)->SetWindowText(msg);
}

void Cswl_gps_aided_imu_filter_appDlg::checkGps()
{
	swl::EarthData::Geodetic measuredGpsGeodetic(0.0, 0.0, 0.0);
	swl::EarthData::ECEF measuredGpsECEF(0.0, 0.0, 0.0);
	swl::EarthData::Speed measuredGpsSpeed(0.0);
	swl::EarthData::Time gpsUtc(0, 0, 0, 0);

	CString msg;

	// get measurements of GPS
	if (!gps_->readData(measuredGpsGeodetic, measuredGpsSpeed, gpsUtc)) return;
	swl::EarthData::geodetic_to_ecef(measuredGpsGeodetic, measuredGpsECEF);

	measuredGpsECEF.x -= initialGpsECEF_.x;
	measuredGpsECEF.y -= initialGpsECEF_.y;
	measuredGpsECEF.z -= initialGpsECEF_.z;

	// elpased time [msec]
	// TODO [check] >>
	const long elapsedTime = ((gpsUtc.min - prevGpsUtc_.min) * 60 + (gpsUtc.sec - prevGpsUtc_.sec)) * 1000 + (gpsUtc.msec - prevGpsUtc_.msec);

	prevGpsUtc_ = gpsUtc;
	prevGpsECEF_ = measuredGpsECEF;

	//
	++step_;

	msg.Format(_T("%f"), measuredGpsGeodetic.lat);
	GetDlgItem(IDC_EDIT_GPS_LAT)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsGeodetic.lon);
	GetDlgItem(IDC_EDIT_GPS_LON)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsGeodetic.alt);
	GetDlgItem(IDC_EDIT_GPS_ALT)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsECEF.x);
	GetDlgItem(IDC_EDIT_GPS_X)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsECEF.y);
	GetDlgItem(IDC_EDIT_GPS_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsECEF.z);
	GetDlgItem(IDC_EDIT_GPS_Z)->SetWindowText(msg);
	msg.Format(_T("%f"), 0 == elapsedTime ? 0 : (measuredGpsECEF.x - prevGpsECEF_.x) / elapsedTime * 1000);
	GetDlgItem(IDC_EDIT_GPS_VEL_X)->SetWindowText(msg);
	msg.Format(_T("%f"), 0 == elapsedTime ? 0 : (measuredGpsECEF.y - prevGpsECEF_.y) / elapsedTime * 1000);
	GetDlgItem(IDC_EDIT_GPS_VEL_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), 0 == elapsedTime ? 0 : (measuredGpsECEF.z - prevGpsECEF_.z) / elapsedTime * 1000);
	GetDlgItem(IDC_EDIT_GPS_VEL_Z)->SetWindowText(msg);
	msg.Format(_T("%f"), measuredGpsSpeed.val);
	GetDlgItem(IDC_EDIT_GPS_SPEED)->SetWindowText(msg);
	msg.Format(_T("%d"), elapsedTime);
	GetDlgItem(IDC_EDIT_GPS_ELAPSED_TIME)->SetWindowText(msg);

	msg.Format(_T("%d"), step_);
	GetDlgItem(IDC_EDIT_STEP)->SetWindowText(msg);
}

void Cswl_gps_aided_imu_filter_appDlg::runFilter()
{
	swl::ImuData::Accel measuredAccel(0.0, 0.0, 0.0), calibratedAccel(0.0, 0.0, 0.0);
	swl::ImuData::Gyro measuredAngularVel(0.0, 0.0, 0.0), calibratedAngularVel(0.0, 0.0, 0.0);
	LARGE_INTEGER performanceCount;
	swl::EarthData::Geodetic measuredGpsGeodetic(0.0, 0.0, 0.0);
	swl::EarthData::ECEF measuredGpsECEF(0.0, 0.0, 0.0);
	swl::EarthData::ECEF measuredGpsVel(0.0, 0.0, 0.0);
	swl::EarthData::Speed measuredGpsSpeed(0.0);
	swl::EarthData::Time gpsUtc(0, 0, 0, 0);

	CString msg;

	// get measurements of IMU & GPS
	if (!imu_->readData(measuredAccel, measuredAngularVel, performanceCount) ||
		!gps_->readData(measuredGpsGeodetic, measuredGpsSpeed, gpsUtc))
		return;

	//
	imu_->calculateCalibratedAcceleration(measuredAccel, calibratedAccel);
	imu_->calculateCalibratedAngularRate(measuredAngularVel, calibratedAngularVel);

	swl::EarthData::geodetic_to_ecef(measuredGpsGeodetic, measuredGpsECEF);

	measuredGpsECEF.x -= initialGpsECEF_.x;
	measuredGpsECEF.y -= initialGpsECEF_.y;
	measuredGpsECEF.z -= initialGpsECEF_.z;

	//
	const __int64 imuElapsedTime = (0 == performanceCount.HighPart && 0 == performanceCount.LowPart) ? 0 : ((performanceCount.QuadPart - prevPerformanceCount_.QuadPart) * 1000 / freq_.QuadPart);
	const long gpsElapsedTime = ((gpsUtc.min - prevGpsUtc_.min) * 60 + (gpsUtc.sec - prevGpsUtc_.sec)) * 1000 + (gpsUtc.msec - prevGpsUtc_.msec);
	measuredGpsVel.x = (measuredGpsECEF.x - prevGpsECEF_.x) / gpsElapsedTime * 1000;
	measuredGpsVel.y = (measuredGpsECEF.y - prevGpsECEF_.y) / gpsElapsedTime * 1000;
	measuredGpsVel.z = (measuredGpsECEF.z - prevGpsECEF_.z) / gpsElapsedTime * 1000;

	//
	if (!runner_->runStep(calibratedAccel, calibratedAngularVel, measuredGpsECEF, measuredGpsVel, measuredGpsSpeed))
		throw std::runtime_error("GPS-aided IMU filter error !!!");

	//
	prevPerformanceCount_ = performanceCount;
	prevGpsUtc_ = gpsUtc;
	prevGpsECEF_ = measuredGpsECEF;

	//
	const gsl_vector *pos = runner_->getFilteredPos();
	const gsl_vector *vel = runner_->getFilteredVel();
	const gsl_vector *accel = runner_->getFilteredAccel();
	const gsl_vector *quat = runner_->getFilteredQuaternion();
	const gsl_vector *angVel = runner_->getFilteredAngularVel();

	const double dist = std::sqrt(gsl_vector_get(pos, 0)*gsl_vector_get(pos, 0) + gsl_vector_get(pos, 1)*gsl_vector_get(pos, 1) + gsl_vector_get(pos, 2)*gsl_vector_get(pos, 2));

	++step_;

	msg.Format(_T("%f"), gsl_vector_get(pos, 0));
	GetDlgItem(IDC_EDIT_STATE_POS_X)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(pos, 1));
	GetDlgItem(IDC_EDIT_STATE_POS_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(pos, 2));
	GetDlgItem(IDC_EDIT_STATE_POS_Z)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(vel, 0));
	GetDlgItem(IDC_EDIT_STATE_VEL_X)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(vel, 1));
	GetDlgItem(IDC_EDIT_STATE_VEL_Y)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(vel, 2));
	GetDlgItem(IDC_EDIT_STATE_VEL_Z)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(quat, 0));
	GetDlgItem(IDC_EDIT_STATE_E0)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(quat, 1));
	GetDlgItem(IDC_EDIT_STATE_E1)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(quat, 2));
	GetDlgItem(IDC_EDIT_STATE_E2)->SetWindowText(msg);
	msg.Format(_T("%f"), gsl_vector_get(quat, 3));
	GetDlgItem(IDC_EDIT_STATE_E3)->SetWindowText(msg);
	msg.Format(_T("%f"), dist);
	GetDlgItem(IDC_EDIT_FILTER_DISTANCE)->SetWindowText(msg);
	msg.Format(_T("%d"), std::max(imuElapsedTime, (__int64)gpsElapsedTime));
	GetDlgItem(IDC_EDIT_FILTER_ELAPSED_TIME)->SetWindowText(msg);

	msg.Format(_T("%d"), step_);
	GetDlgItem(IDC_EDIT_STEP)->SetWindowText(msg);
}
