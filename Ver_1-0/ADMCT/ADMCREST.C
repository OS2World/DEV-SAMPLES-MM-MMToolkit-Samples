/*static char *SCCSID = "@(#)admcrest.c	13.18 92/04/29";*/
/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCIREST.c
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
* INPUT:
*
*
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
*
* INTERNAL REFERENCES:   MCICUE, MCIPAUS, MCISEEK,
*                        MCISTOP, MCISTPA, MCISCPT
*                        MCICNCT
*                        CreateNAssocStream ().
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

#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_ERRORS

#include <os2.h>                        // OS2 defines.
#include <string.h>                     // String functions.
#include <os2medef.h>                   // MME includes files.
#include <audio.h>                      // Audio Device Defines.
#include <ssm.h>                        // SSM spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmsystem.h>                   // MM System Include.
#include <mcidrv.h>                     // MCI Driver Include.
#include <mmio.h>                       // MMIO Include.
#include <mcd.h>                        // AudioIFDriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions
#include <audiomcd.h>                   // Component Definitions
#include "admcfunc.h"                   // Function Prototypes

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCICUE..C
*
* DESCRIPTIVE NAME: Cue Waveform Device.
*
* FUNCTION: Prepare a Waveform Device or an Device Element for Record/Playback.
*
* NOTES: Cueing a Waveform Device translates to mean prerolling of streams
*        in the streaming context. It is presumed that MCI_OPEN precedes
*        MCI_CUE. The validity of this presumption can be checked by looking
*        at ulState. Preroll means source handler fills the buffers and is
*        to roll. The Target handlers are not started, meaning buffers are
*        are not consumed.
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_CUE message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: AudioIFDriverEntry()       - AudioDevice MCD
*
*********************** END OF SPECIFICATIONS **********************/
RC MCICue (FUNCTION_PARM_BLOCK *pFuncBlock)
{
  ULONG          ulrc;                 // Error Value
  ULONG          ulErr;                // Holds error values
  ULONG          ulParam1;             // Msg Flags
  ULONG          ulParam2;             // Msg data
  ULONG          ulAbortNotify = FALSE;// whether or not a action must be aborted
                                       // in case other thread overwrites it
  INSTANCE *     ulpInstance;          // Local Instance
  ULONG          lCnt;                 // Number of Posts
  DWORD          dwCueFlags;           // Mask For Incoming MCI Flags
  DWORD          dwStopFlags;          // Flush or Discard Stop.
  DWORD          dwSetAll;             // Set all flags
  LPMCI_GENERIC_PARMS lpCueParams;     // Msg data ptr
  MCI_WAVE_SET_PARMS  lpSetParms;      // MCI Wave Set Parms

  ULONG         ulAssocFlag = 0;

  /******************************
  * Derefernce pointers.
  ******************************/
  ulParam1 =    pFuncBlock->ulParam1;
  ulParam2 =    pFuncBlock->ulParam2;
  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;

  ulrc = MCIERR_SUCCESS;
  dwStopFlags = SPI_STOP_DISCARD;

  dwSetAll = MCI_WAVE_SET_BITSPERSAMPLE| MCI_WAVE_SET_FORMATTAG|
             MCI_WAVE_SET_CHANNELS| MCI_WAVE_SET_SAMPLESPERSEC;
  /******************************
  * Check for Invalid Flags
  *******************************/
  dwCueFlags = ulParam1;
  dwCueFlags &= ~MCI_NOTIFY;
  dwCueFlags &= ~MCI_WAIT;
  dwCueFlags &= ~(MCI_CUE_INPUT + MCI_CUE_OUTPUT +
                  MCI_WAVE_INPUT + MCI_WAVE_OUTPUT);
  /****************************************
  * Check for Invalid flags
  *****************************************/
  if (dwCueFlags > 0)
      return MCIERR_INVALID_FLAG;

  ulrc = CheckMem (((PVOID)ulParam2),
                   sizeof (MCI_GENERIC_PARMS), PAG_READ);

  if (ulrc == MCIERR_SUCCESS)
      lpCueParams = (LPMCI_GENERIC_PARMS)ulParam2;

  if (ulpInstance == (ULONG)NULL )
      return MCIERR_INSTANCE_INACTIVE;

  if (ulpInstance->ulInstanceSignature != ACTIVE)
      return MCIERR_INSTANCE_INACTIVE;

  /******************************************
  * Check for incompatible flags
  *******************************************/
  if (((ulParam1 & MCI_WAVE_INPUT)||(ulParam1 & MCI_CUE_INPUT)) &&
      ((ulParam1 & MCI_WAVE_OUTPUT) || (ulParam1 & MCI_CUE_OUTPUT)))

      return MCIERR_FLAGS_NOT_COMPATIBLE;

  /****************************************
  * If No Element return error
  *****************************************/

  if ( ( ulpInstance->usFileExists == FALSE) || (ulpInstance->usFileExists == UNUSED) )
     {
     return (MCIERR_FILE_NOT_FOUND);
     }

  /******************************************
  * If a temp file was not opened, then we
  * cannot use recording features
  *******************************************/

  if ( ( (ulParam1 & MCI_WAVE_INPUT)||(ulParam1 & MCI_CUE_INPUT) ) &&
       !ulpInstance->ulUsingTemp &&
       !ulpInstance->usPlayLstStrm )
    {
    return MCIERR_UNSUPPORTED_FUNCTION;
    }

  /******************************************
  * Check for Missing flags
  *******************************************/
  if (!(ulParam1 & MCI_WAVE_INPUT || ulParam1 & MCI_WAVE_OUTPUT ||
      ulParam1 & MCI_CUE_INPUT || ulParam1 & MCI_CUE_OUTPUT))
      {
      // don't return a missing flag, default to output

      ulParam1 &= MCI_WAVE_OUTPUT;

      }

  /**********************************
  * Open The Instance Event Sem
  ***********************************/
  ulrc = DosOpenEventSem ((PSZ)NULL,
                          (PHEV)&(ulpInstance->hEventSem));
  if (ulrc)
      return ulrc;


  /*************************************************
  * PlayList in Paused State an Exclusive state ??
  *************************************************/


  if (ulpInstance->usPlayLstStrm == TRUE )
     {

     // notifies will stop the stream on their own
     // we are only concerned with waits, and previously stopped
     // streams

     if ( STRMSTATE == MCI_PAUSE &&
          !(ulpInstance->usNotifyPending) )
       {

       DosResetEventSem (ulpInstance->hThreadSem, &lCnt);
       ulrc = SpiStopStream (STREAM.hStream,
                               SPI_STOP_DISCARD);
       if (!ulrc)
         {
         DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
         }
       }
     } /* PlayList Pause */

  /************************************
  *  Pause is an exclusive State
  **************************************/
  if (ulpInstance->usPlayLstStrm != TRUE )
      {
      if ((STRMSTATE == MCI_PAUSE  ||
           STRMSTATE == MCI_STOP   ||
           STRMSTATE == STOP_PAUSED ) &&

          (ulpInstance->ulCreateFlag == PREROLL_STATE )
          && !ulpInstance->usNotifyPending                     )
          {
          /***********************************************
          *  Discard Stop the Stream if in Paused State
          ***********************************************/
          DosResetEventSem (ulpInstance->hEventSem, &lCnt);
          ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);
          if (!ulrc)
              DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
          ulpInstance->ulCreateFlag = PREROLL_STATE;

          }   /* Exclusive States */
      }  /* Non PlayList */

  /**********************************************
  * Create and Cue The Playback Stream A --> B
  ***********************************************/
  if ((ulParam1 & MCI_CUE_OUTPUT) || (ulParam1 & MCI_WAVE_OUTPUT))
     {

     DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
     if (ulpInstance->usNotifyPending == TRUE)
        {
        ulpInstance->ulNotifyAborted = TRUE;
        ulpInstance->usNotifyPending = FALSE;
        ulAbortNotify = TRUE;
        }
     DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

     /*****************************************************
     * If a stream is currently in use, destroy or stop it!
     ******************************************************/
     if ( ulAbortNotify == TRUE)
        {
        if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
          {
          // Save is a non-interruptible operation
          // wait for completion

          DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

          }
        else
          {
          PostMDMMessage ( MCI_NOTIFY_ABORTED,
                           ulpInstance->usNotPendingMsg,
                           pFuncBlock );

          if ( ulpInstance->usNotPendingMsg == MCI_PLAY )
             {
             DosResetEventSem( ulpInstance->hEventSem, &lCnt );
             DosResetEventSem( ulpInstance->hThreadSem, &lCnt );
             ulrc = SpiStopStream( STREAM.hStream, SPI_STOP_DISCARD );
             if ( !ulrc )
                {
                DosWaitEventSem( ulpInstance->hThreadSem, -1 );

                // ***************************************
                // ***************************************
                // perform error checking HERE!!!!!!!!!!!!
                // ***************************************
                // ***************************************
                }
             } /* if this is play stream */

          else
             {
             /**************************************
             * Destroy The Previous Stream
             ***************************************/

             DestroyStream (STREAM.hStream);

             /***************************************
             * Update Create Stream Flag
             ****************************************/

             ulpInstance->ulCreateFlag = CREATE_STATE;

             } /* else this is a  record stream */



          } /* if !save */


        } /* if notify pending is true */
      else
        {
        } /* not notify pending */





      /*****************************
      * Set Operation Flag
      ******************************/
      AMPMIX.ulOperation = OPERATION_PLAY;

      /***********************************************
      * If a set was performed on an existing stream,
      * destroy the stream and get new spcb keys
      ***********************************************/
      if ( STRMSTATE == STREAM_SET_STATE )
        {
        DestroyStream (STREAM.hStream);

        // get new spcb key

        ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY );

        if (ulrc)
           {
           return ( ulrc );
           }

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

          /********************************************
          * Create and associate a playback stream
          * This stream has the file system as the
          * source stream handler and audio stream
          * handler as the target.
          *********************************************/
          ulrc = CreateNAssocStream (STREAM.hidASource,
                                     STREAM.hidBTarget,
                                     (HSTREAM *)&(STREAM.hStream),
                                     (INSTANCE *) ulpInstance,
                                     (ULONG)ulAssocFlag,
                                     (PEVFN)PlEventProc);
          if (ulrc)
              return ulrc;

          /*****************************************************
          * In Case of PlayList Do the Associate Seperately
          ******************************************************/
          if (ulAssocFlag == (ULONG)NULL)
              ulrc = AssocMemPlayToAudioStrm (ulpInstance,
                                              PLAY_STREAM);


         }     /* Play Back Stream */
      ulpInstance->ulCreateFlag = PREROLL_STATE;

     /*******************************
     * Preroll Start the stream.
     *******************************/
     DosResetEventSem (ulpInstance->hEventSem, &lCnt);

     if (STRMSTATE != CUEPLAY_STATE)
         {

         ulrc = SpiStartStream (STREAM.hStream,
                                SPI_START_PREROLL);

         if (ulrc)
             return ulrc;
         /*************************************
         * Wait till you Recieve PREROLL Event
         *************************************/
         DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);

         } /* Preroll Start */

     /*************************************
     * UpDate State To Preroll
     **************************************/
     STRMSTATE = CUEPLAY_STATE;
     ulpInstance->ulCreateFlag = PREROLL_STATE;

     } /* default case is wave output */

  /********************************************
  * Create and Cue The Record Stream B --> A
  *********************************************/

  if ((ulParam1 & MCI_CUE_INPUT) || (ulParam1 & MCI_WAVE_INPUT))

     {

     DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
     if (ulpInstance->usNotifyPending == TRUE)
        {
        ulpInstance->ulNotifyAborted = TRUE;
        ulpInstance->usNotifyPending = FALSE;
        ulAbortNotify = TRUE;
        }
     DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

     /*****************************************************
     * If a stream is currently in use, destroy or stop it!
     ******************************************************/
     if ( ulAbortNotify == TRUE)
        {
        if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
          {
          // Save is a non-interruptible operation
          // wait for completion

          DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

          }
        else
          {
          PostMDMMessage ( MCI_NOTIFY_ABORTED,
                           ulpInstance->usNotPendingMsg,
                           pFuncBlock );

          if ( ulpInstance->usNotPendingMsg == MCI_RECORD )

             {
             DosResetEventSem( ulpInstance->hEventSem, &lCnt );
             DosResetEventSem( ulpInstance->hThreadSem, &lCnt );
             ulrc = SpiStopStream( STREAM.hStream, SPI_STOP_FLUSH );
             if ( !ulrc )
                {
                DosWaitEventSem( ulpInstance->hThreadSem, ONE_MINUTE );

                // ***************************************
                // ***************************************
                // perform error checking HERE!!!!!!!!!!!!
                // ***************************************
                // ***************************************
                }
             } /* if this is record stream */

          else
             {
             /**************************************
             * Destroy The Previous Stream
             ***************************************/

             DestroyStream (STREAM.hStream);

             /***************************************
             * Update Create Stream Flag
             ****************************************/

             ulpInstance->ulCreateFlag = CREATE_STATE;

             } /* else this is a play stream */

           } /* notify pending was not a !save */


        } /* if notify pending is true */
      else
        {

        } /* not notify pending */


      if (ulpInstance->usPlayLstStrm != TRUE)
          {

          /********************************************
          * The Open has not yet been done (Record).
          *********************************************/

          if (ulpInstance->usFileExists == UNUSED)
              return (MCIERR_FILE_NOT_FOUND);

          if (ulpInstance->usFileExists == TRUE)
              {
              /************************************
              * Open with Write Flag ON
              ************************************/
              if ( !( ulpInstance->dwmmioOpenFlag & MMIO_EXCLUSIVE ) )
                  {
                  ulrc = mmioClose (ulpInstance->hmmio, 0);

                  ulpInstance->dwmmioOpenFlag = MMIO_READWRITE | MMIO_EXCLUSIVE;

                  ulErr = OpenFile ( ulpInstance, ulpInstance->dwmmioOpenFlag );

                  if ( ulErr )
                     {
                     ulpInstance->dwmmioOpenFlag = MMIO_READ | MMIO_DENYNONE;

                     ulrc = OpenFile ( ulpInstance, ulpInstance->dwmmioOpenFlag );
                     if ( ulrc )
                        {
                        ulpInstance->usFileExists = FALSE;
                        }

                     return ( ulErr );
                     }
                  }
              } /* File Exists */
          else
              {
              ulpInstance->dwmmioOpenFlag = MMIO_READWRITE | MMIO_CREATE | MMIO_EXCLUSIVE;

              ulrc = OpenFile (ulpInstance, ulpInstance->dwmmioOpenFlag );
              if (ulrc)
                  return (ulrc);
              }

          } /* Not PlayList */

      /**********************************************
      * Look for State Transition variables
      *  and change State accordingly (FSA Approach)
      ***********************************************/

      if ((STRMSTATE == CUEPLAY_STATE)|| (STRMSTATE == MCI_PLAY)||
          (STRMSTATE == MCI_PAUSE)) {
           /******************************************
           * Stop The Previous stream
           *******************************************/
           ulrc = SpiStopStream (STREAM.hStream,
                                 SPI_STOP_DISCARD);

           /**************************************
           * Destroy the previous stream
           ***************************************/
           DestroyStream (STREAM.hStream);

           /*******************************************
           * Set Stream Creation flag to create state
           ********************************************/
           ulpInstance->ulCreateFlag = CREATE_STATE;
      }


      /***********************************************
      * If a set was performed on an existing stream,
      * destroy the stream and get new spcb keys
      ***********************************************/
      if ( STRMSTATE == STREAM_SET_STATE )
        {
        DestroyStream (STREAM.hStream);

        // get new spcb key

        ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY );

        if (ulrc)
           {
           return ( ulrc );
           }
        ulpInstance->ulCreateFlag = CREATE_STATE;

        }

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

          /*****************************************************
          * Create The Stream from ADSH ---> FSSH
          ******************************************************/
          ulrc = CreateNAssocStream ( STREAM.hidBSource,
                                      STREAM.hidATarget,
                                      (HSTREAM *)&(STREAM.hStream),
                                      (INSTANCE *) ulpInstance,
                                      (ULONG) ulAssocFlag,
                                      (PEVFN)ReEventProc);
          if (ulrc)
              return ulrc;

          /*******************************************************
          * In Case of PlayList Do the Associate Seperately
          *******************************************************/
          if (ulAssocFlag == (ULONG)NULL)         // Do the Associate Seperately
              ulrc = AssocMemPlayToAudioStrm (ulpInstance,
                                              RECORD_STREAM);

      } /* Create Flag */

      ulpInstance->ulCreateFlag = PREROLL_STATE;
      /***********************************
      * Preroll start the Stream
      ************************************/

      DosResetEventSem (ulpInstance->hEventSem, &lCnt);

      if (STRMSTATE != CUERECD_STATE) {
          ulrc = SpiStartStream (STREAM.hStream,
                                 SPI_START_PREROLL);
          if (ulrc)
              return ulrc;
          /*************************************
          * Wait till you Recieve PREROLL Event
          *************************************/
          DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
      } /* preroll Start */

      /**************************************
      * UpDate State To Preroll
      **************************************/
      STRMSTATE = CUERECD_STATE;
      ulpInstance->ulCreateFlag = PREROLL_STATE;

      }   /* Cue Input */

  /****************************************
  * Close The Event Semaphore
  ****************************************/
  DosCloseEventSem (ulpInstance->hEventSem);

  return (ULONG)(ulrc);
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISCPT.C
*
* DESCRIPTIVE NAME: Set Cue Point Waveform Device.
*
* FUNCTION: Establish Cue points during playback on Waveform Device.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_SET_CUEPOINT message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiEnableEvent()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIScpt (FUNCTION_PARM_BLOCK *pFuncBlock)
{
  ULONG           ulrc;               // Propogated MME Error Code
  ULONG           ulParam1;           // Incoming MCI Flag
  ULONG           ulParam2;           // Incoming MCI Data PTR
  DWORD           dwCPTFlags;         // Mask For Incoming Flags
  INSTANCE        * ulpInstance;      // Local Instance
  HWND            hWnd;               // Call Back Window Hndl
  USHORT          usEvcbIndex;        // Local Index
  ULONG           ulTemp1;            // Conversion Var
  USHORT          usFound;            // Flag
  PID             pid;
  TID             tid;
  LPMCI_CUEPOINT_PARMS lpCueParms;    // Msg MCI Data Structure

  /****************************
  * Intialize Local Vars
  *****************************/
  ulrc = MCIERR_SUCCESS;
  dwCPTFlags = 0;
  usEvcbIndex = 0;
  hWnd = 0;
  ulTemp1 = 0;
  usFound = FALSE;

  /****************************
  * Dereference Pointers
  *****************************/
  ulParam1 = pFuncBlock->ulParam1;
  ulParam2 = pFuncBlock->ulParam2;
  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;

  /**************************
  * Mask unWanted Bits
  **************************/
  dwCPTFlags = ulParam1;
  dwCPTFlags &= ~MCI_WAIT;
  dwCPTFlags &= ~MCI_NOTIFY;
  /*******************************
  * Check for Valid Flags
  *******************************/
  dwCPTFlags &= ~(MCI_SET_CUEPOINT_ON + MCI_SET_CUEPOINT_OFF);

  if (dwCPTFlags > 0 )
      return MCIERR_INVALID_FLAG;
  /****************************************
  * Check to see if ulParam2 Good
  ****************************************/
  ulrc = CheckMem (((PVOID)ulParam2),
                   sizeof (MCI_CUEPOINT_PARMS), PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
      return MCIERR_MISSING_PARAMETER;

  lpCueParms = (LPMCI_CUEPOINT_PARMS)ulParam2;

  if (ulpInstance == (ULONG)NULL )
      return MCIERR_INSTANCE_INACTIVE;

  /************************************
  * Check call back handle
  ************************************/
  hWnd = (HWND)lpCueParms->dwCallback;


  if (!WinQueryWindowProcess(hWnd, &pid, &tid))
      return(MCIERR_INVALID_CALLBACK_HANDLE);

  /************************************************
  * Check For Valid Flags but Invalid Combo
  *************************************************/
  if (ulParam1 & MCI_SET_CUEPOINT_ON && ulParam1 & MCI_SET_CUEPOINT_OFF)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  if (!(ulParam1 & MCI_SET_CUEPOINT_ON || ulParam1 & MCI_SET_CUEPOINT_OFF))
       return MCIERR_MISSING_FLAG;

  /*****************************************
  * No Element Loaded Reject Cuepoint
  ******************************************/
  if (ulpInstance->usFileExists == UNUSED)
      return MCIERR_FILE_NOT_FOUND;

  /*************************
  * Check for Range Errors
  *************************/
  if (lpCueParms->dwCuepoint <= 0)
      return MCIERR_OUTOFRANGE;

  /****************************
  * Enable Cuepoints
  *****************************/
  if (ulParam1 & MCI_SET_CUEPOINT_ON) {

      if (CUEPTINDX >= MAX_CUEPOINTS)
          return MCIERR_CUEPOINT_LIMIT_REACHED;

      for (usEvcbIndex = 0; usEvcbIndex < CUEPTINDX; usEvcbIndex++) {
           if (CUEPTEVCB[usEvcbIndex].mmCuePt == lpCueParms->dwCuepoint)
               return MCIERR_DUPLICATE_CUEPOINT;
      }   /* break out of this */

      CUEPTEVCB[CUEPTINDX].dwCallback = hWnd;
      CUEPTEVCB[CUEPTINDX].wDeviceID = ulpInstance->wWaveDeviceID;

      /********************************************
      * Fill In Time Event Control Block
      *********************************************/
      CUEPTEVCB[CUEPTINDX].evcb.ulType = EVENT_CUE_TIME;
      CUEPTEVCB[CUEPTINDX].evcb.ulFlags = EVENT_SINGLE;
      CUEPTEVCB[CUEPTINDX].evcb.hstream = STREAM.hStream;

      /*******************************************************
      * Time At Which Cue Point Notifucation is generated
      ********************************************************/
      ulrc = ConvertToMM (ulpInstance, (DWORD*)&(ulTemp1),
                               (DWORD)lpCueParms->dwCuepoint);

      CUEPTEVCB[CUEPTINDX].evcb.mmtimeStream =ulTemp1;
      CUEPTEVCB[CUEPTINDX].mmCuePt = lpCueParms->dwCuepoint;

      /*****************************************************
      * Stick In INSTANCE Pointer in The Time EVCB
      ******************************************************/
      CUEPTEVCB[CUEPTINDX].ulpInstance = (ULONG)ulpInstance;


      /*****************************************************
      * Copy CuePoint User Parm into EVCB Structure
      ******************************************************/
      CUEPTEVCB[CUEPTINDX].usCueUsrParm =lpCueParms->wUserParm;

      /****************************
      * Enable the Time Event
      ****************************/
      if (ulpInstance->ulCreateFlag == PREROLL_STATE)
         {
         ulrc = SpiEnableEvent( (PEVCB) &(CUEPTEVCB[CUEPTINDX]),
                                (PHEVENT) &(STREAM.HCuePtHndl[CUEPTINDX]));
         ulpInstance->usCuePt = EVENT_ENABLED;
         }
      else
         {
         ulpInstance->usCuePt = TRUE;
         }

      if (ulrc == ERROR_INVALID_EVENT)
          ulrc = MCIERR_OUTOFRANGE;

      if (ulrc)
          return ulrc;
      /*****************************
      * Increment CuePt Index
      ******************************/
      CUEPTINDX++;

  } /* Set Cue Point On */


  /************************
  * Disable CuePoints
  *************************/

  if (ulParam1 & MCI_SET_CUEPOINT_OFF) {
      for (usEvcbIndex = 0; usEvcbIndex < CUEPTINDX; usEvcbIndex++) {
           if (CUEPTEVCB[usEvcbIndex].mmCuePt == lpCueParms->dwCuepoint) {
               if (ulpInstance->ulCreateFlag == PREROLL_STATE) {
                       /**********************************************
                       * Check Validity of hCuePointEvent
                       **********************************************/
                       ulrc = SpiDisableEvent (STREAM.HCuePtHndl[usEvcbIndex]);
                       CUEPTEVCB[usEvcbIndex].mmCuePt = 0;
                       usFound = TRUE;

               } /* CreateFlag == PrerollState */
               else
               {
                   usFound = TRUE;
                   ulpInstance->usCuePt = FALSE;
                   CUEPTEVCB[usEvcbIndex].mmCuePt = 0;
               }
           } /* Found the CuePoint Block */
      } /* for Loop */

      if (ulrc == ERROR_INVALID_EVENT)
          ulrc = MCIERR_OUTOFRANGE;

      if (usFound == FALSE)
          return MCIERR_INVALID_CUEPOINT;

      if (ulrc)
          return ulrc;
  }   /* CuePoint Off */


  return (ULONG)(MCIERR_SUCCESS);
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISEEK.C
*
* DESCRIPTIVE
*
* FUNCTION:  Waveform Seek.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_SEEK message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiStopStream()     - SSM Spi
*                      spiSeekStream()     - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCISeek (FUNCTION_PARM_BLOCK *pFuncBlock)
{
  ULONG         ulrc;                    // Return Code
  ULONG         ulParam1;                // Flags For this message
  ULONG         ulParam2;                // Data for this message
  ULONG         ulAbortNotify = FALSE;   // abort notification messages
  INSTANCE      * ulpInstance;           // Local Instance Ptr
  ULONG         ulAssocFlag;             // Type of SpiAssociate
  ULONG         lCnt;                    // Semaphore Post Count
  DWORD         dwSeekPoint;             // Seek Point
  ULONG         ulTemp1;                 // Temporary Variable
  DWORD         SpiFlags;                // Flags For Spi Seek
  DWORD         dwSeekFlags;             // Incoming Seek Flags From MCI
  DWORD         dwFileLength;            // Element Length in MM
  LPMCI_SEEK_PARMS   lpSeekParms = NULL; // Message Data Structure


  /********************************************
  * Dereference Pointers From Thread Block
  ********************************************/
  ulParam1 = pFuncBlock->ulParam1;
  ulParam2 = pFuncBlock->ulParam2;
  ulrc = MCIERR_SUCCESS;
  dwSeekPoint = 0;
  ulTemp1 = 0;

  /*******************************************
  * Mask Wait and Notify Bits
  ********************************************/
  ulParam1 &= ~ MCI_NOTIFY;
  ulParam1 &= ~MCI_WAIT;
  dwSeekFlags = ulParam1;
  /********************************************
  * Mask For Valid Flags on MCI_SEEK
  *********************************************/
  dwSeekFlags &= ~(MCI_TO + MCI_TO_START+ MCI_TO_END);

  if (dwSeekFlags > 0)
      return MCIERR_INVALID_FLAG;

  if (ulParam1 & MCI_TO_START && ulParam1 & MCI_TO_END)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  if (ulParam1 & MCI_TO_START && ulParam1 & MCI_TO)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  if (ulParam1 & MCI_TO && ulParam1 & MCI_TO_END)
      return MCIERR_FLAGS_NOT_COMPATIBLE;


  if (!(ulParam1 & MCI_TO_START || ulParam1 & MCI_TO_END ||
       ulParam1 & MCI_TO))

      return MCIERR_MISSING_FLAG;

  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;

  if (ulParam1 & MCI_TO) {
      ulrc = CheckMem (((PVOID)ulParam2),
                       sizeof (MCI_SEEK_PARMS), PAG_READ);
      if (ulrc != MCIERR_SUCCESS)
          return MCIERR_MISSING_PARAMETER;
      lpSeekParms = (LPMCI_SEEK_PARMS)ulParam2;
  }
  if (ulpInstance == (ULONG)NULL )
      return MCIERR_INSTANCE_INACTIVE;

  if (ulpInstance->ulInstanceSignature != ACTIVE)
      return MCIERR_INSTANCE_INACTIVE;
  /************************************
  * No Element Cant Seek
  *************************************/
  if (ulpInstance->usFileExists == FALSE ||
      ulpInstance->usFileExists == UNUSED)
          return MCIERR_FILE_NOT_FOUND;

  /*********************************************
  * MCI_SEEK aborts any ongoing PLAY/RECORD.
  *********************************************/
  DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
  if (ulpInstance->usNotifyPending == TRUE)
     {
     ulpInstance->ulNotifyAborted = TRUE;
     ulpInstance->usNotifyPending = FALSE;
     ulAbortNotify = TRUE;
     }
  DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

  if ( ulAbortNotify )
     {

     if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
        {
        // Save is a non-interruptible operation
        // wait for completion
        DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

        }
      else
        {
        PostMDMMessage ( MCI_NOTIFY_ABORTED,
                         ulpInstance->usNotPendingMsg, pFuncBlock);
        DosResetEventSem (ulpInstance->hEventSem, &lCnt);
        DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

        if (AMPMIX.ulOperation == OPERATION_PLAY)
           {
           SpiFlags = SPI_STOP_DISCARD;
           }
        else
           {
           SpiFlags = SPI_STOP_FLUSH;
           }

        /*****************************************************
        * Stop discard for play or stop flush for record.
        *****************************************************/

        ulrc = SpiStopStream (STREAM.hStream, SpiFlags);
        SpiFlags = 0;

        if (!ulrc)
          {
          DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
          }

        DosResetEventSem (ulpInstance->hEventSem, &lCnt);

        STRMSTATE = MCI_STOP;

        } /* if !save pending */



      }  /* There was a pending Notify */

  /*****************************************
  * Ensure that Stream is stopped.
  * if not stop it discarding all buffers.
  *****************************************/

  if ((STRMSTATE == CUERECD_STATE) ||
      (STRMSTATE == MCI_RECORD)) {
       DosResetEventSem (ulpInstance->hEventSem, &lCnt);

       /*****************************************************
       * Stop discard for play or stop flush for record.
       ******************************************************/
       ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);
       if (!ulrc)
           DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);

  }   /* Exclusive Cue Recd and Recd States */

  /*********************************************
  * Return Error if No Element is Specified
  **********************************************/
  if (ulpInstance->usFileExists == FALSE)
      return MCIERR_FILE_NOT_FOUND;

  if (ulpInstance->ulCreateFlag != PREROLL_STATE) {
      if (ulpInstance->usPlayLstStrm == TRUE)

          ulAssocFlag = (ULONG)NULL;
      else
          ulAssocFlag = PLAY_STREAM;
      /**********************************************
      * Create a Stream with A = Src & B = Tgt
      ***********************************************/
      ulrc = CreateNAssocStream ( STREAM.hidASource,
                                  STREAM.hidBTarget,
                                  &(STREAM.hStream),
                                  (INSTANCE *) ulpInstance,
                                  (ULONG)ulAssocFlag,
                                  (PEVFN)PlEventProc);
      if (ulrc)
          return ulrc;

      if (ulAssocFlag == (ULONG)NULL) // Do the Associate Seperately

          ulrc = AssocMemPlayToAudioStrm (ulpInstance, PLAY_STREAM);

      if (ulrc)
          return (ulrc);
      /******************************
      * UpDate State to created
      *******************************/
      ulpInstance->ulCreateFlag = PREROLL_STATE;

  }  /* Stream Creation */

  /**********************************************
  * Check For Valid Transition States
  ***********************************************/
    DosResetEventSem (ulpInstance->hEventSem, &lCnt);

    /***************************************************
    * Stop discard for play or stop flush for record.
    ****************************************************/
    ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);

    if (!ulrc)
       {
       DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
       }

    STRMSTATE = MCI_STOP;

   ulrc = MCIERR_SUCCESS;

  /********************
  * Stream Seek.
  *******************/
  if (ulpInstance->usPlayLstStrm != TRUE) {

     if (ulParam1 & MCI_TO) {
         ConvertTimeUnits (ulpInstance, (DWORD*)& (dwFileLength),
                           FILE_LENGTH);

         /****************************************
         * Convert Seek to its base
         ******************************************/
        // ConvertToMM (ulpInstance, (DWORD*)& (ulTemp1),
        //            (DWORD)(lpSeekParms->dwTo));

         ulTemp1 = lpSeekParms->dwTo;

         if (ulTemp1 > dwFileLength)
             return MCIERR_OUTOFRANGE;

     } /* To Flag On */
  } /* Non PlayList */

  /********************************************************
  * Parse Different Seek Flags to Translate into SpiFlags
  ********************************************************/
  if (ulParam1 & MCI_TO_START) {
      dwSeekPoint = 0;
      ulTemp1 = 0;
      SpiFlags = SPI_SEEK_ABSOLUTE;
  }

  if (ulParam1 & MCI_TO_END) {
      dwSeekPoint = 0;
      ulTemp1 = 0;
      SpiFlags = SPI_SEEK_FROMEND;
  }
  if (ulParam1 & MCI_TO) {
      dwSeekPoint = lpSeekParms->dwTo;
      SpiFlags = SPI_SEEK_ABSOLUTE;
  }
  if (ulpInstance->usPlayLstStrm == TRUE) {

      DosResetEventSem (ulpInstance->hEventSem, &lCnt);
      ulrc = SpiStopStream (STREAM.hStream,
                            SPI_STOP_DISCARD);

      if (!ulrc)
          DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1);
  }   /* Non PlayList */



   /*********************************************
   * Convert Seek Units to MMTime
   * This translates Stream Seek units to MMTIME
   * ONLY if the user requested a number
   *********************************************/
   if ( lpSeekParms )
      {
      ConvertToMM (ulpInstance, (DWORD*)& (ulTemp1),
                   (DWORD)(lpSeekParms->dwTo));
      }

  /*************************
  * Do the Seek Thing
  *************************/
  ulrc = SpiSeekStream (STREAM.hStream, SpiFlags,
                        (LONG)(ulTemp1));
  if (ulrc)
      return (ulrc);

  STRMSTATE = MCI_SEEK;  // UpDate Current State

  return (ULONG)(ulrc);

}

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISTOP.C
*
* DESCRIPTIVE
*
* FUNCTION:  Stop Playing/Recording.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_STOP message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES: spiStopStream ()   -   SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIStop ( FUNCTION_PARM_BLOCK *pFuncBlock)
{
   ULONG               ulrc;               // MME Error Value Propogated
   INSTANCE         * ulpInstance;         // Local Instance
   ULONG           ulParam1;               // Incoming MCI Flags
   ULONG           lCnt;                   // Semaphore Posts Freq
   ULONG           ulAbortNotify = FALSE;  // whether or not to abort a notify
   DWORD           dwStopFlags;            // Mask for Incoming MCI Flags
   DWORD           dwSpiFlags;

   /**************************
   * Intialize the Vars
   ***************************/
   ulrc = MCIERR_SUCCESS;
   dwStopFlags = 0;
   dwSpiFlags = 0;

   /*****************************
   * Do Some Flag Checking
   ******************************/
   ulParam1 = pFuncBlock->ulParam1;
   dwStopFlags = ulParam1;
   dwStopFlags &= ~MCI_WAIT;
   dwStopFlags &= ~MCI_NOTIFY;

   if (dwStopFlags > 0)
       return MCIERR_INVALID_FLAG;
   ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;

   /*********************************************
   * Check whether this a valid Instance
   *********************************************/
   if (ulpInstance == (ULONG)NULL )
       return MCIERR_INSTANCE_INACTIVE;

   if (ulpInstance->ulInstanceSignature != ACTIVE)
       return MCIERR_INSTANCE_INACTIVE;

   /***************************************************
   * Pending Notifies (asynchronous Play/Record Threads)
   ****************************************************/
   DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
   if (ulpInstance->usNotifyPending == TRUE)
      {
      ulpInstance->ulNotifyAborted = TRUE;
      ulpInstance->usNotifyPending = FALSE;
      ulAbortNotify = TRUE;
      }
   DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );

   if (ulAbortNotify == TRUE)
     {


     if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
        {
        // Save is a non-interruptible operation
        // wait for completion

        DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

        }
     else
        {
        PostMDMMessage ( MCI_NOTIFY_ABORTED,
                         ulpInstance->usNotPendingMsg,
                         pFuncBlock);

        DosResetEventSem (ulpInstance->hEventSem, &lCnt);
        DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

        if ( ulpInstance->usNotPendingMsg == MCI_RECORD ||
             AMPMIX.ulOperation == OPERATION_RECORD )
          {
          dwSpiFlags = SPI_STOP_FLUSH;
          }
        else
          {
          dwSpiFlags = SPI_STOP_STREAM;
          }

        /***********************************
        * Stop The Stream
        ************************************/
        ulrc = SpiStopStream (STREAM.hStream, dwSpiFlags);

        if ( AMPMIX.ulOperation == OPERATION_RECORD )
          {
          if (!ulrc)
             {
             /*****************************
             * Wait for the stopped event
             ******************************/
             DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
             }

          /***********************************
          * Record streams go into a stopped state
          ************************************/
          STRMSTATE = MCI_STOP;

          }
        else
          {
          /*****************************************
          * Since a pause does not generate an event
          * create a fake one so our play thread can
          * clean up
          *****************************************/
          ulpInstance->StreamEvent = EVENT_STREAM_STOPPED;
          ulrc = DosPostEventSem (ulpInstance->hEventSem);

          DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);

          /***************************************
          * Play streams go into a stopped/paused state
          ****************************************/
          STRMSTATE = STOP_PAUSED;
          }


        } /* if no pending save */

        ulpInstance->usNotifyPending = FALSE;

     } /* if notify pending */


   if (STRMSTATE != MCI_STOP && STRMSTATE != STOP_PAUSED )
      {
      DosResetEventSem (ulpInstance->hEventSem, &lCnt);
      ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_STREAM );

      if (ulrc == ERROR_STREAM_ALREADY_STOP)
         {
         ulrc = MCIERR_SUCCESS;
         }

       STRMSTATE = STOP_PAUSED;
      }

   return (ULONG)(MCIERR_SUCCESS);

}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCIPAUS.C
*
* DESCRIPTIVE
*
* FUNCTION:  Waveform Pause.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_PAUSE message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES: spiStopStream ()   -   SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIPaus (FUNCTION_PARM_BLOCK *pFuncBlock)

{
    ULONG           ulrc;
    INSTANCE *      ulpInstance;
    DWORD           dwPausFlags;

    dwPausFlags = 0;
    ulrc = MCIERR_SUCCESS;
    dwPausFlags = pFuncBlock->ulParam1;
    dwPausFlags &= ~MCI_WAIT;
    dwPausFlags &= ~MCI_NOTIFY;

    if (dwPausFlags > 0 )
        return MCIERR_INVALID_FLAG;

    ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;

    if (ulpInstance == (ULONG)NULL )
        return MCIERR_INSTANCE_INACTIVE;

    if (ulpInstance->ulInstanceSignature != ACTIVE)
        return MCIERR_INSTANCE_INACTIVE;


    /****************************************************
    * The default action on an spiStopstream is pause.
    *****************************************************/
    if ((STRMSTATE == MCI_PLAY ) || (STRMSTATE == MCI_RECORD)) {

        ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_STREAM);
        STRMSTATE = MCI_PAUSE;    /* update Instance */

        if (ulrc == ERROR_STREAM_ALREADY_PAUSE)
            ulrc = MCIERR_SUCCESS;
    }

    return (MCIERR_SUCCESS);

}



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCIRESM
*
* DESCRIPTIVE      Audio MCD Resume.
*
* FUNCTION:  Resume Playback/Record from Paused State.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_RESUME message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES: SpiStartStream ()        -   SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC  MCIResm (FUNCTION_PARM_BLOCK * pFuncBlock)
{

   ULONG                  ulrc;            // MME Error Value Propogated
   INSTANCE               * ulpInstance;   // Local Instance
   DWORD                  dwResmFlags;     // Incoming MCI Flags
   DWORD                  dwSetAll;        // Internal Set Flags
   MCI_WAVE_SET_PARMS     lpSetParms;      // Internal Set Struct

   ulrc = MCIERR_SUCCESS;
   /*********************************/
   // Check for invalid flags
   /*********************************/
   dwResmFlags = pFuncBlock->ulParam1;
   dwResmFlags &= ~MCI_WAIT;
   dwResmFlags &= ~MCI_NOTIFY;
   dwSetAll = MCI_WAVE_SET_BITSPERSAMPLE|MCI_WAVE_SET_FORMATTAG|MCI_WAVE_SET_CHANNELS|
              MCI_WAVE_SET_SAMPLESPERSEC;

   if (dwResmFlags > 0 )
           return MCIERR_INVALID_FLAG;


   /************************************/
   // Derefernce pointers.
   /***********************************/
   ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;

   if (ulpInstance == (ULONG)NULL )
       return MCIERR_INSTANCE_INACTIVE;

   if (ulpInstance->ulInstanceSignature != ACTIVE)
       return MCIERR_INSTANCE_INACTIVE;
   /*******************************
   * Set Amp Defaults
   ********************************/
   SetAmpDefaults (ulpInstance);  // Always Restore Amp Stuff

   VSDInstToWaveSetParms (&lpSetParms,ulpInstance);

   /**********************************************************
   * No State Transition if already in States Play or Record
   **********************************************************/
   if ((STRMSTATE == MCI_PLAY) || (STRMSTATE == MCI_RECORD) ||
       (STRMSTATE == MCI_STOP) || (STRMSTATE == MCI_SEEK )  ||
       (STRMSTATE == STREAM_SET_STATE ) || ( STRMSTATE == STOP_PAUSED ) )

       return MCIERR_SUCCESS;

   /*****************************************************
   * Resume PlayBack/Record. only if not in States 8, 9
   *****************************************************/
   if ((STRMSTATE != CUEPLAY_STATE) && (STRMSTATE != CUERECD_STATE) &&
       (STRMSTATE != CREATE_STATE)) {

       ulrc = SpiStartStream (STREAM.hStream, SPI_START_STREAM);

       if (ulrc == ERROR_STREAM_NOT_STOP)
           ulrc = MCIERR_SUCCESS;

       if ( AMPMIX.ulOperation == OPERATION_PLAY )
          {
          STRMSTATE = MCI_PLAY;      // update Instance
          }
       else
          {
          STRMSTATE = MCI_RECORD;
          }
   }            // valid Transition States

   return (MCIERR_SUCCESS);

}




/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISTPA
*
* DESCRIPTIVE      Audio MCD Set Position Advice.
*
* FUNCTION:  Post Media Position Advice Notification messages.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_SET_POSITION_ADVISE message.
*
* EXIT-NORMAL: Return Code MCIERR_SUCCESS
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES: spiStartStream ()         -   SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

RC  MCIStpa (FUNCTION_PARM_BLOCK * pFuncBlock)
{
  ULONG           ulrc;                      // Error Value
  ULONG           ulParam1;                  // Incoming MCI Flags
  INSTANCE        *ulpInstance;              // Local Instance
  DWORD           dwSTPAFlags;               // Mask For Incoming Flags
  ULONG           ulCTime;                   // Converted Time
  PID             pid;
  TID             tid;
  HWND            hWnd;
  LPMCI_POSITION_PARMS lpPositionParms;      // MCI Msg Data

  /***************************
  * Intialize Pointers
  ****************************/
  ulrc = MCIERR_SUCCESS;
  ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;
  ulParam1 = pFuncBlock->ulParam1;
  dwSTPAFlags = ulParam1;

  /**********************************
  * Check for Validity of Flags
  ***********************************/
  dwSTPAFlags &= ~MCI_NOTIFY;
  dwSTPAFlags &= ~MCI_WAIT;
  dwSTPAFlags &= ~(MCI_SET_POSITION_ADVISE_ON +
                   MCI_SET_POSITION_ADVISE_OFF);

  /*************************************
  * Return error if flags are not
  * appropriate for the message
  *************************************/

  if (dwSTPAFlags > 0 )
      return MCIERR_INVALID_FLAG;

  /********************************
  * Null Parameter Conditions
  *********************************/
  ulrc = CheckMem (((PVOID)pFuncBlock->ulParam2),
                   sizeof (MCI_POSITION_PARMS), PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
      return MCIERR_INVALID_BUFFER;


  /******************************************************
  * Dereference MCI Message Data Structure
  ******************************************************/
  lpPositionParms = (LPMCI_POSITION_PARMS)pFuncBlock->ulParam2;

  /******************************************************
  * Check for Invalid Combination of Valid Flags
  *******************************************************/
  if ((ulParam1 & MCI_SET_POSITION_ADVISE_ON) &&
      (ulParam1 & MCI_SET_POSITION_ADVISE_OFF))

      return MCIERR_FLAGS_NOT_COMPATIBLE;

  /************************************************
  * Check for Invalid Combination of Valid Flags
  *************************************************/
  if (!(ulParam1 & MCI_SET_POSITION_ADVISE_ON ||
      ulParam1 & MCI_SET_POSITION_ADVISE_OFF))

      return MCIERR_MISSING_FLAG;

  /******************************************
  * Enable Position Advise
  *******************************************/

  if (ulParam1 & MCI_SET_POSITION_ADVISE_ON) {
     /******************************
     * No Element, Return Error
     ******************************/
     if (ulpInstance->usFileExists == UNUSED)
         return MCIERR_FILE_NOT_FOUND;

     /************************************
     * Check call back handle
     ************************************/
     hWnd = (HWND)lpPositionParms->dwCallback;


     if (!WinQueryWindowProcess(hWnd, &pid, &tid))
          return(MCIERR_INVALID_CALLBACK_HANDLE);

     POSEVCB.dwCallback = (HWND)hWnd;

     POSEVCB.wDeviceID = ulpInstance->wWaveDeviceID;

     /******************************
     * Disable Position Advise
     *******************************/
     if ( ulpInstance->ulCreateFlag == PREROLL_STATE &&
          ulpInstance->usPosAdvise  == TRUE )
        {

        ulrc = SpiDisableEvent(STREAM.hPosEvent);
        }

     /***************************************
     * Fill in Pos Time Event Control Block
     ****************************************/
     POSEVCB.evcb.ulType = EVENT_CUE_TIME;
     POSEVCB.evcb.ulFlags = EVENT_RECURRING;
     POSEVCB.evcb.hstream = STREAM.hStream;

     /********************************************
     * Copy current instance into POSEVCB
     * When the Position Event comes back
     * asynchronously the current instance
     * pointer is necessary to do the callback
     * to the application.
     *********************************************/
     POSEVCB.ulpInstance = (ULONG)ulpInstance;

     /********************************************
     * Copy Position UserParm into instance.
     * This User Parameter is returned on
     * Position change messages to the user.
     *********************************************/
     ulpInstance->usPosUserParm = lpPositionParms->wUserParm;

     ulrc = ConvertToMM (ulpInstance, &(ulCTime),
                             (DWORD)lpPositionParms->dwUnits);

     /**********************************************/
     // Frequency of Position Change Messages
     /**********************************************/

     POSEVCB.evcb.mmtimeStream = POSEVCB.mmCuePt = ulCTime;

     /*********************************************
     * If dwUnits == 0 return MCIERR_OUTOFRANGE
     *********************************************/

     if (ulCTime == 0)
         return MCIERR_OUTOFRANGE;

     /*****************************************
     * Enable the Time Event
     *****************************************/

     if (ulpInstance->ulCreateFlag == PREROLL_STATE)
       {
       ulrc = SpiEnableEvent( (PEVCB) &(POSEVCB),
                              (PHEVENT) &(STREAM.hPosEvent));
       ulpInstance->usPosAdvise = EVENT_ENABLED; // Set The Flag and Return
       }
     else
       {
       ulpInstance->usPosAdvise = TRUE; // Set The Flag and Return
       }

     if (ulrc)
         return ulrc;
  }     /* Set Position Advise on */

  /*****************************************
  * Disable Position Advise
  ******************************************/

  if (ulParam1 & MCI_SET_POSITION_ADVISE_OFF) {

      if (ulpInstance->ulCreateFlag == PREROLL_STATE)

          ulrc = SpiDisableEvent (STREAM.hPosEvent);
      else
          ulpInstance->usPosAdvise = FALSE;

      if (ulrc == ERROR_INVALID_EVENT)
          ulrc = MCIERR_OUTOFRANGE;

      if (ulrc)
          return ulrc;

  }     /* Set Position Advise off */

  return (ULONG)(ulrc);    // return RC
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISYNC
*
* DESCRIPTIVE      Audio MCD Set Sync Offset.
*
* FUNCTION:  Set Synchronization offset.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_SET_SYNC_OFFSET message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES: spiStartStream ()         -   SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC MCISync (FUNCTION_PARM_BLOCK * pFuncBlock)
{
  ULONG                ulrc;                 // Error Value
  INSTANCE             * ulpInstance;       // Local Instance
  LPMCI_SYNC_OFFSET_PARMS lpSyncOffParms;   // Msg Data

  ulrc = MCIERR_SUCCESS;
  ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;
  lpSyncOffParms = (LPMCI_SYNC_OFFSET_PARMS)pFuncBlock->ulParam2;

  /****************************************************************/
  // Copy The Synchronization Offset to instance member.
  /****************************************************************/

  ulpInstance->ulSyncOffset = lpSyncOffParms->dwOffset;

  /***********************************************/
  // Flag the offset condition as true.
  /***********************************************/

  return (ULONG)(MCIERR_SUCCESS);
        // Forced RC
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISAVEFILE
*
* DESCRIPTIVE      Audio MCD Save File.
*
* FUNCTION:  Save Existing element init of RIFF File.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_SAVE message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES: spiStartStream ()         -   SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/
RC  MCISaveFile (FUNCTION_PARM_BLOCK * pFuncBlock)
{
  ULONG                 ulrc;                 // Error Value
  ULONG                 ulParam1;
  DWORD                 dwSpiFlags;
  DWORD                 dwCheckFlags = 0;
  ULONG                  lCnt;

  INSTANCE               * ulpInstance;      // Local Instance
  LPMCI_SAVE_PARMS lpSaveParms;              // Msg Data

  PSZ                   pszSaveName;

  ulrc = MCIERR_SUCCESS;

  ulParam1 =    pFuncBlock->ulParam1;
  lpSaveParms = (LPMCI_SAVE_PARMS) pFuncBlock->ulParam2;
  ulpInstance = (INSTANCE *) pFuncBlock->ulpInstance;

  /*****************************************/
  // is ulParam2 Good
  /****************************************/
  ulrc = CheckMem (((PVOID)lpSaveParms),
                   sizeof (MCI_SAVE_PARMS), PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
        return MCIERR_MISSING_PARAMETER;




  if (ulpInstance == NULL )
    return MCIERR_INSTANCE_INACTIVE;

  if (ulpInstance->ulInstanceSignature != ACTIVE)
    return MCIERR_INSTANCE_INACTIVE;

  dwCheckFlags = ulParam1;

  dwCheckFlags &= ~( MCI_WAIT + MCI_NOTIFY + MCI_SAVE_FILE);

  if (dwCheckFlags > 0)
     {
     return MCIERR_INVALID_FLAG;
     }

    /***********************************************************/
    // Pending Notifies (asynch Play/Record Threads)
    /***********************************************************/
    if (ulpInstance->usNotifyPending == TRUE)
      {

      PostMDMMessage (MCI_NOTIFY_ABORTED, ulpInstance->usNotPendingMsg, pFuncBlock);

      DosResetEventSem (ulpInstance->hEventSem, &lCnt);
      DosResetEventSem (ulpInstance->hThreadSem, &lCnt);


      if (ulpInstance->usNotPendingMsg == MCI_RECORD)
          dwSpiFlags = SPI_STOP_FLUSH;
      else
          dwSpiFlags = SPI_STOP_DISCARD;

      ulrc = SpiStopStream (STREAM.hStream, dwSpiFlags);

      if (!ulrc)
        {

        /***************************
        * Wait Till A thread Dies
        ***************************/

        DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);

        }

      STRMSTATE = MCI_STOP;

      ulpInstance->usNotifyPending = FALSE;
    }

  if ( STRMSTATE == CUERECD_STATE ||
       STRMSTATE == CUEPLAY_STATE ||
       STRMSTATE == MCI_PAUSE        )
      {
      ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD );

        if (!ulrc)
          {
          DosWaitEventSem (ulpInstance->hEventSem, (ULONG) ONE_MINUTE );
          }

      STRMSTATE = MCI_STOP;

      }

  // perform the save if it is possible

  if ( !ulpInstance->ulCanSave )
    {
    return MCIERR_UNSUPPORTED_FUNCTION;
    }

  if ( ulpInstance->usFileExists != TRUE )
     {
     return MCIERR_FILE_NOT_FOUND;
     }

  if ( !ulpInstance->ulUsingTemp )
    {
    return MCIERR_UNSUPPORTED_FUNCTION;
    }


  // did the user pass in a filename?????

  if ( ulParam1 & MCI_SAVE_FILE )
    {
    /*****************************************
    * is the filename valid????
    *****************************************/
    ulrc = CheckMem ( (PVOID) lpSaveParms->lpFileName,
                      1,
                      PAG_READ ) ;

    if (ulrc != MCIERR_SUCCESS)
        return MCIERR_MISSING_PARAMETER;

    pszSaveName = (PSZ) lpSaveParms->lpFileName;
    }
  else
    {

    /*****************************************
    * If the caller had us generate a filename
    * on the open, then save w/o a new filename
    * is invalid
    *****************************************/
    if  ( ulpInstance->ulCreatedName || ulpInstance->ulNoSaveWithoutName )
       {
       return MCIERR_MISSING_FLAG;
       }

    pszSaveName = NULL;
    }


    /***********************************************************
    * If save was requested with temp files, and if no filename
    * was specified, then we have already accomplished our
    * job
    ************************************************************/


  // perform the save message immediately if there is a wait

  if ( ulParam1 & MCI_WAIT )
    {
    ulrc = mmioSendMessage( ulpInstance->hmmio,
                            MMIOM_SAVE,
                            (LONG) pszSaveName,
                            0 );
    if ( ulrc )
      {
      // Get the error code for the last error

      ulrc = mmioGetLastError( ulpInstance->hmmio );
      if ( ulrc == ERROR_FILE_NOT_FOUND ||
           ulrc == ERROR_PATH_NOT_FOUND ||
           ulrc == ERROR_INVALID_DRIVE)
         {
         ulrc = MCIERR_FILE_NOT_FOUND;
         }
      else if ( ulrc == MMIOERR_CANNOTWRITE )
              {
              ulrc = MCIERR_TARGET_DEVICE_FULL;
              }
      else if ( ulrc == ERROR_ACCESS_DENIED ||
                ulrc == ERROR_SHARING_VIOLATION  )
              {
              ulrc = MCIERR_FILE_ATTRIBUTE;
              }
      }


    if ( ulParam1 & MCI_SAVE_FILE )
       {
      if ( ulpInstance->ulCreatedName )
         {
         DosDelete( ( PSZ ) ulpInstance->lpstrAudioFile );
         }

       strcpy( ( PSZ ) ulpInstance->lpstrAudioFile,
                pszSaveName );
       }

    /*******************************************************************
    * Ensure that if a file handle was passed in and we opened successfully
    * that we do not open again with the file handle
    ********************************************************************/
    ulpInstance->ulCreatedName = FALSE;

    }
  else
    /*********************************************/
    //  we must perform the save on a thread since
    // notify was requested (note, if neither notify
    // or wait are specified, then operated like notify
    // except without positing the message.
    /*********************************************/

    {
    /***********************************************************/
    // Let other threads know that a save is about to happen
    /***********************************************************/
    ulpInstance->usNotifyPending = TRUE;
    ulpInstance->usNotPendingMsg = MCI_SAVE;

    DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

    ulrc = DosCreateThread ((PTID) &pFuncBlock->pInstance->SaveThreadID,
                            (PFNTHREAD) StartSave,
                            (ULONG) pFuncBlock,
                            0L,
                            NOTIFY_THREAD_STACKSIZE );

    DosWaitEventSem (ulpInstance->hThreadSem, (ULONG ) -1 );

    }
    return ( ulrc );

}



/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: StartSave.
*
* DESCRIPTIVE NAME: Saves a file on a different thread
*
* FUNCTION:
*
*
* NOTES: This routine is called using  a separate thread spawned by
*        MCD on MCI Notify.
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
RC StartSave (FUNCTION_PARM_BLOCK *pFuncBlock )
{

   ULONG               ulrc;
   ULONG               ulErr;
   ULONG               ulParam1;

   ULONG                  lCnt;

   INSTANCE            *ulpInstance;

   LPMCI_SAVE_PARMS    lpSaveParms;

   CHAR                chSaveName[ CCHMAXPATH ];
   PSZ                 pszSaveName;


   ulrc = MCIERR_SUCCESS;
   ulErr = MCI_NOTIFY_SUCCESSFUL;

   ulpInstance = (INSTANCE *) pFuncBlock->ulpInstance;
   lpSaveParms = (LPMCI_SAVE_PARMS) pFuncBlock->ulParam2;
   ulParam1 = pFuncBlock->ulParam1;


   if ( ulParam1 & MCI_SAVE_FILE )
      {
      // we need to copy this string since it may be cleared
      // when the calling thread leaves

      strcpy( chSaveName, ( PSZ ) lpSaveParms->lpFileName );
      pszSaveName = (PSZ) chSaveName;
      }
   else
      {
      pszSaveName = NULL;
      }

   /*********************************************
   * We can free the save thread now that we have
   * obtained any possible file names
   **********************************************/

   DosPostEventSem ( ulpInstance->hThreadSem );
   DosResetEventSem (ulpInstance->hThreadSem, &lCnt);


     ulrc = mmioSendMessage( ulpInstance->hmmio,
                            MMIOM_SAVE,
                            (LONG) pszSaveName,
                            0 );

   /*********************************************
   * If there were no errors, then the filename the
   * caller requested is the default file name
   **********************************************/

   if ( ulParam1 & MCI_SAVE_FILE && !ulrc )
      {
      if ( ulpInstance->ulCreatedName )
         {
         DosDelete( ( PSZ ) ulpInstance->lpstrAudioFile );
         }

      strcpy( ( PSZ ) ulpInstance->lpstrAudioFile,
               chSaveName );

      }

   if (ulParam1 & MCI_NOTIFY)
      {
      /*****************************************
      * If an error occurred, then inform  the caller
      * of the problem.
      ******************************************/

      if ( ulrc )
         {
         ulErr = mmioGetLastError( ulpInstance->hmmio );
         if ( ulrc == ERROR_FILE_NOT_FOUND ||
              ulrc == ERROR_PATH_NOT_FOUND ||
              ulrc == ERROR_INVALID_DRIVE)
            {
            ulErr = MCIERR_FILE_NOT_FOUND;
            }
         else if ( ulrc == MMIOERR_CANNOTWRITE )
                 {
                 ulErr = MCIERR_TARGET_DEVICE_FULL;
                 }
         else if ( ulrc == ERROR_ACCESS_DENIED ||
                   ulrc == ERROR_SHARING_VIOLATION  )
                 {
                 ulErr = MCIERR_FILE_ATTRIBUTE;
                 }
         }

      PostMDMMessage (ulErr, MCI_SAVE, pFuncBlock);

      }


   /*****************************************
   * Post The Thread Sem To release Notify
   * Thread Blocked on this sem.
   ******************************************/

   DosPostEventSem ( ulpInstance->hThreadSem );

   STRMSTATE = MCI_STOP;
   ulpInstance->usNotifyPending = FALSE;


   ulpInstance->ulCreatedName = FALSE;



   if (ulParam1 & MCI_NOTIFY)
      {
      /*********************************************/
      // Release Thread Block Memory
      /*********************************************/
      ulrc = CleanUp ((PVOID)pFuncBlock);
      }


   return ( ulErr );

} /* Start Save */
