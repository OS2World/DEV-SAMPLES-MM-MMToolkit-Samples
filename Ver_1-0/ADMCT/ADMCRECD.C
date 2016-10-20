/*static char *SCCSID = "@(#)admcrecd.c	13.17 92/04/29";*/
/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCIRECD.C
*
* DESCRIPTIVE NAME: Audio MCD WaveAudio Recording Function.
*
* FUNCTION:Record into  Waveform Audio Element.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_Record message.
*        MCI_FROM Flag
*        MCI_TO   Flag
*        MCI_RECORD_OVERWRITE   Flag
*        MCI_RECORD_INSERT      Flag
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*           The Original Element may be modified.
*           All Recording is lost if MCI_SAVE message is not specified.
*
*
* INTERNAL REFERENCES:   CreateNAssocStream ().
*                        DestroyStream ().
*                        AssocMemPlayToAudioStream ().
*                        DoTillEvent().
*                        ConvertTimeUnits ().
*                        StartRecord().
*                        ReEventProc().
*                        SetAudioDevice().
*                        InitAudioDevice().
*                        SetWaveDeviceDefaults().
*                        OpenFile().
*                        CheckMem ().
*
* EXTERNAL REFERENCES:   DosResetEventSem ()        - OS/2 API
*                        DosPostEventSem  ()        - OS/2 API
*                        DosCreateThread ()         - OS/2 API
*                        SpiEnableEvent ()          - MME API
*                        SpiStopStream ()           - MME API
*                        SpiCreateStream ()         - MME API
*                        SpiAssociate()             - MME API
*                        SpiSeekStream ()           - MME API
*                        mdmDriverNotify ()         - MME API
*                        mmioSetHeader ()           - MME API
*
*********************** END OF SPECIFICATIONS **********************/
#define INCL_BASE
#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES

#include <os2.h>
#include <string.h>
#include <os2medef.h>                   // MME includes files.
#include <audio.h>                      // Audio Device defines
#include <ssm.h>                        // SSM spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmsystem.h>                   // MM System Include.
#include <mcidrv.h>                     // Mci Driver Include.
#include <mmio.h>                       // MMIO Include.
#include <mcd.h>                        // VSDIDriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions
#include <audiomcd.h>                   // Component Definitions.
#include "admcfunc.h"                   // Function Prototypes

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCIRECD.C
*
* DESCRIPTIVE NAME: Waveform Record Routine.
*
* FUNCTION: Record into an Waveform File.
*
* NOTES:  hStream[2] = B --> A = Record stream.
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_PLAY message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  CreateNAssocStream ().
*                       ().
*                       DoTillEvent ().
*                       OpenFile ().
*                       SetAudioDevice ().
*                       SetWaveDeviceDefaults ().
*                       AssocMemPlayToAudioStream ().
*
* EXTERNAL REFERENCES:  SpiStartStream ()    - SSM  Spi
*                       SpiStopStream  ()    - SSM  Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIRecd (FUNCTION_PARM_BLOCK *pFuncBlock)

{
  ULONG                   ulrc;               // Propogated Error Code
  INSTANCE                *ulpInstance;       // Local Instance
  ULONG                   ulParam1;           // Incoming MCI Flags
  USHORT                  usEvcbIndex;        // EVCB Loop Index
  USHORT                  ulAssocFlag;        // Stream Handler Type
  DWORD                   dwSetAll;           // Set all Wave Extens
  DWORD                   dwTempTO;           // Length of Element
  ULONG                    lCnt;               // Sem Posting Count
  DWORD                   dwSpiSeekFlags;     // Stream Seek Flags
  DWORD                   dwTemp1;            // Time format conversion
  DWORD                   dwTemp2;            // Time Format conversion
  DWORD                   dwMMFileLength;     // Media Element Length
  MCI_WAVE_SET_PARMS      lpSetParms;         // MCI Wave Set Parms
  LPMCI_RECORD_PARMS      lpRecordParms;      // MCI Record Parms


  /**********************************
  * Intialize Flags and other vars
  ***********************************/
  ulrc = MCIERR_SUCCESS;
  usEvcbIndex = 0;
  ulAssocFlag = 0;
  dwSetAll = MCI_WAVE_SET_BITSPERSAMPLE| MCI_WAVE_SET_FORMATTAG|
             MCI_WAVE_SET_CHANNELS| MCI_WAVE_SET_SAMPLESPERSEC;

  dwSpiSeekFlags = SPI_SEEK_ABSOLUTE;
  /********************************
  * derefernce pointers
  *********************************/
  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;
  ulParam1 = pFuncBlock->ulParam1;

  /**********************************************************************
  * Destroy The stream if a play preceded the direction of the stream is
    reversed. The previous stream gets destroyed.
  **********************************************************************/
  if (AMPMIX.ulOperation == OPERATION_PLAY)
      {

      if ( ulpInstance->StreamEvent == EVENT_EOS ||
           STRMSTATE == MCI_STOP                 ||
           STRMSTATE == CUEPLAY_STATE            ||
           STRMSTATE == MCI_SEEK                 ||
           STRMSTATE == STOP_PAUSED )
         {
         if ( !ulpInstance->ulOldStreamPos  )
            {
            ulrc = SpiGetTime( STREAM.hStream,
                               ( PMMTIME ) &ulpInstance->ulOldStreamPos );

            // if an error occurred, then don't remember our position in the stream

            if ( ulrc )
               {
               ulpInstance->ulOldStreamPos = 0;
               }
            }

         DestroyStream (STREAM.hStream);

         }   /* EOS and hanging around */

         ulpInstance->ulCreateFlag = CREATE_STATE;

      }      /* Transition from PLAY State */


   /***********************************************
   * If a set was performed on an existing stream,
   * destroy the stream and get new spcb keys
   ***********************************************/
   if ( STRMSTATE == STREAM_SET_STATE )
      {
      DestroyStream (STREAM.hStream);

      ulpInstance->ulCreateFlag = CREATE_STATE;

      }

  /*********************************************************************
  * Create The Recording stream. This normally goes from audio stream
  * handler to the file system stream handler. Based on the associate
  * flag we can change the recording destination to File system or
  * memory (in case of playlist)
  *********************************************************************/
  if (ulpInstance->ulCreateFlag != PREROLL_STATE) {

       if (ulpInstance->usPlayLstStrm == TRUE) {
            ulAssocFlag = (ULONG)NULL;
       }
       else
            ulAssocFlag = RECORD_STREAM;

      // get new spcb key

      ulrc = InitAudioDevice (ulpInstance, OPERATION_RECORD);

      if (ulrc)
         {
         return ( ulrc );
         }
      AMPMIX.ulOperation = OPERATION_RECORD;

      VSDInstToWaveSetParms (&lpSetParms,ulpInstance);


      if (!ulrc)
         ulrc = SetAudioDevice ( ulpInstance,
                                 (LPMCI_WAVE_SET_PARMS)&lpSetParms,
                                 dwSetAll);

      if ( ulrc )
        {
        return ( ulrc );
        }

       /*************************************************
       * If the init caused a new global sys file, use it
       *************************************************/

       STREAM.AudioDCB.ulSysFileNum = AMPMIX.ulGlobalFile;

       /*************************************************
       * Create a Stream with B = Src & A = Tgt
       **************************************************/
       ulrc = CreateNAssocStream (STREAM.hidBSource,
                                  STREAM.hidATarget,
                                  &(STREAM.hStream),
                                  (INSTANCE *) ulpInstance,
                                  (ULONG) ulAssocFlag,
                                  (PEVFN)ReEventProc);
       if (ulrc)
           return (MCIERR_DRIVER_INTERNAL);

       /*****************************************************
       * In Case of PlayList Do the Associate Seperately
       ******************************************************/
       if (ulAssocFlag == (ULONG)NULL)
           ulrc = AssocMemPlayToAudioStrm (ulpInstance,
                                           RECORD_STREAM);


       if (ulpInstance->usPosAdvise == EVENT_ENABLED)
           ulpInstance->usPosAdvise = TRUE;

       if (ulpInstance->usCuePt == EVENT_ENABLED)
           ulpInstance->usCuePt = TRUE;

           /**********************
           * Update State Flags
           ***********************/
       ulpInstance->ulCreateFlag = PREROLL_STATE;

       // If we previously destroyed a stream, seek to the position where we were
       // else just seek to 0

       if ( !(ulParam1 & MCI_FROM ) )
          {
          ulrc = SpiSeekStream ( STREAM.hStream,
                                 SPI_SEEK_ABSOLUTE,
                                ulpInstance->ulOldStreamPos );
          if (ulrc)
            {
            return (ulrc);
            }
          }

       ulpInstance->ulOldStreamPos = 0;

       /************************************
       * place the stream in a stopped state
       * since no activity has occurred
       *************************************/

       STRMSTATE = MCI_STOP;

  }  /* Create Flag != Preroll State */

  if (ulpInstance->usPlayLstStrm != TRUE) {

      /******************************************************************
      * Fill in Wave Format. Do the Set header call here. At this point
      * Recording of data commences at whatever audio device attributes
      * that are currently set. This needs to be written in the wave
      * header format chunk.
      ******************************************************************/

      ulrc = SetAudioHeader (ulpInstance);

      ulrc = SetAmpDefaults (ulpInstance);


      ulrc = InitAudioDevice (ulpInstance, OPERATION_RECORD);
      if ( ulrc )
         {
         return ulrc;
         }

  } /* Non PlayList */


   ConvertTimeUnits (ulpInstance, (DWORD*)& (dwMMFileLength),
                     FILE_LENGTH);

  dwTemp1 = dwMMFileLength;

  /****************************************
  * From Flag For Record is not Supported
  *****************************************/

  if (ulParam1 & MCI_FROM) {


      lpRecordParms= (LPMCI_RECORD_PARMS )pFuncBlock->ulParam2;

      if (STRMSTATE != MCI_STOP )
         {
         /**************************************
         * Reset Internal Semaphores Used
         ****************************************/
         DosResetEventSem (ulpInstance->hEventSem, &lCnt);

         /**************************************
         * Stop The Stream (Discard buffers)
         **************************************/
         ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);

         if (!ulrc)
           {
           /*****************************************
           * Wait for the stream to be stopped
           *****************************************/

           DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
           }
         }

     if ( lpRecordParms->dwFrom > dwTemp1 )
       {
       return MCIERR_OUTOFRANGE;
       }

     ulrc = ConvertToMM (ulpInstance, &(dwTemp2),
                         (DWORD)lpRecordParms->dwFrom);

       if (lpRecordParms->dwFrom > dwTemp1 )
          return MCIERR_OUTOFRANGE;

       /******************************
       * Do the Seek Thing
       ******************************/

       if (!ulrc)
         {
         ulrc = SpiSeekStream (STREAM.hStream,
                               dwSpiSeekFlags,
                               (DWORD)( dwTemp2 + ulpInstance->ulSyncOffset));
         }

    } /* Record From */
  /**************************************************/
  // From Flag For Record is Supported
  /**************************************************/
  if (ulParam1 & MCI_TO) {

      if (!ulrc)
          {
          lpRecordParms= (LPMCI_RECORD_PARMS )pFuncBlock->ulParam2;

          ulrc = ConvertToMM (ulpInstance, (DWORD*)&(dwTempTO),
                                  (DWORD)(lpRecordParms->dwTo ));

          ulrc = CheckMem ((PVOID)pFuncBlock->ulParam2,
                           sizeof (MCI_RECORD_PARMS), PAG_READ);

          if (ulrc != MCIERR_SUCCESS)
              return MCIERR_MISSING_PARAMETER;

          if (!ulrc)
              ulrc = DoTillEvent (ulpInstance, dwTempTO);

          ulpInstance->PlayTo = TRUE;
          }
  }   /* To Flag */


  if (STRMSTATE == MCI_SEEK)
      ulrc = SpiSeekStream (STREAM.hStream,
                            SPI_SEEK_ABSOLUTE,
                            STREAM.mmStreamTime);
  /************************************
  * Stick In Instance PTR in EVCB
  *************************************/
  STREAM.Evcb.ulpInstance = (ULONG)ulpInstance;

  /************************************
  * Enable Position Advise if Needed
  *************************************/
  if (ulpInstance->usPosAdvise == TRUE) {

      /*******************************************
      * Correct The hStream Value
      *******************************************/
      STREAM.PosAdvEvcb.evcb.hstream = STREAM.hStream;

      /***********************************************
       * Stick In INSTANCE Pointer in The Time EVCB
      ************************************************/
      STREAM.PosAdvEvcb.ulpInstance = (ULONG)ulpInstance;


      STREAM.PosAdvEvcb.evcb.mmtimeStream = STREAM.PosAdvEvcb.mmCuePt;
      /************************************************
      * Send The Event down to Audio Stream Handler
      *************************************************/
      ulrc = SpiEnableEvent((PEVCB) &(STREAM.PosAdvEvcb),
                            (PHEVENT) &(STREAM.hPosEvent));

      ulpInstance->usPosAdvise = EVENT_ENABLED;

  } /* Enable Position Advise */

  /*****************************
  * Enable Cue points if any
  ******************************/
  if (ulpInstance->usCuePt == TRUE) {
      /******************************
      * Correct The hStream Value
      *******************************/
      for (usEvcbIndex = 0; usEvcbIndex < CUEPTINDX; usEvcbIndex ++) {
           CUEPTEVCB[usEvcbIndex].evcb.hstream = STREAM.hStream;

           /*************************************************
           * Stick In INSTANCE Pointer in The Time EVCB
           **************************************************/
           CUEPTEVCB[usEvcbIndex].ulpInstance = (ULONG)ulpInstance;

           ulrc = SpiEnableEvent((PEVCB) &(STREAM.MCuePtEvcb[usEvcbIndex]),
                                (PHEVENT) &(STREAM.HCuePtHndl[usEvcbIndex]));
      } /* For loop */

      ulpInstance->usCuePt = EVENT_ENABLED; // Reset The Flag
  }  /* Enable Cue Point */

  /****************************************
  * Send Insert Down To The Wave IO Proc
  *****************************************/

  if (ulParam1 & MCI_RECORD_INSERT)
     {
      if ( ulpInstance->ulCanInsert)
         {
         ulrc = mmioSendMessage( ulpInstance->hmmio,
                                 MMIOM_BEGININSERT,
                                 0,
                                 0);
         if (ulrc)
            {
            ulrc = mmioGetLastError( ulpInstance->hmmio );

            return (ulrc);
            }

           ulpInstance->usRecdInsert = TRUE;
         }
      else
         {
         return MCIERR_UNSUPPORTED_FLAG;
         }
     } /* Record Insert */

  if (!ulrc)
      if (ulParam1 & MCI_NOTIFY) {
          ulpInstance->usNotifyPending = TRUE;
          ulpInstance->usNotPendingMsg = MCI_RECORD;

         /****************************************************
         * This thread is kicked off by the MCD mainly
         * to start the stream. Minimum processing should
         * be done on the SSMs eventThread . The eventProc
         * signalls This thread via semaphores and
         * acts accordingly
         ****************************************************/
         DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

         ulrc= DosCreateThread ((PTID)&(pFuncBlock->pInstance->RecdThreadID),
                                 (PFNTHREAD)StartRecord,
                                 (ULONG) pFuncBlock,
                                 (ULONG) 0L,
                                 (ULONG)NOTIFY_THREAD_STACKSIZE);

         /*************************************************
         * Wait for the Record thread to do the start.
         **************************************************/

         if (!ulrc)
             DosWaitEventSem (ulpInstance->hThreadSem, -1);
      }

      else
      {
          ulpInstance->usWaitPending = TRUE;
          ulpInstance->usWaitMsg= MCI_RECORD;
          ulrc = StartRecord (pFuncBlock);
      }

  /**********************************
  * Release data Access Semaphore
  ***********************************/
  MCD_ExitCrit (ulpInstance);

  return (ULONG)(ulrc);
}



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: EventProc.
*
* DESCRIPTIVE NAME: SSM Event Handler
*
* FUNCTION:  Handle Streaming Event Notifications from SSM.
*
* NOTES: This routine is presumed to receive all types of event
*        notifications from the SSM. The types include Implicit
*        events, Cue point notifications in terms of both time
*        and data. In response to Cue point notifications an
*        MCI_CUEPOINT message is returned to MDM via mdmDriverNotify ()
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code and flError flag is set.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: mdmDriverNotify ()  - MDM   API
*
*********************** END OF SPECIFICATIONS **********************/
RC APIENTRY ReEventProc ( MEVCB *lpevcb)
{
  MTIME_EVCB        *pMTimeEVCB;      // Modified EVCB
  ULONG              ulrc;            // RC
  INSTANCE           * ulpInstance;   // Instance Ptr
  HWND               hWnd;            // Call Back Handle

  ulrc = MCIERR_SUCCESS;

  /***********************************************************
  * EventProc receives asynchronous SSM event notifications
  * These events are signalled to the MCDs thread which
  * blocks itself. The posting of the semaphore in response
  * to implicit events releases the blocked thread which
  * is waiting on the instance based event semaphore.
  * The semaphore is not posted for time events like
  * cuepoint (TIME) and media position changes.
  **********************************************************/

  switch (lpevcb->evcb.ulType)
  {
  case EVENT_IMPLICIT_TYPE:
       ulpInstance = (INSTANCE *)lpevcb->ulpInstance;
       hWnd = (HWND)ulpInstance->dwCallback;

       switch (lpevcb->evcb.ulSubType)
       {
       case EVENT_EOS:
            ulpInstance->StreamEvent = EVENT_EOS;
            DosPostEventSem (ulpInstance->hEventSem);
           break;

       case EVENT_ERROR:
            ulpInstance->StreamEvent = EVENT_ERROR;
            /**************************************
            * End of PlayList event is received
            * as an implicit error event. It
            * is treated as a normal EOS
            ***************************************/
            if (ulpInstance->usPlayLstStrm == TRUE)
                if (lpevcb->evcb.ulStatus == ERROR_END_OF_PLAYLIST)
                    ulpInstance->StreamEvent = EVENT_EOS;

            DosPostEventSem (ulpInstance->hEventSem);
           break;

       case EVENT_STREAM_STOPPED:
            /****************************************
            * Event Stream Stopped. Release the
            * Blocked thread
            *****************************************/
            ulpInstance->StreamEvent = EVENT_STREAM_STOPPED;
            ulpInstance->usFileExists = TRUE;
            DosPostEventSem (ulpInstance->hEventSem);
           break;

       case EVENT_SYNC_PREROLLED:
            /******************************************
            * This event is received in reponse to a
            * preroll start. A Preroll start is done
            * on an MCI_CUE message.
            *******************************************/
            ulpInstance->StreamEvent = EVENT_SYNC_PREROLLED;
            DosPostEventSem (ulpInstance->hEventSem);
           break;

       case EVENT_PLAYLISTMESSAGE:
            mdmDriverNotify ((WORD)ulpInstance->wWaveDeviceID,
                             (HWND)hWnd,
                             MM_MCIPLAYLISTMESSAGE,
                             (WORD) MAKEULONG(lpevcb->evcb.ulStatus,
                                              ulpInstance->wWaveDeviceID),
                             (DWORD)lpevcb->evcb.unused1);

           break;


       } /* SubType case of Implicit Events */
      break;

  case EVENT_CUE_DATA:
      break;

  case EVENT_CUE_TIME_PAUSE:
      /***************************************************
      * This event will arrive if we recorded to a certain
      * position in the stream
      ****************************************************/
      pMTimeEVCB = (MTIME_EVCB *)lpevcb;
      ulpInstance = (INSTANCE *)pMTimeEVCB->ulpInstance;
      ulpInstance->StreamEvent = EVENT_CUE_TIME_PAUSE;

      DosPostEventSem (ulpInstance->hEventSem);
      break;
  case EVENT_CUE_TIME:

       pMTimeEVCB = (MTIME_EVCB *)lpevcb;
       ulpInstance = (INSTANCE *)pMTimeEVCB->ulpInstance;

      /***************************************************
      * Single Time Events are Treated as Time Cue Points
      ****************************************************/
      if ( pMTimeEVCB->evcb.ulFlags == EVENT_SINGLE )
         {
         mdmDriverNotify ((WORD)ulpInstance->wWaveDeviceID,
                          (HWND)pMTimeEVCB->dwCallback,
                          MM_MCICUEPOINT,
                          (WORD)pMTimeEVCB->usCueUsrParm,
                          (DWORD)pMTimeEVCB->evcb.mmtimeStream);
         }

      /***********************************************
      * Recurring Events are Media Position Changes
      ************************************************/
      if ( pMTimeEVCB->evcb.ulFlags == EVENT_RECURRING )
         {
         mdmDriverNotify ( ulpInstance->wWaveDeviceID,
                           (HWND)POSEVCB.dwCallback,
                           MM_MCIPOSITIONCHANGE,
                           (WORD)ulpInstance->usPosUserParm,
                           (DWORD)pMTimeEVCB->evcb.mmtimeStream);
         }


      break;

  default:
      break;

  }          /* All Events case */

  return (ULONG)(MCIERR_SUCCESS);

}              /* of Event Handler */





/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: StartRecord.
*
* DESCRIPTIVE NAME:StartRecording
*
* FUNCTION: SpiStartStream().
*
*
* NOTES: This routine is called using app's wait thread (Wait Case)
*        or a separate thread spawned by MCD on MCI Notify. SSMs
*        asynch Events Signal The MCDs Thread, which acts as needed.
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
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/
RC StartRecord (FUNCTION_PARM_BLOCK *pFuncBlock)
{
  ULONG         ulrc;
  ULONG         lCnt;
  ULONG         ulParam1;
  ULONG         ulErr;
  ULONG         ulSpiFlags;
  LONG          lBogus;             // Temporary Stuff
  LONG          lBytesRead;
  USHORT        usRecdToNotify;
  INSTANCE      * ulpInstance;
  ULONG         ulHoldEvent;

   ulrc = ulErr = MCIERR_SUCCESS;
   usRecdToNotify = FALSE;
   ulpInstance = (INSTANCE *) pFuncBlock->ulpInstance;
   ulParam1 = pFuncBlock->ulParam1;


   /***********************************
   * Reset and Wait on Sem
   ************************************/
   DosResetEventSem (ulpInstance->hEventSem, &lCnt);

   /************************
   * update state
   *************************/

   STRMSTATE = MCI_RECORD;
   /**************************
   * Element Exists is set
   ***************************/
   ulpInstance->usFileExists = TRUE;

  /*****************************
  * no one has issued an aborted
  * or superceded
  *****************************/
  ulpInstance->ulNotifyAborted = FALSE;


   /***********************
   * Start the stream
   ************************/
   ulrc = SpiStartStream (STREAM.hStream, SPI_START_STREAM);

   /*****************************************
    * Post The Thread Sem To release Notify
    * Thread Blocked on this sem.
   /***************************************/
   if (ulParam1 & MCI_NOTIFY)
       DosPostEventSem (ulpInstance->hThreadSem);


   if ( !ulrc )
      {
      /*******************************************
      *  Block this Thread For asynchronous
      *  Events. (Event Proc Releases this block)
      *******************************************/
      DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);

      /*******************************************************
      * Disable Thread Switching. The indefinite blockage of
      * this thread is carried out by waiting on the event
      * semaphore. The blocked thread will continue to run
      * from this point (DosEnterCritSec()) once the event
      * semaphore is posted asynchronously in response to
      * a streaming event.
      *******************************************************/
      DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
      DosEnterCritSec();

      if (ulpInstance->StreamEvent == EVENT_EOS) {
        ulrc = MCI_NOTIFY_ABORTED;
        ulpInstance->usRecdOK = TRUE;
      }
      else
       if (ulpInstance->StreamEvent == EVENT_ERROR)
           ulErr = STREAM.Evcb.evcb.ulStatus;

      /**********************************
      * Disable Notify Pending Flag
      ***********************************/

      if (ulpInstance->usNotifyPending == TRUE)
        ulpInstance->usNotifyPending =FALSE;

      /*****************************
      * Ensure that if we are being aborted
      * by another thread, and we received
      * an event that we ignore the event
      * and let the other process post the
      * notify
      ******************************/
      if (ulpInstance->ulNotifyAborted == TRUE)
         {
         ulpInstance->StreamEvent = EVENT_STREAM_STOPPED;
         }

      /************************************
      * Disable Record To Flag
      ************************************/
      if (ulpInstance->PlayTo == TRUE)
        {

        ulpInstance->PlayTo = FALSE;
        usRecdToNotify = TRUE;       // To Event Occurred
        if ( !ulErr )
           {
           ulErr = MCI_NOTIFY_SUCCESSFUL;
           }

        }

      /*******************************
      * Resume Normal Tasking
      *******************************/
      DosExitCritSec();
      DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

      } /* if no error on a record */
   else
      {
      /**********************************
      * Disable Notify Pending Flag
      ***********************************/

      if (ulpInstance->usNotifyPending == TRUE)
        ulpInstance->usNotifyPending =FALSE;

      if ( ulParam1 & MCI_NOTIFY )
         {
         PostMDMMessage (ulrc, MCI_RECORD, pFuncBlock);
         }

      return ulrc;
      }

   /********************************
   * Disable Position Advise
   *********************************/
   if (ulpInstance->usPosAdvise == TRUE)
       ulrc = SpiDisableEvent(STREAM.hPosEvent);



   /************************************
   * Flush stop The stream before the insert
   * to prevent errors below
   ************************************/
   if (usRecdToNotify == TRUE)
      {
      DosResetEventSem (ulpInstance->hEventSem, &lCnt);
      /************************************
      * Store the event since the stop will
      * override it
      ************************************/

      ulHoldEvent = ulpInstance->StreamEvent;


      /*******************************************************
      * If we are in a playlist and we get the end of playlist
      * error (similar to EOS), do a stop discard to stop
      * the audio stream handler
      *******************************************************/

      if ( ( ulpInstance->usPlayLstStrm == TRUE &&
           ulHoldEvent == EVENT_EOS ) ||
           ( ulHoldEvent == EVENT_ERROR ) ||
           ( ulHoldEvent == EVENT_STREAM_STOPPED ) )
         {
         ulSpiFlags = SPI_STOP_DISCARD;
         }
      else
         {
         ulSpiFlags = SPI_STOP_FLUSH;
         }
      ulrc = SpiStopStream (STREAM.hStream, ulSpiFlags );

      if (!ulrc)
         {
         DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
         }
      ulrc = MCIERR_SUCCESS;

      } /* flush stop the stream */


   // if we received an event error, stop the stream just in case

   if ( ulpInstance->StreamEvent == EVENT_ERROR )
      {
      /***********************************
      * Reset and Wait on Sem
      ************************************/
      DosResetEventSem (ulpInstance->hEventSem, &lCnt);

      ulHoldEvent = ulpInstance->StreamEvent;
      ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD );

      if (!ulrc)
         {
         DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
         }
      ulrc = MCIERR_SUCCESS;
      ulpInstance->StreamEvent = ulHoldEvent;

      }

   if ( ulpInstance->usPlayLstStrm != TRUE )
      {
      if ( ulpInstance->usRecdInsert )
        {

        ulrc = mmioSendMessage( ulpInstance->hmmio,
                                MMIOM_ENDINSERT,
                                0,
                                0);

        if (ulrc)
          {
          ulErr = MCIERR_DRIVER_INTERNAL;
          }

        /***************************************
        * Ensure that it no other record inserts
        * will be done unless specified
        ****************************************/

        ulpInstance->usRecdInsert = FALSE;

        }
      }

   /**************************************
   * Propogate Disk Full Error
   ***************************************/
   if (ulErr == MMIOERR_CANNOTWRITE)
      {
      ulErr = MCIERR_TARGET_DEVICE_FULL;
      }

   if ( ulpInstance->usPlayLstStrm != TRUE )
      {
      /***********************************
      * Set information for the header
      ************************************/
      WAVEHDR.usFormatTag = AMPMIX.sMode;
      WAVEHDR.usChannels = AMPMIX.sChannels;
      WAVEHDR.ulSamplesPerSec = AMPMIX.lSRate;
      XWAVHDR.ulAudioLengthInMS = 0;
      WAVEHDR.usBitsPerSample = ( USHORT ) AMPMIX.lBitsPerSRate;
      WAVEHDR.usBlockAlign = ( USHORT ) AMPMIX.ulBlockAlignment;

      /*************************************************
      * Tell the current IO Proc to clean up the header
      **************************************************/
      ulrc = mmioSetHeader( ulpInstance->hmmio,
                            &ulpInstance->mmAudioHeader,
                            sizeof( MMAUDIOHEADER ),
                            &lBogus,
                            0,
                            0 );

      if (ulrc)
         ulErr = (MCIERR_DRIVER_INTERNAL);

      /***********************************************
      * Figure out what the new size of the file is
      ************************************************/
      ulrc = mmioGetHeader ( ulpInstance->hmmio,
                             (PVOID) &(ulpInstance->mmAudioHeader),
                             sizeof( MMAUDIOHEADER ),
                             &lBytesRead,
                             (ULONG)NULL,
                             (ULONG)NULL );

      if (ulrc != MMIO_SUCCESS )
         ulErr = MCIERR_DRIVER_INTERNAL;

      // update our header size

      ulpInstance->mmckinfo.ckSize = XWAVHDR.ulAudioLengthInBytes;

      } /* update header if !playlist stream */



   /*****************************************
   * Handle posting of notifies for record to
   *****************************************/

   if (usRecdToNotify == TRUE)
      {
      /************************************
      * If an error occured, the stop will
      * override  the value we had previously
      * so restore it
      ************************************/
      ulpInstance->StreamEvent = ulHoldEvent;

      /************************************
      * We only want to post success if the
      * stream hit the cue time pause
      ************************************/
      if (ulParam1 & MCI_NOTIFY && ulHoldEvent == EVENT_CUE_TIME_PAUSE )
           {
           PostMDMMessage (ulErr, MCI_RECORD, pFuncBlock);
           }

      if (ulParam1 & MCI_NOTIFY && ulHoldEvent == EVENT_EOS )
           {
           PostMDMMessage ( MCIERR_TARGET_DEVICE_FULL, MCI_RECORD, pFuncBlock);

           // Ensure that the normal notify won't process the message
           ulpInstance->StreamEvent = EVENT_STREAM_STOPPED;

           }


      ulrc = SpiDisableEvent(STREAM.hPlayToEvent);

      } /* Record Till XX with Notify On */

    /*****************************************/
    // Post The Notification only after
    // File is saved in case of To
    /*****************************************/
    if (ulpInstance->usWaitPending == TRUE) {
        ulpInstance->usWaitPending = FALSE;
        STRMSTATE = MCI_STOP;
        return (ulErr);
    }

    /***********************************/
    // Normal Notifies
    /***********************************/
    if (ulParam1 & MCI_NOTIFY) {
        ulpInstance->usNotifyPending =FALSE;
        if ((ulpInstance->StreamEvent == EVENT_EOS) ||
           (ulpInstance->StreamEvent == EVENT_ERROR))
           {
           /*******************************************
           * Post The Notification Message for Record
           ********************************************/
           PostMDMMessage (ulErr, MCI_RECORD, pFuncBlock);
           }

         /********************************************
         * Update Internal States to stop
         *********************************************/
         STRMSTATE = MCI_STOP;

    }   /* Notify is On */


    /**************************************************
    * Post The Semaphore to clear any Blocked threads
    **************************************************/
    DosPostEventSem (ulpInstance->hThreadSem);

    /**********************************************
    * Post Any Other Error Notifications if Notify
    **********************************************/
    if (ulParam1 & MCI_NOTIFY) {

       /********************************
       * Release Thread Block Memory
       *********************************/
       ulrc = CleanUp ((PVOID)pFuncBlock);
    }

    return (ulErr);
}
