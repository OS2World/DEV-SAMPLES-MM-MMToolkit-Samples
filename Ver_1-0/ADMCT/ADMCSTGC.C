/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: MCISTAT
*
* DESCRIPTIVE NAME: Audio MCD Status Routine
*
* FUNCTION:Get Instance Status
*
* NOTES:
*
* ENTRY POINTS:
*     LINKAGE:   CALL FAR
*
* INPUT: MCI_STATUS message.
*
* EXIT-NORMAL:Lo Word Return  Code MCIERR_SUCCESS, HighWord Contains
*             constant defining type of quantity returned.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
*
* INTERNAL REFERENCES:
*                        ConvertTimeUnits ().
*                        ConVertToMM
*                        SetAudioDevice().
*                        InitAudioDevice().
*                        SetWaveDeviceDefaults().
*                        CheckMem ().
*
* EXTERNAL REFERENCES:
*                        SpiGetTime       ()        - MME API
*                        mciSendCommand   ()        - MME API
*
*********************** END OF SPECIFICATIONS **********************/
/*static char *SCCSID = "@(#)admcstgc.c	13.9 92/04/15";*/
#define INCL_BASE                       // Base Dos APIs.
#define INCL_ERRORS                     // All the errors.

#include <os2.h>                        // OS2 includes.
#include <string.h>                     // String Functions
#include <math.h>                       // Math Functions
#include <os2medef.h>                   // MME includes files.
#include <ssm.h>                        // SSM spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmsystem.h>                   // MM System Include.
#include <audio.h>                      // Audio DD Defines
#include <mcidrv.h>                     // Mci Driver include.
#include <mmio.h>                       // MMIO Include.
#include <mcd.h>                        // AUDIO IF DriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions.
#include <audiomcd.h>                   // Component Definitions.
#include "admcfunc.h"                   // Function Prototypes

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: MCISTAT.C
*
* DESCRIPTIVE NAME: Waveform Status Routine.
*
* FUNCTION: Get Current Status of an Waveform Instance.
*
* NOTES: After the status is obtained from the device specific DLL
*        the corresponding field in the instance structure is updated
*        to reflect the most recent state.
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
* INTERNAL REFERENCES: VSDIDriverEntry().
*
* EXTERNAL REFERENCES: DosQueryProcAddr - OS/2 API.
*
*********************** END OF SPECIFICATIONS *******************************/

RC MCIStat (FUNCTION_PARM_BLOCK *pFuncBlock)

{

  ULONG               ulrc;                // Error Value
  ULONG               ulParam1;            // MCI Msg Flags
  ULONG               ulParam2;            // MCI Msg Data
  INSTANCE*           ulpInstance;         // Local Instance
  DWORD               dwFileSize;          // Length of the file
  ULONG               ulTemp1;             // Temporary Stuff
  ULONG               ulTemp2;             // Temporary Stuff
  ULONG               ulErr;               // RC
  DWORD               dwStatFlags;         // Mask For Incoming Flags
  MCI_STATUS_PARMS    WaveStat;            // Internal Use
  LPMCI_STATUS_PARMS  pParams;             // Msg Data Ptr

  /*****************************/
  // Intialize The vars
  /*****************************/
  ulrc = MCIERR_SUCCESS;
  dwFileSize = 0;
  ulTemp1 = 0;
  /******************************/
  // Derefernce Pointers
  /******************************/
  ulParam1 =   pFuncBlock->ulParam1;
  ulParam2 =   pFuncBlock->ulParam2;
  ulpInstance= (INSTANCE *)(pFuncBlock->ulpInstance);
  /*****************************/
  // Check for Invalid Flags
  /****************************/
  dwStatFlags = ulParam1;
  dwStatFlags &= ~MCI_WAIT;
  dwStatFlags &= ~MCI_NOTIFY;
  dwStatFlags &= ~ (MCI_STATUS_ITEM + MCI_TRACK);

  if (dwStatFlags > 0 )
          return MCIERR_INVALID_FLAG;

  ulrc = CheckMem ((PVOID)pFuncBlock->ulParam2,
                   sizeof (MCI_STATUS_PARMS), PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
          return MCIERR_MISSING_PARAMETER;

  if (ulParam1 & MCI_TRACK)
          return MCIERR_UNSUPPORTED_FLAG;


  pParams =    (LPMCI_STATUS_PARMS)ulParam2;
  if (ulpInstance == (ULONG)NULL )
          return MCIERR_INSTANCE_INACTIVE;

  if (ulpInstance->ulInstanceSignature != ACTIVE)
          return MCIERR_INSTANCE_INACTIVE;

  /*****************************************************************
  * Send a status request over to the devspcfc DLL
  * over the VSD Interface. All device specific status fields get
  * updated by DevSpcfc DLL. Status flags cannot be ORed.
  *****************************************************************/
  if (ulParam1 & MCI_STATUS_ITEM ) {

      switch (pParams->dwItem)
      {
      case MCI_STATUS_POSITION:
           {
           if (ulpInstance->ulCreateFlag != CREATE_STATE ) {
               STREAM.mmStreamTime = 0;
               /********************************************
               * Query the stream for current stream time
               *********************************************/
               ulrc = SpiGetTime (STREAM.hStream,
                                  (PMMTIME)&(STREAM.mmStreamTime));
               if (!ulrc)

                   /*****************************************
                   * Convert MMTIME units to current Time base
                   ******************************************/
                  ConvertTimeUnits (ulpInstance, (DWORD*)&(ulTemp1),
                                    (DWORD) (STREAM.mmStreamTime));

               pParams->dwReturn = (DWORD)ulTemp1;
           }
           else
               pParams->dwReturn = 0;  // No Stream Alive

           ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);

          }
          break;

      case MCI_STATUS_LENGTH:
           {

           if (ulpInstance->usFileExists == UNUSED)
               return (MCIERR_FILE_NOT_FOUND);

           if (ulpInstance->usPlayLstStrm == TRUE)
               return MCIERR_INDETERMINATE_LENGTH;

           if (AMPMIX.ulOperation == OPERATION_PLAY)
              {
              if ( ulpInstance->mmioHndlPrvd )
                 {
                 /*****************************************************
                 *  if the user passed in the handle, they may have
                 *  updated the size of the file so update our instance
                 *****************************************************/

                 ulrc = GetAudioHeader( ulpInstance );
                 if ( ulrc )
                   {
                   return MCIERR_DRIVER_INTERNAL;
                   }
                 }

              /******************************************
              * the function ConvertTimeUnits also
              * returns media element length in the
              * current time units.
              ******************************************/
              ConvertTimeUnits (ulpInstance, &ulTemp1, FILE_LENGTH);

              pParams->dwReturn = ulTemp1;

              }      /* Play Back case */

           else
               if (ulpInstance->ulCreateFlag == PREROLL_STATE)
                   {
                   if ( STRMSTATE == MCI_RECORD )
                      {
                      ulrc = SpiGetTime ( STREAM.hStream,
                                          ( PMMTIME)&(STREAM.mmStreamTime));

                      if (!ulrc)
                         {
                         ulTemp2 = STREAM.mmStreamTime;
                         }

                      ConvertTimeUnits (ulpInstance, &ulTemp1, ulTemp2);

                      /******************************************
                      * the function ConvertTimeUnits also
                      * returns media element length in the
                      * current time units.
                      ******************************************/
                      ConvertTimeUnits (ulpInstance, &ulTemp2, FILE_LENGTH);

                      /******************************************
                      * if the current record position is smaller
                      * tham the file length, then report the
                      * file length.
                      ******************************************/
                      if ( ulTemp1 < ulTemp2 )
                         {
                         ulTemp1 = ulTemp2;
                         }

                      pParams->dwReturn = ulTemp1;
                      }
                   else
                      {
                      if ( ulpInstance->mmioHndlPrvd )
                         {
                         /*****************************************************
                         *  if the user passed in the handle, they may have
                         *  updated the size of the file so update our instance
                         *****************************************************/

                         ulrc = GetAudioHeader( ulpInstance );
                         if ( ulrc )
                           {
                           return MCIERR_DRIVER_INTERNAL;
                           }
                         }

                      /******************************************
                      * the function ConvertTimeUnits also
                      * returns media element length in the
                      * current time units.
                      *******************************************/
                      ConvertTimeUnits (ulpInstance, &ulTemp1, FILE_LENGTH);

                      pParams->dwReturn = ulTemp1;
                      }

                   } /* Stream Created and Recording */

           else
                  pParams->dwReturn = 0;         /* No Element Case */

           ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);

           }
           break;

      case MCI_STATUS_NUMBER_OF_TRACKS:
            return MCIERR_UNSUPPORTED_FLAG;
           break;

      case MCI_STATUS_SPEED_FORMAT:
            return MCIERR_UNSUPPORTED_FLAG;
           break;
      case MCI_STATUS_CURRENT_TRACK:
            return MCIERR_UNSUPPORTED_FLAG;
           break;

      case MCI_STATUS_POSITION_IN_TRACK:
            return MCIERR_UNSUPPORTED_FLAG;
           break;

      case MCI_STATUS_VOLUME:
            ulrc = mciSendCommand ((WORD)ulpInstance->wAmpDeviceID,
                                  (WORD)MCI_STATUS,
                                  (DWORD)MCI_STATUS_ITEM |
                                  MCI_WAIT,
                                  (DWORD)(pParams),
                                  pFuncBlock->usUserParm);
            ulrc = MAKEULONG (ulrc, MCI_COLONIZED2_RETURN);
          break;
      case MCI_AMP_STATUS_BALANCE:
      case MCI_AMP_STATUS_BASS   :
      case MCI_AMP_STATUS_TREBLE :
      case MCI_AMP_STATUS_GAIN   :
            ulrc = mciSendCommand ((WORD)ulpInstance->wAmpDeviceID,
                                  (WORD)MCI_STATUS,
                                  (DWORD)MCI_STATUS_ITEM |
                                  MCI_WAIT,
                                  (DWORD)(pParams),
                                  pFuncBlock->usUserParm);
            ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED );
          break;


      case MCI_WAVE_STATUS_CHANNELS:
            WaveStat.dwItem = MCI_WAVE_STATUS_CHANNELS;
            ulrc = ulpInstance->pfnVSD (&AMPMIX,
                                        MCI_STATUS,
                                        MCI_STATUS_ITEM |
                                        MCI_WAVE_STATUS_CHANNELS,
                                        (LONG) &WaveStat,
                                        0L);
            pParams->dwReturn = WaveStat.dwReturn;
            ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);

          break;

      case MCI_WAVE_STATUS_SAMPLESPERSEC:
            WaveStat.dwItem = MCI_WAVE_STATUS_SAMPLESPERSEC;
            ulrc = ulpInstance->pfnVSD (&AMPMIX,
                                        MCI_STATUS,
                                        MCI_STATUS_ITEM |
                                        MCI_WAVE_STATUS_SAMPLESPERSEC,
                                        (LONG)&WaveStat,
                                        0L);
            pParams->dwReturn = WaveStat.dwReturn;
            ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);

          break;

      case MCI_WAVE_STATUS_AVGBYTESPERSEC:

               pParams->dwReturn = ulpInstance->ulAverageBytesPerSec;
             ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);
          break;

      case MCI_WAVE_STATUS_BITSPERSAMPLE:
            WaveStat.dwItem = MCI_WAVE_STATUS_BITSPERSAMPLE;
            ulrc = ulpInstance->pfnVSD (&AMPMIX,
                                        MCI_STATUS,
                                        MCI_STATUS_ITEM |
                                        MCI_WAVE_STATUS_BITSPERSAMPLE,
                                        (LONG)&WaveStat,
                                        0L);
            pParams->dwReturn = WaveStat.dwReturn;
            ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);
          break;

      case MCI_WAVE_STATUS_LEVEL:
            pParams->dwReturn = 0;
            ulrc = MAKEULONG (ulrc, MCI_INTEGER_RETURNED);
          break;

      case MCI_WAVE_STATUS_FORMATTAG:
            WaveStat.dwItem = MCI_WAVE_STATUS_FORMATTAG;
            ulrc = ulpInstance->pfnVSD (&AMPMIX,
                                        MCI_STATUS,
                                        MCI_STATUS_ITEM |
                                        MCI_WAVE_STATUS_FORMATTAG,
                                        (LONG) &WaveStat,
                                         0L);
            pParams->dwReturn = WaveStat.dwReturn;
            ulrc = MAKEULONG (ulrc, MCI_FORMAT_TAG_RETURN);
          break;

      case MCI_STATUS_MEDIA_PRESENT:
            pParams->dwReturn = MCI_TRUE;
            ulrc = MAKEULONG(ulrc, MCI_TRUE_FALSE_RETURN);
          break;

      case MCI_WAVE_STATUS_BLOCKALIGN:
           pParams->dwReturn = AMPMIX.ulBlockAlignment;
           ulrc = MAKEULONG(ulrc, MCI_INTEGER_RETURNED);
          break;

      case MCI_STATUS_MODE:
           {
              /********************************************
               * Always Return an Integer for this case
              ********************************************/
              ulrc = MAKEULONG (ulrc, MCI_MODE_RETURN);
              if (ulpInstance->usMediaPresent != MCI_TRUE)
                  pParams->dwReturn = MCI_MODE_NOT_READY;
              else
                  {
                     switch (STRMSTATE)
                     {
                      case MCI_PLAY:
                            pParams->dwReturn = MCI_MODE_PLAY;
                           break;

                      case MCI_RECORD:
                            pParams->dwReturn = MCI_MODE_RECORD;
                           break;

                      case MCI_STOP:
                            pParams->dwReturn = MCI_MODE_STOP;
                           break;

                      case MCI_PAUSE:
                            pParams->dwReturn = MCI_MODE_PAUSE;
                           break;

                      case MCI_SEEK:
                            pParams->dwReturn = MCI_MODE_SEEK;
                           break;

                      default:
                           {
                              /********************************
                              * Check if Amp/Mixer is Ready
                              *********************************/
                              WaveStat.dwItem = MCI_STATUS_READY;
                              ulErr = mciSendCommand ((WORD)ulpInstance->wAmpDeviceID,
                                                      (WORD)MCI_STATUS,
                                                      (DWORD)MCI_STATUS_ITEM | MCI_WAIT,
                                                      (DWORD)(&WaveStat),
                                                      pFuncBlock->usUserParm);

                              if (WaveStat.dwReturn == MCI_FALSE)
                                   pParams->dwReturn = MCI_MODE_NOT_READY;
                              else
                                   pParams->dwReturn = MCI_MODE_STOP;
                           } /* Default Case */
                     }    /* of Possible Modes (Switch) */
                  }       /* The device was ready */
           }  /* Status Mode */
          break;

      case MCI_STATUS_TIME_FORMAT:
           {
            switch (ulpInstance->ulTimeUnits)
            {
             case lMMTIME:
                   pParams->dwReturn = MCI_FORMAT_MMTIME;
                  break;

             case lMILLISECONDS:
                   pParams->dwReturn = MCI_FORMAT_MILLISECONDS;
                  break;

             case lBYTES:
                   pParams->dwReturn = MCI_FORMAT_BYTES;
                  break;

             case lSAMPLES:
                   pParams->dwReturn = MCI_FORMAT_SAMPLES;
                  break;
             } /* Switch */

             ulrc = MAKEULONG (ulrc, MCI_TIME_FORMAT_RETURN);

          }
          break;

      case MCI_STATUS_READY:
            if (ulpInstance->usMediaPresent == MCI_TRUE) {
                 /*****************************************
                 * Check if Amp/Mixer is Ready and active
                 *****************************************/
                 ulrc = mciSendCommand ((WORD)ulpInstance->wAmpDeviceID,
                                        (WORD)MCI_STATUS,
                                        (DWORD)MCI_STATUS_ITEM | MCI_WAIT,
                                        (DWORD)(pParams),
                                        pFuncBlock->usUserParm);
            }
            else
                 pParams->dwReturn = MCI_FALSE;

            ulrc = MAKEULONG(ulrc, MCI_TRUE_FALSE_RETURN);
          break;

      default:
             return (MCIERR_INVALID_FLAG);

      }       /* end of switch */
  }   /* Status Item */
  else
        return  MCIERR_MISSING_FLAG;

  return (ULONG) (ulrc);

}        /* end of wavstat */
