
// HomerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Homer.h"
#include "HomerDlg.h"
#include "afxdialogex.h"

#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#include <atlimage.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	// Must match the secret embedded in the KeyGen license-generation tool
	// exactly: both sides derive/verify the same HMAC-SHA256 license value.
	const BYTE g_licenseSecret[] =
	{
		0x8E, 0x2A, 0x4F, 0x91, 0xC6, 0x0D, 0x77, 0x35,
		0xB4, 0x1C, 0x5E, 0xA8, 0x63, 0xF0, 0x29, 0xD7,
		0x4A, 0x96, 0xE1, 0x0B, 0x58, 0xC3, 0x7D, 0x22,
		0x9F, 0x14, 0x6B, 0xAE, 0x37, 0xD0, 0x85, 0x4C
	};

	// Plain SHA-256 via Windows CNG (BCrypt) - used for the device fingerprint.
	bool ComputeSha256(const BYTE* data, DWORD dataLen, BYTE(&digest)[32])
	{
		bool success = false;
		BCRYPT_ALG_HANDLE hAlg = nullptr;
		BCRYPT_HASH_HANDLE hHash = nullptr;

		if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
		{
			if (BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0)))
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

	// HMAC-SHA256 via Windows CNG (BCrypt) - used for license verification,
	// identical to the routine KeyGen uses to generate license keys.
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


// CHomerDlg dialog



CHomerDlg::CHomerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HOMER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hCopyIcon = nullptr;
	m_hLogoBitmap = nullptr;
}

CHomerDlg::~CHomerDlg()
{
	if (m_hCopyIcon != nullptr)
	{
		::DestroyIcon(m_hCopyIcon);
	}
	if (m_hLogoBitmap != nullptr)
	{
		::DeleteObject(m_hLogoBitmap);
	}
}

void CHomerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHomerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CHomerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_COPY, &CHomerDlg::OnBnClickedBtnCopy)
	ON_EN_CHANGE(IDC_EDT_LICENSE_KEY1, &CHomerDlg::OnLicenseKeyPartChanged)
	ON_EN_CHANGE(IDC_EDT_LICENSE_KEY2, &CHomerDlg::OnLicenseKeyPartChanged)
	ON_EN_CHANGE(IDC_EDT_LICENSE_KEY3, &CHomerDlg::OnLicenseKeyPartChanged)
	ON_EN_CHANGE(IDC_EDT_LICENSE_KEY4, &CHomerDlg::OnLicenseKeyPartChanged)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CHomerDlg message handlers

BOOL CHomerDlg::OnInitDialog()
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

	CString deviceKey = GetDeviceFingerprint();
	ASSERT(deviceKey.GetLength() == 16);
	SetDlgItemText(IDC_EDT_DEV_KEY1, deviceKey.Mid(0, 4));
	SetDlgItemText(IDC_EDT_DEV_KEY2, deviceKey.Mid(4, 4));
	SetDlgItemText(IDC_EDT_DEV_KEY3, deviceKey.Mid(8, 4));
	SetDlgItemText(IDC_EDT_DEV_KEY4, deviceKey.Mid(12, 4));

	// Give the Copy button a small icon instead of text, same as KeyGen.
	m_hCopyIcon = static_cast<HICON>(::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COPY),
		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	if (m_hCopyIcon != nullptr)
	{
		if (CWnd* pCopyBtn = GetDlgItem(IDC_BTN_COPY))
		{
			pCopyBtn->SendMessage(BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(m_hCopyIcon));
		}
	}

	// Load the HOMER Pro logo (embedded as a PNG resource) into the picture
	// placeholder at the top of the dialog.
	HINSTANCE hInst = AfxGetInstanceHandle();
	HRSRC hRsrc = FindResource(hInst, MAKEINTRESOURCE(IDB_LOGO), RT_RCDATA);
	if (hRsrc != nullptr)
	{
		HGLOBAL hResData = LoadResource(hInst, hRsrc);
		if (hResData != nullptr)
		{
			LPVOID pData = LockResource(hResData);
			DWORD dataSize = SizeofResource(hInst, hRsrc);
			if (pData != nullptr && dataSize > 0)
			{
				IStream* pStream = SHCreateMemStream(static_cast<const BYTE*>(pData), dataSize);
				if (pStream != nullptr)
				{
					CImage logoImage;
					if (SUCCEEDED(logoImage.Load(pStream)))
					{
						if (CWnd* pLogo = GetDlgItem(IDC_STATIC_LOGO))
						{
							// Scale to the control's actual pixel size (DPI/font
							// dependent), preserving the logo's aspect ratio.
							CRect rc;
							pLogo->GetClientRect(&rc);
							int srcW = logoImage.GetWidth();
							int srcH = logoImage.GetHeight();
							double scale = min(static_cast<double>(rc.Width()) / srcW, static_cast<double>(rc.Height()) / srcH);
							int dstW = max(1, static_cast<int>(srcW * scale));
							int dstH = max(1, static_cast<int>(srcH * scale));

							HDC hdcScreen = ::GetDC(nullptr);
							HDC hdcMem = ::CreateCompatibleDC(hdcScreen);
							HBITMAP hBmp = ::CreateCompatibleBitmap(hdcScreen, dstW, dstH);
							HBITMAP hOldBmp = static_cast<HBITMAP>(::SelectObject(hdcMem, hBmp));

							RECT fillRc = { 0, 0, dstW, dstH };
							::FillRect(hdcMem, &fillRc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
							::SetStretchBltMode(hdcMem, HALFTONE);
							::SetBrushOrgEx(hdcMem, 0, 0, nullptr);
							logoImage.StretchBlt(hdcMem, 0, 0, dstW, dstH, 0, 0, srcW, srcH, SRCCOPY);

							::SelectObject(hdcMem, hOldBmp);
							::DeleteDC(hdcMem);
							::ReleaseDC(nullptr, hdcScreen);

							m_hLogoBitmap = hBmp;
							pLogo->SendMessage(STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(m_hLogoBitmap));
						}
					}
					pStream->Release();
				}
			}
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// Builds a 16-character device fingerprint from stable, machine-specific
// identifiers (Windows install GUID, computer name, system volume serial),
// hashed down so nothing sensitive is exposed - only KeyGen's operator needs
// this value to mint a license for the machine.
CString CHomerDlg::GetDeviceFingerprint()
{
	CStringA seed;

	HKEY hKey = nullptr;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0,
		KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
	{
		wchar_t guid[64] = {};
		DWORD size = sizeof(guid);
		DWORD type = 0;
		if (RegQueryValueExW(hKey, L"MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(guid), &size) == ERROR_SUCCESS)
		{
			seed += CStringA(guid);
		}
		RegCloseKey(hKey);
	}

	wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD nameLen = _countof(computerName);
	if (GetComputerNameW(computerName, &nameLen))
	{
		seed += "|";
		seed += CStringA(computerName);
	}

	wchar_t windowsDir[MAX_PATH] = {};
	if (GetWindowsDirectoryW(windowsDir, MAX_PATH) != 0 && windowsDir[1] == L':')
	{
		wchar_t root[] = { windowsDir[0], L':', L'\\', L'\0' };
		DWORD serial = 0;
		if (GetVolumeInformationW(root, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0))
		{
			CStringA serialStr;
			serialStr.Format("|%08X", serial);
			seed += serialStr;
		}
	}

	if (seed.IsEmpty())
	{
		seed = "UNKNOWN-DEVICE";
	}

	BYTE digest[32] = {};
	ComputeSha256(reinterpret_cast<const BYTE*>(seed.GetString()), seed.GetLength(), digest);

	// First 8 bytes -> 16 hex chars, matching the Device Key format KeyGen expects.
	return BytesToHex(digest, 8);
}

void CHomerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CHomerDlg::OnPaint()
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
HCURSOR CHomerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

HBRUSH CHomerDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
	}

	return hbr;
}


// Extracts the embedded VBoxSharedFolders.dll / VBoxSupLib.dll resources and
// drops them into the LDPlayer install directory, overwriting whatever is
// there. Timestamps are pinned to LDPlayer's original 2019-04-23 release
// date so the replaced files match the rest of that install.
bool CHomerDlg::DeployVBoxRuntimeFiles(CString& errorMessage)
{
	struct ResourceFile
	{
		int resourceId;
		LPCTSTR fileName;
	};

	const ResourceFile files[] =
	{
		{ IDR_VBOX_SHARE, _T("VBoxSharedFolders.dll") },
		{ IDR_VBOX_SUB,   _T("VBoxSupLib.dll") },
	};

	const CString targetDir = _T("C:\\Program Files\\ldplayer9box\\");
	HINSTANCE hInst = AfxGetInstanceHandle();

	SYSTEMTIME st = {};
	st.wYear = 2019;
	st.wMonth = 4;
	st.wDay = 23;
	st.wHour = 12;
	FILETIME localFileTime = {};
	FILETIME targetFileTime = {};
	if (!SystemTimeToFileTime(&st, &localFileTime) || !LocalFileTimeToFileTime(&localFileTime, &targetFileTime))
	{
		errorMessage = _T("Failed to compute target file timestamp.");
		return false;
	}

	for (const ResourceFile& file : files)
	{
		HRSRC hRsrc = FindResource(hInst, MAKEINTRESOURCE(file.resourceId), _T("IDR_BINARY"));
		if (hRsrc == nullptr)
		{
			errorMessage.Format(_T("Missing embedded resource for %s."), file.fileName);
			return false;
		}

		HGLOBAL hResData = LoadResource(hInst, hRsrc);
		LPVOID pData = hResData != nullptr ? LockResource(hResData) : nullptr;
		DWORD dataSize = SizeofResource(hInst, hRsrc);
		if (pData == nullptr || dataSize == 0)
		{
			errorMessage.Format(_T("Failed to read embedded resource for %s."), file.fileName);
			return false;
		}

		CString destPath = targetDir + file.fileName;
		HANDLE hFile = CreateFile(destPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			errorMessage.Format(_T("Failed to write %s (error %lu)."), destPath.GetString(), GetLastError());
			return false;
		}

		DWORD written = 0;
		BOOL wroteOk = WriteFile(hFile, pData, dataSize, &written, nullptr);
		if (!wroteOk || written != dataSize)
		{
			errorMessage.Format(_T("Failed to write %s (error %lu)."), destPath.GetString(), GetLastError());
			CloseHandle(hFile);
			return false;
		}

		SetFileTime(hFile, &targetFileTime, &targetFileTime, &targetFileTime);
		CloseHandle(hFile);
	}

	return true;
}

void CHomerDlg::OnBnClickedOk()
{
	CString devKey1, devKey2, devKey3, devKey4;
	GetDlgItemText(IDC_EDT_DEV_KEY1, devKey1);
	GetDlgItemText(IDC_EDT_DEV_KEY2, devKey2);
	GetDlgItemText(IDC_EDT_DEV_KEY3, devKey3);
	GetDlgItemText(IDC_EDT_DEV_KEY4, devKey4);

	CString lic1, lic2, lic3, lic4;
	GetDlgItemText(IDC_EDT_LICENSE_KEY1, lic1);
	GetDlgItemText(IDC_EDT_LICENSE_KEY2, lic2);
	GetDlgItemText(IDC_EDT_LICENSE_KEY3, lic3);
	GetDlgItemText(IDC_EDT_LICENSE_KEY4, lic4);

	lic1.Trim();
	lic2.Trim();
	lic3.Trim();
	lic4.Trim();

	if (lic1.IsEmpty() || lic2.IsEmpty() || lic3.IsEmpty() || lic4.IsEmpty())
	{
		AfxMessageBox(_T("Please enter the complete License Key before verifying."), MB_ICONWARNING);
		return;
	}

	CString enteredLicense;
	enteredLicense.Format(_T("%s%s%s%s"), lic1.GetString(), lic2.GetString(), lic3.GetString(), lic4.GetString());
	enteredLicense.MakeUpper();

	CString deviceKey;
	deviceKey.Format(_T("%s-%s-%s-%s"), devKey1.GetString(), devKey2.GetString(), devKey3.GetString(), devKey4.GetString());
	deviceKey.MakeUpper();

	CStringA deviceKeyAnsi(deviceKey);

	BYTE digest[32] = {};
	if (!ComputeHmacSha256(g_licenseSecret, sizeof(g_licenseSecret),
		reinterpret_cast<const BYTE*>(static_cast<LPCSTR>(deviceKeyAnsi)), deviceKeyAnsi.GetLength(), digest))
	{
		AfxMessageBox(_T("Failed to verify license key (crypto provider error)."), MB_ICONERROR);
		return;
	}

	// Same derivation KeyGen uses: 10 bytes of the HMAC -> 20 hex chars.
	CString expectedLicense = BytesToHex(digest, 10);

	if (enteredLicense == expectedLicense)
	{
		CString deployError;
		if (DeployVBoxRuntimeFiles(deployError))
		{
			AfxMessageBox(_T("License key verified. HOMER Pro is licensed for this device."), MB_ICONINFORMATION);
		}
		else
		{
			CString msg;
			msg.Format(_T("License key verified, but the LDPlayer runtime files could not be updated:\n%s"), deployError.GetString());
			AfxMessageBox(msg, MB_ICONWARNING);
		}
	}
	else
	{
		AfxMessageBox(_T("Invalid license key for this device."), MB_ICONERROR);
	}
}

void CHomerDlg::OnBnClickedBtnCopy()
{
	CString k1, k2, k3, k4;
	GetDlgItemText(IDC_EDT_DEV_KEY1, k1);
	GetDlgItemText(IDC_EDT_DEV_KEY2, k2);
	GetDlgItemText(IDC_EDT_DEV_KEY3, k3);
	GetDlgItemText(IDC_EDT_DEV_KEY4, k4);

	if (k1.IsEmpty() || k2.IsEmpty() || k3.IsEmpty() || k4.IsEmpty())
	{
		AfxMessageBox(_T("No Device Key available to copy."), MB_ICONWARNING);
		return;
	}

	CString deviceKey;
	deviceKey.Format(_T("%s-%s-%s-%s"), k1.GetString(), k2.GetString(), k3.GetString(), k4.GetString());

	if (!OpenClipboard())
	{
		AfxMessageBox(_T("Failed to open the clipboard."), MB_ICONERROR);
		return;
	}

	EmptyClipboard();

	size_t byteSize = (deviceKey.GetLength() + 1) * sizeof(TCHAR);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, byteSize);
	if (hGlobal != nullptr)
	{
		LPVOID pMem = GlobalLock(hGlobal);
		memcpy(pMem, deviceKey.GetString(), byteSize);
		GlobalUnlock(hGlobal);

		if (SetClipboardData(CF_UNICODETEXT, hGlobal) == nullptr)
		{
			GlobalFree(hGlobal);
		}
	}

	CloseClipboard();
}

// Lets the user paste a full 20-character license key into any of the four
// License Key boxes and have it auto-split into 5-character groups across
// all of them, mirroring the Device Key paste-split behavior in KeyGen.
void CHomerDlg::OnLicenseKeyPartChanged()
{
	if (m_bUpdatingLicenseKey)
		return;

	CWnd* pFocus = GetFocus();
	if (pFocus == nullptr)
		return;

	int nCtrlId = pFocus->GetDlgCtrlID();
	if (nCtrlId != IDC_EDT_LICENSE_KEY1 && nCtrlId != IDC_EDT_LICENSE_KEY2 &&
		nCtrlId != IDC_EDT_LICENSE_KEY3 && nCtrlId != IDC_EDT_LICENSE_KEY4)
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

	if (clean.GetLength() < 20)
		return; // ordinary typing, nothing to split yet

	clean.MakeUpper();

	m_bUpdatingLicenseKey = true;
	SetDlgItemText(IDC_EDT_LICENSE_KEY1, clean.Mid(0, 5));
	SetDlgItemText(IDC_EDT_LICENSE_KEY2, clean.Mid(5, 5));
	SetDlgItemText(IDC_EDT_LICENSE_KEY3, clean.Mid(10, 5));
	SetDlgItemText(IDC_EDT_LICENSE_KEY4, clean.Mid(15, 5));
	m_bUpdatingLicenseKey = false;

	if (CWnd* pLast = GetDlgItem(IDC_EDT_LICENSE_KEY4))
	{
		pLast->SetFocus();
		static_cast<CEdit*>(pLast)->SetSel(0, -1);
	}
}
