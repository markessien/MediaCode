#include "stdafx.h"

#include <QTML.h>
#include <MediaHandlers.h>
#include "MediaCtl.h"

// QuickTime Callback for handling all events from QT Movie Controller
// Only handle the idle event for obtaining the audio VU meter levels
// Parameter refCon is the CQuickTime class pointer defined in CMediaCtrl class
Boolean MCFilter(MovieController mc, short action, void *params, long refCon)
{
	switch (action)
	{
		// handle idle events
		case mcActionIdle:
		{
			// While idle, retrieve audio VU meter levels
			CQuickTime*	myQTClass = (CQuickTime*)refCon;
			LevelMeterInfo	LMInfo;
			int	iLeftLevel, iRightLevel;

			// Get VU peak levels from QT interface
			MediaGetSoundLevelMeterInfo(myQTClass->theSoundMediaHandler, &LMInfo);
			
			if (LMInfo.numChannels > 0)
			{
				long	lparm;

				iLeftLevel = (100 * LMInfo.leftMeter) / 255;
				iRightLevel = (100 * LMInfo.rightMeter) / 255;

				lparm = iLeftLevel;
				lparm = lparm << 16;
				lparm += iRightLevel;

				// Notify this to the control using window message
				if (myQTClass->theViewHwnd)
					SendMessage(myQTClass->theViewHwnd, WM_QTVUPEAKLEVEL, LMInfo.numChannels, lparm);
			}
			break;
		}
	}
	return FALSE;
}

// QuickTime Callback for handling the playback completed event
// Parameter refCon is the CQuickTime class pointer defined in CMediaCtrl class
void QTStopCallBackProc(QTCallBack cb, long refCon)
{
	CQuickTime *myQTClass = (CQuickTime *)refCon;

	// Notify this event to the control using window message
	if (myQTClass->theViewHwnd)
		SendMessage(myQTClass->theViewHwnd, WM_QTPLAYCOMPLETED, 0, 0L);
}
