/*************************************************************************
 * File Name    :  movie.c
 *
 * Description  :  This file contains the C source code required for the
 *                 movie sample program.
 *
 * Concepts     :  The sample program illustrates how an application
 *                 can play a movie in a independent manner.
 *
 *                 This sample application will demonstrate device control
 *                 of a Software Motion Video device, playing a movie
 *                 in an application specified window and in a window
 *                 provided by the Softare Motion Video subsystem.
 *
 * MMPM/2 API's :  List of all MMPM/2 API's that are used in
 *                 this module
 *
 *                 mciSendCommand    MCI_OPEN
 *                                   MCI_PLAY
 *                                   MCI_PAUSE
 *                                   MCI_RESUME
 *                                   MCI_CLOSE
 *                                   MCI_SET
 *                                   MCI_WINDOW
 *                                   MCI_PUT
 *                  mmioGetHeader
 *                  mmioSet
 *                  mmioOpen
 *                  mmioClose
 *
 * Required
 *    Files     :  movie.c        Source Code.
 *                 movie.h        Include file.
 *                 movie.dlg      Dialog definition.
 *                 movie.rc       Resources.
 *                 movie.mak      Make file.
 *                 movie.def      Linker definition file.
 *                 movie.ico      Program icon.
 *
 * Copyright (C) IBM 1993
 *************************************************************************/

#define  INCL_WIN                   /* required to use Win APIs.           */
#define  INCL_PM                    /* required to use PM APIs.            */
#define  INCL_WINHELP               /* required to use IPF.                */
#define  INCL_OS2MM                 /* required for MCI and MMIO headers   */
#define  INCL_MMIOOS2               /* required for MMVIDEOHEADER          */
#define  INCL_MMIO_CODEC            /* required for circular, secondary    */
#define  INCL_SW                    /* required for circular, secondary    */
                                    /* windows and graphic buttons         */
#define  INCL_WINSTDFILE            /* required for open file dlg          */
#define  INCL_WINSTDDLGS            /* required for open file dlg          */

#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sw.h>

#include "movie.h"

enum DeviceStates {ST_STOPPED, ST_PLAYING, ST_PAUSED};
/*
 * Procedure/Function Prototypes
 */

MRESULT EXPENTRY MainDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

BOOL    DoesFileExist( PSZ pszFileName );
VOID    Finalize( VOID );
VOID    Initialize( VOID );
VOID    InitializeHelp( VOID );
USHORT  MessageBox( USHORT usMessageID, ULONG  ulStyle );
BOOL    PlayTheAppFile( HWND hwnd );
BOOL    PlayTheDefaultFile( HWND hwnd );
BOOL    PlayTheMovie( HWND hwnd );
VOID    SetMovieVolume( VOID );
VOID    ShowMCIErrorMessage( ULONG ulError );
VOID    StopTheDevice( VOID );
VOID    ResumeTheMovie( VOID );
VOID    PauseTheMovie( VOID );
VOID    CloseTheDevice( VOID );
BOOL    OpenTheDevice( HWND );
BOOL    SendString( HWND hwnd, PCHAR pcMCIString, USHORT usUserParm );
VOID    ResizeMovieWindow(HWND hwnd);

/*************** End of Procedure/Function Prototypes *************************/

  /*
  * Global Variables.
  */
HAB    hab;
HMQ    hmq;
HWND   hwndFrame;                      /* Handle to the frame window.         */
HWND   hwndClient;                     /* Handle to the client App window     */
HWND   hwndAppFrame;                   /* Handle to the App frame window      */
HWND   hwndHelpInstance;               /* Handle to Main Help window.         */
HWND   hwndMainDialogBox;              /* Handle to the dialog window.        */
HWND   hwndPlayPB;                     /* Handle to the play push button      */
HWND   hwndApplication;                /* Handle to the Application RB        */
HWND   hwndStandard;                   /* Handle to the Standard RB           */
HWND   hwndVolumeSlider;               /* Handle to the Volume slider         */
ULONG  ulMovieLength;                  /* Length of movie in mmtimeperframe   */
ULONG  ulMovieWidth;                   /* Width of movie                      */
ULONG  ulMovieHeight;                  /* Height of movie                     */
SWP    swpAppFrame;
HMMIO  hFile;                          /* handle to file                      */

enum   DeviceStates   eState = ST_STOPPED;      /* state of the device       */

SHORT  sVolumeLevel   = INIT_VOLUME;   /* desired volume level                */
BOOL   fPassedDevice  = FALSE;         /* for MCI_ACQUIRE to play the file    */
BOOL   fDeviceOpen    = FALSE;         /* indicate we've opened for first time*/
BOOL   fDevicePlaying = FALSE;         /* indicate we've opened for first time*/
CHAR   achMsgBoxTitle[MAXNAMEL];       /* Error message box title             */

CHAR   szReturn[CCHMAXPATH];           /* return value from mciSendString     */
SHORT  sResizeWindow = 0;              /* flag set for resizing the widow     */
HPOINTER  hptrWait, hptrArrow;  /* Pointer handles for use during long waits. */

/************************** End of Global Variables ***************************/

/******************************************************************************
 * Name         : main
 *
 * Description  : This function calls the Initialize procedure to prepare
 *                everything for the program's operation, enters the
 *                message loop, then calls Finalize to shut everything down
 *                when the program is terminated.
 *
 * Concepts     : None.
 *
 * MMPM/2 API's : None.
 *
 * Parameters   : argc - Number of parameters passed into the program.
 *                argv - Command line parameters.
 *
 * Return       : TRUE is returned to the operating system.
 *
 ******************************************************************************/
INT main( VOID )
{
   QMSG    qmsg;

   Initialize();

   while ( WinGetMsg( hab, (PQMSG) &qmsg, (HWND) NULL, 0, 0) )
      WinDispatchMsg( hab, (PQMSG) &qmsg );

   Finalize();

   return( TRUE);

} /* End of main */

/******************************************************************************
 * Name         : Initialize
 *
 * Description  : This function performs the necessary initializations and
 *                setups that are required to show/run a secondary window
 *                as a main window.  The message queue will be created, as will
 *                the dialog box.
 *
 * Concepts     : None
 *
 * MMPM/2 API's : WinLoadSecondaryWindow
 *
 * Parameters   : None.
 *
 * Return       : None.
 *
 ******************************************************************************/
VOID Initialize( VOID)
{
   CHAR     szDefaultSize[CCHMAXPATH];   /* buffer for default size menu text */
   CHAR     achTitle[MAXNAMEL];          /* buffer for window title text      */
   LONG     lmciSendStringRC;            /* return code from SendString       */

   hab       = WinInitialize( 0);
   hmq       = WinCreateMsgQueue( hab, 0);

   hptrArrow = WinQuerySysPointer ( HWND_DESKTOP, SPTR_ARROW, FALSE );
   hptrWait  = WinQuerySysPointer ( HWND_DESKTOP, SPTR_WAIT,  FALSE );

   /*****************************************************************/
   /* Initialize the window. First, change pointer to a waiting     */
   /* pointer, since this might take a couple of seconds.           */
   /*****************************************************************/

   WinSetPointer ( HWND_DESKTOP, hptrWait );

   WinLoadString(
      hab,                                  /* HAB for this window.           */
      (HMODULE) NULL,                       /* Get the string from the .exe.  */
      IDS_DEFAULTSIZE,                      /* Which string to get.           */
      sizeof(szDefaultSize),                /* The size of the buffer.        */
      szDefaultSize);                       /* The buffer to place the string.*/

   WinLoadString(
      hab,                                  /* HAB for this dialog box.       */
      (HMODULE) NULL,                       /* Get the string from the .exe.  */
      IDS_MOVIE_ERROR,                      /* Which string to get.           */
      (SHORT) sizeof( achMsgBoxTitle),      /* The size of the buffer.        */
      achMsgBoxTitle);                      /* The buffer to place the string.*/

   hwndFrame =                    /* Returns the handle to the frame.         */
       WinLoadSecondaryWindow(
          HWND_DESKTOP,           /* Parent of the dialog box.                */
          HWND_DESKTOP,           /* Owner of the dialog box.                 */
          (PFNWP) MainDlgProc,    /* 'Window' procedure for the dialog box.   */
          (HMODULE) NULL,         /* Where is the dialog.  Null is EXE file.  */
          IDD_MAIN_WINDOW,        /* Dialog ID.                               */
          (PVOID) NULL);          /* Creation Parameters for the dialog.      */

   /**************************************************************************/
   /* Retrieve the handle to the dialog box by specifying the QS_DIALOG flag.*/
   /**************************************************************************/

   hwndMainDialogBox = WinQuerySecondaryHWND(hwndFrame, QS_DIALOG);

   /**************************************************************************/
   /* Add Default Size menu item to system menu of the secondary window.     */
   /**************************************************************************/

   WinInsertDefaultSize(hwndFrame, szDefaultSize);

   /******************************************************************/
   /* Get the window title string from the Resource string table     */
   /* and set it as the window text for the dialog window.           */
   /******************************************************************/
   WinLoadString(
      hab,                                  /* HAB for this dialog box.       */
      (HMODULE) NULL,                       /* Get the string from the .exe.  */
      IDS_PROGRAM_TITLE,                    /* Which string to get.           */
      (SHORT) sizeof( achTitle),            /* The size of the buffer.        */
      achTitle);                            /* The buffer to place the string.*/

   WinSetWindowText( hwndFrame, achTitle);

  /*************************************************************************/
  /* This new window is created for playing a movie in a application       */
  /* defined window.  When playing a movie in this window the Sendstring   */
  /* function will be passed this windows handle in order to know where    */
  /* to put the movie before playing.                                      */
  /*************************************************************************/

    WinRegisterClass(
      hab,
    "MovieWindow",
    (PFNWP) NULL,
    CS_SIZEREDRAW | CS_MOVENOTIFY,
    0 );

  hwndAppFrame =  WinCreateWindow((HWND)hwndMainDialogBox,
                                 "MovieWindow",
                                  NULL,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  (HWND)hwndMainDialogBox,
                                  HWND_TOP, ID_APPWINDOW, NULL, NULL );

   WinShowWindow( hwndFrame, TRUE );           /* Display the main window.    */

   InitializeHelp();                           /* Initialize the help.        */

   /*
    * Now that we're done here, change the pointer back to the arrow.
    */
   WinSetPointer ( HWND_DESKTOP, hptrArrow );

} /* end initialization */

/******************************************************************************
 * Name         : Finalize
 *
 * Description  : This routine is called after the message dispatch loop
 *                has ended because of a WM_QUIT message.  The code will
 *                destroy the help instance, messages queue, and window.
 *
 * Concepts     : None.
 *
 * MMPM/2 API's : WinDestroySecondaryWindow
 *
 * Parameters   : None.
 *
 * Return       : None.
 *
 ******************************************************************************/
VOID Finalize( VOID )
{
   /*
    * Destroy the Help Instances.
    */
   if ( hwndHelpInstance != (HWND) NULL)
   {
      WinDestroyHelpInstance( hwndHelpInstance );
   }

   WinDestroySecondaryWindow( hwndFrame );
   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );

}  /* End of Finalize */

/******************************************************************************
 * Name         : MainDlgProc
 *
 * Description  : This function controls the main dialog box.  It will handle
 *                received messages such as pushbutton notifications, timing
 *                events, etc.
 *
 * Concepts     : Illustrates:
 *                  - How to participate in MMPM/2 device sharing scheme.
 *                  - How to handle notification messages.
 *                  - How to implement graphic pushbutton's
 *                  - How to implement secondary windows.
 *
 * MMPM/2 API's : WinDefSecondaryWindowProc
 *                mciSendString
 *                   - acquire
 *
 * Parameters   : hwnd - Handle for the Main dialog box.
 *                msg  - Message received by the dialog box.
 *                mp1  - Parameter 1 for the just received message.
 *                mp2  - Parameter 2 for the just received message.
 *
 * Return       : 0 or the result of default processing.
 *
 ******************************************************************************/
MRESULT EXPENTRY MainDlgProc( HWND   hwnd,
                              ULONG  msg,
                              MPARAM mp1,
                              MPARAM mp2 )
{
   LONG        lmciSendStringRC;      /* return value form mciSendString      */
   HPOINTER    hpProgramIcon;         /* handle to program's icon             */
   USHORT      usCommandMessage;      /* command message for notify           */
   USHORT      usNotifyCode;          /* notification message code            */
   SHORT       sWmControlID;          /* WM_CONTROL id                        */
   ULONG       ulFrame;               /* video position in frames.            */
   MRESULT     mresult;               /* return result                        */

   switch( msg )
   {
      case WM_INITDLG :
         hpProgramIcon =
            WinLoadPointer(
               HWND_DESKTOP,
               (HMODULE) NULL,              /* Resource is kept in .Exe file. */
               ID_ICON );                   /* Which icon to use.             */

         WinDefSecondaryWindowProc(
            hwnd,                    /* Dialog window handle.                 */
            WM_SETICON,              /* Message to the dialog.  Set it's icon.*/
            (MPARAM) hpProgramIcon,
            (MPARAM) 0 );            /* mp2 no value.                         */

         /*
          * get all the required handles.
          */
                                                         /* volume slider     */
         hwndVolumeSlider = WinWindowFromID( hwnd, IDC_VOLUME_SLIDER );

                                                         /* play push button  */
         hwndPlayPB      = WinWindowFromID( hwnd, IDC_GPB_PLAY );

                                                         /* application RB    */
         hwndApplication = WinWindowFromID( hwnd, IDC_RADIO1 );

        if (!DoesFileExist( "movie.avi" ))
          {
             MessageBox( IDS_CANNOT_FIND_MOVIE_FILE,              /* ID of the message          */
                         MB_CANCEL | MB_ERROR  | MB_MOVEABLE);    /* style         */

             WinEnableWindow( hwndPlayPB, FALSE );
          }


         /********************************************************************/
         /* The slider control cannot be completely set from the dialog      */
         /* template so some aspects must be set here.  We will set the      */
         /* volume range to 0-100, increment to 1-10, and the initial        */
         /* volume level to 100.                                             */
         /********************************************************************/
         WinSendMsg( hwndVolumeSlider,
                     CSM_SETRANGE,
                     (MPARAM) 0L,
                     (MPARAM) 100L);

         WinSendMsg( hwndVolumeSlider,
                     CSM_SETINCREMENT,
                     (MPARAM) 10L,
                     (MPARAM) 1L);

         WinSendMsg( hwndVolumeSlider,
                     CSM_SETVALUE,
                     (MPARAM) sVolumeLevel,
                     (MPARAM) NULL);

         /*****************/
         /* Volume Slider */
         /*****************/

         WinEnableWindow( hwndVolumeSlider, TRUE );

         /**********************************************/
         /* Set the application window as the default  */
         /**********************************************/

         WinSendMsg(hwndApplication, BM_SETCHECK, MPFROMSHORT(1), NULL);

         return( (MRESULT) 0);

      case WM_CLOSE :

         /*********************************************/
         /* If the device is opened, close the device  /
         /*********************************************/

         if (fDeviceOpen)
            CloseTheDevice();

         return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2 ) );

      case WM_HELP :

       /*********************************************************************/
       /* The dialog window has received a request for help from the user.  */
       /* Send the HM_DISPLAY_HELP message to the Help Instance with the IPF*/
       /* resource identifier for the correct HM panel.  This will show the */
       /* help panel for this sample program.                               */
       /*********************************************************************/

         WinSendMsg( hwndHelpInstance,
                     HM_DISPLAY_HELP,
                     MPFROMSHORT(1),
                     MPFROMSHORT(HM_RESOURCEID));
         return( (MRESULT) 0);


      /*************************************************************/
      /* This message is recieved when the user changes the volume */
      /* or the movie position slider.                             */
      /*************************************************************/

      case WM_CONTROL:
        sWmControlID  = SHORT1FROMMP(mp1);
        usNotifyCode  = (USHORT) SHORT2FROMMP(mp1);

        switch ( sWmControlID )
        {

           case IDC_VOLUME_SLIDER:
              if ( ( usNotifyCode == CSN_CHANGED ) ||
                   ( usNotifyCode == CSN_TRACKING ) )
              {

               /**************************************************************/
               /*  Every time the volume control setting is changed, save the*/
               /*  new value in sVolumeLevel.                                */
               /**************************************************************/

                 sVolumeLevel = SHORT1FROMMP (mp2);

                /*****************************/
                /*  Set the new volume level */
                /*****************************/

                 SetMovieVolume();
              }
              break;
        }

        return( (MRESULT) 0);

      case WM_COMMAND :

        /**********************************************************************/
        /* To get which pushbutton was pressed the SHORT1FROMMP macro is used.*/
        /**********************************************************************/

         switch (SHORT1FROMMP(mp1))
         {
            case IDC_GPB_PLAY:               /* user selected "Play"          */
               switch ( eState )
               {
                  /**********************************************************/
                  /* If the Movie is currently stopped, check to see if we  */
                  /* were playing, if we were resume.                       */
                  /**********************************************************/
                  case ST_STOPPED:

                    /****************************************************/
                    /* Open the moive, resize the window if needed for  */
                    /* the movie, then start playing.                   */
                    /****************************************************/
                     if (!fDeviceOpen)
                       {
                         OpenTheDevice ( hwnd );
                       } /* if */
                     if(sResizeWindow == 0 )
                       {
                         ResizeMovieWindow(hwnd);
                         sResizeWindow = 1;
                       }
                     PlayTheMovie( hwnd );
                     break;

                  /******************************************************/
                  /* If the Movie is currently paused, resume the Movie */
                  /******************************************************/
                  case ST_PAUSED:
                     ResumeTheMovie();
                     break;
               }
               break;

            case IDC_GPB_PAUSE:
               switch ( eState )
               {
                 /************************************************/
                 /* If the Movie is currently playing, pause it. */
                 /************************************************/

                 case ST_PLAYING:
                    PauseTheMovie();
                    break;

                 /***********************************************************/
                 /* If the Movie is currently paused, resume the Movie play.*/
                 /***********************************************************/

                 case ST_PAUSED:
                    ResumeTheMovie();
                    break;
               }
              break;

            case IDC_GPB_REWIND:         /* user selected "Rewind" pushbutton */
               if (fDeviceOpen)          /* If the device is opened           */
                 {
                  StopTheDevice();
                  fDevicePlaying = FALSE;
                 }
               break;

            case IDC_GPB_STOP:             /* user selected "Stop" pushbutton */

               /***********************************************************/
               /* If the Movie is not in stopped state, stop the device.  */
               /***********************************************************/

               if (eState != ST_STOPPED)
                 {
                    StopTheDevice();
                 }
               break;

            default:
               break;

         }  /* End of Command Switch */

         return( (MRESULT) 0);

      /**************************************************************
       * The next two messages are handled so that this application can
       * participate in device sharing.  Since it opens the device as
       * shareable device, other applications can gain control of the device.
       * When this happens, we will receive a MM_MCIPASSDEVICE message.  We
       * keep track of this device passing in the fPassedDevice Boolean
       * variable.
       *
       * We use the WM_ACTIVATE message to participate in device
       * sharing.  We first check to see if this is an activate
       * or a deactivate message (indicated by mp1).  Then,
       * we check to see if we've passed control of the duets'
       * devices.  If these conditions are true, then
       * we issue an acquire device command to regain control of
       * the device, since we're now the active window on the screen.
       *
       * This is one possible method that can be used to implement
       * device sharing. For applications that are more complex
       * than this sample program, developers may wish to take
       * advantage of a more robust method of device sharing.
       * This can be done by using the MCI_ACQUIRE_QUEUE flag on
       * the MCI_ACQUIREDEVICE command.  Please refer to the MMPM/2
       * documentation for more information on this flag.
       **************************************************************/

      case MM_MCIPASSDEVICE:
         if (SHORT1FROMMP(mp2) == MCI_GAINING_USE)           /* GAINING USE */
           {
             fPassedDevice = FALSE;             /* Gaining control of device.*/
           }
         else                                                /* LOSING USE  */
           {
             fPassedDevice = TRUE;              /* Losing  control of device.*/
           }
         return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2 ) );

      case WM_ACTIVATE:

         /*********************************************************************/
         /* We use the WM_ACTIVATE message to participate in device sharing.  */
         /* We first check to see if this is an activate or a deactivate      */
         /* message (indicated by mp1).  Then, we check to see if we've passed*/
         /* control of the Movie device.  If these conditions are true,       */
         /* then we issue an acquire device command to regain control of      */
         /* the device, since we're now the active window on the screen.      */
         /*********************************************************************/
         if ((BOOL)mp1 && fPassedDevice == TRUE)
         {
           /***********************************************************/
           /* To acquire the device, we will issue MCI_ACQUIREDEVICE  */
           /* command to the MCI.                                     */
           /***********************************************************/

            SendString( hwnd, "acquire movie notify", 0 );
         }
         return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2 ) );

      case MM_MCINOTIFY:

       /***********************************************************************/
       /* This message is returned to an application when a device            */
       /* successfully completes a command that was issued with a NOTIFY      */
       /* flag, or when an error occurs with the command.                     */
       /*                                                                     */
       /* This message returns two values. A user parameter (mp1) and         */
       /* the command message (mp2) that was issued. The low word of mp1      */
       /* is the Notification Message Code which indicates the status of the  */
       /* command like success or failure. The high word of mp2 is the        */
       /* Command Message which indicates the source of the command.          */
       /***********************************************************************/

         usNotifyCode    = (USHORT) SHORT1FROMMP( mp1);  /* low-word  */
         usCommandMessage = (USHORT) SHORT2FROMMP( mp2); /* high-word */

         switch (usCommandMessage)
         {
            case MCI_PLAY:
               switch (usNotifyCode)
               {
                  case MCI_NOTIFY_SUPERSEDED:
                  case MCI_NOTIFY_ABORTED:
                     /* we don't need to handle these messages. */
                     break;

                  /********************************************************/
                  /* This case is used for either successful completion   */
                  /* of a command or for and error.                       */
                  /********************************************************/

                  default:
                   if (eState != ST_STOPPED)
                     {
                        eState = ST_STOPPED;
                        if ( usNotifyCode != MCI_NOTIFY_SUCCESSFUL)

                           /*************************************************/
                           /* Notification error message. We need to display*/
                           /* the error message to the user.                */
                           /*************************************************/

                           ShowMCIErrorMessage( usNotifyCode);
                        CloseTheDevice();
                     }

                    break;
               }
               break;
         }
         return( (MRESULT) 0);

      default:
         return( WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2));

   }  /* End of msg Switch */

} /* End of MainDlgProc */

/******************************************************************************
 * Name         :  PlayTheAppFile
 *
 * Description  :  This procedure will Load a specific movie file into the
 *                 Application defined video window.
 *
 * Concepts     :
 *                 - Loading a file into an already open device.
 *
 * MMPM/2 API's :  mciSendString
 *                    - load
 *                    - put
 *                    - window handle
 *
 * Parameters   :  hwnd  - Handle for the Main window.
 *
 * Return       :  TRUE  -  if the operation was initiated without error.
 *                 FALSE -  if an error occurred.
 *
 ******************************************************************************/
BOOL PlayTheAppFile( HWND hwnd )
{
   LONG    lmciSendStringRC;          /* return code from SendString         */

   CHAR    szHandle[10] = "";         /* string used for window handle       */
   CHAR    szx[5]       = "";         /* string used for x position of window*/
   CHAR    szy[5]       = "";         /* string used for y position of window*/
   CHAR    szcx[5]      = "";        /* string used for cx position of window*/
   CHAR    szcy[5]      = "";        /* string used for cy position of window*/

   /*******************************************************************/
   /* The szWindowString and szPutString are used as a foundation     */
   /* for building a string command to send to sendstring             */
   /*******************************************************************/

   CHAR    szWindowString[CCHMAXPATH] =
             "window movie handle ";      /* string command to mciSendString  */

   CHAR    szPutString[CCHMAXPATH] =
            "put movie destination at ";  /* string command to mciSendString */

   /**********************************************************************/
   /* Change pointer to a waiting pointer first, since this might take a */
   /* couple of seconds.                                                 */
   /**********************************************************************/

   WinSetPointer( HWND_DESKTOP, hptrWait );

   /******************************************************/
   /* Convert the Frame window handle to a string so    */
   /* we can use the mciSendStringCommand.               */
   /******************************************************/

   hwndAppFrame    = WinWindowFromID( hwnd, ID_APPWINDOW );

   _ultoa(hwndAppFrame,szHandle,10);

   strcat (szWindowString, szHandle);  /* concatenate the converted handle to*/
   strcat (szWindowString, " ");     /* the window string so we can issue the*/
   strcat (szWindowString, "wait");  /* send string command                  */

   lmciSendStringRC = SendString(hwnd, szWindowString, 0);

   /* Load the movie */

   if (!(lmciSendStringRC = SendString(hwnd, "load movie movie.avi wait", 0)))
     {
       ShowMCIErrorMessage(lmciSendStringRC);
       return( FALSE );
     }


   /******************************************************/
   /* Convert the Frame windows sizes to strings so      */
   /* we can use the mciSendStringCommand to put the     */
   /* video in our application Frame window.             */
   /******************************************************/

     WinQueryWindowPos (hwndAppFrame, &swpAppFrame);

       swpAppFrame.x = 0;
       swpAppFrame.y = 0;

     /**********************************************************/
     /* convert the application windows coordinates to strings */
     /* so we can issue the the sendstring command             */
     /**********************************************************/
     _ultoa(swpAppFrame.x,szx,10);
     _ultoa(swpAppFrame.y,szy,10);
     _ultoa(swpAppFrame.cx,szcx,10);
     _ultoa(swpAppFrame.cy,szcy,10);

     /**********************************************************/
     /* concatenate the converted strings to the the put string*/
     /* so we can issue the the sendstring command             */
     /*                                                        */
     /* The following restrictions on PUT DESTINATION exist in */
     /* OS/2 2.1: The x and y values of the destination must be*/
     /* zero, although the cx and cy values may be any value   */
     /* up to the width and height of the window. Also,        */
     /* PUT DESTINATION will have no effect if executed before */
     /* playing starts.                                        */
     /**********************************************************/

     strcat (szPutString, szx);
     strcat (szPutString, " ");
     strcat (szPutString, szy);
     strcat (szPutString, " ");
     strcat (szPutString, szcx);
     strcat (szPutString, " ");
     strcat (szPutString, szcy);
     strcat (szPutString, " ");
     strcat (szPutString, "wait");

     lmciSendStringRC = SendString( hwnd, szPutString, 0 );

   /**********************************************/
   /* Check to see if the digital video can play */
   /* If it cannot play disable the play button  */
   /* Display a message error loading video      */
   /**********************************************/

   fDevicePlaying = TRUE;

   if (!SendString( hwnd,
                   "capability movie can play wait",
                    0 ))
     {

      WinEnableWindow( hwndPlayPB, FALSE );

      return( FALSE );
     }

   /******************************************************************/
   /* Now that we're done here, change the pointer back to the arrow.*/
   /******************************************************************/
   WinSetPointer ( HWND_DESKTOP, hptrArrow );

   return( TRUE );

}  /* end of PlayTheAppFile */

/******************************************************************************
 * Name         :  PlayTheDefaultFile
 *
 * Description  :  This procedure will load a specific movie file into
 *                 the applications default video window.  Default video
 *                 window will appear in the lower left origin of the screen.
 *
 *
 * Concepts     :
 *                 - Loading a file into an already open device.
 *
 * MMPM/2 API's :  mciSendString
 *                    - load
 *
 * Parameters   :  hwnd  - Handle for the Main dialog box.
 *
 * Return       :  TRUE  -  if the operation was initiated without error.
 *                 FALSE -  if an error occurred.
 *
 ******************************************************************************/
BOOL PlayTheDefaultFile( HWND hwnd )
{
   LONG    lmciSendStringRC;            /* return code from SendString       */


   /**********************************************************************/
   /* Change pointer to a waiting pointer first, since this might take a */
   /* couple of seconds.                                                 */
   /**********************************************************************/

   WinSetPointer( HWND_DESKTOP, hptrWait );

   if (!(lmciSendStringRC = SendString(hwnd, "load movie movie.avi wait", 0)))
     {
       ShowMCIErrorMessage(lmciSendStringRC);
       return( FALSE );
     }

   fDevicePlaying = TRUE;

   /**********************************************/
   /* Check to see if the digital video can play */
   /* If it cannot play disable the play button  */
   /* Display a message error loading video      */
   /**********************************************/

   if (!SendString( hwnd,
                   "capability movie can play wait",
                    0 ))
     {

      WinEnableWindow( hwndPlayPB, FALSE );

      return( FALSE );
     }

   /******************************************************************/
   /* Now that we're done here, change the pointer back to the arrow.*/
   /******************************************************************/
   WinSetPointer ( HWND_DESKTOP, hptrArrow );

   return( TRUE );

}  /* end of PlayTheDefaultFile */

/*************************************************************************
 * Name         :  PlayTheMovie
 *
 * Description  :  This procedure will begin the playing of an Movie file.
 *                 It is invoked when the user selects the Play pushbutton
 *                 on the application's main window.  The movie will be
 *                 played in either a application defined window or in a
 *                 default window depending on the state of the radio buttons.
 *
 *
 * Concepts     :  Playing a Movie file using the MCI interface.
 *
 * MMPM/2 API's :  mciSendString
 *                    - play
 *
 * Parameters   :  hwnd - Handle for the Main dialog box.
 *
 * Return       :  TRUE  -  if the operation was initiated without error.
 *                 FALSE -  if an error occurred.
 *
 ******************************************************************************/
BOOL PlayTheMovie( HWND hwnd)
{

   /*******************************************/
   /* Check to see which window we want to    */
   /* play the movie in.                      */
   /*******************************************/

  if (fDevicePlaying == FALSE)
    {
       if (WinSendMsg(hwndApplication, BM_QUERYCHECK, NULL,NULL ))
         {

           if (!PlayTheAppFile(hwnd))
             {
               return (FALSE);
             }

         } /* if */
       else
         {
           if (!PlayTheDefaultFile(hwnd))
             {
               return( FALSE );
             } /* if */
         }
    } /* if */

   eState = ST_PLAYING;                      /* set state to PLAYING          */

   SetMovieVolume();                        /* set the starting volume       */

   /**************************************************************************/
   /* To play the Movie, we will issue an MCI_PLAY command via mciSendString.*/
   /* A MM_MCINOTIFY message will be sent to the window specified in         */
   /* SendString when the operation is completed.                            */
   /**************************************************************************/
   SendString( hwnd,"play movie notify", 0 );

   return( TRUE );

}  /* end of PlayTheMovie */

/*************************************************************************
 * Name         :  ResumeTheMovie
 *
 * Description  :  This procedure will resume the playing of the Movie
 *                 has been paused.
 *
 * Concepts     :  Resuming the Movie using MCI interface.
 *
 * MMPM/2 API's :  mciSendString
 *                    - resume
 *
 * Parameters   :  none.
 *
 * Return       :  none.
 *
 *************************************************************************/
VOID ResumeTheMovie( VOID )
{

   /*******************************************************************/
   /* To resume the Movie, we will issue an MCI_RESUME command via    */
   /* MciSendString.                                                  */
   /*******************************************************************/
   if (SendString( (HWND)NULL, "resume movie wait", 0 ) )
     {
       eState = ST_PLAYING;
       SetMovieVolume();
     }

   return;
}  /* End of ResumeTheMovie */

/******************************************************************************
 * Name         :  PauseTheMovie
 *
 * Description  :  This procedure will pause Movie that is playing.
 *
 * Concepts     :  Pausing the Movie using MCI interface.
 *
 * MMPM/2 API's :  mciSendString
 *                    - pause
 *
 * Parameters   :  none.
 *
 * Return       :  none.
 *
 ******************************************************************************/
VOID PauseTheMovie( VOID )
{

   /*******************************************************************/
   /* To pause the Movie, we will issue an MCI_PAUSE command via      */
   /* MciSendString.                                                  */
   /*******************************************************************/

   if ( SendString( (HWND)NULL, "pause movie wait", 0 ) )
    {
       eState = ST_PAUSED;
    }

   return;
}  /* end of PauseTheMovie */

/******************************************************************************
 * Name         :  StopTheDevice
 *
 * Description  :  This procedure will stop the device that is playing or
 *                 recording.
 *
 * Concepts     :  Stopping a device using the MCI interface.
 *
 * MMPM/2 API's :  mciSendString
 *                    - stop
 *
 * Parameters   :  none
 *
 * Return       :  nothing.
 *
 ******************************************************************************/
VOID StopTheDevice( VOID )
{

   /*
    * To stop the device , we will issue an string command using mciSendString.
    * This stop command is done for the alias.
    */
   if (SendString( (HWND)NULL, "stop movie wait", 0 ) )
   {
      if (eState == ST_PLAYING)
        {
          eState = ST_STOPPED;
        }
   }

   return;
}  /* end of StopTheDevice */


/*************************************************************************
 * Name         :  CloseTheDevice
 *
 * Description  :  This procedure will close the Movie device.
 *
 * Concepts     :  Closing a device using MCI interface.
 *
 * MMPM/2 API's :  mciSendString
 *                    - close
 *
 * Parameters   :  None.
 *
 * Return       :  nothing.
 *
 ******************************************************************************/
VOID CloseTheDevice( VOID)
{

   /*
    * To stop the device , we will issue a string command using mciSendString.
    * This stop command is done for the alias.
    */
   fDeviceOpen = FALSE;
   fDevicePlaying = FALSE;
   SendString((HWND)NULL, "close movie", 0 );

   return;

}  /* end of CloseTheDevice */

/*************************************************************************
 * Name         :  OpenTheDevice
 *
 * Description  :  This procedure will open the Movie device.
 *
 * Concepts     :  Opening a device using MCI interface.
 *
 * MMPM/2 API's :  mciSendString
 *                    - open
 *
 * Parameters   :  None.
 *
 * Return       :  nothing.
 *
 ******************************************************************************/
BOOL OpenTheDevice( HWND hwnd)
{
  if (!fDeviceOpen )

  {
     /******************************************************************/
     /* To open the device, we will issue MCI_OPEN command to the MCI  */
     /* for digital video.                                             */
     /******************************************************************/
     if ( SendString( hwnd,
                      "open digitalvideo alias movie wait shareable",
                      0 ) )

     {
        /* Open success, set the flag and return true */

        fDeviceOpen = TRUE;

        return(TRUE);

     }
     else
        return( FALSE );
  }

}  /* end of OpenTheDevice */

/*************************************************************************
 * Name         :  SendString
 *
 * Description  :  This procedure will send string to MCI.
 *
 * Concepts     :
 *
 * MMPM/2 API's :  mciSendString
 *
 * Parameters   :  hwnd        - window handle.
 *                 pcMCIString - string command.
 *                 usUserParm  - user parameter.
 *
 * Return       :  TRUE  - if the operation was initiated without error.
 *                 FALSE - if an error occurred.
 *
 ******************************************************************************/
BOOL  SendString( HWND hwnd, PCHAR pcMCIString, USHORT usUserParm )
{
   LONG           lmciSendStringRC;    /* return value fromm mciSendString     */


   lmciSendStringRC =
       mciSendString( (PSZ)pcMCIString,
                      (PSZ)szReturn,
                      (USHORT)CCHMAXPATH,
                      (HWND)hwnd,
                      (USHORT)usUserParm );

   if (lmciSendStringRC != 0)
   {
      ShowMCIErrorMessage(lmciSendStringRC);
      return( FALSE );
   }

   return( TRUE );
}


/******************************************************************************
 * Name         :  MessageBox
 *
 * Description  :  This procedure will display messages for the application
 *                 based upon string IDs passed in.  The actual text will be
 *                 loaded from the string table in the resource.
 *
 * Concepts     :  None.
 *
 * MMPM/2 API's :  None.
 *
 * Parameters   :  usMessageID - ID of the message string
 *                 ulStyle     - Style of the message box (WinMessageBox)
 *
 * Return       :  TRUE  -  if the operation was initiated without error.
 *                 FALSE -  if an error occurred.
 *
 ******************************************************************************/
USHORT MessageBox( USHORT usMessageID, ULONG  ulStyle)
{
   CHAR     achMessage[LEN_ERROR_MESSAGE];
   USHORT   usResult;

   /*
    * Get the string from the Resource defined string table and show it
    * in the message box.
    */
   WinLoadString(
      hab,                             /* HAB for this dialog box.            */
      (HMODULE) NULL,                  /* Get the string from the .exe file.  */
      usMessageID,                     /* Which string to get.                */
      (SHORT) sizeof( achMessage),     /* The size of the buffer.             */
      achMessage );                    /* The buffer to place the string.     */

   usResult =
      WinMessageBox(
         HWND_DESKTOP,                 /* Parent handle of the message box.   */
         hwndMainDialogBox,            /* Owner handle of the message box.    */
         achMessage,                   /* String to show in the message box.  */
         achMsgBoxTitle,               /* Title to shown in the message box.  */
         (USHORT) ID_MESSAGEBOX,       /* Message Box Id.                     */
         ulStyle );                    /* The style of the message box.       */

   return( usResult );

}  /* End of MessageBox */


/******************************************************************************
 * Name         :  ShowMCIErrorMessage
 *
 * Description  :  This window procedure displays an MCI error message
 *                 based upon a ulError return code.  The MCI function
 *                 mciGetErrorString is used to convert the error code into
 *                 a text string and the title is pulled from the resource
 *                 based upon a string id.
 *
 * Concepts     :  Using mciGetErrorString to convert an error code into
 *                 a textual message.
 *
 * MMPM/2 API's :  mciGetErrorString
 *
 * Parameters   :  ulError  -  MCI error code.
 *
 * Return       :  nothing
 *
 ******************************************************************************/
VOID  ShowMCIErrorMessage( ULONG ulError)
{
   CHAR  achBuffer[LEN_ERROR_MESSAGE];

   switch(mciGetErrorString( ulError, (PSZ)achBuffer,   sizeof( achBuffer)))
   {
      case MCIERR_SUCCESS:
         /*
          * This is what we want.  We were able to use mciGetErrorString to
          * retrieve a textual error message we can show in a message box.
          */
         WinMessageBox( HWND_DESKTOP,
                        hwndMainDialogBox,
                        achBuffer,
                        achMsgBoxTitle,
                        (USHORT) ID_MESSAGEBOX,
                        MB_CANCEL | MB_HELP | MB_ERROR | MB_MOVEABLE);
         break;

      case MCIERR_INVALID_DEVICE_ID:
      case MCIERR_OUTOFRANGE:
      case MCIERR_INVALID_BUFFER:
      default:
         MessageBox( IDS_UNKNOWN,
                     MB_CANCEL | MB_HELP | MB_ERROR | MB_MOVEABLE);
         break;
   }

   return;

}  /* end of ShowMCIErrorMessage */


/******************************************************************************
 * Name         :  DoesFileExist
 *
 * Description  :  This helper function determines if a file with a given
 *                 file name exists. If it does, we want to attain specific
 *                 information for future use.  This information is attained
 *                 by getting the track ID of the movie.
 *
 * Concepts     :  Using MMIO interface to access a data file and header
 *                 information.
 *
 * MMPM/2 API's :  mmioOpen
 *                 mmioClose
 *                 mmioGetHeader
 *                 mmioSet
 *
 * Parameters   :  pszFileName - The file name to be tested.
 *
 * Return       :  TRUE  -  if the a file exists matching pszFilename
 *                          and there are no errors getting the movie file
 *                          header information.
 *                 FALSE -  if the file does not exist or problem getting
 *                          the movie file header information
 *
 *
 ******************************************************************************/
BOOL DoesFileExist( PSZ pszFileName )
{
   BOOL  bReturn = 0;                    /* Function return value       */
   HMMIO hFile;                          /* Handle to file              */
   ULONG  lHeaderLengthMovie;            /* Header length of movie file */
   ULONG  lHeaderLengthVideo;            /* Header length of movie file */
   ULONG  ulTrackID;                     /* Track ID                    */
   LONG   lBytes;                        /* Number of bytes read        */
   MMMOVIEHEADER  mmMovieHeader;         /* Std Movie Header            */
   MMVIDEOHEADER  mmVideoHeader;         /* Std Video Header            */
   MMEXTENDINFO   mmExtendinfo;          /* Std Extended Information    */
   MMIOINFO       mmioinfo;              /* Structure for mmio info     */
   PMMIOPROC      pIOProc;
   PMMIOPROC      pAnswer;
   HMODULE        hmod;
   FOURCC         fcc;
   CHAR           LoadError[100];
   ULONG          rc = MMIO_SUCCESS;

   /*********************************************************/
   /* Reset all my my mmio information structures           */
   /*********************************************************/

   memset(&mmioinfo, '\0', sizeof(MMIOINFO));
   memset(&mmExtendinfo, '\0', sizeof(MMEXTENDINFO));
   memset(&mmMovieHeader, '\0', sizeof(MMMOVIEHEADER));
   memset(&mmVideoHeader, '\0', sizeof(MMVIDEOHEADER));

   mmioinfo.ulTranslate =  MMIO_TRANSLATEHEADER;   /* Set to std translation */

                                    /* Set all operations on the active track*/
   mmExtendinfo.ulFlags = MMIO_TRACK;

   hFile = mmioOpen( pszFileName, &mmioinfo, MMIO_READ );

   if (hFile != (HMMIO) NULL)
    {

      mmExtendinfo.ulTrackID = -1;                   /* reset the track id */

     /*********************************************************/
     /* After the file is open set the extend information     */
     /*********************************************************/

      bReturn = mmioSet(hFile, &mmExtendinfo, MMIO_SET_EXTENDEDINFO);

      lHeaderLengthMovie = sizeof(MMMOVIEHEADER);

     /*********************************************************/
     /* Read the movie header informationn and get the track  */
     /* information.                                          */
     /*********************************************************/
      if (!(bReturn = mmioGetHeader(hFile,
                                    &mmMovieHeader,
                                    lHeaderLengthMovie,
                                    &lBytes,
                                    0L,
                                    0L)));
       {

          /**************************************************************/
          /* Here we will use the mmioSet to set the extended info      */
          /* to the correct track after issuing the mmioGetHeader. Once */
          /* we have the correct track we can issue mmioGetHeader       */
          /* requesting the movie information we wish to have. In this  */
          /* sample we are asking for the height, width, length of the  */
          /* movie.                                                     */
          /**************************************************************/

          mmExtendinfo.ulTrackID = mmMovieHeader.ulNextTrackID;

          bReturn = mmioSet(hFile, &mmExtendinfo, MMIO_SET_EXTENDEDINFO);

          lHeaderLengthVideo = sizeof(MMVIDEOHEADER);

          /*******************************************************/
          /* Get the information we need from the video header   */
          /*******************************************************/

          bReturn = mmioGetHeader(hFile,
                                  &mmVideoHeader,
                                  lHeaderLengthVideo,
                                  &lBytes,
                                  0L,
                                  0L);

          ulMovieWidth  = mmVideoHeader.ulWidth;

          ulMovieHeight = mmVideoHeader.ulHeight;

          ulMovieLength = mmVideoHeader.ulLength;

          /*****************************************************/
          /* Here we are using the mmioSet to reset the tracks */
          /* We do this just in case we would need to do some- */
          /* thing different with the movie file.              */
          /*****************************************************/
          mmExtendinfo.ulTrackID = MMIO_RESETTRACKS;
          bReturn = mmioSet(hFile, &mmExtendinfo,MMIO_SET_EXTENDEDINFO);

          mmioClose( hFile, 0);

          bReturn = TRUE;
          return (bReturn);
       }
    }
   bReturn = FALSE;
   return (bReturn);


}
/******************************************************************************
 * Name         :  SetMovieVolume
 *
 * Description  :  This helper function sets the Movie file volume based upon
 *                 the position of the volume slider.  The slider will be
 *                 queried and the Movie file volume will be set.
 *
 * Concepts     :  Setting the volume of a device.
 *
 * MMPM/2 API's :  mciSendString
 *                    - set
 *
 * Parameters   :  none.
 *
 * Return       :  none.
 *
 ******************************************************************************/
VOID SetMovieVolume( VOID )
{
   LONG      lmciSendStringRC;             /* return value form mciSendString */
   CHAR      szVolume[4] = "";             /* to hold the volume level        */
   CHAR      szCommandString[CCHMAXPATH] = /* string command to MCI           */
               "set movie audio volume ";


   if ((!fPassedDevice) && (eState == ST_PLAYING ))
   {
      /*
       * To set the volume,  first, build the string command for the
       * MCI.  Then an MCI_SET command should be issued to the device
       * to perform the volume change.
       */
      _itoa(sVolumeLevel, szVolume, 10);
      strcat( szCommandString, szVolume);
      strcat( szCommandString, " ");
      strcat( szCommandString, "wait");

      lmciSendStringRC =
          mciSendString( (PSZ)szCommandString,
                         (PSZ)szReturn,
                         CCHMAXPATH,
                         (HWND)NULL,
                         (USHORT)NULL );
      if (lmciSendStringRC != 0)
         ShowMCIErrorMessage(lmciSendStringRC);

   }
   return;
}  /* end of SetMovieVolume */

/*************************************************************************
 * Name         : InitializeHelp
 *
 * Description  : This procedure will set up the initial values in the
 *                global help structure.  This help structure will then
 *                be passed on to the Help Manager when the Help Instance
 *                is created.  The handle to the Help Instance will be
 *                kept for later use.
 *
 * Concepts     : None.
 *
 * MMPM/2 API's : None.
 *
 * Parameters   : None.
 *
 * Return       : None.
 *
 *************************************************************************/
VOID InitializeHelp( VOID )
{

   HELPINIT helpInit;                 /* Help initialization structure.          */
   CHAR     achHelpLibraryName[LEN_HELP_LIBRARY_NAME];
   CHAR     achHelpWindowTitle[LEN_HELP_WINDOW_TITLE];

   /*
    * Load the strings for the Help window title and library name.
    * Initialize the help structure and associate the help instance.
    */
   WinLoadString( hab,
                  (HMODULE) NULL,
                  IDS_HELP_WINDOW_TITLE,
                  (SHORT) sizeof( achHelpWindowTitle),
                  achHelpWindowTitle);

   WinLoadString( hab,
                  (HMODULE) NULL,
                  IDS_HELP_LIBRARY_NAME,
                  (SHORT) sizeof( achHelpLibraryName),
                  achHelpLibraryName);

   memset ( &helpInit, 0, sizeof(helpInit) );
   /*
    * Initialize the help structure.
    */
   helpInit.cb                 = sizeof( helpInit);  /* size of the help struc*/
   helpInit.ulReturnCode       = (ULONG) 0;          /* RC from HM init       */
   helpInit.pszTutorialName    = (PSZ) NULL;         /* No tutorial program   */
   helpInit.pszHelpWindowTitle = achHelpWindowTitle; /* The Help window title.*/
   helpInit.fShowPanelId       = (ULONG) 0;          /* help panel ID.        */
   helpInit.pszHelpLibraryName = achHelpLibraryName; /* library name          */
   helpInit.phtHelpTable       = (PVOID)(0xffff0000 | ID_MOVIE_HELPTABLE);

   /*
    * Create the Help Instance for IPF.
    * Give IPF the Anchor Block handle and address of the IPF initialization
    * structure, and check that creation of Help was a success.
    */
   hwndHelpInstance = WinCreateHelpInstance(
                         hab,                   /* Anchor Block Handle.       */
                         &helpInit );           /* Help Structure.            */

   if ( hwndHelpInstance == (HWND) NULL)
   {
      MessageBox( IDS_HELP_CREATION_FAILED,     /* ID of the message          */
                  MB_OK | MB_INFORMATION  | MB_MOVEABLE);    /* style         */
   }
   else
   {
      if ( helpInit.ulReturnCode)
      {
         WinDestroyHelpInstance( hwndHelpInstance);
         MessageBox( IDS_HELP_CREATION_FAILED,     /* ID of the message       */
                     MB_OK | MB_INFORMATION | MB_MOVEABLE);     /* style      */
      }
      else  /* help creation worked */
         WinAssociateHelpInstance(
            hwndHelpInstance,        /* The handle of the Help Instance.      */
            hwndFrame);              /* Associate to this dialog window.      */
   }  /* End of IF checking the creation of the Help Instance. */

}  /* End of InitializeHelp */

/******************************************************************************
 * Name         : ResizeMovieWindow
 *
 * Description  : This function will resize the applications movie window
 *                according to the height and width of the movie file being
 *                opened.
 *
 * Parameters   : hwnd - Handle of the secondary window.
 *
 * Return       : none.
 *
 ******************************************************************************/
VOID ResizeMovieWindow (HWND hwnd)
{

   HWND hwndSEFrame;                 /* handle for the Secondary Window Frame */
   SWP  swpSEFrame;              /* struc containing information on Frame     */
   SWP  swpRewind;               /* struc containing information rewind button*/
   LONG cxWidthBorder = 0;
   LONG cyWidthBorder = 0;
   LONG cyTitleBar = 0;

  /*************************************************************/
  /* Query the position of the secondary window frame          */
  /* and the rewind button so we can postion our application   */
  /* frame for the movie.                                      */
  /*************************************************************/

   WinQueryWindowPos( hwndFrame, &swpSEFrame );

   WinQueryWindowPos( WinWindowFromID( hwnd, IDC_GPB_REWIND ),
                      &swpRewind );

   cxWidthBorder = (LONG) WinQuerySysValue(HWND_DESKTOP, SV_CXSIZEBORDER);
   cyWidthBorder = (LONG) WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER);
   cyTitleBar =    (LONG) WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);

  /*************************************************************/
  /* Check to see if our movie size is larger than our frame   */
  /* if it is then resize our frame to fit the movie.          */
  /*************************************************************/

   if ((swpSEFrame.cx - (2 * cxWidthBorder)) <= ulMovieWidth)
    {
      swpSEFrame.cx = ulMovieWidth + (2 * cxWidthBorder);
    } /* if */
   if ((swpSEFrame.cy - swpRewind.cy - ulMovieHeight - cyTitleBar - (2 * cyWidthBorder)
                                                         < ulMovieHeight ))
    {
      swpSEFrame.cy = ulMovieHeight + cyTitleBar + (2 * cyWidthBorder);
    } /* if */

  /*************************************************************/
  /* Set the position and size of the application window       */
  /* to the position and size of the movie.                    */
  /*************************************************************/

   WinSetWindowPos( hwndAppFrame, HWND_TOP,
                    (swpSEFrame.cx - ulMovieWidth - cxWidthBorder) / 2,
                    (cyWidthBorder + swpRewind.y + swpRewind.cy) + 5,
                    ulMovieWidth,
                    ulMovieHeight,
                    SWP_SIZE | SWP_MOVE);

}
