
// HomerDlg.h : header file
//

#pragma once


// CHomerDlg dialog
class CHomerDlg : public CDialogEx
{
// Construction
public:
	CHomerDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CHomerDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HOMER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	HICON m_hCopyIcon;
	HBITMAP m_hLogoBitmap;
	bool m_bUpdatingLicenseKey = false;

	CString GetDeviceFingerprint();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLicenseKeyPartChanged();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedBtnCopy();
};
