/*static char *SCCSID = "@(#)admcplay.c	13.16 92/04/30";*/
/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: ADMCPLAY.C
*
* DESCRIPTIVE NAME: Audio MCD WaveAudio Playback Function.
*
* FUNCTION:Play an Waveform Audio Element.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_PLAY message.
*        MCI_FROM Flag
*        MCI_TO   Flag
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
*
* INTERNAL REFERENCES:   CreateNAssocStream ().
*                        DestroyStream ().
*                        AssocMemPlayToAudioStream ().
*                        PostMDMMessage ().
*                        DoTillEvent().
*                        ConvertTimeUnits ().
*                        StartPlay().
*                        PlEventProc().
*                        SetAudioDevice().
*                        InitAudioDevice().
*                        SetWaveDeviceDefaults().
*                        OpenFile().
*                        CheckMem ().
*
* EXTERNAL REFERENCES:   DosResetEventSem ()        - OS/2 API
*                        DosPostEventSem  ()        - OS/2 API
*                        DosCreateThread  ()        - OS/2 API
*                        SpiEnableEvent   ()        - MME API
*                        SpiStopStream    ()        - MME API
*                        SpiCreateStream  ()        - MME API
*                        SpiAssociate     ()        - MME API
*                        SpiSeekStream    ()        - MME API
*                        mdmDriverNotify  ()        - MME API
*                        mmioGetHeader    ()        - MME API
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
* SUBROUTINE NAME: ADMCPLAY.C
*
* DESCRIPTIVE NAME: Waveform Play Routine.
*
* FUNCTION: Play an Waveform File.
*
* NOTES:  hStream[1] = A --> B = Playback stream.
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
* INTERNAL REFERENCES:   ADMCERR (). CreateNAssocStream ().
*
* EXTERNAL REFERENCES:  spiStartStream ()    - SSM  Spi
*                       spiStopStream  ()    - SSM  Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIPlay (FUNCTION_PARM_BLOCK *pFuncBlock)

{

   ULONG         ulParam1;           // Flags for this Msg
   ULONG         ulParam2;           // Data for this Msg
   ULONG         ulrc;               // Error Code Propogated
   ULONG         lCnt;               // Number Of Posts
   ULONG         ulForceInit = FALSE;// force the card to be reinited
                                     // needed between destroys of similar streams
   ULONG         ulAbortNotify  = FALSE;    // whether or not to abort notify's

   INSTANCE      *ulpInstance;       // Local Instance
   DWORD         dwSeekPoint;        // Seek Point
   ULONG         ulAssocFlag;        // Associate or Skip
   DWORD         dwSpiSeekFlags;     // Spi Seek Flags;
   DWORD         dwMMFileLength = 0; // Length of the File in MMTIME
   DWORD         dwTemp1 = 0;        // Scratch For z Time Conversion
   DWORD         dwTemp2 = 0;        // Scratch for Play From
   DWORD         dwTempTO = 0;       // Scratch for Play Till
   DWORD         dwSetAll;           // Audio Device Flags
   USHORT        usNotifyType;       // Notification Code
   USHORT        usEvcbIndex;        // loop Counter
   USHORT        usAbortType;
   LPMCI_PLAY_PARMS   lpPlayParms;   // Msg Data Ptr
   MCI_WAVE_SET_PARMS lpSetParms;    // Related Msg data Ptr


   /**************************************
   * Intialize Variables on Stack
   **************************************/
   ulrc = MCIERR_SUCCESS;
   dwSeekPoint = 0;
   usNotifyType = 0;
   usEvcbIndex = 0;
   dwSpiSeekFlags = SPI_SEEK_ABSOLUTE;
   ulParam1 = pFuncBlock->ulParam1;

   dwSetAll = MCI_WAVE_SET_BITSPERSAMPLE| MCI_WAVE_SET_FORMATTAG |
              MCI_WAVE_SET_CHANNELS | MCI_WAVE_SET_SAMPLESPERSEC;

   /**************************************
   * Derefernce Pointers
   **************************************/
   ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;
   ulParam2 =  pFuncBlock->ulParam2;

   /*******************************
   * Check If This a valid Instance
   *******************************/
   if (ulpInstance == (ULONG)NULL )
       return MCIERR_INSTANCE_INACTIVE;

   /*********************************
   * Process Pending Notifies
   **********************************/

   DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
   if (ulpInstance->usNotifyPending == TRUE)
      {
      ulpInstance->ulNotifyAborted = TRUE;
      ulpInstance->usNotifyPending = FALSE;
      ulAbortNotify = TRUE;
      }
   DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

   if ( ulAbortNotify == TRUE)
       {

       if (ulpInstance->usNotPendingMsg == MCI_PLAY) {

           if ( ulParam1 & MCI_NOTIFY )
              {
              usAbortType = MCI_NOTIFY_SUPERSEDED;
              }
           else
              {
              usAbortType = MCI_NOTIFY_ABORTED;
              }

           PostMDMMessage (usAbortType, MCI_PLAY,

                          pFuncBlock);

           /*******************************************/
           // Reset UserParm for Current Play
           /*******************************************/
           if (ulParam1 & MCI_NOTIFY)
               ulpInstance->usUserParm = pFuncBlock->usUserParm;
           /**************************************
           * Reset Internal Semaphores Used
           ****************************************/
           DosResetEventSem (ulpInstance->hEventSem, &lCnt);
           DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

           /**************************************
           * Stop The Stream (Discard buffers)
           **************************************/
           ulrc = SpiStopStream (STREAM.hStream,
                                 SPI_STOP_DISCARD);
           if (!ulrc)
             {
             /*****************************************
             * Wait for Previous Thread to Die
             *****************************************/
             DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
             }

           STRMSTATE = MCI_STOP;

           ulrc = MCIERR_SUCCESS;
           /***********************************
           * Update Internal States
           ************************************/
           ulpInstance->ulCreateFlag = PREROLL_STATE;

       }   /* Case of a Play Superseeding a Play */

       else
        if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
           {
           // Save is a non-interruptible operation
           // wait for completion

           DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

           }
       else
       {              /* The Pending Msg was not a Play */

           PostMDMMessage (MCI_NOTIFY_ABORTED,
                           MCI_RECORD, pFuncBlock);

           if (ulParam1 & MCI_NOTIFY)
                   ulpInstance->usUserParm = pFuncBlock->usUserParm;
           /***********************************************/
           // Reset Internal Semaphores
           /***********************************************/
           DosResetEventSem (ulpInstance->hEventSem, &lCnt);
           DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

           ulrc = SpiStopStream (STREAM.hStream,
                                 SPI_STOP_FLUSH);
           if (!ulrc)
             {
             /*************************************************
             * Wait for Record thread to complete
             *************************************************/
             DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
             }

           ulrc = MCIERR_SUCCESS;

           ulpInstance->usFileExists = TRUE;

           if ( !ulpInstance->ulOldStreamPos )
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
           ulpInstance->ulCreateFlag = PLAY_STREAM;
           STRMSTATE = MCI_STOP;

           /*************************************************
           * Ensure that cuepoints will be remembered across
           * record/play streams
           *************************************************/

       } /* else Aborted */
      }   /* Notify Pending  */

  else

     {
        ulrc = MCIERR_SUCCESS;

     }



   /**************************************************/
   // transition from Stop Recd State To PLAY
   /***************************************************/

   if (AMPMIX.ulOperation == OPERATION_RECORD)
       {
       DosResetEventSem (ulpInstance->hEventSem, &lCnt);
       DosResetEventSem (ulpInstance->hThreadSem, &lCnt);


       /*******************************
       * Destroy the stream
       ********************************/

       if ( !ulpInstance->ulOldStreamPos )
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
       ulpInstance->ulCreateFlag = CREATE_STATE;

       }      /* Transition from Recd State */



   DosResetEventSem (ulpInstance->hEventSem, &lCnt);

   /*************************************************
   * Check For validity of Data Passed in From MCI
   **************************************************/

   lpPlayParms= (LPMCI_PLAY_PARMS )pFuncBlock->ulParam2;

   if (STRMSTATE == CUERECD_STATE)
       ulpInstance->ulCreateFlag = CREATE_STATE;



   AMPMIX.ulOperation = OPERATION_PLAY;


   if (STRMSTATE == CUERECD_STATE) {

       SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);
       if ( !ulpInstance->ulOldStreamPos )
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

       // since we are switching from record to play states, we should
       // remove deny write access from the file so others can play back

       if ( !ulpInstance->ulUsingTemp && !ulpInstance->mmioHndlPrvd )
          {
          ulrc = mmioClose (ulpInstance->hmmio, 0);
          ulrc = OpenFile (ulpInstance, MMIO_READ| MMIO_DENYNONE);

          // if the open failed, we better tell the user

          if ( !ulrc )
            {
            ulpInstance->usFileExists = FALSE;
            return ( MCIERR_DRIVER_INTERNAL );
            }

          }

       ulpInstance->ulCreateFlag = CREATE_STATE;

   }  /* Cue Recd State to Play State Transition */


   /***********************************************
   * If a set was performed on an existing stream,
   * destroy the stream and get new spcb keys
   ***********************************************/
   if ( STRMSTATE == STREAM_SET_STATE )
      {
      DestroyStream (STREAM.hStream);

      ulpInstance->ulCreateFlag = CREATE_STATE;

      }


   if (ulpInstance->ulCreateFlag != PREROLL_STATE)
       {

       if (ulpInstance->usPlayLstStrm == TRUE)
          {
          ulAssocFlag = (ULONG)NULL;
          }
       else
           ulAssocFlag = PLAY_STREAM;

       /*************************************************
       * Always Reinit The Device before stream creation.
       *************************************************/

       ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY);

       if (ulrc)
          {
          return ulrc;
          }

       SetAmpDefaults(ulpInstance);

       AMPMIX.ulOperation = OPERATION_PLAY;

       VSDInstToWaveSetParms (&lpSetParms,ulpInstance);
       if (!ulrc)
         {
         ulrc = SetAudioDevice (ulpInstance,
                                (LPMCI_WAVE_SET_PARMS)&lpSetParms,
                                dwSetAll );
         }

       if ( ulrc )
         {
         return ( ulrc );
         }

       /*************************************************
       * If the init caused a new global sys file, use it
       *************************************************/

       STREAM.AudioDCB.ulSysFileNum = AMPMIX.ulGlobalFile;

       /***********************************************
       * Create a Stream with A = Src & B = Tgt
       ***********************************************/
       ulrc = CreateNAssocStream (STREAM.hidASource,
                                  STREAM.hidBTarget,
                                  &(STREAM.hStream),
                                  (INSTANCE *) ulpInstance,
                                  (ULONG)ulAssocFlag,
                                  (PEVFN)PlEventProc);
       if (ulrc)
           return (MCIERR_DRIVER_INTERNAL);

       /*****************************************************
       * In Case of PlayList Do the Associate Seperately
       *****************************************************/
       if (ulAssocFlag == (ULONG)NULL)     // Do the Associate Seperately
          {
          ulrc = AssocMemPlayToAudioStrm (ulpInstance, PLAY_STREAM);
          }

       /*******************************************
       * Stream is destroyed. Reset cuepopint and
       * position advise flags to enabled state
       * and send it down on the new stream
       ********************************************/
       if (ulpInstance->usPosAdvise == EVENT_ENABLED)
           ulpInstance->usPosAdvise = TRUE;

       if (ulpInstance->usCuePt == EVENT_ENABLED)
           ulpInstance->usCuePt = TRUE;

       // If we previously destroyed a stream, seek to the position where we were
       // else just seek to 0

       if ( !(ulParam1 & MCI_FROM ) )
          {
          ulrc = SpiSeekStream ( STREAM.hStream,
                                 SPI_SEEK_ABSOLUTE,
                                ulpInstance->ulOldStreamPos );

          if (ulrc)
            {
            // it is possible that the card may report a position that is
            // greater than what is actually recorded, so work around this
            // problem

            if ( ulrc == ERROR_SEEK_PAST_END )
               {
               ulrc = SpiSeekStream ( STREAM.hStream,
                                     SPI_SEEK_FROMEND,
                                     0 );

               if ( ulrc )
                  {
                  return ( ulrc );
                  }
               }
            else
               {
               return (ulrc);
               }
            }
          }

       ulpInstance->ulOldStreamPos = 0;
       ulForceInit = TRUE;

       /************************************
       * place the stream in a stopped state
       * since no activity has occurred
       *************************************/

       STRMSTATE = MCI_STOP;

       }  /* PreRoll State */

   /***************************
   * Set Create Flag
   ****************************/
   ulpInstance->ulCreateFlag = PREROLL_STATE;

   /*********************************************
   * Calculate the Media length
   **********************************************/

      ConvertTimeUnits ( ulpInstance, (DWORD*)& (dwMMFileLength),
                         FILE_LENGTH);


      dwTemp1 = dwMMFileLength;

      /***********************************
      * Do a Seek to support FROM
      ***********************************/
      if (ulParam1 & MCI_FROM)
         {
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



           ulrc = ConvertToMM ( ulpInstance, &(dwTemp2),
                                (DWORD)lpPlayParms->dwFrom);


           /*********************************
           * Do the Seek Thing
           **********************************/
           if ( !ulrc )
              {
              ulrc = SpiSeekStream ( STREAM.hStream,
                                     dwSpiSeekFlags,
                                     dwTemp2
                                     + ulpInstance->ulSyncOffset );

              if ( ulrc )
                 {
                 return ( ulrc ) ;
                 }
              }
         } /* Play From */

      if (ulParam1 & MCI_TO)
         {

            ulrc = ConvertToMM ( ulpInstance, (DWORD*)&(dwTempTO),
                                 (DWORD)lpPlayParms->dwTo);

            if (!ulrc)
               ulrc = DoTillEvent (ulpInstance, dwTempTO);

            ulpInstance->PlayTo = TRUE;

         } // Of Play Till XXXX


   /*********************************************
   * UpDate Internal State Variables and FLAGS
   **********************************************/
   ulpInstance->ulOperation = OPERATION_PLAY;
   ulpInstance->ulCreateFlag = PREROLL_STATE;
   STRMSTATE = MCI_PLAY;

   /***************************************
   * Enable Position Advise if Needed
   ****************************************/
   if (!ulrc)
      {
      if (ulpInstance->usPosAdvise == TRUE)
         {

         /*********************************
         * Correct The hStream Value
         **********************************/
         STREAM.PosAdvEvcb.evcb.hstream = STREAM.hStream;

         /**********************************************
         * Stick In INSTANCE Pointer in The Time EVCB
         ***********************************************/

         STREAM.PosAdvEvcb.ulpInstance = (ULONG)ulpInstance;

         /**************************************************************
         * Update position advise cuepoints created in different stream
         **************************************************************/

         STREAM.PosAdvEvcb.evcb.mmtimeStream = STREAM.PosAdvEvcb.mmCuePt;
         ulrc = SpiEnableEvent( (PEVCB) &(STREAM.PosAdvEvcb),
                                 (PHEVENT) &(STREAM.hPosEvent));

         ulpInstance->usPosAdvise = EVENT_ENABLED;

          }  /* PosnAdvise */
      } // No RC

   /*******************************
   * Enable Cue points if any
   *******************************/
   if (!ulrc)
      {
      if (ulpInstance->usCuePt == TRUE)

         {
         /*********************************
         * Correct The hStream Value
         ***********************************/
         for (usEvcbIndex = 0; usEvcbIndex < CUEPTINDX; usEvcbIndex ++)
           {
           CUEPTEVCB[usEvcbIndex].evcb.hstream = STREAM.hStream;

           /*********************************************
           * Stick In INSTANCE Pointer in The Time EVCB
           **********************************************/
           CUEPTEVCB[usEvcbIndex].ulpInstance = (ULONG)ulpInstance;

           ulrc = SpiEnableEvent( (PEVCB) &(CUEPTEVCB[usEvcbIndex]),
                                  (PHEVENT) &(STREAM.HCuePtHndl[usEvcbIndex]));
           } /* For loop */

         ulpInstance->usCuePt = EVENT_ENABLED;   // disable CuePoints
         } /* There are CuePts to be enabled */
      } /* No Return Code */

   if (ulParam1 & MCI_NOTIFY) {
       ulpInstance->usNotifyPending = TRUE;
       ulpInstance->usNotPendingMsg = MCI_PLAY;
   }

   /*************************************************
   * Stick In Instance PTR in EVCB
   *************************************************/
   STREAM.Evcb.ulpInstance = (ULONG)ulpInstance;
   STRMSTATE = MCI_PLAY;


   /************************************************
   * If Notify Create a Thread to do the start
   ************************************************/
   if (!ulrc) {
       if (ulParam1 & MCI_NOTIFY) {
           ulpInstance->usNotifyPending = TRUE;
           ulpInstance->usNotPendingMsg = MCI_PLAY;
          /****************************************************
          * This thread is kicked off by the MCD mainly
          * to start the stream. Minimum processing should
          * be done on the SSMs eventThread . The eventProc
          * signalls This thread via semaphores and
          * acts accordingly
          ****************************************************/

           DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

           ulrc = DosCreateThread ((PTID)&(pFuncBlock->pInstance->RecdThreadID),
                                   (PFNTHREAD)StartPlay,
                                   (ULONG) pFuncBlock,
                                   (ULONG) 0L,
                                   (ULONG)NOTIFY_THREAD_STACKSIZE);

           /*************************************************
           * Wait for the Play thread to do the start.
           **************************************************/
           if (!ulrc)
               DosWaitEventSem (ulpInstance->hThreadSem, -1);
       }
       else
       {
           ulpInstance->usWaitPending = TRUE;
           ulpInstance->usWaitMsg= MCI_PLAY;
           ulrc = StartPlay (pFuncBlock);
       }

   /**********************************
   * Release data Access Semaphore
   ***********************************/

   MCD_ExitCrit (ulpInstance);

   } // No RC
   return (ULONG)(ulrc);
}




/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: EventProc.
*
* DESCRIPTIVE NAME: SSM Event Notifications Receiever.
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

RC APIENTRY PlEventProc ( MEVCB *lpevcb)
{
  MTIME_EVCB        *pMTimeEVCB;      // Modified Time EVCB
  ULONG             ulrc;             // Local RC
  INSTANCE          * ulpInstance;    // Current Instance
  HWND              hWnd;             // CallBack Handle
  DWORD             dwMMTime;         // Current MMtime


  ulrc = MCIERR_SUCCESS;
  dwMMTime = 0;

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
            ulrc = DosPostEventSem (ulpInstance->hEventSem);
           break;

       case EVENT_ERROR:


            /**************************************
            * End of PlayList event is received
            * as an implicit error event. It
            * is treated as a normal EOS
            ***************************************/

            if (ulpInstance->usPlayLstStrm == TRUE)
               {
               if ( lpevcb->evcb.ulStatus == MCIERR_CUEPOINT_LIMIT_REACHED )
                  {
                  mdmDriverNotify ((WORD)ulpInstance->wWaveDeviceID,
                                   (HWND)hWnd,
                                   MM_MCINOTIFY,
                                   (WORD)(lpevcb->evcb.unused1),
                                   (DWORD) MAKEULONG(MCI_PLAY, lpevcb->evcb.ulStatus));
                  }
               }
            else if (lpevcb->evcb.ulStatus != ERROR_DEVICE_UNDERRUN)
               {
               ulpInstance->StreamEvent = EVENT_ERROR;
               ulrc = DosPostEventSem (ulpInstance->hEventSem);
               }
            else
               {
               mdmDriverNotify ((WORD)ulpInstance->wWaveDeviceID,
                                (HWND)hWnd,
                                MM_MCINOTIFY,
                                (WORD)(lpevcb->evcb.unused1),
                                (DWORD) MAKEULONG(MCI_PLAY, lpevcb->evcb.ulStatus));
               }

           break;

       case EVENT_STREAM_STOPPED:
            ulpInstance->StreamEvent = EVENT_STREAM_STOPPED;
            ulrc = DosPostEventSem (ulpInstance->hEventSem);
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
                             ulpInstance->dwOpenCallBack,
                             MM_MCIPLAYLISTMESSAGE,
                             (WORD) MAKEULONG(lpevcb->evcb.ulStatus, ulpInstance->wWaveDeviceID),
                             (DWORD)lpevcb->evcb.unused1);
           break;

       case EVENT_PLAYLISTCUEPOINT:
            mdmDriverNotify ((WORD)ulpInstance->wWaveDeviceID,
                             ulpInstance->dwOpenCallBack,
                             MM_MCICUEPOINT,
                             (WORD) MAKEULONG(lpevcb->evcb.ulStatus, ulpInstance->wWaveDeviceID),
                             (DWORD)lpevcb->evcb.unused1);
           break;


       } /* SubType case of Implicit Events */
      break;

  case EVENT_CUE_DATA:
          break;
  case EVENT_CUE_TIME_PAUSE:
       {
       /***************************************************
       * This event will arrive if we recorded to a certain
       * position in the stream
       ****************************************************/

       pMTimeEVCB = (MTIME_EVCB *)lpevcb;
       ulpInstance = (INSTANCE *)pMTimeEVCB->ulpInstance;
       ulpInstance->StreamEvent = EVENT_CUE_TIME_PAUSE;

       DosPostEventSem (ulpInstance->hEventSem);
       }
       break;

  case EVENT_CUE_TIME:
       {
       pMTimeEVCB  = (MTIME_EVCB *)lpevcb;
       ulpInstance = (INSTANCE *)pMTimeEVCB->ulpInstance;

       /*************************************************
       * Single Events are Treated as Time Cue Points
       **************************************************/
       if ( pMTimeEVCB->evcb.ulFlags == EVENT_SINGLE)
           {
           mdmDriverNotify ( (WORD)ulpInstance->wWaveDeviceID,
                             (HWND)pMTimeEVCB->dwCallback,
                             MM_MCICUEPOINT,
                             (WORD)pMTimeEVCB->usCueUsrParm,
                             (DWORD)pMTimeEVCB->evcb.mmtimeStream);
           }

       /************************************************
       * Recurring Events Are Media Position Changes
       ************************************************/
       if (pMTimeEVCB->evcb.ulFlags == EVENT_RECURRING)
          {
          mdmDriverNotify ( ulpInstance->wWaveDeviceID,
                            (HWND)POSEVCB.dwCallback,
                            MM_MCIPOSITIONCHANGE,
                            (WORD)ulpInstance->usPosUserParm,
                            ( DWORD)pMTimeEVCB->evcb.mmtimeStream);
          } /* Event Cue Time */

       }
       break;
  default: break;

  }          /* All Events case */

  return (ULONG)(MCIERR_SUCCESS);

  }             /* of Event Handler */




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: StartPlay
*
* DESCRIPTIVE NAME:Start Play
*
* FUNCTION: SpiStartStream().
*
*
* NOTES: This routine is called using app's wait thread (Wait Case)
*        or a separate thread spawned by MCD on MCI Notify. SSMs
*        asynch Events Signal The MCDs Thread, which acts as needed.
*        When a streaming operation is interuptted (usually by a stop)
*        the interuptting thread waits for the MCDs thread to complete
*        its remaing tasks. This wait is controlled via the instance based
*        thread semaphore.
*
*        Further, on a notify the Play Notify command does not return
*        until the newly created thread is ready to block itself. This
*        ensures that any other MCI messages that are free to be intercepted
*        following the MCI_PLAY message operate on a running stream.
*        This also means there is minimum latency between the return of the
*        Play command to the application and start of audible sound.
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
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/

RC   StartPlay (FUNCTION_PARM_BLOCK * pFuncBlock)
{

  ULONG         ulrc;
  ULONG         lCnt;
  ULONG         ulParam1;
  USHORT        usPlayToNotify;
  INSTANCE      * ulpInstance;
  ULONG         ulErr;
  ULONG         ulHoldEvent;



  ulrc = MCIERR_SUCCESS;
  ulErr = MCIERR_SUCCESS;
  ulpInstance = (INSTANCE *) pFuncBlock->ulpInstance;
  ulParam1 = pFuncBlock->ulParam1;
  usPlayToNotify = FALSE;




  /***********************************
  * Reset the event semaphore
  ************************************/
  DosResetEventSem (ulpInstance->hEventSem, &lCnt);


  /*********************
  * update state
  *********************/
  STRMSTATE = MCI_PLAY;

  /*****************************
  * no one has issued an aborted
  * or superceded
  *****************************/
  ulpInstance->ulNotifyAborted = FALSE;

  /****************************
  * Start Playing the Stream.
  *****************************/
  ulrc = SpiStartStream (STREAM.hStream, SPI_START_STREAM);

  /* if we had trouble starting the stream report an error */

  if ( ulrc )
     {
     /**********************************
     * Disable Notify Pending Flag
     ***********************************/

     if (ulpInstance->usNotifyPending == TRUE)
        {
        ulpInstance->usNotifyPending =FALSE;
        }

     if ( ulParam1 & MCI_NOTIFY )
        {
        PostMDMMessage (ulrc, MCI_PLAY, pFuncBlock);
        }

     return ulrc;
     }

  /*****************************************
   * Post The Thread Sem To release Notify
   * Thread Blocked on this sem.
  /***************************************/
  if (ulParam1 & MCI_NOTIFY)
      DosPostEventSem (ulpInstance->hThreadSem);


  /*******************************************
  *  Block this Thread For asynchronous
  *  Events. (Event Proc Releases this block)
  *******************************************/
  DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);

  /*******************************************************
   * Disable thread switching. The indefinite blockage of
   * this thread is carried out by waiting on the event
   * semaphore. The blocked thread will continue to run
   * from this point (DosEnterCritSec()) once the event
   * semaphore is posted asynchronously in response to
   * a streaming event.
  *******************************************************/
  DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
  DosEnterCritSec();

  if (ulpInstance->StreamEvent == EVENT_EOS)
      ulErr = MCI_NOTIFY_SUCCESSFUL;
  else
      if (ulpInstance->StreamEvent == EVENT_ERROR)
          ulErr = STREAM.Evcb.evcb.ulStatus;

  /*****************************
  * Turn OFF Notifies
  ******************************/
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

  if (ulpInstance->PlayTo == TRUE) {

      ulpInstance->PlayTo = FALSE;
      usPlayToNotify = TRUE;   // To Event Occurred
  }

  /****************************
  * Resume Normal Tasking
  ****************************/
  DosExitCritSec ();
  DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

  /******************************
  * Disable Position Advise
  *******************************/
  if (ulpInstance->usPosAdvise == TRUE)
      {
      ulrc = SpiDisableEvent(STREAM.hPosEvent);
      }

  if (usPlayToNotify == TRUE)
      {
      ulrc = SpiDisableEvent(STREAM.hPlayToEvent);

      if (ulParam1 & MCI_NOTIFY)
         {
          if (ulpInstance->StreamEvent == EVENT_CUE_TIME_PAUSE )
              {
              PostMDMMessage (MCI_NOTIFY_SUCCESSFUL,
                              MCI_PLAY,
                              pFuncBlock);
              }
         }

      // Since we recieved the pause event, do a stop pause to place
      // stream in reasonable state

      SpiStopStream (STREAM.hStream, SPI_STOP_STREAM );


      }  /* Play To Notify */

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

  /***************************************
  * Release the Blocked Wait Thread
  * after updating  state at this point.
  ****************************************/
  if (ulpInstance->usWaitPending == TRUE) {
      ulpInstance->usWaitPending = FALSE;
      STRMSTATE = STOP_PAUSED;
      return MCIERR_SUCCESS;
  }

  /******************************
  * Regular Notifies
  ******************************/
  if (ulParam1 & MCI_NOTIFY)
      {
      if ((ulpInstance->StreamEvent == EVENT_EOS) ||
          (ulpInstance->StreamEvent == EVENT_ERROR))
          {
          PostMDMMessage (ulErr, MCI_PLAY, pFuncBlock);
          }

      STRMSTATE = STOP_PAUSED;

      } /* Notify On */

  /****************************************
  * Release Blocked Threads (if Any)
  ******************************************/
  DosPostEventSem (ulpInstance->hThreadSem);

  if (ulParam1 & MCI_NOTIFY) {

      /**********************************
      * Release Thread Block Memory
      ***********************************/
       ulrc = CleanUp ((PVOID)pFuncBlock);

  }
   return (MCIERR_SUCCESS);
  }
