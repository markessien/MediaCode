// MediaCtl.cpp : Implementation of the CMediaCtrl ActiveX Control class.

#include "stdafx.h"
#include "Media.h"
#include "MediaCtl.h"
#include "MediaPpg.h"

#include <atlbase.h>

#include <QTML.h>		// QuickTime interface
#include <FixMath.h>
#include <MediaHandlers.h>
#include <math.h>
#include <dbt.h>

#include <uuids.h>
#include "keyprovider.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
#define REGISTER_FILTERGRAPH
#endif

// Global data
DWORD g_dwGraphRegister=0;  // For running object table

// Compute (a * b + rnd) / c
LONGLONG WINAPI llMulDiv(LONGLONG a, LONGLONG b, LONGLONG c, LONGLONG rnd);

// Flag to prevent calling media position timer event recursively
BOOL	bInTimeProc;

// Global key provider object created/released during the
// Windows Media graph-building stage.
CKeyProvider prov;

// For set/reset CDROM Digital Playback setting on Windows 2K/XP
extern void DisableCDROMDigitalPlayback(char *szDrive);
extern void EnableCDROMDigitalPlayback(char *szDrive);

#include <initguid.h>

// Define VU Meter GUID 
DEFINE_GUID(CLSID_VUMeter, 
0xa87a43af, 0x226d, 0x4b85, 0x92, 0xf4, 0xbd, 0x19, 0xe9, 0x2c, 0x91, 0x4e);

// Array to store hard-coded volume for Directshow interface
long	lDXVolumeArray[]={
	-10000, -5700, -5400, -5100, -4800, -4000, -3500, -3100, -2900, -2700,
	-2550, -2400, -2250, -2100, -1950, -1800, -1650, -1550, -1450, -1350,
	-1250, -1150, -1050, -960, -950, -940, -930, -920, -910, -900,
	-890, -870, -860, -850, -840, -830, -820, -810, -800, -790,
	-780, -770, -760, -750, -740, -730, -720, -710, -705, -700,
	-660, -620, -580, -540, -500, -460, -420, -380, -370, -360,
	-351, -332, -314, -297, -280, -264, -248, -233, -218, -204,
	-190, -177, -164, -152, -141, -131, -121, -112, -103, -95,
	-87, -79, -72, -65, -58, -52, -46, -41, -36, -31,
	-27, -23, -19, -16, -13, -10, -8, -6, -4, -2, 0
};

IMPLEMENT_DYNCREATE(CMediaCtrl, COleControl)

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMediaCtrl, COleControl)
	//{{AFX_MSG_MAP(CMediaCtrl)
	ON_WM_CREATE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SETFOCUS()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CMediaCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CMediaCtrl)
	DISP_PROPERTY_EX(CMediaCtrl, "PlayState", GetPlayState, SetPlayState, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "DisplayMode", GetDisplayMode, SetDisplayMode, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "MediaPositionLong", GetMediaPositionLong, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "TimeFormat", GetTimeFormat, SetTimeFormat, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "MediaType", GetMediaType, SetNotSupported, VT_I2)
	DISP_PROPERTY_EX(CMediaCtrl, "MediaLength", GetMediaLength, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "Volume", GetVolume, SetVolume, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "VideoWidth", GetVideoWidth, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "VideoHeight", GetVideoHeight, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "MCIDevID", GetMCIDeviceID, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CMediaCtrl, "Mute", GetMuteState, SetMuteState, VT_BOOL)
	DISP_PROPERTY_EX(CMediaCtrl, "AdvancePercent", GetAdvancePercent, SetAdvancePercent, VT_R8)
	DISP_FUNCTION(CMediaCtrl, "MediaFile", OnChangedMediaFile, VT_I4, VTS_BSTR VTS_BOOL VTS_VARIANT VTS_VARIANT)
	DISP_FUNCTION(CMediaCtrl, "PauseMedia", OnPauseMedia, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "ResumeMedia", OnResumeMedia, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "StopMedia", OnStopMedia, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "GetNumberOfSoundCardMixers", GetNumberOfSoundCardMixers, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "HasASoundCardInstalled", HasASoundCardInstalled, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "MediaSpeed", SetMediaSpeed, VT_R8, VTS_R8)
	DISP_FUNCTION(CMediaCtrl, "SeekTo", SeekTo, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CMediaCtrl, "GetWindowHwnd", GetWindowHwnd, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "FastForward", OnFastForward, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "Play", OnPlay, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "CloseMedia", OnCloseMedia, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "Resize", OnResize, VT_EMPTY, VTS_I4 VTS_I4)
	DISP_FUNCTION(CMediaCtrl, "CheckDXVersion", OnCheckDXVersion, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CMediaCtrl, "Rewind", OnRewind, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CMediaCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CMediaCtrl, COleControl)
	//{{AFX_EVENT_MAP(CMediaCtrl)
	EVENT_CUSTOM("Finished", FireFinished, VTS_NONE)
	EVENT_CUSTOM("FileLoaded", FireFileLoaded, VTS_NONE)
	EVENT_CUSTOM("MediaPosition", FireMediaPosition, VTS_BSTR  VTS_I4)
	EVENT_CUSTOM("VUPeakLevel", FireVUPeakLevel, VTS_I2  VTS_I2  VTS_I2)
	EVENT_CUSTOM("CDMute", FireCDMute, VTS_BOOL)
	EVENT_CUSTOM("CDVolume", FireCDVolume, VTS_I4)
	EVENT_STOCK_CLICK()
	EVENT_STOCK_DBLCLICK()
	EVENT_STOCK_MOUSEUP()
	EVENT_STOCK_MOUSEMOVE()
	EVENT_CUSTOM_ID("Error", DISPID_ERROREVENT, Fire_Error, VTS_I2  VTS_PBSTR  VTS_SCODE  VTS_BSTR  VTS_BSTR  VTS_I4  VTS_PBOOL)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CMediaCtrl, 1)
	PROPPAGEID(CMediaPropPage::guid)
END_PROPPAGEIDS(CMediaCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMediaCtrl, "MEDIA.MediaCtrl.1",
	0x81c410cf, 0xf6b7, 0x4b8a, 0x88, 0xdd, 0xba, 0x3e, 0xe5, 0xcf, 0x87, 0x60)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CMediaCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DMedia =
		{ 0x8af7fbac, 0x88a5, 0x4261, { 0xb5, 0xcc, 0x80, 0x5d, 0x3a, 0x48, 0x87, 0x4c } };
const IID BASED_CODE IID_DMediaEvents =
		{ 0x983a604e, 0x70aa, 0x4a01, { 0xb3, 0xdf, 0xd6, 0x24, 0x91, 0x60, 0x7c, 0x68 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwMediaOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CMediaCtrl, IDS_MEDIA, _dwMediaOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::CMediaCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CMediaCtrl

BOOL CMediaCtrl::CMediaCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_MEDIA,
			IDB_MEDIA,
			afxRegApartmentThreading,
			_dwMediaOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// Licensing strings

static const TCHAR BASED_CODE _szLicFileName[] = _T("Media.lic");

static const WCHAR BASED_CODE _szLicString[] =
	L"Copyright (c) 2002 Mystik Media (3-09151910)";


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::CMediaCtrlFactory::VerifyUserLicense -
// Checks for existence of a user license

BOOL CMediaCtrl::CMediaCtrlFactory::VerifyUserLicense()
{
	return AfxVerifyLicFile(AfxGetInstanceHandle(), _szLicFileName,
		_szLicString);
}

/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::CMediaCtrlFactory::GetLicenseKey -
// Returns a runtime licensing key

BOOL CMediaCtrl::CMediaCtrlFactory::GetLicenseKey(DWORD dwReserved,
	BSTR FAR* pbstrKey)
{
	if (pbstrKey == NULL)
		return FALSE;

	*pbstrKey = SysAllocString(_szLicString);
	return (*pbstrKey != NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::CMediaCtrl - Constructor

CMediaCtrl::CMediaCtrl()
{
	InitializeIIDs(&IID_DMedia, &IID_DMediaEvents);

	// Initialize Control's member variables
	m_lVolume						= VOLUME_FULL;
	m_psCurrent						= Stopped;
	m_lDisplayMode					= Normal;
	m_lMediaLength					= 0;
	m_lMediaPosition				= 0;
	m_bSeekToZeroWhenFinished		= FALSE;
	m_bPlayInLoop					= FALSE;
	m_bHideWindowForAudioOnlyFile	= FALSE;

	// Initialize DirectShow interfaces
	m_pGraph				= NULL;
	m_pMControl				= NULL;
	m_pBasicAudio			= NULL;
	m_pBasicVideo			= NULL;
	m_pMEvent				= NULL;
	m_pMSeek				= NULL;
	m_pVideoWindow			= NULL;
	m_pTeeFilter			= NULL;
	m_pVUMeterFilter		= NULL;

	// Initialize other member variables
	m_pQuickTime			= NULL;
	m_TimerEventID			= NULL;
	m_bQuickTimeInstalled	= FALSE;
	m_QTStopCallBackID		= NULL;
	m_QTCBTimeBase			= NULL;
	m_lVideoWidth			= 0;
	m_lVideoHeight			= 0;
	m_lCheckFinishedCount	= 0;
	m_bSkipCheckFinished	= TRUE;
	m_lLastMediaPos			= 0;
	m_iMCBarHeight			= 0;
	m_hMixer				= 0;
	m_dwVolumeControlID		= 0;
	m_dwMuteControlID		= 0;
	m_lOldCDROMVolume		= 0;
	m_bOldCDROMMuteState	= FALSE;
	m_lOldVolume			= 0;
	m_bOldMuteState			= FALSE;
	m_lGlobalVolume			= 100;
	m_lLastStopPosition		= 0;
	m_dAdvancePercent		= 0.5;

	// Assign pre-set DX volume array for scale [0-100]
	// in physical range (-10000 to 0)
	m_plDXMediaVolumeLookup	= lDXVolumeArray;

	// Initialize QTML and QuickTime interface
	int errCode = InitializeQTML(0);

	if (!errCode)
	{
		errCode = EnterMovies();
		if (!errCode)
		{
			// Successful, initialize a QuickTime class and variables for playback
			m_pQuickTime = new CQuickTime();
			m_bQuickTimeInstalled = TRUE;
		}
	}

	// Set up timer event for reporting media position
	m_TimerEventID = timeSetEvent(500, 0, MediaPositionTimerProc, (DWORD)this, TIME_PERIODIC);
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::~CMediaCtrl - Destructor

CMediaCtrl::~CMediaCtrl()
{
	if (m_TimerEventID)
		timeKillEvent(m_TimerEventID);

	if (m_bQuickTimeInstalled == TRUE)
	{
		// Exit QuickTime and terminate QTML
		ExitMovies();
		TerminateQTML();

		if (m_pQuickTime) 
 			delete m_pQuickTime;
	}

	// Free up any DirectShow resources
	FreePlaybackResources();
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::OnDraw - Drawing function

void CMediaCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	CBrush brAmbientBack(TranslateColor(AmbientBackColor()));
	pdc->FillRect(rcBounds, &brAmbientBack);

	if (m_bQuickTimeInstalled == TRUE)
	{
		if (m_pQuickTime)
		{
			if (m_pQuickTime->theMovie)
				UpdateMovie(m_pQuickTime->theMovie);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::DoPropExchange - Persistence support

void CMediaCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// Set the defaults for the control's properties
	PX_Long(pPX, _T("DisplayMode"), (long&)m_lDisplayMode, Normal);
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::OnResetState - Reset control to default state

void CMediaCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl::AboutBox - Display an "About" box to the user

void CMediaCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_MEDIA);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl message handlers

int CMediaCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	MIXERCAPS	mixCaps;
	HMIXER		hMixer;

	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Clear the background of the control
	ClearControl();

	// Register the window to receive MM_MIXM_CONTROL_CHANGE window message
	int nNumDevices = mixerGetNumDevs();

	if (nNumDevices < 1)
	{
		// If no sound card is installed, fire Error event
		Fire_Error(mediaErrorNoSoundCardInstalled, "No sound card is installed.");
	}

	// Get sound card details
	if (mixerGetDevCaps(0, &mixCaps, sizeof(MIXERCAPS)) == MMSYSERR_NOERROR)
	{
		// Open sound card device
		if (mixerOpen(&hMixer, 0, (DWORD)m_hWnd, 0,
				CALLBACK_WINDOW | MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR)
		{
			short	nVolume;
			BOOL	bMute;

			// Save Mixer handle for later windows callback message MM_MIXM_CONTROL_CHANGE
			m_hMixer = hMixer;
			GetCdromVolume(&nVolume);
			GetCdromMuteState(&bMute);

			m_lOldCDROMVolume = nVolume;
			m_bOldCDROMMuteState = bMute;
		}
		else
		{
			Fire_Error(mediaErrorGetSoundCardDetails, "Failed to open any sound card device.");
		}
	}
	else
	{
		Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get capabilities of the sound card");
	}
	return 0;
}

HRESULT CMediaCtrl::HandleGraphEvent()
{
    LONG evCode;
    ULONG evParam1, evParam2;
    HRESULT hr=S_OK;

    // Make sure that we don't access the media event interface
    // after it has already been released.
    if (!m_pMEvent)
	    return S_OK;

    // Process all queued events
    while(SUCCEEDED(m_pMEvent->GetEvent(&evCode, (LONG_PTR *) &evParam1,
                    (LONG_PTR *) &evParam2, 0)))
    {
        // Free memory associated with callback, since we're not using it
        hr = m_pMEvent->FreeEventParams(evCode, evParam1, evParam2);

		if (evCode == EC_VUMETERLEVEL)
		{
			// Audio VU meter levels and media position combined event
			long						lValue;
			short						iLeftLevel, iRightLevel;
			long						lPos;
			char						szTime[80];
			LPVUMeterCallbackStruc		lpVUMeterCBStruc;

			lpVUMeterCBStruc = (LPVUMeterCallbackStruc)evParam2;

			// Get VU Levels on left and right channels
			lValue = lpVUMeterCBStruc->ulVUMeterLevels;

			iLeftLevel = (short)(lValue >> 16);
			iRightLevel = (short)(lValue & 0xFF);

			lPos = (long)llMulDiv(1, lpVUMeterCBStruc->rtSampleStartTime,
									10000000, 0) * 1000;
			lPos += m_lLastStopPosition;
			m_lMediaPosition = lPos;

			wsprintf(szTime, "%02d:%02d:%02d", lPos/1440000, lPos/60000, (lPos/1000)%60);

			// Notify VU meter levels
			FireVUPeakLevel((short)evParam1, iLeftLevel, iRightLevel);
			
			// Notify media position
			FireMediaPosition((LPCTSTR)szTime, lPos);

			// Free allocated memory for input parameter (evParam2)
			GlobalFree(lpVUMeterCBStruc);
			break;
		}
		else if (evCode == EC_MEDIAPOS)
		{
			// Media position event for video-only media file
			long						lPos;
			char						szTime[80];
			LPVUMeterCallbackStruc		lpVUMeterCBStruc;

			// Decode input parameter
			lpVUMeterCBStruc = (LPVUMeterCallbackStruc)evParam2;

			lPos = (long)llMulDiv(1, lpVUMeterCBStruc->rtSampleStartTime,
									10000000, 0) * 1000;
			lPos += m_lLastStopPosition;
			m_lMediaPosition = lPos;

			wsprintf(szTime, "%02d:%02d:%02d", lPos/1440000, lPos/60000, (lPos/1000)%60);

			// Notify media position
			FireMediaPosition((LPCTSTR)szTime, lPos);

			// Free allocated memory for input parameter (evParam2)
			GlobalFree(lpVUMeterCBStruc);
			break;
		}
        else if (evCode == EC_COMPLETE)
        {
			// Playback completed event
			if (m_bPlayInLoop == TRUE)
			{
				// Playback again in a loop
				
				// Reset to beginning
				LONGLONG pos=0;

				// Reset to first frame of movie
				hr = m_pMSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning ,
											   NULL, AM_SEEKING_NoPositioning);
				if (FAILED(hr))
				{
					// Some custom filters (like the Windows CE MIDI filter)
					// may not implement seeking interfaces (IMediaSeeking)
					// to allow seeking to the start.  In that case, just stop
					// and restart for the same effect.  This should not be
					// necessary in most cases.
					if (FAILED(hr = m_pMControl->Stop()))
					{
						break;
					}

					if (FAILED(hr = m_pMControl->Run()))
					{
						break;
					}
				}
			}
			else
			{
				// Media playback is finished/stopped
				m_psCurrent = Stopped;

				if (m_bSeekToZeroWhenFinished == TRUE)
				{
					// If this is the end of the clip, reset to beginning
					LONGLONG pos=0;

					// Reset to first frame of movie
					hr = m_pMSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning ,
												   NULL, AM_SEEKING_NoPositioning);
					if (FAILED(hr))
					{
						// Some custom filters (like the Windows CE MIDI filter)
						// may not implement seeking interfaces (IMediaSeeking)
						// to allow seeking to the start.  In that case, just stop
						// and restart for the same effect.  This should not be
						// necessary in most cases.
						if (FAILED(hr = m_pMControl->Stop()))
						{
							break;
						}

						if (FAILED(hr = m_pMControl->Run()))
						{
							break;
						}
					}
				}

				if (FAILED(hr = m_pMControl->Stop()))
				{
					break;
				}
				FireFinished();
				m_bSkipCheckFinished = TRUE;
				m_lLastStopPosition = 0;
			}
        }
		else if (evCode == EC_ERRORABORT)
        {
            char szErrMsg[1000];

			AMGetErrorText(evParam1, szErrMsg, 1000);
			TRACE(szErrMsg);
            break;
        }
    }

    return hr;
}

/*   Compute (a * b + d) / c */
LONGLONG WINAPI llMulDiv(LONGLONG a, LONGLONG b, LONGLONG c, LONGLONG d) {
    /*  Compute the absolute values to avoid signed arithmetic problems */
    ULARGE_INTEGER ua, ub;
    DWORDLONG uc;

    ua.QuadPart = (DWORDLONG)(a >= 0 ? a : -a);
    ub.QuadPart = (DWORDLONG)(b >= 0 ? b : -b);
    uc          = (DWORDLONG)(c >= 0 ? c : -c);
    BOOL bSign = (a < 0) ^ (b < 0);

    /*  Do long multiplication */
    ULARGE_INTEGER p[2];
    p[0].QuadPart  = UInt32x32To64(ua.LowPart, ub.LowPart);

    /*  This next computation cannot overflow into p[1].HighPart because
    the max number we can compute here is:

    (2 ** 32 - 1) * (2 ** 32 - 1) +  // ua.LowPart * ub.LowPart
    (2 ** 32) *  (2 ** 31) * (2 ** 32 - 1) * 2    // x.LowPart * y.HighPart * 2

    == 2 ** 96 - 2 ** 64 + (2 ** 64 - 2 ** 33 + 1)
    == 2 ** 96 - 2 ** 33 + 1
    < 2 ** 96
    */

    ULARGE_INTEGER x;
    x.QuadPart     = UInt32x32To64(ua.LowPart, ub.HighPart) +
        UInt32x32To64(ua.HighPart, ub.LowPart) +
        p[0].HighPart;
    p[0].HighPart  = x.LowPart;
    p[1].QuadPart  = UInt32x32To64(ua.HighPart, ub.HighPart) + x.HighPart;

    if(d != 0) {
        ULARGE_INTEGER ud[2];
        if(bSign) {
            ud[0].QuadPart = (DWORDLONG)(-d);
            if(d > 0) {
                /*  -d < 0 */
                ud[1].QuadPart = (DWORDLONG)(LONGLONG)-1;
            }
            else {
                ud[1].QuadPart = (DWORDLONG)0;
            }
        }
        else {
            ud[0].QuadPart = (DWORDLONG)d;
            if(d < 0) {
                ud[1].QuadPart = (DWORDLONG)(LONGLONG)-1;
            }
            else {
                ud[1].QuadPart = (DWORDLONG)0;
            }
        }
        /*  Now do extended addition */
        ULARGE_INTEGER uliTotal;

        /*  Add ls DWORDs */
        uliTotal.QuadPart  = (DWORDLONG)ud[0].LowPart + p[0].LowPart;
        p[0].LowPart       = uliTotal.LowPart;

        /*  Propagate carry */
        uliTotal.LowPart   = uliTotal.HighPart;
        uliTotal.HighPart  = 0;

        /*  Add 2nd most ls DWORDs */
        uliTotal.QuadPart += (DWORDLONG)ud[0].HighPart + p[0].HighPart;
        p[0].HighPart      = uliTotal.LowPart;

        /*  Propagate carry */
        uliTotal.LowPart   = uliTotal.HighPart;
        uliTotal.HighPart  = 0;

        /*  Add MS DWORDLONGs - no carry expected */
        p[1].QuadPart     += ud[1].QuadPart + uliTotal.QuadPart;

        /*  Now see if we got a sign change from the addition */
        if((LONG)p[1].HighPart < 0) {
            bSign = !bSign;

            /*  Negate the current value (ugh!) */
            p[0].QuadPart  = ~p[0].QuadPart;
            p[1].QuadPart  = ~p[1].QuadPart;
            p[0].QuadPart += 1;
            p[1].QuadPart += (p[0].QuadPart == 0);
        }
    }

    /*  Now for the division */
    if(c < 0) {
        bSign = !bSign;
    }


    /*  This will catch c == 0 and overflow */
    if(uc <= p[1].QuadPart) {
        return bSign ? (LONGLONG)0x8000000000000000 :
        (LONGLONG)0x7FFFFFFFFFFFFFFF;
    }

    DWORDLONG ullResult;

    /*  Do the division */
    /*  If the dividend is a DWORD_LONG use the compiler */
    if(p[1].QuadPart == 0) {
        ullResult = p[0].QuadPart / uc;
        return bSign ? -(LONGLONG)ullResult : (LONGLONG)ullResult;
    }

    /*  If the divisor is a DWORD then its simpler */
    ULARGE_INTEGER ulic;
    ulic.QuadPart = uc;
    if(ulic.HighPart == 0) {
        ULARGE_INTEGER uliDividend;
        ULARGE_INTEGER uliResult;
        DWORD dwDivisor = (DWORD)uc;
        // ASSERT(p[1].HighPart == 0 && p[1].LowPart < dwDivisor);
        uliDividend.HighPart = p[1].LowPart;
        uliDividend.LowPart = p[0].HighPart;
#ifndef USE_LARGEINT
        uliResult.HighPart = (DWORD)(uliDividend.QuadPart / dwDivisor);
        p[0].HighPart = (DWORD)(uliDividend.QuadPart % dwDivisor);
        uliResult.LowPart = 0;
        uliResult.QuadPart = p[0].QuadPart / dwDivisor + uliResult.QuadPart;
#else
        /*  NOTE - this routine will take exceptions if
        the result does not fit in a DWORD
        */
        if(uliDividend.QuadPart >= (DWORDLONG)dwDivisor) {
            uliResult.HighPart = EnlargedUnsignedDivide(uliDividend,
                dwDivisor,
                &p[0].HighPart);
        }
        else {
            uliResult.HighPart = 0;
        }
        uliResult.LowPart = EnlargedUnsignedDivide(p[0],
            dwDivisor,
            NULL);
#endif
        return bSign ? -(LONGLONG)uliResult.QuadPart :
        (LONGLONG)uliResult.QuadPart;
    }


    ullResult = 0;

    /*  OK - do long division */
    for(int i = 0; i < 64; i++) {
        ullResult <<= 1;

        /*  Shift 128 bit p left 1 */
        p[1].QuadPart <<= 1;
        if((p[0].HighPart & 0x80000000) != 0) {
            p[1].LowPart++;
        }
        p[0].QuadPart <<= 1;

        /*  Compare */
        if(uc <= p[1].QuadPart) {
            p[1].QuadPart -= uc;
            ullResult += 1;
        }
    }

    return bSign ? - (LONGLONG)ullResult : (LONGLONG)ullResult;
}

LRESULT CMediaCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (m_CurrentPlaybackInterface == QuickTime)
	{
		if (message == WM_ERASEBKGND)
		{
			LRESULT theResult = COleControl::WindowProc(message, wParam, lParam);
			if (GetNativeWindowPort(m_hWnd))
			{
				m_pQuickTime->ProcessMovieEvent(m_hWnd, message, wParam, lParam);	
			}
			return theResult;
		}
		else if (message == WM_MEDIAPOSITION)
		{
			char		szTime[80];
			long		lPos;
			TimeValue	lTimeValue;
			TimeRecord	CurTimeRecord;

			if (m_bMediaLoaded == FALSE)
			{
				return 0L;
			}

			if (m_psCurrent != Playing)
			{
				return 0L;
			}

			if (m_pQuickTime)
			{
				if (m_pQuickTime->theMovie)
				{
					USES_CONVERSION;

					// Get Current Time
					lTimeValue = GetMovieTime(m_pQuickTime->theMovie, &CurTimeRecord);

					lPos = (long)llMulDiv(1000, lTimeValue, CurTimeRecord.scale, 0);

					m_lMediaPosition = lPos;

					wsprintf(szTime, "%02d:%02d:%02d", lPos/1440000, lPos/60000, (lPos/1000)%60);

					// Fire the event via a public method because we cannot fire the control's
					// event directly
					FireMediaPosition((LPCTSTR)szTime, lPos);

					if (m_bSkipCheckFinished == FALSE)
					{
						if (m_lLastMediaPos == lPos)
						{
							if (m_lCheckFinishedCount < 10)
							{
								m_lCheckFinishedCount++;
							}
							else
							{
								m_lCheckFinishedCount = 0;
								if (lPos >= m_lMediaLength)
								{
									FireFinished();
									m_bSkipCheckFinished = TRUE;
									m_lLastStopPosition = 0;
								}
							}
						}
						else
						{
							m_lCheckFinishedCount = 0;
						}
						lPos = m_lLastMediaPos;
					}
				}
			}

			return 0L;
		}
		else if (message == WM_QTVUPEAKLEVEL)
		{
			short	iNumChannels, iLeftLevel, iRightLevel;

			iNumChannels = wParam;
			iLeftLevel = (short)(lParam >> 16);
			iRightLevel = lParam & 0xFFFF;

			FireVUPeakLevel(iNumChannels, iLeftLevel, iRightLevel);
			return 0L;
		}
		else if (message == WM_QTPLAYCOMPLETED)
		{
			m_psCurrent = Stopped;

			if (m_bPlayInLoop != TRUE)
			{
				if (m_bSeekToZeroWhenFinished == TRUE)
				{
					// Seek to the beginning
					while (IsMovieDone(m_pQuickTime->theMovie) == false) ;
					StopMovie(m_pQuickTime->theMovie);
					GoToBeginningOfMovie(m_pQuickTime->theMovie);
					m_lMediaPosition = 0;
				}
			}

			FireFinished();
			m_bSkipCheckFinished = TRUE;
			m_lLastStopPosition = 0;
			return 0L;
		}
		else if (message == MM_MIXM_CONTROL_CHANGE)
		{
			if (m_hMixer)
			{
				if (m_hMixer == (HMIXER)wParam)
				{
					if (m_dwVolumeControlID == (DWORD)lParam)
					{
						short nVolume;

						if (GetCdromVolume(&nVolume) == TRUE)
						{
							FireCDVolume(nVolume);
						}
					}
					else if (m_dwMuteControlID == (DWORD)lParam)
					{
						BOOL bMute;

						if (GetCdromMuteState(&bMute) == TRUE)
						{
							FireCDMute(bMute);
						}
					}
				}
			}
			return 0L;
		}
		else
		{
			if (GetNativeWindowPort(m_hWnd))
			{
				m_pQuickTime->ProcessMovieEvent(m_hWnd, message, wParam, lParam);
			}

			if (message == WM_DESTROY)
			{
				MSG msg;

				while (PeekMessage(&msg, m_hWnd, 0, 0, PM_NOREMOVE))
				{
					if (msg.message == WM_QUIT)
					{
						break;
					}

					if (GetMessage(&msg, m_hWnd, 0, 0))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}

				if (m_pQuickTime)
				{
					if (m_QTStopCallBackID)
					{
						DisposeCallBack(m_QTStopCallBackID);
						m_QTStopCallBackID = NULL;
					}
					m_pQuickTime->OnMovieWindowDestroy();
				}

				if (m_hMixer)
				{
					// Restore saved CDROM line volume and mute values
					SetCdromVolume((short)m_lOldCDROMVolume);
					SetCdromMuteState(m_bOldCDROMMuteState);

					// Close CDROM device
					mixerClose(m_hMixer);
				}
			}
			return COleControl::WindowProc(message, wParam, lParam);
		}
	}
	else
	{
		switch(message)
		{
			case MM_MIXM_CONTROL_CHANGE:
			{
				if (m_hMixer)
				{
					if (m_hMixer == (HMIXER)wParam)
					{
						if (m_dwVolumeControlID == (DWORD)lParam)
						{
							short nVolume;

							if (GetCdromVolume(&nVolume) == TRUE)
							{
								FireCDVolume(nVolume);
							}
						}
						else if (m_dwMuteControlID == (DWORD)lParam)
						{
							BOOL bMute;

							if (GetCdromMuteState(&bMute) == TRUE)
							{
								FireCDMute(bMute);
							}
						}
					}
				}
				break;
			}
			case MM_MCINOTIFY:
			{
				switch (wParam)
				{
					case MCI_NOTIFY_ABORTED:
						break;
					case MCI_NOTIFY_FAILURE:
					{
						if (m_psCurrent == Playing)
						{
							m_psCurrent = Stopped;
							m_bMediaLoaded = FALSE;
							m_lMediaLength = 0;
							m_CDAudio.Close();
						}
						break;
					}
					case MCI_NOTIFY_SUCCESSFUL:
						if (m_bPlayInLoop == TRUE)
						{
							m_CDAudio.Play(m_CDLastTrackPos, m_CDLastTrackPos + m_lMediaLength, TRUE);
						}
						FireFinished();
						m_bSkipCheckFinished = TRUE;
						m_lLastStopPosition = 0;
						break;
					case MCI_NOTIFY_SUPERSEDED:
						break;
				}
				break;
			}
			case WM_GRAPHNOTIFY:
				HandleGraphEvent();
				break;
			case WM_ERASEBKGND:
				ClearControl();
				break;
			case WM_DESTROY:
			{
				MSG msg;

				while (PeekMessage(&msg, m_hWnd, 0, 0, PM_NOREMOVE))
				{
					if (msg.message == WM_QUIT)
					{
						break;
					}

					if (GetMessage(&msg, m_hWnd, 0, 0))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}

				if (m_pQuickTime)
				{
					if (m_QTStopCallBackID)
					{
						DisposeCallBack(m_QTStopCallBackID);
						m_QTStopCallBackID = NULL;
					}
					m_pQuickTime->OnMovieWindowDestroy();
				}

				if (m_hMixer)
				{
					// Restore saved CDROM line volume and mute values
					SetCdromVolume((short)m_lOldCDROMVolume);
					SetCdromMuteState(m_bOldCDROMMuteState);

					// Close CDROM device
					mixerClose(m_hMixer);
				}

				break;
			}
			case WM_MEDIAPOSITION:
			{
				// Return the current position of the media file to the container
				// Poistion in time (hh:mm:ss) and millisecond
				char	szTime[80];
				long	lPos;

				if (m_bMediaLoaded == FALSE)
				{
					break;
				}

				if (m_psCurrent != Playing)
				{
					break;
				}

				if (m_CurrentPlaybackInterface == MCI)
				{
					lPos = m_CDAudio.GetCurrentPos() - m_CDLastTrackPos;

					wsprintf(szTime, "%02d:%02d:%02d", lPos/1440000, lPos/60000, (lPos/1000)%60);

					m_lMediaPosition = lPos;

					FireMediaPosition((LPCTSTR)szTime, lPos);
				}

				if (m_bSkipCheckFinished == FALSE)
				{
					if (m_lLastMediaPos == lPos)
					{
						if (m_lCheckFinishedCount < 10)
						{
							m_lCheckFinishedCount++;
						}
						else
						{
							m_lCheckFinishedCount = 0;
							if (lPos >= m_lMediaLength)
							{
								FireFinished();
								m_bSkipCheckFinished = TRUE;
								m_lLastStopPosition = 0;
							}
						}
					}
					else
					{
						m_lCheckFinishedCount = 0;
					}
					lPos = m_lLastMediaPos;
				}

				break;
			}

			case WM_DEVICECHANGE:
			{
				// Check whether a CD or DVD was removed from a drive.
				//char szMsg[1000];

				if (wParam == DBT_DEVICEREMOVECOMPLETE)
				{
					PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;

					PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;

					if (lpdbv->dbcv_flags & DBTF_MEDIA)
					{
						if (m_CurrentPlaybackInterface == MCI)
						{
							m_CDAudio.Stop();
						}

						/*
						wsprintf (szMsg, "Drive %c: Media was removed.\n",
							FirstDriveFromMask(lpdbv->dbcv_unitmask));

						AfxMessageBox(szMsg);
						*/
					}
					return TRUE;
				}
  
				break;
			}
			default:
				return COleControl::WindowProc(message, wParam, lParam);

		} // Window msgs handling

		// Pass this message to the video window for notification of system changes
		if (m_pVideoWindow)
			m_pVideoWindow->NotifyOwnerMessage((LONG_PTR)m_hWnd, message, wParam, lParam);

		return COleControl::WindowProc(message, wParam, lParam);
	}
}

long CMediaCtrl::OnChangedMediaFile(LPCTSTR szPlaybackFile, BOOL bLoopMe, const VARIANT FAR& varPlayNow, const VARIANT FAR& varSeekToZeroAtTheEndOfPlayback)
{
    HRESULT			hr;
	BOOL			bPlayFlag;	// OLE automation compatible boolean data type

	// Blank control
	ClearControl();

	// Reset the media loaded flag
	m_lMediaType = UnknownMedia;
	m_bMediaLoaded = FALSE;
	m_lVideoWidth = 0;
	m_lVideoHeight = 0;
	m_bSkipCheckFinished = FALSE;
	m_lCheckFinishedCount = 0;
	m_lLastStopPosition = 0;

	// Set playback loop flag
	m_bPlayInLoop = bLoopMe;

	// Reset VU Peak Levels
	FireVUPeakLevel(0, 0, 0);

	// Check the optional PlayNow variant parameter
	if (varPlayNow.vt == VT_BOOL)
	{
		// if the variant is a boolean just use the value
		if (varPlayNow.boolVal == VARIANT_TRUE)
		{
			bPlayFlag = TRUE;
		}
		else
		{
			bPlayFlag = FALSE;
		}
	}
	else
	{
		if (varPlayNow.vt == VT_ERROR || varPlayNow.vt == VT_EMPTY)
		{
			// optional parameter, use FALSE as the default value
			bPlayFlag = FALSE;
		}
		else
		{
			// get a variant that we can use for conversion purposes 
			VARIANT varConvertedValue;
			
			// initialize the variant 
			::VariantInit(&varConvertedValue); 
			// see if we can convert the data type to something useful
			if (::VariantChangeType(&varConvertedValue, (VARIANT *) &varPlayNow,
							0, VT_BOOL) == S_OK)
			{
				if (varConvertedValue.boolVal == VARIANT_FALSE)
				{
					bPlayFlag = FALSE;
				}
				else
				{
					bPlayFlag = TRUE;
				}
			}
			else
			{
				Fire_Error(mediaErrorInvalidParameter, "The parameter [PlayNow] is not a boolean data type.");
				return -1;
			}
		}
	}

	// Check the optional SeekToZeroAtTheEndOfPlayback variant parameter
	if (varSeekToZeroAtTheEndOfPlayback.vt == VT_BOOL)
	{
		// if the variant is a boolean just use the value
		if (varPlayNow.boolVal == VARIANT_TRUE)
			m_bSeekToZeroWhenFinished = TRUE;
		else
			m_bSeekToZeroWhenFinished = FALSE;
	}
	else
	{
		if (varSeekToZeroAtTheEndOfPlayback.vt == VT_ERROR ||
			varSeekToZeroAtTheEndOfPlayback.vt == VT_EMPTY)
		{
			// optional parameter, use FALSE as the default value
			m_bSeekToZeroWhenFinished = FALSE;
		}
		else
		{
			// get a variant that we can use for conversion purposes 
			VARIANT varConvertedValue;
			
			// initialize the variant 
			::VariantInit(&varConvertedValue); 
			// see if we can convert the data type to something useful
			if (::VariantChangeType(&varConvertedValue, (VARIANT *) &m_bSeekToZeroWhenFinished,
							0, VT_BOOL) == S_OK)
			{
				if (varConvertedValue.boolVal == VARIANT_FALSE)
				{
					m_bSeekToZeroWhenFinished = FALSE;
				}
				else
				{
					m_bSeekToZeroWhenFinished = TRUE;
				}
			}
			else
			{
				Fire_Error(mediaErrorInvalidParameter, "The parameter [SeekToZeroAtTheEndOfPlayback] is not a boolean data type.");
				return -1;
			}
		}
	}

	if (CheckFileExisted(szPlaybackFile) == FALSE)
	{
		Fire_Error(mediaErrorFileNotFound, "The media file is not found.");
		return -1;
	}

	m_lMediaLength = 0;

	// Free-up previous DirectShow allocated playback resources
	if (m_CurrentPlaybackInterface == DirectShow)
	{
		// DirectShow resources, stop playback if playing
		if (m_psCurrent == Playing)
		{
			OnStopMedia();
		}

		FreePlaybackResources();
	}
	else if (m_CurrentPlaybackInterface == QuickTime)
	{
		// QuickTime Resources
		if (m_pQuickTime)
		{
			if (m_QTStopCallBackID)
			{
				DisposeCallBack(m_QTStopCallBackID);
				m_QTStopCallBackID = NULL;
			}
			m_pQuickTime->CloseMovie();
		}
	}
	else if (m_CurrentPlaybackInterface == MCI)
	{
		// MCI resources
		if (m_psCurrent == Playing)
		{
			OnStopMedia();
		}
		m_CDAudio.Close();
		
		// Change CDROM hardware setting
		//EnableCDROMDigitalPlayback("D:\\");
	}

	if (IsAudioCDFile(szPlaybackFile))
	{
		BYTE	nTrackToPlay;
		char	szDrive[20];
		
		lstrcpyn(szDrive, szPlaybackFile, 3);

		// Change CDROM hardware setting
		//DisableCDROMDigitalPlayback(szDrive);

		if (!m_CDAudio.Open(szDrive, TRUE))
		{
			// Set the callback window for asynchronous operations
			m_CDAudio.SetCallbackWnd(m_hWnd);

			// Set time format for CD audio
			m_CDAudio.SetTimeFormat(CCdAudio::FormatMilliseconds);

			nTrackToPlay = ConvertFilenameToAudioTrack(szPlaybackFile);

			if (m_CDAudio.IsReady())
			{
				if (nTrackToPlay)
				{
					CMsf msf = m_CDAudio.GetTrackLength(nTrackToPlay);

					m_lMediaLength = (DWORD)msf;
					m_lMediaType = Audio;

					if (nTrackToPlay > 0)
					{
						m_CDLastTrackPos = m_CDAudio.GetTrackPos(nTrackToPlay);
					}
					else
					{
						m_CDLastTrackPos = 0;
					}

					m_CDTrackNo = nTrackToPlay;
					m_CurrentPlaybackInterface = MCI;
					m_bMediaLoaded = TRUE;
					FireFileLoaded();

					// Set default volume
					SetVolume(m_lGlobalVolume);

					// Hide video window for audio-only file
					m_bHideWindowForAudioOnlyFile = TRUE;
					ShowWindow(SW_HIDE);

					// Start playback
					if (bPlayFlag == TRUE)
					{
						m_CDAudio.PlayTrack(nTrackToPlay, TRUE);
						m_psCurrent = Playing;
					}
				}
			}
			else
			{
				Fire_Error(mediaErrorCDRomNotReady, "The CD-ROM is not ready to play.");
			}
		}
	}
	else
	{
		// First try using DirectShow interface to play the media file, if failed,
		// try using QuickTime interface...
		hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
								IID_IGraphBuilder, (void **)&m_pGraph);

		if (FAILED(hr))
		{
			// Failed to create DirectShow interface for media playback
			Fire_Error(mediaErrorCreateGraphBuilderFailed, "Failed to create DirectShow Graph Filter to render the media file.");
			return -1;
		}

		USES_CONVERSION;

		WCHAR wFile[2048];

		// Convert filename to wide character string
		wcscpy(wFile, T2W(szPlaybackFile));

		// Use special handling for Windows Media files
		if (IsWindowsMediaFile(szPlaybackFile))
		{
			IFileSourceFilter	*pFSF=NULL;
			IBaseFilter			*pReader=NULL;

			// Load the improved ASF reader filter by CLSID
			hr = CoCreateInstance(CLSID_WMAsfReader, NULL, CLSCTX_INPROC_SERVER,
										IID_IBaseFilter, (void **) &pReader);

			if (SUCCEEDED(hr))
			{
				// Add the ASF reader filter to the graph.  For ASF/WMV/WMA content,
				// this filter is NOT the default and must be added explicitly.
				hr = m_pGraph->AddFilter(pReader, L"ASF Reader");
				if (FAILED(hr))
				{
					//FireError(MYCTL_E_CREATEDSHOWGRAPHFILTER, "Failed to add ASF reader filter to graph.");
					SAFE_RELEASE(pReader);
					goto BuildGraphAsNormal;
				}

				// Create the key provider that will be used to unlock the WM SDK
				if (FAILED(AddKeyProvider(m_pGraph)))
				{
					SAFE_RELEASE(pReader);
					goto BuildGraphAsNormal;
				}

				// Set its source filename
				if (FAILED(pReader->QueryInterface(IID_IFileSourceFilter, (void **) &pFSF)))
				{
					hr = m_pGraph->RemoveFilter(pReader);
					SAFE_RELEASE(pReader);
					goto BuildGraphAsNormal;
				}

				if (FAILED(pFSF->Load(wFile, NULL)))
				{
					SAFE_RELEASE(pFSF);
					hr = m_pGraph->RemoveFilter(pReader);
					SAFE_RELEASE(pReader);
					goto BuildGraphAsNormal;
				}

				SAFE_RELEASE(pFSF);

				// Render the output pins of the ASF reader to build the
				// remainder of the graph automatically
				if (FAILED(RenderOutputPins(m_pGraph, pReader)))
				{
					hr = m_pGraph->RemoveFilter(pReader);
					SAFE_RELEASE(pReader);
					goto BuildGraphAsNormal;
				}

				// Since the graph is built and the filters are added to the graph,
				// the WM ASF reader interface can be released.
				SAFE_RELEASE(pReader);

				hr = 0;

				goto BuildGraphDone;
			}
			else
			{
				// Create the key provider that will be used to unlock the WM SDK
				if (FAILED(AddKeyProvider(m_pGraph)))
				{
					goto BuildGraphAsNormal;
				}
			}
		}

BuildGraphAsNormal:

		hr = m_pGraph->RenderFile(wFile, NULL);

BuildGraphDone:

		if (FAILED(hr))
		{
			// Failed when opening the media file using DirectShow interface,
			// Try QuickTime interface if available

			// Free up any unused DirectShow resources.
			FreePlaybackResources();
			
			// Report error if QuickTime is not installed
			if (m_bQuickTimeInstalled == FALSE)
			{
				Fire_Error(mediaErrorUnknownFileFormat, "Unable to render the media file, probably the required codec is not installed.");
				InvalidateControl();
				ShowWindow(SW_HIDE);
				return -1;
			}

			// Register this HWND with QTML
			CreatePortAssociation(m_hWnd, NULL, 0L);	
			m_pQuickTime->theViewHwnd = m_hWnd;
			
			if (m_pQuickTime->OpenMovie((unsigned char *)szPlaybackFile) == TRUE)
			{
				int iControlWidth, iControlHeight, iNewWidth, iNewHeight;
				int	xoffset, yoffset;

				ComponentResult	theErr = noErr;
				
				// Check Media Type
				GetQTMediaType(m_pQuickTime);

				// Set playback in loops
				if (m_bPlayInLoop == TRUE)
				{
					MCDoAction(m_pQuickTime->theMC, mcActionSetLooping, (void*)TRUE);
				}
				
				// Get movie duration
				TimeValue	lDuration;
				TimeScale	lTimeScale;
				TimeBase	lTimeBase;
				lTimeBase = GetMovieTimeBase(m_pQuickTime->theMovie);
				lDuration = GetMovieDuration(m_pQuickTime->theMovie);
				lTimeScale = GetMovieTimeScale(m_pQuickTime->theMovie);

				m_lMediaLength = (long)llMulDiv(1000, lDuration, lTimeScale, 0);

				if (m_lMediaType == Video)
				{
					// Get the size of the control
					GetControlSize(&iControlWidth, &iControlHeight);

					m_lVideoWidth = m_pQuickTime->theMovieRect.right;
					m_lVideoHeight = m_pQuickTime->theMovieRect.bottom;

					// Set video window size according to display mode
					if (m_lDisplayMode == Normal)
					{
						// Set the control size same as the input
						SetControlSize(m_pQuickTime->theMovieRect.right,
									m_pQuickTime->theMovieRect.bottom);

						xoffset = 0;
						yoffset = 0;
						iNewWidth = m_pQuickTime->theMovieRect.right;
						iNewHeight = m_pQuickTime->theMovieRect.bottom;
					}
					else if (m_lDisplayMode == StretchFit)
					{
						// Set the size same as the control
						xoffset = 0;
						yoffset = 0;
						iNewWidth = iControlWidth;
						iNewHeight = iControlHeight;
					}
					else if (m_lDisplayMode == Fit)
					{
						int		iXOffset, iYOffset;
						float	fXRatio, fYRatio, fAspectRatio;

						fAspectRatio = (float)m_pQuickTime->theMovieRect.right / (float)m_pQuickTime->theMovieRect.bottom;

						fXRatio = (float)m_pQuickTime->theMovieRect.right / (float)iControlWidth;
						fYRatio = (float)m_pQuickTime->theMovieRect.bottom / (float)iControlHeight;
						
						if (fXRatio >= fYRatio)
						{
							int iYNewSize = (int)((float)iControlWidth / fAspectRatio);
							iYOffset = (iControlHeight - iYNewSize) / 2;

							xoffset = 0;
							yoffset = iYOffset;
							iNewWidth = iControlWidth;
							iNewHeight = iYNewSize;
						}
						else
						{
							int iXNewSize = (int)((float)iControlHeight * fAspectRatio);
							iXOffset = (iControlWidth - iXNewSize) / 2;

							xoffset = iXOffset;
							yoffset = 0;
							iNewWidth = iXNewSize;
							iNewHeight = iControlHeight;
						}
					}
				}

				// if the controller bar is attached, detach it (and remember we did so)
				BOOL wasAttached;
				if (MCIsControllerAttached(m_pQuickTime->theMC) == 1)
				{
					wasAttached = TRUE;
					MCSetControllerAttached(m_pQuickTime->theMC, FALSE);
				}
				
				// get the rectangle of the controller
				Rect rcBarRect;
				MCGetControllerBoundsRect(m_pQuickTime->theMC, &rcBarRect);
				
				// now reattach the controller bar, if it was originally attached
				if (wasAttached == TRUE)
					MCSetControllerAttached(m_pQuickTime->theMC, TRUE);

				m_iMCBarHeight = rcBarRect.bottom - rcBarRect.top;

				Rect WndRect;
				WndRect.top   =(short)yoffset;
				WndRect.left  =(short)xoffset;
				WndRect.bottom=(short)iNewHeight + m_iMCBarHeight;
				WndRect.right =(short)iNewWidth;

				// Set up the video window in QuickTime Movie Controller
				MCSetControllerBoundsRect(m_pQuickTime->theMC, &WndRect);

				// Fire FileLoaded event when media file is completely loaded
				m_CurrentPlaybackInterface = QuickTime;
				m_bMediaLoaded = TRUE;
				FireFileLoaded();

				// Set default volume
				SetVolume(m_lGlobalVolume);

				// Set up callback at stop
				m_QTStopCallBackID = NewCallBack(GetMovieTimeBase(m_pQuickTime->theMovie),
													callBackAtExtremes);

				if (m_QTStopCallBackID)
					CallMeWhen(m_QTStopCallBackID, QTStopCallBackProc, (long)this->m_pQuickTime, triggerAtStop, 0, 0);

				// determine showing video window or not
				if (m_lMediaType == Audio)
				{
					m_bHideWindowForAudioOnlyFile = TRUE;
					ShowWindow(SW_HIDE);
				}
				else
				{
					m_bHideWindowForAudioOnlyFile = FALSE;
					if (m_bControlVisible == TRUE)
					{
						ShowWindow(SW_SHOW);
					}
				}

				// Start playback
				if (bPlayFlag == TRUE)
				{
					if (m_pQuickTime->theMC)
					{
						ComponentResult	theErr = noErr;

						Fixed w = GetMoviePreferredRate(m_pQuickTime->theMovie);

						theErr = MCDoAction(m_pQuickTime->theMC, mcActionPrerollAndPlay,
									(void *)GetMoviePreferredRate(m_pQuickTime->theMovie));
						
						if (theErr == noErr)
						{
							m_psCurrent = Playing;
							m_CurrentPlaybackInterface = QuickTime;
						}
					}
				}
			}
			else
			{
				m_CurrentPlaybackInterface = DirectShow;
				m_lMediaType = UnknownMedia;
				DestroyPortAssociation((CGrafPtr)GetNativeWindowPort(m_hWnd));
				m_pQuickTime->theViewHwnd = NULL;
				Fire_Error(mediaErrorUnknownFileFormat, "Unable to render the media file, probably the required codec is not installed.");
				InvalidateControl();
				ShowWindow(SW_HIDE);
				return -1;
			}
		}
		else
		{
			// Connect VUMeter Filter to capture samples
			hr = ConnectGrabberFilter(m_pGraph);

			// QueryInterface for DirectShow interfaces
			m_pGraph->QueryInterface(IID_IMediaControl, (void **)&m_pMControl);
			m_pGraph->QueryInterface(IID_IMediaEventEx, (void **)&m_pMEvent);
			m_pGraph->QueryInterface(IID_IMediaSeeking, (void **)&m_pMSeek);

			if (m_pMSeek)
			{
				LONGLONG	MediaDuration;
				m_pMSeek->GetDuration(&MediaDuration);
				m_lMediaLength = (long)llMulDiv(1, MediaDuration, 10000, 0);
			}

			if (!m_lMediaLength)
			{
				InvalidateControl();
				FreePlaybackResources();
				Fire_Error(mediaErrorEmptyMediaFile, "The media length is zero and the file will not be played.");
				return -1;
			}

			// Query for video interfaces, which may not be relevant for audio files
			m_pGraph->QueryInterface(IID_IVideoWindow, (void **)&m_pVideoWindow);
			m_pGraph->QueryInterface(IID_IBasicVideo, (void **)&m_pBasicVideo);

			// Set the message drain of the video window to point to our main
			// application window.  If this is an audio-only or MIDI file, 
			// then put_MessageDrain will fail.
			hr = m_pVideoWindow->put_MessageDrain((OAHWND) m_hWnd);
			if (FAILED(hr))
			{
				m_lMediaType = Audio;
			}
			else
			{
				m_lMediaType = Video;
			}

			// Have the graph signal event via window callbacks for performance
			m_pMEvent->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0);

			if (m_lMediaType == Video)
			{
				m_pVideoWindow->put_Owner((OAHWND)m_hWnd);
				m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

				long x, y, cx, cy;

				m_pVideoWindow->GetWindowPosition(&x, &y, &cx, &cy);

				m_lVideoWidth = cx;
				m_lVideoHeight = cy;

				int iControlWidth, iControlHeight;
				GetControlSize(&iControlWidth, &iControlHeight);

				// Set video window size
				if (m_lDisplayMode == Fit)
				{
					int		iXOffset, iYOffset;
					float	fXRatio, fYRatio, fAspectRatio;

					fAspectRatio = (float)cx / (float)cy;

					fXRatio = (float)cx / (float)iControlWidth;
					fYRatio = (float)cy / (float)iControlHeight;
					
					if (fXRatio >= fYRatio)
					{
						int iYNewSize = (int)((float)iControlWidth / fAspectRatio);
						iYOffset = (iControlHeight - iYNewSize) / 2;
						m_pVideoWindow->SetWindowPosition(0, iYOffset, iControlWidth, iYNewSize);
					}
					else
					{
						int iXNewSize = (int)((float)iControlHeight * fAspectRatio);
						iXOffset = (iControlWidth - iXNewSize) / 2;
						m_pVideoWindow->SetWindowPosition(iXOffset, 0, iXNewSize, iControlHeight);
					}
				}
				else if (m_lDisplayMode == Normal)
				{
					// Normal window size as media source input
					SetControlSize(cx, cy);
					m_pVideoWindow->SetWindowPosition(0, 0, cx, cy);
				}
				else if (m_lDisplayMode == StretchFit)
				{
					// Stretch to fit into window
					m_pVideoWindow->SetWindowPosition(0, 0, iControlWidth, iControlHeight);
				}
				else
				{
					m_pVideoWindow->SetWindowPosition(0, 0, iControlWidth, iControlHeight);
				}
			}

			// determine showing video window or not
			if (m_lMediaType == Audio)
			{
				m_bHideWindowForAudioOnlyFile = TRUE;
				ShowWindow(SW_HIDE);
			}
			else
			{
				m_bHideWindowForAudioOnlyFile = FALSE;
				if (m_bControlVisible == TRUE)
				{
					ShowWindow(SW_SHOW);
				}
			}

			// Fire FileLoaded event when media file is completely loaded
			m_CurrentPlaybackInterface = DirectShow;
			m_bMediaLoaded = TRUE;
			FireFileLoaded();

			// Set default volume
			SetVolume(m_lGlobalVolume);

			// Add our graph to the running object table, which will allow
			// the GraphEdit application to "spy" on our graph
#ifdef REGISTER_FILTERGRAPH
			hr = AddGraphToRot(m_pGraph, &g_dwGraphRegister);
			if (FAILED(hr))
			{
				TRACE(TEXT("Failed to register filter graph with ROT!"));
				g_dwGraphRegister = 0;
			}
#endif

			if (bPlayFlag == TRUE)
			{
				// Run the graph to play the media file
				if (SUCCEEDED(m_pMControl->Run()))
				{
					m_psCurrent=Playing;
				}
			}
			else
			{
				// Display the first frame of the media clip, if it contains video.
				// StopWhenReady() pauses all filters internally (which allows the video
				// renderer to queue and display the first video frame), after which
				// it sets the filters to the stopped state.  This enables easy preview
				// of the video's poster frame.
				if (m_lMediaType == Video)
					hr = m_pMControl->StopWhenReady();
			}
		}
	}
	return 0;
}

long CMediaCtrl::GetPlayState() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	// Get playback state
	return m_psCurrent;
}

void CMediaCtrl::SetPlayState(long nNewValue) 
{
	// Read-only property
	SetNotSupported();
}

long CMediaCtrl::GetDisplayMode() 
{
	return m_lDisplayMode;
}

void CMediaCtrl::SetDisplayMode(long nNewValue) 
{
	// Set the window mode for valid value only
	if (nNewValue == Fit || nNewValue == Normal || nNewValue == StretchFit)
	{
		m_lDisplayMode = (WINDOWFITMODE)nNewValue;
		SetModifiedFlag();
	}
}

void CMediaCtrl::OnPauseMedia() 
{
	if (m_psCurrent == Playing)
	{
		switch (m_CurrentPlaybackInterface)
		{
			case DirectShow:
			{
				// for DirectShow interface
				if (!m_pMControl)
				{
					return;
				}
				if (SUCCEEDED(m_pMControl->Pause()))
				{
					m_psCurrent = Paused;
				}
				break;
			}
			case QuickTime:
			{
				// for QuickTime interface
				if (m_pQuickTime->theMovie)
				{
					StopMovie(m_pQuickTime->theMovie);
					m_psCurrent = Paused;
				}
				break;
			}
			case MCI:
			{
				m_CDAudio.Pause();
				m_CDLastPos = m_CDAudio.GetCurrentPos();
				m_psCurrent = Paused;
				break;
			}
		}
		m_bSkipCheckFinished = TRUE;
	}
}

void CMediaCtrl::OnResumeMedia() 
{
	if (m_psCurrent == Paused)
	{
		switch (m_CurrentPlaybackInterface)
		{
			case DirectShow:
			{
				// for DirectShow interface
				if (!m_pMControl)
				{
					return;
				}

				if (SUCCEEDED(m_pMControl->Run()))
				{
					m_psCurrent = Playing;
				}
				break;
			}
			case QuickTime:
			{
				// for QuickTime interface
				if (m_pQuickTime->theMovie)
				{
					//MCDoAction(m_pQuickTime->theMC, mcActionResume, NULL);
					StartMovie(m_pQuickTime->theMovie);
					m_psCurrent = Playing;
				}
				break;
			}
			case MCI:
			{
				//m_CDAudio.Seek(m_CDLastPos, FALSE);
				m_CDAudio.Play(m_CDLastPos, m_CDLastTrackPos + m_lMediaLength, TRUE);
				m_psCurrent = Playing;
				break;
			}
		}
		m_bSkipCheckFinished = FALSE;
	}
}

void CMediaCtrl::OnStopMedia() 
{
	if (m_psCurrent == Playing || m_psCurrent == Paused)
	{
		if (m_CurrentPlaybackInterface == DirectShow)
		{
			if (!m_pMControl)
			{
				return;
			}

			// for DirectShow interface
			if (SUCCEEDED(m_pMControl->Stop()))
			{
				m_psCurrent = Stopped;
			}
			m_bSkipCheckFinished = TRUE;
		}
		else if (m_CurrentPlaybackInterface == QuickTime)
		{
			// for QuickTime interface
			if (m_pQuickTime->theMovie)
			{
				StopMovie(m_pQuickTime->theMovie);
				m_psCurrent = Stopped;
			}
			m_bSkipCheckFinished = TRUE;
		}
		else if (m_CurrentPlaybackInterface == MCI)
		{
			m_CDAudio.Stop();
			m_psCurrent = Stopped;
		}
	}
}

void CMediaCtrl::FreePlaybackResources()
{
    HRESULT hr;

#ifdef REGISTER_FILTERGRAPH
        // Remove filter graph from the running object table   
	if (g_dwGraphRegister)
		RemoveGraphFromRot(g_dwGraphRegister);
#endif

    // Relinquish ownership (IMPORTANT!) after hiding video window
    if(m_pVideoWindow)
    {
        hr = m_pVideoWindow->put_Visible(OAFALSE);
        hr = m_pVideoWindow->put_Owner(NULL);
    }

    // Disable event callbacks
    if (m_pMEvent)
        hr = m_pMEvent->SetNotifyWindow((OAHWND)NULL, 0, 0);

	if (m_CurrentPlaybackInterface == MCI)
	{
		m_CDAudio.Stop();
	}

    // Release and zero DirectShow interfaces
	SAFE_RELEASE(m_pVUMeterFilter);
    SAFE_RELEASE(m_pTeeFilter);
	SAFE_RELEASE(m_pMEvent);
    SAFE_RELEASE(m_pMSeek);
    SAFE_RELEASE(m_pMControl);
    SAFE_RELEASE(m_pBasicAudio);
    SAFE_RELEASE(m_pBasicVideo);
    SAFE_RELEASE(m_pVideoWindow);
    SAFE_RELEASE(m_pGraph);
}

long CMediaCtrl::GetMediaPositionLong() 
{
	long	ltmp;

	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			// For DirectShow
			LONGLONG	MediaCurrentPos;
			
			if (m_pMSeek)
			{
				if (SUCCEEDED(m_pMSeek->GetCurrentPosition(&MediaCurrentPos)))
				{
					ltmp = (long)llMulDiv(1, MediaCurrentPos, 10000000, 0);
					//m_lMediaPosition = (long)llMulDiv(1, MediaCurrentPos, 10000, 0);
					m_lMediaPosition = ltmp * 1000;
					return m_lMediaPosition;
				}
			}
			break;
		}
		case QuickTime:
		{
			// For QuickTime
			TimeValue	lTimeValue;
			TimeRecord	CurTimeRecord;

			lTimeValue = GetMovieTime(m_pQuickTime->theMovie, &CurTimeRecord);

			ltmp = (long)llMulDiv(1, lTimeValue, CurTimeRecord.scale, 0);

			return (ltmp * 1000);
		}
		case MCI:
		{
			return m_CDAudio.GetCurrentPos() - m_CDLastTrackPos;
		}
		default:
			// Do nothing
			break;
	}
	return 0;
}

long CMediaCtrl::GetNumberOfSoundCardMixers() 
{
	UINT nbMixers = mixerGetNumDevs();
	if (nbMixers < 1)
	{
		return 0;
	}
	else
	{
		return nbMixers;
	}
}

BOOL CMediaCtrl::HasASoundCardInstalled() 
{
	UINT nbMixers = mixerGetNumDevs();
	if (nbMixers < 1)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

double CMediaCtrl::SetMediaSpeed(double Speed) 
{
    HRESULT hr;
	PLAYSTATE psOldState;
	double	dCurrentSpeed;

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			// for DirectShow interface
			if (m_pMSeek)
			{
				if (SUCCEEDED(m_pMSeek->GetRate(&dCurrentSpeed)))
				{
					if (Speed != 0.0)
					{
						// Make sure it is stopped
						psOldState = m_psCurrent;
						if (m_psCurrent == Playing)
						{
							m_lLastStopPosition = m_lMediaPosition;
							m_pMControl->Stop();
						}

						// Disconnect Tee and VU Meter filters first
						DisconnectGrabberFilter(m_pGraph);

						hr = m_pMSeek->SetRate(Speed);
						if (SUCCEEDED(hr))
						{
							ConnectGrabberFilter(m_pGraph);

							if (psOldState == Playing && m_psCurrent == Playing)
							{
								m_pMControl->Run();
							}

							return dCurrentSpeed;
						}
						else
						{
							ConnectGrabberFilter(m_pGraph);
							m_pMControl->Run();
						}
					}
				}
			}
			break;
		}
		case QuickTime:
		{
			if (Speed != 0.0)
			{
				Fixed w = X2Fix(Speed);
				Fixed x = GetMoviePreferredRate(m_pQuickTime->theMovie);
				Fixed y = FixMul(x, w);
				MCDoAction(m_pQuickTime->theMC, mcActionPlay, (void*)y);

				return (double)Fix2X(x);
			}
			break;
		}
		case MCI:
		{
			break;
		}
	}

	return 0.0;
}

// Seek to the requested media position
void CMediaCtrl::SeekTo(long SeekPosition) 
{
    HRESULT		hr;

	// Raise error for negative position
	if (SeekPosition < 0)
	{
		Fire_Error(mediaErrorInvalidParameter, "Seek position must be a positive number.");
		return;
	}

	// Return if media file is not loaded
	if (m_bMediaLoaded == FALSE)
	{
		return;
	}

	// Seek to the end if value > media length
	if (SeekPosition > m_lMediaLength)
	{
		SeekPosition = m_lMediaLength;
	}

	m_bSkipCheckFinished = TRUE;

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			// for DirectShow interface
			
			LONGLONG pos = llMulDiv(10000, SeekPosition, 1, 0);

			if (m_pMSeek)
			{
				// seek to absolute position
				hr = m_pMSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning,
											   NULL, AM_SEEKING_NoPositioning);
				m_lLastStopPosition = SeekPosition;
			}
			break;
		}
		case QuickTime:
		{
			TimeRecord	lTimeRecord;

			lTimeRecord.base = GetMovieTimeBase(m_pQuickTime->theMovie);
			lTimeRecord.scale = GetMovieTimeScale(m_pQuickTime->theMovie);
			lTimeRecord.value.hi = 0;
			lTimeRecord.value.lo = (long)llMulDiv(lTimeRecord.scale, SeekPosition, 1000, 0);
			if (m_pQuickTime->theMC)
				MCDoAction(m_pQuickTime->theMC, mcActionGoToTime, (void*)&lTimeRecord);

			break;
		}
		case MCI:
		{
			m_CDAudio.Seek(m_CDLastTrackPos+SeekPosition, FALSE);
			m_CDAudio.Play(m_CDLastTrackPos+SeekPosition, m_CDLastTrackPos+m_lMediaLength, TRUE);
			break;
		}
		default:
			// Do nothing
			break;
	}
	m_bSkipCheckFinished = FALSE;
}

// Return the window handle of the control
long CMediaCtrl::GetWindowHwnd() 
{
	return GetHwnd();
}

long CMediaCtrl::GetTimeFormat() 
{
	m_lTimeFormat = UnknownUnit;

	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	/*
	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
			// for DirectShow interface
			GUID	gTimeFormat;

			if (m_pMSeek)
			{
				if (SUCCEEDED(m_pMSeek->GetTimeFormat(&gTimeFormat)))
				{
					if (gTimeFormat == TIME_FORMAT_FRAME)
					{
						m_lTimeFormat = Frame;
					}
					else if (gTimeFormat == TIME_FORMAT_MEDIA_TIME)
					{
						m_lTimeFormat = Millisecond;
					}
				}
			}
			break;
		case QuickTime:
			break;
		case MCI:
			m_lTimeFormat = Millisecond;
		default:
			// Do nothing
			break;
	}

	return m_lTimeFormat;
	*/

	return Millisecond;
}

void CMediaCtrl::SetTimeFormat(long nNewValue) 
{
/*
	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
			// for DirectShow interface
			GUID	gTimeFormat;

			if (nNewValue == Millisecond)
			{
				gTimeFormat = TIME_FORMAT_MEDIA_TIME;
			}
			else if (nNewValue == Frame)
			{
				gTimeFormat = TIME_FORMAT_FRAME;
			}
			else
			{
				return;
			}

			if (m_pMSeek)
			{
				if (SUCCEEDED(m_pMSeek->SetTimeFormat(&gTimeFormat)))
				{
					if (gTimeFormat == TIME_FORMAT_FRAME)
					{
						m_lTimeFormat = Frame;
					}
					else if (gTimeFormat == TIME_FORMAT_MEDIA_TIME)
					{
						m_lTimeFormat = Millisecond;
					}
				}
			}
			break;
		case QuickTime:
			break;
		case MCI:
			break;
		default:
			// Do nothing
			break;
	}

	SetModifiedFlag();
*/
}

void CMediaCtrl::OnFastForward() 
{
    HRESULT			hr;
	long			lIncrement;
	long			lNewPos;
	double			dtmp;

	// Return if media file is not loaded
	if (m_bMediaLoaded == FALSE)
	{
		return;
	}

	dtmp = m_lMediaLength;
	dtmp = dtmp * m_dAdvancePercent / 100.0;
	lIncrement = (long)dtmp;

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			// for DirectShow interface

			// Move forward
			lNewPos = m_lMediaPosition + lIncrement;
			if (lNewPos <= m_lMediaLength)
			{
				LONGLONG pos = llMulDiv(10000, lNewPos, 1, 0);

				if (m_pMSeek)
				{
					// seek to absolute position
					hr = m_pMSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning,
												   NULL, AM_SEEKING_NoPositioning);
					m_lLastStopPosition = lNewPos;
				}
			}
			//char	str[100];
			//wsprintf(str, "LP=%d, MP=%d, I=%d, lPos=%d\n", m_lLastStopPosition, m_lMediaPosition, lIncrement, lPos);
			//TRACE(str);
			break;
		}
		case QuickTime:
		{
			lNewPos = m_lMediaPosition + lIncrement;
			if (lNewPos <= m_lMediaLength)
			{
				SeekTo(lNewPos);
				m_lMediaPosition = lNewPos;
			}
			break;
		}
		case MCI:
		{
			lNewPos = m_lMediaPosition + lIncrement;
			if (lNewPos <= m_lMediaLength)
			{
				SeekTo(lNewPos);
				m_lMediaPosition = lNewPos;
			}
			break;
		}
		default:
			// Do nothing
			break;
	}
}

// Check the existence of the input media file,
// return TRUE if found, else FALSE
BOOL CMediaCtrl::CheckFileExisted(LPCTSTR szMediaFile)
{
	HANDLE	hFile;

	hFile = CreateFile(szMediaFile, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return TRUE;
	}

	return FALSE;
}

// Clear the control's background
void CMediaCtrl::ClearControl()
{
	HDC		hdc;
	CRect	rcBounds;

	hdc = ::GetDC(m_hWnd);

	// Fill the background with the container's background color
	GetClientRect(&rcBounds);

	CBrush brAmbientBack(TranslateColor(AmbientBackColor()));
	::FillRect(hdc, rcBounds, (HBRUSH)brAmbientBack);

	::ReleaseDC(m_hWnd, hdc);
}

// Timer callback for firing media position event,
// return position in time (hh:mm:ss) and millisecond
void CALLBACK MediaPositionTimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	CMediaCtrl *pMedia = (CMediaCtrl *)dwUser;

	if (pMedia->m_hWnd)
	{
		PostMessage(pMedia->m_hWnd, WM_MEDIAPOSITION, 0, 0L);
	}
}

// Re-play the media file (must be already loaded) from the beginning 
void CMediaCtrl::OnPlay() 
{
	m_lLastStopPosition = 0;

	// Return if media file is not loaded
	if (m_bMediaLoaded == FALSE)
	{
		return;
	}
	
	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			// For DirectShow
			HRESULT hr;
			LONGLONG pos = 0;

			if (m_pMSeek)
			{
				// Seek to the beginning
				hr = m_pMSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning,
											   NULL, AM_SEEKING_NoPositioning);
			}

			// Play the media
			if (m_pMControl)
			{
				if (SUCCEEDED(m_pMControl->Run()))
				{
					m_psCurrent = Playing;
					m_bSkipCheckFinished = FALSE;
				}
			}
			break;
		}
		case QuickTime:
		{
			// For QuickTime
			if (m_pQuickTime->theMovie)
			{
				// Seek to the beginning
				GoToBeginningOfMovie(m_pQuickTime->theMovie);

				if (m_QTStopCallBackID)
				{
					CallMeWhen(m_QTStopCallBackID, QTStopCallBackProc, (long)this, triggerAtStop, 0, 0);
				}
				
				// Play the media
				StartMovie(m_pQuickTime->theMovie);
				// Update playback status
				m_psCurrent = Playing;
				m_bSkipCheckFinished = FALSE;
			}
			break;
		}
		case MCI:
		{
			if (m_CDTrackNo)
			{
				m_CDAudio.PlayTrack(m_CDTrackNo, TRUE);
				m_psCurrent = Playing;
			}
		}
		default:
			// Do nothing
			break;
	}
}

// Return media type, Audio or Video
short CMediaCtrl::GetMediaType() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	if (m_bMediaLoaded == FALSE)
	{
		m_lMediaType = UnknownMedia;
	}
	return m_lMediaType;
}

// Determine the media type for QuickTime media file
void CMediaCtrl::GetQTMediaType(CQuickTime *pQuickTime)
{
	Track	aTrack = NULL;
	long	aTrackCount = 0;
	long	index;
	OSType	aMediaType;
	BOOL	haveMediaType = FALSE;
	BOOL	haveVideo = FALSE;
	BOOL	haveAudio = FALSE;
	
	m_lMediaType = UnknownMedia;
	
	aTrackCount = GetMovieTrackCount(pQuickTime->theMovie);
	if (aTrackCount == 0)
	{
		// no tracks in movie
		return; 
	}
 
	for (index = 1; index <= aTrackCount; index++)
	{
		aTrack = GetMovieIndTrack(pQuickTime->theMovie, index);
		GetMediaHandlerDescription( GetTrackMedia(aTrack), &aMediaType, NULL, NULL);
		
		if (aMediaType == VideoMediaType)
			haveVideo = TRUE;
		else if (aMediaType == SoundMediaType)
			haveAudio = TRUE;
	}

	if (haveAudio == TRUE && haveVideo == FALSE)
	{
		m_lMediaType = Audio;
	}
	else if (haveAudio == TRUE && haveVideo == TRUE)
	{
		m_lMediaType = Video;
	}
	else if (haveAudio == FALSE && haveVideo == TRUE)
	{
		m_lMediaType = Video;
	}
}

// Return media length in millisecond
long CMediaCtrl::GetMediaLength() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	if (m_bMediaLoaded == TRUE)
	{
		return m_lMediaLength;
	}
	else
	{
		return 0;
	}
}

// Return the audio renderer filter if found
HRESULT CMediaCtrl::FindAudioRenderer(IGraphBuilder *pGB, IBaseFilter **ppFilter)
{
    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    IPin *pPin;
    ULONG ulFetched, ulInPins, ulOutPins;
    BOOL bFound=FALSE;

    // Verify graph builder interface
    if (!pGB)
        return E_NOINTERFACE;

    // Clear the filter pointer in case there is no match
    if (ppFilter)
        *ppFilter = NULL;

    // Get filter enumerator
    hr = pGB->EnumFilters(&pEnum);
    if (FAILED(hr))
        return hr;

    pEnum->Reset();

    // Enumerate all filters in the graph
    while(!bFound && (pEnum->Next(1, &pFilter, &ulFetched) == S_OK))
    {
        // Find a filter with one input and no output pins
        hr = CountFilterPins(pFilter, &ulInPins, &ulOutPins);
        if (FAILED(hr))
            break;

        if ((ulInPins == 1) && (ulOutPins == 0))
        {
            // Get the first pin on the filter
            pPin = NULL;
            pPin = GetInPin(pFilter, 0);

            // Read this pin's major media type
            AM_MEDIA_TYPE type={0};
            hr = pPin->ConnectionMediaType(&type);
            if (FAILED(hr))
                break;

            // Is this pin's media type the requested type?
            // If so, then this is the renderer for which we are searching.
            // Copy the interface pointer and return.
            if (type.majortype == MEDIATYPE_Audio ||
				type.majortype == MEDIATYPE_Midi)
            {
                // Found our filter
                *ppFilter = pFilter;
                bFound = TRUE;
            }
            // This is not the renderer, so release the interface.
            else
                pFilter->Release();

            // Delete memory allocated by ConnectionMediaType()
            FreeMediaType(type);
			pPin->Release();
        }
        else
        {
            // No match, so release the interface
            pFilter->Release();
        }
    }

    pEnum->Release();
    return hr;
}

// Return the video renderer filter if found
HRESULT CMediaCtrl::FindVideoRenderer(IGraphBuilder *pGB, IBaseFilter **ppFilter)
{
    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    IPin *pPin;
    ULONG ulFetched, ulInPins, ulOutPins;
    BOOL bFound=FALSE;

    // Verify graph builder interface
    if (!pGB)
        return E_NOINTERFACE;

    // Clear the filter pointer in case there is no match
    if (ppFilter)
        *ppFilter = NULL;

    // Get filter enumerator
    hr = pGB->EnumFilters(&pEnum);
    if (FAILED(hr))
        return hr;

    pEnum->Reset();

    // Enumerate all filters in the graph
    while(!bFound && (pEnum->Next(1, &pFilter, &ulFetched) == S_OK))
    {
        // Find a filter with one input and no output pins
        hr = CountFilterPins(pFilter, &ulInPins, &ulOutPins);
        if (FAILED(hr))
            break;

        if ((ulInPins == 1) && (ulOutPins == 0))
        {
            // Get the first pin on the filter
            pPin = NULL;
            pPin = GetInPin(pFilter, 0);

            // Read this pin's major media type
            AM_MEDIA_TYPE type={0};
            hr = pPin->ConnectionMediaType(&type);
            if (FAILED(hr))
                break;

            // Is this pin's media type the requested type?
            // If so, then this is the renderer for which we are searching.
            // Copy the interface pointer and return.
            if (type.majortype == MEDIATYPE_Video)
            {
                // Found our filter
                *ppFilter = pFilter;
                bFound = TRUE;
            }
            // This is not the renderer, so release the interface.
            else
                pFilter->Release();

            // Delete memory allocated by ConnectionMediaType()
            FreeMediaType(type);
			pPin->Release();
        }
        else
        {
            // No match, so release the interface
            pFilter->Release();
        }
    }

    pEnum->Release();
    return hr;
}

// Reconnect directshow filter in case of changing media speed.
HRESULT CMediaCtrl::ReconnectGraph(IGraphBuilder *pGB)
{
    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    IPin *pPin;
    ULONG ulFetched, ulInPins, ulOutPins;
    BOOL bFound=FALSE;

    // Verify graph builder interface
    if (!pGB)
        return E_NOINTERFACE;

    // Get filter enumerator
    hr = pGB->EnumFilters(&pEnum);
    if (FAILED(hr))
        return hr;

    pEnum->Reset();

    // Enumerate all filters in the graph
    while(!bFound && (pEnum->Next(1, &pFilter, &ulFetched) == S_OK))
    {
        // Find a filter with one output pin and it is not connected
        hr = CountFilterPins(pFilter, &ulInPins, &ulOutPins);
        if (FAILED(hr))
            break;

        if (ulOutPins == 1)
        {
			IPin *pConnectedPin=NULL;

            // Get the first pin on the filter
            pPin = NULL;
            pPin = GetOutPin(pFilter, 0);

			if (pPin)
			{
				hr = pPin->ConnectedTo(&pConnectedPin);

				if (hr == VFW_E_NOT_CONNECTED)
				{
					// that is the one, reconnect this pin
					hr = pGB->Render(pPin);
	                bFound = TRUE;
				}
				pPin->Release();
		        pFilter->Release();
			}
        }
        else
        {
            // No match, so release the interface
            pFilter->Release();
        }
    }

    pEnum->Release();
    return hr;
}

// Free allocated memory for retrieved MediaType structure
void WINAPI CMediaCtrl::FreeMediaType(AM_MEDIA_TYPE &mt)
{
    if (mt.cbFormat != 0)
	{
        CoTaskMemFree((PVOID)mt.pbFormat);

        // Strictly unnecessary but tidier
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    
	if (mt.pUnk != NULL)
	{
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

// Count the number of input/output pins in a filter
HRESULT CMediaCtrl::CountFilterPins(IBaseFilter *pFilter, ULONG *pulInPins, ULONG *pulOutPins)
{
    HRESULT hr=S_OK;
    IEnumPins *pEnum=0;
    ULONG ulFound;
    IPin *pPin;

    // Verify input
    if (!pFilter || !pulInPins || !pulOutPins)
        return E_POINTER;

    // Clear number of pins found
    *pulInPins = 0;
    *pulOutPins = 0;

    // Get pin enumerator
    hr = pFilter->EnumPins(&pEnum);
    if(FAILED(hr)) 
        return hr;

    pEnum->Reset();

    // Count every pin on the filter
    while(S_OK == pEnum->Next(1, &pPin, &ulFound))
    {
        PIN_DIRECTION pindir = (PIN_DIRECTION)3;

        hr = pPin->QueryDirection(&pindir);

        if (pindir == PINDIR_INPUT)
            (*pulInPins)++;
        else
            (*pulOutPins)++;

        pPin->Release();
    } 

    pEnum->Release();
    return hr;
}

// Retrieve the input pin index by Num
IPin * CMediaCtrl::GetInPin(IBaseFilter *pFilter, int Num)
{
    CComPtr< IPin > pComPin;
    GetPin(pFilter, PINDIR_INPUT, Num, &pComPin);
    return pComPin;
}

// Retrieve the input or output pin index by iNum and the specified pin direction
HRESULT CMediaCtrl::GetPin(IBaseFilter *pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin)
{
    CComPtr< IEnumPins > pEnum;
    *ppPin = NULL;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if(FAILED(hr)) 
        return hr;

    ULONG ulFound;
    IPin *pPin;
    hr = E_FAIL;

    while(S_OK == pEnum->Next(1, &pPin, &ulFound))
    {
        PIN_DIRECTION pindir = (PIN_DIRECTION)3;
        pPin->QueryDirection(&pindir);
        if(pindir == dirrequired)
        {
            if(iNum == 0)
            {
				pPin->AddRef();
                *ppPin = pPin;
                // Found requested pin, so clear error
                hr = S_OK;
                break;
            }
            iNum--;
        } 

        pPin->Release();
    }

    return hr;
}

// Insert Tee and VU Meter filters into the graph
HRESULT CMediaCtrl::ConnectGrabberFilter(IGraphBuilder *pGB)
{
	IBaseFilter		*pFilter;
	HRESULT			hr;

	if (!pGB)
	{
		return E_POINTER;
	}

	hr = CoCreateInstance(CLSID_InfTee, NULL, CLSCTX_INPROC_SERVER,
							IID_IBaseFilter, (void **)&m_pTeeFilter);

	if (FAILED(hr))
	{
		Fire_Error(mediaErrorCreateTeeFilterFailed, "Failed to create DirectShow Tee Filter.");
		return E_FAIL;
	}
	
	// Add Tee Grabber Filter to the Graph Builder Filter
	hr = pGB->AddFilter(m_pTeeFilter, L"Tee Filter");

	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pTeeFilter);
		Fire_Error(mediaErrorTeeFilterNotInserted, "Failed to add Tee Filter to Filter Graphs.");
		return E_FAIL;
	}


	// Locate the audio renderer filter
	hr = FindAudioRenderer(pGB, &pFilter);
	if (SUCCEEDED(hr))
	{
		// Locate video renderer if no audio renderer is found
		if (!pFilter)
		{
			hr = FindVideoRenderer(pGB, &pFilter);
		}

		if (SUCCEEDED(hr) && pFilter)
		{
			IPin *pTeeInPin;
			IPin *pTeeOutPin;

			pTeeInPin = GetInPin(m_pTeeFilter, 0);
			pTeeOutPin = GetOutPin(m_pTeeFilter, 0);

			if (pTeeInPin && pTeeOutPin)
			{
				IPin	*pEndPin;
				IPin	*pStartPin;

				// Find its input pin
				pEndPin = GetInPin(pFilter, 0);
					
				if (pEndPin)
				{
					// Find which pin this pin connected to, disconnect it
					hr = pEndPin->ConnectedTo(&pStartPin);
					if (SUCCEEDED(hr))
					{
						// Disconnect connection
						hr = pGB->Disconnect(pStartPin);
						hr = pGB->Disconnect(pEndPin);

						// Reconnect audio grabber in between
						hr = pGB->Connect(pStartPin, pTeeInPin);
						hr = pGB->Connect(pTeeOutPin, pEndPin);

						SAFE_RELEASE(pStartPin);
					}
					SAFE_RELEASE(pEndPin);
				}
				SAFE_RELEASE(pTeeInPin);
				SAFE_RELEASE(pTeeOutPin);
			}
			SAFE_RELEASE(pFilter);
		}
	}

	// Connect Tee Filter to VUMeter Filter
	hr = CoCreateInstance(CLSID_VUMeter, NULL, CLSCTX_INPROC_SERVER,
							IID_IBaseFilter, (void **)&m_pVUMeterFilter);

	if (FAILED(hr))
	{
		Fire_Error(mediaErrorCreateVUMeterFilterFailed, "Failed to create DirectShow VUMeter Filter.");
		return E_FAIL;
	}
	
	// Add Tee Grabber Filter to the Graph Builder Filter
	hr = pGB->AddFilter(m_pVUMeterFilter, L"VU Meter Filter");

	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pVUMeterFilter);
		Fire_Error(mediaErrorVUMeterNotInserted, "Failed to add VUMeter Filter to Filter Graphs.");
		return E_FAIL;
	}

	IPin *pVUMeterInPin;
	IPin *pTeeOutPin;

	pVUMeterInPin = GetInPin(m_pVUMeterFilter, 0);
	pTeeOutPin = GetOutPin(m_pTeeFilter, 1);

	if (pVUMeterInPin && pTeeOutPin)
	{
		hr = pGB->Connect(pTeeOutPin, pVUMeterInPin);
	}

	SAFE_RELEASE(pVUMeterInPin);
	SAFE_RELEASE(pTeeOutPin);

	return S_OK;
}

// Remove Tee and VU Meter filters from the graph
HRESULT CMediaCtrl::DisconnectGrabberFilter(IGraphBuilder *pGB)
{
	HRESULT			hr;

	if (!pGB)
	{
		return E_POINTER;
	}

	if (m_pTeeFilter && m_pVUMeterFilter)
	{
		hr = pGB->RemoveFilter(m_pTeeFilter);
		hr = pGB->RemoveFilter(m_pVUMeterFilter);

		SAFE_RELEASE(m_pTeeFilter);
		SAFE_RELEASE(m_pVUMeterFilter);

		// Reconnect all existing filters
		ReconnectGraph(pGB);
	}
	return S_OK;
}

// Get output pin in a filter index by Num
IPin * CMediaCtrl::GetOutPin(IBaseFilter *pFilter, int Num)
{
    CComPtr< IPin > pComPin;
    GetPin(pFilter, PINDIR_OUTPUT, Num, &pComPin);
    return pComPin;
}

// Free allocated resources 
void CMediaCtrl::OnCloseMedia() 
{
	if (m_bMediaLoaded == FALSE)
		return;

	if (m_psCurrent == Playing)
	{
		OnStopMedia();
	}

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			FreePlaybackResources();

			break;
		}
		case QuickTime:
		{
			if (m_pQuickTime)
			{
				if (m_QTStopCallBackID)
				{
					DisposeCallBack(m_QTStopCallBackID);
					m_QTStopCallBackID = NULL;
				}
				m_pQuickTime->CloseMovie();
			}
			break;
		}
		case MCI:
		{
			m_CDAudio.Close();
			break;
		}
	}
	m_psCurrent = Stopped;
	m_lMediaType = UnknownMedia;
	m_lMediaLength = 0;
}

// Return TRUE for Windows Media file
BOOL CMediaCtrl::IsWindowsMediaFile(LPCTSTR lpszFile)
{
    if (_tcsstr(lpszFile, TEXT(".asf")) ||
        _tcsstr(lpszFile, TEXT(".ASF")) ||
        _tcsstr(lpszFile, TEXT(".asx")) ||
        _tcsstr(lpszFile, TEXT(".ASX")) ||
        _tcsstr(lpszFile, TEXT(".wma")) ||
        _tcsstr(lpszFile, TEXT(".WMA")) ||
        _tcsstr(lpszFile, TEXT(".wmv")) ||
        _tcsstr(lpszFile, TEXT(".WMV")))
        return TRUE;
    else
        return FALSE;
}

// Add supports for Windows Media files
HRESULT CMediaCtrl::AddKeyProvider(IGraphBuilder *pGraph)
{
    HRESULT hr;

    // Instantiate the key provider class, and AddRef it
    // so that COM doesn't try to free our static object.
    prov.AddRef();

    // Give the graph an IObjectWithSite pointer to us for callbacks & QueryService.
    IObjectWithSite* pObjectWithSite = NULL;

    hr = pGraph->QueryInterface(IID_IObjectWithSite, (void**)&pObjectWithSite);
    if (SUCCEEDED(hr))
    {
        // Use the IObjectWithSite pointer to specify our key provider object.
        // The filter graph manager will use this pointer to call
        // QueryService to do the unlocking.
        // If the unlocking succeeds, then we can build our graph.
	        
        hr = pObjectWithSite->SetSite((IUnknown *) (IServiceProvider *) &prov);
        pObjectWithSite->Release();
    }

    return hr;
}

// Add required filter for playback on specific pin (for Windows Media support)
HRESULT CMediaCtrl::RenderOutputPins(IGraphBuilder *pGB, IBaseFilter *pFilter)
{
	HRESULT			hr = S_OK;
	IEnumPins *		pEnumPin = NULL;
	IPin *			pConnectedPin = NULL, * pPin = NULL;
	PIN_DIRECTION	PinDirection;
	ULONG			ulFetched;

    // Enumerate all pins on the filter
	hr = pFilter->EnumPins(&pEnumPin);

	if(SUCCEEDED(hr))
	{
        // Step through every pin, looking for the output pins
		while (S_OK == (hr = pEnumPin->Next(1L, &pPin, &ulFetched)))
		{
            // Is this pin connected?  We're not interested in connected pins.
			hr = pPin->ConnectedTo(&pConnectedPin);
			if (pConnectedPin)
			{
				pConnectedPin->Release();
				pConnectedPin = NULL;
			}

            // If this pin is not connected, render it.
			if (hr == VFW_E_NOT_CONNECTED)
			{
				hr = pPin->QueryDirection(&PinDirection);
				if ((hr == S_OK) && (PinDirection == PINDIR_OUTPUT))
				{
                    hr = pGB->Render(pPin);
				}
			}
			pPin->Release();

            // If there was an error, stop enumerating
            if (FAILED(hr))                      
                break;
		}
	}

    // Release pin enumerator
	pEnumPin->Release();
	return hr;
}

// Notification to keep track the visibility of the control's window
void CMediaCtrl::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) 
{
	COleControl::OnWindowPosChanged(lpwndpos);
	
	// Save the control's Visible flag
	if (lpwndpos->flags & SWP_HIDEWINDOW)
	{
		if (m_bHideWindowForAudioOnlyFile != TRUE)
				m_bControlVisible = FALSE;
	}
	else if (lpwndpos->flags & SWP_SHOWWINDOW)
	{
		m_bControlVisible = TRUE;
	}
}

// Notification to keep track the visibility of the control's window
void CMediaCtrl::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	COleControl::OnShowWindow(bShow, nStatus);
	
	// Save the control's Visible flag
	if (bShow)
	{
		m_bControlVisible = TRUE;
	}
	else
	{
		if (m_bHideWindowForAudioOnlyFile != TRUE)
				m_bControlVisible = FALSE;
	}
}

// Adds a DirectShow filter graph to the Running Object Table,
// allowing GraphEdit to "spy" on a remote filter graph.
HRESULT CMediaCtrl::AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
    IMoniker * pMoniker;
    IRunningObjectTable *pROT;
    WCHAR wsz[128];
    HRESULT hr;

    if (FAILED(GetRunningObjectTable(0, &pROT)))
        return E_FAIL;

    wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, 
              GetCurrentProcessId());

    hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) 
    {
        hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
        pMoniker->Release();
    }
    pROT->Release();
    return hr;
}

// Adds a DirectShow filter graph to the Running Object Table,
// allowing GraphEdit to "spy" on a remote filter graph.
// Removes a filter graph from the Running Object Table
void CMediaCtrl::RemoveGraphFromRot(DWORD pdwRegister)
{
    IRunningObjectTable *pROT;

    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
    {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
}

// Find out the track number from the input audio file 
BYTE CMediaCtrl::ConvertFilenameToAudioTrack(LPCTSTR lpszFile)
{
	BYTE	nTrackNo=0;
	char	szDrive[MAX_PATH];
	char	szTrack[20];
	char	*p1, *p2;
	int		i, nTrack;

	lstrcpy(szDrive, lpszFile);
	p1 = strchr(szDrive, '\\');
	if (p1)
	{
		*(p1+1) = 0;

		if (GetDriveType(szDrive) == DRIVE_CDROM)
		{
			lstrcpy(szDrive, lpszFile);
			for (i=0; i<lstrlen(szDrive); i++)
			{
				szDrive[i] = tolower(szDrive[i]);
			}

			p1 = strstr(szDrive, "track");
			p2 = strstr(szDrive, ".cda");

			if (p1 && p2)
			{
				for (i=0; i<p2-p1-5; i++)
				{
					szTrack[i] = *(p1+5+i);
				}
				szTrack[i] = 0;

				nTrack = atoi(szTrack);

				if (nTrack > 255)
				{
					// Fire Error
					return 0;
				}
				nTrackNo = (BYTE)nTrack;
			}
		}
	}
	return 	nTrackNo;
}

// return TRUE if the input file is a Audio CD file
BOOL CMediaCtrl::IsAudioCDFile(LPCTSTR lpszPlayFile)
{
	int		nLen;

	nLen = lstrlen(lpszPlayFile);

	if (lpszPlayFile[nLen-4] == '.' &&
		lpszPlayFile[nLen-3] == 'c' &&
		lpszPlayFile[nLen-2] == 'd' &&
		lpszPlayFile[nLen-1] == 'a')
	{
		return TRUE;
	}
	return FALSE;
}

// Return the width of the video window
long CMediaCtrl::GetVideoWidth() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	return m_lVideoWidth;
}

// Return the height of the video window
long CMediaCtrl::GetVideoHeight() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	return m_lVideoHeight;
}

// Resize the video window according to DisplayMode
void CMediaCtrl::OnResize(long Width, long Height) 
{
	HDC		hdc;
	int		iLogPixelSX, iLogPixelSY;
	long	lNewControlWidth, lNewControlHeight;

	hdc = ::GetDC(NULL);

	iLogPixelSX = ::GetDeviceCaps(hdc, LOGPIXELSX);
	iLogPixelSY = ::GetDeviceCaps(hdc, LOGPIXELSY);

	::ReleaseDC(NULL, hdc);

	lNewControlWidth = iLogPixelSX * Width / 1440;
	lNewControlHeight = iLogPixelSY * Height / 1440;

	if (lNewControlWidth > 0 && lNewControlHeight > 0)
	{
		if (m_bMediaLoaded == TRUE && m_lMediaType == Video)
		{
			switch (m_CurrentPlaybackInterface)
			{
				case DirectShow:
				{
					if (m_lDisplayMode == Normal)
					{
						// Normal window size as media source input
						SetControlSize(m_lVideoWidth, m_lVideoHeight);
						m_pVideoWindow->SetWindowPosition(0, 0, m_lVideoWidth, m_lVideoHeight);
					}
					else if (m_lDisplayMode == Fit)
					{
						int		iXOffset, iYOffset;
						float	fXRatio, fYRatio, fAspectRatio;

						fAspectRatio = (float)m_lVideoWidth / (float)m_lVideoHeight;

						fXRatio = (float)m_lVideoWidth / (float)lNewControlWidth;
						fYRatio = (float)m_lVideoHeight / (float)lNewControlHeight;
						
						SetControlSize(lNewControlWidth, lNewControlHeight);

						if (fXRatio >= fYRatio)
						{
							int iYNewSize = (int)((float)lNewControlWidth / fAspectRatio);
							iYOffset = (lNewControlHeight - iYNewSize) / 2;
							m_pVideoWindow->SetWindowPosition(0, iYOffset, lNewControlWidth, iYNewSize);
						}
						else
						{
							int iXNewSize = (int)((float)lNewControlHeight * fAspectRatio);
							iXOffset = (lNewControlWidth - iXNewSize) / 2;
							m_pVideoWindow->SetWindowPosition(iXOffset, 0, iXNewSize, lNewControlHeight);
						}
					}
					else if (m_lDisplayMode == StretchFit)
					{
						SetControlSize(lNewControlWidth, lNewControlHeight);
						m_pVideoWindow->SetWindowPosition(0, 0, lNewControlWidth, lNewControlHeight);
					}

					InvalidateControl();

					break;
				}
				case QuickTime:
				{
					int	xoffset, yoffset, iNewWidth, iNewHeight;

					if (m_lDisplayMode == Normal)
					{
						xoffset = 0;
						yoffset = 0;
						iNewWidth = m_lVideoWidth;
						iNewHeight = m_lVideoHeight;

						// Set the control size same as the input
						SetControlSize(m_lVideoWidth, m_lVideoHeight);
					}
					else if (m_lDisplayMode == StretchFit)
					{
						// Set the size same as the control
						xoffset = 0;
						yoffset = 0;
						iNewWidth = lNewControlWidth;
						iNewHeight = lNewControlHeight;

						SetControlSize(lNewControlWidth, lNewControlHeight);
					}
					else if (m_lDisplayMode == Fit)
					{
						int		iXOffset, iYOffset;
						float	fXRatio, fYRatio, fAspectRatio;

						fAspectRatio = (float)m_lVideoWidth / (float)m_lVideoHeight;

						fXRatio = (float)m_lVideoWidth / (float)lNewControlWidth;
						fYRatio = (float)m_lVideoHeight / (float)lNewControlHeight;
						
						if (fXRatio >= fYRatio)
						{
							int iYNewSize = (int)((float)lNewControlWidth / fAspectRatio);
							iYOffset = (lNewControlHeight - iYNewSize) / 2;

							xoffset = 0;
							yoffset = iYOffset;
							iNewWidth = lNewControlWidth;
							iNewHeight = iYNewSize;
						}
						else
						{
							int iXNewSize = (int)((float)lNewControlHeight * fAspectRatio);
							iXOffset = (lNewControlWidth - iXNewSize) / 2;

							xoffset = iXOffset;
							yoffset = 0;
							iNewWidth = iXNewSize;
							iNewHeight = lNewControlHeight;
						}
						SetControlSize(lNewControlWidth, lNewControlHeight);
					}

					Rect WndRect;
					WndRect.top   =(short)yoffset;
					WndRect.left  =(short)xoffset;
					WndRect.bottom=(short)iNewHeight + m_iMCBarHeight;
					WndRect.right =(short)iNewWidth;

					// Set up the video window in QuickTime Movie Controller
					MCSetControllerBoundsRect(m_pQuickTime->theMC, &WndRect);

					InvalidateControl();

					break;
				}
				default:
					break;
			}
		}
	}
}

// Return the MCI CDROM device ID
long CMediaCtrl::GetMCIDeviceID() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	return m_CDAudio.GetDeviceID();
}

// Return CDROM volume 
BOOL CMediaCtrl::GetCdromVolume(short *nVolume)
{
	MIXERLINE	mxl;
	BOOL		bRet=FALSE;

	*nVolume = -1;

	if (!m_hMixer)
	{
		return FALSE;
	}

	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

	// Get CDROM line details
	if (mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxl,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
	{
		MIXERLINECONTROLS	mxlc;
		MIXERCONTROL		mxc;

		MIXERCONTROLDETAILS	mxcd;
		MIXERCONTROLDETAILS_UNSIGNED	*vol;

		// Check the number of channels available on the CDROM line
		if (mxl.cChannels > 0)
		{
			// Volume
			
			// Allocate memory for each channel's volume
			vol = new MIXERCONTROLDETAILS_UNSIGNED[mxl.cChannels];

			if (!vol)
			{
				// If memory allocation failed, fire Error event
				Fire_Error(mediaErrorOutOfMemory, "Memory allocation failed.", 0);
				return bRet;
			}

			// Get CDROM volume control
			ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));
			mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
			mxlc.dwLineID = mxl.dwLineID;
			mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
			mxlc.cControls = 1;
			mxlc.cbmxctrl = sizeof(MIXERCONTROL);
			mxlc.pamxctrl = &mxc;
			
			mxc.cbStruct = sizeof(MIXERCONTROL);

			if (mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxlc,
						MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
			{
				// Save Volume Control ID for later windows callback
				// message MM_MIXM_CONTROL_CHANGE
				m_dwVolumeControlID = mxc.dwControlID;

				// Get CDROM volume
				ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));
				mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mxcd.dwControlID = mxc.dwControlID;
				mxcd.cChannels = mxl.cChannels;
				mxcd.cMultipleItems = 0;
				mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mxcd.paDetails = &vol[0];

				if (mixerGetControlDetails((HMIXEROBJ)m_hMixer, &mxcd,
							MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
				{
					// Convert volume in 0 - 100 scale
					*nVolume = (short)((vol[0].dwValue * 100) / (mxc.Bounds.dwMaximum - mxc.Bounds.dwMinimum));

					bRet = TRUE;
				}
				else
				{
					Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM volume.");
				}
			}
			else
			{
				Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM volume line control.");
			}

			// Release allocated memory
			delete vol;
		}
		else
		{
			Fire_Error(mediaErrorGetSoundCardDetails, "No audio channel is found.");
		}
	}
	else
	{
		Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM line information.");
	}

	return bRet;
}

// Set CDROM volume 
BOOL CMediaCtrl::SetCdromVolume(short nVolume)
{
	MIXERLINE	mxl;
	BOOL		bRet=FALSE;

	if (!m_hMixer)
	{
		return FALSE;
	}

	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

	// Get CDROM line details
	if (mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxl,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
	{
		MIXERLINECONTROLS	mxlc;
		MIXERCONTROL		mxc;

		MIXERCONTROLDETAILS	mxcd;
		MIXERCONTROLDETAILS_UNSIGNED	*vol;

		// Check the number of channels available on the CDROM line
		if (mxl.cChannels > 0)
		{
			// Volume
			
			// Allocate memory for each channel's volume
			vol = new MIXERCONTROLDETAILS_UNSIGNED[mxl.cChannels];

			if (!vol)
			{
				// If memory allocation failed, fire Error event
				Fire_Error(mediaErrorOutOfMemory, "Memory allocation failed.", 0);
				return bRet;
			}

			// Get CDROM volume control
			ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));
			mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
			mxlc.dwLineID = mxl.dwLineID;
			mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
			mxlc.cControls = 1;
			mxlc.cbmxctrl = sizeof(MIXERCONTROL);
			mxlc.pamxctrl = &mxc;
			
			mxc.cbStruct = sizeof(MIXERCONTROL);

			if (mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxlc,
					MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
			{
				// Set CDROM volume
				ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));
				mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mxcd.dwControlID = mxc.dwControlID;
				mxcd.cChannels = mxl.cChannels;
				mxcd.cMultipleItems = 0;
				mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mxcd.paDetails = &vol[0];

				// Convert volume into device's scale
				DWORD dwVolume = nVolume * (mxc.Bounds.dwMaximum - mxc.Bounds.dwMinimum) / 100;

				for (DWORD i=0; i<mxcd.cChannels; i++)
				{
					vol[i].dwValue = dwVolume;
				}
				
				if (mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd,
							MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
				{
					bRet = TRUE;
				}
				else
				{
					Fire_Error(mediaErrorGetSoundCardDetails, "Failed to set CDROM volume.");
				}
			}
			else
			{
				Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM volume line control.");
			}

			// Release allocated memory
			delete vol;
		}
		else
		{
			Fire_Error(mediaErrorGetSoundCardDetails, "No audio channel is found.");
		}
	}
	else
	{
		Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM line information.");
	}

	return bRet;
}

// Get CDROM Mute state
BOOL CMediaCtrl::GetCdromMuteState(BOOL *MuteState)
{
	MIXERLINE	mxl;
	BOOL		bRet=FALSE;

	*MuteState = FALSE;

	if (!m_hMixer)
	{
		return FALSE;
	}

	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

	// Get CDROM line details
	if (mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxl,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
	{
		MIXERLINECONTROLS				mxlc;
		MIXERCONTROL					mxc;
		MIXERCONTROLDETAILS				mxcd;
		MIXERCONTROLDETAILS_BOOLEAN		mute;

		// Get CDROM Mute
		ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));
		mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
		mxlc.dwLineID = mxl.dwLineID;
		mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = &mxc;
		
		mxc.cbStruct = sizeof(MIXERCONTROL);

		if (mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxlc,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
		{
			// Save Mute Control ID for later windows callback
			// message MM_MIXM_CONTROL_CHANGE
			m_dwMuteControlID = mxc.dwControlID;

			// Get Mute state
			ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));
			mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
			mxcd.dwControlID = mxc.dwControlID;
			mxcd.cChannels = 1;
			mxcd.cMultipleItems = 0;
			mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
			mxcd.paDetails = &mute;

			if (mixerGetControlDetails((HMIXEROBJ)m_hMixer, &mxcd,
						MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
			{
				// Return Mute state
				if (mute.fValue)
				{
					*MuteState = TRUE;
				}
				else
				{
					*MuteState = FALSE;
				}

				bRet = TRUE;
			}
			else
			{
				Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM mute state.");
			}
		}
		else
		{
			Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM mute line control.");
		}
	}
	else
	{
		Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM line information.");
	}

	return bRet;
}

// Set CDROM Mute state
BOOL CMediaCtrl::SetCdromMuteState(BOOL MuteState)
{
	MIXERLINE	mxl;
	BOOL		bRet=FALSE;

	if (!m_hMixer)
	{
		return FALSE;
	}

	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

	// Get CDROM line details
	if (mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxl,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
	{
		MIXERLINECONTROLS				mxlc;
		MIXERCONTROL					mxc;
		MIXERCONTROLDETAILS				mxcd;
		MIXERCONTROLDETAILS_BOOLEAN		mute;

		// Get CDROM Mute
		ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));
		mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
		mxlc.dwLineID = mxl.dwLineID;
		mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = &mxc;
		
		mxc.cbStruct = sizeof(MIXERCONTROL);

		if (mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxlc,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
		{
			// Set Mute state
			ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));
			mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
			mxcd.dwControlID = mxc.dwControlID;
			mxcd.cChannels = 1;
			mxcd.cMultipleItems = 0;
			mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
			mxcd.paDetails = &mute;

			mute.fValue = MuteState;

			if (mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd,
						MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
			{
				bRet = TRUE;
			}
			else
			{
				Fire_Error(mediaErrorGetSoundCardDetails, "Failed to set CDROM mute state.");
			}
		}
		else
		{
			Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM mute line control.");
		}
	}
	else
	{
		Fire_Error(mediaErrorGetSoundCardDetails, "Failed to get CDROM line information.");
	}

	return bRet;
}

// Return Mute state
BOOL CMediaCtrl::GetMuteState() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}

	return m_bGlobalMuteState;
}

// Set Mute state
void CMediaCtrl::SetMuteState(BOOL bNewValue) 
{
	// Set the global mute state
	m_bGlobalMuteState = bNewValue;

	if (SetCdromMuteState(bNewValue) == FALSE)
	{
		Fire_Error(mediaErrorSetMuteState, "Failed to set CDROM mute state.");
	}

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			IBasicAudio *pBasicAudio=NULL;
			long		lVolume;

			if (m_pGraph)
			{
				m_pGraph->QueryInterface(IID_IBasicAudio, (void **)&pBasicAudio);

				if (pBasicAudio)
				{
					if (bNewValue == TRUE)
					{
						// Mute
						lVolume = -10000;

						if (FAILED(pBasicAudio->put_Volume(lVolume)))
						{
							Fire_Error(mediaErrorSetVolume, "Failed to set volume of DirectShow Media.", 0);
						}
						SAFE_RELEASE(pBasicAudio);
					}
					else
					{
						// Un-Mute
						SAFE_RELEASE(pBasicAudio);
						SetVolume(m_lGlobalVolume);
					}
				}
			}
			break;
		}
		case QuickTime:
		{
			if (m_pQuickTime->theMovie)
			{
				if (bNewValue == TRUE)
				{
					// Mute
					SetMovieVolume(m_pQuickTime->theMovie, -1 * GetMovieVolume(m_pQuickTime->theMovie));
				}
				else
				{
					// Un-Mute
					SetVolume(m_lGlobalVolume);
				}
			}
			break;
		}
		default:
			break;
	}
}

// Determine the version of DirectX installed
BOOL CMediaCtrl::OnCheckDXVersion() 
{
    DWORD  dwDXVersion = GetDXVersion();

    switch( dwDXVersion )
    {
        case 0x000:
            // No DirectX installed
        case 0x100:
            // DirectX 1 installed
        case 0x200:
            // DirectX 2 installed
			break;

        case 0x300:
            // DirectX 3 installed
        case 0x500:
            // DirectX 5 installed
        case 0x600:
            // DirectX 6 installed
        case 0x601:
            // DirectX 6.1 installed
        case 0x700:
            // DirectX 7
        case 0x800:
            // DirectX 8.0
        case 0x801:
            // DirectX 8.1 or better installed
			return TRUE;

        default:
            // Unknown version of DirectX installed.
            break;
    }
	return FALSE;
}

// Get media volume 
long CMediaCtrl::GetVolume() 
{
	// Not supported in design mode
	if (AmbientUserMode()== FALSE)
	{
		GetNotSupported();
	}
	return m_lGlobalVolume;
}

// Set media volume 
void CMediaCtrl::SetVolume(long lNewValue) 
{
	double		y;

	// Set the global Volume
	m_lGlobalVolume = lNewValue;

	if (SetCdromVolume((short)lNewValue) == FALSE)
	{
		Fire_Error(mediaErrorSetVolume, "Failed to set CDROM volume.");
	}

	if (m_bMediaLoaded == FALSE)
	{
		// Fire Error
		return;
	}

	if (lNewValue < 0 || lNewValue > 100)
	{
		Fire_Error(mediaErrorSetVolume, "Volume value is out of range (0-100).");
		return;
	}

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			IBasicAudio *pBasicAudio=NULL;
			long		lVolume;

			// contruct volume value using a exponential function
			//y = 3.05 / 100;
			//y = y * (100 - lNewValue);
			//lVolume = (long)pow(20, y) * -1;
			
			lVolume = m_plDXMediaVolumeLookup[lNewValue];
			if (m_pGraph)
			{
				m_pGraph->QueryInterface(IID_IBasicAudio, (void **)&pBasicAudio);

				if (pBasicAudio)
				{
					if (FAILED(pBasicAudio->put_Volume(lVolume)))
					{
						Fire_Error(mediaErrorSetVolume, "Failed to set volume of DirectShow Media.");
					}
					SAFE_RELEASE(pBasicAudio);
				}
			}
			break;
		}
		case QuickTime:
		{
			if (m_pQuickTime)
			{
				ShortFixed vol;
				Fixed f;
				unsigned char  intPart, fracPart;

				y = (double)lNewValue / 100.0;
				f = X2Fix(y);
				
				intPart = (char)(f >> 16 & 0xFF);
				fracPart = (char)(f >> 8 & 0xFF);
				
				vol = intPart;
				vol = vol << 8;
				vol += fracPart;
				if (m_pQuickTime->theMovie)
					SetMovieVolume(m_pQuickTime->theMovie, vol);
			}
			break;
		}
	}
}

// Rewind the media file
void CMediaCtrl::OnRewind() 
{
    HRESULT			hr;
	long			lIncrement;
	long			lNewPos;
	double			dtmp;
	//char			str[100];

	// Return if media file is not loaded
	if (m_bMediaLoaded == FALSE)
	{
		return;
	}

	dtmp = m_lMediaLength;
	dtmp = dtmp * m_dAdvancePercent / 100.0;
	lIncrement = (long)dtmp;

	switch (m_CurrentPlaybackInterface)
	{
		case DirectShow:
		{
			// for DirectShow interface

			// Move forward
			lNewPos = m_lMediaPosition - lIncrement;
			if (lNewPos >= 0)
			{
				LONGLONG pos = llMulDiv(10000, lNewPos, 1, 0);

				if (m_pMSeek)
				{
					// seek to absolute position
					hr = m_pMSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning,
												   NULL, AM_SEEKING_NoPositioning);
					m_lLastStopPosition = lNewPos;
				}
			}
			//wsprintf(str, "LP=%d, MP=%d, I=%d, lPos=%d\n", m_lLastStopPosition, m_lMediaPosition, lIncrement, lPos);
			//TRACE(str);
			break;
		}
		case QuickTime:
		{
			lNewPos = m_lMediaPosition - lIncrement;
			if (lNewPos >= 0)
			{
				SeekTo(lNewPos);
				m_lMediaPosition = lNewPos;
			}
			break;
		}
		case MCI:
		{
			lNewPos = m_lMediaPosition - lIncrement;
			if (lNewPos >= 0)
			{
				SeekTo(lNewPos);
				m_lMediaPosition = lNewPos;
			}
			break;
		}
		default:
			// Do nothing
			break;
	}
}

// Get the percentage of media length for fastforward or rewind
double CMediaCtrl::GetAdvancePercent() 
{
	return m_dAdvancePercent;
}

// Set the percentage of media length for fastforward or rewind
void CMediaCtrl::SetAdvancePercent(double newValue) 
{
	if (newValue > 0.0 && newValue <= 50.0)
	{
		m_dAdvancePercent = newValue;
		SetModifiedFlag();
	}
}

// for firing custom error event
void CMediaCtrl::Fire_Error(SCODE scode, LPCTSTR lpszDescription, UINT nHelpID)
{
	USES_CONVERSION;

	ExternalAddRef();   // "Insurance" addref -- keeps control alive.

	BSTR bstrDescription = ::SysAllocString(T2COLE(lpszDescription));
	LPCTSTR lpszSource = AfxGetAppName();
	LPCTSTR lpszHelpFile = _T("");

	if (nHelpID != 0)
		lpszHelpFile = AfxGetApp()->m_pszHelpFilePath;

	if (lpszHelpFile == NULL)
		lpszHelpFile = _T("");

	BOOL bCancelDisplay = TRUE;

	FireEvent(DISPID_ERROREVENT, EVENT_PARAM(VTS_I2 VTS_PBSTR VTS_SCODE VTS_BSTR VTS_BSTR VTS_I4 VTS_PBOOL), (WORD)SCODE_CODE(scode),
		&bstrDescription, scode, lpszSource, lpszHelpFile, (DWORD)nHelpID,
		&bCancelDisplay);

	if (!bCancelDisplay)
		MessageBox(lpszDescription, lpszSource);

	::SysFreeString(bstrDescription);

	ExternalRelease();
}
