
// KeyGenDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "KeyGen.h"
#include "KeyGenDlg.h"
#include "afxdialogex.h"

#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	// Secret shared with the license-verification code. Anyone holding this
	// value can mint valid licenses, so it must never ship in a public build.
	const BYTE g_licenseSecret[] =
	{
		0x8E, 0x2A, 0x4F, 0x91, 0xC6, 0x0D, 0x77, 0x35,
		0xB4, 0x1C, 0x5E, 0xA8, 0x63, 0xF0, 0x29, 0xD7,
		0x4A, 0x96, 0xE1, 0x0B, 0x58, 0xC3, 0x7D, 0x22,
		0x9F, 0x14, 0x6B, 0xAE, 0x37, 0xD0, 0x85, 0x4C
	};

	// HMAC-SHA256 via Windows CNG (BCrypt) - no external crypto dependency.
	bool ComputeHmacSha256(const BYTE* key, DWORD keyLen, const BYTE* data, DWORD dataLen, BYTE(&digest)[32])
	{
		bool success = false;
		BCRYPT_ALG_HANDLE hAlg = nullptr;
		BCRYPT_HASH_HANDLE hHash = nullptr;

		if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG)))
		{
			if (BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, const_cast<PUCHAR>(key), keyLen, 0)))
			{
				if (BCRYPT_SUCCESS(BCryptHashData(hHash, const_cast<PUCHAR>(data), dataLen, 0)) &&
					BCRYPT_SUCCESS(BCryptFinishHash(hHash, digest, sizeof(digest), 0)))
				{
					success = true;
				}
				BCryptDestroyHash(hHash);
			}
			BCryptCloseAlgorithmProvider(hAlg, 0);
		}
		return success;
	}

	CString BytesToHex(const BYTE* data, size_t len)
	{
		static const wchar_t* hexDigits = L"0123456789ABCDEF";
		CString result;
		for (size_t i = 0; i < len; ++i)
		{
			result.AppendChar(hexDigits[(data[i] >> 4) & 0x0F]);
			result.AppendChar(hexDigits[data[i] & 0x0F]);
		}
		return result;
	}
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CKeyGenDlg dialog



CKeyGenDlg::CKeyGenDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_KEYGEN_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hCopyIcon = nullptr;
}

CKeyGenDlg::~CKeyGenDlg()
{
	if (m_hCopyIcon != nullptr)
	{
		::DestroyIcon(m_hCopyIcon);
	}
}

void CKeyGenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKeyGenDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CKeyGenDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDT_DEV_KEY1, &CKeyGenDlg::OnDevKeyPartChanged)
	ON_EN_CHANGE(IDC_EDT_DEV_KEY2, &CKeyGenDlg::OnDevKeyPartChanged)
	ON_EN_CHANGE(IDC_EDT_DEV_KEY3, &CKeyGenDlg::OnDevKeyPartChanged)
	ON_EN_CHANGE(IDC_EDT_DEV_KEY4, &CKeyGenDlg::OnDevKeyPartChanged)
	ON_BN_CLICKED(IDC_BTN_COPY, &CKeyGenDlg::OnBnClickedBtnCopy)
END_MESSAGE_MAP()


// CKeyGenDlg message handlers

BOOL CKeyGenDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// Give the Copy button a small icon instead of text.
	m_hCopyIcon = static_cast<HICON>(::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COPY),
		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	if (m_hCopyIcon != nullptr)
	{
		if (CWnd* pCopyBtn = GetDlgItem(IDC_BTN_COPY))
		{
			pCopyBtn->SendMessage(BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(m_hCopyIcon));
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CKeyGenDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CKeyGenDlg::OnPaint()
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
HCURSOR CKeyGenDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CKeyGenDlg::OnBnClickedOk()
{
	CString devKey1, devKey2, devKey3, devKey4;
	GetDlgItemText(IDC_EDT_DEV_KEY1, devKey1);
	GetDlgItemText(IDC_EDT_DEV_KEY2, devKey2);
	GetDlgItemText(IDC_EDT_DEV_KEY3, devKey3);
	GetDlgItemText(IDC_EDT_DEV_KEY4, devKey4);

	devKey1.Trim();
	devKey2.Trim();
	devKey3.Trim();
	devKey4.Trim();

	if (devKey1.IsEmpty() || devKey2.IsEmpty() || devKey3.IsEmpty() || devKey4.IsEmpty())
	{
		AfxMessageBox(_T("Please enter a complete Device Key before generating a license."), MB_ICONWARNING);
		return;
	}

	CString deviceKey;
	deviceKey.Format(_T("%s-%s-%s-%s"), devKey1.GetString(), devKey2.GetString(), devKey3.GetString(), devKey4.GetString());
	deviceKey.MakeUpper();

	// Device key is expected to be plain alphanumeric, so the ANSI conversion is lossless.
	CStringA deviceKeyAnsi(deviceKey);

	BYTE digest[32] = {};
	if (!ComputeHmacSha256(g_licenseSecret, sizeof(g_licenseSecret),
		reinterpret_cast<const BYTE*>(static_cast<LPCSTR>(deviceKeyAnsi)), deviceKeyAnsi.GetLength(), digest))
	{
		AfxMessageBox(_T("Failed to generate license key (crypto provider error)."), MB_ICONERROR);
		return;
	}

	// 10 bytes of the HMAC -> 20 hex chars -> four 5-character groups.
	CString licenseHex = BytesToHex(digest, 10);
	ASSERT(licenseHex.GetLength() == 20);

	SetDlgItemText(IDC_EDT_LICENSE_KEY1, licenseHex.Mid(0, 5));
	SetDlgItemText(IDC_EDT_LICENSE_KEY2, licenseHex.Mid(5, 5));
	SetDlgItemText(IDC_EDT_LICENSE_KEY3, licenseHex.Mid(10, 5));
	SetDlgItemText(IDC_EDT_LICENSE_KEY4, licenseHex.Mid(15, 5));
}

// Lets the user paste a full 16-character device key into any of the four
// boxes and have it auto-split into 4-character groups across all of them.
void CKeyGenDlg::OnDevKeyPartChanged()
{
	if (m_bUpdatingDevKey)
		return;

	CWnd* pFocus = GetFocus();
	if (pFocus == nullptr)
		return;

	int nCtrlId = pFocus->GetDlgCtrlID();
	if (nCtrlId != IDC_EDT_DEV_KEY1 && nCtrlId != IDC_EDT_DEV_KEY2 &&
		nCtrlId != IDC_EDT_DEV_KEY3 && nCtrlId != IDC_EDT_DEV_KEY4)
		return;

	CString text;
	pFocus->GetWindowText(text);

	// Strip separators in case the pasted key already contains dashes/spaces.
	CString clean;
	for (int i = 0; i < text.GetLength(); ++i)
	{
		if (_istalnum(text[i]))
			clean.AppendChar(text[i]);
	}

	if (clean.GetLength() < 16)
		return; // ordinary typing, nothing to split yet

	clean.MakeUpper();

	m_bUpdatingDevKey = true;
	SetDlgItemText(IDC_EDT_DEV_KEY1, clean.Mid(0, 4));
	SetDlgItemText(IDC_EDT_DEV_KEY2, clean.Mid(4, 4));
	SetDlgItemText(IDC_EDT_DEV_KEY3, clean.Mid(8, 4));
	SetDlgItemText(IDC_EDT_DEV_KEY4, clean.Mid(12, 4));
	m_bUpdatingDevKey = false;

	if (CWnd* pLast = GetDlgItem(IDC_EDT_DEV_KEY4))
	{
		pLast->SetFocus();
		static_cast<CEdit*>(pLast)->SetSel(0, -1);
	}
}

void CKeyGenDlg::OnBnClickedBtnCopy()
{
	CString k1, k2, k3, k4;
	GetDlgItemText(IDC_EDT_LICENSE_KEY1, k1);
	GetDlgItemText(IDC_EDT_LICENSE_KEY2, k2);
	GetDlgItemText(IDC_EDT_LICENSE_KEY3, k3);
	GetDlgItemText(IDC_EDT_LICENSE_KEY4, k4);

	if (k1.IsEmpty() || k2.IsEmpty() || k3.IsEmpty() || k4.IsEmpty())
	{
		AfxMessageBox(_T("Generate a license key before copying."), MB_ICONWARNING);
		return;
	}

	CString licenseKey;
	licenseKey.Format(_T("%s-%s-%s-%s"), k1.GetString(), k2.GetString(), k3.GetString(), k4.GetString());

	if (!OpenClipboard())
	{
		AfxMessageBox(_T("Failed to open the clipboard."), MB_ICONERROR);
		return;
	}

	EmptyClipboard();

	size_t byteSize = (licenseKey.GetLength() + 1) * sizeof(TCHAR);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, byteSize);
	if (hGlobal != nullptr)
	{
		LPVOID pMem = GlobalLock(hGlobal);
		memcpy(pMem, licenseKey.GetString(), byteSize);
		GlobalUnlock(hGlobal);

		if (SetClipboardData(CF_UNICODETEXT, hGlobal) == nullptr)
		{
			GlobalFree(hGlobal);
		}
	}

	CloseClipboard();
}
