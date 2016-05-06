#if !defined(AFX_MEDIACTL_H__13DFFCF2_3283_438D_8D4F_785F3049060A__INCLUDED_)
#define AFX_MEDIACTL_H__13DFFCF2_3283_438D_8D4F_785F3049060A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <tchar.h>
#include <Qedit.h>
#include "global.h"
#include "cdaudio.h"

#ifdef __cplusplus
extern "C" {
#endif
void CALLBACK MediaPositionTimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
#ifdef __cplusplus
}
#endif

extern DWORD  GetDXVersion();

#include "CQuickTime.h"

// MediaCtl.h : Declaration of the CMediaCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CMediaCtrl : See MediaCtl.cpp for implementation.

class CMediaCtrl : public COleControl
{
	DECLARE_DYNCREATE(CMediaCtrl)

// Constructor
public:
	CMediaCtrl();
	CQuickTime			*m_pQuickTime;		// QuickTime object
	WAVEFORMATEX		m_DSAudioFormat;	// Audio format
	PLAYBACKINTERFACE	m_CurrentPlaybackInterface;	// Current playback interface used
	PLAYSTATE			m_psCurrent;				// Current playback state

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMediaCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CMediaCtrl();

	IGraphBuilder	*m_pGraph;
	IMediaControl	*m_pMControl;
	IVideoWindow	*m_pVideoWindow;
	IMediaEventEx	*m_pMEvent;
	IBasicAudio		*m_pBasicAudio;
	IBasicVideo		*m_pBasicVideo;
	IMediaSeeking	*m_pMSeek;
	IBaseFilter		*m_pTeeFilter;
	IBaseFilter		*m_pVUMeterFilter;

	HWND			m_ControlHwnd;
	WINDOWFITMODE	m_lDisplayMode;
	BOOL			m_bAudioOnly;
	LONG			m_lVolume;
	TIMEFORMATMODE	m_lTimeFormat;
	LONG			m_lMediaLength;
	LONG			m_lMediaPosition;
	BOOL			m_bSeekToZeroWhenFinished;
	BOOL			m_bInitDirectShow;
	BOOL			m_bQuickTimeInstalled;
	BOOL			m_bPlayInLoop;
	long			m_lVideoHeight;
	long			m_lVideoWidth;
	long			m_lLastMediaPos;
	short			m_lCheckFinishedCount;
	short			m_iMCBarHeight;
	BOOL			m_bSkipCheckFinished;
	short			m_lMediaType;

	UINT			m_TimerEventID;

	CCdAudio		m_CDAudio;

	BEGIN_OLEFACTORY(CMediaCtrl)        // Class factory and guid
		virtual BOOL VerifyUserLicense();
		virtual BOOL GetLicenseKey(DWORD, BSTR FAR*);
	END_OLEFACTORY(CMediaCtrl)

//	DECLARE_OLECREATE_EX(CMediaCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CMediaCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CMediaCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CMediaCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CMediaCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CMediaCtrl)
	afx_msg long GetPlayState();
	afx_msg void SetPlayState(long nNewValue);
	afx_msg long GetDisplayMode();
	afx_msg void SetDisplayMode(long nNewValue);
	afx_msg long GetMediaPositionLong();
	afx_msg long GetTimeFormat();
	afx_msg void SetTimeFormat(long nNewValue);
	afx_msg short GetMediaType();
	afx_msg long GetMediaLength();
	afx_msg long GetVolume();
	afx_msg void SetVolume(long nNewValue);
	afx_msg long GetVideoWidth();
	afx_msg long GetVideoHeight();
	afx_msg long GetMCIDeviceID();
	afx_msg BOOL GetMuteState();
	afx_msg void SetMuteState(BOOL bNewValue);
	afx_msg double GetAdvancePercent();
	afx_msg void SetAdvancePercent(double newValue);
	afx_msg long OnChangedMediaFile(LPCTSTR PlaybackFile, BOOL LoopMe, const VARIANT FAR& PlayNow, const VARIANT FAR& SeekToZeroAtTheEndOfPlayback);
	afx_msg void OnPauseMedia();
	afx_msg void OnResumeMedia();
	afx_msg void OnStopMedia();
	afx_msg long GetNumberOfSoundCardMixers();
	afx_msg BOOL HasASoundCardInstalled();
	afx_msg double SetMediaSpeed(double Speed);
	afx_msg void SeekTo(long SeekPosition);
	afx_msg long GetWindowHwnd();
	afx_msg void OnFastForward();
	afx_msg void OnPlay();
	afx_msg void OnCloseMedia();
	afx_msg void OnResize(long Width, long Height);
	afx_msg BOOL OnCheckDXVersion();
	afx_msg void OnRewind();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CMediaCtrl)
	void FireFinished()
		{FireEvent(eventidFinished,EVENT_PARAM(VTS_NONE));}
	void FireFileLoaded()
		{FireEvent(eventidFileLoaded,EVENT_PARAM(VTS_NONE));}
	void FireMediaPosition(LPCTSTR TimeLength, long Length)
		{FireEvent(eventidMediaPosition,EVENT_PARAM(VTS_BSTR  VTS_I4), TimeLength, Length);}
	void FireVUPeakLevel(short nChannels, short leftMeter, short rightMeter)
		{FireEvent(eventidVUPeakLevel,EVENT_PARAM(VTS_I2  VTS_I2  VTS_I2), nChannels, leftMeter, rightMeter);}
	void FireCDMute(BOOL bMuteFlag)
		{FireEvent(eventidCDMute,EVENT_PARAM(VTS_BOOL), bMuteFlag);}
	void FireCDVolume(long lVolume)
		{FireEvent(eventidCDVolume,EVENT_PARAM(VTS_I4), lVolume);}
//	void Fire_Error(short Number, BSTR FAR* Description, SCODE Scode, LPCTSTR Source, LPCTSTR HelpFile, long HelpContext, BOOL *CancelDisplay)
//		{FireEvent(DISPID_ERROREVENT,EVENT_PARAM(VTS_I2  VTS_PBSTR  VTS_SCODE  VTS_BSTR  VTS_BSTR  VTS_I4  VTS_PBOOL), Number, Description, Scode, Source, HelpFile, HelpContext, CancelDisplay);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:

	enum {
	//{{AFX_DISP_ID(CMediaCtrl)
	dispidPlayState = 1L,
	dispidDisplayMode = 2L,
	dispidMediaPositionLong = 3L,
	dispidTimeFormat = 4L,
	dispidMediaType = 5L,
	dispidMediaLength = 6L,
	dispidVolume = 7L,
	dispidVideoWidth = 8L,
	dispidVideoHeight = 9L,
	dispidMCIDevID = 10L,
	dispidMute = 11L,
	dispidAdvancePercent = 12L,
	dispidMediaFile = 13L,
	dispidPauseMedia = 14L,
	dispidResumeMedia = 15L,
	dispidStopMedia = 16L,
	dispidGetNumberOfSoundCardMixers = 17L,
	dispidHasASoundCardInstalled = 18L,
	dispidMediaSpeed = 19L,
	dispidSeekTo = 20L,
	dispidGetWindowHwnd = 21L,
	dispidFastForward = 22L,
	dispidPlay = 23L,
	dispidCloseMedia = 24L,
	dispidResize = 25L,
	dispidCheckDXVersion = 26L,
	dispidRewind = 27L,
	eventidFinished = 1L,
	eventidFileLoaded = 2L,
	eventidMediaPosition = 3L,
	eventidVUPeakLevel = 4L,
	eventidCDMute = 5L,
	eventidCDVolume = 6L,
	//}}AFX_DISP_ID
	};

private:
	long	m_lGlobalVolume;
	BOOL	m_bGlobalMuteState;
	BOOL	m_bOldCDROMMuteState;
	long	m_lOldCDROMVolume;
	BOOL	m_bOldMuteState;
	long	m_lOldVolume;
	DWORD	m_dwVolumeControlID;
	DWORD	m_dwMuteControlID;
	double	m_dAdvancePercent;
	long	*m_plDXMediaVolumeLookup;

	// Sound card CDROM line interface
	HMIXER	m_hMixer;
	BOOL	SetCdromMuteState(BOOL MuteState);
	BOOL	SetCdromVolume(short nVolume);
	BOOL	GetCdromMuteState(BOOL *MuteState);
	BOOL	GetCdromVolume(short *nVolume);
	
	// Audio CD related
	DWORD	m_CDLastPos;
	DWORD	m_CDLastTrackPos;
	BYTE	m_CDTrackNo;
	BOOL	m_bHideWindowForAudioOnlyFile;
	BOOL	IsAudioCDFile(LPCTSTR lpszPlayFile);
	BYTE	ConvertFilenameToAudioTrack(LPCTSTR lpszFile);
	
	// Registration to DirectShow Runtime Object Table manager 
	void	RemoveGraphFromRot(DWORD pdwRegister);
	HRESULT	AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);

	BOOL	m_bControlVisible;
	HRESULT	RenderOutputPins(IGraphBuilder *pGB, IBaseFilter *pFilter);
	HRESULT	AddKeyProvider(IGraphBuilder *pGraph);
	
	// Windows Media Related
	BOOL	IsWindowsMediaFile(LPCTSTR lpszFile);

	// QuickTime Related
	void		GetQTMediaType(CQuickTime *pQuickTime);
	TimeBase	m_QTCBTimeBase;
	QTCallBack	m_QTStopCallBackID;

	// Directshow helper functions
	BOOL	m_bMediaLoaded;
	ULONG	m_lLastStopPosition;
	IPin	*GetInPin( IBaseFilter * pFilter, int Num );
	IPin	*GetOutPin( IBaseFilter * pFilter, int Num );
	HRESULT	GetPin( IBaseFilter * pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin);
	HRESULT CountFilterPins(IBaseFilter *pFilter, ULONG *pulInPins, ULONG *pulOutPins);
	HRESULT FindAudioRenderer(IGraphBuilder *pGB, IBaseFilter **ppFilter);
	HRESULT FindVideoRenderer(IGraphBuilder *pGB, IBaseFilter **ppFilter);
	void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt);
	void	FreePlaybackResources();
	HRESULT	HandleGraphEvent();
	HRESULT	ConnectGrabberFilter(IGraphBuilder *pGB);
	HRESULT	DisconnectGrabberFilter(IGraphBuilder *pGB);
	HRESULT ReconnectGraph(IGraphBuilder *pGB);

	void	ClearControl();
	BOOL	CheckFileExisted(LPCTSTR szMediaFile);
	void	Fire_Error(SCODE scode, LPCTSTR lpszDescription, UINT nHelpID=0);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MEDIACTL_H__13DFFCF2_3283_438D_8D4F_785F3049060A__INCLUDED)
