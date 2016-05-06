// MediaPpg.cpp : Implementation of the CMediaPropPage property page class.

#include "stdafx.h"
#include "Media.h"
#include "MediaPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMediaPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMediaPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMediaPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMediaPropPage, "MEDIA.MediaPropPage.1",
	0x7fb23b5, 0x3c36, 0x4ecd, 0xa5, 0x5b, 0x52, 0xf7, 0x94, 0x8d, 0x38, 0x51)


/////////////////////////////////////////////////////////////////////////////
// CMediaPropPage::CMediaPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMediaPropPage

BOOL CMediaPropPage::CMediaPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MEDIA_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMediaPropPage::CMediaPropPage - Constructor

CMediaPropPage::CMediaPropPage() :
	COlePropertyPage(IDD, IDS_MEDIA_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMediaPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMediaPropPage::DoDataExchange - Moves data between page and properties

void CMediaPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMediaPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMediaPropPage message handlers
