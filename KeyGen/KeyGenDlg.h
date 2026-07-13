
// KeyGenDlg.h : header file
//

#pragma once


// CKeyGenDlg dialog
class CKeyGenDlg : public CDialogEx
{
// Construction
public:
	CKeyGenDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CKeyGenDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KEYGEN_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	HICON m_hCopyIcon;
	bool m_bUpdatingDevKey = false;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedOk();
	afx_msg void OnDevKeyPartChanged();
	afx_msg void OnBnClickedBtnCopy();
	DECLARE_MESSAGE_MAP()
};
