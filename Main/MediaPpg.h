#if !defined(AFX_MEDIAPPG_H__DAFC863A_368C_483A_8800_F837585C41EE__INCLUDED_)
#define AFX_MEDIAPPG_H__DAFC863A_368C_483A_8800_F837585C41EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// MediaPpg.h : Declaration of the CMediaPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMediaPropPage : See MediaPpg.cpp.cpp for implementation.

class CMediaPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMediaPropPage)
	DECLARE_OLECREATE_EX(CMediaPropPage)

// Constructor
public:
	CMediaPropPage();

// Dialog Data
	//{{AFX_DATA(CMediaPropPage)
	enum { IDD = IDD_PROPPAGE_MEDIA };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMediaPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MEDIAPPG_H__DAFC863A_368C_483A_8800_F837585C41EE__INCLUDED)
