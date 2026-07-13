
// KeyGen.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CKeyGenApp:
// See KeyGen.cpp for the implementation of this class
//

class CKeyGenApp : public CWinApp
{
public:
	CKeyGenApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CKeyGenApp theApp;
