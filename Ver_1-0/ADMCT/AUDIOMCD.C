/*********************** END OF SPECIFICATIONS *******************************
*
* SOURCE FILE NAME:  AUDIOMCD.C
*
* DESCRIPTIVE NAME:  WAVEFORM MCD (PLAYER)
*
*              Copyright (c) IBM Corporation  1991
*                        All Rights Reserved
*
* STATUS: MM Extensions 1.0
*
* FUNCTION: Support waveform MCI messages. This module accepts input from
*           MDM. This MCI driver communicates with SSM using SPIs for creation,
*           operation, and destruction of data streams. It also invokes device
*           specific DLLs (e.g  AUDIOIF)  to obtain device specific information
*           like device capabilities, status and so on. It supports play back
*           and recording of audio waveform data.
*
*
* NOTES:    THIS MODULE RESIDES AT RING 3
*
*    DEPENDENCIES: NONE
*    RESTRICTIONS: NONE
*
* EXTERNAL ENTRY POINTS:
*
*
* INTERNAL ENTRY POINTS:
*                    ADMC_INIT () -- Custom DLL Intialization Routine
*                    ADMC_EXIT () --  DLL Termination Routine // Crntly Disabled
*
*
*
* EXTERNAL REFERENCES (system):
*
*
*
* MODIFICATION HISTORY:
* DATE      DEVELOPER         CHANGE DESCRIPTION
*********************** END OF SPECIFICATIONS ********************************/
#define INCL_BASE
#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES

#include <os2.h>                        // OS2 defines.
#include <string.h>
#include <os2medef.h>                   // MME includes files.
#include <stdlib.h>                     // Math functions
#include <audio.h>                      // Audio Device defines
#include <ssm.h>                        // SSM Spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmsystem.h>                   // MM System Include.
#include <mcipriv.h>                    // MCI Connection stuff
#include <mcidrv.h>                     // MCI Driver include.
#include <mmio.h>                       // MMIO Include.
#include <mcd.h>                        // AudioIFDriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions
#include <audiomcd.h>                   // Component Definitions.
#include "admcfunc.h"                   // Function Prototypes.

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: mciDriverEntry
*
* DESCRIPTIVE NAME: Single Entry point into Waveform MCD Player.
*
* FUNCTION: Process MCI Waveform messages.
*
* NOTES: Device specific information is obtained via the
*        VSDIDriverEntry interface.
*
* ENTRY POINTS:  mciDriverEntry ()
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_MESSAGES.
*
* PARAMETERS:   ulpInstance  - pointer to waveform  instance structure.
*               usMessage    - MCI message to act on.
*               ulParam1     - MCI Flags for this message.
*               ulParam2     - Message  data.
*               usUserParm   - User Parameter Returned on Notifications.
*
* EXIT-NORMAL: Return Code 0
*
* EXIT_ERROR:  MME Error Code
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*                      ADMCINIT, MCIOPEN, MCDCAPS, MCISET,
*                      MCISTAT, MCICUE,  MCISEEK, MCIPLAY,
*                      MCISCPT, MCDINFO,  MCIPAUS, MCICLOS.
*                      MCISTOP
*
* EXTERNAL REFERENCES: mdmDriverNotify, SSM SPIs, MMIO APIs, AudioIFDriverEntry
*                      heap Allocation and management APIs
*                      OS/2 APIs
*
*********************** END OF SPECIFICATIONS ********************************/

ULONG   EXPENTRY mciDriverEntry (LPTR ulpInstance,       // Instance Ptr
                                 USHORT usMessage,       // Message
                                 ULONG ulParam1,         // Flags
                                 ULONG ulParam2,         // Data
                                 USHORT usUserParm)      // Data
{

  ULONG                   ulrc;                // RC
  ULONG                   ulErrorCode;         // RC Internal
  ULONG                   ulPathLength;        // length of path that we generated

  LONG                    lReturnCode;         // return code for mmio functions

  PBYTE                   pDBCSName;           // pointer to the name of the file

  MMDRV_OPEN_PARMS        *pdrv_open_parms;    // MDM Driver Open Parms
  MMDRV_OPEN_PARMS        *mdmpdrv_open_parms; // MDM MCI Driver Data
  FUNCTION_PARM_BLOCK     *pFuncBlock;       // Local Info Block
  extern HHUGEHEAP        heap;                // Global MCD Heap
  extern HEV              hEventInitCompleted; // DLL Intialization
  extern ULONG            lPost;               // Posting Frequency
  extern ULONG            ulWaitForInit;       // Intialization Stuff

  ulrc =        MCIERR_SUCCESS;
  ulErrorCode = MCIERR_SUCCESS;

  if ( usMessage == MCI_MASTERAUDIO )
     {
     return MCIERR_SUCCESS;
     }

  /************************************************************************
  * Force DLL Intialization to Complete.If The EntryPoint is called before
  * Intialization complets, The Calling thread is blocked on a semaphore
  * indefinitely waiting.
  ************************************************************************/


  if ((heap == (ULONG)NULL) || (ulWaitForInit == TRUE)) {
      if (hEventInitCompleted == (ULONG)NULL) {

          /*****************************
          * Dont Task me Out
          *****************************/
          DosEnterCritSec ();
          ulrc = DosCreateEventSem ((ULONG)NULL,
                                    (PHEV)&(hEventInitCompleted),
                                    DC_SEM_SHARED,
                                    FALSE);

          /****************************
          * Resume Normal Tasking
          ****************************/
          DosExitCritSec ();
          if (ulrc)
               return (ulrc);
          /**************************************
          * Reset Intialization Semaphores Used
          ****************************************/
          DosResetEventSem (hEventInitCompleted, &lPost);

      } /* Event Sem was not Created */
      /*********************************
      * Wait on the Intialization Sem
      **********************************/
      DosWaitEventSem (hEventInitCompleted, -1);
      ulWaitForInit = FALSE;

  }  /* Heap Was Null */


  switch (usMessage)
  {
     /*************************************
     * Unsupported Functions
     *************************************/

  case MCI_ACQUIREDEVICE:
  case MCI_DEVICESETTINGS:
  case MCI_ESCAPE:
  case MCI_RELEASEDEVICE:
  case MCI_STEP:
  case MCI_SYSINFO:
  case MCI_SPIN:
  case MCI_UPDATE:
  case MCI_GETTOC:
          return MCIERR_UNSUPPORTED_FUNCTION;
          break;
  }
  /***************************************
  * Allocate A Function Parameter Block.
  ***************************************/
  ulrc = AcquireProcSem ();

  if (!(pFuncBlock = HhpAllocMem (heap,
                        sizeof (FUNCTION_PARM_BLOCK))))
  {
          ReleaseProcSem ();
          return MCIERR_OUT_OF_MEMORY;
  }

  /******************************************
  * Copy The default parameters.
  /******************************************/
  pFuncBlock->usMessage = usMessage;
  pFuncBlock->ulpInstance = ulpInstance;
  pFuncBlock->pInstance = (INSTANCE *)ulpInstance;
  pFuncBlock->usUserParm = usUserParm;
  pFuncBlock->ulParam1 = ulParam1;
  pFuncBlock->ulParam2 = ulParam2;

  ReleaseProcSem ();
  /************************************
  * Check For Illegal Flags
  ************************************/
  if (ulrc = CheckFlags (ulParam1))
      return (ulrc);

  /************************************
  * Asynchronous messages
  *************************************/
  if (!((ulParam1 & MCI_WAIT) || (ulParam1 & MCI_NOTIFY)))
     {
     if (usMessage != MCI_OPEN)
        {
          /*****************************
          * Default to a Notify
          ******************************/
          pFuncBlock->ulParam1 |= MCI_NOTIFY;
          /*******************************************
          * Set The Asynch message flag. This flag is
          * visible to PostMDMMessage which does not
          * call mdmDriverNotify if set.
          ********************************************/

           if ( usMessage == MCI_PLAY || usMessage == MCI_RECORD )
               {
               pFuncBlock->pInstance->dwCallback = 0;
               }
           else
               {
               pFuncBlock->dwCallback = 0;
               }

         }   /* Not Open */
     } /* Not wait or Notify */


  /************************************************************************
  * Request The Instance Data Access Sem and block all incoming Threads on
  * this semaphore. In case of MCI_PLAY and MCI_RECORD an earlier Release
  * of the Semaphore occurs. All other Incoming MCI Messages on different
  * threads are serialized.
  ************************************************************************/


  if (usMessage != MCI_OPEN )

      {
      if ( !(usMessage == MCI_CLOSE  &&
           ulParam1 & MCI_CLOSE_EXIT ) )
         {
         AbortWaitOperation ((INSTANCE *)ulpInstance);

         MCD_EnterCrit ((INSTANCE *) ulpInstance);
         }
      }

  /****************************************
  * Allocate Memory on a Notify Thread
  ****************************************/
  if (ulParam1 & MCI_NOTIFY)
      {

      if (usMessage != MCI_OPEN)
          {

          if ( usMessage == MCI_PLAY )
             {
             ulrc = CheckMem ((PVOID)ulParam2,
                           sizeof (MCI_PLAY_PARMS),
                           PAG_READ|PAG_WRITE);

             if (ulrc != MCIERR_SUCCESS)
                {
                MCD_ExitCrit ((INSTANCE *) ulpInstance);
                return MCIERR_MISSING_PARAMETER;
                }
             }
          else if ( usMessage == MCI_RECORD )
             {
             ulrc = CheckMem ((PVOID)ulParam2,
                           sizeof (MCI_RECORD_PARMS),
                           PAG_READ|PAG_WRITE);

             if (ulrc != MCIERR_SUCCESS)
                {
                MCD_ExitCrit ((INSTANCE *) ulpInstance);
                return MCIERR_MISSING_PARAMETER;
                }
             }

          /********************************
          * Turn The Notify Flag On
          *********************************/
          pFuncBlock->ulNotify = TRUE;

          /*********************************
            * Get The Window Callback Handle
          *********************************/
          if ( usMessage == MCI_PLAY || usMessage == MCI_RECORD )
             {
             pFuncBlock->pInstance->dwCallback = (((LPMCI_GENERIC_PARMS)ulParam2)->dwCallback);
             }
          else
             {
             pFuncBlock->dwCallback = (((LPMCI_GENERIC_PARMS)ulParam2)->dwCallback);
             }

          } /* Message .NE. MCI Open */
       } /* Notify Flag is On */

    if ( pFuncBlock->ulParam1 & MCI_NOTIFY )
       {
       if ((usMessage == MCI_PLAY)|| (usMessage == MCI_RECORD))
          {
          ulrc = AllocNCopyMessageParmMem (usMessage,
                                           pFuncBlock,
                                           ulParam2,
                                           usUserParm);
          }
       }


  switch (usMessage)
  {

  case MCI_OPEN:
       {
       /********************************
       * Check for valid Mem
       ********************************/
       if (ulrc = CheckMem ((PVOID)ulParam2,
                            sizeof (DWORD),
                            PAG_READ))

           return MCIERR_MISSING_PARAMETER;


       AcquireProcSem ();
       /**************************************
       * Waveform audio instance structure
       ****************************************/
       if (!(pFuncBlock->pInstance = HhpAllocMem (heap,
                                                    sizeof (INSTANCE)))) {

           HhpFreeMem (heap, pFuncBlock);

           ReleaseProcSem ();
           return MCIERR_OUT_OF_MEMORY;
       }
       /*********************************
       * MCI Driver Open Structure.
       **********************************/
       if (!(pdrv_open_parms = HhpAllocMem (heap,
                               sizeof (MMDRV_OPEN_PARMS)))) {

           HhpFreeMem (heap, pFuncBlock->pInstance);
           HhpFreeMem (heap, pFuncBlock);

           ReleaseProcSem ();

           return MCIERR_OUT_OF_MEMORY;
       }
       /**********************************************
       * Copy Parameters into Private structs.
       **********************************************/

       mdmpdrv_open_parms = (MMDRV_OPEN_PARMS *)ulParam2;

       memcpy (pdrv_open_parms, mdmpdrv_open_parms,
               sizeof (MMDRV_OPEN_PARMS));

       ReleaseProcSem ();

       if (ulParam1 & MCI_OPEN_PLAYLIST &&
           ulParam1 & MCI_OPEN_ELEMENT) {

           ReleaseInstanceMemory (pFuncBlock);

           return (MCIERR_FLAGS_NOT_COMPATIBLE);
       }
       /***************************************
       * Audio File Name If Specified
       ***************************************/
       if (ulParam1 & MCI_OPEN_ELEMENT)
          {

            /***************************************
            * Check the pointer if the user passed
            * in a file name
            ***************************************/
           if ( mdmpdrv_open_parms->lpstrElementName )
              {
              ulrc = CheckMem ( (PVOID)mdmpdrv_open_parms->lpstrElementName,
                                1, PAG_WRITE);
              if (ulrc)
                 {

                 ReleaseInstanceMemory (pFuncBlock);
                 /*******************************
                 * Return File Not Found
                 *******************************/
                 return MCIERR_FILE_NOT_FOUND;
                 }
              pFuncBlock->pInstance->ulCreatedName = FALSE;

              if (mdmpdrv_open_parms->lpstrElementName != (ULONG)NULL)
                 {
                  strcpy ( pFuncBlock->pInstance->lpstrAudioFile,
                           (PSZ)mdmpdrv_open_parms->lpstrElementName);

                  pdrv_open_parms->lpstrElementName =
                           (LPSTR)mdmpdrv_open_parms->lpstrElementName;

               }
              }
           else
              {
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
                 return MCIERR_INVALID_WORKPATH;
                 }

              ulPathLength = CCHMAXPATH;

              lReturnCode = GenerateUniqueFile( pDBCSName,
                                                &ulPathLength,
                                                &pFuncBlock->pInstance->hTempFile );

              if ( lReturnCode != MMIO_SUCCESS )
                 {
                 return MCIERR_INVALID_WORKPATH;
                 }

               // copy the name we just generated for future storage

               strcpy ( pFuncBlock->pInstance->lpstrAudioFile, pDBCSName );

               pdrv_open_parms->lpstrElementName = ( LPSTR ) pDBCSName;


               pFuncBlock->pInstance->ulCreatedName = TRUE;
               }

          }   /* Open Element */

       /****************************
       * Memory Play List Case
       *****************************/
       if (ulParam1 & MCI_OPEN_PLAYLIST) {
           pdrv_open_parms->lpstrElementName =
                    (LPSTR)mdmpdrv_open_parms->lpstrElementName;
       }
       /*****************************
       * Open MMIO Handle
       *****************************/
       if (ulParam1 & MCI_OPEN_MMIO) {
           pdrv_open_parms->lpstrElementName =
                    (LPSTR)mdmpdrv_open_parms->lpstrElementName;
       }
       /****************************************
       * Set Thread Parm Block Pointers.
       *****************************************/
       pFuncBlock->ulParam2 =   (ULONG) pdrv_open_parms;
       pFuncBlock->usUserParm = usUserParm;

       /**********************************************
       * MDM Return Information.
       * Pointer to this instance
       * Resource Units Consumed by this Instance.
       ***********************************************/
       mdmpdrv_open_parms->pInstance = (LPTR) pFuncBlock->pInstance;
       mdmpdrv_open_parms->usResourceUnitsRequired = 0;

       pFuncBlock->ulpInstance = (LPTR) pFuncBlock->pInstance;
       /****************************
       * Set Notify Flag
       ***************************/
       if (ulParam1 & MCI_NOTIFY)
          {
          pFuncBlock->ulNotify = TRUE;
          pFuncBlock->dwCallback = (((LPMCI_GENERIC_PARMS)ulParam2)->dwCallback);
          }
       else
          {
          pFuncBlock->ulNotify = FALSE;
          }

       if (ulParam1 & MCI_OPEN_PLAYLIST)
           {
           pFuncBlock->dwCallback  = (((LPMCI_GENERIC_PARMS)ulParam2)->dwCallback);

           // store an extra copy so that play list cue points can be remembered

           pFuncBlock->pInstance->dwOpenCallBack  = (((LPMCI_GENERIC_PARMS)ulParam2)->dwCallback);
           }

       /***************************************
       * Save User parameter
       ****************************************/
       if (pFuncBlock->ulNotify == TRUE) {
           pFuncBlock->usUserParm = usUserParm;
       }

       /*********************************************
       * Create The Event Semaphore for Streaming
       *********************************************/
       ulrc = DosCreateEventSem ((ULONG)NULL,
                                (PHEV)&(pFuncBlock->pInstance->hEventSem),
                                DC_SEM_SHARED,
                                FALSE);

       if (ulrc) {

           ReleaseInstanceMemory (pFuncBlock);
           return (ulrc);
       }
       /*********************************************
       * Create The Event Semaphore  for Thread Cntrl
       *********************************************/
       ulrc = DosCreateEventSem ((ULONG)NULL,
                                (PHEV)&(pFuncBlock->pInstance->hThreadSem),
                                DC_SEM_SHARED,
                                FALSE);
       if (ulrc) {

           ReleaseInstanceMemory (pFuncBlock);
           return (ulrc);
       }

       ulrc = DosCreateMutexSem ((ULONG)NULL,
                        (PHEV) &(pFuncBlock->pInstance->hmtxDataAccess),
                                DC_SEM_SHARED,
                                (ULONG)FALSE);
       if (ulrc) {

           ReleaseInstanceMemory (pFuncBlock);
           return (ulrc);      /* Failed Sem Create */
       }
       /***********************************
       * Open The Semaphore
       **********************************/
       ulrc = DosOpenMutexSem ((ULONG)NULL,
                  (PHMTX)&(pFuncBlock->pInstance->hmtxDataAccess));

       if (ulrc)
           {
           ReleaseInstanceMemory (pFuncBlock);
           return (ulrc);        /* Failed Sem Open */
           }

       ulrc = DosCreateMutexSem ( NULL,
                                  &(pFuncBlock->pInstance->hmtxNotifyAccess),
                                  DC_SEM_SHARED,
                                  FALSE);
       if (ulrc)
          {

          ReleaseInstanceMemory (pFuncBlock);
          return (ulrc);      /* Failed Sem Create */
          }

       /***********************************
       * Open Semaphore which controls access
       * to the status of notifications
       **********************************/
       ulrc = DosOpenMutexSem ( NULL, &pFuncBlock->pInstance->hmtxNotifyAccess);

       if (ulrc)
          {
          ReleaseInstanceMemory (pFuncBlock);
          return (ulrc);        /* Failed Sem Open */
          }

       ulrc = MCIOpen ( pFuncBlock);

       if (ulrc) {

           /****************************************
            *  Free Up all the Resources if Open Fails
            * Close the AudioDD (if initt)ed
            *****************************************/

            DosCloseEventSem (pFuncBlock->pInstance->hEventSem);
            DosCloseEventSem (pFuncBlock->pInstance->hThreadSem);
            DosCloseMutexSem (pFuncBlock->pInstance->hmtxDataAccess);
            ReleaseInstanceMemory (pFuncBlock);

       } /* Error On Open */


       }
      break;

  case MCI_GETDEVCAPS:
       ulrc = MCICaps (pFuncBlock, MCI_DEVTYPE_WAVEFORM_AUDIO);
      break;

  case MCI_SET:
       /********************************/
       // Check for valid Mem
       /********************************/
        if (ulrc = CheckMem ((PVOID)ulParam2,
                               sizeof (DWORD),
                               PAG_READ))
          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return MCIERR_MISSING_PARAMETER;
          }

        ulrc = MCISet (pFuncBlock);
       break;

  case MCI_STATUS:
       /********************************/
          // Check for valid Mem
       /********************************/
       if (ulrc = CheckMem ((PVOID)ulParam2,
                            sizeof (DWORD),
                            PAG_READ|PAG_WRITE))
          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return MCIERR_MISSING_PARAMETER;
          }


       ulrc = MCIStat (pFuncBlock);
      break;

  case MCI_CUE:
       ulrc = MCICue (pFuncBlock);
      break;

  case MCI_SEEK:
       ulrc = MCISeek(pFuncBlock);
      break;

  case MCI_PLAY:
       {
       ulrc = ReportMMEErrors (MCI_PLAY, pFuncBlock);
       if (ulrc)
          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return (ulrc);
          }

       if (ulParam1 & MCI_NOTIFY) {
           if (pFuncBlock->pInstance->usNotifyPending == FALSE)
               pFuncBlock->pInstance->usUserParm = usUserParm;
       }

       ulrc = MCIPlay (pFuncBlock);
       }
      break;

  case MCI_RECORD:
       {
       ulrc = ReportMMEErrors (MCI_RECORD, pFuncBlock);
       if (ulrc)
          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return (ulrc);
          }
       /****************************************************/
        // OverWrite Instance UserParm if no pending notifies
       /*****************************************************/
       if (ulParam1 & MCI_NOTIFY) {
           if (pFuncBlock->pInstance->usNotifyPending == FALSE)
               pFuncBlock->pInstance->usUserParm = usUserParm;
       }
       ulrc = MCIRecd (pFuncBlock);
       }
      break;

  case MCI_SET_CUEPOINT:
       ulrc = MCIScpt (pFuncBlock);
      break;

  case MCI_INFO:
       /********************************/
       // Check for valid Mem
       /********************************/
       if (ulrc = CheckMem ((PVOID)ulParam2,
                            sizeof (DWORD),
                            PAG_READ|PAG_WRITE))

          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return MCIERR_MISSING_PARAMETER;
          }

       ulrc = MCIInfo (pFuncBlock);
      break;

  case MCI_PAUSE:
       ulrc = MCIPaus (pFuncBlock);
      break;

  case MCI_CLOSE:
       ulrc = MCIClos (pFuncBlock);
      break;

  case MCI_CONNECTOR:
       ulrc = MCICnct (pFuncBlock);
      break;


  case MCI_STOP:
       ulrc = MCIStop (pFuncBlock);
      break;

  case MCIDRV_SAVE:
       ulrc = MCISave (pFuncBlock);
      break;

  case MCIDRV_RESTORE:
       ulrc = MCIRest (pFuncBlock);
      break;

  case MCI_RESUME:
       ulrc = MCIResm (pFuncBlock);
      break;

  case MCI_SAVE:
       ulrc = MCISaveFile( pFuncBlock );
      break;

  case MCI_LOAD:
       /*******************************
       * Check for valid Mem
       ********************************/
       if (ulrc = CheckMem ((PVOID)ulParam2,
                            sizeof (DWORD),
                            PAG_READ))

          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return MCIERR_MISSING_PARAMETER;
          }
       ulrc = MCILoad (pFuncBlock);
      break;

  case MCI_MASTERAUDIO:
       ulrc = MCIERR_SUCCESS;
      break;

  case MCI_SET_POSITION_ADVISE:
       /*****************************
       * Check for valid Mem
       *******************************/
       if (ulrc = CheckMem ((PVOID)ulParam2,
                            sizeof (DWORD),
                            PAG_READ))

          {
          MCD_ExitCrit ((INSTANCE *) ulpInstance);
          CleanUp ((PVOID)pFuncBlock);
          return MCIERR_MISSING_PARAMETER;
          }
       ulrc = MCIStpa (pFuncBlock);
      break;

  case MCI_SET_SYNC_OFFSET:
       ulrc = MCISync (pFuncBlock);
      break;

  case MCIDRV_SYNC:
       ((PMCIDRV_SYNC_PARMS)(pFuncBlock->ulParam2))->hStream =
          pFuncBlock->pInstance->StreamInfo.hStream;

      break;

  default:
          return  MCIERR_UNRECOGNIZED_COMMAND;
      break;


  }   /* Switch */


  /***********************************************************************
  * Post The message if notify for synchronous messages only. Exclusive
  * messages are Open, Close, Play and Record. An Open is not complete
  * until it is restored. So a Restore Postes the Notification for an
  * Open Command.
  ***********************************************************************/

  if (pFuncBlock->ulNotify == TRUE) {

      if (usMessage != MCI_PLAY   && usMessage != MCI_CLOSE &&
          usMessage != MCI_RECORD && usMessage != MCI_SAVE &&
          usMessage != MCIDRV_RESTORE )
          {

          if (usMessage == MCI_OPEN )
             {

             if (LOBYTE(ulrc) == MCIERR_SUCCESS)
                {
                  PostMDMMessage (MCI_NOTIFY_SUCCESSFUL,
                                  MCI_OPEN, pFuncBlock);
                }

             pFuncBlock->ulNotify = FALSE;
             }
           else
             {
              if (LOBYTE(ulrc) == MCIERR_SUCCESS)

                  PostMDMMessage (MCI_NOTIFY_SUCCESSFUL,
                                  usMessage, pFuncBlock);
             }  /* Not a Restore */
          }   /* Exclusive Messages */
  }  /* Notify is On */

  /*************************
  * Free Thread Block
  **************************/
  if (usMessage != MCI_PLAY && usMessage != MCI_RECORD && usMessage != MCI_SAVE )

      ulErrorCode = CleanUp ((PVOID)pFuncBlock);

  /*********************************
  * Release all Blocked Threads
  *********************************/
  if (usMessage != MCI_OPEN )
   MCD_ExitCrit ((INSTANCE *) ulpInstance);


  return (ULONG)(ulrc);    /* Return to MDM */

} /* mciDriverEntry */




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: MCIOPEN.C
*
* DESCRIPTIVE NAME: Open Waveform Player.
*
* FUNCTION:
*
* NOTES: This function uses SSM  SPIs to  make use of streaming services
*        transparent to the layer above. On an MCI_OPEN a stream is
*        created. The default source handler is the fileSystem handler
*        the default target handler is the waveform handler. The stream
*        data object is associated,(SPIFILE_OBJ) event detection is
*        enabled. This routine also gets a device handle from the specific
*        DLL for later use (STATUS).
*
*        The DLL Name in MMDRV_OPEN_PARMS specifies the DLL to call on an
*        OPEN. The DLL is assumed to be AUDIOIF.DLL at this stage.
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
  WORD                 wConnLength;              // Amp Mixer Connections
  ULONG                ulrc;                     // Propogated Error Code
  INSTANCE             *ulpInstance;             // Full Blown Instance
  ULONG                ulParam1;                 // Incoming Flags From MCI
  char                 szLoadError[MAX_ERROR_LENGTH];// DosLoadModule
  DWORD                dwDeviceTypeID;           // Device Type ID For Connections
  DWORD                dwOpenFlags;              // Mask For Incoming Flags
  MMDRV_OPEN_PARMS     *pDrvOpenParams;          // MCI Driver Open Parameter Block
  MCI_AMP_OPEN_PARMS   MCIAmpOpenParms;          // MCI AmpMixer Open Parameters
  DEFAULTCONNECTIONS2  DefCon;                   // MCI Connections Block
  char                  szPDDName[MAX_PDD_NAME];
  CHAR                 TempPath[CCHMAXPATH];     // Place to store temp files

  ulrc =      MCIERR_SUCCESS;
  ulParam1 =  pFuncBlock->ulParam1;
  ulpInstance = pFuncBlock->pInstance;
  pDrvOpenParams = (MMDRV_OPEN_PARMS *) pFuncBlock->ulParam2;

  /********************************************
  * !!!! IMPORTANT
  * Make Sure strcpy, and strncpy are
  * using the right sizes.
  * redifine structures. to match the sizes
  * defined in public header files
  **********************************************/

  dwOpenFlags = ulParam1;
  /**********************************/
  // Mask Unwanted Bits
  /**********************************/
  dwOpenFlags &= ~MCI_WAIT;
  dwOpenFlags &= ~MCI_NOTIFY;
  /**********************************/
  // Check For Validity of Flags
  /**********************************/
  dwOpenFlags &= ~(MCI_OPEN_SHAREABLE+MCI_OPEN_TYPE_ID + MCI_OPEN_ELEMENT
                   + MCI_OPEN_PLAYLIST +MCI_OPEN_ALIAS + MCI_OPEN_MMIO +
                   MCI_READONLY );

  if (dwOpenFlags >  0)
      return MCIERR_INVALID_FLAG;

  ulpInstance->usMediaPresent = MCI_FALSE;

  /*******************************************
  * Set Default Stream Handler Names
  * This HardWires The Handlers being Used
  * as FSSH, and ADSH. This stuff needs to
  * Change if New Handlers need to be used
  * with Audio MCD.
  /*******************************************/
  /********************************************
  * Amp/Mixer Intialization Constants
  *********************************************/
  AMPMIX.lLeftVolume =  NOT_INTIALIZED;
  AMPMIX.lRightVolume = NOT_INTIALIZED;
  AMPMIX.lBass =        NOT_INTIALIZED;
  AMPMIX.lTreble     =  NOT_INTIALIZED;
  AMPMIX.lBalance    =  NOT_INTIALIZED;

  /*****************************
  * Wave Initlztn Constants
  *******************************/
  AMPMIX.sChannels =    NOT_INTIALIZED;
  AMPMIX.sMode =        NOT_INTIALIZED;
  AMPMIX.lSRate =       NOT_INTIALIZED;
  ulpInstance->ulAverageBytesPerSec = NOT_INTIALIZED;

  ulpInstance->usFileExists = UNUSED;
  ulpInstance->ulSyncOffset = 0;
  AMPMIX.lBitsPerSRate = 0;
  ulpInstance->mmioHndlPrvd = FALSE;
  ulpInstance->usNotifyPending = FALSE;
  ulpInstance->ulOldStreamPos  = 0;


  /*********************************
  * Set Cue Point Index = 0
  *********************************/
  ulpInstance->StreamInfo.usCuePtIndex = 0;

  /*******************************
  * Load the Devspcfc DLL
  ********************************/
  if ((ulrc = DosLoadModule (szLoadError,sizeof(szLoadError),
                             pDrvOpenParams->szDevDLLName,
                             &(ulpInstance->hModHandle))))
     {
          return ulrc;
     }

  strcpy (ulpInstance->szDevDLL, pDrvOpenParams->szDevDLLName);

  /*****************************************
  * Get the AudioIF Driver Entry point
  ******************************************/
  if (!ulrc) {
      if ((ulrc = DosQueryProcAddr (ulpInstance->hModHandle,
                                    0L,VSDI,
                                    (PFN *) &(ulpInstance->pfnVSD))))
           return ulrc;

  }  /* No RC */

  /*************************************
  * Check Incoming MCI Flags
  ************************************/
  if (ulParam1 & MCI_OPEN_PLAYLIST && ulParam1 & MCI_OPEN_ELEMENT)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  if (ulParam1 & MCI_OPEN_PLAYLIST && ulParam1 & MCI_OPEN_MMIO)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  if (ulParam1 & MCI_OPEN_ELEMENT && ulParam1 & MCI_OPEN_MMIO)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  /***************************************
  * Ensure that if read only flag is passed
  * in that open element is too
  ****************************************/
  if ( ( ulParam1 & MCI_READONLY ) && !( ulParam1 & MCI_OPEN_ELEMENT ) )
     {
     return MCIERR_MISSING_FLAG;
     }

  /*********************************
  * Amp/Mixer Open Flags
  **********************************/
  dwOpenFlags = 0;
  if (ulParam1 & MCI_OPEN_SHAREABLE)
      dwOpenFlags = MCI_OPEN_SHAREABLE;

  ulpInstance->ulCanSave   = TRUE;
  ulpInstance->ulCanInsert = TRUE;
  ulpInstance->ulCanRecord = TRUE;
  ulpInstance->ulUsingTemp = FALSE;

  if (ulParam1 & MCI_OPEN_PLAYLIST) {
      ulpInstance->pPlayList = (PVOID)pDrvOpenParams->lpstrElementName;
      ulpInstance->usPlayLstStrm = TRUE;
      ulpInstance->usFileExists = TRUE;

      /*****************************
      * Set Amp Mixer Default Values
      *******************************/

      SetAmpDefaults (ulpInstance);

      /*****************************
      * Init The Audio Device.
      *****************************/
      strcpy (AMPMIX.szDeviceName,
              ulpInstance->szAudioDevName);

      strcpy (ulpInstance->StreamInfo.AudioDCB.szDevName,
              ulpInstance->szAudioDevName);


      strcpy (AMPMIX.szDriverName,
              ulpInstance->szDevDLL);

      /*****************************
      * Sampling Information
      ******************************/
      ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY);
            if (ulrc)
             return ulrc;
  }
  /*********************************************************
  * Get 'A' stream Handler Handles for Source & target opr.
  **********************************************************/
  if (!(ulParam1 & MCI_OPEN_PLAYLIST)) {


      if (ulrc = SpiGetHandler((PSZ)DEFAULT_SOURCE_HANDLER_NAME,
                               &(ulpInstance->StreamInfo.hidASource),
                               &(ulpInstance->StreamInfo.hidATarget)))
          {
          return ulrc;
          }
  }
  else
  {
      if (ulrc = SpiGetHandler((PSZ)MEM_PLAYLIST_SH,
                               &(ulpInstance->StreamInfo.hidASource),
                               &(ulpInstance->StreamInfo.hidATarget)))
          {
          return ulrc;
          }
  }
  /***********************************************************
  * Get 'B' stream Handler Handles for Source & target opr.
  *************************************************************/

  if (ulrc = SpiGetHandler((PSZ)DEFAULT_TARGET_HANDLER_NAME,
                            &(ulpInstance->StreamInfo.hidBSource),
                            &(ulpInstance->StreamInfo.hidBTarget)))
          {
          return ulrc;
          }


  /***********************************
  * Temporary File creation Flags
  ************************************/

  if ( (ulParam1 & MCI_READONLY ) ||
       !ulpInstance->ulCanSave    ||
       ulParam1 & MCI_OPEN_PLAYLIST )

    {
    ulpInstance->ulOpenTemp = MCI_FALSE;
    }
  else
    {
    ulpInstance->ulOpenTemp = MCI_TRUE;
    } /* else read only flag was NOT specified */


  /*********************************************/
  // Copy Audio Device Name
  /*********************************************/
  if (ulParam1 & MCI_OPEN_ELEMENT) {
      ulpInstance->usMediaPresent = MCI_TRUE;
      if ((PVOID)pDrvOpenParams->lpstrElementName != (ULONG)NULL) {

          strcpy (ulpInstance->lpstrAudioFile,
                  (PSZ)pDrvOpenParams->lpstrElementName);

          /*****************************************
          * Find out if the File Exists.
          ******************************************/
          ulpInstance->dwmmioOpenFlag = MMIO_READ | MMIO_DENYNONE;

          /******************************************************
          * If the user wants to open temp, then modify the flags
          ******************************************************/
          if ( ulpInstance->ulOpenTemp )
             {
             ulpInstance->dwmmioOpenFlag |= MMIO_READWRITE |
                                            MMIO_EXCLUSIVE;
             ulpInstance->dwmmioOpenFlag &= ~MMIO_DENYNONE;
             ulpInstance->dwmmioOpenFlag &= ~MMIO_READ;
             }

          ulrc = OpenFile (ulpInstance, ulpInstance->dwmmioOpenFlag);
          if (!ulrc )
              {
               /****************************
               * Set Amp Defaults
               *****************************/

               ulrc = SetAmpDefaults(ulpInstance);


               strcpy ((PSZ)AMPMIX.szDriverName,
                        ulpInstance->szDevDLL);

               /************************************
               * Init Audio Device
               *************************************/

               if ( ulpInstance->ulCreatedName )
                 {
                 ulrc = InitAudioDevice (ulpInstance, OPERATION_RECORD );
                 }
               else
                 {
                 ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY );
                 }
               if (ulrc)
                   return ulrc;
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

                   ulpInstance->ulOperation = OPERATION_RECORD;
                   ulpInstance->ulCreateFlag = CREATE_STATE;
                   ulrc = MCIERR_SUCCESS;

                   ulpInstance->dwmmioOpenFlag = MMIO_CREATE | MMIO_READWRITE | MMIO_EXCLUSIVE;


                   /********************************/
                   // Open The Element
                   /*******************************/

                   ulrc = OpenFile (ulpInstance,
                                    ulpInstance->dwmmioOpenFlag);

                   if ( ulrc )
                      {
                      if ( ulrc == ERROR_FILE_NOT_FOUND )
                         {
                         return MCIERR_FILE_NOT_FOUND;
                         }
                       else
                         {
                         return (ulrc);
                         }
                      }
                   ulpInstance->usFileExists = TRUE;

                   } /* File Not Found */

               if (ulrc)
                 {
                 // if a person requests that we open the file temp and we
                 // do not have write permission, return an error


                 return (ulrc);
                 }
               } /* No RC */
      } /* Element Name not NULL */
  } /* Element Flag Specified */

  /***********************************************************
  * perform this after the open because open file will modify
  * the state of the following flags
  ***********************************************************/

  if ( ulParam1 & MCI_READONLY )
     {
     ulpInstance->ulCanSave = MCI_FALSE;
     ulpInstance->ulCanRecord = MCI_FALSE;
     }


  /*******************************************
  * Element Handle Provided (DCR XX)
  *******************************************/
  if (ulParam1 & MCI_OPEN_MMIO) {

      ulpInstance->hmmio = (HMMIO)pDrvOpenParams->lpstrElementName;
      ulpInstance->mmioHndlPrvd = TRUE;
      ulpInstance->dwmmioOpenFlag = MMIO_READWRITE | MMIO_EXCLUSIVE;

      ulrc = OpenFile (ulpInstance, ulpInstance->dwmmioOpenFlag);
      if (ulrc)
            return (ulrc);

      ulpInstance->usFileExists = TRUE;
      /***********************************/
          // Amp/Mixer Defaults.
      /**********************************/
      ulrc = SetAmpDefaults (ulpInstance);

      /************************************/
          // Init Audio Device
      /***********************************/

      ulrc = InitAudioDevice (ulpInstance, OPERATION_PLAY);

      if (ulrc)
             return ulrc;
      ulpInstance->usVSDInit = TRUE;
      /**********************************/
      // Set FileName in Instance to Null
      /**********************************/

  }    /* Open mmio Flag */


  /**********************************************/
  // If the temp flag has sent and the io proc
  // has the ability to record with temp files
  /**********************************************/

  if ( ulpInstance->ulOpenTemp &&
       ( ( ulParam1 & MCI_OPEN_ELEMENT ) ||
         ( ulParam1 & MCI_OPEN_MMIO ) ) )
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
    if (ulrc)
       {
       ulrc = mmioGetLastError( ulpInstance->hmmio );

       if (ulrc == MMIOERR_CANNOTWRITE )
          {
          return MCIERR_TARGET_DEVICE_FULL;
          }
       else
          {
          return ulrc;
          }
       }

    ulpInstance->ulUsingTemp = MCI_TRUE;

    } /* if ulOpenTemp */



  if (ulpInstance->usVSDInit != TRUE) {

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
         ulpInstance->ulAverageBytesPerSec = NOT_INTIALIZED;
         }

      /********************************
      * Amp/Mixer Defaults.
      /**********************************/
      ulrc = SetAmpDefaults (ulpInstance);
      /************************************
      * Wave Record Defaults.
      ***********************************/
      SetWaveDeviceDefaults (ulpInstance, OPERATION_RECORD);

      ulrc = InitAudioDevice (ulpInstance, OPERATION_RECORD);

      if (ulrc)
          return ulrc;
  }  /* VSDInit = FALSE */

  /*************************************
  * Since we did not create the file
  * the open file routines are not smart
  * enough to figure out how to set the
  * defaults
  *************************************/


  /*****************************
  * Obtain WaveAudio Device ID
  *****************************/
  ulpInstance->wWaveDeviceID = pDrvOpenParams->wDeviceID;
  dwDeviceTypeID = (DWORD)(MAKEULONG (MCI_DEVTYPE_WAVEFORM_AUDIO,
                                      pDrvOpenParams->usDeviceOrd));

  /******************************************************
  * Ensure that the INI file contains the right device id
  ******************************************************/

  if ( pDrvOpenParams->usDeviceType != MCI_DEVTYPE_WAVEFORM_AUDIO )
     {
     return MCIERR_INI_FILE;
     }

  wConnLength = sizeof(DEFAULTCONNECTIONS2);

  ulrc =  mciQueryDefaultConnections (dwDeviceTypeID,&DefCon,
                                      &wConnLength);

  /******************************************************
  * Ensure that the INI file says that we are connected
  * to an amp mixer
  ******************************************************/

  if ( DWORD_LOWD( DefCon.dwDeviceTypeID2 ) == MCI_DEVTYPE_WAVEFORM_AUDIO )
     {
     return MCIERR_INI_FILE;
     }

  /******************************
  * Open an AMP/MIXER Instance
  ******************************/
  MCIAmpOpenParms.lpstrDeviceType = (LPSTR)DefCon.dwDeviceTypeID2;

  MCIAmpOpenParms.dwCallback = (ULONG)NULL;

  GetPDDName (DefCon.dwDeviceTypeID2, szPDDName);

  strcpy (ulpInstance->szAudioDevName,
          szPDDName);
  /*****************************
  * Open The Audio Device.
  *****************************/
  strcpy ((PSZ)AMPMIX.szDeviceName,
          ulpInstance->szAudioDevName);

  strcpy ((PSZ)STREAM.AudioDCB.szDevName,
          ulpInstance->szAudioDevName);

  /*******************************/
  // Stick in AmpMixer Instance Ptr
  /******************************/
  MCIAmpOpenParms.ulDevDataPtr = (ULONG)&AMPMIX;
  ulrc = mciSendCommand (0,
                         MCI_OPEN,
                         MCI_OPEN_TYPE_ID| MCI_WAIT|dwOpenFlags ,
                         (DWORD) &MCIAmpOpenParms,
                         0);

  if (ulrc)
      return ulrc;
  /***********************
  * Copy SysFileNum
  ***********************/
  ulpInstance->wAmpDeviceID = MCIAmpOpenParms.wDeviceID;
  ulpInstance->usVSDInit = TRUE;
  STREAM.AudioDCB.ulSysFileNum = AMPMIX.ulGlobalFile;
  STREAM.AudioDCB.ulDCBLen = sizeof (DCB_AUDIOSH);

  /***********************
  * Make a connection
  **********************/
  ulrc = mciConnection ( ulpInstance->wWaveDeviceID,
                         1,
                         ulpInstance->wAmpDeviceID,
                         1,
                         MCI_MAKECONNECTION);
  if (ulrc)
      return ulrc;

  /***************************
  * Flags & Other Stuff
  ***************************/
  ulpInstance->ulTimeUnits = lMMTIME;
  ulpInstance->StreamInfo.ulState = CREATE_STATE;
  ulpInstance->ulCreateFlag = CREATE_STATE;
  ulpInstance->ulInstanceSignature = ACTIVE;
  ulpInstance->PlayThreadID = NOT_INTIALIZED;
  ulpInstance->RecdThreadID = NOT_INTIALIZED;
  ulpInstance->StreamInfo.Evcb.evcb.ulSubType = EVENT_EOS|EVENT_ERROR;
  /********************************************************
  * Stick in Instance Ptr in Modified EVCB
  ********************************************************/
  ulpInstance->StreamInfo.Evcb.ulpInstance = (ULONG)ulpInstance;
  strcpy (ulpInstance->szDevDLL, pDrvOpenParams->szDevDLLName);

  return (ULONG)(MCIERR_SUCCESS);

}      /* end of Open */



/********************* START OF SPECIFICATIONS *********************
*
* SUBROU1TINE NAME:  MCICLOS.C
*
* DESCRIPTIVE NAME: Close Waveform Device.
*
* FUNCTION:  Terminate Current activities and close the device.
*
* NOTES: ExitList: When The Process is killed all threads Die.
*                  This has a special bearing to MCI_RECORD
*                  because The Record thread cannot send messages
*                  to the Wave IO Proc to correct header and data
*                  chunk sizes. Even  a event thread based design
*                  would fail because even the event thread is killed.
*                  The IO Procs do implement an AUTO Save Feature.
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_CLOSE message.
*
* EXIT-NORMAL: Instance deleted, Device released.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS: None.
*
* INTERNAL REFERENCES: PostMDMMessage(), DestroyStream().
*
* EXTERNAL REFERENCES: SpiDestroyStream ()    - SSM SPI.
*                      mdmDriverNotify  ()    - MDM API.
*                      mciSendCommand   ()    - MDM API.
*                      mciConnection    ()    - MDM API.
*                      mmioClose        ()    - MMIO API.
*                      HhpFreeMem       ()    - Heap Manager.
*                      DosCloseEventSem ()    - OS/2 API.
*                      DosCloseMutexSem ()    - OS/2 API.
*                      DosFreeModule    ()    - OS/2 API.
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIClos (FUNCTION_PARM_BLOCK *pFuncBlock)
{
  ULONG               ulrc;           // RC
  ULONG               ulParam1;       // Incoming MCI Flags
  ULONG               ulParam2;       // Incoming MCI Data
  ULONG               ulAbortNotify = FALSE; // whether to abort play/record operation
  INSTANCE            *ulpInstance;   // Active Instance
  DWORD               dwCloseFlags;   // Mask for MCI Flags

  /*******************************************
  * Dererence Pointers From Thread Block
  ********************************************/
  ulParam1 =   pFuncBlock->ulParam1;
  ulParam2 =   pFuncBlock->ulParam2;
  ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;

  /***************************
  * Intialize Variables
  ***************************/
  ulrc = MCIERR_SUCCESS;
  dwCloseFlags = ulParam1;

  /************************************
  * Check for Illegal Flags
  *************************************/
  dwCloseFlags &= ~(MCI_WAIT + MCI_NOTIFY + MCI_CLOSE_EXIT) ;

  if (dwCloseFlags > 0)
      return MCIERR_INVALID_FLAG;

  /********************************************
  * Check For Validity of Instance PTR Passed
  ********************************************/
  if (ulpInstance == (ULONG)NULL )
          return MCIERR_INSTANCE_INACTIVE;

  /***************************************
  * Check Instance Signature
  ***************************************/
  if (ulpInstance->ulInstanceSignature != ACTIVE)
      return MCIERR_INSTANCE_INACTIVE;

  /****************************************
  * If There are any Pending Notifies
  * Post Abort Message for Those Operations
  /*****************************************/

   DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );
   if (ulpInstance->usNotifyPending == TRUE)
      {
      ulpInstance->ulNotifyAborted = TRUE;
      ulpInstance->usNotifyPending = FALSE;
      ulAbortNotify = TRUE;
      }
   DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess );


  if ( ulAbortNotify == TRUE) {

     if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
        {
        // Save is a non-interruptible operation
        // wait for completion

        DosWaitEventSem( ulpInstance->hThreadSem, (ULONG ) -1 );

        }
      else
        {
        PostMDMMessage (MCI_NOTIFY_ABORTED,
                        ulpInstance->usNotPendingMsg,
                        pFuncBlock);

        ulpInstance->usNotifyPending = FALSE;
        // note, record will be cleaned up by the io proc

        }
  } /* Pending Notifies */

  /***************************************
  *   Destroy the Stream
  ***************************************/
  if (ulpInstance->StreamInfo.hStream != (ULONG)NULL) {
       ulrc = DestroyStream (ulpInstance->StreamInfo.hStream);

  }

  /*********************************
  * Close The Media Element
  **********************************/
  if (ulpInstance->hmmio != (ULONG)NULL)
     {
     /*********************************
     * Only close the file if no handle
     * was passed to us
     **********************************/
     if (ulpInstance->mmioHndlPrvd != TRUE)
        {

        ulrc = mmioClose (ulpInstance->hmmio, 0);

        /*********************************
        * If we created this file and the
        * user did not save it, then
        * delete it
        **********************************/
        if ( ulpInstance->ulCreatedName )
           {

           DosDelete( (PSZ ) ulpInstance->lpstrAudioFile );
           }
        }
     }
  /*******************************************
  * Break The Default Amp/Mixer Connection
  ********************************************/
  ulrc = mciConnection (ulpInstance->wWaveDeviceID,
                        1,
                        ulpInstance->wAmpDeviceID,
                        1,
                        MCI_BREAKCONNECTION);

  /****************************************
  * Close The Amp/Mixer.
  *****************************************/
  ulrc = mciSendCommand ((WORD)ulpInstance->wAmpDeviceID,
                         MCI_CLOSE,
                         MCI_WAIT,
                         (DWORD)NULL,
                         0);


  /********************************************
  * Free all the Explicit Dynamic Links
  * Device Specific DLL ---> AUDIOIF
  * Custom IO Proc  ----> WAVEIOPROC
  * Custom IO Proc  ----> AVCAPROC
  *******************************************/

  //DosFreeModule (ulpInstance->hModHandle);
  //DosFreeModule (ulpInstance->hModIOProc);

  /*******************************************
  * Close Internal Semaphores
  *******************************************/
  DosCloseEventSem (ulpInstance->hEventSem);
  DosCloseEventSem (ulpInstance->hThreadSem);
  DosCloseMutexSem (ulpInstance->hmtxDataAccess);
  DosCloseMutexSem (ulpInstance->hmtxNotifyAccess);

  /*****************************************
  * Post Close Complete message If needed
  *****************************************/
  if (ulParam1 & MCI_NOTIFY)
      PostMDMMessage (MCI_NOTIFY_SUCCESSFUL, MCI_CLOSE,
                      pFuncBlock);

  /*******************************************
  * Free Thread Parm Block & Assoc Pointers
  *******************************************/
  CleanUp ((PVOID) pFuncBlock->pInstance);

  return (ULONG)(MCIERR_SUCCESS);     // Return Success
}





/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:  MCISET.C
*
* DESCRIPTIVE NAME: Set Waveform Device Parameters.
*
* FUNCTION:
*
* NOTES:
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
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES:  VSDIDriverInterface()  -  VSD
*
*********************** END OF SPECIFICATIONS **********************/
RC MCISet (FUNCTION_PARM_BLOCK *pFuncBlock)

{
  ULONG                ulrc;              // Error Value
  ULONG                ulParam1;          // Msg Flags
  ULONG                ulParam2;          // Msg Data Ptr
  INSTANCE *           ulpInstance;       // Local Instance
  DWORD                dwAmpFlags;        // Amp/Mixer Flags
  DWORD                dwSetFlags;        // Mask For Incoming MCI Flags
  MCI_AMP_SET_PARMS    AmpSetParms;       // Volume Cmnds
  extern HHUGEHEAP     heap;              // Global Heap
  LPMCI_WAVE_SET_PARMS lpSetParms;        // Ptr to set Parms
  LPMCI_WAVE_SET_PARMS lpPrvtSet;         // Ptr to set Parms



  ulrc = MCIERR_SUCCESS;
  dwSetFlags = ulParam1 = pFuncBlock->ulParam1;

  ulParam2 = pFuncBlock->ulParam2;
  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;

  if (ulpInstance->ulInstanceSignature != ACTIVE)
      return MCIERR_INSTANCE_INACTIVE;

  if (ulParam1 & MCI_SET_ON && ulParam1 & MCI_SET_OFF)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  if (ulParam1 & MCI_SET_ITEM )
     {
     return MCIERR_UNSUPPORTED_FLAG;
     }

  if (ulParam1 & MCI_SET_TIME_FORMAT && ulParam1 & MCI_OVER )
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  ulrc = CheckMem ((PVOID)ulParam2,
                   sizeof (MCI_WAVE_SET_PARMS),
                   PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
      return MCIERR_MISSING_PARAMETER;


  lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;
  ulrc = CheckMem ((PVOID)lpSetParms,
                   sizeof (MCI_WAVE_SET_PARMS),
                   PAG_READ);
  if (ulrc)
      return MCIERR_INVALID_BUFFER;

  if (!(lpPrvtSet = HhpAllocMem (heap, sizeof (MCI_WAVE_SET_PARMS))))
      return MCIERR_OUT_OF_MEMORY;

  /**************************************/
  // Check For Validity of Flags
  /*************************************/
  dwSetFlags &= ~MCI_WAIT;
  dwSetFlags &= ~MCI_NOTIFY;

  if (dwSetFlags == 0)
      return MCIERR_MISSING_PARAMETER;

  if ( ( ulParam1 & MCI_SET_AUDIO ) &&
       (( ulParam1 & MCI_AMP_SET_BALANCE ) ||
        ( ulParam1 & MCI_AMP_SET_TREBLE  ) ||
        ( ulParam1 & MCI_AMP_SET_BASS    ) ||
        ( ulParam1 & MCI_AMP_SET_GAIN    ) ||
        ( ulParam1 & MCI_AMP_SET_PITCH))  )
     {
     return MCIERR_UNSUPPORTED_FLAG;
     }

  /***************************************
  * Mask defining Known MCI Set Flags
  ****************************************/
  dwSetFlags &= ~(MCI_SET_AUDIO + MCI_SET_TIME_FORMAT + MCI_SET_VOLUME +
      MCI_SET_ON + MCI_SET_OFF + MCI_WAVE_SET_CHANNELS + MCI_WAVE_SET_BITSPERSAMPLE +
      MCI_WAVE_SET_SAMPLESPERSEC + MCI_WAVE_SET_FORMATTAG + MCI_WAVE_SET_BLOCKALIGN +
      MCI_WAVE_SET_AVGBYTESPERSEC + MCI_SET_DOOR_OPEN + MCI_SET_DOOR_CLOSED +
      MCI_SET_DOOR_LOCK + MCI_SET_DOOR_UNLOCK + MCI_SET_VIDEO + MCI_OVER );


  /*******************************************
  * Return Invalid If any Other bits are set
  *******************************************/
  if (dwSetFlags > 0)
      return MCIERR_INVALID_FLAG;

  /*********************
  * Audio Flags
  *********************/
  if (ulParam1 & MCI_SET_AUDIO) {
      lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;

      /****************************
      * Copy The Info to Amp Set
      *****************************/
      AmpSetParms.dwLevel = lpSetParms->dwLevel;
      AmpSetParms.dwAudio = lpSetParms->dwAudio;
      AmpSetParms.dwOver = lpSetParms->dwOver;
      if (ulParam1 & MCI_SET_ON)
          dwAmpFlags = MCI_SET_ON;
      else
          dwAmpFlags = MCI_SET_OFF;
      /**************************************
          * Send the Request To the Amp/Mixer
      ***************************************/
      ulrc = mciSendCommand (ulpInstance->wAmpDeviceID,
                             (USHORT)MCI_SET,
                             ulParam1&~(MCI_NOTIFY) |MCI_WAIT,
                             (DWORD)&AmpSetParms,
                             pFuncBlock->usUserParm);
  }      /* of Audio Flag */


  if (ulParam1 & MCI_SET_DOOR_OPEN) {
      ulrc = (ULONG)(MCIERR_UNSUPPORTED_FLAG);
  }

  if (ulParam1 & MCI_SET_DOOR_CLOSED) {
      ulrc = (ULONG)(MCIERR_UNSUPPORTED_FLAG);
  }

  if (ulParam1 & MCI_SET_DOOR_LOCK) {
      ulrc = (ULONG)(MCIERR_UNSUPPORTED_FLAG);
  }

  if (ulParam1 & MCI_SET_DOOR_UNLOCK) {
      ulrc = (ULONG)(MCIERR_UNSUPPORTED_FLAG);
  }
  if (ulParam1 & MCI_SET_VIDEO) {
      ulrc = (ULONG)(MCIERR_UNSUPPORTED_FLAG);
  }

  /***************************
  * Time Formats Supported
  ****************************/
  if (ulParam1 & MCI_SET_TIME_FORMAT) {

      switch (lpSetParms->dwTimeFormat) {
           case MCI_FORMAT_MMTIME:
                ulpInstance->ulTimeUnits = lMMTIME;
                break;

           case MCI_FORMAT_MILLISECONDS:
                ulpInstance->ulTimeUnits = lMILLISECONDS;
                break;

           case MCI_FORMAT_SAMPLES:
                ulpInstance->ulTimeUnits = lSAMPLES;
                break;

           case MCI_FORMAT_BYTES:
                ulpInstance->ulTimeUnits = lBYTES;
                break;

           default:     return MCIERR_INVALID_TIME_FORMAT_FLAG;
      } /* Switch Statement */
  } /* Time Format */

  /************************
  * WaveAudio Extensions
  *************************/

  dwSetFlags = ulParam1 & (  MCI_WAVE_SET_CHANNELS
                           + MCI_WAVE_SET_BITSPERSAMPLE
                           + MCI_WAVE_SET_SAMPLESPERSEC
                           + MCI_WAVE_SET_FORMATTAG
                           + MCI_WAVE_SET_AVGBYTESPERSEC );



  // if any of the wave set flags are greater than 0, then it is possible we may have
  // to stop the stream or

  if ( !ulrc && dwSetFlags > 0 )
    {
    // if the stream has been created, then we must destroy, perform the set
    // and set a flag to indicate that the stream must be created

    if ( ulpInstance->ulCreateFlag == PREROLL_STATE )
       {
       // if we are actually streaming, then we cannot perform the set

       if ( STRMSTATE == MCI_PLAY || STRMSTATE == MCI_RECORD || STRMSTATE == MCI_PAUSE )

          {
          ulrc = MCIERR_INVALID_MODE;
          }
       else
          {

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

          // set the stream into create state and destroy it

          ulpInstance->ulCreateFlag = CREATE_STATE;
          STRMSTATE = STREAM_SET_STATE;
          }
       }
    } /* if no errors have occurred */

  if ( !ulrc )
   {
   if (ulParam1 &  MCI_WAVE_SET_CHANNELS )
      {
      lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;
      memcpy (lpPrvtSet, lpSetParms, sizeof(MCI_WAVE_SET_PARMS));
      ulrc = SetAudioDevice (ulpInstance,
                             lpPrvtSet,
                             MCI_WAVE_SET_CHANNELS );
      }

   if (ulParam1 &  MCI_WAVE_SET_SAMPLESPERSEC)
     {
     lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;
     memcpy (lpPrvtSet, lpSetParms, sizeof(MCI_WAVE_SET_PARMS));
     ulrc = SetAudioDevice (ulpInstance,
                             lpPrvtSet,
                             MCI_WAVE_SET_SAMPLESPERSEC);

     }

   if (ulParam1 &  MCI_WAVE_SET_AVGBYTESPERSEC)
      {
      lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;
      memcpy (lpPrvtSet, lpSetParms, sizeof(MCI_WAVE_SET_PARMS));

      if ( lpSetParms->nAvgBytesPerSec < ( ULONG ) ( AMPMIX.lSRate * ( AMPMIX.lBitsPerSRate / 8 ) * AMPMIX.sChannels ) )
         {
         ulrc = MCIERR_OUTOFRANGE;
         }
      else
         {
         ulpInstance->ulAverageBytesPerSec = lpSetParms->nAvgBytesPerSec;
         }
      }

  if (ulParam1 &  MCI_WAVE_SET_BLOCKALIGN)
     {
     ulrc = MCIERR_UNSUPPORTED_FLAG;

     }

   if (ulParam1 & MCI_WAVE_SET_BITSPERSAMPLE)
      {
      lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;
      memcpy (lpPrvtSet, lpSetParms, sizeof(MCI_WAVE_SET_PARMS));
      ulrc = SetAudioDevice (ulpInstance,
                             lpPrvtSet,
                             MCI_WAVE_SET_BITSPERSAMPLE);
      }

   if (ulParam1 & MCI_WAVE_SET_FORMATTAG)
      {

      lpSetParms = (LPMCI_WAVE_SET_PARMS)ulParam2;
      memcpy (lpPrvtSet, lpSetParms, sizeof(MCI_WAVE_SET_PARMS));
      ulrc = SetAudioDevice (ulpInstance,
                            (LPMCI_WAVE_SET_PARMS)lpPrvtSet,
                            MCI_WAVE_SET_FORMATTAG);

      }
  } /* if no errors have happened */

  dwSetFlags = ulParam1 & (  MCI_WAVE_SET_CHANNELS
                           + MCI_WAVE_SET_BITSPERSAMPLE
                           + MCI_WAVE_SET_SAMPLESPERSEC );

  // if any of the wave set flags are greater than 0, then recalculate avg. bytes per sec
  // if no errors have happened previously

  if ( dwSetFlags  && !ulrc )
    {
    ulpInstance->ulAverageBytesPerSec = AMPMIX.lSRate * ( AMPMIX.lBitsPerSRate / 8 ) * AMPMIX.sChannels;
    }

  /******************************************************************
  * Operation record means SetHeader is due. You should also verify
  * A valid element exists at this stage. (MMIO_CREATE is done).
  *******************************************************************/
  if (!ulrc)
     {
     if ( ulpInstance->mmioHndlPrvd ||
          ( AMPMIX.ulOperation == OPERATION_RECORD && STRMSTATE != MCI_RECORD ) )
          {
          /**************************************
          * A better check is needed.
          ***************************************/
          if (ulpInstance->usFileExists == TRUE)
             {
             ulrc = SetAudioHeader (ulpInstance);
             }
          } /* Not Recording */

     } /* No errors */

  if ((ulParam1 & MCI_SET_VOLUME) && !(ulParam1 & MCI_SET_AUDIO)) {
      return MCIERR_MISSING_FLAG;
  }

  CleanUp ((PVOID)lpPrvtSet);

  return (ulrc);
}



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:  MCISAVE
*
* DESCRIPTIVE NAME: Save Waveform Instance State.
*
* FUNCTION:
*
* NOTES:
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
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES:  VSDIDriverInterface()  -  VSD
*
*********************** END OF SPECIFICATIONS **********************/
RC MCISave (FUNCTION_PARM_BLOCK *pFuncBlock)
{


  INSTANCE     * ulpInstance;
  ULONG        ulParam1;
  ULONG        ulrc;

  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;
  ulParam1 = pFuncBlock->ulParam1;
  ulrc = MCIERR_SUCCESS;

  if (ulpInstance == (ULONG)NULL )
          return MCIERR_INSTANCE_INACTIVE;

  /******************************************
  * Was A Stream was Created Before
  ******************************************/
  if (ulpInstance->ulCreateFlag == PREROLL_STATE) {

      /************************************************
      * Pause The Stream for Context Switching
      **************************************************/
      switch (STRMSTATE)
      {
      case MCI_PLAY: case MCI_RECORD:
              ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_STREAM);
              STRMSTATE = SAVEPAUS_STATE;
              break;

      case CUERECD_STATE:
      case CUEPLAY_STATE:
      case MCI_PAUSE:
      case MCI_STOP:
              break;
      default:
              break;
      }   /* Switch */

      ulpInstance->usSaveFlag = TRUE; // Set The Flag to Saved State

  } /* Stream Created */

  return (MCIERR_SUCCESS); // Dont Return Error Conditions
}



/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:  MCIREST
*
* DESCRIPTIVE NAME: Restore Waveform Instance State.
*
* FUNCTION:
*
* NOTES:
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
* INTERNAL REFERENCES:  MCIERR ().
*
* EXTERNAL REFERENCES:  VSDIDriverInterface()  -  VSD
*
*********************** END OF SPECIFICATIONS **********************/
RC MCIRest (FUNCTION_PARM_BLOCK *pFuncBlock)
{


  INSTANCE        * ulpInstance;
  ULONG           ulParam1;
  ULONG           ulrc;
  DWORD           dwSetAll;
  MCI_WAVE_SET_PARMS lpSetParms;


  ulrc = MCIERR_SUCCESS;
  dwSetAll = MCI_WAVE_SET_BITSPERSAMPLE| MCI_WAVE_SET_FORMATTAG|
             MCI_WAVE_SET_CHANNELS | MCI_WAVE_SET_SAMPLESPERSEC;

  ulpInstance= (INSTANCE *)pFuncBlock->ulpInstance;
  ulParam1 = pFuncBlock->ulParam1;

  if (ulpInstance == (ULONG)NULL )
      return MCIERR_INSTANCE_INACTIVE;

  /**************************************************
  * Was A Stream was Created Before and Was it Saved
  ***************************************************/
  if (ulpInstance->usSaveFlag == TRUE) {
      if (ulpInstance->ulCreateFlag == PREROLL_STATE) {

          /*******************************************
          * If The Stream was in a running state
          * before Context switch start the stream
          * else just set the restored flag to true
          ********************************************/

          if (STRMSTATE == SAVEPAUS_STATE) {
              /******************************
              * Amp Defaults Restored
              ******************************/
              SetAmpDefaults (ulpInstance);

              VSDInstToWaveSetParms (&lpSetParms,ulpInstance);
              /*******************************
              * Reset Device attribs
              *******************************/
              ulrc = SetAudioDevice (ulpInstance,
                                    (LPMCI_WAVE_SET_PARMS)&lpSetParms,
                                    dwSetAll);

              /***********************************************
              * Start the stream from this position
              ************************************************/
              ulrc = SpiStartStream (STREAM.hStream,
                                     SPI_START_STREAM);

              if (ulrc)
                  return (ulrc);

              STRMSTATE = MCI_PLAY;
              ulpInstance->usRestFlag = TRUE; // Set The Flag to Restored State
          } /* Save Paus State */
      } /* PreRoll State */
  } /* True Save Flag */

  return (MCIERR_SUCCESS); // Dont Return Error Conditions
}


/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: MCIMASVOL
*
* DESCRIPTIVE NAME: Waveform Master Volume Routine.
*
* FUNCTION: Set Master Volume within an Waveform Instance.
*
* NOTES: An AMP/MIXER instance is associated with every waveform instance.
*        This routine accepts a MCI_MASTERAUDIO message from MDM and
*        routes it to the AMP/MIXER.
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
* INTERNAL REFERENCES: mciSendCommand().
*
* EXTERNAL REFERENCES: DosQueryProcAddr - OS/2 API.
*
*********************** END OF SPECIFICATIONS *******************************/

RC MCIMsvl (FUNCTION_PARM_BLOCK *pFuncBlock)
{

   ULONG                   ulrc;
   INSTANCE                *ulpInstance;
   LPMCI_MASTERAUDIO_PARMS lpMasterAudioParms;


   ulrc = MCIERR_SUCCESS;
   ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;
   lpMasterAudioParms = (LPMCI_MASTERAUDIO_PARMS)pFuncBlock->ulParam2;

   /***********************************************************/
   // Route the request to the AMP/MIXER Via MDM
   /***********************************************************/

   ulrc = mciSendCommand (ulpInstance->wAmpDeviceID,
                          MCI_MASTERAUDIO,
                          pFuncBlock->ulParam1| MCI_WAIT ,
                          pFuncBlock->ulParam2,
                          pFuncBlock->usUserParm);
   if (ulrc)
           return ulrc;      // To be resolved.

   return ulrc;
}
