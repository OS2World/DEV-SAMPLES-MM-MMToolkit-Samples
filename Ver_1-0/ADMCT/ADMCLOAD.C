/*static char *SCCSID = "@(#)admcload.c	13.17 92/05/01";*/
/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCILOAD.C
*
* DESCRIPTIVE NAME: Audio MCD Load Element Routine.
*
* FUNCTION:Load an Waveform Element.
*
* NOTES:
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
* INTERNAL REFERENCES:    CreateNAssocStream ().
*                         DestroyStream().
*                         SetAudioDevice().
*                         VSDInstToWaveSetParms().
*                         PostMDMMessage ().
*                         OpenFile().
*
* EXTERNAL REFERENCES:    SpiStopStream  ().
*                         SpiAssociate   ().
*                         SpiDisableEvent().
*                         SpiSeekStream  ().
*                         mmioSendMessage().
*                         mmioClose ().
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
* SUBROUTINE NAME: MCILOAD.C
*
* DESCRIPTIVE NAME: Audio MCD Load Element Routine.
*
* FUNCTION:Load an Waveform Element.
*
* NOTES:
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
* INTERNAL REFERENCES:DestroyStream(), ReadRIFFWaveHeaderInfo()
*                     SetAmpDefaults(), SetWaveDeviceDefaults().
*                     SetAudioDevice().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS **********************/


RC MCILoad ( FUNCTION_PARM_BLOCK *pFuncBlock)
{

  ULONG                ulPathLength;        // length of path that we generated

  LONG                 lReturnCode;         // return code for mmio functions

  ULONG                ulrc;                // MME Propogated Error RC
  ULONG                ulParam1;            // Incoming MCI Flags
  ULONG                lCnt;                // Semaphore Posting Freq
  ULONG                ulErr1;              // Internal Error Conditions
  ULONG                ulAbortNotify = FALSE;// indicates whether to abort a previous
                                            // operation
  DWORD                dwLoadFlags;         // Incoming Flags Mask
  DWORD                dwSpiFlags;
  DWORD                dwSetAll;            //  Reinit AudioIF

  USHORT               usEvcbIndex;         // Evcb Counter

  INSTANCE *           ulpInstance;         // Local Instance//

  LPMCI_LOAD_PARMS     lpLoadParms;         // App MCI Data Struct
  MCI_WAVE_SET_PARMS   SetParms;            // App MCI Data Struct

  PBYTE                pDBCSName;           // pointer to the name of the file

  extern HHUGEHEAP     heap;                // Global MCD Heap
  CHAR                 TempPath[ CCHMAXPATH ]; // holds path for temp files

  /*********************************
  * Intialize Variables
  **********************************/
  ulrc = MCIERR_SUCCESS;
  ulErr1 = MCIERR_SUCCESS;
  dwLoadFlags = 0;
  /**************************************
  * Dereference Pointers
  **************************************/
  ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;
  ulParam1 = pFuncBlock->ulParam1;
  ulParam1 &= ~MCI_OPEN_ELEMENT;
  dwLoadFlags = ulParam1;
  /**************************************
  * Mask UnWanted Bits
  **************************************/
  dwLoadFlags &= ~MCI_NOTIFY;
  dwLoadFlags &= ~MCI_WAIT;
  dwLoadFlags &= ~MCI_OPEN_MMIO;
  dwLoadFlags &= ~MCI_READONLY;
  dwLoadFlags &= ~MCI_OPEN_ELEMENT;

  dwSetAll = MCI_WAVE_SET_BITSPERSAMPLE|MCI_WAVE_SET_FORMATTAG|
             MCI_WAVE_SET_CHANNELS | MCI_WAVE_SET_SAMPLESPERSEC;


  if (dwLoadFlags > 0 )
     {

     if ( ulParam1 & MCI_OPEN_PLAYLIST)
        {
        return MCIERR_UNSUPPORTED_FLAG;
        }

     return MCIERR_INVALID_FLAG;
     }

  /****************************************
  * Check For Valid Parameters
  ****************************************/
  ulrc = CheckMem ((PVOID)pFuncBlock->ulParam2,
                   sizeof (MCI_LOAD_PARMS), PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
          return MCIERR_MISSING_PARAMETER;

  /***************************************
  * Initialize values
  ****************************************/

  ulpInstance->ulCanSave   = TRUE;
  ulpInstance->ulCanInsert = TRUE;
  ulpInstance->ulCanRecord = TRUE;

  lpLoadParms = (LPMCI_LOAD_PARMS)pFuncBlock->ulParam2;

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

     // if there is a save pending, wait for it to complete

     if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
        {
        // Save is a non-interruptible operation
        // wait for completion

        DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

        }
      else
        {
        /****************************
        * Post Aborted Message
        *****************************/
        PostMDMMessage ( MCI_NOTIFY_ABORTED,
                         ulpInstance->usNotPendingMsg,
                         pFuncBlock);

        /***************************************
        * Reset Internal semaphores
        ****************************************/
        DosResetEventSem (ulpInstance->hEventSem, &lCnt);
        DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

        /**********************************
        * Stop Streaming
        **********************************/
        ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);

        if (!ulrc)
          {
          DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1 );
          }


        STRMSTATE = MCI_STOP;

        } /* if pending message not save */


     } /* notify pending is true */

  else

     {
     // stop may have done a stop pause, so ensure that another stop is done

     if ( STRMSTATE == STOP_PAUSED )
        {

        /***************************************
        * Reset Internal semaphores
        ****************************************/
        DosResetEventSem (ulpInstance->hEventSem, &lCnt);
        DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

        if ( AMPMIX.ulOperation == OPERATION_RECORD )
           {
           dwSpiFlags = SPI_STOP_FLUSH;
           }
        else
           {
           dwSpiFlags = SPI_STOP_DISCARD;
           }

        /**********************************
        * Stop Streaming
        **********************************/
        ulrc = SpiStopStream (STREAM.hStream, dwSpiFlags );
        if (!ulrc)
           DosWaitEventSem (ulpInstance->hEventSem, (ULONG) -1 );
        }

     }

  ulpInstance->ulOldStreamPos  = 0;

  /********************************************
  * Discard CuePoints since Element Changed
  ********************************************/
  if ((ulpInstance->usCuePt == TRUE) || (ulpInstance->usCuePt == EVENT_ENABLED)) {
     for (usEvcbIndex = 0; usEvcbIndex < STREAM.usCuePtIndex; usEvcbIndex++) {
          STREAM.MCuePtEvcb[usEvcbIndex].mmCuePt = 0;
          if (ulpInstance->ulCreateFlag == PREROLL_STATE)
              SpiDisableEvent (STREAM.HCuePtHndl[usEvcbIndex]);

     }
     ulpInstance->usCuePt = FALSE;

     /***************************
     * Reset CuePt Index to 0
     ***************************/
     STREAM.usCuePtIndex = 0;
  } /* There Were CuePoints */

  /*******************************************/
  // Disable Position Advise
  /*******************************************/

  if ((ulpInstance->usPosAdvise == TRUE) || (ulpInstance->usPosAdvise == EVENT_ENABLED)) {
      if (ulpInstance->ulCreateFlag == PREROLL_STATE)

          SpiDisableEvent (STREAM.hPosEvent);

      ulpInstance->usPosAdvise = FALSE;
  }



  /*****************************
  * Close Previous Element
  *****************************/
  if (ulpInstance->hmmio != (ULONG)NULL) {

      ulrc = mmioClose (ulpInstance->hmmio, 0);

      if ( ulpInstance->ulCreatedName )
         {
         DosDelete( ( PSZ ) ulpInstance->lpstrAudioFile );
         }

      ulpInstance->ulCreatedName = FALSE;

      ulpInstance->hmmio = (ULONG)NULL;

  } /* hmmio not Null */


  /***************************************
  * Check for valid element names
  ****************************************/

  if ( !( ulParam1 & MCI_OPEN_MMIO )  )
     {
     if ( lpLoadParms->lpstrElementName )
        {
        if (ulrc = CheckMem ( (PVOID)lpLoadParms->lpstrElementName,
                              1, PAG_WRITE))
           {
           return MCIERR_MISSING_PARAMETER;
           }
        ulpInstance->ulCreatedName = FALSE;
        }
      else
        {
        /**************************************
        * if the user requests a read only file
        * and we must create it, return error
        **************************************/

        if ( ulParam1 & MCI_READONLY )
           {
           return MCIERR_FILE_NOT_FOUND;
           }

        if (!( pDBCSName = HhpAllocMem (heap, CCHMAXPATH )))
           {
           return MCIERR_OUT_OF_MEMORY;
           }

        /**********************************************
        * Query the default path to place temp files and
        * generate a temporary file name
        **********************************************/

        ulrc = mciQuerySysValue( MSV_WORKPATH, pDBCSName );

        if ( !ulrc )
           {
           return MCIERR_INI_FILE;
           }

        ulPathLength = CCHMAXPATH;

        lReturnCode = GenerateUniqueFile( pDBCSName,
                                          &ulPathLength,
                                          &ulpInstance->hTempFile );

        if ( lReturnCode != MMIO_SUCCESS )
           {
           return MCIERR_FILE_NOT_FOUND;
           }

        ulpInstance->ulCreatedName = TRUE;


        } /* else the user did not pass in a name */

     } /* if the open mmio flag was not passed in */


  /*************************************************************
  * Destroy Previous Stream. (can be modified to just re assoc)
  *************************************************************/
  if (ulpInstance->ulCreateFlag == PREROLL_STATE)
     {

     /************************************
     * Destroy The Stream
     *************************************/
     DestroyStream (STREAM.hStream);

     /*************************************
     * Set Stream Creation Flag
     **************************************/

     ulpInstance->ulCreateFlag = CREATE_STATE;
     }  /* PreRoll State */

  /******************************************************
  * If hmmio is passed in just update instance copy of
  * hmmio, reset filexists flag and return success
  *****************************************************/
  if (ulParam1 & MCI_OPEN_MMIO) {
      ulpInstance->hmmio = (HMMIO)lpLoadParms->lpstrElementName;
      ulpInstance->mmioHndlPrvd = TRUE;
      ulpInstance->dwmmioOpenFlag = MMIO_READWRITE | MMIO_EXCLUSIVE;

      ulrc = OpenFile (ulpInstance, ulpInstance->dwmmioOpenFlag);
      if (ulrc)
          return (ulrc);

      ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY);
      if (ulrc)
          return ulrc;

      ulpInstance->usFileExists = TRUE;
      /*******************************************
      * Copy audio device attributes into MCI
      * Wave Set structure
      ********************************************/
      VSDInstToWaveSetParms ( &SetParms, ulpInstance);

      /**********************************************
      * Set the Audio device attributes.
      * dwSetAll is a flag set to waveaudio extensions
      *************************************************/
      ulrc = SetAudioDevice (ulpInstance, &SetParms, dwSetAll);

      if (ulrc)
           return (ulrc);

  }

  /***********************************
  * Temporary File creation Flags
  ************************************/

  if ( (ulParam1 & MCI_READONLY) || !ulpInstance->ulCanSave )
     {
     ulpInstance->ulOpenTemp = MCI_FALSE;
     }
  else
     {
     ulpInstance->ulOpenTemp = MCI_TRUE;
     }

  ulpInstance->ulUsingTemp = MCI_FALSE;


  if ( !( ulParam1 & MCI_OPEN_MMIO ) )

      {
       /********************************************
       *  Flag Media Present  as true
       **********************************************/
       ulpInstance->usMediaPresent = MCI_TRUE;

       /********************************************
       *  Copy the file to load, and check to see if
       *  we should copy the name passed in or copy
       *  the one the one we generated if the name
       *  was null
       **********************************************/

       if ( lpLoadParms->lpstrElementName )
          {
          strcpy ( (PSZ)ulpInstance->lpstrAudioFile,
                   (PSZ)lpLoadParms->lpstrElementName);
          }
       else
          {
          strcpy ( (PSZ)ulpInstance->lpstrAudioFile,
                   pDBCSName );

          }
       /***********************************************
       * Find out if the File Exists.
       ***************************************************/
       ulpInstance->dwmmioOpenFlag = MMIO_READ | MMIO_DENYNONE;

        /******************************************************
        * If the user wants to open temp, then modify the flags
        ******************************************************/
        if ( ulpInstance->ulOpenTemp )
           {
           ulpInstance->dwmmioOpenFlag |= MMIO_READWRITE | MMIO_EXCLUSIVE;
           ulpInstance->dwmmioOpenFlag &= ~MMIO_DENYNONE;
           ulpInstance->dwmmioOpenFlag &= ~MMIO_READ;
           }


       ulrc = OpenFile( ulpInstance, ulpInstance->dwmmioOpenFlag );


       if ( !ulrc )
         {
         if ( ulpInstance->ulCreatedName )
            {
            AMPMIX.ulOperation = OPERATION_RECORD;
            }
         else
            {
            AMPMIX.ulOperation = OPERATION_PLAY;
            }

         ulpInstance->usFileExists = TRUE;
         }
       else
         {
         if (ulrc == ERROR_FILE_NOT_FOUND)
            {
            /********************************
            * if this is a read only file then
            * we cannot create a new one
            *********************************/

            if ( ulParam1 & MCI_READONLY )
               {
               ulpInstance->usFileExists = FALSE;
               return MCIERR_FILE_NOT_FOUND;
               }

            AMPMIX.ulOperation = OPERATION_RECORD;
            ulpInstance->ulCreateFlag = CREATE_STATE;
            ulrc = MCIERR_SUCCESS;
            ulpInstance->dwmmioOpenFlag = MMIO_CREATE | MMIO_READWRITE | MMIO_EXCLUSIVE;

            /********************************
            * Open The Element
            *******************************/

            ulrc = OpenFile ( ulpInstance,
                              ulpInstance->dwmmioOpenFlag);

            if (ulrc)
               {
               ulpInstance->usFileExists = FALSE;

               if ( ulrc == ERROR_FILE_NOT_FOUND )
                  {
                  return MCIERR_FILE_NOT_FOUND;
                  }
               else
                  {
                  return (ulrc);
                  }
               } /* if an error occurred */



            ulpInstance->usFileExists = TRUE;
            SetWaveDeviceDefaults (ulpInstance, OPERATION_RECORD);
            ulrc = MCIERR_SUCCESS;

            }
         else
            {
            ulpInstance->usFileExists = FALSE;

            // copy back the name we had before


            return (ulrc);
            }

         } /* else there was an error on the load open */

      // ensure that the mode and other actions are set

      /*************************************
      * Since we did not create the file
      *  the open file routines are not smart
      * enough to figure out how to set the
      * defaults
      *************************************/
      if ( ulpInstance->ulCreatedName )
         {
         AMPMIX.sChannels =    NOT_INTIALIZED;
         AMPMIX.sMode =        NOT_INTIALIZED;
         AMPMIX.lSRate =       NOT_INTIALIZED;
         AMPMIX.lBitsPerSRate = 0;

         ulpInstance->ulAverageBytesPerSec = NOT_INTIALIZED;
         }

      ulrc = InitAudioDevice (ulpInstance, AMPMIX.ulOperation );
      if (ulrc)
          return ulrc;

       VSDInstToWaveSetParms ( &SetParms, ulpInstance);

       ulrc = SetAudioDevice (ulpInstance, &SetParms, dwSetAll);

       if (ulrc)
           return (ulrc);

       }       /* Element Specified on Load */


  /***********************************************************
  * perform this after the open because open file will modify
  * the state of the following flags
  ***********************************************************/

  if ( ulParam1 & MCI_READONLY )
     {
     ulpInstance->ulCanSave = MCI_FALSE;
     ulpInstance->ulCanRecord = MCI_FALSE;
     }


  /*****************************************************
  * Currently return true if a playlst strm was created
  *****************************************************/
  if (ulpInstance->usPlayLstStrm == TRUE) {
      if (ulrc = SpiGetHandler((PSZ)"FSSH", &(STREAM.hidASource),
                               &(STREAM.hidATarget)))
          return ulrc;

      ulpInstance->usPlayLstStrm = FALSE;

  }  /* Trash the old Stream Handler Handles */

  /*************************************************
  * Reassociate The Stream Handlers with the new
  * stream object
  *************************************************/
  if (ulpInstance->ulCreateFlag == PREROLL_STATE) {

      /*********************************************
      * Fill in Associate Control Block Info
      *********************************************/
      STREAM.acbmmio.ulObjType = ACBTYPE_MMIO;
      STREAM.acbmmio.ulACBLen = sizeof (ACB_MMIO);
      STREAM.acbmmio.hmmio = ulpInstance->hmmio;

      /***********************************************
      * Associate FileSystem as source if Playing
      ***********************************************/
      if (AMPMIX.ulOperation == OPERATION_PLAY)
         {
         if (ulrc = SpiAssociate ((HSTREAM)STREAM.hStream,
                                   STREAM.hidASource,
                                   (PVOID) &(STREAM.acbmmio)))
              return ulrc;
      /******************************************************************
      * We need to seek to 0 to reset the target stream handlers position
      *******************************************************************/
         ulrc = ( STREAM.hStream,
                                SPI_SEEK_ABSOLUTE,
                                0L );
         if (ulrc)
            return (ulrc);


         }

      /***********************************************
      * Associate FileSystem as Target if Recording
      ***********************************************/
      if (AMPMIX.ulOperation == OPERATION_RECORD) {
          if (ulrc = SpiAssociate ((HSTREAM)STREAM.hStream,
                                   STREAM.hidATarget,
                                   (PVOID) &(STREAM.acbmmio)))
            return ulrc;
      } /* Record */
  } /* Preoll State Flag */

  // ulpInstance->usFileExists = TRUE;

  /**********************************************
  * If the temp flag has sent, open the temp file
  **********************************************/

  if ( ulpInstance->ulOpenTemp )
    {

    /**********************************************
    * Query the default path to place temp files and
    * pass it on to the IO Proc
    **********************************************/

    ulrc = mciQuerySysValue( MSV_WORKPATH, TempPath );

    if ( !ulrc )
       {
       return MCIERR_INI_FILE;
       }

    ulrc = mmioSendMessage( ulpInstance->hmmio,
                            MMIOM_TEMPCHANGE,
                            ( LONG ) TempPath,
                            0 );
    if ( ulrc )
      {
      ulrc = mmioGetLastError( ulpInstance->hmmio );
      if ( ulrc == MMIOERR_CANNOTWRITE )
         {

         mmioClose( ulpInstance->hmmio, 0 );
         ulpInstance->usFileExists = FALSE;

         return MCIERR_TARGET_DEVICE_FULL;
         }
      else
         {
         return ulrc;
         }
      }
    ulpInstance->ulUsingTemp = MCI_TRUE;

    } /* if ulOpenTemp */


  STRMSTATE = MCI_STOP;

  return (ULONG)(ulrc);

}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCICNCT.c
*
* DESCRIPTIVE NAME: Audio MCD Connections.
*
* FUNCTION:Process Connector Message.
*
* NOTES:
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
* INTERNAL REFERENCES:
*                     DestroyStream(),
*                     SetAmpDefaults(),
*                     SetWaveDeviceDefaults().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS **********************/
RC MCICnct ( FUNCTION_PARM_BLOCK *pFuncBlock)
{
   ULONG            ulrc;              // RC
   ULONG            ulParam1;          // Incoming MCI Flags
   ULONG            ulConnectorType;   // Struct to hold connector flag
   ULONG            ulHoldConFlag;     // flag to hold enable/disable etc.
   ULONG            ulFlagCount;
   INSTANCE *       ulpInstance;        // Local Instance

   LPMCI_CONNECTOR_PARMS lpConParms;   // MCI Msg Data

   ulrc = MCIERR_SUCCESS;
   ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;
   ulParam1 = pFuncBlock->ulParam1;

   lpConParms = (LPMCI_CONNECTOR_PARMS)pFuncBlock->ulParam2;


   /****************************************
   * Check For Valid Parameters
   *****************************************/

   ulrc = CheckMem ( (PVOID) lpConParms,
                     sizeof (MCI_CONNECTOR_PARMS),
                     PAG_READ);

   if ( ulrc != MCIERR_SUCCESS )
     {
     return MCIERR_MISSING_PARAMETER;
     }

   if ( ( ulParam1 & MCI_NOTIFY ) && ( ulParam1 & MCI_WAIT ) )
      {
      return MCIERR_INVALID_FLAG;
      }

   /****************************************
   * Remember if we have to notify
   *****************************************/
   ulParam1 &= ~( MCI_NOTIFY + MCI_WAIT );

   /****************************************
   * Ensure a command flag was sent
   *****************************************/

   if ( !ulParam1 )
     {
     return MCIERR_MISSING_FLAG;
     }

   ulConnectorType = ( ulParam1 & MCI_CONNECTOR_TYPE ) ? 1 : 0;

   ulHoldConFlag = ulParam1 & ~( MCI_CONNECTOR_TYPE| MCI_CONNECTOR_INDEX);

   ulFlagCount = ( ulParam1 & MCI_ENABLE_CONNECTOR ) ? 1 : 0;
   ulFlagCount += ( ulParam1 & MCI_DISABLE_CONNECTOR ) ? 1 : 0;
   ulFlagCount += ( ulParam1 & MCI_QUERY_CONNECTOR_STATUS ) ? 1 : 0;

   if ( ulFlagCount > 1 )
      return MCIERR_FLAGS_NOT_COMPATIBLE;

   // if they want to set a type

   if ( ulConnectorType )
         {
         if ( ulParam1 & MCI_CONNECTOR_INDEX && ( lpConParms->dwConnectorIndex != 1) )
              {
              if ( lpConParms->dwConnectorType != MCI_SPEAKERS_CONNECTOR )
                 {
                 return MCIERR_INVALID_CONNECTOR_INDEX;
                }
               else
                 {
                 if ( lpConParms->dwConnectorIndex != 2 )
                    {
                    return MCIERR_INVALID_CONNECTOR_INDEX;
                    }
                 }
              }

          if ( ulHoldConFlag & ~( MCI_ENABLE_CONNECTOR  |
                                  MCI_DISABLE_CONNECTOR |
                                  MCI_QUERY_CONNECTOR_STATUS ) )
             {
             return MCIERR_INVALID_FLAG;
             }
         switch( ulHoldConFlag )
                 {
                 /*-------------------------------------------------------------*
                 * Enable connector
                 *-------------------------------------------------------------*/

                 case MCI_ENABLE_CONNECTOR:
                       if ( ulConnectorType )
                         {
                         switch( lpConParms->dwConnectorType )
                                 {
                                 // since we know that we are connected to an amp/mixer
                                 // we can pass the command on

                                 case MCI_LINE_IN_CONNECTOR:
                                 case MCI_AUDIO_IN_CONNECTOR   :
                                 case MCI_MICROPHONE_CONNECTOR:
                                 case MCI_LINE_OUT_CONNECTOR:
                                 case MCI_AUDIO_OUT_CONNECTOR  :
                                 case MCI_SPEAKERS_CONNECTOR:
                                 case MCI_HEADPHONES_CONNECTOR:

                                    ulrc = mciSendCommand ( ulpInstance->wAmpDeviceID,
                                                           (USHORT) MCI_CONNECTOR,
                                                           ulParam1,
                                                           (DWORD) lpConParms,
                                                           pFuncBlock->usUserParm );
                                    break;
                                 case MCI_WAVE_STREAM_CONNECTOR:

                                    ulrc = MCIERR_CANNOT_MODIFY_CONNECTOR;
                                    break;
                                 case MCI_MIDI_STREAM_CONNECTOR:
                                 case MCI_AMP_STREAM_CONNECTOR:
                                 case MCI_CD_STREAM_CONNECTOR  :
                                 case MCI_VIDEO_IN_CONNECTOR   :
                                 case MCI_VIDEO_OUT_CONNECTOR  :
                                 case MCI_PHONE_SET_CONNECTOR  :
                                 case MCI_PHONE_LINE_CONNECTOR :
                                 case MCI_UNIVERSAL_CONNECTOR  :

                                    ulrc = MCIERR_UNSUPPORTED_CONN_TYPE;
                                    break;
                                 default:
                                         ulrc = MCIERR_INVALID_CONNECTOR_TYPE;
                                         break;
                                 }
                         }
                       break;

                 /*-------------------------------------------------------------*
                 * Disable connector
                 *-------------------------------------------------------------*/

                 case MCI_DISABLE_CONNECTOR:
                       if ( ulConnectorType )
                         {
                         switch( lpConParms->dwConnectorType )
                                 {
                                 case MCI_LINE_IN_CONNECTOR:
                                 case MCI_AUDIO_IN_CONNECTOR   :
                                 case MCI_MICROPHONE_CONNECTOR:
                                 case MCI_LINE_OUT_CONNECTOR:
                                 case MCI_AUDIO_OUT_CONNECTOR  :
                                 case MCI_SPEAKERS_CONNECTOR:
                                 case MCI_HEADPHONES_CONNECTOR:

                                    ulrc = mciSendCommand ( ulpInstance->wAmpDeviceID,
                                                           (USHORT) MCI_CONNECTOR,
                                                           ulParam1,
                                                           (DWORD) lpConParms,
                                                           pFuncBlock->usUserParm );
                                         break;
                                 case MCI_WAVE_STREAM_CONNECTOR:

                                    ulrc = MCIERR_CANNOT_MODIFY_CONNECTOR;
                                    break;
                                 case MCI_MIDI_STREAM_CONNECTOR:
                                 case MCI_AMP_STREAM_CONNECTOR:
                                 case MCI_CD_STREAM_CONNECTOR  :
                                 case MCI_VIDEO_IN_CONNECTOR   :
                                 case MCI_VIDEO_OUT_CONNECTOR  :
                                 case MCI_PHONE_SET_CONNECTOR  :
                                 case MCI_PHONE_LINE_CONNECTOR :
                                 case MCI_UNIVERSAL_CONNECTOR  :

                                    ulrc = MCIERR_UNSUPPORTED_CONN_TYPE;
                                    break;

                                 default:
                                         ulrc = MCIERR_INVALID_CONNECTOR_TYPE;
                                         break;
                                 } /* switch the connector type */


                         } /* if caller wants to know the type */
                       break;
                 /*-------------------------------------------------------------*
                 * QUERY conn     ector
                 *-------------------------------------------------------------*/

                 case MCI_QUERY_CONNECTOR_STATUS :
                       if ( ulConnectorType )
                         {

                         switch( lpConParms->dwConnectorType )
                                 {
                                 case MCI_LINE_IN_CONNECTOR:
                                 case MCI_AUDIO_IN_CONNECTOR   :
                                 case MCI_MICROPHONE_CONNECTOR:
                                 case MCI_LINE_OUT_CONNECTOR:
                                 case MCI_AUDIO_OUT_CONNECTOR  :
                                 case MCI_SPEAKERS_CONNECTOR:
                                 case MCI_HEADPHONES_CONNECTOR:

                                    ulrc = mciSendCommand ( ulpInstance->wAmpDeviceID,
                                                           (USHORT) MCI_CONNECTOR,
                                                           ulParam1,
                                                           (DWORD) lpConParms,
                                                           pFuncBlock->usUserParm );
                                    break;

                                 case MCI_WAVE_STREAM_CONNECTOR:
                                   if ( ulParam1 & MCI_CONNECTOR_INDEX )
                                     {
                                     if (lpConParms->dwConnectorIndex != 1)

                                        {
                                        return MCIERR_INVALID_CONNECTOR_INDEX;
                                        }
                                   }
                                   lpConParms->dwReturn = TRUE;
                                   ulrc = MAKEULONG( MCIERR_SUCCESS, MCI_TRUE_FALSE_RETURN );
                                   break;
                                 case MCI_MIDI_STREAM_CONNECTOR:
                                 case MCI_AMP_STREAM_CONNECTOR:
                                 case MCI_CD_STREAM_CONNECTOR  :
                                 case MCI_VIDEO_IN_CONNECTOR   :
                                 case MCI_VIDEO_OUT_CONNECTOR  :
                                 case MCI_PHONE_SET_CONNECTOR  :
                                 case MCI_PHONE_LINE_CONNECTOR :
                                 case MCI_UNIVERSAL_CONNECTOR  :

                                    ulrc = MCIERR_UNSUPPORTED_CONN_TYPE;
                                    break;

                                 default:
                                         ulrc = MCIERR_INVALID_CONNECTOR_TYPE;
                                         break;
                                 }

                         }

                       break;

                 /*-------------------------------------------------------------*
                 * Unknown request
                 *-------------------------------------------------------------*/

                 default:
                         ulrc = MCIERR_MISSING_FLAG;
                         break;
                 } /* switch the connector type */

         } /* if !ulrc */
  else if ( ulParam1 & MCI_CONNECTOR_INDEX )
     {
     if (lpConParms->dwConnectorIndex != 1)

        {
        return MCIERR_INVALID_CONNECTOR_INDEX;
        }

     if (  ( ulParam1 & ~( MCI_CONNECTOR_INDEX + MCI_QUERY_CONNECTOR_STATUS) ) > 0  )
       {
       return MCIERR_INVALID_FLAG;
       }


     if ( !(ulParam1 & MCI_QUERY_CONNECTOR_STATUS)  )
        {
        return MCIERR_MISSING_FLAG;
        }

     lpConParms->dwReturn = TRUE;
     ulrc = MAKEULONG( MCIERR_SUCCESS, MCI_TRUE_FALSE_RETURN );

     } /* see if the index only flag is set */
  else
     // else there was neither a connector index nor a connector type
     // must be one or both
     {
     if ( !(ulParam1 & MCI_QUERY_CONNECTOR_STATUS)  )
        {
        return MCIERR_MISSING_FLAG;
        }

     lpConParms->dwReturn = TRUE;
     ulrc = MAKEULONG( MCIERR_SUCCESS, MCI_TRUE_FALSE_RETURN );
     }


   return (ulrc);
}
















/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: AllocMessageParmMem
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Allocate Memory for MCI Message parameter when MCI_NOTIFY
*           Flag is On..
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
* INTERNAL REFERENCES: HhpAllocMem().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
ULONG   AllocNCopyMessageParmMem (USHORT usMessage, FUNCTION_PARM_BLOCK * pFuncBlock, ULONG ulCopyFrom, USHORT usUserParm)
{

  extern HHUGEHEAP        heap;      // Global Heap
  LPMCI_PLAY_PARMS        prPLY;     // MCI Play Params Pointer
  LPMCI_RECORD_PARMS      prREC;     // MCI Record Parms Pointer


  /******************************************************/
  // Based on message Allocate the appropriate structure
  /*****************************************************/

  pFuncBlock->ulNotify = TRUE;
  usUserParm = usUserParm;


  switch (usMessage) {

  case MCI_PLAY:
          {
                  if (!(prPLY = HhpAllocMem (heap, sizeof (MCI_PLAY_PARMS))))
                          return MCIERR_OUT_OF_MEMORY;


                  /***************************************************/
                  // Copy DriverEntry Parms into Prvt Strct
                  /***************************************************/
                  memcpy (prPLY, (LPMCI_PLAY_PARMS)ulCopyFrom, sizeof (MCI_PLAY_PARMS));
                  pFuncBlock->ulParam2 = (ULONG)prPLY;

          }
          break;

  case MCI_RECORD:
          {
                  if (!(prREC = HhpAllocMem (heap,sizeof (MCI_RECORD_PARMS))))
                          return MCIERR_OUT_OF_MEMORY;
                  /***************************************************/
                  // Copy DriverEntry Parms into Prvt Strct
                  /***************************************************/
                  memcpy (prREC, (LPMCI_RECORD_PARMS)ulCopyFrom, sizeof (MCI_RECORD_PARMS));
                  pFuncBlock->ulParam2 = (ULONG)prREC;
          }
          break;

  } /* of Switch */

  return (ULONG)(MCIERR_SUCCESS);
}
