// CQuickTime.h : interface of the CQuickTime class
//
/////////////////////////////////////////////////////////////////////////////

#include <Movies.h>
#include <scrap.h>

#include <windows.h>	// Microsoft Windows
#include <winuser.h>


#ifndef __CQUICKTIME__
#define __CQUICKTIME__

#ifdef __cplusplus
extern "C" {
#endif
Boolean MCFilter(MovieController mc, short action, void*params, long refCon);
void QTStopCallBackProc(QTCallBack cb, long refCon);
#ifdef __cplusplus
}
#endif

class CQuickTime
{

public:
	CQuickTime();
	~CQuickTime();

	virtual BOOL	OpenMovie(unsigned char *fullPath);
	virtual void	CloseMovie(void);
	virtual void	NewMovieFile(void);
	virtual void	ProcessMovieEvent(HWND hWnd, unsigned int message, unsigned int wParam, long lParam); 
	virtual void	OnMovieWindowDestroy();
	virtual void	CreateNewMovieController(Movie theMovie);
	virtual MediaHandler GetSoundMediaHandler(Movie theMovie);

	virtual void	GetFileNameFromFullPath(unsigned char *fileName); 
	virtual void	GetAppName(unsigned char *appName); 

	virtual void	CToPstr(char *theString);
	virtual void	PToCstr(char *theString);

	virtual Movie	GetMovie(void);

public:
	unsigned char	theAppName[128];

	BOOL			movieOpened;
	Movie			theMovie;
    MovieController theMC;
	MediaHandler    theSoundMediaHandler;
	Rect			theMovieRect;
	Rect			theMCRect;
	unsigned char	theFullPath[255];

	HWND			theHwnd;
	HWND			theViewHwnd;
	Rect			theWindowRect;

// Operations

};

#endif
