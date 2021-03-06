
// OpenCVMFCDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "Debug.h"

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <fstream>

#include "stdlib.h"
#include <omp.h>
#include <tchar.h>

#include "OpenCVMFC.h"
#include "OpenCVMFCDlg.h"
#include "afxdialogex.h"

#include "ImageProcess.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma warning(disable:4996)   // 这个很重要,防止提示编译错误.

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// COpenCVMFCDlg 对话框

const int COpenCVMFCDlg::UPDATE_INTERVAL;

COpenCVMFCDlg::COpenCVMFCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_OPENCVMFC_DIALOG, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

COpenCVMFCDlg::~COpenCVMFCDlg() {
#ifdef _DEBUG
	closeConsole();
#else
	closeConsole();
#endif
}

BEGIN_MESSAGE_MAP(COpenCVMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(OPEN_FILE, &COpenCVMFCDlg::OnBnClickedFile)
	ON_NOTIFY(TRBN_THUMBPOSCHANGING, ROTATE_SLIDER, &COpenCVMFCDlg::OnRotateSliderChanged)
	ON_NOTIFY(NM_CUSTOMDRAW, ROTATE_SLIDER, &COpenCVMFCDlg::OnRotateSliderChanged)
	ON_NOTIFY(NM_CUSTOMDRAW, XSCALE_SILDER, &COpenCVMFCDlg::OnXScaleSliderChanged)
	ON_NOTIFY(NM_CUSTOMDRAW, YSCALE_SILDER, &COpenCVMFCDlg::OnYScaleSliderChanged)

	ON_EN_KILLFOCUS(ROTATE_INPUT, &COpenCVMFCDlg::OnRotateInputBlur)
	ON_EN_KILLFOCUS(XSCALE_INPUT, &COpenCVMFCDlg::OnXScaleInputBlur)
	ON_EN_KILLFOCUS(YSCALE_INPUT, &COpenCVMFCDlg::OnYScaleInputBlur)
	ON_CBN_SELCHANGE(SCALE_ALGO, &COpenCVMFCDlg::OnScaleAlgoChanged)

	ON_MESSAGE(WM_TRANSLATE, &COpenCVMFCDlg::onProcessTranslateCompleted)
	ON_BN_CLICKED(DFT_ENABLE, &COpenCVMFCDlg::OnDFTEnable)
	ON_BN_CLICKED(NO_NOISE, &COpenCVMFCDlg::OnNoNoise)
	ON_BN_CLICKED(GAUS_NOISE, &COpenCVMFCDlg::OnGausNoise)
	ON_BN_CLICKED(SALT_NOISE, &COpenCVMFCDlg::OnSaltNoise)
	ON_EN_KILLFOCUS(GAUS_AVG, &COpenCVMFCDlg::OnNoiseAvgBlur)
	ON_EN_KILLFOCUS(GAUS_STD, &COpenCVMFCDlg::OnNoiseStdBlur)
	ON_EN_KILLFOCUS(SALT_FACTOR, &COpenCVMFCDlg::OnNoiseFactorBlur)
	ON_BN_CLICKED(AUTO_FIT, &COpenCVMFCDlg::OnAutoFitClicked)
	ON_CBN_SELCHANGE(FILTER_TYPE, &COpenCVMFCDlg::OnFilterTypeChanged)
	ON_BN_CLICKED(REFRESH, &COpenCVMFCDlg::OnRefreshButtonClicked)
	ON_BN_CLICKED(AUTO_REFRESH, &COpenCVMFCDlg::OnAutoRefreshClicked)
	ON_EN_KILLFOCUS(GAUS_FILTER_STD, &COpenCVMFCDlg::OnEnKillfocusFilterStd)
END_MESSAGE_MAP()


// COpenCVMFCDlg 消息处理程序

BOOL COpenCVMFCDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
#ifdef _DEBUG
	openConsole();
	LOG("初始化对话框完成");
#else
	// 为了输出处理时间，还是需要 openConsole
	openConsole();
	LOG("初始化对话框完成");
#endif
	AfxBeginThread((AFX_THREADPROC)&COpenCVMFCDlg::updateDialog, this);

	initialize();

	initializing = false;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void COpenCVMFCDlg::initialize() {
	ImageProcess::initialize();

	m_autoFit.SetCheck(true);
	refreshTransformInputs();
	refreshNoiseInputs();
	initializeScaleAlgo();
	initializeThreadInputs();
	initializeFilterInputs();
}

void COpenCVMFCDlg::setTransformParams(double angle, double xScale, double yScale) {
	param.angle = angle;
	param.xScale = xScale;
	param.yScale = yScale;

	int value; CString num;
	value = round(angle / MAX_ROTATE_ANGLE * 100);
	m_rotateSlider.SetPos(value);
	value = round((xScale - MIN_SCLAE_RATE) /
		(MAX_SCLAE_RATE - MIN_SCLAE_RATE) * 100);
	m_xSlider.SetPos(value);
	value = round((yScale - MIN_SCLAE_RATE) /
		(MAX_SCLAE_RATE - MIN_SCLAE_RATE) * 100);
	m_ySlider.SetPos(value);
	/*
	num.Format(_T("%.2f"), angle);
	m_rotateInput.SetWindowTextW(num);
	num.Format(_T("%.2f"), xScale);
	m_xInput.SetWindowTextW(num);
	num.Format(_T("%.2f"), yScale);
	m_yInput.SetWindowTextW(num);
	*/
}

void COpenCVMFCDlg::setTransformAlgo(int algo) {
	m_scaleAlgo.SetCurSel(param.algo = algo);
}

void COpenCVMFCDlg::setDFTEnable(bool val) {
	m_dftEnable.SetCheck(param.dft = val);
}

void COpenCVMFCDlg::setNoiseType(int type) {
	param.noise.type = type;
	switch (type) {
	case 0: m_noNoise.SetCheck(true); OnNoNoise(); break;
	case 1: m_gausNoise.SetCheck(true); OnGausNoise(); break;
	case 2: m_saltNoise.SetCheck(true); OnSaltNoise(); break;
	}
}

void COpenCVMFCDlg::setNoiseParams(double avg, double std, double factor) {
	param.noise.avg = avg;
	param.noise.std = std;
	param.noise.factor = factor;

	CString num;
	num.Format(_T("%.2f"), avg);
	m_gausAvg.SetWindowTextW(num);
	num.Format(_T("%.2f"), std);
	m_gausStd.SetWindowTextW(num);
	num.Format(_T("%.2f"), factor);
	m_saltFactor.SetWindowTextW(num);
}

void COpenCVMFCDlg::setFilterType(int type) {
	m_filterType.SetCurSel(param.filter.type = type);
	OnFilterTypeChanged();
}

void COpenCVMFCDlg::setFilterParams(double std) {
	param.filter.std = std;

	CString num;
	num.Format(_T("%.2f"), std);
	m_gausFilterStd.SetWindowTextW(num);
}

void COpenCVMFCDlg::setThreadMode(int type) {
	m_threadMode.SetCurSel(type);
}

void COpenCVMFCDlg::setThreadCount(int cnt, bool enable) {
	openCL = max(openCL, 0);
	if(cnt > 0) m_threadInput.SetCurSel(cnt-1);
	m_threadInput.EnableWindow(enable);
}

void COpenCVMFCDlg::initializeScaleAlgo() {
	m_scaleAlgo.InsertString(0, _T("双三次插值"));
	m_scaleAlgo.InsertString(1, _T("双三次插值(CL)"));
	m_scaleAlgo.InsertString(2, _T("双线性插值"));
	setTransformAlgo(0);
}

void COpenCVMFCDlg::initializeThreadInputs() {
	for (int i = 1; i <= MAX_THREAD_COUNT; ++i) {
		CString num; num.Format(_T("%d"), i);
		m_threadInput.InsertString(i-1, num);
	}
	setThreadCount(1, true);

	m_threadMode.InsertString(0, _T("WIN"));
	m_threadMode.InsertString(1, _T("OpenMP"));
	//m_threadMode.InsertString(2, _T("OpenCL"));
	setThreadMode(0);
}

void COpenCVMFCDlg::initializeFilterInputs() {
	m_filterType.InsertString(0, _T("无滤波"));
	m_filterType.InsertString(1, _T("中值滤波"));
	m_filterType.InsertString(2, _T("中值滤波(CL)"));
	m_filterType.InsertString(3, _T("均值滤波"));
	m_filterType.InsertString(4, _T("高斯滤波"));
	m_filterType.InsertString(5, _T("维纳滤波"));
	m_filterType.InsertString(6, _T("双边滤波")); 
	setFilterType(0);

	setFilterParams(ImageProcess::GAUS_NOISE_STD);
}

UINT COpenCVMFCDlg::updateDialog(void* p) {
	while (1) {
		Sleep(UPDATE_INTERVAL);
		COpenCVMFCDlg* dlg = (COpenCVMFCDlg*)p;
		dlg->updateDialog();
	}
	return 0;
}

void COpenCVMFCDlg::updateDialog() {
	if (dirty && !processing) {
		dirty = false;
		refresh();
		refreshSrcPicture();
	}
}

void COpenCVMFCDlg::openConsole() {
	// 保证函数只执行一次
	static bool flag = false;
	if (flag) return; flag = true;

	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

void COpenCVMFCDlg::closeConsole() {
	FreeConsole();
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
}

void COpenCVMFCDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。
void COpenCVMFCDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else 
		CDialogEx::OnPaint(); 
}

void COpenCVMFCDlg::refreshTransformInputs() {
	setTransformParams(0, 1, 1);
}

void COpenCVMFCDlg::refreshNoiseInputs() {
	setNoiseType(0);

	setNoiseParams(ImageProcess::GAUS_NOISE_AVG,
		ImageProcess::GAUS_NOISE_STD, ImageProcess::SALT_NOISE_FACTOR);
}

void COpenCVMFCDlg::refreshSrcPicture() {
	if (m_pImgSrc != NULL) {
		int height, width;
		CRect rect, rect2;
		height = m_pImgSrc->GetHeight();
		width = m_pImgSrc->GetWidth();

		m_srcImage.GetClientRect(&rect);
		CDC *pDC = m_srcImage.GetDC();
		SetStretchBltMode(pDC->m_hDC, STRETCH_HALFTONE);

		if (width <= rect.Width() && height <= rect.Width()) {
			rect2 = CRect(rect.TopLeft(), CSize(width, height));
			m_pImgSrc->StretchBlt(pDC->m_hDC, rect, WHITENESS);
			m_pImgSrc->StretchBlt(pDC->m_hDC, rect2, SRCCOPY);
		} else if (autoFit) {
			float xScale = (float)rect.Width() / (float)width;
			float yScale = (float)rect.Height() / (float)height;
			float ScaleIndex = (xScale <= yScale ? xScale : yScale);
			rect2 = CRect(rect.TopLeft(), CSize((int)width*ScaleIndex, (int)height*ScaleIndex));
			m_pImgSrc->StretchBlt(pDC->m_hDC, rect, WHITENESS);
			m_pImgSrc->StretchBlt(pDC->m_hDC, rect2, SRCCOPY);
		} else {
			int realW = min(rect.Width(), width);
			int realH = min(rect.Height(), height);
			rect2 = CRect(rect.TopLeft(), CSize(realW, realH));
			m_pImgSrc->StretchBlt(pDC->m_hDC, rect, WHITENESS);
			m_pImgSrc->StretchBlt(pDC->m_hDC, rect2, rect2, SRCCOPY);
		}
		ReleaseDC(pDC);
	}
}

void COpenCVMFCDlg::refreshOutPicture() {
	if (m_pImgSrc != NULL && m_pImgDist != NULL && !m_pImgDist->IsNull()) {
		int height, width;
		CRect rect, rect2, srcRect;
		height = m_pImgDist->GetHeight();
		width = m_pImgDist->GetWidth();

		m_outImage.GetClientRect(&rect);
		CDC *pDC = m_outImage.GetDC();
		SetStretchBltMode(pDC->m_hDC, STRETCH_HALFTONE);
		
		if (width <= rect.Width() && height <= rect.Width()) {
			rect2 = CRect(rect.TopLeft(), CSize(width, height));
			m_pImgDist->StretchBlt(pDC->m_hDC, rect, WHITENESS);
			m_pImgDist->StretchBlt(pDC->m_hDC, rect2, SRCCOPY);
		} else if (autoFit) {
			float xScale = (float)rect.Width() / (float)width;
			float yScale = (float)rect.Height() / (float)height;
			float ScaleIndex = (xScale <= yScale ? xScale : yScale);
			rect2 = CRect(rect.TopLeft(), CSize((int)width*ScaleIndex, (int)height*ScaleIndex));
			m_pImgDist->StretchBlt(pDC->m_hDC, rect, WHITENESS);
			m_pImgDist->StretchBlt(pDC->m_hDC, rect2, SRCCOPY);
		} else {
			int realW = min(rect.Width(), width);
			int realH = min(rect.Height(), height);
			rect2 = CRect(rect.TopLeft(), CSize(realW, realH));
			m_pImgDist->StretchBlt(pDC->m_hDC, rect, WHITENESS);
			m_pImgDist->StretchBlt(pDC->m_hDC, rect2, rect2, SRCCOPY);
		}

		ReleaseDC(pDC);
	}
}

void COpenCVMFCDlg::refresh() {
	if (m_pImgSrc == NULL) return;
	LOG("刷新处理中……");
	
	m_pImgDist = ImageProcess::createImage(m_pImgDist);

	param.startTime = (double)GetTickCount();
	int count = param.threadCount = m_threadInput.GetCurSel() + 1;

	ImageProcess::setParam(param);

	if (count <= 1)
		refreshSingleThread();
	else {
		processing = true;
		int mode = m_threadMode.GetCurSel();
		switch (mode) {
		case 0: refreshMultWINThreads(count); break;
		case 1: refreshMultOpenMPThreads(count); break;
		}
	}
}


void COpenCVMFCDlg::refreshSingleThread() {
	ImageFragment *frag = ImageProcess::createImageFragment(m_pImgSrc, m_pImgDist);
	ImageProcess::process(frag, false);
}

void COpenCVMFCDlg::refreshMultWINThreads(int count) {
	ImageFragment** frags = new ImageFragment*[count];
	for (int i = 0; i < count; ++i)
		frags[i] = ImageProcess::createImageFragment(m_pImgSrc, m_pImgDist, count, i);

	for (int i = 0; i < count; ++i) 
		// process 第二参数默认传 false
		AfxBeginThread((AFX_THREADPROC)&ImageProcess::process, frags[i]);
}

void COpenCVMFCDlg::refreshMultOpenMPThreads(int count) {
	ImageFragment** frags = new ImageFragment*[count];
	for (int i = 0; i < count; ++i)
		frags[i] = ImageProcess::createImageFragment(m_pImgSrc, m_pImgDist, count, i);

	param.threadCount = 1;
	#pragma omp parallel for num_threads(count)
	for (int i = 0; i < count; ++i) 
		ImageProcess::process(frags[i]);		
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_TRANSLATE, 1, NULL);
}

LRESULT COpenCVMFCDlg::onProcessTranslateCompleted(WPARAM wParam, LPARAM lParam) {
	static int tempCount = 0;
	if ((int)wParam == 1) {
		//LOG("Terminated");
		// 当所有线程都返回了值1代表全部结束~显示时间
		if (param.threadCount == ++tempCount) {
			double endTime = (double)GetTickCount();
			LOG("处理用时: " << (endTime - param.startTime) << " ms");
			refreshOutPicture();
			processing = false;
			tempCount = 0;
		}
	}
	return 0;
}


//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR COpenCVMFCDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void COpenCVMFCDlg::OnBnClickedFile() {
	TCHAR extends[] = _T("JPEG(*jpg)|*.jpg|*.bmp|TIFF(*.tif)|*.tif|All Files (*.*)|*.*||");
	CString filePath("");

	CFileDialog fileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY, extends);

	if (fileDialog.DoModal() == IDOK) {
		VERIFY(filePath = fileDialog.GetPathName());

		CString strFilePath(filePath);

		m_pathEdit.SetWindowTextW(strFilePath);

		m_pImgSrc = ImageProcess::createImage(m_pImgSrc);
		m_pImgSrc->Load(strFilePath);

		refreshTransformInputs();

		if (autoRefresh) dirty = true;
	}
}

void COpenCVMFCDlg::OnRotateSliderChanged(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if (initializing) return;

	param.angle = m_rotateSlider.GetPos() / 100.0 * MAX_ROTATE_ANGLE;

	CString angleStr;
	angleStr.Format(_T("%.2f"), param.angle);

	m_rotateInput.SetWindowTextW(angleStr);

	if (autoRefresh) dirty = true;
}


void COpenCVMFCDlg::OnXScaleSliderChanged(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if (initializing) return;

	param.xScale = MIN_SCLAE_RATE +
		m_xSlider.GetPos() / 100.0 * (MAX_SCLAE_RATE - MIN_SCLAE_RATE);

	CString scaleStr;
	scaleStr.Format(_T("%.2f"), param.xScale);

	m_xInput.SetWindowTextW(scaleStr);

	if (autoRefresh) dirty = true;
}


void COpenCVMFCDlg::OnYScaleSliderChanged(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if (initializing) return;

	param.yScale = MIN_SCLAE_RATE +
		m_ySlider.GetPos() / 100.0 * (MAX_SCLAE_RATE - MIN_SCLAE_RATE);

	CString scaleStr;
	scaleStr.Format(_T("%.2f"), param.yScale);

	m_yInput.SetWindowTextW(scaleStr);

	if (autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnRotateInputBlur() {
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_rotateInput.GetWindowTextW(str);
	param.angle = _wtof(str.GetBuffer(str.GetLength()));

	int value = round(param.angle / MAX_ROTATE_ANGLE * 100);
	m_rotateSlider.SetPos(value);
}


void COpenCVMFCDlg::OnXScaleInputBlur() {
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_xInput.GetWindowTextW(str);
	param.xScale = _wtof(str.GetBuffer(str.GetLength()));

	int value = round((param.xScale - MIN_SCLAE_RATE) /
		(MAX_SCLAE_RATE - MIN_SCLAE_RATE) * 100);
	m_xSlider.SetPos(value);
}

void COpenCVMFCDlg::OnYScaleInputBlur() {
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_yInput.GetWindowTextW(str);
	param.yScale = _wtof(str.GetBuffer(str.GetLength()));

	int value = round((param.yScale - MIN_SCLAE_RATE) /
		(MAX_SCLAE_RATE - MIN_SCLAE_RATE) * 100);
	m_ySlider.SetPos(value);
}


void COpenCVMFCDlg::OnScaleAlgoChanged() {
	// TODO: 在此添加控件通知处理程序代码
	int algo = m_scaleAlgo.GetCurSel();
	if (algo == 1 && param.algo != 1)
		setThreadCount(1, ++openCL == 0);
	if (algo != 1 && param.algo == 1)
		setThreadCount(0, --openCL == 0);
	param.algo = algo;

	if (autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnDFTEnable() {
	// TODO: 在此添加控件通知处理程序代码
	param.dft = m_dftEnable.GetCheck();
	if (param.dft) MessageBox(_T("傅里叶变换需要较长时间，请耐心等待！"));
	if(autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnNoNoise() {
	// TODO: 在此添加控件通知处理程序代码
	if (m_noNoise.GetCheck()) {
		param.noise.type = 0;
		m_gausAvg.EnableWindow(false);
		m_gausStd.EnableWindow(false);
		m_saltFactor.EnableWindow(false);
	}
	if (autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnGausNoise() {
	// TODO: 在此添加控件通知处理程序代码
	if (m_gausNoise.GetCheck()) { 
		param.noise.type = 1;
		m_gausAvg.EnableWindow(true);
		m_gausStd.EnableWindow(true);
		m_saltFactor.EnableWindow(false);
	}
	if (autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnSaltNoise() {
	// TODO: 在此添加控件通知处理程序代码
	if (m_saltNoise.GetCheck()) {
		param.noise.type = 2;
		m_gausAvg.EnableWindow(false);
		m_gausStd.EnableWindow(false);
		m_saltFactor.EnableWindow(true);
	}
	if (autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnNoiseAvgBlur() {
	// TODO: 在此添加控件通知处理程序代码
	CString str; m_gausAvg.GetWindowTextW(str);
	param.noise.avg = _wtof(str.GetBuffer(str.GetLength()));
	if (autoRefresh) dirty = true;
}

void COpenCVMFCDlg::OnNoiseStdBlur() {
	// TODO: 在此添加控件通知处理程序代码
	CString str; m_gausStd.GetWindowTextW(str);
	param.noise.std = _wtof(str.GetBuffer(str.GetLength()));
	if (autoRefresh) dirty = true;
}


void COpenCVMFCDlg::OnNoiseFactorBlur() {
	// TODO: 在此添加控件通知处理程序代码
	CString str; m_saltFactor.GetWindowTextW(str);
	param.noise.factor = _wtof(str.GetBuffer(str.GetLength()));
	if (autoRefresh) dirty = true;
}


void COpenCVMFCDlg::OnAutoFitClicked() {
	// TODO: 在此添加控件通知处理程序代码
	autoFit = m_autoFit.GetCheck();
	refreshSrcPicture();
	refreshOutPicture();
}


void COpenCVMFCDlg::OnFilterTypeChanged() {
	// TODO: 在此添加控件通知处理程序代码
	int type = m_filterType.GetCurSel();
	if (type == 4 || type == 6) // 高斯滤波/双边滤波
		m_gausFilterStd.EnableWindow(true);
	else
		m_gausFilterStd.EnableWindow(false);

	if (type == 2 && param.filter.type != 2) // 中值算法
		setThreadCount(1, ++openCL <= 0);
	else if(type != 2 && param.filter.type == 2)
		setThreadCount(0, --openCL <= 0);

	param.filter.type = type;

	if (autoRefresh) dirty = true;
}


void COpenCVMFCDlg::OnRefreshButtonClicked() {
	// TODO: 在此添加控件通知处理程序代码
	dirty = true;
}


void COpenCVMFCDlg::OnAutoRefreshClicked() {
	// TODO: 在此添加控件通知处理程序代码
	autoRefresh = m_autoRefresh.GetCheck();
}


void COpenCVMFCDlg::OnEnKillfocusFilterStd() {
	// TODO: 在此添加控件通知处理程序代码
	CString str; m_gausFilterStd.GetWindowTextW(str);
	param.filter.std = _wtof(str.GetBuffer(str.GetLength()));
	if (autoRefresh) dirty = true;
}


