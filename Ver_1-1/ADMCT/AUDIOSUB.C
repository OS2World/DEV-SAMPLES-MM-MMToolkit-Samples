/*******************************************************************
*
* SOURCE FILE NAME: AUDIOSUB.C
*
* DESCRIPTIVE NAME: Support functions for waveaudio MCI Driver.
*
*              Copyright (c) IBM Corporation  1991, 1993
*                        All Rights Reserved
*
* NOTES: This source illustrates the following concepts.
*         A. Correct processing of notifications.
*         B. Handling calls the are neither MCI_WAIT or
*            MCI_NOTIFY (PostMDMMessage).
*         C.
*         D. Using mmioGetHeader to obtain audio settings from a file (GetHeader).
*         E. Creating a playlist stream (AssociatePlaylist).
*         F. Installing an IO Proc (InstallIOProc).
*         G. Communicating with IO Procs with different capabilites (OpenFile).
*         H. Processing audio files with various file formats (OpenFile).
*         H. Time format conversion (ConvertToMM + ConvertTimeFormat).
*         I. Creating an SPI stream (CreateNAssociateStream)
*         J. Associating an SPI stream (CreateNAssociateStream)
*         I. Setting an event for use in play to/record to (DoTillEvent).

* FUNCTION: PostMDMMessage
*           InstallIOProc
*           OpenFile
*           ConvertToMM
*           ConvertTimeUnits
*           CreateNAssocStream
*           DoTillEvent
*
*********************** END OF SPECIFICATIONS ***********************/

#define INCL_BASE
#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES

#include <os2.h>                        // OS2 defines.
#include <string.h>                     // String functions.
#include <os2medef.h>                   // MME includes files.
#include <math.h>                       // Standard Math Lib
#include <stdlib.h>                     // Standard Library
#include <audio.h>                      // Audio Device defines
#include <ssm.h>                        // SSM spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmioos2.h>                    // MMIO Include.
#include <mcios2.h>                     // MM System Include.
#include <mmdrvos2.h>                   // MCI Driver include.
#include <mcd.h>                        // AudioIFDriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions
#include <admcdat.h>
#include <qos.h>
#include <audiomcd.h>                   // Component Definitions.
#include <admcfunc.h>

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: DestroyStream()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: call SpiDestroyStream
*
*
* NOTES:
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/

RC DestroyStream ( HSTREAM   *phStream)
{

  ULONG ulrc = MCIERR_SUCCESS;

  /************************************
  * Call SpiDestroyStream
  *************************************/
  if ( *phStream != (ULONG) NULL)
     {
     ulrc = SpiDestroyStream ( *phStream);
     }

  /* Ensure that this stream is removed from our instance */

  *phStream = 0;

  return ulrc;
}



/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: PostMDMMessage()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: This function informs the caller that a given command
*           has completed (either succesfully or with an error).
*           All MCI functions that this MCD handles process their
*           notifications here.
*
*           This function uses mdmDriverNotify to inform the caller
*           of completion.
*
*
* NOTES:
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: mdmDriverNotify
*
*********************** END OF SPECIFICATIONS *******************************/

RC PostMDMMessage ( ULONG                ulErrCode,
                    USHORT               usMessage,
                    FUNCTION_PARM_BLOCK  *pFuncBlock)

{

  USHORT  usNotifyType;
  USHORT  usUserParm;
  HWND    hWnd;



  /***************************************************************
  * Determine the MCI Notification Code for this message
  ****************************************************************/

  switch ( ULONG_LOWD(ulErrCode) )
  {

  case MCI_NOTIFY_SUCCESSFUL:
        usNotifyType = MCI_NOTIFY_SUCCESSFUL;
       break;

  case MCI_NOTIFY_SUPERSEDED:
        usNotifyType = MCI_NOTIFY_SUPERSEDED;
       break;

  case MCI_NOTIFY_ABORTED:
        usNotifyType = MCI_NOTIFY_ABORTED;
       break;

  default:
        usNotifyType = (USHORT)ulErrCode;
       break;
  }


  /*******************************************************************
  * MCI Messages PLAY and RECORD call this routine to notify the
  * application asynchronously. The user parameter from the instance
  * block is used for notifying play/record message status. All
  * other messages use the function block user parameter.
  *******************************************************************/

  if (usMessage != MCI_PLAY && usMessage != MCI_RECORD)
     {
     usUserParm = pFuncBlock->usUserParm;
     hWnd = pFuncBlock->hwndCallBack;
     }
  else
     {
     usUserParm = pFuncBlock->pInstance->usUserParm;
     hWnd = pFuncBlock->pInstance->hwndCallBack;
     }

  /* Inform the application of the event */

  mdmDriverNotify (pFuncBlock->pInstance->usWaveDeviceID,
                   hWnd,
                   MM_MCINOTIFY,
                   usUserParm,
                   MAKEULONG (usMessage, usNotifyType));


  return ( MCIERR_SUCCESS );
}




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME:              SetAudioDevice()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Set AudioDevice Attributes
*
*
* NOTES:   This Request is Routed to the Audio Device Via
*          the AudioIf Interface.
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES: AudioIFDriverEntry().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC SetAudioDevice (INSTANCE             *ulpInstance,
                   PMCI_WAVE_SET_PARMS  pSetParms,
                   ULONG                ulParam1 )


{

  ULONG       ulrc;
  extern     HID                 hidBTarget;


  /*---------------------------------------------- 
  * When we perform this command, it is possible
  * that we could lose use (i.e. the amp mixer may
  * change resource units calling MDM on this
  * thread AFTER the MCIDRV_SAVE came through.
  * Therefore, release
  * the data access semaphore so that MCIDRV_SAVE
  * will not hang.
  *-----------------------------------------------*/

  MCD_ExitCrit( ulpInstance );

  /*********************************
   * Send A Set Across the AudioIF
   * Driver Interface
   ********************************/

   ulrc = ulpInstance->pfnVSD (&AMPMIX,
                               MCI_SET,
                               ulParam1,
                               (LONG)pSetParms,
                               0L);
   /*--------------------------------------------------
   * It is no longer possible to lose use.  Therefore,
   * reacquire the semaphore.  Note: we will always be
   * able to acquire this since it is protected by the
   * save access semaphore which everyone goes through
   * besides save and close_exit.
   *--------------------------------------------------*/
   MCD_EnterCrit( ulpInstance );

   /*---------------------------------------------------------
   * Retrieve the block sizes from the stream protocol.  We will
   * use these values for time conversions and seeks.
   *-------------------------------------------------------------*/

   if ( !ulrc )
      {
      ulpInstance->StreamInfo.SpcbKey.ulDataType    = AMPMIX.ulDataType;
      ulpInstance->StreamInfo.SpcbKey.ulDataSubType = AMPMIX.ulSubType;
      ulpInstance->StreamInfo.SpcbKey.ulIntKey = 0;

      /*------------------------------------------------------
      * Get certain streaming information from the stream handler
      * we have loaded.
      *--------------------------------------------------------*/
      ulrc = SpiGetProtocol( hidBTarget, 
                             &ulpInstance->StreamInfo.SpcbKey,
                             &ulpInstance->StreamInfo.spcb );

      if ( ulrc )
         {
         return ( ulrc );
         }

      ulpInstance->ulBytes   = ulpInstance->StreamInfo.spcb.ulBytesPerUnit;
      ulpInstance->ulMMTime  = ulpInstance->StreamInfo.spcb.mmtimePerUnit;

      }

   return (ulrc);
}

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:              AbortWait
*
* DESCRIPTIVE NAME: Release a Wait Thread
*
* FUNCTION: Release a wait Thread
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: MCI_INFO message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS **********************/
RC AbortWaitOperation (INSTANCE * ulpInstance)
 {

  ULONG           ulrc;
  ULONG           lCnt;

  /*******************************************************
  * Check to see if the wait pending flag is set.
  * If this flag is set, then there is another thread
  * processing a command with the MCI_WAIT flag.  Since
  * it is done with a wait, there is no "official" means
  * to abort it.  Therefore, we will perform an artificial
  * SpiStop on the stream so the WAIT thread will think
  * that it is finished processing.
  ********************************************************/

  if (ulpInstance->usWaitPending == TRUE)
     {
     if ( ulpInstance->usWaitMsg != MCI_SAVE )
        {
        DosResetEventSem (ulpInstance->hEventSem, &lCnt);
        
        ulrc = SpiStopStream ( ulpInstance->StreamInfo.hStream,
                               SPI_STOP_DISCARD);
        
        /*********************************************
        * Wait for the Stopped event . Notice that
        * more than one thread will be released and
        * free to run as a result of the stop event
        *********************************************/
        
        if (!ulrc)
           DosWaitEventSem (ulpInstance->hEventSem, -1 );
        }

     } /* If there is a wait operation alive */

  return (MCIERR_SUCCESS);

} /* AbortWaitOperation */


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:      GetAudioHeader
*
* DESCRIPTIVE NAME: Get Audio Header From The IO Proc.
*
* FUNCTION: Obtain Wave Header information.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_INFO message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* NOTES: This function will usually be called either when
*        the file is first loaded or after a record (which
*        can change the settings of the file--like the length).
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: mmioGetHeader ()   -  MMIO API
*
*********************** END OF SPECIFICATIONS **********************/
RC   GetAudioHeader (INSTANCE * ulpInstance)
{
  ULONG  ulrc;
  LONG   BytesRead;

  /*******************************************************
  * A streaming MCD should utilize MMIO to perform all
  * file manipulations.  If we use MMIO, then the MCD
  * will be free from file dependencies (i.e. if a RIFF
  * io proc or a VOC io proc is loaded will be irrelevant.
  ********************************************************/

  ulrc = mmioGetHeader ( ulpInstance->hmmio,
                         (PVOID) &ulpInstance->mmAudioHeader ,
                         sizeof( ulpInstance->mmAudioHeader ),
                         (PLONG) &BytesRead,
                         (ULONG) NULL,
                         (ULONG) NULL);

  if ( ulrc == MMIO_SUCCESS )
      {

      /******************************************
      * Copy the data from the call into the instance
      * so that we can set the amp/mixer up with the
      * values that the file specifies.
      ******************************************/

      AMPMIX.sMode            = WAVEHDR.usFormatTag;
      AMPMIX.sChannels        = WAVEHDR.usChannels;
      AMPMIX.lSRate           = WAVEHDR.ulSamplesPerSec;
      AMPMIX.lBitsPerSRate    = WAVEHDR.usBitsPerSample;
      ulpInstance->ulDataSize = XWAVHDR.ulAudioLengthInBytes;
      AMPMIX.ulBlockAlignment = ( ULONG )WAVEHDR.usBlockAlign;
      ulpInstance->ulAverageBytesPerSec = WAVEHDR.usChannels * WAVEHDR.ulSamplesPerSec * ( WAVEHDR.usBitsPerSample / 8 );

      } /* SuccesFul GetHeader */

    else
      {
      ulrc = mmioGetLastError( ulpInstance->hmmio );

      }
      return (ulrc);

}  /* GetAudioHeader */

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: SetAudioHeader
*
* DESCRIPTIVE NAME: Set Audio Header in a file.
*
* FUNCTION: Save device settings (like bits/sample) in a file.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_INFO message.
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: mmioGetHeader ()   -  MMIO API
*
*********************** END OF SPECIFICATIONS **********************/
RC   SetAudioHeader (INSTANCE * ulpInstance)
{
  ULONG  ulrc;
  LONG   lBogus;

  /* Fill in the necessary parameters for the setheader call */

  WAVEHDR.usFormatTag       = AMPMIX.sMode;
  WAVEHDR.usChannels        = AMPMIX.sChannels;
  WAVEHDR.ulSamplesPerSec   = AMPMIX.lSRate;
  XWAVHDR.ulAudioLengthInMS = 0;
  WAVEHDR.usBitsPerSample   = (USHORT) AMPMIX.lBitsPerSRate;
  WAVEHDR.usBlockAlign      = (USHORT) AMPMIX.ulBlockAlignment;
  WAVEHDR.ulAvgBytesPerSec  = ulpInstance->ulAverageBytesPerSec;

  /***********************************************
  * Tell the io proc that is manipulating the file
  * to set the file header with the values that
  * we are sending.
  ***********************************************/

  ulrc = mmioSetHeader( ulpInstance->hmmio,
                        &ulpInstance->mmAudioHeader,
                        sizeof( MMAUDIOHEADER ),
                        &lBogus,
                        0,
                        0 );



  if (ulrc)
     {
     /*------------------------------------------
     * MMIO returns failure (-1), GetLastError
     * should be called for additional info
     *-----------------------------------------*/

     return ( mmioGetLastError( ulpInstance->hmmio ) );
     }


  return (ulrc);
} /* SetAudioHeader


/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: SetWaveDeviceDefaults()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Allocate Memory for MCI Message parameter.
*
*
* NOTES:
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
VOID SetWaveDeviceDefaults (INSTANCE * ulpInstance, ULONG ulOperation)
{

  AMPMIX.sMode            = ulpInstance->lDefaultFormat;   // DATATYPE_WAVEFORM
  AMPMIX.lSRate           = ulpInstance->lDefaultSRate;    // 22 Khz
  AMPMIX.ulOperation      = ulOperation;                   // Play or Record
  AMPMIX.sChannels        = ulpInstance->lDefaultChannels; // Stereo
  AMPMIX.lBitsPerSRate    = ulpInstance->lDefaultBPS;      // 8 bits/sam
  AMPMIX.ulBlockAlignment = DEFAULT_BLOCK_ALIGN;           // bogus value
  ulpInstance->ulAverageBytesPerSec =  ulpInstance->lDefaultChannels *
                                       ( ulpInstance->lDefaultBPS / 8 ) *
                                       ulpInstance->lDefaultSRate;
  AMPMIX.ulFlags = FIXED|VOLUME|TREBLE|BASS;
}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: VSDInstToWaveSetParms
*
* DESCRIPTIVE NAME:
*
* FUNCTION: copy parameters from VSD Instance struct to MCI_WAVE_SET_PARMS
*
*
* NOTES:
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
VOID VSDInstToWaveSetParms ( PMCI_WAVE_SET_PARMS pWaveSetParms,
                             INSTANCE            *ulpInstance)
{

  pWaveSetParms->usChannels       = AMPMIX.sChannels;
  pWaveSetParms->usFormatTag      = AMPMIX.sMode;
  pWaveSetParms->ulSamplesPerSec  = AMPMIX.lSRate;
  pWaveSetParms->usBitsPerSample  = (USHORT) AMPMIX.lBitsPerSRate;
}


/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME:AssocMemPlayToAudioStrm ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Associate The Memory Play List Stream Handler with
*           the currently connected audio stream handler.
*
*
* NOTES:
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:SpiAssociate()
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
RC AssociatePlayList (INSTANCE * ulpInstance, ULONG Operation)

{
  ULONG ulrc;

  /******************************************************
  * Fill in the Playlist Access Control Block (ACB)
  * with the correct information to set up a stream.
  ******************************************************/

  ulpInstance->StreamInfo.acbPlayList.ulObjType   = ACBTYPE_MEM_PLAYL;
  ulpInstance->StreamInfo.acbPlayList.ulACBLen    = sizeof(ACB_MEM_PLAYL);
  ulpInstance->StreamInfo.acbPlayList.pMemoryAddr = (PVOID)ulpInstance->pPlayList;

  /*********************************************
  * If this is a playback request, then the
  * memory stream handler will be the source
  * and the audio stream handler will be the
  * target.
  *********************************************/

  if (Operation == PLAY_STREAM)
      {
      ulrc = SpiAssociate (ulpInstance->StreamInfo.hStream,
                           ulpInstance->StreamInfo.hidASource,
                           (PVOID) &(ulpInstance->StreamInfo.acbPlayList));
      }

  /*********************************************
  * If this is record request, then the
  * memory stream handler will be the target
  * and the audio stream handler will be the
  * source (i.e. we are recording to memory)
  *********************************************/

    else
      {
      ulrc = SpiAssociate ( ulpInstance->StreamInfo.hStream,
                            ulpInstance->StreamInfo.hidATarget,
                            (PVOID)&ulpInstance->StreamInfo.acbPlayList);
      }

  return (ulrc);

} /* AssociatePlaylist */

/************************** START OF SPECIFICATIONS ***********************
*                                                                          *
* SUBROUTINE NAME: MCD_EnterCrit                                           *
*                                                                          *
* FUNCTION: This routine acquires access to the common areas via a         *
*           system semaphore.                                              *
*                                                                          *
* NOTES:    This routine contains OS/2 system specific functions.          *
*           DosRequestMutexSem                                             *
*                                                                          *
* INPUT:    None.                                                          *
*                                                                          *
* OUTPUT:   rc = error return code is failure to acquire semaphore.        *
*                                                                          *
* SIDE EFFECTS: Access acquired.                                           *
*                                                                          *
*************************** END OF SPECIFICATIONS **************************/

ULONG MCD_EnterCrit (INSTANCE * ulpInstance )
{
  /*****************************************************************
  * Request the system semaphore for the common data area.
  *****************************************************************/

  return( DosRequestMutexSem (ulpInstance->hmtxDataAccess, -1));  // wait for semaphore
}





/************************** START OF SPECIFICATIONS ***********************
*                                                                          *
* SUBROUTINE NAME: MCD_ExitCrit                                            *
*                                                                          *
* FUNCTION: This routine releases access to the common areas via a         *
*           system semaphore.                                              *
*                                                                          *
* NOTES:    This routine contains OS/2 system specific functions.          *
*           DosReleaseMutexSem                                             *
*                                                                          *
* INPUT:    None.                                                          *
*                                                                          *
* OUTPUT:   rc = error return code is failure to release semaphore.        *
*                                                                          *
* SIDE EFFECTS: Access released.                                           *
*                                                                          *
*************************** END OF SPECIFICATIONS **************************/

ULONG MCD_ExitCrit (INSTANCE * ulpInstance)
{
  /************************************************************
  * Release the system semaphore for the common data area.
  *************************************************************/

  return( DosReleaseMutexSem (ulpInstance->hmtxDataAccess));
}


/************************** START OF SPECIFICATIONS ***********************
*                                                                          *
* SUBROUTINE NAME: MCD_EnterCrit                                           *
*                                                                          *
* FUNCTION: This routine acquires access to the common areas via a         *
*           system semaphore.                                              *
*                                                                          *
* NOTES:    This routine contains OS/2 system specific functions.          *
*           DosRequestMutexSem                                             *
*                                                                          *
* INPUT:    None.                                                          *
*                                                                          *
* OUTPUT:   rc = error return code is failure to acquire semaphore.        *
*                                                                          *
* SIDE EFFECTS: Access acquired.                                           *
*                                                                          *
*************************** END OF SPECIFICATIONS **************************/

ULONG GetSaveSem (INSTANCE * ulpInstance )
{
  /*****************************************************************
  * Request the system semaphore for MCIDRV_SAVE access
  *****************************************************************/

  return( DosRequestMutexSem (ulpInstance->hmtxSaveAccess, -1));  // wait for semaphore
}





/************************** START OF SPECIFICATIONS ***********************
*                                                                          *
* SUBROUTINE NAME: MCD_ExitCrit                                            *
*                                                                          *
* FUNCTION: This routine releases access to the common areas via a         *
*           system semaphore.                                              *
*                                                                          *
* NOTES:    This routine contains OS/2 system specific functions.          *
*           DosReleaseMutexSem                                             *
*                                                                          *
* INPUT:    None.                                                          *
*                                                                          *
* OUTPUT:   rc = error return code is failure to release semaphore.        *
*                                                                          *
* SIDE EFFECTS: Access released.                                           *
*                                                                          *
*************************** END OF SPECIFICATIONS **************************/

ULONG GiveUpSaveSem (INSTANCE * ulpInstance)
{
  /************************************************************
  * Release the system semaphore for MCIDRV_SAVE access.
  *************************************************************/

  return( DosReleaseMutexSem (ulpInstance->hmtxSaveAccess));
}





/************************** START OF SPECIFICATIONS ************************
*                                                                          *
* SUBROUTINE NAME: AcquireProcSem                                          *
*                                                                          *
* FUNCTION: This routine acquires access to the common areas via a         *
*           system semaphore.                                              *
*                                                                          *
* NOTES:    This routine contains OS/2 system specific functions.          *
*           DosRequestMutexSem                                             *
*                                                                          *
* INPUT:    None.                                                          *
*                                                                          *
* OUTPUT:   rc = error return code is failure to acquire semaphore.        *
*                                                                          *
* SIDE EFFECTS: Access acquired.                                           *
*                                                                          *
*************************** END OF SPECIFICATIONS **************************/

ULONG AcquireProcSem ( void )
{
  extern HMTX   hmtxProcSem;

  /**************************************************************
  * Request the system semaphore for the common data area.
  ***************************************************************/

  return( DosRequestMutexSem (hmtxProcSem, -1));
}

/************************** START OF SPECIFICATIONS ************************
*                                                                          *
* SUBROUTINE NAME: ReleaseProcSem                                          *
*                                                                          *
* FUNCTION: This routine releases access to the common areas via a         *
*           system semaphore.                                              *
*                                                                          *
* NOTES:    This routine contains OS/2 system specific functions.          *
*           DosReleaseMutexSem                                             *
*                                                                          *
* INPUT:    None.                                                          *
*                                                                          *
* OUTPUT:   rc = error return code is failure to release semaphore.        *
*                                                                          *
* SIDE EFFECTS: Access released.                                           *
*                                                                          *
*************************** END OF SPECIFICATIONS **************************/

ULONG ReleaseProcSem ( void )
{
   extern HMTX  hmtxProcSem;

  /**********************************************************
  * Release the system semaphore for the common data area.
  ***********************************************************/

  return( DosReleaseMutexSem (hmtxProcSem) );
}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: CleanUp ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Call HhpFreeMem to dealocatte memory from global heap.
*
*
* NOTES:    Release Memory.
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES: HhpFreeMem().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC CleanUp (PVOID MemToFree)
{
  extern HHUGEHEAP heap;

  /****************************
  * Enter Data Critical Section
  *****************************/
  AcquireProcSem ();

  /***************************
  * Free Memory off of the heap
  ****************************/

  HhpFreeMem (heap, MemToFree);

  /****************************
  * Exit Data Critical Section
  *****************************/

  ReleaseProcSem ();

  return (MCIERR_SUCCESS);

} /* CleanUp */




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: OpenFile
*
* DESCRIPTIVE NAME: OpenFile
*
* FUNCTION: Open Element , Install IO Procs, Get Header Information
*
*
* NOTES:
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* NOTES: Concepts this functions illustrates.
*          A. Why MMIO_TRANSLATE_HEADER is used.
*          B.
*
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/
RC OpenFile(INSTANCE * pInstance, ULONG ulFlags )
{

  MMIOINFO         mmioinfo;             // mmioInfo Structure
  MMFORMATINFO     mmformatinfo;         /* io proc cap struct */
  PSZ              pFileName;            // Element Name
  LONG             lNumFormats;          /* Number of format info structs */


  /********************************
  * Intialize data structures
  ********************************/

  ULONG ulrc = MCIERR_SUCCESS;


   memset( &mmioinfo, '\0', sizeof( MMIOINFO ));
   mmioinfo.fccIOProc = mmioFOURCC( 'W', 'A', 'V', 'E' ) ;


   /**************************************************
   * Set header translation flag on. It is imporatant
   * that we do this because the loaded io proc has
   * to communicate with us via the standard audio
   * header.  Until we get device capabilities, we
   * must assume that the audio device can handle
   * the raw data.
   **************************************************/

   mmioinfo.ulTranslate = MMIO_TRANSLATEHEADER;


   pFileName = pInstance->pszAudioFile;
   pInstance->fFileExists = FALSE;


    /********************************************
    * When we called the mmio api for temp files,
    * it returned a DOS handle to the file.  The
    * mmioOpen api allows one to pass in a Dos
    * handle rather than a filename if you place
    * the handle in adwInfo[ 0 ] and make the
    * name NULL.
    *********************************************/

    if ( pInstance->ulCreatedName )
       {
       mmioinfo.aulInfo[ 0 ] = pInstance->hTempFile;
       pFileName = ( PSZ ) NULL;
       }

    /***********************************************
    * Try to open the file.  Note: the first
    * open attempt will always specifically
    * try to open the file with the RIFF IO Proc.
    * We do this for speed reasons--if this fails
    * we let mmio do an auto identify on the file,
    * mmio will ask every io proc in the system if
    * it can load the file.
    ***********************************************/

    pInstance->hmmio = mmioOpen ( pFileName,
                                  &mmioinfo,
                                  ulFlags );

    /*********************************************************
    * If we have no file handle--the RIFF IO Proc failed to
    * open the file for one ofthe following reasons:
    *  1. Sharing violation.
    *  2. Path/File errors.
    *********************************************************/

    if ( pInstance->hmmio == (ULONG) NULL )
        {

        /*********************************************
        * We will get a sharing violation if we are
        * writing to a read-only device (CD), if the
        * file is readonly or other misc. problems.
        *********************************************/

        if (mmioinfo.ulErrorRet == ERROR_ACCESS_DENIED ||
            mmioinfo.ulErrorRet == ERROR_WRITE_PROTECT ||
            mmioinfo.ulErrorRet == ERROR_NETWORK_ACCESS_DENIED ||
            mmioinfo.ulErrorRet == ERROR_SHARING_VIOLATION )
            {

            /*********************************************************
            * If the wave fails the open, we should still try w/o the
            * exclusive flag so that another IO can possibly load it.
            * We do this because a sharing violation could occur if
            * we tried exclusively to open the file and other process
            * has it open.
            **********************************************************/

            ulFlags &= ~MMIO_EXCLUSIVE;

            pInstance->hmmio = mmioOpen ( pFileName,
                                          &mmioinfo,
                                          ulFlags );

            if ( mmioinfo.ulErrorRet == ERROR_ACCESS_DENIED ||
                 mmioinfo.ulErrorRet == ERROR_WRITE_PROTECT ||
                 mmioinfo.ulErrorRet == ERROR_NETWORK_ACCESS_DENIED ||
                 mmioinfo.ulErrorRet == ERROR_SHARING_VIOLATION )
               {
               return MCIERR_FILE_ATTRIBUTE;
               }

            pInstance->hmmio = 0;

            } /* if a file attribute error occurred */

        /*********************************************
        * We will get a file not found or an open
        * failure if the path does not exist, or the
        * file DNE or the drive is invalid.
        *********************************************/


        if (mmioinfo.ulErrorRet == ERROR_FILE_NOT_FOUND ||
            mmioinfo.ulErrorRet == ERROR_PATH_NOT_FOUND ||
            mmioinfo.ulErrorRet == ERROR_OPEN_FAILED )
           {
           return ( ERROR_FILE_NOT_FOUND );
           } /* Error File Not Found */

        /*********************************************
        * Map OS/2 errors to MCI or MMIO errors
        *********************************************/


        if (mmioinfo.ulErrorRet == ERROR_FILENAME_EXCED_RANGE )
           {
           return ( MMIOERR_INVALID_FILENAME );
           } /* Error File Not Found */


//        /* Ask mmio to get info about the file */
//
//        ulrc = mmioIdentifyFile( pFileName,
//                                 (ULONG) NULL,
//                                 &mmformatinfo,
//                                 &fccStorageSystem,
//                                 0,
//                                 0 );
//
//        /* if the call worked, interegate the io proc */
//
//        if ( ulrc == MMIO_SUCCESS )
//
//           {
//           /***********************************************
//           * If this is a non audio I/O proc, don't open
//           ************************************************/
//
//         if ( mmformatinfo.ulMediaType != MMIO_MEDIATYPE_AUDIO )
//               {
//               return (MCIERR_INVALID_MEDIA_TYPE);
//               }

           /***********************************************
           * The current release does not support opening
           * IO Procs other that RIFF format in write mode.
           * Therefore, temporary files are impossible.
           ************************************************/
           ulFlags = MMIO_READ | MMIO_DENYNONE;
           pInstance->ulOpenTemp = FALSE;


           memset( &mmioinfo, '\0', sizeof( mmioinfo ) );

           // need to find out why a mmioinfo struct cause identify to fail

           mmioinfo.ulTranslate = MMIO_TRANSLATEDATA;
           mmioinfo.aulInfo[ 3 ] = MMIO_MEDIATYPE_AUDIO;


           /* Open the file */

           pInstance->hmmio = mmioOpen ( pFileName,
                                         &mmioinfo,
                                         ulFlags);

           /* Check for errors--see comments from above */

           if (pInstance->hmmio == (ULONG) NULL)
             {
             if ( mmioinfo.ulErrorRet == MMIOERR_MEDIA_NOT_FOUND )
               {
               return  ( MCIERR_INVALID_MEDIA_TYPE );
               }

             return  ( mmioinfo.ulErrorRet );
             }

           memset( &mmformatinfo, '\0', sizeof( MMFORMATINFO ) );
           mmformatinfo.fccIOProc = mmioinfo.fccIOProc;

           ulrc = mmioGetFormats( &mmformatinfo,
                                     1,
                                     &mmformatinfo,
                                     &lNumFormats,
                                     0,
                                     0 );

           pInstance->ulCapabilities = 0;

           if ( !ulrc )
              {
              /*--------------------------------------------------
              * Determine what capabilities the current io
              * proc really has (i.e. can it record, save etc.)
              *-------------------------------------------------*/

              if (mmformatinfo.ulFlags & MMIO_CANSAVETRANSLATED)
                 {
                 pInstance->ulCapabilities = CAN_SAVE;
                 pInstance->ulOpenTemp = TRUE;
                 }

              if (mmformatinfo.ulFlags & MMIO_CANINSERTTRANSLATED )
                 {
                 pInstance->ulCapabilities |= CAN_INSERT;
                 }

              if ( mmformatinfo.ulFlags & MMIO_CANWRITETRANSLATED)
                 {
                 pInstance->ulCapabilities |= CAN_RECORD;
                 }

              /*-------------------------------------------------
              * If the io proc has the ability to record, close
              * and reopen the file with the flags necessary to
              * do this
              *-------------------------------------------------*/

              if ( pInstance->ulCapabilities )
                 {
                 mmioClose( pInstance->hmmio, 0 );

                 /***********************************************
                 * The current release does not support opening
                 * IO Procs other that RIFF format in write mode.
                 * Therefore, temporary files are impossible.
                 ************************************************/
                 ulFlags = MMIO_READWRITE | MMIO_EXCLUSIVE;
                
                 memset( &mmioinfo, '\0', sizeof( mmioinfo ) );
                
                 // need to find out why a mmioinfo struct cause identify to fail
                
                 mmioinfo.ulTranslate = MMIO_TRANSLATEDATA;
                 mmioinfo.aulInfo[ 3 ] = MMIO_MEDIATYPE_AUDIO;
                
                 /* Open the file */
                 pInstance->hmmio = mmioOpen ( pFileName,
                                               &mmioinfo,
                                               ulFlags);

                 if ( !pInstance->hmmio )
                    {
                    return ( mmioinfo.ulErrorRet );
                    }
                 } /* if we must close and reopen the file */

              }
           else
              {
              ulrc = MCIERR_SUCCESS;
              }
//           } /* if identify was successful */
//        else
//           {
//           return ( ulrc );
//           }
        } /* If the file was not opened with OPEN_MMIO */

    else
        {
        /************************************************
        * Since the wave IO Proc opened the file, we know
        * that it has the following capabilities.
        *************************************************/
        pInstance->ulCapabilities =  ( CAN_INSERT | CAN_DELETE | CAN_UNDOREDO +
                                       CAN_SAVE   | CAN_INSERT | CAN_RECORD  );
        }

  /******************************************
  * Get The Header Information
  *******************************************/

  if ( !(ulFlags & MMIO_CREATE) )
     {

      ulrc = GetAudioHeader (pInstance);

     } /* Not Create Flag */
  else
     {
     pInstance->ulDataSize = 0;
     }

  pInstance->fFileExists = TRUE;


  /*******************************************************************
  * You cannot do the set header immediately after file creation
  * because future sets on samples, bitpersample, channels may follow
  ********************************************************************/

  return (ulrc);

} /* OpenFile */


/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ConvertToMM ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Convert Time values from MMTIME units to current time format.
*
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC   ConvertToMM ( INSTANCE  *ulpInstance,
                   PULONG    pulSeekPoint,
                   ULONG     ulValue)
{
/*also use floats are reverse the division/multiplications.
*  why does this function have a return value? */

  ULONG      ulBytesPerSample;
  ULONG      ulTemp1;


  switch (ulpInstance->ulTimeUnits)
  {

  case lMMTIME:
       *pulSeekPoint = ulValue;
      break;

  case lMILLISECONDS:
       *pulSeekPoint = MSECTOMM ( ulValue);
      break;

  case lSAMPLES:
       ulBytesPerSample = (AMPMIX.lBitsPerSRate / 8);
       ulTemp1 = ulValue * ulBytesPerSample;

       ulTemp1 /= ulpInstance->ulBytes ;
       *pulSeekPoint = ulTemp1 * ulpInstance->ulMMTime;

      break;
  default:  /* the time value must be bytes */

       ulTemp1 = ulValue / ulpInstance->ulBytes ;
       *pulSeekPoint = ulTemp1 * ulpInstance->ulMMTime;
//       Bytes2Mmtime( ulpInstance->ulBytes,  (LONG) ulValue, ( LONG ) ulpInstance->ulMMTime, ( PLONG ) pulSeekPoint);

      break;
  } /* switch time units */

  return ( MCIERR_SUCCESS );

} /* ConvertToMM */

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ConvertTimeUnits ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Convert Time values from the current base to MMTIME units.
*           We obtained the conversion factors from SpiGetProtocol which
*           has the smallest breakdowns of mmtime per block of time and
*           bytes per block of time.
*
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC  ConvertTimeUnits ( INSTANCE    *ulpInstance,
                       PULONG      pulSeekPoint,
                       ULONG       ulValue)
{
/* add a flag and remove this file length kludge */

  ULONG      ulBytesPerSample;
  ULONG      ulTemp1;


  /*---------------------------------------------------
  * due to inaccuracies in the conversions, if the request is bytes or samples
  *  simply return the number
  *  this file length stuff is garbarge, add a new flag
  *---------------------------------------------------*/
  /**************************************
  * Computation of Media Element Length
  * This routine is called with FILE LENGTH
  * value to signify Total length is
  * requested.
  ***************************************/
  if ( ulValue == FILE_LENGTH)
      {

      /* due to inaccuracies in the conversions, if the request is bytes or samples
      *  simply return the number itself
      */

      if ( ulpInstance->ulTimeUnits == lBYTES )
         {
         *pulSeekPoint = ulpInstance->ulDataSize;
         return ( MCIERR_SUCCESS );
         }
      else if ( ulpInstance->ulTimeUnits == lSAMPLES )
         {
         ulBytesPerSample = (AMPMIX.lBitsPerSRate / 8);
         *pulSeekPoint = ulpInstance->ulDataSize / ulBytesPerSample;
         return ( MCIERR_SUCCESS );
         }

      /***********************************************
      * Get the number of blocks of audio information the
      * desired number of bytes consumes.
      *************************************************/
      ulTemp1 = ulpInstance->ulDataSize / ulpInstance->ulBytes;



      /***********************************************
      * Multiply the blocks above by the length in time
      * of a block.
      *************************************************/

      ulValue = ulTemp1 * ulpInstance->ulMMTime;

      } /* Return File Length */


  switch (ulpInstance->ulTimeUnits)
  {
  case lMMTIME:
       *pulSeekPoint = ulValue;
      break;

  case lMILLISECONDS:
       *pulSeekPoint = MSECFROMMM ( ulValue );
      break;

  case lSAMPLES:
       ulBytesPerSample = (AMPMIX.lBitsPerSRate / 8);

       ulTemp1 = ulValue / ulpInstance->ulMMTime ;
       ulTemp1 *= ulpInstance->ulBytes;
       *pulSeekPoint = ulTemp1 / ulBytesPerSample;
      break;

  default: // time value must be bytes
       ulTemp1 = ulValue / ulpInstance->ulMMTime ;
      *pulSeekPoint = ulTemp1 * ulpInstance->ulBytes;
//       Mmtime2Bytes( ulpInstance->ulBytes,  (LONG) ulValue, ( LONG ) ulpInstance->ulMMTime, ( PLONG ) pulSeekPoint);
      
      break;

  } /* Of Switch */

  return ( MCIERR_SUCCESS );

} /* ConvertTimeUnits */

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: CreateNAssocStream
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Create a stream and associate this stream with its data
*           object.
*
*
* NOTES: This function illustrates how one can create a custom event
*        structure and have SSM return events in this structure.
*        This allows one to place information like window callback handles
*        in the event structure, so that when a certain event occurs,
*        a message can be sent via this window call back handle.
*
*        The creation of this custom structure can be found in audiomcd.h
*        and the use of this structure can be found in admcplay.c and
*        admcrecd.c.  The initialization of this structure can be
*        found in admcopen.c
*
* INPUT:
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code (from SPI).
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:  SpiCreateStream ()      - SSM SPI
*                       SpiAssociate ()         - SSM SPI
*
*********************** END OF SPECIFICATIONS *******************************/

RC CreateNAssocStream ( HID       hidSrc,         /* Source Handler HID */
                        HID       hidTgt,         /* Target Handler HID */
                        HSTREAM   *hStream,       /* Stream Handle ()   */
                        INSTANCE  *pInstance,     /* Instance Pointer   */
                        ULONG     Operation,      /* Play or Record     */
                        PEVFN     EventProc)      /* Event Entry Point  */

{

  ULONG     ulrc;

  /*********************************************************
  * Create the stream that play/record/cue will use.
  * The caller supplies the source and target stream
  * handlers and the audio device control block should
  * have been filled in previously ( see InitAudioDevice )
  * The caller also supplies the EventProc(edure)
  * where all of the stream events will be reported.
  *
  * Note: pInstance->StreamInfo.Evcb contains the
  *********************************************************/


  ulrc = SpiCreateStream ( hidSrc,
                           hidTgt,
                           &pInstance->StreamInfo.SpcbKey,
                           (PDCB) &pInstance->StreamInfo.AudioDCB,
                           (PDCB) &pInstance->StreamInfo.AudioDCB,
                           (PIMPL_EVCB) &pInstance->StreamInfo.Evcb,
                           (PEVFN) EventProc,
                           (ULONG) NULL,
                           hStream,
                           &pInstance->StreamInfo.hEvent);

  if (ulrc)
      return (ulrc);

  /**********************************************************
  * The stream must be associated with a data object
  * in our case a mmio file handle.  The file system
  * stream handler (FSSH) will always be the stream handler
  * that we want to associate the data object with,
  * therefore, if we have created a playback stream then
  * FSSH is the source, so associate with the source.  On a
  * record stream, FSSH is the target, so associate with
  * the target.
  **********************************************************/


  pInstance->StreamInfo.acbmmio.ulObjType = ACBTYPE_MMIO;
  pInstance->StreamInfo.acbmmio.ulACBLen = sizeof (ACB_MMIO);
  pInstance->StreamInfo.acbmmio.hmmio = pInstance->hmmio;

  if (Operation == PLAY_STREAM)
      {

      ulrc = SpiAssociate ( (HSTREAM)*hStream,
                            hidSrc,
                            (PVOID) &pInstance->StreamInfo.acbmmio);

      }  /* Associating play stream */

  else if ( Operation == RECORD_STREAM )
      {
      ulrc = SpiAssociate ( (HSTREAM) *hStream,
                            hidTgt,
                            (PVOID) &pInstance->StreamInfo.acbmmio);

      } /* Associating record stream */


  return (ulrc);

} /* CreateStream */

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: DoTillEvent()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Enable a Time Event to process MCI_PLAY or MCI_RECORD with
*           the MCI_TO Flag on.
*
* NOTES: This function will create a cue time pause event.  When the
*        stream reaches the event during a play or record, the
*        audio stream handler will signal the MCD--note, the stream
*        will not be stopped, it is the MCD's responsibility.
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: MCIERR_SUCCESS.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES: SpiEnableEvent()
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
RC CreateToEvent (INSTANCE *ulpInstance, ULONG ulTo)
{
/* rename this function CreateToEvent */


  ULONG ulrc;

  /*********************************************************
  * Set up a cue time pause event at the place in
  * the stream where the caller wants us to play/record
  * to.  Note: this event will pause the stream and
  * will be considerably more accurate than just
  * setting a cue point, receiving the event and stopping
  * the stream (since a normal cue point will force
  * bleed over).
  *********************************************************/

  ulpInstance->StreamInfo.TimeEvcb.hwndCallback      = ulpInstance->hwndCallBack;
  ulpInstance->StreamInfo.TimeEvcb.usDeviceID         = ulpInstance->usWaveDeviceID;
  ulpInstance->StreamInfo.TimeEvcb.evcb.ulType       = EVENT_CUE_TIME_PAUSE;
  ulpInstance->StreamInfo.TimeEvcb.evcb.ulFlags      = EVENT_SINGLE;
  ulpInstance->StreamInfo.TimeEvcb.evcb.hstream      = ulpInstance->StreamInfo.hStream;
  ulpInstance->StreamInfo.TimeEvcb.ulpInstance       = (ULONG) ulpInstance;
  ulpInstance->StreamInfo.TimeEvcb.evcb.mmtimeStream = ulTo;

  /* Enable the cue time pause event. */

  ulrc = SpiEnableEvent((PEVCB) &ulpInstance->StreamInfo.TimeEvcb.evcb,
                        (PHEVENT) &ulpInstance->StreamInfo.hPlayToEvent);

  return ( ulrc );

} /* CreateToEvent */

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:              GetPDDName
*
* DESCRIPTIVE NAME:
*
* FUNCTION:     Retrieves the PDD name (AUDIO1$, etc.) based
*               on which AmpMixer we're connected to
*
* NOTES:        DCR 104
*
* INPUT:
*         ULONG         ulDeviceType
*         CHAR          szPDDName [MAX_PDD_NAME]        - OUTPUT
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:
*
*********************** END OF SPECIFICATIONS *********************************/

RC GetPDDName (ULONG ulDeviceType, CHAR szPDDName [MAX_PDD_NAME])
{

   ULONG                   rc;

   CHAR                    szIndex[2];
   CHAR                    szAmpMix[9] = "AMPMIX0";

   MCI_SYSINFO_PARMS       SysInfo;
   MCI_SYSINFO_LOGDEVICE   SysInfoParm;
   MCI_SYSINFO_QUERY_NAME  QueryNameParm;

   memset (&SysInfo, '\0', sizeof(SysInfo));
   memset (&SysInfoParm, '\0', sizeof(SysInfoParm));
   memset (&QueryNameParm, '\0', sizeof(QueryNameParm));

   SysInfo.ulItem       = MCI_SYSINFO_QUERY_NAMES;
   SysInfo.usDeviceType  = LOUSHORT(ulDeviceType);
   SysInfo.pSysInfoParm = &QueryNameParm;

   itoa (HIUSHORT(ulDeviceType), szIndex, 10);

   szIndex[1] = '\0';

   strncat (szAmpMix, szIndex, 2);
   strcpy (QueryNameParm.szLogicalName, szAmpMix);

   if (rc = mciSendCommand (0,
                            MCI_SYSINFO,
                            MCI_SYSINFO_ITEM | MCI_WAIT,
                            (PVOID) &SysInfo,
                            0))
           return (rc);




   /*******************************************
   * Get PDD associated with our AmpMixer
   * Device name is in pSysInfoParm->szPDDName
   ********************************************/

   SysInfo.ulItem       = MCI_SYSINFO_QUERY_DRIVER;
   SysInfo.usDeviceType  = (USHORT) ulDeviceType;
   SysInfo.pSysInfoParm = &SysInfoParm;

   strcpy (SysInfoParm.szInstallName, QueryNameParm.szInstallName);

   if (rc = mciSendCommand (0,
                            MCI_SYSINFO,
                            MCI_SYSINFO_ITEM | MCI_WAIT,
                            (PVOID) &SysInfo,
                            0))
       return (rc);

   strcpy (szPDDName, SysInfoParm.szPDDName);

   return ( MCIERR_SUCCESS );

} /* GetPDDName */


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: Generic thread abort
*
* DESCRIPTIVE NAME:
*
* FUNCTION: This will abort any waiting command on another thread.
*
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:
*
*********************** END OF SPECIFICATIONS *********************************/


void GenericThreadAbort( INSTANCE             *ulpInstance,
                         FUNCTION_PARM_BLOCK  *pFuncBlock, 
                         ULONG                ulCloseFlag )
 

{
BOOL     fForceDiscard = FALSE;

  if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
     {
     /****************************************
     ** Save is a non-interruptible operation
     ** wait for completion
     *****************************************/

     DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );
     }
   else
     {
     PostMDMMessage ( MCI_NOTIFY_ABORTED, 
                      ulpInstance->usNotPendingMsg,
                      pFuncBlock);


     /* Stop the pending thread */

     if ( !(ulCloseFlag & MCI_CLOSE_EXIT) )
        {
        if ( pFuncBlock->usMessage == MCI_CLOSE )
           {
           /*****************************************
           * create a fake event so the thread we are
           * waiting on will have a chance to clean
           * up.  It is important that we do not do
           * an spistop since the instance may not
           * be currently active.
           *****************************************/
 
           ulpInstance->StreamEvent = EVENT_STREAM_STOPPED;
           DosPostEventSem (ulpInstance->hEventSem);
 
           DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
           }

        else
           {
           ThreadedStop ( ulpInstance );
           }
        }

     } /* if !save pending */

} /* GenericThreadAbort */



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: ThreadedStop
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Stop pending plays or records
*
* INPUT:
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:
*
*********************** END OF SPECIFICATIONS *********************************/

void ThreadedStop( INSTANCE     *ulpInstance )


{
ULONG   ulrc;
ULONG   ulCount;
ULONG   ulSpiFlags;




   if ( AMPMIX.ulOperation == OPERATION_PLAY )
      {
      ulSpiFlags = SPI_STOP_DISCARD;
      }
   else
      {
      ulSpiFlags = SPI_STOP_FLUSH;
      }

   /*****************************************************
   * Stop discard for play because data loss is not
   * important.
   * Stop flush a record stream so that no data will be
   * lost.
   *****************************************************/

   ulrc = SpiStopStream (ulpInstance->StreamInfo.hStream, ulSpiFlags);

   if (!ulrc)
     {
     DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
     }

   STRMSTATE = MCI_STOP;

  return;

} /* ThreadedStop */
