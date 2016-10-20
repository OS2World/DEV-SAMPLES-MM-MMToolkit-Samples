
/*****************************************************************************\
*  
*  BocaSoft System Sounds interface code 
*
*  Copyright (c) BocaSoft Incorporated 1993
*
*  BocaSoft Incorporated  
*  117 NW 43rd St. 
*  Boca Raton, FL 
*  33431 
*  (407)392-7743  
*  
*  
*  These two routines demonstrate a simple way for an OS/2 PM application 
*  to trigger sounds within the BocaSoft System Sounds product.  By using
*  our product in conjunction with MMPM/2 a developer can take advantage 
*  of high performance audio playback with minimal code.
*  
*  The first routine, "FindSystemSounds", returns a window handle which in
*  turn is used by the second routine, "PlaySound", to post a window message
*  with the appropriate audio index.  The audio index must be associated with
*  an enabled audio file within the BocaSoft product.  We suggest using
*  either the User Events (45-49) or an obscure key combination such as 
*  CTRL-SHIFT-ALT to avoid conflicts with main system events.  Index values
*  for keys can be found in the keys dialog in the "index" field. 
*
*  You must have a PM application to use this technique.  Other application
*  types may use the named-pipe interface.  See the included REXX and batch
*  files for more information. 
*
*  Event Index Values 	-> 0 to 49
*  
*  Key Index Values:
*  ----------------
*  
*  No control keys 	-> index = 128 + scan code
*  SHIFT		-> index = 128 + scan code + (256 * 1)
*  ALT			-> index = 128 + scan code + (256 * 2)
*  CTRL	 		-> index = 128 + scan code + (256 * 3)
*  SHIFT | ALT		-> index = 128 + scan code + (256 * 4)
*  SHIFT | CTRL		-> index = 128 + scan code + (256 * 5)
*  ALT | CTRL		-> index = 128 + scan code + (256 * 6)
*  SHIFT | ALT | CTRL	-> index = 128 + scan code + (256 * 7)
*  
\*****************************************************************************/

#define		INCL_WIN
#define		INCL_PM                         

#include	<os2.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#define	SSND_PROG_NAME	"BocaSoft System Sounds"
#define	SSND_WM_PLAY	0x5000

ULONG	PlaySound (HWND, ULONG);
HWND	FindSystemSounds (VOID);

main()
{
	HMQ	hmq;	/* Message queue handle */
	HWND	hwnd;	/* System Sounds window handle */	
	HAB	hab;	/* PM anchor block handle */

	/* Initialize PM */

	if (!(hab = WinInitialize(0))) 
		exit (1);

	/* Create a message queue */

	if (!(hmq = WinCreateMsgQueue( hab, 0 )))
		exit (1);

	/* Find BocaSoft System Sounds */

	if (!(hwnd = FindSystemSounds()))
		exit (1);

	/* Play "User Event (45)" - (make sure it is enabled) */

	PlaySound (hwnd, 45);
}



/*****************************************************************************\
*
*  Subroutine Name:  FindSystemSounds
*
*  Function:  Get the window handle for BocaSoft System Sounds.
*
\*****************************************************************************/

HWND	FindSystemSounds (VOID)
{
 	HWND  	hwndParent;     /* whose child windows are enumed */ 
	HWND  	hwndNext;	/* current enumeration handle     */
 	HENUM  	henum; 		/* enumeration handle		  */
	CHAR	szWinName[50];  /* window text */
	BOOL	bFound = FALSE;
 
	/* Enumerate all desktop windows and search for 
	BocaSoft System Sounds */

	hwndParent = HWND_DESKTOP;
 
	henum = WinBeginEnumWindows (hwndParent);
 
	while ((hwndNext = WinGetNextWindow (henum)) != NULLHANDLE)
	{
		WinQueryWindowText (hwndNext, sizeof(szWinName), szWinName);

		if (!(strcmp(szWinName, SSND_PROG_NAME)))
		{
			bFound = TRUE;
			break;
		}
	}
	WinEndEnumWindows (henum);

	if (!bFound)
		hwndNext = 0;

	return (hwndNext);
}


/*****************************************************************************\
*
*  Subroutine Name:  PlaySound
*
*  Function:  Post a window message to BocaSoft System Sounds with
*             a sound index.
*
\*****************************************************************************/

ULONG	PlaySound (HWND hwnd,
	ULONG ulIndex)
{
	ULONG	ulRc = 0;

	/* Send a message to System Sounds */

	ulRc = WinPostMsg (hwnd,
		SSND_WM_PLAY,
		0,
		(MPARAM)ulIndex);

	return (ulRc);
}




