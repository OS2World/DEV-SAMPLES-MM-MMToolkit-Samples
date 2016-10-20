/******************
*
* SOURCE FILE NAME:             AUDIOSUB.C
*
* DESCRIPTIVE NAME:     Support functions for waveaudio MCI Driver.
*
*              Copyright (c) IBM Corporation  1991
*                        All Rights Reserved
*
* STATUS: MM Extensions 1.0
*
* NOTES:    THIS MODULE RESIDES AT RING 3
*
*    DEPENDENCIES: NONE
*    RESTRICTIONS: NONE
*
* MODIFICATION HISTORY:
* DATE      DEVELOPER         CHANGE DESCRIPTION
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
#include <mmsystem.h>                   // MM System Include.
#include <mcidrv.h>                     // MCI Driver include.
#include <mmio.h>                       // MMIO Include.
#include <mcd.h>                        // AudioIFDriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions
#include <admcdat.h>                    // Data Format defines
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

RC DestroyStream (HSTREAM hStream)
{

  ULONG ulrc;
  /************************************
  * Call SpiDestroyStream
  *************************************/
  ulrc = SpiDestroyStream (hStream);

  return ulrc;
}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: SetAmpDefaults ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Set AMP Mixer default values for treble, bass and so on.
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
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/

RC  SetAmpDefaults (INSTANCE * ulpInstance)
{

   /*********************************
   * Amp/Mixer Default Audio levels
   **********************************/

   AMPMIX.usMasterVolume = 50;
   AMPMIX.lVolumeDelay = 0L;
   AMPMIX.lBalanceDelay = 0L;
   AMPMIX.lPitch = 50L;

   /************************************************************
   * Check to see if volume, bass, treble, and balance levels
   * have been explicitly set by the user. If they have not
   * been intialized then the default values are set
   **************************************************************/
   if (AMPMIX.lLeftVolume == NOT_INTIALIZED)
       AMPMIX.lLeftVolume = 100L;

   if (AMPMIX.lRightVolume == NOT_INTIALIZED)
       AMPMIX.lRightVolume = 100L;

   if (AMPMIX.lBass == NOT_INTIALIZED)
       AMPMIX.lBass = 50L;

   if (AMPMIX.lBalance == NOT_INTIALIZED)
       AMPMIX.lBalance = 50L;

   if (AMPMIX.lBass == NOT_INTIALIZED)
       AMPMIX.lBass = 50L;

   if (AMPMIX.lTreble == NOT_INTIALIZED)
       AMPMIX.lTreble = 100L;

   return (MCIERR_SUCCESS);
}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: CheckFlags()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Check for illegal combination of flags
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
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/
RC  CheckFlags (ULONG ulParam1)
{
  if ((ulParam1 & MCI_WAIT) && (ulParam1 & MCI_NOTIFY))
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  return (MCIERR_SUCCESS);
}


/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: CheckMem                                                */
/*                                                                          */
/* DESCRIPTIVE NAME: Memory Check                                           */
/*                                                                          */
/* FUNCTION: Tests memory at specified address and length to see if it      */
/*           exists for the application and if it has the right access.     */
/*                                                                          */
/* NOTES:    This routine contains OS/2 system specific functions.          */
/*           DosQueryMem                                                    */
/*                                                                          */
/* INPUT:    pmem      - Address of memory to test                          */
/*           ulLength  - Length of memory to test                           */
/*           ulFlags   - Memory flags where:                                */
/*                          PAG_EXECUTE                                     */
/*                          PAG_READ                                        */
/*                          PAG_WRITE                                       */
/*                                                                          */
/* OUTPUT:   rc = error return code.                                        */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

RC CheckMem ( PVOID pMem,
              ULONG ulLength,
              ULONG ulFlags )
{

  RC rc = NO_ERROR;                       // local return code
  ULONG ulLengthLeft;                     // length left to check
  ULONG ulTotLength = 0L;                 // Total length checked
  ULONG ulRetFlags = (ULONG)NULL;         // Flags returned from Dos call

  /**************************************************************************/
  /*                                                                        */
  /*   Set up to check memory.                                              */
  /*                                                                        */
  /**************************************************************************/

  ulLengthLeft = ulLength;
  while ((!rc) && (ulTotLength < ulLength))
    {                                             // Call OS to check mem
      if (!(rc = DosQueryMem(pMem, &ulLengthLeft, &ulRetFlags)))
        {                                         // We have the flags
          if ((ulRetFlags & PAG_FREE) ||          // if free then error
              !(ulRetFlags & PAG_COMMIT) ||       // if not committed then error
                                                  // if execute only
              ((ulRetFlags & ulFlags) != ulFlags))
            {
              rc = ERROR_INVALID_BLOCK;
            }
          else
            {
              pMem =(PVOID)((ULONG)pMem + ulLengthLeft);
              ulTotLength += ulLengthLeft;
              ulLengthLeft = ulLength - ulTotLength;
            }
        }
    }
  return(rc);
}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME:              PostMDMMessage()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Post The appropriate Notification message using mdmDriverNotify ()
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
* INTERNAL REFERENCES:  Post_MDM_Message()
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC PostMDMMessage (ULONG ulErrCode, USHORT usMessage,
                      FUNCTION_PARM_BLOCK *pFuncBlock)
{

  ULONG   ulrc = MCIERR_SUCCESS;
  USHORT  usNotifyType;
  USHORT  usUserParm;
  HWND    hWnd;



  usNotifyType = MCI_NOTIFY_SUCCESSFUL;        // Intialize as success

  /***************************************************************
  * Determine the MCI Notification Code for this message
  ****************************************************************/

  switch (DWORD_LOWD(ulErrCode))
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
     hWnd = pFuncBlock->dwCallback;
     }
  else
     {
     usUserParm = pFuncBlock->pInstance->usUserParm;
     hWnd = pFuncBlock->pInstance->dwCallback;
     }

  /******************************************************************
  * If Incoming message is a play and it is from an async play then
  * do not call driver notify.
  *******************************************************************/

  ulrc = mdmDriverNotify ((WORD)pFuncBlock->pInstance->wWaveDeviceID,
                          (HWND)hWnd,
                          MM_MCINOTIFY,
                          usUserParm,
                          MAKEULONG (usMessage, usNotifyType));


  return (MCIERR_SUCCESS);
}



/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME:              MCDCAPS
*
* DESCRIPTIVE NAME: Waveform and MIDI Audio Device Capabilities.
*
* FUNCTION: Get Waveform and MIDI Audio Device Static Capabilities.
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_GETDEVCAPS message.
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES: None.
*
*********************** END OF SPECIFICATIONS ********************************/

RC MCICaps( FUNCTION_PARM_BLOCK *pFuncBlock, ULONG ulDeviceType)
{
  ULONG                  ulrc;                // MME Error Value
  ULONG                  ulParam1;            // Msg Flags
  ULONG                  ulParam2;            // Msg Data
  INSTANCE               * ulpInstance;       // Local Instance
  LPMCI_GETDEVCAPS_PARMS pParams;             // Msg Data Ptr
  ULONG                  ulType;              // Msgs supported.
  DWORD                  dwCapsFlags;         // Mask for Incoming MCI Flags

  /**************************************
  * Derefernce Pointers.
  **************************************/
  ulParam1 =   pFuncBlock->ulParam1;
  ulrc = MCIERR_SUCCESS;
  dwCapsFlags = 0;
  ulParam2 =   pFuncBlock->ulParam2;
  ulpInstance= (INSTANCE *)(pFuncBlock->ulpInstance);
  dwCapsFlags = ulParam1;

  /***********************************
  * Check incoming MCI Flags.
  ************************************/
  dwCapsFlags &= ~MCI_WAIT;
  dwCapsFlags &= ~MCI_NOTIFY;

  /*****************************************
  * Check for Invalid Flags
  *****************************************/
  dwCapsFlags &= ~(MCI_GETDEVCAPS_MESSAGE + MCI_GETDEVCAPS_ITEM);
  if (dwCapsFlags > 0 )
      return MCIERR_INVALID_FLAG;

  /********************************************
  * Check for Invalid Combination of flags
  ********************************************/

  if (ulParam1 & MCI_GETDEVCAPS_ITEM && ulParam1 & MCI_GETDEVCAPS_MESSAGE)
      return MCIERR_FLAGS_NOT_COMPATIBLE;

  /*********************************************
  * Check Pointer to Return Information
  **********************************************/
  ulrc = CheckMem ((PVOID)pFuncBlock->ulParam2,
                   sizeof (MCI_GETDEVCAPS_PARMS), PAG_READ | PAG_WRITE );

  if (ulrc != MCIERR_SUCCESS)
      return MCIERR_MISSING_PARAMETER;

  ulType = ulParam1 & (MCI_GETDEVCAPS_MESSAGE | MCI_GETDEVCAPS_ITEM);

  /************************************
  * Check for missing flags
  *************************************/
  if (!ulType)
      return (MCIERR_MISSING_FLAG);

  pParams = (LPMCI_GETDEVCAPS_PARMS )ulParam2;    // Dereference Pointers.

  switch (ulType)
  {

  case MCI_GETDEVCAPS_MESSAGE:
       {

       switch (pParams->wMessage)
       {


       case MCI_RELEASEDEVICE: case MCI_ACQUIREDEVICE:
       case MCI_OPEN:        case MCI_PLAY:       case MCI_PAUSE:
       case MCI_SEEK:        case MCI_RECORD:     case MCI_CLOSE:
       case MCI_INFO:        case MCI_GETDEVCAPS: case MCI_SET:
       case MCI_STATUS:      case MCI_MASTERAUDIO:case MCI_CUE:
       case MCI_STOP:        case MCI_LOAD:       case MCI_RESUME:
       case MCI_SET_POSITION_ADVISE:      case MCI_SET_CUEPOINT:
       case MCI_CONNECTOR: case MCI_SET_SYNC_OFFSET:case MCI_SAVE:

            pParams->dwReturn = MCI_TRUE;

            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
           break;

       /******************************
        * List Unsupported Functions
       ******************************/

      case MCI_DEVICESETTINGS:
      case MCI_STEP:            case MCI_SYSINFO:
      case MCI_UPDATE:          case MCI_GETTOC:
      case MCI_SPIN:            case MCI_ESCAPE:
           pParams->dwReturn = MCI_FALSE;

           ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
          break;

      default: return MCIERR_UNRECOGNIZED_COMMAND;

      } /* Message Switch */
  } /* Item Switch */
  break;                                     // Message case

  case MCI_GETDEVCAPS_ITEM:
  {
       switch (pParams->dwItem)
       {

       case MCI_GETDEVCAPS_DEVICE_TYPE:
            pParams->dwReturn = ulDeviceType;
            ulrc = MAKEULONG (ulrc, MCI_DEVICENAME_RETURN);
           break;

       case MCI_GETDEVCAPS_CAN_RECORD:
            if (ulDeviceType == MCI_DEVTYPE_WAVEFORM_AUDIO)
              {
              if ( ulpInstance->ulCanRecord )
                {
                pParams->dwReturn = MCI_TRUE;
                }
              else
                {
                pParams->dwReturn = MCI_FALSE;
                }
              }
            else
            {
                pParams->dwReturn = MCI_FALSE;
            }
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
           break;


       case MCI_GETDEVCAPS_HAS_AUDIO:
            pParams->dwReturn = MCI_TRUE;
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
           break;

       case MCI_GETDEVCAPS_HAS_VIDEO:
            pParams->dwReturn = MCI_FALSE;
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
           break;

       case MCI_GETDEVCAPS_USES_FILES:
            pParams->dwReturn = MCI_TRUE;
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
           break;

       case MCI_GETDEVCAPS_CAN_PLAY:
            pParams->dwReturn = MCI_TRUE;
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
           break;

       case MCI_GETDEVCAPS_CAN_SAVE:
             if ( ulpInstance->ulCanSave )
               {
               pParams->dwReturn = MCI_TRUE;
               }
             else
               {
               pParams->dwReturn = MCI_FALSE;
               }
             ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
            break;

case MCI_GETDEVCAPS_CAN_RECORD_INSERT:
             if ( ulpInstance->ulCanInsert )
               {
               pParams->dwReturn = MCI_TRUE;
               }
             else
               {
               pParams->dwReturn = MCI_FALSE;
               }

             ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
            break;

       case MCI_GETDEVCAPS_CAN_EJECT:
             pParams->dwReturn = MCI_FALSE;
             ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
            break;

       case MCI_GETDEVCAPS_CAN_STREAM:
             pParams->dwReturn = MCI_TRUE;
             ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
             break;

       case MCI_GETDEVCAPS_CAN_PROCESS_INTERNAL:
             pParams->dwReturn = MCI_FALSE;
             ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
             break;

       case MCI_GETDEVCAPS_CAN_LOCKEJECT:
            pParams->dwReturn = MCI_FALSE;
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
            break;

       case MCI_GETDEVCAPS_CAN_SETVOLUME:
            pParams->dwReturn = MCI_TRUE;
            ulrc = MAKEULONG (ulrc, MCI_TRUE_FALSE_RETURN);
            break;

       case MCI_GETDEVCAPS_PREROLL_TYPE:
            pParams->dwReturn = MCI_PREROLL_NOTIFIED;
             ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);
            break;

       case MCI_GETDEVCAPS_PREROLL_TIME:
            pParams->dwReturn = 0;    // This is to be corrected
            ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);
            break;

       default:  return MCIERR_UNSUPPORTED_FLAG;
              break;


       }       /* items of item */
  }       /* case of item flag */
  break;

  default: return MCIERR_FLAGS_NOT_COMPATIBLE;

  }        /* All Inclusive */

  return (ULONG)(ulrc);

}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:              MCDINFO
*
* DESCRIPTIVE NAME: Information about Waveform Device.
*
* FUNCTION: Obtain Info about a  Waveform Device.
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
* EFFECTS:
*
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS **********************/

RC MCIInfo (FUNCTION_PARM_BLOCK *pFuncBlock)
{

  ULONG            ulrc;               // Propogated MME Error Value
  ULONG            ulParam1;           // Incoming MCI Msg Flags
  ULONG            ulParam2;           // Msg Data
  DWORD            dwInfoFlags;        // Mask for Incoming Flags
  INSTANCE         * ulpInstance;      // Local Instance
  LPMCI_INFO_PARMS lpInfoParms;        // Msg Data Ptr

  ulrc = MCIERR_SUCCESS;
  dwInfoFlags = 0;

  ulParam1 = pFuncBlock->ulParam1 & NOTIFY_MASK;
  ulParam2 = pFuncBlock->ulParam2;

  dwInfoFlags = ulParam1;

  /*******************************************
  * Turn off Expected Flags Bits
  ********************************************/
  dwInfoFlags &= ~( MCI_INFO_FILE + MCI_INFO_PRODUCT);

  /************************************************/
  // Return Error if any bits are still set
  /************************************************/
  if (dwInfoFlags > 0 )
      return MCIERR_INVALID_FLAG;

  /****************************************************
   * Check For Valid Flags but Invalid combination
  *****************************************************/
  if (ulParam1 & MCI_INFO_FILE && ulParam1 & MCI_INFO_PRODUCT)
      return (MCIERR_FLAGS_NOT_COMPATIBLE);

  if (!(ulParam1 & MCI_INFO_FILE || ulParam1 & MCI_INFO_PRODUCT))
      return (MCIERR_MISSING_FLAG);

  /******************************************************
  * Derefernce Instance pointer from the function block
  ******************************************************/
  ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;

  /*************************************************
  * Check For valid MCI Data Struct pointer
  *************************************************/
  if (ulrc = CheckMem ((PVOID)pFuncBlock->ulParam2,
                       sizeof (MCI_INFO_PARMS),
                       PAG_READ | PAG_WRITE) )

      return (MCIERR_MISSING_PARAMETER);

  lpInfoParms = (LPMCI_INFO_PARMS)ulParam2;

  /***************************************
  * Check for valid Instance
  ***************************************/
  if (ulpInstance == (ULONG)NULL )
      return MCIERR_INSTANCE_INACTIVE;

  if (ulpInstance->ulInstanceSignature != ACTIVE)
      return MCIERR_INSTANCE_INACTIVE;


  if (ulParam1 & MCI_INFO_FILE) {
      if (ulpInstance->usFileExists == UNUSED ||
          ulpInstance->usFileExists == FALSE )

          return MCIERR_FILE_NOT_FOUND;

      /************************************
      * Ensure the size of the buffer the
      * user passed is valid
      *************************************/
       if (ulrc = CheckMem ((PVOID)lpInfoParms->lpstrReturn,
                            lpInfoParms->dwRetSize,
                            PAG_READ | PAG_WRITE) )

         {
         return (MCIERR_INVALID_BUFFER);
         }

      if (strlen(ulpInstance->lpstrAudioFile) > lpInfoParms->dwRetSize)
        {
        lpInfoParms->dwRetSize = strlen (ulpInstance->lpstrAudioFile);
        return (MCIERR_INVALID_BUFFER);
        }
      else
        {
        strcpy((PSZ)lpInfoParms->lpstrReturn, ulpInstance->lpstrAudioFile);
        }

  }  /* If File Info was Requested */

  /**********************************
  * Product Information
  ***********************************/
  if (ulParam1 & MCI_INFO_PRODUCT) {

      /*******************************
      * Get Product Information
      * From AudioIF which is the
      * device specific component.
      ********************************/
      ulrc = ulpInstance->pfnVSD (&AMPMIX,
                                  MCI_INFO,
                                  ulParam1,
                                  (ULONG)lpInfoParms,
                                  0L);
  }  /* INFO Product */

  return (ulrc);
 }


/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME:      InitAudioDevice
*
* DESCRIPTIVE NAME:     ""
*
* FUNCTION: Open and Intialize the VSD Device.
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
* INTERNAL REFERENCES: AudioIFDriverEntry().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC  InitAudioDevice (INSTANCE *ulpInstance, ULONG ulOperation)
{

  ULONG   ulrc    = MCIERR_SUCCESS;
  USHORT  usFound = 0;
  USHORT  i;

  /***************************
  * Intialize Defaults
  * if unIntialized
  ***************************/
  if (AMPMIX.sMode == NOT_INTIALIZED)
      AMPMIX.sMode = DEFAULT_MODE;

  if (AMPMIX.sChannels == NOT_INTIALIZED)
      AMPMIX.sChannels = DEFAULT_CHANNELS;

  if (AMPMIX.lSRate == NOT_INTIALIZED)
      AMPMIX.lSRate = DEFAULT_SAMPLERATE;

  if (AMPMIX.lBitsPerSRate == 0)
      AMPMIX.lBitsPerSRate = DEFAULT_BITSPERSAMPLE;

  if (AMPMIX.ulBlockAlignment == NOT_INTIALIZED)
      AMPMIX.ulBlockAlignment = DEFAULT_BLOCK_ALIGN;
  if ( ulpInstance->ulAverageBytesPerSec == NOT_INTIALIZED )

      ulpInstance->ulAverageBytesPerSec = DEFAULT_CHANNELS *
                                          DEFAULT_SAMPLERATE *
                                          ( DEFAULT_BITSPERSAMPLE / 8 );
  /******************************************
  * Do a Table Look up mainly to
  * determine data type and subtype
  * once a match is found, SPCB KEY
  * used for stream creation is set
  * and the Device Dependant flags are
  * built in this routine
  *******************************************/
  if (AMPMIX.sMode != DATATYPE_MIDI)    {
      for (i = 0; i < NUM_DATATYPES; i++) {
           if (DataFormat[i].channels == AMPMIX.sChannels)
               if (DataFormat[i].srate == AMPMIX.lSRate)
                   if (DataFormat[i].ulDataType == (ULONG)AMPMIX.sMode)
                       if (DataFormat[i].bits_per_sample == AMPMIX.lBitsPerSRate){
                           STREAM.SpcbKey.ulDataType = DataFormat[i].ulDataType;
                           STREAM.SpcbKey.ulDataSubType = DataFormat[i].ulDataSubType;
                           AMPMIX.sMode = (SHORT)DataFormat[i].ulDataType;
                           ulpInstance->usModeIndex = i;
                           AMPMIX.ulFlags = FIXED|VOLUME|TREBLE|BASS;
                           usFound = 1;
                           break;

                       }  /* BitsPersample Matches */

      }  /* For Loop */

      if (!usFound)
          return (MCIERR_DEVICE_NOT_READY);
  } /* Data Type != MIDI */

  AMPMIX.ulOperation = ulOperation;

  /*********************************************
  * Update Device and Driver Names
  *********************************************/
  strcpy ((PSZ)AMPMIX.szDriverName,
          ulpInstance->szDevDLL);

  strcpy ((PSZ)AMPMIX.szDeviceName,
          ulpInstance->szAudioDevName);

  strcpy (STREAM.AudioDCB.szDevName,
          ulpInstance->szAudioDevName);

  /*************************
  * Set Init Flag to true
  *************************/
  ulpInstance->usVSDInit = TRUE;

  return (ulrc);

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
                   LPMCI_WAVE_SET_PARMS lpSetParms,
                   ULONG                ulParam1 )

{

  ULONG ulrc = MCIERR_SUCCESS;

  /*********************************
   * Send A Set Across the AudioIF
   * Driver Interface
   ********************************/

   ulrc = ulpInstance->pfnVSD (&AMPMIX,
                               MCI_SET,
                               ulParam1,
                               (LONG)lpSetParms,
                               0L);

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
*     LINKAGE:   CALL FAR
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

  ulrc = MCIERR_SUCCESS;
  lCnt = 0;
  /*******************************************************
  * Check to see if the wait pending flag is set.
  * If it is then release the wait block by
  * a SpiStopStream request. The Stop will
  * generate a EVENT_STREAM_STOPPED event, which will
  * post the event semaphore. The posting of the event
  * semaphore will unblock the wait thread which will
  * return to the application.
  ********************************************************/

  if (ulpInstance->usWaitPending == TRUE) {
      DosResetEventSem (ulpInstance->hEventSem, &lCnt);

      ulrc = SpiStopStream (STREAM.hStream, SPI_STOP_DISCARD);

      /*********************************************
      * Wait for the Stopped event . Notice that
      * more than one thread will be released and
      * free to run as a result of the stop event
      *********************************************/
      if (!ulrc)
          DosWaitEventSem (ulpInstance->hEventSem, -1);
  }
  return MCIERR_SUCCESS;

 }

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
* EFFECTS:
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

  ulrc = MCIERR_SUCCESS;

  if (ulpInstance->hmmio == (ULONG)NULL)
      return MCIERR_FILE_NOT_FOUND;

  /**************************************
  * Make the Get header Call to the
  * I/O Proc concerned.
  ***************************************/

  ulrc = mmioGetHeader (ulpInstance->hmmio,
                        (PVOID)&(ulpInstance->mmAudioHeader),
                        sizeof(ulpInstance->mmAudioHeader),
                        (PLONG)&BytesRead,
                        (ULONG)NULL,
                        (ULONG)NULL);

  if ( ulrc == MMIO_SUCCESS ) {

      /******************************************
          * Intialize AudioIF members
      ******************************************/
      AMPMIX.sMode =       WAVEHDR.usFormatTag;
      AMPMIX.sChannels = WAVEHDR.usChannels;
      AMPMIX.lSRate =     WAVEHDR.ulSamplesPerSec;
      AMPMIX.lBitsPerSRate = WAVEHDR.usBitsPerSample;
      ulpInstance->mmckinfo.ckSize =  XWAVHDR.ulAudioLengthInBytes;
      AMPMIX.ulBlockAlignment = ( ULONG )WAVEHDR.usBlockAlign;
      ulpInstance->ulAverageBytesPerSec = WAVEHDR.ulAvgBytesPerSec;
      if ( ulpInstance->ulAverageBytesPerSec == 0 )
         {

         ulpInstance->ulAverageBytesPerSec = WAVEHDR.usChannels * WAVEHDR.ulSamplesPerSec * ( WAVEHDR.usBitsPerSample / 8 );
         }

  } /* SuccesFul GetHeader */


        return (ulrc);
}

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
  LONG  lBogus;

  ulrc = MCIERR_SUCCESS;

  if (ulpInstance->hmmio == (ULONG)NULL)
      return MCIERR_FILE_NOT_FOUND;

  WAVEHDR.usFormatTag = AMPMIX.sMode;
  WAVEHDR.usChannels = AMPMIX.sChannels;
  WAVEHDR.ulSamplesPerSec = AMPMIX.lSRate;
  XWAVHDR.ulAudioLengthInMS = 0;
  WAVEHDR.usBitsPerSample = (USHORT)AMPMIX.lBitsPerSRate;
  WAVEHDR.usBlockAlign = (USHORT)AMPMIX.ulBlockAlignment;

  /********************************************
  * Send Set Header Info MSg to WAVE IO Proc
  ********************************************/
  ulrc = mmioSetHeader( ulpInstance->hmmio,
                        &ulpInstance->mmAudioHeader,
                        sizeof( MMAUDIOHEADER ),
                        &lBogus,
                        0,
                        0 );

  if (ulrc)
      return (MCIERR_DRIVER_INTERNAL);



        return (ulrc);
}







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
VOID SetWaveDeviceDefaults (INSTANCE * ulpInstance, ULONG ulOperation)
{

  AMPMIX.sMode = DEFAULT_MODE;                  // DATATYPE_WAVEFORM
  AMPMIX.lSRate =DEFAULT_SAMPLERATE;            // 22 Khz
  AMPMIX.ulOperation =ulOperation;              // Play or Record
  AMPMIX.sChannels = DEFAULT_CHANNELS;          // Stereo
  AMPMIX.lBitsPerSRate = DEFAULT_BITSPERSAMPLE; // 8 bits/sam
  AMPMIX.ulBlockAlignment = DEFAULT_BLOCK_ALIGN;// 8 bits/sam
  ulpInstance->ulAverageBytesPerSec = DEFAULT_CHANNELS * ( DEFAULT_BITSPERSAMPLE / 8 ) * DEFAULT_SAMPLERATE;

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
* INTERNAL REFERENCES: HhpAllocMem().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
VOID VSDInstToWaveSetParms (LPMCI_WAVE_SET_PARMS lpWaveSetParms, INSTANCE * ulpInstance)
{

  lpWaveSetParms->nChannels =    AMPMIX.sChannels;
  lpWaveSetParms->wFormatTag =   AMPMIX.sMode;
  lpWaveSetParms->nSamplesPerSec = AMPMIX.lSRate;
  lpWaveSetParms->wBitsPerSample = (WORD)AMPMIX.lBitsPerSRate;
}
/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME:AssocMemPlayToAudioStrm ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Associate The Memory Play List Stream Handler with its Data Object.
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
* INTERNAL REFERENCES:SpiAssociate()
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
RC AssocMemPlayToAudioStrm (INSTANCE * ulpInstance, ULONG Operation)
{
  ULONG ulrc = MCIERR_SUCCESS;

  /******************************************************
  * Fill in the Play List ACB with the right info
  ******************************************************/
  STREAM.acbPlayList.ulObjType = ACBTYPE_MEM_PLAYL;
  STREAM.acbPlayList.ulACBLen = sizeof(ACB_MEM_PLAYL);
  STREAM.acbPlayList.pMemoryAddr= (PVOID)ulpInstance->pPlayList;

  if (Operation == PLAY_STREAM) {
      ulrc = SpiAssociate (STREAM.hStream,
                           STREAM.hidASource,
                           (PVOID) &(STREAM.acbPlayList));
  }
  else

      if (Operation == RECORD_STREAM) {
          ulrc = SpiAssociate (STREAM.hStream,
                               STREAM.hidATarget,
                               (PVOID)&(STREAM.acbPlayList));
      }

  return (ulrc);

}

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: MCD_EnterCrit                                           */
/*                                                                          */
/* FUNCTION: This routine acquires access to the common areas via a         */
/*           system semaphore.                                              */
/*                                                                          */
/* NOTES:    This routine contains OS/2 system specific functions.          */
/*           DosRequestMutexSem                                             */
/*                                                                          */
/* INPUT:    None.                                                          */
/*                                                                          */
/* OUTPUT:   rc = error return code is failure to acquire semaphore.        */
/*                                                                          */
/* SIDE EFFECTS: Access acquired.                                           */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

DWORD MCD_EnterCrit (INSTANCE * ulpInstance )
{
  /**************************************************************************/
  /*                                                                        */
  /*   Request the system semaphore for the common data area.               */
  /*                                                                        */
  /**************************************************************************/
  return((DWORD)DosRequestMutexSem (ulpInstance->hmtxDataAccess, -1));  // wait for semaphore
}





/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: MCD_ExitCrit                                           */
/*                                                                          */
/* FUNCTION: This routine releases access to the common areas via a         */
/*           system semaphore.                                              */
/*                                                                          */
/* NOTES:    This routine contains OS/2 system specific functions.          */
/*           DosReleaseMutexSem                                             */
/*                                                                          */
/* INPUT:    None.                                                          */
/*                                                                          */
/* OUTPUT:   rc = error return code is failure to release semaphore.        */
/*                                                                          */
/* SIDE EFFECTS: Access released.                                           */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

DWORD MCD_ExitCrit (INSTANCE * ulpInstance)
{
  /**************************************************************************/
  /*                                                                        */
  /*   Release the system semaphore for the common data area.               */
  /*                                                                        */
  /**************************************************************************/

  return((DWORD)DosReleaseMutexSem (ulpInstance->hmtxDataAccess));
     // release semaphore
}


/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: AcquireProcSem                                          */
/*                                                                          */
/* FUNCTION: This routine acquires access to the common areas via a         */
/*           system semaphore.                                              */
/*                                                                          */
/* NOTES:    This routine contains OS/2 system specific functions.          */
/*           DosRequestMutexSem                                             */
/*                                                                          */
/* INPUT:    None.                                                          */
/*                                                                          */
/* OUTPUT:   rc = error return code is failure to acquire semaphore.        */
/*                                                                          */
/* SIDE EFFECTS: Access acquired.                                           */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

DWORD AcquireProcSem ()
{
  extern HMTX   hmtxProcSem;
  /**************************************************************************/
  /*                                                                        */
  /*   Request the system semaphore for the common data area.               */
  /*                                                                        */
  /**************************************************************************/

  return((DWORD)DosRequestMutexSem (hmtxProcSem, -1));  // wait for semaphore
}

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ReleaseProcSem                                          */
/*                                                                          */
/* FUNCTION: This routine releases access to the common areas via a         */
/*           system semaphore.                                              */
/*                                                                          */
/* NOTES:    This routine contains OS/2 system specific functions.          */
/*           DosReleaseMutexSem                                             */
/*                                                                          */
/* INPUT:    None.                                                          */
/*                                                                          */
/* OUTPUT:   rc = error return code is failure to release semaphore.        */
/*                                                                          */
/* SIDE EFFECTS: Access released.                                           */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

DWORD ReleaseProcSem ()
{
   extern HMTX  hmtxProcSem;
  /**************************************************************************/
  /*                                                                        */
  /*   Release the system semaphore for the common data area.               */
  /*                                                                        */
  /**************************************************************************/

  return((DWORD)DosReleaseMutexSem (hmtxProcSem));// release semaphore
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
  * Free Memory off heap
  ****************************/

  HhpFreeMem (heap, MemToFree);

  /****************************
  * Exit Data Critical Section
  *****************************/

  ReleaseProcSem ();

  return (ULONG)(MCIERR_SUCCESS);

}


/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ReleaseInstanceMemory ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Call CleanUp to dealocatte Instance memory from global heap.
*
*
* NOTES:    Release Memory.
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
* INTERNAL REFERENCES: HhpFreeMem().
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC ReleaseInstanceMemory (FUNCTION_PARM_BLOCK * pThreadBlock)
{

  /******************************************
  * Free Thread Parm Block & Assoc Pointers
  *******************************************/
  CleanUp ((PVOID) pThreadBlock->pInstance);
  CleanUp ((PVOID)pThreadBlock);

  return (ULONG)(MCIERR_SUCCESS);

}
/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: InstallIOProc
*
* DESCRIPTIVE NAME: ""
*
* FUNCTION: Install Customized IO Procs Like AVC IO Proc and WAVE IO Proc.
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
* INTERNAL REFERENCES: WAVE IO Proc
*
* EXTERNAL REFERENCES: DosQueryProcAddr - OS/2 API.
*
*********************** END OF SPECIFICATIONS *******************************/
RC InstallIOProc (ULONG ulIOProc, MMIOINFO * pmmioinfo)
  {


  CHAR             Failure[ 100 ];
  CHAR             ModuleName[124];
  CHAR             EntryPoint[124];
  ULONG            rc;
  HMODULE          hMod;
  LPMMIOPROC       pIoProc, pAnswer;

  /**********************************************
  * Clear structures used off the stack
  ***********************************************/
  memset (&ModuleName, '\0', sizeof(ModuleName));
  memset (&EntryPoint, '\0', sizeof(EntryPoint));
  memset (&Failure, '\0', sizeof(Failure));

  if (ulIOProc == WAVE_IO_PROC) {

      /***************************************************
      * Fill in FOURCC, ModuleName and EntryPoint Info
      ****************************************************/

      pmmioinfo->fccIOProc = mmioFOURCC( 'W', 'A', 'V', 'E' ) ;
      strcpy( ModuleName, "WAVEPROC" );
      strcpy( EntryPoint, "WAVEIOProc");
  }
  else
      if (ulIOProc == AVC_IO_PROC) {

      /***************************************************
       * Fill in FOURCC, ModuleName and EntryPoint Info
      ****************************************************/
           pmmioinfo->fccIOProc = mmioFOURCC( 'A', 'V', 'C', 'A' ) ;
           strcpy( ModuleName, "AVCAPROC" );
           strcpy( EntryPoint, "AVCAIOProc");
       }
  /******************************************
  * See if it is a known IO Proc
  *******************************************/
  pAnswer = mmioInstallIOProc(pmmioinfo->fccIOProc,
                              NULL, MMIO_FINDPROC );
  if (!pAnswer ) {
      /*************************************
      * Load The IO Proc
      *************************************/
      rc = DosLoadModule( Failure, 100L, ModuleName, &(hMod));
      if (rc)
          return (MCIERR_CANNOT_LOAD_DRIVER);

      /**************************************
      * Obtain Procedure Entry Point
      ***************************************/
      rc = DosQueryProcAddr( hMod, (ULONG)NULL,
                             EntryPoint, ( PFN *) &pIoProc );
      if (rc)
          return(rc);
      /****************************************
      * Install the IO Proc
      *****************************************/
      pAnswer = mmioInstallIOProc(pmmioinfo->fccIOProc,
                                  pIoProc, MMIO_INSTALLPROC );

  }



   return MCIERR_SUCCESS;
}

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
* EFFECTS:
*
* INTERNAL REFERENCES: None
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS *******************************/
RC OpenFile(INSTANCE * pInstance, DWORD dwFlags )
{

  MMIOINFO         mmioinfo;             // mmioInfo Structure
  MMIOINFO         mmioinfoCopy;         // Copy of struct if we need to retry
  MMFORMATINFO     mmformatinfo;         // Format information
  FOURCC           fccStorageSystem;     // FCC Storage System
  PSZ              pFileName;            // Element Name
  USHORT           useAvcProc;           // Use AVCA IO Proc


  /********************************
  * Intialize Data Structs
  ********************************/
  ULONG ulrc = MCIERR_SUCCESS;
  useAvcProc = FALSE;
  memset( &mmioinfo, '\0', sizeof( MMIOINFO ));

  /**************************************
  * By Default install wave IO Proc
  * If already installed function
  * just returns success
  **************************************/
  ulrc = InstallIOProc(WAVE_IO_PROC,
                      (MMIOINFO *)&mmioinfo);

  if (ulrc)
     return (MCIERR_CANNOT_LOAD_DRIVER);

  /************************************
  * Set Header Translation Flag on
  *************************************/
  mmioinfo.dwTranslate = MMIO_TRANSLATEHEADER;

  if ( pInstance->mmioHndlPrvd == TRUE) {
      pFileName = (ULONG)NULL;
      mmioinfo.hmmio = pInstance->hmmio;
  }
  else
     {
     pFileName = pInstance->lpstrAudioFile;
     pInstance->usFileExists = TRUE;
     }

  /*************************************
  * Do the Open. If mmiohandle is
  * provided a psuedo open is done
  * Through the WAVE IO Proc with
  * NULL File Name
  *************************************/
  if (pInstance->mmioHndlPrvd != TRUE) {

      if ( pInstance->ulCreatedName )
         {
         mmioinfo.adwInfo[ 0 ] = pInstance->hTempFile;
         pFileName = ( PSZ ) NULL;
         }

      memmove( &mmioinfoCopy, &mmioinfo, sizeof( MMIOINFO ) );

      /*********************************************************
      * Default is to allow saves to be performed without a file
      * name unless otherwise indicated
      **********************************************************/
      pInstance->ulNoSaveWithoutName = FALSE;

      pInstance->hmmio = mmioOpen ( pFileName,
                                    &mmioinfo,
                                    dwFlags );

      /*********************************************************
       * Find RIFF Chunk with Form Type WAVE for speed reasons
      **********************************************************/
      if ( pInstance->hmmio == (ULONG)NULL ) {

          if (mmioinfo.dwErrorRet == ERROR_ACCESS_DENIED ||
              mmioinfo.dwErrorRet == ERROR_SHARING_VIOLATION )
              {
              return MCIERR_FILE_ATTRIBUTE;

              } /* if a file attribute error occurred */

          if (mmioinfo.dwErrorRet == ERROR_FILE_NOT_FOUND ||
              mmioinfo.dwErrorRet == ERROR_OPEN_FAILED )
             {
             pInstance->ulCanSave   = MCI_TRUE;
             pInstance->ulCanInsert = MCI_TRUE;
             pInstance->ulCanRecord = MCI_TRUE;

             return ERROR_FILE_NOT_FOUND;
             }        /* Error File Not Found */

          if (  !pInstance->hmmio )
             {
             /*****************************************************
             * Do autoidentify if main I/O proc can't ID the File
             ******************************************************/

             ulrc = InstallIOProc( AVC_IO_PROC,
                                   (MMIOINFO *) &mmioinfo);
             useAvcProc = TRUE;

             if (ulrc)
                return (MCIERR_CANNOT_LOAD_DRIVER);


             ulrc = mmioIdentifyFile( pFileName,
                                      (ULONG)NULL,
                                      &mmformatinfo,
                                      &fccStorageSystem,
                                      0,
                                      0 );

             if ( ulrc == MMIO_SUCCESS )

                {
                /***********************************************
                * if this is a non audio I/O proc, don't open
                ************************************************/
                if ( mmformatinfo.dwMediaType != MMIO_MEDIATYPE_AUDIO )
                    {
                    return MCIERR_INVALID_MEDIA_TYPE;
                    }

                if ( !(mmformatinfo.dwFlags & MMIO_CANSAVETRANSLATED) &&
                     ( ( dwFlags & MMIO_WRITE ) ||
                     ( dwFlags & MMIO_READWRITE ) ) )
                   {
                   dwFlags &= ~MMIO_WRITE;
                   dwFlags &= ~MMIO_READWRITE;
                   dwFlags &= ~MMIO_EXCLUSIVE;
                   dwFlags |= MMIO_READ | MMIO_DENYNONE;
                   pInstance->ulOpenTemp = FALSE;
                   pInstance->dwmmioOpenFlag = dwFlags;
                   }

                memset( &mmioinfo, '\0', sizeof( mmioinfo ) );

                // need to find out why a mmioinfo struct cause identify to fail

                mmioinfo.dwTranslate = MMIO_TRANSLATEDATA;

                if ( pInstance->ulCreatedName )
                   {
                   mmioinfo.adwInfo[ 0 ] = pInstance->hTempFile;
                   pFileName = ( PSZ ) NULL;
                   }

                pInstance->hmmio = mmioOpen ( pFileName,
                                              &mmioinfo,
                                              dwFlags);


                if (pInstance->hmmio == (ULONG)NULL)
                  {
                  if ( mmioinfo.dwErrorRet == ERROR_ACCESS_DENIED ||
                       mmioinfo.dwErrorRet == ERROR_SHARING_VIOLATION )
                    {
                    return MCIERR_FILE_ATTRIBUTE;
                    }

                  return  ( mmioinfo.dwErrorRet );
                  }

                pInstance->usFileExists = TRUE;

                if (mmformatinfo.dwFlags & MMIO_CANSAVETRANSLATED)
                   {
                   pInstance->ulCanSave = MCI_TRUE;
                   }
                else
                   {
                   pInstance->ulCanSave = MCI_FALSE;
                   }

                if (mmformatinfo.dwFlags & MMIO_CANINSERTTRANSLATED )
                   {
                   pInstance->ulCanInsert = MCI_TRUE;
                   }
                else
                   {
                   pInstance->ulCanInsert = MCI_FALSE;
                   }

                if ( mmformatinfo.dwFlags & MMIO_CANWRITETRANSLATED)
                   {
                   pInstance->ulCanRecord = MCI_TRUE;
                   }
                else
                   {
                   pInstance->ulCanRecord = MCI_FALSE;
                   }
                }
             else
                {
                /**********************************
                * need an improved error code here
                ***********************************/
                return ERROR_FILE_NOT_FOUND;

                } /* if !success on 2nd open */

             } /* if !hmmio has not been opened */

      } /* hmmio == Null */

   else
      {
        pInstance->ulCanSave   = MCI_TRUE;
        pInstance->ulCanInsert = MCI_TRUE;
        pInstance->ulCanRecord = MCI_TRUE;
      }
  } /* mmio handle Provided not true */

  /******************************************
   * Get The Header Information (RIFF + AVC)
  *******************************************/
  if (!(dwFlags & MMIO_CREATE))
     {

      ulrc = GetAudioHeader (pInstance);

     } /* Not Create Flag */
  else
     {
     pInstance->mmckinfo.ckSize = 0;
     }

  /*******************************************************************
  * You cannot do the set header immediately after file creation
  * because future sets on samples, bitpersample, channels may follow
  ********************************************************************/
  /*
  else
  {
      ulrc = SetAudioHeader (pInstance);
  }
  */

  return (ULONG)(ulrc);
  }




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ReportMMPMerrors
*
* DESCRIPTIVE NAME: Process Error Conditions for Play and Record
*
* FUNCTION: Report Errors, Do PreProcessing for MCI Messages Play
*           and Record. Preprocessing Includes Pending Notifies and
*           Element Creation Through WaveIOProc.
*
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
* INTERNAL REFERENCES: WaveIOPROC.
*
* EXTERNAL REFERENCES: DosQueryProcAddr - OS/2 API.
*
*********************** END OF SPECIFICATIONS *******************************/

RC ReportMMEErrors (USHORT usMessage, FUNCTION_PARM_BLOCK *pFuncBlock)
{

  ULONG       ulrc;               // RC
  ULONG       ulErr;              // Holds error return codes
  ULONG       lCnt;               // Semaphore Posts
  ULONG       ulParam1;           // Incoming MCI Flags
  ULONG       ulAbortNotify = FALSE;
  USHORT      usAbortType;
  INSTANCE*   ulpInstance;        // Local Instance
  DWORD       dwPlayFlags;        // Mask for MCI Play
  DWORD       dwRecordFlags;      // Mask for MCI Record
  DWORD       dwMMFileLength = 0; // Length of the File in MMTIME
  DWORD       dwTemp1 = 0;        // Scratch For z Time Conversion
  DWORD       dwTemp2 = 0;        // Scratch for Play From
  DWORD       dwTempTO = 0;       // Scratch for Play Till
  DWORD       dwFromPosition = 0; // holds where we will play from
  LPMCI_PLAY_PARMS   lpPlayParms;  // Msg Data Ptr
  LPMCI_RECORD_PARMS   lpRecordParms;  // Msg Data Ptr

  /******************************************/
  // Intialize The local VARS
  /******************************************/
  ulrc = MCIERR_SUCCESS;
  ulParam1 = pFuncBlock->ulParam1;
  ulpInstance = (INSTANCE *)pFuncBlock->ulpInstance;

  switch (usMessage)
  {
  case MCI_PLAY:
      {
      /*********************************/
      // Check For Illegal Flags
      /**********************************/
      dwPlayFlags = pFuncBlock->ulParam1;
      dwPlayFlags &= ~(MCI_FROM + MCI_TO + MCI_NOTIFY + MCI_WAIT);

      if (dwPlayFlags > 0)
          return MCIERR_INVALID_FLAG;
      /*******************************************/
      // Checkto see If A Valid Element Specified
      /*******************************************/

      if (ulpInstance->usPlayLstStrm == FALSE)
          if (ulpInstance->usFileExists != TRUE)
              return MCIERR_FILE_NOT_FOUND;

      /****************************************/
       // Do we have a valid Msg Data Struct Ptr
      /*****************************************/
      if ((pFuncBlock->ulParam1 & MCI_TO) ||
          (pFuncBlock->ulParam1 & MCI_FROM) ||
          (pFuncBlock->ulNotify))

          ulrc = CheckMem ((PVOID)pFuncBlock->ulParam2,
                           sizeof (MCI_PLAY_PARMS), PAG_READ);


          if (ulrc != MCIERR_SUCCESS)
              return MCIERR_MISSING_PARAMETER;

      ConvertTimeUnits ( ulpInstance,
                         &dwMMFileLength,
                         FILE_LENGTH);

      dwTemp1 = dwMMFileLength;

      ConvertToMM( ulpInstance, &dwMMFileLength, dwMMFileLength );


      /***********************************
      * Do a Seek to support FROM
      ***********************************/

      lpPlayParms= (LPMCI_PLAY_PARMS )pFuncBlock->ulParam2;
      if (ulParam1 & MCI_FROM)
         {

          if (ulpInstance->usPlayLstStrm != TRUE)

           {
           if ( lpPlayParms->dwFrom > dwTemp1 && AMPMIX.ulOperation != OPERATION_RECORD )
              {
              return MCIERR_OUTOFRANGE;
              }
           else
              {
               if ( ulpInstance->ulCreateFlag != PREROLL_STATE )
                  {
                  if ( lpPlayParms->dwFrom > dwTemp1 )
                    {
                    return MCIERR_OUTOFRANGE;
                    }
                  return MCIERR_SUCCESS;

                  } /* if not in preroll state */

                  ulrc = SpiGetTime( STREAM.hStream,
                                     ( PMMTIME ) &( STREAM.mmStreamTime ) );

                  if ( ulrc )
                    {
                    return ( ulrc );
                    }

                  if ( AMPMIX.ulOperation == OPERATION_RECORD )
                     {

                     // if record, then our to point must be less than
                     // what we have recorded so far

                     ulrc = ConvertToMM ( ulpInstance,
                                          (DWORD*) &dwTemp1,
                                          lpPlayParms->dwFrom );

                     if ( dwTemp1 > (DWORD ) STREAM.mmStreamTime &&
                          dwTemp1 > dwMMFileLength )
                       {
                       return MCIERR_OUTOFRANGE;
                       }
                     }
                  else
                    {
                    // ensure that if the to flag is specified, it will realize that
                    // the play will start from the from position

                     ulrc = ConvertToMM ( ulpInstance,
                                          (DWORD*) &dwFromPosition,
                                          lpPlayParms->dwFrom );
                    }


              }
           } /* Non PlayList */
         } /* Play From */

      /************************************************************
      * Enable a Single Non Recurring TIme Event to Support MCI_TO
      ************************************************************/

      if (ulParam1 & MCI_TO)
         {
         if (ulpInstance->usPlayLstStrm != TRUE)
            {
            /******************************
            * Range Checking on TO
            *******************************/

            if ( lpPlayParms->dwTo > dwTemp1 && AMPMIX.ulOperation != OPERATION_RECORD )
               {
               return MCIERR_OUTOFRANGE;
               }
            else
               {
               if ( ulParam1 & MCI_FROM )
                  {
                  if ( lpPlayParms->dwTo <= lpPlayParms->dwFrom )
                     {
                     return MCIERR_OUTOFRANGE;
                     }
                  }

               if ( ulpInstance->ulCreateFlag != PREROLL_STATE )
                  {
                  if ( lpPlayParms->dwTo == 0 )
                    {
                    return MCIERR_OUTOFRANGE;
                    }
                  return MCIERR_SUCCESS;
                  }

               ulrc = SpiGetTime( STREAM.hStream,
                                  ( PMMTIME ) &( STREAM.mmStreamTime ) );

               if ( ulrc )
                  {
                  return ( ulrc );
                  }

               // if the to flag was not passed in, then set the from position
               // to be equivalent to our current position

               if ( dwFromPosition == 0 &&
                    !(ulParam1 & MCI_FROM ) )
                  {
                  dwFromPosition = ( DWORD ) STREAM.mmStreamTime;
                  }

               if ( AMPMIX.ulOperation == OPERATION_RECORD )
                  {

                  // if record, then our to point must be less than
                  // what we have recorded so far

                  ulrc = ConvertToMM ( ulpInstance,
                                       (DWORD*) &dwTempTO,
                                       lpPlayParms->dwTo);


                  // if we are past what we have recorded so far and past the
                  // length of the file in case of recording in the middle
                  // return an error

                  if ( dwTempTO > (DWORD ) STREAM.mmStreamTime &&
                       dwTempTO > dwMMFileLength )
                    {
                    return MCIERR_OUTOFRANGE;
                    }
                  }
               else
                  {
                  ulrc = ConvertToMM ( ulpInstance,
                                       (DWORD*) &dwTempTO,
                                       lpPlayParms->dwTo);

                  if ( dwTempTO <= ( DWORD ) STREAM.mmStreamTime &&
                       dwTempTO <= dwFromPosition )
                    {
                    /********************************************************
                    * it is possible that we had rounding problems so ensure
                    * that user did indeed pass in an illegal value
                    ********************************************************/
                    if ( ( ulParam1 & MCI_FROM ) &&
                         !(lpPlayParms->dwTo <= lpPlayParms->dwFrom ) )
                       {
                       break;
                       }

                    return MCIERR_OUTOFRANGE;
                    }
                  }
               }

            }

         } // Of Play Till XXXX
       } /* Case of MCI Play */
      break;

  case MCI_RECORD:
       {

       dwRecordFlags = pFuncBlock->ulParam1;
       /****************************************
       * Check for Invalid Flags
       ****************************************/
       dwRecordFlags &= ~(MCI_FROM + MCI_TO + MCI_RECORD_INSERT +
                          MCI_RECORD_OVERWRITE + MCI_WAIT + MCI_NOTIFY);

       if (dwRecordFlags > 0)
           return MCIERR_INVALID_FLAG;

       if (ulpInstance == (ULONG)NULL )
           return MCIERR_INSTANCE_INACTIVE;


       /************************************************/
       // Check For Unsupported Flags
       /************************************************/

       if( (pFuncBlock->ulParam1 & MCI_RECORD_INSERT) &&
            (pFuncBlock->ulParam1 & MCI_RECORD_OVERWRITE))

             return MCIERR_FLAGS_NOT_COMPATIBLE;

       /************************************************/
       // Check For Unsupported Flags
       /************************************************/
       if (!(pFuncBlock->ulParam1 & MCI_RECORD_INSERT))
           pFuncBlock->ulParam1 |= MCI_RECORD_OVERWRITE;

       /***********************************************/
                  // Check For Valid Element
       /***********************************************/
       if (ulpInstance->usFileExists == UNUSED)
           return MCIERR_FILE_NOT_FOUND;

       ulrc = CheckMem ( (PVOID)pFuncBlock->ulParam2,
                         sizeof (MCI_RECORD_PARMS), PAG_READ);


       if (ulrc != MCIERR_SUCCESS)
          {
          return MCIERR_MISSING_PARAMETER;
          }

       if ( !ulpInstance->ulUsingTemp && !ulpInstance->usPlayLstStrm )
          {
          return MCIERR_UNSUPPORTED_FUNCTION;
          }

       lpRecordParms= (LPMCI_RECORD_PARMS )pFuncBlock->ulParam2;

      ConvertTimeUnits ( ulpInstance,
                         &dwMMFileLength,
                         FILE_LENGTH);

      dwTemp1 = dwMMFileLength;

      ConvertToMM( ulpInstance, &dwMMFileLength, dwMMFileLength );


// if future, with playlist we should check to see if the stream is active
// if it isn't then do a seek to end to determine the length

       if (ulParam1 & MCI_FROM)
          {
          // we cannot perform length checks on play list streams

          if ( lpRecordParms->dwFrom > dwTemp1 && ulpInstance->usPlayLstStrm != TRUE )
            {
            return MCIERR_OUTOFRANGE;
            }

          // the play will start from the from position

          ulrc = ConvertToMM ( ulpInstance,
                               (DWORD*) &dwFromPosition,
                               lpRecordParms->dwFrom );

          }  /* Record From */

       if (ulParam1 & MCI_TO)
          {

          if ( lpRecordParms->dwTo <= 0 )
            {
            return MCIERR_OUTOFRANGE;
            }

          if ( ulParam1 & MCI_FROM )
            {
            // if the user wants to record behind the request return an error

            if ( lpRecordParms->dwTo <= lpRecordParms->dwFrom )
               {
               return MCIERR_OUTOFRANGE;
               }

            } /* if the from flag specified */

          if ( ulpInstance->ulCreateFlag == PREROLL_STATE )
             {
             ulrc = SpiGetTime( STREAM.hStream,
                                ( PMMTIME ) &( STREAM.mmStreamTime ) );

             if ( ulrc )
                {
                return ( ulrc );
                }

             ConvertToMM ( ulpInstance,
                           (DWORD*) &dwTempTO,
                           lpRecordParms->dwTo );


             // if the to flag was not passed in, then set the from position
             // to be equivalent to our current position

             if ( dwFromPosition == 0 &&
                  !(ulParam1 & MCI_FROM ) )
                {
                dwFromPosition = ( DWORD ) STREAM.mmStreamTime;
                }

             if ( dwTempTO <= STREAM.mmStreamTime &&
                  dwTempTO <= dwFromPosition         )
                {
                /********************************************************
                * it is possible that we had rounding problems so ensure
                * that user did indeed pass in an illegal value
                ********************************************************/
                if ( ( ulParam1 & MCI_FROM ) &&
                     !(lpRecordParms->dwTo <= lpRecordParms->dwFrom ) )
                   {
                   break;
                   }

                return MCIERR_OUTOFRANGE;
                }

             } /* if stream has been created */


          }


       /***********************************************
       * The Previous Notifies belong here because
       * File open is done here.
       ************************************************/
       DosRequestMutexSem( ulpInstance->hmtxNotifyAccess, -1 );

       if (ulpInstance->usNotifyPending == TRUE)
          {
          ulpInstance->ulNotifyAborted = TRUE;
          ulpInstance->usNotifyPending = FALSE;
          ulAbortNotify = TRUE;
          }

       DosReleaseMutexSem( ulpInstance->hmtxNotifyAccess);

       if ( ulAbortNotify == TRUE) {
           if (ulpInstance->usNotPendingMsg == MCI_RECORD) {

           if ( ulParam1 & MCI_NOTIFY )
              {
              usAbortType = MCI_NOTIFY_SUPERSEDED;
              }
           else
              {
              usAbortType = MCI_NOTIFY_ABORTED;
              }
           PostMDMMessage ( usAbortType,
                            MCI_RECORD, pFuncBlock);

           /*******************************************/
           // Reset UserParm for Current Record
           /*******************************************/
           if (ulParam1 & MCI_NOTIFY)
               ulpInstance->usUserParm = pFuncBlock->usUserParm;

           /****************************************
           * Reset Internal Semaphores
           ****************************************/
           DosResetEventSem (ulpInstance->hEventSem, &lCnt);
           DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

           /***************************
           * Stop The Stream
           ***************************/
           ulrc = SpiStopStream (STREAM.hStream,
                                 SPI_STOP_DISCARD);
           if (!ulrc)
              {
              /*****************************************
              * Wait for Previous Thread to Die
              *****************************************/

              DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);
              }
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

              STRMSTATE = MCI_STOP;

              ulrc = MCIERR_SUCCESS;

              /***********************************
              * Update Internal States
              ************************************/
              ulpInstance->ulCreateFlag = PREROLL_STATE;

           } /* Superseded */
          else if ( ulpInstance->usNotPendingMsg == MCI_SAVE )
             {
             // Save is a non-interruptible operation
             // wait for completion

             DosWaitEventSem( ulpInstance->hThreadSem, ( ULONG ) -1 );
             }
           else
              {
              PostMDMMessage (MCI_NOTIFY_ABORTED, MCI_PLAY, pFuncBlock);
              DosResetEventSem (ulpInstance->hEventSem, &lCnt);
              DosResetEventSem (ulpInstance->hThreadSem, &lCnt);

              if (ulParam1 & MCI_NOTIFY)
                  ulpInstance->usUserParm = pFuncBlock->usUserParm;
              /********************************
              * Stop The Stream
              ********************************/
              ulrc = SpiStopStream (STREAM.hStream,
                                    SPI_STOP_DISCARD);
              if (!ulrc)
                  {
                  /*****************************************/
                  // Wait for Previous Thread to Die
                  /*****************************************/
                  DosWaitEventSem (ulpInstance->hThreadSem, (ULONG) -1);

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
                  }  /* no RC */
           }  /* Case of Record aborting a Play */
       }      /* Notify Pending   */

       /*******************************************/
       // Element was Open with Read Flag on
       // at open time.
       /*******************************************/
       if (ulpInstance->usPlayLstStrm != TRUE) {
           if (STREAM.ulState != CUERECD_STATE) {
               if ( !( ulpInstance->dwmmioOpenFlag & MMIO_WRITE ) &&
                    !( ulpInstance->dwmmioOpenFlag & MMIO_READWRITE )  &&
                    !( ulpInstance->mmioHndlPrvd ) ) {

                      /*******************************************/
                      // Destroy if a PlayBack Stream was created
                       /*******************************************/
                       if (ulpInstance->ulCreateFlag == PREROLL_STATE)
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

                           DestroyStream (STREAM.hStream);
                           ulpInstance->ulCreateFlag = CREATE_STATE;
                           }

                       if (ulpInstance->hmmio != (ULONG)NULL)
                           ulrc = mmioClose (ulpInstance->hmmio, 0);

                       ulpInstance->dwmmioOpenFlag &= ~MMIO_READ;
                       ulpInstance->hmmio = (ULONG)NULL;
               } /* MMIO_READ Flag on */
           } /* Cue Record State */
       } /* Non PlayList */


       if (ulpInstance->usPlayLstStrm != TRUE) {

           /**********************************************/
           // Open The element if not already Open
           // This always open the element with create
           // flag on once, at start of record.
           /**********************************************/
           if (ulpInstance->hmmio == (ULONG)NULL) {

               /********************************/
                  // Update mmioOpen Flags
               /*******************************/
               if (ulpInstance->usFileExists == FALSE)
                   {
                   ulpInstance->dwmmioOpenFlag = MMIO_CREATE;
                   }

                ulpInstance->dwmmioOpenFlag |= MMIO_READWRITE|MMIO_EXCLUSIVE;
                ulpInstance->dwmmioOpenFlag &= ~MMIO_DENYNONE;

                /********************************
                * Open The Element
                *******************************/

                   ulErr = OpenFile (ulpInstance, ulpInstance->dwmmioOpenFlag);

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

           } /* hmmio != NULL */

           if (pFuncBlock->ulParam1 & MCI_RECORD_INSERT)
               {
               ulpInstance->usRecdInsert = TRUE;

               }   /* Insert Specified */

       } /* Non PlayList Case */
       } /* Case of MCI_RECORD */
      break;
  } /* of Switch */

  return (MCIERR_SUCCESS);
}
/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ConvertToMM ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Convert Time values from MMTIME units to current base.
*
*
* NOTES:    Release Memory.
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
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC   ConvertToMM (INSTANCE * ulpInstance, DWORD *dwSeekPoint, DWORD value)
{

  ULONG      ulBytesPerSample = 0;
  ULONG      ulTemp1;


  /***************************
  * Intialize The Stack Vars
  ****************************/
  ulTemp1 = 0;

  switch (ulpInstance->ulTimeUnits)
  {

  case lMMTIME:
       *dwSeekPoint = value;
      break;

  case lMILLISECONDS:
       *dwSeekPoint = MSECTOMM (value);
      break;

  case lSAMPLES:
       ulBytesPerSample = (AMPMIX.lBitsPerSRate / 8);
       ulTemp1 = value * ulBytesPerSample;

       ulTemp1 /= DATATYPE.ulBytes ;
       *dwSeekPoint = ulTemp1 * DATATYPE.ulMMTime;

      break;

  case lBYTES:
       ulTemp1 = value / DATATYPE.ulBytes ;
       *dwSeekPoint = ulTemp1 * DATATYPE.ulMMTime;

      break;
  }

  return MCIERR_SUCCESS;

}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ConvertTimeUnits ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Convert Time values from the current base to MMTIME units.
*
*
* NOTES:    Release Memory.
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
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/

RC  ConvertTimeUnits (INSTANCE * ulpInstance, DWORD *dwSeekPoint, DWORD value)
{
  ULONG      ulBytesPerSample;
  ULONG      ulTemp1 = 0;


  // due to inaccuracies in the conversions, if the request is bytes or samples
  // simply return the number

  /**************************************
  * Computation of Media Element Length
  * This routine is called with FILE LENGTH
  * value to signify Total length is
  * requested.
  ***************************************/
  if (value == FILE_LENGTH) {

      /* due to inaccuracies in the conversions, if the request is bytes or samples
      *  simply return the number itself
      */
      if ( ulpInstance->ulTimeUnits == lBYTES )
         {
         *dwSeekPoint = ulpInstance->mmckinfo.ckSize;
         return MCIERR_SUCCESS;
         }
      else if ( ulpInstance->ulTimeUnits == lSAMPLES )
         {
         ulBytesPerSample = (AMPMIX.lBitsPerSRate / 8);
         *dwSeekPoint = ulpInstance->mmckinfo.ckSize / ulBytesPerSample;
         return MCIERR_SUCCESS;
         }

      /***********************************************
      * Get the number of blocks of audio information the
      * desired number of bytes consumes.
      *************************************************/
      ulTemp1 = ulpInstance->mmckinfo.ckSize / DATATYPE.ulBytes;



      /***********************************************
      * Multiply the blocks above by the length in time
      * of a block.
      *************************************************/

      value = ulTemp1 * DATATYPE.ulMMTime;
  }


  /****************************
  * Intialize The Stack Vars
  *****************************/

  switch (ulpInstance->ulTimeUnits)
  {
  case lMMTIME:
       *dwSeekPoint = value;
      break;

  case lMILLISECONDS:
       *dwSeekPoint = MSECFROMMM (value);
      break;

  case lSAMPLES:
       ulBytesPerSample = (AMPMIX.lBitsPerSRate / 8);

       ulTemp1 = value / DATATYPE.ulMMTime ;
       ulTemp1 *= DATATYPE.ulBytes;
       *dwSeekPoint = ulTemp1 / ulBytesPerSample;
      break;

  case lBYTES:
       ulTemp1 = value / DATATYPE.ulMMTime ;
       *dwSeekPoint = ulTemp1 * DATATYPE.ulBytes;
      break;

  } /* Of Switch */

  return MCIERR_SUCCESS;

}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: CreateNAssocStream ()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Create a stream and associate this stream with its data
*           object.
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
* INTERNAL REFERENCES:
*
* EXTERNAL REFERENCES:  SpiCreateStream ()      - SSM SPI
*                       SpiAssociate ()         - SSM SPI
*
*********************** END OF SPECIFICATIONS *******************************/

RC     CreateNAssocStream (HID hidSrc,           /* Source Handler HID */
                          HID hidTgt,            /* Target Handler HID */
                          HSTREAM *hStream,      /* Stream Handle ()   */
                          INSTANCE * pInstance,  /* Instance Pointer   */
                          ULONG  Operation,      /* Play or Record     */
                          PEVFN  EventProc)      /* Event Entry Point  */
{

  ULONG     ulrc;
  ulrc = MCIERR_SUCCESS;
  /*************************************************/
  // SpiCreateStream()
  /*************************************************/
  ulrc = SpiCreateStream (hidSrc, hidTgt,
                         (PSPCBKEY)&(pInstance->StreamInfo.SpcbKey),
                         (PDCB)&(pInstance->StreamInfo.AudioDCB),
                         (PDCB)&(pInstance->StreamInfo.AudioDCB),
                         (PIMPL_EVCB)&(pInstance->StreamInfo.Evcb),
                         (PEVFN)EventProc, (ULONG)NULL,(hStream),
                         &(pInstance->StreamInfo.hEvent));

  if (ulrc)
      return ulrc;

  /********************
  * SpiAssociate()
  *********************/
  pInstance->StreamInfo.acbmmio.ulObjType = ACBTYPE_MMIO;
  pInstance->StreamInfo.acbmmio.ulACBLen = sizeof (ACB_MMIO);
  pInstance->StreamInfo.acbmmio.hmmio = pInstance->hmmio;

  if (Operation == PLAY_STREAM) {

      if (ulrc = SpiAssociate ((HSTREAM)*hStream, hidSrc,
                               (PVOID) &(pInstance->StreamInfo.acbmmio)))
          return ulrc;
  }
  else
  if (Operation == RECORD_STREAM) {
      ulrc = SpiAssociate ((HSTREAM)*hStream, hidTgt,
                           (PVOID)&(pInstance->StreamInfo.acbmmio));
  }
  return ulrc;
}

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: DoTillEvent()
*
* DESCRIPTIVE NAME:
*
* FUNCTION: Enable a Time Event to process MCI_PLAY or MCI_RECORD with
*           the MCI_TO Flag on.
*
* NOTES:   When This Time Event is received The EventProc Signalls
*          the Blocked Thread by Posting a Semaphore. The Blocked
*          Thread awakens and stops the stream thereby stopping
*          MCI_PLAY or MCI_RECORD.
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
* INTERNAL REFERENCES: SpiEnableEvent()
*
* EXTERNAL REFERENCES:
*
*********************** END OF SPECIFICATIONS *******************************/
RC DoTillEvent (INSTANCE *ulpInstance, DWORD dwTo)
{
  ULONG ulrc = MCIERR_SUCCESS;
  /***************************************
  * Enable a Time Cue Point.
  ***************************************/
  /***************************************/
  /* Set up a  TCP at This location      */
  /* When Event handler gets controll    */
  /* stop the stream                     */
  /***************************************/

  TIMEEVCB.dwCallback = (HWND)ulpInstance->dwCallback;
  TIMEEVCB.wDeviceID = ulpInstance->wWaveDeviceID;
  TIMEEVCB.evcb.ulType = EVENT_CUE_TIME_PAUSE;
  TIMEEVCB.evcb.ulFlags = EVENT_SINGLE;
  TIMEEVCB.evcb.hstream = STREAM.hStream;
  TIMEEVCB.ulpInstance = (ULONG)ulpInstance;
  TIMEEVCB.evcb.mmtimeStream =dwTo;

  /**************************************/
  // Enable A non recurring Time Event
  /*************************************/

  ulrc = SpiEnableEvent((PEVCB) &(TIMEEVCB.evcb),
                        (PHEVENT) &(STREAM.hPlayToEvent));

  return ulrc;

}

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
*                       ULONG         ulDeviceType
*                       CHAR          szPDDName [MAX_PDD_NAME]        - OUTPUT
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
   LPMCI_SYSINFO_LOGDEVICE pSysInfoParm;
   LPMCI_SYSINFO_QUERY_NAME pQueryNameParm;
   extern HHUGEHEAP heap;

   /****************************************************
   * Get install name for ampmixer based on device type
   *****************************************************/
   AcquireProcSem ();
   if (!(pQueryNameParm = HhpAllocMem (heap,
                                       sizeof (MCI_SYSINFO_QUERY_NAME))))
       return (MCIERR_OUT_OF_MEMORY);

   ReleaseProcSem ();
   SysInfo.dwItem       = MCI_SYSINFO_QUERY_NAMES;
   SysInfo.wDeviceType  = LOUSHORT(ulDeviceType);
   SysInfo.pSysInfoParm = pQueryNameParm;

   /*********************************
   * AmpMixer name is AMPMIX01 or 02
   *********************************/
   _itoa (HIUSHORT(ulDeviceType), szIndex, 10);

     szIndex[1] = '\0';

   strncat (szAmpMix, szIndex, 2);
   strcpy (pQueryNameParm->szLogicalName, szAmpMix);

   if (rc = mciSendCommand (0,
                            MCI_SYSINFO,
                            MCI_SYSINFO_ITEM | MCI_WAIT,
                            (DWORD)&SysInfo,
                            0))
           return (rc);


   /*******************************************
   * Get PDD associated with our AmpMixer
   * Device name is in pSysInfoParm->szPDDName
   ********************************************/
   if (!(pSysInfoParm = HhpAllocMem (heap,
                                     sizeof (MCI_SYSINFO_LOGDEVICE))))
       return (MCIERR_OUT_OF_MEMORY);

   SysInfo.dwItem       = MCI_SYSINFO_QUERY_DRIVER;
   SysInfo.wDeviceType  = (WORD)ulDeviceType;
   SysInfo.pSysInfoParm = pSysInfoParm;

   strcpy (pSysInfoParm->szInstallName, pQueryNameParm->szInstallName);

   if (rc = mciSendCommand (0,
                            MCI_SYSINFO,
                            MCI_SYSINFO_ITEM | MCI_WAIT,
                            (DWORD)&SysInfo,
                            0))
       return (rc);

   strcpy (szPDDName, pSysInfoParm->szPDDName);

   //*************
   // Free memory
   //*************

   CleanUp ((PVOID) pSysInfoParm);
   CleanUp ((PVOID) pQueryNameParm);



}
/**************************************************************************
**   GenerateUniqueFile                                              **
***************************************************************************
*
* ARGUMENTS:
*
* ARGUMENTS:
*
*     pszPathName    - Possible path where to create this file.
*                      Can be empty.  The full path name of the file will
*                      be returned in this parameter.
*     pulPathLength  - Length of the path that was passed.  The length
*                      of the path name will be returned in this parameter.
*     phfile         - Open dos handle.
*
* RETURN:
*
*     0 - Success,
*     1 - Invalid parameter determined by GenerateUniqueFile Routine
*     Other - Indicates DosOpen Error return.
*
* DESCRIPTION:
*
*     This routine will attempt to open a new file.  The fully qualified
*     name of the file and the open DOS handle will be returned if
*     successful.
*
* GLOBAL VARS REFERENCED:
*
* GLOBAL VARS MODIFIED:
*
* NOTES:
*
*     If the path name parameter is specified, it can or cannot have
*     the trailing / or \ included.  The full path is returned in all
*     successful cases.
*
* SIDE EFFECTS:
*
*     A DOS file may be opened.  It is the responsibility of the caller
*     to handle housekeeping for this file.
*
***************************************************************************/

#define COLON        ':'
#define SLASH        '/'
#define BACKSLASH    '\\'
#define DOT          '.'
#define _MAX_PATH       260                 // max. length of full pathname
#define _MAX_DRIVE      3                   // max. length of drive component
#define _MAX_DIR        256                 // max. length of path component
#define _MAX_FNAME      256              // max. length of file name component
#define _MAX_EXT        256                 // max. length of extension component
ULONG APIENTRY GenerateUniqueFile( PSZ pszPathName,
                                   PULONG pulPathLength,
                                   PHFILE phfile )
{
#define MAX_RETRIES     1000000L  // should be enough, 1 million
#define RADIX_10        10
#define NAME_8_3        11
#define DOT_POSITION    8
#define EXTENSION_LEN   3
   PEAOP2 pEABuf;
   ULONG  ulActionTaken;
   ULONG  ulFileAttribute;
   ULONG  ulFileSize;
   ULONG  ulOpenFlags;
   ULONG  ulOpenMode;
   ULONG  ulReturnCode;

   CHAR szDir     [_MAX_DIR];
   CHAR szDrive   [_MAX_DRIVE + 1];
   CHAR szExt     [_MAX_EXT];
   CHAR szName    [_MAX_FNAME];
   CHAR szTempName[_MAX_FNAME];
   CHAR szTempPath[_MAX_PATH];

   ULONG cTries;
   BOOL  fUnique;

   ULONG ulCopyLength;
   ULONG ulDiskNumber;
   ULONG ulLogicalDriveMap;
   ULONG ulPathSize;
   ULONG ulTempNameLength;
   ULONG ulTempPathLength;

   if (!pszPathName || !pulPathLength || !phfile)
      {
      return (TRUE);
      }

   *phfile = 0L;

   /*
    * Zap storage.  Copy path.
    */

   memset( szDrive, '\0', sizeof(szDrive) );
   memset( szDir,   '\0', sizeof(szDir) );
   memset( szName,  '\0', sizeof(szName) );
   memset( szExt,   '\0', sizeof(szExt) );

   memset( szTempPath, '\0', sizeof(szTempPath) );

   ulTempPathLength = strlen( pszPathName );

   if (ulTempPathLength >= *pulPathLength)
      {
      return (TRUE);
      }

   /*
    * Determine if the path contains a / or \ terminator, and if not
    * tack one on to the end so _splitpath will work correctly.
    */

   if ((pszPathName[ulTempPathLength - 1L] != '/') &&
       (pszPathName[ulTempPathLength - 1L] != '\\'))
      {
      pszPathName[ulTempPathLength] = '\\';
      }

   /*
    * Determine if a path was specified.
    */

   if (*pulPathLength > 0L)
      {
      _splitpath( pszPathName,
                  szDrive,
                  szDir,
                  szName,
                  szExt );
      }

   /*
    * Fill in missing pieces of the path.
    */

   if (*szDrive == (CHAR)NULL)
      {
      DosQueryCurrentDisk( &ulDiskNumber,
                           &ulLogicalDriveMap );

      szDrive[0] = (CHAR)(ulDiskNumber + 64);
      szDrive[1] = ':';
      }
   else
      {
      ulDiskNumber = (CHAR)(szDrive[0] - 64);
      }

   strcpy( szTempPath, szDrive );

   if (*szDir == (CHAR)NULL)
      {
      szTempPath[2] = '\\';

      ulPathSize = _MAX_PATH;


      DosQueryCurrentDir( ulDiskNumber,
                          szDir,
                          &ulPathSize );

      if ((szDir[0] == '\\') ||
          (szDir[0] == '/'))
         {
         if (strlen( szDir ) > 1L)
            {
            strncpy( &szTempPath[3],
                     &szDir[1],
                     (USHORT)min( strlen( szTempPath - 3L ),
                                  _MAX_PATH - 3L ) );
            }
         else
            {
            strncpy( &szTempPath[3],
                     szDir,
                     (USHORT)min( strlen( szTempPath - 3L ),
                                  _MAX_PATH - 3L ) );
            }
         }
      }
   else
      {
      strncpy( &szTempPath[2],
               szDir,
               (USHORT)min( (*pulPathLength - 3L),
                            _MAX_PATH - 3L ) );
      }

   ulTempPathLength = strlen( szTempPath );

   if (ulTempPathLength >= _MAX_PATH)
      {
      return (TRUE);
      }

   /*
    * Make sure there is room for the filename.
    */

   if (ulTempPathLength > _MAX_PATH - NAME_8_3 + 1L)
      {
      return (TRUE);
      }

   /*
    * Tack on a name.  Go into loop.
    */

   fUnique = FALSE;

   for (cTries = 1L; cTries < MAX_RETRIES; ++cTries)
      {
      memset( szTempName,'\0', sizeof(szTempName) );
      memset( szName,  '\0', sizeof(szName) );
      strcpy( szName, "00000000.000" );

      _ltoa( cTries,
             szTempName,
             RADIX_10 );

      if ((ulTempNameLength = strlen( szTempName )) > NAME_8_3)
         {
         break;
         }

      /*
       * Make the name 8_3 format.
       */

      if (ulTempNameLength >= EXTENSION_LEN)
         {

         /*
          * Copy the extension first, then the name.
          */

         strncpy( &szName[DOT_POSITION + 1],
                  &szTempName[ulTempNameLength - EXTENSION_LEN],
                  (USHORT)EXTENSION_LEN );

         ulTempNameLength -= EXTENSION_LEN;

         if (ulTempNameLength > 0)
            {
            strncpy( &szName[DOT_POSITION - ulTempNameLength],
                     &szTempName[0],
                     (USHORT)ulTempNameLength );
            }
         }
      else
         {

         /*
          * Copy the extension.
          */

         strncpy( &szName[NAME_8_3 + 1 - ulTempNameLength],
                  &szTempName[0],
                  (USHORT)ulTempNameLength );
         }

      /*
       * There should be enough room for the name to be copied.
       * String lengths were verified above.
       */

      strcpy( &szTempPath[ulTempPathLength],
              szName );

      /*
       * Attempt to open the file.  If already existing,
       * stay in the loop.
       */

      ulOpenFlags = 0L;
      ulOpenMode = 0L;
      ulFileSize = 0L;
      ulFileAttribute = 0L;
      pEABuf = NULL;

      ulFileAttribute |= FILE_NORMAL;
      ulOpenFlags |= FILE_CREATE | OPEN_ACTION_FAIL_IF_EXISTS;
      ulOpenMode |= OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE;

      /*
       * If Open fails, stay in loop.  Eventually, a failure
       * will occur.
       */

      if ((ulReturnCode = DosOpen( szTempPath,
                                   phfile,
                                   &ulActionTaken,
                                   ulFileSize,
                                   ulFileAttribute,
                                   ulOpenFlags,
                                   ulOpenMode,
                                   pEABuf )) == 0L)
         {
         fUnique = TRUE;
         break;
         }
      else
         {
         if (ulReturnCode != ERROR_OPEN_FAILED)
            {
            break;
            }
         }
      }

   /*
    * If successful, update count and copy name. Also return length of
    * entire path with temp file name included.
    * Handle is already set.
    * Success if a 0.  On Failure the DosOpen is returned at this point.
    */

   if (fUnique)
      {
      ulCopyLength = min( *pulPathLength, strlen(szTempPath));

      strncpy( pszPathName,
               szTempPath,
               (USHORT)ulCopyLength );

      if (ulCopyLength < *pulPathLength)
         {
         pszPathName[ulCopyLength] = '\0';
         }

      *pulPathLength = strlen(szTempPath);
      }

   return (((fUnique) ? FALSE : ulReturnCode));
}
