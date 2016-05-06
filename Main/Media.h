#if !defined(AFX_MEDIA_H__5872EB23_1BE6_498E_9F2E_8E0ACF10B6B9__INCLUDED_)
#define AFX_MEDIA_H__5872EB23_1BE6_498E_9F2E_8E0ACF10B6B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Media.h : main header file for MEDIA.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMediaApp : See Media.cpp for implementation.

class CMediaApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MEDIA_H__5872EB23_1BE6_498E_9F2E_8E0ACF10B6B9__INCLUDED)
