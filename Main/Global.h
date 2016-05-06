#include <evcode.h>
//
// Constants
//
#define VOLUME_FULL     0L
#define VOLUME_SILENCE  -10000L

#define WM_GRAPHNOTIFY		WM_USER+13
#define WM_MEDIAPOSITION	WM_USER+14
#define WM_QTVUPEAKLEVEL	WM_USER+15
#define WM_QTPLAYCOMPLETED	WM_USER+16

#define EC_VUMETERLEVEL		EC_USER+1
#define EC_MEDIAPOS			EC_USER+2

#define MYCTL_E_QTNOTINSTALLED				CUSTOM_CTL_SCODE(1000)
#define MYCTL_E_FILENOTFOUND				CUSTOM_CTL_SCODE(1001)
#define MYCTL_E_UNKNOWNFILEFORMAT			CUSTOM_CTL_SCODE(1002)
#define MYCTL_E_INVALIDPARAMTER				CUSTOM_CTL_SCODE(1003)
#define MYCTL_E_CREATEDSHOWGRAPHFILTER		CUSTOM_CTL_SCODE(1004)
#define MYCTL_E_EMPTYMEDIAFILE				CUSTOM_CTL_SCODE(1005)
#define	MYCTL_E_CREATEDSHOWTEEFILTER		CUSTOM_CTL_SCODE(1006)
#define	MYCTL_E_ADDTEEFILTER				CUSTOM_CTL_SCODE(1007)
#define	MYCTL_E_SETAUDIOMEDIATYPE			CUSTOM_CTL_SCODE(1008)
#define	MYCTL_E_CREATEVUMETERFILTER			CUSTOM_CTL_SCODE(1009)
#define MYCTL_E_ADDVUMETERFILTER			CUSTOM_CTL_SCODE(1010)
#define MYCTL_E_CDROMNOTREADY				CUSTOM_CTL_SCODE(1011)
#define MYCTL_E_NOSOUNDCARD					CUSTOM_CTL_SCODE(1012)
#define MYCTL_E_GETSOUNDCARDDETAILS			CUSTOM_CTL_SCODE(1013)
#define MYCTL_E_GETMUTESTATE				CUSTOM_CTL_SCODE(1014)
#define MYCTL_E_SETMUTESTATE				CUSTOM_CTL_SCODE(1015)
#define MYCTL_E_GETVOLUME					CUSTOM_CTL_SCODE(1016)
#define MYCTL_E_SETVOLUME					CUSTOM_CTL_SCODE(1017)

enum PLAYSTATE {Stopped, Playing, Paused};
enum WINDOWFITMODE {Normal, Fit, StretchFit};
enum TIMEFORMATMODE {UnknownUnit, Millisecond, Frame};
enum PLAYBACKINTERFACE {DirectShow, QuickTime, MCI};
enum MEDIATYPE {UnknownMedia, Audio, Video};
enum AUDIOFORMAT {UnknownFormat, Mono8, Stereo8, Mono16, Stereo16};

enum MEDIAERROR
{
		mediaErrorNoError = 0,
		mediaErrorGeneric,
		mediaErrorException,
		mediaErrorOutOfMemory,
		mediaErrorFileNotFound,
		mediaErrorQTNotInstalled,
		mediaErrorUnknownFileFormat,
		mediaErrorInvalidParameter,
		mediaErrorCreateGraphBuilderFailed,
		mediaErrorEmptyMediaFile,
		mediaErrorCreateTeeFilterFailed,
		mediaErrorTeeFilterNotInserted,
		mediaErrorSetAudioMediaType,
		mediaErrorCreateVUMeterFilterFailed,
		mediaErrorVUMeterNotInserted,
		mediaErrorCDRomNotReady,
		mediaErrorNoSoundCardInstalled,
		mediaErrorGetSoundCardDetails,
		mediaErrorGetMuteState,
		mediaErrorSetMuteState,
		mediaErrorGetVolume,
		mediaErrorSetVolume,
		
		mediaErrorSysDeviceEnumCreationFailed,
		mediaErrorCreateClassEnumeratorFailed,
		mediaErrorGraphRenderFailed,
		mediaErrorMediaControlNotRetrieved,
		mediaErrorGraphCouldNotBeRun
};

//
// Macros
//
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

typedef struct tagCallbackStruc
{
	REFERENCE_TIME		rtSampleStartTime;
	ULONG				ulVUMeterLevels;
} VUMeterCallbackStruc, *LPVUMeterCallbackStruc;
