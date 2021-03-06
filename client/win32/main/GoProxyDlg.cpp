
// FullProxyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoProxy.h"
#include "GoProxyDlg.h"
#include "afxdialogex.h"

#include "common.h"
#include "engine_ip.h"
#include "common_def.h"
#include "proxyConfig.h"
#include "clientMain.h"
#include "procMgr.h"

#include "CBaseObj.h"
#include "CBaseConn.h"
#include "CClient.h"
#include "CSrvRemote.h"
#include "CVpnRemote.h"
#include "CConMgr.h"
#include "CRemoteServer.h"

#include "GoProxyProcDlg.h"


#include "CNetAccept.h"
#include "CNetRecv.h"
#include "CRemoteServer.h"
#include "CLocalServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
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

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CGoProxyDlg dialog



CGoProxyDlg::CGoProxyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGoProxyDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CGoProxyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_PROXY_IP, m_lstProxyIp);
	DDX_Control(pDX, IDC_BTN_START, m_btnStart);

	DDX_Control(pDX, IDC_COMBO_PROXY_TYPE, m_cmb_proxy_type);
	DDX_Control(pDX, IDC_IPADDR_VPN, m_vpn_ipaddr);
	DDX_Control(pDX, IDC_EDT_VPN_PORT, m_edt_vpn_port);
	DDX_Control(pDX, IDC_EDT_LOCAL_PORT, m_edt_local_port);
	DDX_Control(pDX, IDC_CHK_AUTH, m_chk_auth);
	DDX_Control(pDX, IDC_EDIT_USERNAME, m_edt_username);
	DDX_Control(pDX, IDC_EDIT_PASSWD, m_edt_passwd);
	DDX_Control(pDX, IDC_COMBO_ENCRY_TYPE, m_cmb_encry_type);
	DDX_Control(pDX, IDC_EDIT_ENCRY_PWD, m_edt_encry_key);
	DDX_Control(pDX, IDC_STATIC_PROXY_SRV, m_stc_proxy_server);
	DDX_Control(pDX, IDC_BTN_PROC_CFG, m_btnProcCfg);
	DDX_Control(pDX, IDC_BTN_SAVE, m_btnSaveCfg);
	DDX_Control(pDX, IDCANCEL, m_btnExit);
	DDX_Control(pDX, IDC_COMBO_SERVERS, m_cmbServers);
	DDX_Control(pDX, ID_BTN_SRV_FRESH, m_btnSrvFresh);
	DDX_Control(pDX, IDC_SERVER_STATIC, m_server_static);
}

BEGIN_MESSAGE_MAP(CGoProxyDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CGoProxyDlg::OnBnClickedCancel)
	ON_BN_CLICKED(ID_PROC_CONFIG, &CGoProxyDlg::OnBnClickedProcConfig)

	ON_MESSAGE(WM_UPDATE_SERVERS_COMB, OnUpdateProxyServers)
	ON_MESSAGE(WM_UPDATE_PROXY_IP_LIST, OnUpdateProxyIpList)
	ON_MESSAGE(WM_TOP_SHOW, OnTopShow)
	ON_MESSAGE(WM_TRAY_NOTIFY, OnTrayNotify)
	ON_COMMAND(MY_APP_EXIT, OnMyAppExitMenu)

	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_START, &CGoProxyDlg::OnBnClickedBtnStart)

	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BTN_PROC_CFG, &CGoProxyDlg::OnBnClickedBtnProcCfg)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CGoProxyDlg::OnBnClickedBtnSave)

	ON_CBN_SELCHANGE(IDC_COMBO_PROXY_TYPE, &CGoProxyDlg::OnCbnSelchangeComboProxyType)
	ON_CBN_SELCHANGE(IDC_COMBO_ENCRY_TYPE, &CGoProxyDlg::OnCbnSelchangeComboEncryType)
	ON_BN_CLICKED(IDC_CHK_AUTH, &CGoProxyDlg::OnBnClickedChkAuth)
	ON_BN_CLICKED(ID_BTN_SRV_FRESH, &CGoProxyDlg::OnBnClickedBtnSrvFresh)
END_MESSAGE_MAP()


// CGoProxyDlg message handlers
void CGoProxyDlg::init_servers_display()
{
	CString strTmp;
	m_cmbServers.GetWindowTextA(strTmp);

	m_cmbServers.ResetContent();
	
	/*插入数据*/
	int cfg_sel = -1;
	int cur_sel = -1;

	char ipstr[IP_DESC_LEN] = { 0 };
	proxy_cfg_t* cfginfo = proxy_cfg_get();
	if (cfginfo->server_ip != 0)
	{
		engine_ipv4_to_str(htonl(cfginfo->server_ip), ipstr);
	}

	for (int ii = 0; ii < g_servers_cnt; ii++)
	{
		m_cmbServers.InsertString(ii, g_servers[ii]);
		if (strncmp(g_servers[ii], ipstr, IP_DESC_LEN) == 0)
		{
			cfg_sel = ii;
		}
		else if (strncmp(g_servers[ii], strTmp.GetBuffer(0), IP_DESC_LEN) == 0)
		{
			cur_sel = ii;
		}
	}

	if (cfg_sel == -1 && (ipstr[0] != 0))
	{
		char defaultstr[64] = {0};
		SNPRINTF(defaultstr, 64, "%s(已失效)", ipstr);
		m_cmbServers.InsertString(g_servers_cnt, defaultstr);
		m_cmbServers.SetCurSel(g_servers_cnt);
	}

	if (cur_sel != -1)
	{
		m_cmbServers.SetCurSel(cur_sel);
	}
	else if (cfg_sel != -1)
	{
		m_cmbServers.SetCurSel(cfg_sel);
	}
}

void CGoProxyDlg::init_config_display()
{
	proxy_cfg_t* cfginfo = proxy_cfg_get();
	CString str;
	char ipstr[IP_DESC_LEN] = { 0 };

	/*proxy type*/
	m_cmb_proxy_type.ResetContent();
	m_cmb_proxy_type.InsertString(0, g_proxy_cfg_type_desc[PROXY_VPN]);
	m_cmb_proxy_type.InsertString(1, g_proxy_cfg_type_desc[PROXY_SERVER]);
	m_cmb_proxy_type.SetCurSel(cfginfo->proxy_type - 1);
	if (cfginfo->proxy_type == PROXY_VPN)
	{
		m_cmbServers.ShowWindow(SW_SHOW);
		m_stc_proxy_server.ShowWindow(SW_SHOW);
		m_btnSrvFresh.ShowWindow(SW_SHOW);

		m_server_static.SetWindowText(_T("VPN服务器:"));
	}
	else
	{
		m_cmbServers.ShowWindow(SW_HIDE);
		m_stc_proxy_server.ShowWindow(SW_HIDE);
		m_btnSrvFresh.ShowWindow(SW_HIDE);

		m_server_static.SetWindowText(_T("代理服务器:"));
	}

	/*vpn ip*/
	engine_ipv4_to_str(htonl(cfginfo->vpn_ip), ipstr);
	m_vpn_ipaddr.SetWindowText(ipstr);
	str.Format(_T("%d"), cfginfo->vpn_port);
	m_edt_vpn_port.SetWindowText(str);

	/*local port*/
	str.Format(_T("%d"), cfginfo->local_port);
	m_edt_local_port.SetWindowText(str);

	/*auth*/
	m_edt_username.SetWindowText(cfginfo->username);
	m_edt_passwd.SetWindowText(cfginfo->passwd);
	if (cfginfo->is_auth)
	{
		m_chk_auth.SetCheck(1);
		m_edt_username.EnableWindow(TRUE);
		m_edt_passwd.EnableWindow(TRUE);
	}
	else
	{
		m_chk_auth.SetCheck(0);
		m_edt_username.EnableWindow(FALSE);
		m_edt_passwd.EnableWindow(FALSE);
	}

	if (cfginfo->proxy_proto == SOCKS_4)
	{
		m_chk_auth.EnableWindow(FALSE);
		m_edt_username.EnableWindow(FALSE);
		m_edt_passwd.EnableWindow(FALSE);
	}

	/*encry*/
	m_cmb_encry_type.ResetContent();
	m_cmb_encry_type.InsertString(0, g_proxy_cfg_encry_type_desc[ENCRY_NONE]);
	m_cmb_encry_type.InsertString(1, g_proxy_cfg_encry_type_desc[ENCRY_XOR]);
	m_cmb_encry_type.InsertString(2, g_proxy_cfg_encry_type_desc[ENCRY_RC4]);
	m_cmb_encry_type.SetCurSel(cfginfo->encry_type - 1);
	m_edt_encry_key.SetWindowText(cfginfo->encry_key);
	if (cfginfo->encry_type == ENCRY_NONE)
	{
		m_edt_encry_key.EnableWindow(FALSE);
	}
}

BOOL CGoProxyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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


	LONG lStyle; 
	DWORD dwStyle;
	CRect rectList;

	lStyle = GetWindowLong(m_lstProxyIp.m_hWnd, GWL_STYLE);//获取当前窗口style 
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位 
	lStyle |= LVS_REPORT; //设置style 
	SetWindowLong(m_lstProxyIp.m_hWnd, GWL_STYLE, lStyle);//设置style 
	dwStyle = m_lstProxyIp.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl） 
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl） 
	m_lstProxyIp.SetExtendedStyle(dwStyle); //设置扩展风格 
	
	m_lstProxyIp.GetClientRect(rectList); //获得当前客户区信息
	m_lstProxyIp.InsertColumn(0, "PID", LVCFMT_LEFT, rectList.Width() *2/ 20);
	m_lstProxyIp.InsertColumn(1, "目的ip/端口", LVCFMT_LEFT, rectList.Width() *3/ 20);	
	m_lstProxyIp.InsertColumn(2, "持续时间", LVCFMT_LEFT, rectList.Width() *2/ 20);
	m_lstProxyIp.InsertColumn(3, "状态",     LVCFMT_LEFT, rectList.Width() *2/ 20);
	m_lstProxyIp.InsertColumn(4, "接受(pkt/Byte)", LVCFMT_LEFT, rectList.Width() *3/ 20);
	m_lstProxyIp.InsertColumn(5, "发送(pkt/Byte)", LVCFMT_LEFT, rectList.Width() *3/ 20);
	m_lstProxyIp.InsertColumn(6, "域名", LVCFMT_LEFT, rectList.Width() *5/ 20);

	// TODO: Add extra initialization here
	if (proxy_init() != 0)
	{
		return FALSE;
	}
	
	init_config_display();
	init_servers_display();

	CRect rect;
	GetClientRect(rect);

	int width = rect.right - 12 -5;

	int a[4]= {0, rect.right-width*4/5, rect.right-width*3/5, rect.right};
	m_StatusBar.Create(WS_CHILD|WS_VISIBLE|SBT_OWNERDRAW|CCS_NORESIZE, 
			CRect(12, rect.bottom-30, rect.right-5, rect.bottom-5), this, 0);
    m_StatusBar.SetParts(sizeof(a)/sizeof(a[0]), a);
    /*x, y, width, height*/
    //m_StatusBar.MoveWindow(0, rect.bottom-20, rect.right, 20);

    int a1[5]= {0,rect.right-width*3/4, rect.right-width*2/4, rect.right-width/4, rect.right};
    m_StatusBar1.Create(WS_CHILD|WS_VISIBLE|SBT_OWNERDRAW|CCS_NORESIZE, 
    		CRect(12, rect.bottom-55, rect.right-5, rect.bottom-30), this, 0);
    m_StatusBar1.SetParts(sizeof(a1)/sizeof(a1[0]), a1);
    //m_StatusBar1.MoveWindow(0, rect.bottom-40, rect.right, 20);

	if (g_is_start)
	{
		m_btnStart.SetWindowTextA(_T("停止代理"));
	}
	else
	{
		m_btnStart.SetWindowTextA(_T("启动代理"));
	}

	if (g_LocalServ == NULL)
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("本地监听停止"), 1, 0);
	}
	else
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("本地已监听"), 1, 0);
	}

	if (is_remote_authed())
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器已认证"), 2, 0);
	}
	else if (is_remote_connected())
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器正在认证中"), 2, 0);
	}
	else if (g_RemoteServ != NULL)
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器正在连接中"), 2, 0);
	}
	else
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器断开"), 2, 0);
	}

	m_StatusBar1.SetText(_T("当前连接数: 0"), 1, 0);
	m_StatusBar1.SetText(_T("初始请求: 0"), 2, 0);
	m_StatusBar1.SetText(_T("正在建连: 0"), 3, 0);
	m_StatusBar1.SetText(_T("连接成功: 0"), 4, 0);

	NOTIFYICONDATA nc; 
	nc.cbSize = sizeof(NOTIFYICONDATA); 
	nc.hIcon = AfxGetApp()->LoadIcon(IDI_ICON1); 
	nc.hWnd = m_hWnd; 
	strcpy(nc.szTip,APP_NAME); 
	nc.uCallbackMessage = WM_TRAY_NOTIFY; 
	nc.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP; 
	nc.uID = IDI_ICON1; 
	Shell_NotifyIcon(NIM_ADD,&nc); 

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGoProxyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGoProxyDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGoProxyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CGoProxyDlg::OnBnClickedProcConfig()
{
	// TODO: Add your control notification handler code here
}


void CGoProxyDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	if (MessageBox("确实要退出吗？", "提示", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
	{
		//proxy_free();

		CDialogEx::OnCancel();
	}
}


void CGoProxyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	proxy_free();
}

/*WM_CLOSE ==> OnClose ==> OnCancel ==> DestroyWindow ==> WM_DESTROY==》 onDestroy
 ==> WM_POST_NCDESTOY ==》 delete self*/
void CGoProxyDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	//CDialog::OnClose();
	ShowWindow(SW_HIDE);// 当最小化市，隐藏主窗口
}


void  CGoProxyDlg::OnMyAppExitMenu() 
{
	if (MessageBox("确实要退出吗？", "提示", MB_OKCANCEL|MB_ICONWARNING) == IDOK)
	{
		CDialogEx::OnCancel();
	}
}

LRESULT  CGoProxyDlg::OnTrayNotify(WPARAM wParam,LPARAM lParam) 
{ 
	if (lParam == WM_LBUTTONUP)
	{
		this->ShowWindow(SW_SHOWNORMAL);
		this->SetActiveWindow();
		this->SetForegroundWindow();
		this->BringWindowToTop();
	}
	else if (lParam == WM_RBUTTONUP)
	{
		LPPOINT lpoint = new tagPOINT;
        ::GetCursorPos(lpoint);                    // 得到鼠标位置
        CMenu menu;
        menu.CreatePopupMenu();                    // 声明一个弹出式菜单
        menu.AppendMenu(MF_STRING, MY_APP_EXIT, _T("关闭"));
        menu.TrackPopupMenu(TPM_LEFTALIGN, lpoint->x ,lpoint->y, this);
        HMENU hmenu = menu.Detach(); 
        menu.DestroyMenu();
        delete lpoint;
	}

	return 0;
}

LRESULT  CGoProxyDlg::OnTopShow(WPARAM wParam, LPARAM lParam)
{
	int want = (int)wParam;

	if (want == 0)
	{
		//::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		this->ShowWindow(SW_SHOWNORMAL);
		this->SetActiveWindow();
		this->SetForegroundWindow();
		this->BringWindowToTop();
	}
	else
	{
		this->ShowWindow(SW_MINIMIZE);
	}
	return 0;
}

afx_msg LRESULT CGoProxyDlg::OnUpdateProxyServers(WPARAM wParam,LPARAM lParam)
{
	init_servers_display();
	return 0;
}

LRESULT CGoProxyDlg::OnUpdateProxyIpList(WPARAM wParam,LPARAM lParam)
{
    for(int i=m_lstProxyIp.GetItemCount()-1; i>=0;i--)
	{
		m_lstProxyIp.DeleteItem(i);
	}
	
	CBaseConnection *connObj = NULL;
	int nRow = 0, nRowRet = 0;

	char strTemp[256] = { 0 };
	uint64_t cur_time = util_get_cur_time();

/*
	m_lstProxyIp.InsertColumn(0, "PID", LVCFMT_LEFT, rectList.Width() *2/ 20);
	m_lstProxyIp.InsertColumn(1, "目的ip/端口", LVCFMT_LEFT, rectList.Width() *4/ 20);
	m_lstProxyIp.InsertColumn(2, "目的域名", LVCFMT_LEFT, rectList.Width() *4/ 20);
	m_lstProxyIp.InsertColumn(3, "持续时间", LVCFMT_LEFT, rectList.Width() *2/ 20);
	m_lstProxyIp.InsertColumn(4, "状态",     LVCFMT_LEFT, rectList.Width() *2/ 20);
	m_lstProxyIp.InsertColumn(5, "接受流量", LVCFMT_LEFT, rectList.Width() *3/ 20);
	m_lstProxyIp.InsertColumn(6, "发送流量", LVCFMT_LEFT, rectList.Width() *3/ 20);
*/
	MUTEX_LOCK(g_ConnMgr->m_obj_lock);

	char pidStr[32] = {0};

	char remote_ipstr[IP_DESC_LEN] = {0};
	char portStr[32] = {0};
	int status = 0;

	uint32_t total_conn = 0;
	uint32_t init_conn = 0;
	uint32_t conning = 0;
	uint32_t ok_conn = 0;

	CONN_LIST_RItr itr = g_ConnMgr->m_conn_objs.rbegin();
	while (itr != g_ConnMgr->m_conn_objs.rend())
	{
		connObj = *itr;

		MUTEX_LOCK(connObj->m_remote_lock);

		CClient *cliObj = CLIENT_CONVERT(connObj->m_client);

		if (NULL != cliObj)
		{
			if (cliObj->m_is_standard_socks)
			{
				SNPRINTF(pidStr, sizeof(pidStr), "%s", "标准进程");
			}
			else
			{
				if (0 != cliObj->m_proc_id)
				{
					SNPRINTF(pidStr, sizeof(pidStr), "%lu", cliObj->m_proc_id);
				}
				else
				{
					SNPRINTF(pidStr, sizeof(pidStr), "--");
				}
			}
			status = cliObj->m_status;
		}
		else
		{
			SNPRINTF(pidStr, sizeof(pidStr), "--");
			status = 0;
		}

		/*proc id*/
		nRowRet = m_lstProxyIp.InsertItem(nRow, pidStr);//插入行

		if (NULL != cliObj)
		{
			if (cliObj->m_real_remote_ipaddr != 0)
			{
				engine_ipv4_to_str(htonl(cliObj->m_real_remote_ipaddr), remote_ipstr);
			}
			else
			{
				SNPRINTF(remote_ipstr, sizeof(remote_ipstr), "----");
			}

			if (cliObj->m_real_remote_port != 0)
			{
				SNPRINTF(portStr, sizeof(portStr), "%u", cliObj->m_real_remote_port);
			}
			else
			{
				SNPRINTF(portStr, sizeof(portStr), "--");
			}

			/*域名*/
			m_lstProxyIp.SetItemText(nRowRet, 6, cliObj->m_remote_domain);
		}
		else
		{
			SNPRINTF(portStr, sizeof(portStr), "--");
			SNPRINTF(remote_ipstr, sizeof(remote_ipstr), "----");

			/*域名*/
			m_lstProxyIp.SetItemText(nRowRet, 6, "");
		}
		
		MUTEX_UNLOCK(connObj->m_remote_lock);

		/*ip/port*/
		memset(strTemp, 0, sizeof(strTemp));
		SNPRINTF(strTemp, 256, "%s/%s", remote_ipstr, portStr);
		m_lstProxyIp.SetItemText(nRowRet, 1, strTemp);

		/*time*/
		memset(strTemp, 0, sizeof(strTemp));
		SNPRINTF(strTemp, 64, _T("%lu秒"), cur_time - connObj->m_setup_time);
		m_lstProxyIp.SetItemText(nRowRet, 2, strTemp);

		total_conn++;
		/*status*/
		if (status == SOCKS_INIT)
		{
			init_conn++;
			m_lstProxyIp.SetItemText(nRowRet, 3, "初始请求");
		}
		else if (status == SOCKS_HANDSHAKING)
		{
			conning++;
			m_lstProxyIp.SetItemText(nRowRet, 3, "正在握手");
		}
		else if (status == SOCKS_AUTHING)
		{
			conning++;
			m_lstProxyIp.SetItemText(nRowRet, 3, "正在认证");
		}
		else if (status == SOCKS_CONNECTING)
		{
			conning++;
			m_lstProxyIp.SetItemText(nRowRet, 3, "正在建连");
		}
		else if (status == SOCKS_CONNECTED)
		{
			ok_conn++;
			m_lstProxyIp.SetItemText(nRowRet, 3, "已连接");
		}
		else if (status == SOCKS_CONNECTED_FAILED)
		{
			m_lstProxyIp.SetItemText(nRowRet, 3, "连接失败");
		}
		else
		{
			init_conn++;
			m_lstProxyIp.SetItemText(nRowRet, 3, "borning");
		}

		/*recv pkts/bytes*/
		memset(strTemp, 0, sizeof(strTemp));
		SNPRINTF(strTemp, 256, "%llu/%llu", 
			connObj->m_send_remote_data_cnt, 
			connObj->m_send_remote_bytes);
		m_lstProxyIp.SetItemText(nRowRet, 4, strTemp);

		/*send pkts/bytes*/
		memset(strTemp, 0, sizeof(strTemp));
		SNPRINTF(strTemp, 256, "%llu/%llu", 
			connObj->m_send_client_data_cnt, 
			connObj->m_send_client_bytes);
		m_lstProxyIp.SetItemText(nRowRet, 5, strTemp);

		itr++;
		nRow++;
	}

    MUTEX_UNLOCK(g_ConnMgr->m_obj_lock);

    if (is_remote_authed())
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器已认证"), 2, 0);
	}
	else if (is_remote_connected())
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器正在认证中"), 2, 0);
	}
	else if (g_RemoteServ != NULL)
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器正在连接中"), 2, 0);
	}
	else
	{
		//设置状态栏文本
		m_StatusBar.SetText(_T("服务器断开"), 2, 0);
	}

	CString strBar;
	strBar.Format("当前连接数: %u", total_conn);	
	m_StatusBar1.SetText(strBar, 1, 0);

	strBar.Format("初始请求: %u", init_conn);
	m_StatusBar1.SetText(strBar, 2, 0);

	strBar.Format("正在建连: %u", conning);
	m_StatusBar1.SetText(strBar, 3, 0);

	strBar.Format("连接成功: %u", ok_conn);
	m_StatusBar1.SetText(strBar, 4, 0);

	return 0;
}

void CGoProxyDlg::OnBnClickedBtnStart()
{
	// TODO: Add your control notification handler code here
	CString strTmp;

	m_btnStart.GetWindowTextA(strTmp);
	if (strTmp == _T("启动代理"))
	{
		if (g_is_start == true)
		{
			MessageBox("操作无效: 当前已经处于代理启动状态!", "提示", MB_OK);
			return;
		}

		proxy_set_status(true);
		if (NULL == g_LocalServ)
		{
			proxy_cfg_t* cfginfo = proxy_cfg_get();
			g_LocalServ = new CLocalServer(cfginfo->local_port);
		    if(0 != g_LocalServ->init())
		    {
		    	MessageBox(_T("local server init failed!"), _T("Error"), MB_OK);
		        return;
		    }
		    //MessageBox("本地监听启动成功!", "提示", MB_OK);
		}

		proxy_proc_mgr_start();

		MessageBox("代理启动成功!", "提示", MB_OK);
		m_btnStart.SetWindowTextA(_T("停止代理"));
		//设置状态栏文本
		m_StatusBar.SetText(_T("本地已监听"), 1, 0);

		/*通知主窗口更新*/
    	::PostMessage(g_parentWnd, WM_UPDATE_PROXY_IP_LIST, NULL, NULL);
	}
	else if (strTmp == _T("停止代理"))
	{
		if (g_is_start == false)
		{
			MessageBox("操作无效: 当前不处于代理停止状态!", "提示", MB_OK);
			return;
		}

		proxy_set_status(false);

		if (g_LocalServ != NULL)
		{
			g_LocalServ->free();
			g_LocalServ = NULL;
		}

		/*remote server*/
		MUTEX_LOCK(m_remote_srv_lock);
		if (g_RemoteServ != NULL)
		{
			g_RemoteServ->free();
		}
		MUTEX_UNLOCK(m_remote_srv_lock);

		/*停止进程注入*/
		proxy_proc_mgr_stop();

		MessageBox("代理停止成功!", "提示", MB_OK);
		m_btnStart.SetWindowTextA(_T("启动代理"));
		//设置状态栏文本
		m_StatusBar.SetText(_T("本地监听停止"), 1, 0);

		/*通知主窗口更新*/
    	::PostMessage(g_parentWnd, WM_UPDATE_PROXY_IP_LIST, NULL, NULL);
	}
}

BOOL CGoProxyDlg::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	NOTIFYICONDATA nc;
	nc.cbSize = sizeof(NOTIFYICONDATA);
	nc.hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	nc.hWnd = m_hWnd;
	strcpy(nc.szTip, APP_NAME);
	nc.uCallbackMessage = WM_TRAY_NOTIFY;
	nc.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nc.uID = IDI_ICON1;


	Shell_NotifyIcon(NIM_DELETE, &nc);

	return CDialogEx::DestroyWindow();
}


void CGoProxyDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (nType == SIZE_MINIMIZED)
	{
		ShowWindow(SW_HIDE);// 当最小化市，隐藏主窗口
	}
}

void CGoProxyDlg::OnBnClickedBtnProcCfg()
{
	// TODO: Add your control notification handler code here
	GoProxyProcDlg procCfgDlg;
	procCfgDlg.DoModal();
}


void CGoProxyDlg::OnBnClickedBtnSave()
{
	// TODO: Add your control notification handler code here
	proxy_cfg_t cfginfo;
	memset(&cfginfo, 0, sizeof(cfginfo));

	CString strTmp;
	int tmp = 0;

	m_edt_local_port.GetWindowTextA(strTmp);
	tmp = _tcstoul(strTmp, NULL, 10);
	if (tmp <= 0 || tmp > 65535)
	{
		MessageBox("Invalid Local port, should be 1~65535!", "Error", MB_OK);
		return;
	}
	cfginfo.local_port = (short)tmp;

	m_edt_vpn_port.GetWindowTextA(strTmp);
	tmp = _tcstoul(strTmp, NULL, 10);
	if (tmp <= 0 || tmp > 65535)
	{
		MessageBox("Invalid vpn port, should be 1~65535!", "Error", MB_OK);
		return;
	}
	cfginfo.vpn_port = (short)tmp;

	m_cmbServers.GetWindowTextA(strTmp);
	int iptmp = inet_addr(strTmp.GetBuffer(0));
	if (-1 == iptmp)
	{
		cfginfo.server_ip = 0;
	}
	else
	{
		cfginfo.server_ip = ntohl(iptmp);
	}

	m_vpn_ipaddr.GetWindowTextA(strTmp);
	cfginfo.vpn_ip = inet_addr(strTmp.GetBuffer(0));
	cfginfo.vpn_ip = ntohl(cfginfo.vpn_ip);

	cfginfo.proxy_type = m_cmb_proxy_type.GetCurSel() + 1;
	cfginfo.encry_type = m_cmb_encry_type.GetCurSel() + 1;

	m_edt_username.GetWindowText(strTmp);
	strncpy(&cfginfo.username[0], strTmp.GetBuffer(0), 64);
	cfginfo.username[63] = 0;

	m_edt_passwd.GetWindowText(strTmp);
	strncpy(&cfginfo.passwd[0], strTmp.GetBuffer(0), 64);
	cfginfo.passwd[63] = 0;

	if (m_chk_auth.GetCheck() == 0)
	{
		cfginfo.is_auth = false;
	}
	else
	{
		cfginfo.is_auth = true;
	}

	m_edt_encry_key.GetWindowText(strTmp);
	strncpy(&cfginfo.encry_key[0], strTmp.GetBuffer(0), 64);
	cfginfo.encry_key[63] = 0;

	/*vpn only support socks5*/
	if (cfginfo.proxy_type == PROXY_VPN)
	{
		if (cfginfo.proxy_proto == SOCKS_4)
		{
			MessageBox("Only socks5 support in VPN mode!", "Error", MB_OK);
			return;
		}
	}

	if (0 == proxy_cfg_set(&cfginfo))
	{
		MessageBox("配置保存成功!", "提示", MB_OK);
	}
	else
	{
		MessageBox("配置保存失败!", "提示", MB_OK);
	}

	/*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_IP_LIST, NULL, NULL);
	return;
}

void CGoProxyDlg::OnCbnSelchangeComboProxyType()
{
	// TODO: Add your control notification handler code here
	int proxy_type = m_cmb_proxy_type.GetCurSel() + 1;
	if (proxy_type == PROXY_VPN)
	{
		m_cmbServers.ShowWindow(SW_SHOW);
		m_stc_proxy_server.ShowWindow(SW_SHOW);
		m_btnSrvFresh.ShowWindow(SW_SHOW);

		m_server_static.SetWindowText(_T("VPN服务器:"));
	}
	else
	{
		m_cmbServers.ShowWindow(SW_HIDE);
		m_stc_proxy_server.ShowWindow(SW_HIDE);
		m_btnSrvFresh.ShowWindow(SW_HIDE);

		m_server_static.SetWindowText(_T("代理服务器:"));
	}
}

void CGoProxyDlg::OnCbnSelchangeComboEncryType()
{
	// TODO: Add your control notification handler code here
	int encry_type = m_cmb_encry_type.GetCurSel() + 1;
	if (encry_type == ENCRY_NONE)
	{
		m_edt_encry_key.EnableWindow(FALSE);
	}
	else
	{
		m_edt_encry_key.EnableWindow(TRUE);
	}
}

void CGoProxyDlg::OnBnClickedChkAuth()
{
	// TODO: Add your control notification handler code here
	if (m_chk_auth.GetCheck())
	{
		m_edt_username.EnableWindow(TRUE);
		m_edt_passwd.EnableWindow(TRUE);
	}
	else
	{
		m_edt_username.EnableWindow(FALSE);
		m_edt_passwd.EnableWindow(FALSE);
	}
}


void CGoProxyDlg::OnBnClickedBtnSrvFresh()
{
	// TODO: Add your control notification handler code here
}
