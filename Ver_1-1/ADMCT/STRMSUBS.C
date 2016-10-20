/*****************************************************************************
*
* SOURCE FILE NAME:             STRMSUBS.C
*
*
*
*              Copyright (c) IBM Corporation  1991, 1993
*                        All Rights Reserved
*
* DESCRIPTIVE NAME:     Support functions for wave streaming requests
*
* NOTES: Conecpts illustrated in this source file.
*         1. How to close mmio files, and temp files (CloseFile).
*         2. How to stop a record/playback stream (StopStream).
*         3. How to use a semaphore to protect in progress commands
*            from being aborted in sensitive areas.
*         4. Why a stream must be destroyed after an MCI_SET (DestroySetStream).
*         5. How to handle superceding and aborting of notifies.
*         6. How and why to set a cue time pause event for
*            MCI_TO (ProccessFromToFlags).
*         7. How to enable cuepoints and positionchanges (EnableEvents).
*
*    DEPENDENCIES: NONE
*    RESTRICTIONS: NONE
*
* FUNCTIONS: CloseFile
*            StopStream
*            GetNotifyAbortAccess
*            DestroySetStream
*            AbortInProgressNotify
*            PrepareAndCreateStream
*            ProcessFromToFlags
*            EnableEvents
*            QualityofService
*
*********************** END OF SPECIFICATIONS ********************************/
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
#include <qos.h>
#include <audiomcd.h>                   // Component Definitions.
#include <admcfunc.h>




/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: GetNotifyAbortAccess
*
* DESCRIPTIVE NAME:
*
* FUNCTION:     Allows functions to obtain access to abort notifications
*
* NOTES:
*
* INPUT:
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:
*
*********************** END OF SPECIFICATIONS *********************************/

void  GetNotifyAbortAccess ( INSTANCE *ulpInstance,
                             PULONG   pulAbortNotify )

{
ULONG   ulCount;

  /****************************************************
  * This semaphore controls whether or not it is safe
  * to abort a previous notify thread.
  * If the thread is in a critical section, it will have
  * access to this semaphore and we will wait till it
  * completes.
  ****************************************************/

  DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );

  /****************************************************
  * The fNotifyPending boolean indicates whether or not
  * a command is currently in progress (i.e. a play notify
  * was done before the current command).   If there is
  * a command in progress, set a few flags and let the
  * caller know that they must abort the in progress
  * operation.
  ********************************************************/

  /* investigate--change this usNotifyPending to fNotifyPending */

  if (ulpInstance->usNotifyPending == TRUE)
     {
     ulpInstance->ulNotifyAborted = TRUE;
     ulpInstance->usNotifyPending = FALSE;
     *pulAbortNotify = TRUE;
     DosResetEventSem ( ulpInstance->hThreadSem, &ulCount);
     }

  DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );


} /* GetNotifyAbortAccess */

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: DestroySetSream
*
* DESCRIPTIVE NAME: Destroys a stream because a set has made it invalid
*
* FUNCTION:     Since sets to a created stream cannot be communicated
                to a stream handler, then stream must be destroyed
                and created anew.
*
* NOTES:
*
* INPUT:
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:
*
*********************** END OF SPECIFICATIONS *********************************/

void DestroySetStream ( INSTANCE     *ulpInstance )

{
   /***********************************************
   * If a set was performed on an existing stream,
   * destroy the stream and get new spcb keys
   ***********************************************/

   if ( STRMSTATE == STREAM_SET_STATE )
     {
     DestroyStream ( &STREAM.hStream);

     ulpInstance->ulCreateFlag = CREATE_STATE;

     }

   return;

} /* DestorySetStream */


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: AbortInProgressNotify
*
* DESCRIPTIVE NAME: Stops a notify which was already in progress
*
* FUNCTION: Stops save, record and playback notifies
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: INSTANCE pointer
*        ULONG ulMessage -- either MCI_RECORD or MCI_PLAY
*        ULONG ulParam1
*
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiStopStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

ULONG AbortInProgressNotify( INSTANCE             *ulpInstance,
                             FUNCTION_PARM_BLOCK  *pFuncBlock,
                             ULONG                ulParam1,
                             ULONG                ulMessage )

{
   USHORT usAbortType;
   HWND   hwndHoldHandle;

   /***************************************************
   * Here is how an MCD should interrupt an
   * operation which was in progress. If the
   * currently active operation is the same as the
   * new operation, then we should post a
   * superceded message (e.g. if a play interrupts
   * a play, a "Play superceded" should be posted.
   * If a play interrupts a record, then an
   * aborted message should be posted (i.e. "record
   * aborted".
   ***************************************************/

   /***************************************************
   * Are we interrupting the same operation (i.e. play
   * interrupting a play)?
   ****************************************************/

   if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
       {
       /***********************************
       * Can't interrupt a save because data
       * can be lost
       ************************************/

       DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

       }

    else
      {

      /* Stop the pending thread */

      ThreadedStop ( ulpInstance );


      if ( ulpInstance->usNotPendingMsg == ulMessage &&
           !ulpInstance->fFakeNotify                 &&
           (ulParam1 & MCI_NOTIFY) )
         {
         /************************************************
         * If the new operation is a notify, then we are
         * superceded.  If not, then it is aborted.
         ************************************************/
        
         usAbortType = MCI_NOTIFY_SUPERSEDED;
         }   /* Case of a Play Superseeding a Play */

      else
         {
         /************************************************
         * If the new operation is a notify, then we are
         * superceded.  If not, then it is aborted.
         ************************************************/
        
         usAbortType = MCI_NOTIFY_ABORTED;
        
         /*****************************************
         * Before we destroy the stream determine
         * where we were
         *****************************************/
        
         GetOldStreamPosition( ulpInstance );
        
         DestroyStream ( &STREAM.hStream);
        
         ulpInstance->ulCreateFlag = PLAY_STREAM;

         } /* if we are aborting a different command */


       /*---------------------------------------------
       * If we are aborting a command, ensure that
       * we notify it on the correct callback handle
       *---------------------------------------------*/
       hwndHoldHandle = ulpInstance->hwndCallBack;

       if ( ulParam1 & MCI_NOTIFY )
          {
          ulpInstance->hwndCallBack = ulpInstance->hwndOldCallBack;
          }

       /*---------------------------------------------
       * Inform the caller that the previous command
       * has be officially terminated.
       *---------------------------------------------*/


       PostMDMMessage ( usAbortType,
                        ulpInstance->usNotPendingMsg ,
                        pFuncBlock);


       /*---------------------------------------------
       * now ensure that we will notify the caller
       * on the correct window handle.
       *---------------------------------------------*/

       ulpInstance->hwndCallBack = hwndHoldHandle;


       if (ulParam1 & MCI_NOTIFY)
           ulpInstance->usUserParm = pFuncBlock->usUserParm;



      } /* else save is not pending */

  return ( MCIERR_SUCCESS );

} /* AbortInProgressNotify */


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: GetOldStreamPosition
*
* DESCRIPTIVE NAME: Remembers current position in the stream.
*
* FUNCTION:
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: INSTANCE ptr
*        ULONG ulMessage -- either MCI_RECORD or MCI_PLAY
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* NOTES: This function remembers our position in the stream when
*        the caller switches between play and record streams.  This
*        allows the user to pick up at the same position.
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: SpiGetTime
*
*********************** END OF SPECIFICATIONS **********************/


ULONG GetOldStreamPosition( INSTANCE *ulpInstance )

{
  ULONG ulrc = MCIERR_SUCCESS;

  if ( !ulpInstance->ulOldStreamPos )
     {
     ulrc = SpiGetTime( STREAM.hStream,
                        ( PMMTIME ) &ulpInstance->ulOldStreamPos );

     /**********************************************
     * if an error occurred, then don't remember our
     * position in the stream.
     ***********************************************/

     if ( ulrc )
       {
       ulpInstance->ulOldStreamPos = 0;
       }
     } /* if we have no old stream position. */

     return ( MCIERR_SUCCESS );

} /* GetOldStreamPosition */

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: PrepareAndCreateStream
*
* DESCRIPTIVE NAME: Does work to create a stream
*
* FUNCTION: Inits variables and creates a stream
*           Boolean determines whether card must be set up also.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: INSTANCE ptr
*        ULONG ulMessage -- either MCI_RECORD or MCI_PLAY
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiCreateStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

ULONG PrepareAndCreateStream( INSTANCE *ulpInstance,
                              ULONG    ulOperation,
                              BOOL     fSetUpCard )


{
    ULONG               ulrc = MCIERR_SUCCESS;   // return codes
    ULONG               ulAssocFlag;             // stream association flag

    MCI_WAVE_SET_PARMS  SetParms;      // MCI Wave Set Parms

    ULONG               ulSetAll;        // Set all flags

    ulSetAll = MCI_WAVE_SET_BITSPERSAMPLE| MCI_WAVE_SET_FORMATTAG     |
               MCI_WAVE_SET_CHANNELS     | MCI_WAVE_SET_SAMPLESPERSEC;



     if (ulpInstance->ulCreateFlag != PREROLL_STATE)
        {

        /************************************************************
        * Determine which type of stream we will have to associate
        * Playlists will associate the memory stream handler,
        * and non-playlists will use the file system SH.
        *************************************************************/
        if (ulpInstance->usPlayLstStrm == TRUE)
           {
           ulAssocFlag = (ULONG)NULL;
           }
        else
           {

           /******************************************************
           * Set a flag to indicate which direction the stream
           * handlers should go (e.g. play and record streams
           * go in different directions.
           ******************************************************/

           if ( ulOperation == OPERATION_RECORD )
              {
              ulAssocFlag = RECORD_STREAM;
              }
           else
              {
              ulAssocFlag = PLAY_STREAM;
              }
           }


         /********************************************
         * Tell the amp mixer that we are about to
         * switch to the desired operation
         *********************************************/

         AMPMIX.ulOperation = ulOperation;

         if ( fSetUpCard )
            {
            /*************************************************
            * Always Reinit The Device before stream creation.
            *************************************************/
      
      
            VSDInstToWaveSetParms (&SetParms, ulpInstance);

            ulrc = SetAudioDevice ( ulpInstance,
                                    (PMCI_WAVE_SET_PARMS) &SetParms,
                                    ulSetAll );

            if ( ulrc )
               {
               return ( ulrc );
               }


            } /* If the card must be inited */

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
         if ( ulOperation == OPERATION_RECORD )
            {

            ulrc = CreateNAssocStream ( STREAM.hidBSource,
                                        STREAM.hidATarget,
                                        &STREAM.hStream,
                                        ulpInstance,
                                        ulAssocFlag,
                                        (PEVFN)RecordEventRoutine );

            }
         else
            {
            /********************************************
            * Create and associate a playback stream
            * This stream has the audio stream handler
            * as the source stream handler and file
            * system stream handler as the target.
            *********************************************/

            ulrc = CreateNAssocStream ( STREAM.hidASource,
                                        STREAM.hidBTarget,
                                        &STREAM.hStream,
                                        ulpInstance,
                                        ulAssocFlag,
                                        (PEVFN) PlayEventRoutine );
            }

         if (ulrc)
             return ulrc;

         /*****************************************************
         * In case of playlist do the associate seperately
         * Playlists associate the playlist with the memory
         * stream handlers.
         ******************************************************/

         if ( ulAssocFlag == (ULONG) NULL )
            {
            if ( ulOperation == OPERATION_RECORD )
              {
              ulAssocFlag = RECORD_STREAM;
              }
            else
              {
              ulAssocFlag = PLAY_STREAM;
              }

            ulrc = AssociatePlayList ( ulpInstance, ulAssocFlag );

            }


        } /* Play/Record Cue Stream */

    return ( ulrc );


} /* PrepareAndCreateStream */




/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: RememberStreamState
*
* DESCRIPTIVE NAME: remembers cuepoints + position accross streams
*
* FUNCTION: After a stream has been destroyed, this function will
*           set the new one up so that position advises, cue points
*           and stream position will be just like the old one.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: INSTANCE ptr
*        ULONG ulMessage -- either MCI_RECORD or MCI_PLAY
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiSeekStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

ULONG RememberStreamState( INSTANCE  *ulpInstance,
                           ULONG     ulParam1,
                           BOOL      fSeekNeeded )

{
   ULONG ulrc = MCIERR_SUCCESS;


   /*******************************************
   * Stream is destroyed. Reset cuepopint and
   * position advise flags to enabled state
   * and send it down on the new stream
   ********************************************/

   if (ulpInstance->usPosAdvise == EVENT_ENABLED)
       ulpInstance->usPosAdvise = TRUE;

   if (ulpInstance->usCuePt == EVENT_ENABLED)
       ulpInstance->usCuePt = TRUE;
   /*---------------------------------------
   * If we previously destroyed a stream,
   * seek to the position where we were
   * else just seek to 0
   *--------------------------------------*/

   if ( !(ulParam1 & MCI_FROM ) && fSeekNeeded )
      {
      ulrc = SpiSeekStream ( STREAM.hStream,
                             SPI_SEEK_ABSOLUTE,
                             ulpInstance->ulOldStreamPos );

      if (ulrc)
        {

        /*-------------------------------------------------------------
        * it is possible that the card may report a position that is
        * greater than what is actually recorded, so work around this
        * problem
        *-------------------------------------------------------------*/

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

        } /* if an error occurred */

      } /* if no from parameter */


    ulpInstance->ulOldStreamPos = 0;

    /************************************
    * place the stream in a stopped state
    * since no activity has occurred
    *************************************/

    STRMSTATE = MCI_STOP;

    return ( ulrc );


 } /* RememberStreamState */




/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: ProcessFromToFlags
*
* DESCRIPTIVE NAME: Uses from to flags to ensure to place stream
*
* FUNCTION: Uses MCI_FROM flag to seek to the correct position and
*           the MCI_TO to determine where to stop streaming.
*           Note: with new SPI API this will no longer be needed
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: FUNCTION_PARM_BLOCK
*        INSTANCE *ptr
*        ULONG    ulMessage -- either MCI_RECORD or MCI_PLAY
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiSeekStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

ULONG ProcessFromToFlags( FUNCTION_PARM_BLOCK  *pFuncBlock,
                          INSTANCE             *ulpInstance,
                          ULONG                ulOperation,
                          ULONG                ulParam1 )


{

   PMCI_PLAY_PARMS        pPlayParms;        // Msg Data Ptr
   PMCI_RECORD_PARMS      pRecordParms;      // MCI Record Parms


   ULONG                   ulConvertNumber;    // convert number to mmtime
   ULONG                   ulrc = MCIERR_SUCCESS;
   ULONG                   ulCount;            // Semphore counter


   /***********************************
   * Do a Seek to support FROM
   ***********************************/

   if (ulParam1 & MCI_FROM)
      {
      if (STRMSTATE != MCI_STOP )
        {
        /**************************************
        * Reset semaphore used to indicate when
        * streaming event has occurred.
        ****************************************/

        DosResetEventSem (ulpInstance->hEventSem, &ulCount);

        /**************************************
        * Stop the stream (discard buffers)
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

       /******************************************
       * Convert the number that the user passed in
       * (i.e. from samples) to mmtime so that we
       * can seek to the point in the stream.
       *******************************************/


       if ( ulOperation == MCI_RECORD )
          {
          pRecordParms= (PMCI_RECORD_PARMS )pFuncBlock->ulParam2;

          ulrc = ConvertToMM ( ulpInstance,
                               &ulConvertNumber,
                               pRecordParms->ulFrom);
          }
       else
          {
          pPlayParms= (PMCI_PLAY_PARMS )pFuncBlock->ulParam2;

          ulrc = ConvertToMM ( ulpInstance,
                               &ulConvertNumber,
                               pPlayParms->ulFrom);
          }


        /*********************************
        * Do the Seek Thing
        **********************************/

        if ( !ulrc )
           {
           ulrc = SpiSeekStream ( STREAM.hStream,
                                  SPI_SEEK_ABSOLUTE,
                                  ulConvertNumber);

           if ( ulrc )
              {
              return ( ulrc ) ;
              }
           }

      } /* Play From */

   /*************************************************
   * If the caller specified a to position via
   * MCI_TO, then convert the number to mm time
   * (we do this because it simplifies the code,
   * only dealing with one time base).  Then
   * set a cue time pause event in the stream at
   * the to point.  The streaming operation will
   * proceed till the pause event, report the event
   * and thus, it appears to the caller like we
   * stopped right at the to spot.
   *************************************************/

   if (ulParam1 & MCI_TO)
      {
      if ( ulOperation == MCI_RECORD )
          {
          pRecordParms= ( PMCI_RECORD_PARMS )pFuncBlock->ulParam2;

          ulrc = ConvertToMM ( ulpInstance,
                               &ulConvertNumber,
                               pRecordParms->ulTo );
          }
       else
          {
          pPlayParms= ( PMCI_PLAY_PARMS )pFuncBlock->ulParam2;

          ulrc = ConvertToMM ( ulpInstance,
                               &ulConvertNumber,
                               pPlayParms->ulTo );
          }

       /* Place the cue time pause event in the stream */

       if ( !ulrc )
          {
          ulrc = CreateToEvent ( ulpInstance, ulConvertNumber );
          }

       ulpInstance->fToEvent = TRUE;

      } // Of Play Till XXXX


  return ( ulrc );

} /* ProcessFromToFlags */



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: EnableEvents
*
* DESCRIPTIVE NAME: enables cue points and position advises
*
* FUNCTION: For newly created streams, this will enable cue points
*           and position advises which were sent to the previously
*           destroyed stream (i.e. a play following a record).
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiSeekStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

ULONG EnableEvents( INSTANCE *ulpInstance )

{

   ULONG         ulrc = MCIERR_SUCCESS;

   MTIME_EVCB    *pMCuePtEVCB;  // pointer


   /***************************************
   * Enable Position Advise if Needed
   ****************************************/


   if (ulpInstance->usPosAdvise == TRUE)
      {

      /*********************************
      * Correct The hStream Value
      **********************************/
      STREAM.PosAdvEvcb.evcb.hstream = STREAM.hStream;

      /**********************************************
      * Stick our instance pointer in the time
      * Event Control Block (EVCB).  This will allow
      * the event procedure to access instance data
      * (see PlayEventProc and RecordEventProc for
      * more information).
      ***********************************************/

      STREAM.PosAdvEvcb.ulpInstance = (ULONG) ulpInstance;

      /**************************************************************
      * Update position advise cuepoints created in different stream
      **************************************************************/

      STREAM.PosAdvEvcb.evcb.mmtimeStream = STREAM.PosAdvEvcb.mmCuePt;

      ulrc = SpiEnableEvent( (PEVCB) &(STREAM.PosAdvEvcb),
                             (PHEVENT) &(STREAM.hPosEvent));

      /* Set flag to indicate that the position advises were enabled */

      ulpInstance->usPosAdvise = EVENT_ENABLED;

      } /* if Positionadvise is true */


   /*******************************
   * Enable Cue points if any
   *******************************/

   if (!ulrc)
      {
      // toast this ushort business--use ulongs.

      if (ulpInstance->usCuePt == TRUE)

         {
         /*********************************
         * Get cue point linked list pointer
         ***********************************/

         pMCuePtEVCB = CUEPOINT;


         /***********************************************
         * The current MCD has a linked list of
         * cuepoints.   Proceed through the linked
         * list and enable each one in the stream.
         * Warning: just because the MCD supports
         * unlimited cuepoints, does not mean that the
         * stream handler does.
         ***********************************************/

         while ( pMCuePtEVCB )
           {
           pMCuePtEVCB->evcb.hstream = STREAM.hStream;

          /**********************************************
          * Stick our instance pointer in the time
          * Event Control Block (EVCB).  This will allow
          * the event procedure to access instance data
          * (see PlayEventProc and RecordEventProc for
          * more information).
          ***********************************************/

           pMCuePtEVCB->ulpInstance = (ULONG ) ulpInstance;

           /* Enable the cuepoints */

           ulrc = SpiEnableEvent( (PEVCB) pMCuePtEVCB,
                                  (PHEVENT) &pMCuePtEVCB->HCuePtHndl );

           pMCuePtEVCB = pMCuePtEVCB->pNextEVCB;

           } /* while there are cuepoints to place in the stream */


         ulpInstance->usCuePt = EVENT_ENABLED;

         } /* There are CuePts to be enabled */

      } /* if no return codes so far */

   return ( ulrc );

} /* EnableEvents */



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: StopStream
*
* DESCRIPTIVE NAME: Stops a stream
*
* FUNCTION: Sets up semaphores, stops the stream and waits for the
*           event.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiSeekStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/


ULONG StopStream( INSTANCE *ulpInstance,
                  ULONG    ulOperation )

{
   ULONG        ulCount;                // semaphore counter
   ULONG        ulSpiFlags;             // flags to use on the spi stop
   ULONG        ulrc = MCIERR_SUCCESS;  // return code

   /***************************************
   * Reset Internal semaphores
   ****************************************/

   DosResetEventSem ( ulpInstance->hEventSem, &ulCount);
   DosResetEventSem ( ulpInstance->hThreadSem, &ulCount);

   /********************************************
   * Do different stops based on the stream type
   * record streams must be flushed so that no
   * data will be lost, whereas playback streams
   * are not concerned with data loss since they
   * create no data.
   ********************************************/

   if ( AMPMIX.ulOperation == OPERATION_RECORD )
      {
      ulSpiFlags = SPI_STOP_FLUSH;
      }
   else
      {
      ulSpiFlags = SPI_STOP_DISCARD;
      }

   /**********************************
   * Stop Streaming
   **********************************/
   ulrc = SpiStopStream (STREAM.hStream, ulSpiFlags );

   if (!ulrc)
      {
      DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1 );
      }

   if ( ulrc == ERROR_STREAM_ALREADY_STOP )
      {
      ulrc = MCIERR_SUCCESS;
      }

   return ( ulrc );

} /* StopStream */




/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: CloseFile
*
* DESCRIPTIVE NAME: Close a multimedia i/o file
*
* FUNCTION: Closes the file and if it was a temporary file which we
*           opened without saving it, ensure that it is deleted.  If
*           the file was opened via OPEN_MMIO, DO NOT close the file.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: FUNCTION_PARM_BLOCK
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: MCIERR ().
*
* EXTERNAL REFERENCES: spiSeekStream()    - SSM Spi
*
*********************** END OF SPECIFICATIONS **********************/

ULONG CloseFile( INSTANCE *ulpInstance )


{
  ULONG ulrc = MCIERR_SUCCESS;

  /********************************************
  * If there is a file handle to close and
  * if the caller did not pass in the file
  * handle via OPEN_MMIO close the file.
  * If we close an open mmio handle, then
  * the application will not be able to use
  * it.
  ********************************************/

  if (ulpInstance->hmmio != 0 && !ulpInstance->fOpenMMIO )
     {

      ulrc = mmioClose (ulpInstance->hmmio, 0);

      /* If we created the temporary file and it wasn't saved--delete it */

      if ( ulpInstance->ulCreatedName )
         {
         DosDelete( ( PSZ ) ulpInstance->pszAudioFile );
         }
      /* Set flags to clear handle and ensure we don't delete the next temp file */

      ulpInstance->ulCreatedName = FALSE;

      ulpInstance->hmmio = 0;

     } /* hmmio not Null */

   return ( ulrc );

} /* CloseFile */



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: QualityofService
*
* FUNCTION: Some OS/2 networks have specific IO procs which can 
*           manage network traffic.  We will use the mmio messages
*           begin_stream to inform these io procs
*           that we wish to stream across the network.
*
*           If you are writing a streaming MCD, it is advantageous
*           to use these messages since performance across LAN's
*           may be improved.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: FUNCTION_PARM_BLOCK
*
* EXIT-NORMAL: MCIERR_SUCCESS
*
* EXIT_ERROR:
*
* EFFECTS:
*
* EXTERNAL REFERENCES: mmioSendMessage()    - MMIO function
*
*********************** END OF SPECIFICATIONS **********************/
ULONG   BeginQualityofService ( INSTANCE      *pInstance,
                                LONG          lStreamDirection )

{

LONG         rc;
CUSTOM_QOS   CustomQOS;


  /* Currently, there are 4 parameters to QOS, this will increase */

  CustomQOS.QOSParms.lNumQOSParms = 4;

  /*---------------------------------------------------
  * The first parameter is the service request. It
  * can currently contain one of three values:
  *
  * A. GUARANTEED.  The MCD wants to know if the
  *    network can guarantee the traffic we need.
  *
  * B. DONTCARE.  We will ask for a certain level of
  *    support.  However, if it is unavailable, we
  *    can continue.
  *
  * C. DONTRESERVE. The MCD wants no guarantees about
  *    whether the network can support this transfer.
  *
  *----------------------------------------------------*/
  CustomQOS.QOSParms.QOSParms[0].lQOSParmId  = SERVICE_REQUEST;
  CustomQOS.QOSParms.QOSParms[0].lQOSValue    = pInstance->lQosValue;

  /*-------------------------------------------------
  * QOS needs to know the number of streaming
  * buffers SSM will be using.  A streaming MCD
  * can obtain this information via SpiGetProtocol.
  *-------------------------------------------------*/

//  pQOS->lQOSParmId  = NUM_STREAM_BUFFERS;
//  pQOS->lQOSValue    = pInstance->StreamInfo.spcb.ulMinBuf;
//  pQOS++;


  /*-----------------------------------------------------
  * MAX_EE_JITTER describes the allowable amount of
  * variation the streaming subsystem allows
  * in end-to-end delay between client and server.
  *
  * The generic calculation for max_ee_jitter is
  * (if x is the number of bytes required for a single
  * data unit (e.g. # of bytes for a frame of video)
  * then jitter = ( streaming buffer size - x ) /
  * max. transfer rate.
  *
  *-----------------------------------------------------*/


  CustomQOS.QOSItem[0].lQOSParmId  = MAX_EE_JITTER;
  CustomQOS.QOSItem[0].lQOSValue   = ( pInstance->StreamInfo.spcb.ulBufSize - 0 ) /
                              ( pInstance->ulAverageBytesPerSec / 1000 );

  /*-------------------------------------------
  * The following parameters (max_data_rate
  * and avg_data_rate) are the same for digital
  * audio.  However, this may differ on a case
  * by case basis for other MCD's.
  *
  * The calculation for avg. bytes per sec is
  * done in GetAudioHeader in audiosub.c
  *-------------------------------------------*/

  CustomQOS.QOSItem[1].lQOSParmId  = MAX_DATA_RATE;
  CustomQOS.QOSItem[1].lQOSValue    = pInstance->ulAverageBytesPerSec;

  CustomQOS.QOSItem[2].lQOSParmId  = AVG_DATA_RATE;
  CustomQOS.QOSItem[2].lQOSValue    = pInstance->ulAverageBytesPerSec;;

  /* Tell the network io proc that we are about to start streaming */

  rc = mmioSendMessage( pInstance->hmmio,
                        MMIOM_BEGINSTREAM,
                        ( LONG ) lStreamDirection,
                        ( LONG ) &CustomQOS );



  return ( rc );

} /* BeginQualityofService */


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: EndQualityofService
*
* FUNCTION: Some OS/2 networks have specific IO procs which can
*           manage network traffic.  We will use the mmio messages
*           begin_stream and end_stream to inform these io procs
*           that we wish to stream across the network.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT: FUNCTION_PARM_BLOCK
*
* EXIT-NORMAL: MCIERR_SUCCESS
*
* EXIT_ERROR:
*
* EFFECTS:
*
* EXTERNAL REFERENCES: mmioSendMessage()    - MMIO function
*
*********************** END OF SPECIFICATIONS **********************/

ULONG   EndQualityofService ( INSTANCE      *pInstance )

{
LONG         rc;

  rc = mmioSendMessage( pInstance->hmmio,
                        MMIOM_ENDSTREAM,
                        0,
                        0 );

  return ( rc );

} /* EndQualityofService */


//#ifdef PTRFIX
//
///********************* START OF SPECIFICATIONS *********************
//*
//* SUBROUTINE NAME: DataTranslation
//*
//* FUNCTION: Some audio devices cannot support specific file formats
//*           (i.e. 16-bit on a SoundBlaster).  If the device cannot
//*           do the compression itself, ask the io proc we are
//*           connected to to decompress.
//*
//* NOTES:
//*
//* ENTRY POINTS:
//*
//* INPUT: FUNCTION_PARM_BLOCK
//*
//* EXIT-NORMAL: MCIERR_SUCCESS
//*
//* EXIT_ERROR:
//*
//* EFFECTS:
//*
//* EXTERNAL REFERENCES: mmioSendMessage()    - MMIO function
//*
//*********************** END OF SPECIFICATIONS **********************/
//
//ULONG   DataTranslation( INSTANCE      *pInstance )
//
//{
//LONG         rc;
//
//
//  rc = mmioSendMessage( pInstance->hmmio,
//                        MMIOM_TRANSLATETYPE,
//                        ( LONG ) &pInstance->mmAudioHeader,
//                        ( LONG ) pInstance->ulRealTimeTranslation );
//
//  if ( rc )
//     {
//     return (rc);
//     }
//
//  rc = GetAudioHeader( pInstance );
//
//  return ( rc );
//
//} /* EndQualityofService */
//
//#endif
