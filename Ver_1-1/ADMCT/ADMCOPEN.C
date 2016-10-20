/*********************** END OF SPECIFICATIONS *******************************
*
* SOURCE FILE NAME:  MCIOPEN.C
*
*
*
*              Copyright (c) IBM Corporation  1991, 1993
*                        All Rights Reserved
*
* DESCRIPTIVE NAME:  WAVEFORM MCD (PLAYER)
*
* FUNCTION: Opens the waveaudio device.  Supports device only, elements,
*           mmio file handles and play lists.
*
* On an MCI_OPEN, a streaming MCD should perform the following actions:
*
*
* A. Check flags and validate pointers.
* B. Get default values from the ini files (if necessary).
* C. Process MCI_OPEN_PLAYLIST (if supported).
* D. Process MCI_OPEN_MMIO (if supported).
* E. Process MCI_OPEN_ELEMENT (if supported).
* F. Connect to the amp/mixer.
* G. Get the stream protocol key (if necessary).
*
*********************** END OF SPECIFICATIONS ********************************/
#define INCL_BASE
#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES
#define INCL_WINCLIPBOARD
#define INCL_WINWINDOWMGR


#include <os2.h>                        // OS2 defines.
#include <string.h>
#include <os2medef.h>                   // MME includes files.
#include <stdlib.h>                     // Math functions
#include <audio.h>                      // Audio Device defines
#include <ssm.h>                        // SSM Spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmioos2.h>                    // MMIO Include.
#include <mcios2.h>                     // MM System Include.
#include <mcipriv.h>                    // MCI Connection stuff
#include <mmdrvos2.h>                   // MCI Driver include.
#include <mcd.h>                        // AudioIFDriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions
#include <qos.h>
#include <audiomcd.h>                   // Component Definitions.
#include "admcfunc.h"                   // Function Prototypes.
#include <sw.h>



/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: MCIOPEN.C
*
* DESCRIPTIVE NAME: Open Waveform Player.
*
* FUNCTION:
*
* NOTES: This function opens the device that we are connected to.
*        It is called from the main switch statement (MCI_OPEN) and
*        this function will do the following actions.
*        A. Parse flags.
*        B. Determine stream handlers to be used in streaming.
*        C. Connect to a device (usually the amp/mixer).
*        D. Open the file.
*        E. Set up various variables.
*
* NO streams are created on the open.  This will be done only on
* the streaming operations
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERNCES:   OpenFile (),
*                       SetWaveDeviceDefaults (),
*                       SetAudioDevice (),
*                       InitAudioDevice,
*                       SetAmpDefaults (),
*                       CheckMem ().
*
*
* EXTERNAL REFERENCES:  SpiGetHandler      ()   -       SSM Spi
*                       AudioIFDriverEntry ()   -       Audio IF Interface
*                       DosLoadModule      ()   -       OS/2 API
*                       DosQueryProcAddr   ()   -       OS/2 API
*                       mdmDriverNotify    ()   -       MDM  API
*                       mciSendCommand     ()   -       MDM  API
*                       mciConnection      ()   -       MDM  API
*                       mciQueryDefaultConnections () - MDM  API
*
*********************** END OF SPECIFICATIONS ********************************/

RC MCIOpen (FUNCTION_PARM_BLOCK *pFuncBlock)
{
  ULONG                ulrc;                     // Propogated Error Code
  INSTANCE             *ulpInstance;             // Full Blown Instance
  ULONG                ulParam1;                 // Incoming Flags From MCI
  ULONG                ulOpenFlags;              // Mask For Incoming Flags
  MMDRV_OPEN_PARMS     *pDrvOpenParams;          // MCI Driver Open Parameter Block

  extern HID           hidA2Source;              // hid's for memory playlist
  extern HID           hidA2Target;
  extern HID           hidBTarget;

  ulParam1 =  pFuncBlock->ulParam1;
  ulpInstance = pFuncBlock->pInstance;
  pDrvOpenParams = (MMDRV_OPEN_PARMS *) pFuncBlock->ulParam2;


  ulOpenFlags = ulParam1;

  /**********************************
  * Check For Validity of Flags
  **********************************/

  ulOpenFlags &= ~(  MCI_WAIT          + MCI_NOTIFY       +
                    MCI_OPEN_SHAREABLE + MCI_OPEN_TYPE_ID + MCI_OPEN_ELEMENT +
                    MCI_OPEN_PLAYLIST  + MCI_OPEN_ALIAS   + MCI_OPEN_MMIO    +
                    MCI_READONLY );


  if (ulOpenFlags >  0)
      return MCIERR_INVALID_FLAG;



  /*************************************
  * Check Incoming MCI Flags
  ************************************/
  if ( ( ulParam1 & MCI_OPEN_PLAYLIST ) &&
       ( ( ulParam1 & MCI_OPEN_ELEMENT )  ||
         ( ulParam1 & MCI_OPEN_MMIO ) 
       ) )
     return ( MCIERR_FLAGS_NOT_COMPATIBLE );

  if ( ( ulParam1 & MCI_OPEN_ELEMENT ) &&
       ( ulParam1 & MCI_OPEN_MMIO ) )
     return ( MCIERR_FLAGS_NOT_COMPATIBLE );

  /***************************************
  * Ensure that if read only flag is passed
  * in that open element is too
  ****************************************/
  if ( ( ulParam1 & MCI_READONLY ) &&
       !( ulParam1 & MCI_OPEN_ELEMENT || ulParam1 & MCI_OPEN_MMIO ) )
     {
     return ( MCIERR_MISSING_FLAG );
     }


  /***************************************
  * Fill in information to return to the
  * Media Device Manager (MDM).  It will
  * store our instance for us for future
  * messages such as save/restore/play etc.
  ***************************************/


  pDrvOpenParams->pInstance = (PVOID) pFuncBlock->pInstance;

  /*----------------------------------------------
  * Resource units describe how many processes
  * threads can open different instances of
  * the streaming or non-streaming MCI driver.
  * Since our streaming MCD consumes no resources
  * (i.e. an infinite number of instances can be
  * opened) set the resources used to 0.  Note:
  * this is not true of the amp/mixer since it is
  * constrained by the capabilities of the audio
  * hardware it is attached to.
  *---------------------------------------------*/
  pDrvOpenParams->usResourceUnitsRequired = 0;

  pFuncBlock->ulpInstance = (PVOID) pFuncBlock->pInstance;

  if (ulParam1 & MCI_NOTIFY)
     {
     pFuncBlock->ulNotify = TRUE;
     pFuncBlock->hwndCallBack = pDrvOpenParams->hwndCallback;
     }
  else
     {
     pFuncBlock->ulNotify = FALSE;
     }


  /* Get the default values that the user placed in the MMPM2.INI file */

  ulrc = GetDefaults( pDrvOpenParams->pDevParm, ulpInstance );

  if ( ulrc )
     {
     return ( ulrc );
     }

  /*----------------------------------
  * Initialize appropriate variables
  * which will be used for streaming
  * and other needs.
  *----------------------------------*/

  OpenInit( ulpInstance );


  /*-----------------------------------
  * Load the VSD which the ini file
  * says we are connected to.  The name
  * of this VSD is in the MMDRV_OPEN_PARMS
  * structure.
  *
  * VSD's are used to implement device
  * specific functions (such as setting
  * attributes on an audio card).
  *--------------------------------------*/

  ulrc = DetermineConnections( ulpInstance, pDrvOpenParams );

  if ( ulrc )
     {
     return ( ulrc );
     }

  /*********************************************
  * Create semaphores necessary for our operation
  * This MCD uses semaphores to control access
  * to memory operations, and synchronization.
  *
  * Note: One really neat thing about semaphores
  * in OS/2 is that the process that creates
  * them automatically has access to them without
  * having to open them.
  *********************************************/

  ulrc = CreateSemaphores( pFuncBlock );

  if ( ulrc )
    {
    return ( ulrc );
    }

  /**************************************
  * This MCD currently always attaches
  * itself to an amp/mixer.  We want
  * to open the ampmixer with the same
  * flags that this MCD was opened with
  * (i.e. if someone opened the audio MCD
  * w/o the shareable flag) we should open
  * the amp with the same flags so that
  * the device cannot be taken away from
  * the calling application.
  ***************************************/

  if (ulParam1 & MCI_OPEN_SHAREABLE)
      ulOpenFlags = MCI_OPEN_SHAREABLE;

  /* Check to see if the playlist flag was specified */

  if (ulParam1 & MCI_OPEN_PLAYLIST)
     {
     /*--------------------------------------------
     * Streaming MCD's must perform the following
     * operations with the playlist flag:
     *
     * A. Remember the callback handle with which the
     *    playlist was opened.  All playlist cuepoints
     *    and messages will be reported on this window
     *    handle.
     * B. Retrieve a pointer to the playlist instructions
     *    in the pszElementName field.
     * C. Load the appropriate memory stream handler.
     *
     *-----------------------------------------------------*/

      pFuncBlock->hwndCallBack  = pDrvOpenParams->hwndCallback;

      /* store an extra copy so that play list cue points can be remembered */

      pFuncBlock->pInstance->hwndOpenCallBack  = pDrvOpenParams->hwndCallback;

      /* Store the playlist pointer */

      ulpInstance->pPlayList = (PVOID) pDrvOpenParams->pszElementName;
      ulpInstance->usPlayLstStrm = TRUE;
      ulpInstance->fFileExists = TRUE;

     if (ulrc = SpiGetHandler((PSZ)MEM_PLAYLIST_SH,
                              &hidA2Source,
                              &hidA2Target ) )
       {
       return ( ulrc );
       }

      /* The handler ids for the playlist were obtained in the dllinit */

      ulpInstance->StreamInfo.hidASource = hidA2Source;
      ulpInstance->StreamInfo.hidATarget = hidA2Target;


      /*****************************
      * Default to playback
      ******************************/

      AMPMIX.ulOperation = OPERATION_PLAY;

     } /* Play list flag was specified */



  /*****************************************
  * if the caller did not pass in a playlist
  * then maybe they passed in OPEN_ELEMENT
  * or OPEN_MMIO.
  *****************************************/
  else
     {

     /***************************************
     * If the caller has passed in the open_element
     * flag, then check to ensure that we have
     * valid string.
     ***************************************/

     if (ulParam1 & MCI_OPEN_ELEMENT)
        {

        /*-------------------------------------------------------
        * Readonly means we cannot write to the file, therefore,
        * there is no need to create temporary files etc. since we
        * cannot change the original file.
        *--------------------------------------------------------*/
   
        if ( ulParam1 & MCI_READONLY )
   
           {
           ulpInstance->ulOpenTemp = MCI_FALSE;
           ulpInstance->ulCapabilities &= ~( CAN_SAVE | CAN_RECORD );
           }
        else
           {
           ulpInstance->ulOpenTemp = MCI_TRUE;
           } /* else read only flag was NOT specified */


        /* Ensure that the filename is valid */

        ulrc = CheckForValidElement( ulpInstance,
                                     pDrvOpenParams->pszElementName,
                                     ulParam1 );

        if ( ulrc )
           {
           return ( ulrc );
           }


        /* Open the file, and setup necessary flags */

        ulrc = ProcessElement( ulpInstance, ulParam1, MCI_OPEN );
   
        if ( ulrc )
           {
           return ( ulrc );
           }

        } /* Open Element */



     if ( ulParam1 & MCI_READONLY )

        {
        ulpInstance->ulCapabilities &= ~( CAN_SAVE | CAN_RECORD );
        }

     } /* else not a play list open */




  /*******************************************
  * Check to see if the caller passed in
  * open_mmio.  If so, then set up necessary
  * flags etc.  From the caller's perspective
  * it is advantageous to use OPEN_MMIO since
  * it reduces the time it takes to perform
  * an MCI_OPEN (i.e. all the MMIO work is
  * done before the MCI work).
  *******************************************/

  ulrc = OpenHandle( ulpInstance,
                     ulParam1,
                     (HMMIO) pDrvOpenParams->pszElementName);

  if ( ulrc )
     {
     return ( ulrc );
     }



  /*---------------------------------------------
  * If the IO proc we are using
  * has the ability to record with temp files,
  * and this is not a MCI_READONLY open, then
  * setup the temporary files.
  *---------------------------------------------*/

  if ( ulpInstance->ulOpenTemp )
     {
     ulrc = SetupTempFiles( ulpInstance, ulParam1 );

     if ( ulrc )
        {
        return ( ulrc );
        }

    } /* if ulOpenTemp */

  /*-------------------------------------------
  * Connect ourselves to an amp/mixer.  This
  * routine figures out which amp/mixer we are
  * connected to, opens it and connects us to
  * it.  Once we are connected to the amp, we
  * will lose use and gain use of the device
  * at the same time it does.
  *--------------------------------------------*/

  ulrc = ConnectToAmp( ulpInstance, pDrvOpenParams, ulOpenFlags );

  if ( ulrc )
     {
     CloseFile( ulpInstance );
     return ( ulrc );
     }

  /***********************
  * Copy info from amp/mixer to streaming structures
  ***********************/


  STREAM.SpcbKey.ulDataType    = AMPMIX.ulDataType;
  STREAM.SpcbKey.ulDataSubType = AMPMIX.ulSubType;

  STREAM.SpcbKey.ulIntKey = 0;

  /*----------------------------------------------------------
  * Retrieve the mmtime and byte per unit numbers from the
  * SPCB key so that can calculate values for seeks etc.
  * This information will also be used to communicate network
  * usage parameters to network io procs.
  *---------------------------------------------------------*/

  ulrc = SpiGetProtocol( hidBTarget, &STREAM.SpcbKey, &ulpInstance->StreamInfo.spcb );

  if ( ulrc )
     {
     return ( ulrc );
     }

  ulpInstance->ulBytes  = ulpInstance->StreamInfo.spcb.ulBytesPerUnit;
  ulpInstance->ulMMTime = ulpInstance->StreamInfo.spcb.mmtimePerUnit;

  STREAM.AudioDCB.ulSysFileNum = AMPMIX.ulGlobalFile;
  STREAM.AudioDCB.ulDCBLen = sizeof (DCB_AUDIOSH);

  /*------------------------------------------------
  * Connect ourselves to the amp/mixer.  This
  * will enable us to stream and also allows us
  * to receive the MCIDRV_SAVE and MCIDRV_RESTORE
  * when the amp loses and gains use of the
  * device (see audiomcd.c for more information).
  *------------------------------------------------*/

  ulrc = mciConnection ( ulpInstance->usWaveDeviceID,
                         1,
                         ulpInstance->usAmpDeviceID,
                         1,
                         MCI_MAKECONNECTION);
  if (ulrc)
     {
     CloseFile( ulpInstance );
     return ulrc;
     }

  /***************************
  * Flags & Other Stuff
  ***************************/

  ulpInstance->ulTimeUnits = lMMTIME;
  ulpInstance->StreamInfo.ulState = MCI_STOP;
  ulpInstance->ulCreateFlag = CREATE_STATE;

  /*---------------------------------------------
  * Initialize our custom event structure.
  * SSM allows us to pass a pointer to an
  * event control block when we create a
  * stream.  ADMC has added some additional
  * information to the end of this structure
  * (such as window handles and instance pointers)
  * so that the event routine can have access to
  * our instance data.  See admcplay.c, admcrecd.c
  * and audiosub.c for additional info.
  *------------------------------------------------*/

  ulpInstance->StreamInfo.Evcb.evcb.ulSubType = EVENT_EOS|EVENT_ERROR;

  /* Stick in Instance Ptr in Modified EVCB */

  ulpInstance->StreamInfo.Evcb.ulpInstance = (ULONG)ulpInstance;

  return (MCIERR_SUCCESS);

}      /* end of Open */



//MRESULT EXPENTRY ClipboardProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
//
//{
//MCI_PLAY_PARMS  mciPlayParms;
//MCI_OPEN_PARMS  mciop;
//
//HAB             habClipboard;
//PULONG          pulDataSize;
//
//PVOID           pTempBuffer;
//
//HMMIO           hmmioMem;
//
//DWORD           rc;
//MMIOINFO        mmioinfo;
//
//  switch ( msg )
//     {
//     case WM_RENDERFMT :
////               if ( SHORT1FROMMP( mp1 ) == 12 ) // CF_WAVE
//                  {
//
//                  habClipboard = WinQueryAnchorBlock( HWND_DESKTOP );
//
//                  if ( !WinOpenClipbrd( habClipboard ) )
//                     {
//                     // return an error defined in feature, hardware for now.
//                     return ( ( MRESULT  ) 0 );
//
//                     }
//
//                  pTempBuffer = ( PSZ ) WinQueryClipbrdData( habClipboard, 12 /*CF_WAVE*/ );
//
//                  if ( !pTempBuffer )
//                     {
//                     // return an error defined in feature, hardware for now.
//                     WinCloseClipbrd( habClipboard );
//                     return ( ( MRESULT ) 0 );
//                     }
//
//                  /*********************************************************************
//                  * We need to find out how much data is in the file, so retrieve
//                  * the length of the riff chunk
//                  *********************************************************************/
//                  pulDataSize = ( PULONG ) pTempBuffer + 1;
//
//
//
//                  memset( &mmioinfo, '\0', sizeof( MMIOINFO ) );
//
//                  /*********************************************************************
//                  * Prepare to open a memory file--the buffer * in the clipboard contains
//                  * actual Riff-file  which the wave io proc already knows how
//                  * to parse--use it to retrieve the information  and keep the MCD from
//                  * file format dependence.
//                  *********************************************************************/
//
//
//                  mmioinfo.fccIOProc = mmioFOURCC( 'W', 'A', 'V', 'E' ) ;
//                  mmioinfo.fccChildIOProc = FOURCC_MEM;
//                  mmioinfo.cchBuffer = ( *pulDataSize) + 8;
//                  mmioinfo.pchBuffer = pTempBuffer;
//
//
//                  hmmioMem = mmioOpen( NULL,
//                                       &mmioinfo,
//                                       MMIO_READ );
//
//
//                  if ( !hmmioMem )
//                     {
//                     return ( ( MRESULT ) 0 );
//                     }
//
//
//                  mciop.lpstrElementName = ( LPSTR ) hmmioMem;
//                  mciop.dwCallback = (DWORD) NULL;
//                  mciop.lpstrDeviceType = ( LPSTR ) MCI_DEVTYPE_WAVEFORM_AUDIO;
//
//                  rc = mciSendCommand( 0,
//                                  MCI_OPEN,
//                                  MCI_WAIT | MCI_OPEN_MMIO |
//                                  MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE,
//                                  ( DWORD ) &mciop,
//                                  0 );
//
//                  if ( rc != MCIERR_SUCCESS )
//                     {
//                     return ( ( MRESULT ) 0 );
//                     }
//
//                  rc = mciSendCommand( mciop.wDeviceID,
//                                  MCI_PLAY,
//                                  MCI_WAIT,
//                                  ( DWORD ) &mciPlayParms,
//                                  0 );
//
//                  rc = mciSendCommand( mciop.wDeviceID,
//                                  MCI_CLOSE,
//                                  MCI_WAIT,
//                                  ( DWORD ) &mciPlayParms,
//                                  0 );
//
//
//                  }
//               break;
//     default :
//               return (( MRESULT )  WinDefSecondaryWindowProc( hwnd, msg, mp1, mp2 ) );
//
//     }
//}
