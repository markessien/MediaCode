/*
	File:		CQuickTime.c

	Written by: Keith Gurganus

	Modified by: Calvin Tong
*/

#include "stdafx.h"

#include "QTML.h"
#include "CQuickTime.h"
#include <MediaHandlers.h>

CQuickTime::CQuickTime()
{
	movieOpened = FALSE;
	theMovie = NULL;
    theMC = NULL;
	theSoundMediaHandler = NULL;
	theHwnd = NULL;
	theViewHwnd = NULL;
	theAppName[0] = '\0';
	theFullPath[0] = '\0';
}

CQuickTime::~CQuickTime()
{
	CloseMovie();
}

void CQuickTime::NewMovieFile(void)
{
	// Close any previously opened movie
	CloseMovie();

	// Set the port	
	SetGWorld((CGrafPtr)GetHWNDPort(theViewHwnd), nil);

	// Create a newMovie
	theMovie = NewMovie(newMovieActive);

	if ( theMovie != NULL )
	{
		// set window title to name	
		strcpy((char*)theFullPath,"Untitled");

		// Create the movie controller
		CreateNewMovieController(theMovie);
		
		movieOpened = TRUE;
	}	
}


BOOL CQuickTime::OpenMovie(unsigned char *fullPath)
{
	BOOL	isMovieGood = FALSE;

	if ( strlen ((char*)fullPath ) != 0)
	{
		OSErr				err;
		short				theFile = 0;
		long				controllerFlags = 0L;
		FSSpec				sfFile;
		short				movieResFile;

		// Close any previously opened movie
		CloseMovie();
	
		// make a copy of our full path name
		lstrcpy ( (char *)theFullPath, (const char *) fullPath );

		// convert theFullPath to pstring
		CToPstr((char*)theFullPath);

		// Make a FSSpec with a pascal string filename
		FSMakeFSSpec(0,0L,theFullPath, &sfFile);
		
		// Set the port	
		SetGWorld((CGrafPtr)GetHWNDPort(theViewHwnd), nil);

		// Open the movie file
		err = OpenMovieFile(&sfFile, &movieResFile, fsRdPerm);
		if (err == noErr)
		{
			// Get the Movie from the file
			err = NewMovieFromFile(&theMovie,movieResFile, 
									nil, 
									nil, 
									newMovieActive, /* flags */
									nil);
		
			// Close the movie file
			CloseMovieFile(movieResFile);

			if (err == noErr)
			{
				// Create the movie controller
			   	CreateNewMovieController(theMovie);
				isMovieGood = movieOpened = TRUE;	
				PToCstr((char*)theFullPath);
			} else
				theFullPath[0] = '\0'; 
				
		} else
			theFullPath[0] = '\0';
		
		theSoundMediaHandler = GetSoundMediaHandler(theMovie);
		MediaSetSoundLevelMeteringEnabled(theSoundMediaHandler, TRUE);
	}
	return isMovieGood;
}

void CQuickTime::CloseMovie(void)
{
	if (movieOpened == TRUE)
	{
		movieOpened = FALSE;
 	
		if (theMC)
			DisposeMovieController(theMC);

		if (theMovie)
			DisposeMovie(theMovie);

		theMovie = NULL;
		theMC = NULL;
	}

	/* set window title to empty name */
	theFullPath[0] ='\0';
}

void CQuickTime::ProcessMovieEvent(HWND hWnd, unsigned int message, unsigned int wParam, long lParam) 
{	
	// Convert the Windows event to a QTML event
	MSG				theMsg;
	EventRecord		macEvent;
	LONG			thePoints = GetMessagePos();

    theMsg.hwnd = hWnd;
    theMsg.message = message;
    theMsg.wParam = wParam;
    theMsg.lParam = lParam;
    theMsg.time = GetMessageTime();
    theMsg.pt.x = LOWORD(thePoints);
    theMsg.pt.y = HIWORD(thePoints);

	// tranlate a windows event to a mac event
	WinEventToMacEvent(&theMsg, &macEvent);

	// Pump messages as mac event
    MCIsPlayerEvent(theMC,(const EventRecord *)&macEvent);
}

void CQuickTime::CreateNewMovieController(Movie theMovie)
{
	long 	controllerFlags;

	// 0,0 Movie coordinates
	GetMovieBox(theMovie, &theMovieRect);
	MacOffsetRect(&theMovieRect, -theMovieRect.left, -theMovieRect.top);

	CRect WndRect0;
	::GetClientRect(theViewHwnd,&WndRect0);
	Rect WndRect;
	WndRect.top   =(short)WndRect0.top;
    WndRect.left  =(short)WndRect0.left;
    WndRect.bottom=(short)WndRect0.bottom;
    WndRect.right =(short)WndRect0.right;

	// Attach a movie controller
	theMC = NewMovieController(theMovie, &WndRect, mcTopLeftMovie );
	MCSetVisible(theMC, FALSE);

	// Enable editing
	MCEnableEditing(theMC,FALSE);

	// suppress movie badge
	MCDoAction(theMC, mcActionSetUseBadge, (void *)FALSE);

	// Tell the controller to attach a movie's CLUT to the window as appropriate.
	MCDoAction(theMC, mcActionGetFlags, &controllerFlags);
	MCDoAction(theMC, mcActionSetFlags, (void *)(controllerFlags | mcFlagsUseWindowPalette));
	// Do not Allow the controller to accept keyboard events
	MCDoAction(theMC, mcActionSetKeysEnabled, (void *)FALSE);
	// Disable drag support
	MCDoAction(theMC, mcActionSetDragEnabled, (void *)FALSE);
	// Set the controller action filter
	MCSetActionFilterWithRefCon(theMC, MCFilter, (long)this);
}

void CQuickTime::OnMovieWindowDestroy() 
{	
	CGrafPtr	windowPort = NULL;
	
	// close any movies	before destroying PortAssocation
	CloseMovie();

	// Destroy the view's GrafPort <-> HWND association
	if (theViewHwnd)
		windowPort = (CGrafPtr)GetHWNDPort(theViewHwnd);
	
	if (windowPort)
		DestroyPortAssociation(windowPort);
}

void CQuickTime::GetFileNameFromFullPath(unsigned char *fileName) 
{
	/* pluck the filename from the fullpath, */
	int		i = 0, j = -1, stringLen = 0;

	stringLen = strlen((char *)theFullPath);
	if (stringLen > 0 ) {
		while(i<stringLen){
			if (theFullPath[i] == 0x5c || theFullPath[i] == '/' )
				j = i;
			i++;
		}
		if ( j>-1)
			strcpy((char *)fileName, (char *)&theFullPath[j+1]);
		else
			strcpy((char *)fileName, (char *)theFullPath);

	}
}

void CQuickTime::GetAppName(unsigned char *appName)
{ 
#if (!WIN32)
	HINSTANCE theModule = (HINSTANCE)GetWindowLong(theHwnd, GWL_HINSTANCE);
	GetModuleFileName(theModule, (char *)appName, strlen((char *)appName) );
#else
	if (strlen((char*)theAppName) > 0)
		strcpy((char*)appName, (char*)theAppName);
#endif
}

void CQuickTime::CToPstr(char *theString)
{
	char	tempString[256];

	tempString[0] = strlen (theString);
	tempString[1] = '\0';

	strcat ( tempString, theString );
	strcpy	( theString, tempString );
}

void CQuickTime::PToCstr(char *theString)
{
	char	tempString[256];
	int		len = theString[0];

	memcpy ( tempString, &theString[1], theString[0]);
	tempString[len] = '\0';
	strcpy	( theString, tempString );
}

Movie CQuickTime::GetMovie()
{
	return theMovie;
}

//////////
//
// GetSoundMediaHandler
// Return the sound media handler for a movie.
// 
//////////

MediaHandler CQuickTime::GetSoundMediaHandler(Movie theMovie)
{
	Track		myTrack;
	Media		myMedia;

	myTrack = GetMovieIndTrackType(theMovie, 1, AudioMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly);
	if (myTrack != NULL)
	{
		myMedia = GetTrackMedia(myTrack);
		return (GetMediaHandler(myMedia));
	} 
		
	return(NULL);
}
