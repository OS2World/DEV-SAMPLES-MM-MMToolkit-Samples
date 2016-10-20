/********************* START OF SPECIFICATulIONS *********************
*
* SUBROUTINE NAME: MCISet
*
* DESCRIPTIVE NAME: Audio MCD Set Routine
*
*              Copyright (c) IBM Corporation  1991, 1993
*                        All Rights Reserved
*
* FUNCTION: Set various audio attributes.
*
* NOTES:
*
* ENTRY POINTS:
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
*                        SetWaveDeviceDefaults().
*                        CheckMem ().
*
* EXTERNAL REFERENCES:
*                        SpiGetTime       ()        - MME API
*                        mciSendCommand   ()        - MME API
*
*********************** END OF SPECIFICATIONS **********************/
#define INCL_BASE                       // Base Dos APIs.
#define INCL_ERRORS                     // All the errors.

#include <os2.h>                        // OS2 includes.
#include <string.h>                     // String Functions
#include <math.h>                       // Math Functions
#include <os2medef.h>                   // MME includes files.
#include <ssm.h>                        // SSM spi includes.
#include <meerror.h>                    // MM Error Messages.
#include <mmioos2.h>                    // MMIO Include.
#include <mcios2.h>                     // MM System Include.
#include <audio.h>                      // Audio DD Defines
#include <mmdrvos2.h>                   // Mci Driver include.
#include <mcd.h>                        // AUDIO IF DriverInterface.
#include <hhpheap.h>                    // Heap Manager Definitions.
#include <qos.h>
#include <audiomcd.h>                   // Component Definitions.
#include <admcfunc.h>                   // Function Prototypes

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
  ULONG                ulrc;           // Error Value
  ULONG                ulParam1;       // Msg Flags

  INSTANCE             *ulpInstance;   // Local Instance
  ULONG                ulSetFlags;     // Mask For Incoming MCI Flags
  MCI_AMP_SET_PARMS    AmpSetParms;    // Volume Cmnds
  PMCI_WAVE_SET_PARMS  pSetParms;      // Ptr to set Parms
  MCI_WAVE_SET_PARMS   AmpSet;         // Ptr to set Parms



  ulSetFlags = ulParam1 = pFuncBlock->ulParam1;

  pSetParms = ( PMCI_WAVE_SET_PARMS ) pFuncBlock->ulParam2;
  ulpInstance= (INSTANCE *) pFuncBlock->ulpInstance;

  /**************************************
  * Check For Validity of Flags
  * For example, it is impossible to set
  * something on AND off.
  **************************************/

  if (ulParam1 & MCI_SET_ON && ulParam1 & MCI_SET_OFF)
      return MCIERR_FLAGS_NOT_COMPATIBLE;


  /*************************************
  * The current wave driver does not
  * support the following flags
  *************************************/

  if ( ulParam1 & MCI_SET_DOOR_OPEN     ||
       ulParam1 & MCI_SET_DOOR_CLOSED   ||
       ulParam1 & MCI_SET_DOOR_LOCK     ||
       ulParam1 & MCI_SET_DOOR_UNLOCK   ||
       ulParam1 & MCI_SET_VIDEO         ||
       ulParam1 & MCI_SET_ITEM          ||
       ulParam1 &  MCI_WAVE_SET_BLOCKALIGN )

     {
     return (MCIERR_UNSUPPORTED_FLAG);
     }

  /***********************************************
  * The caller cannot use the wave driver to set
  * audio operations.  These must be done with the
  * amp/mixer.  The amp/mixer connected to the
  * wave driver can be accessed via mci_connection
  *************************************************/


  if ( ( ulParam1 & MCI_SET_AUDIO ) &&
       (( ulParam1 & MCI_AMP_SET_BALANCE ) ||
        ( ulParam1 & MCI_AMP_SET_TREBLE  ) ||
        ( ulParam1 & MCI_AMP_SET_BASS    ) ||
        ( ulParam1 & MCI_AMP_SET_GAIN    ) ||
        ( ulParam1 & MCI_AMP_SET_PITCH))  )
     {
     return MCIERR_UNSUPPORTED_FLAG;
     }



  if (ulParam1 & MCI_SET_TIME_FORMAT && ulParam1 & MCI_OVER )
      return ( MCIERR_FLAGS_NOT_COMPATIBLE );

  if ((ulParam1 & MCI_SET_VOLUME) && !(ulParam1 & MCI_SET_AUDIO))
     {
     return MCIERR_MISSING_FLAG;
     }


  /*********************************
  * The caller is required to send in
  * a valid pointer to mci_set_parms
  ***********************************/

  ulrc = CheckMem ((PVOID) pSetParms,
                   sizeof (MCI_WAVE_SET_PARMS),
                   PAG_READ);

  if (ulrc != MCIERR_SUCCESS)
      return ( MCIERR_MISSING_PARAMETER );

  ulSetFlags &= ~(MCI_WAIT + MCI_NOTIFY );

  if (ulSetFlags == 0)
      return MCIERR_MISSING_PARAMETER;


  /***************************************
  * Mask defining Known MCI Set Flags
  ****************************************/

  ulSetFlags &= ~( MCI_SET_AUDIO + MCI_SET_TIME_FORMAT + MCI_SET_VOLUME +
                   MCI_SET_ON    + MCI_SET_OFF         + MCI_WAVE_SET_CHANNELS +
                   MCI_WAVE_SET_BITSPERSAMPLE          + MCI_WAVE_SET_SAMPLESPERSEC +
                   MCI_WAVE_SET_FORMATTAG              + MCI_WAVE_SET_BLOCKALIGN +
                   MCI_WAVE_SET_AVGBYTESPERSEC         + MCI_SET_DOOR_OPEN +
                   MCI_SET_DOOR_CLOSED                 + MCI_SET_DOOR_LOCK +
                   MCI_SET_DOOR_UNLOCK + MCI_SET_VIDEO + MCI_OVER );


  /*******************************************
  * Return invalid if any other bits are set
  *******************************************/

  if (ulSetFlags > 0)
      return MCIERR_INVALID_FLAG;



  /*******************************************
  * We do allow certain audio sets to pass
  * on to the amp/mixer that we are connected
  * to (e.g. volume).  Thus crude audio control
  * is possible via the audio mcd, however, more
  * sophisticated stuff should be sent directly
  * to the amp.
  * Prepare and send the audio command to the
  * amp via the mcisendcommand.
  *******************************************/

  if (ulParam1 & MCI_SET_AUDIO)
     {
     /****************************
     * Copy The Info to Amp Set
     *****************************/

     AmpSetParms.ulLevel = pSetParms->ulLevel;
     AmpSetParms.ulAudio = pSetParms->ulAudio;
     AmpSetParms.ulOver = pSetParms->ulOver;

     /**************************************
         * Send the Request To the Amp/Mixer
     ***************************************/

     ulrc = mciSendCommand ( ulpInstance->usAmpDeviceID,
                             MCI_SET,
                             ulParam1&~(MCI_NOTIFY) |MCI_WAIT,
                             (PVOID) &AmpSetParms,
                             pFuncBlock->usUserParm);

     if ( ulrc )
        {
        return ( ulrc );
        }

     } /* of Audio Flag */



  /******************************************
  * The audio driver currently supports
  * only mmtime, milliseconds, sampling rate
  * and bytes as the time format, other
  * requests will receive invalid time format
  *******************************************/

  if (ulParam1 & MCI_SET_TIME_FORMAT)
      {
      switch (pSetParms->ulTimeFormat)
         {
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

         default: return MCIERR_INVALID_TIME_FORMAT_FLAG;

         } /* Switch time format */

      } /* Time Format flag was passed in */

  /******************************
  * Check for wave specific flags
  *******************************/

  ulSetFlags = ulParam1 & (  MCI_WAVE_SET_CHANNELS
                           + MCI_WAVE_SET_BITSPERSAMPLE
                           + MCI_WAVE_SET_SAMPLESPERSEC
                           + MCI_WAVE_SET_FORMATTAG
                           + MCI_WAVE_SET_AVGBYTESPERSEC );


  /*************************************************************
  ** If any of the wave set flags are greater than 0, then it is
  ** possible we may have to destroy the stream to inform the
  ** stream handlers of the change in the data rates.  This is
  ** necessary since there is no method of to communicate the
  ** date rate other than at stream creation--thus these set
  ** operations can be rather expensive.
  *************************************************************/

  if ( ulSetFlags > 0 )
    {

    /*******************************************************
    ** If the stream has been created, then we must destroy,
    ** perform the set and set a flag to indicate that the
    ** stream must be created
    *******************************************************/

    if ( ulpInstance->ulCreateFlag == PREROLL_STATE )
       {
       /* if we are actually streaming, then we cannot perform the set */

       if ( STRMSTATE == MCI_PLAY   ||
            STRMSTATE == MCI_RECORD ||
            STRMSTATE == MCI_PAUSE )

          {
          return ( MCIERR_INVALID_MODE );
          }
       else
          {

          if ( !ulpInstance->ulOldStreamPos )
            {
            ulrc = SpiGetTime( STREAM.hStream,
                               ( PMMTIME ) &ulpInstance->ulOldStreamPos );

            /************************************************
            * if an error occurred, then don't remember our
            * position in the stream
            ************************************************/
            if ( ulrc )
              {
              ulpInstance->ulOldStreamPos = 0;
              }
            }
          /***********************************
          ** set the stream into create state
          ** following commands will recreate
          ***********************************/

          ulpInstance->ulCreateFlag = CREATE_STATE;
          }

       } /* If a stream has already been created */

    /* Set up a structure to call the VSD with */

    memcpy (&AmpSet, pSetParms, sizeof(MCI_WAVE_SET_PARMS));


    ulSetFlags = 0;

    if (ulParam1 &  MCI_WAVE_SET_CHANNELS )
       {
       ulSetFlags = MCI_WAVE_SET_CHANNELS;
       }

    if (ulParam1 &  MCI_WAVE_SET_SAMPLESPERSEC)
      {
      ulSetFlags |= MCI_WAVE_SET_SAMPLESPERSEC;
      }



    if (ulParam1 & MCI_WAVE_SET_BITSPERSAMPLE)
       {
       ulSetFlags |= MCI_WAVE_SET_BITSPERSAMPLE;
       }

    if (ulParam1 & MCI_WAVE_SET_FORMATTAG)
       {
       ulSetFlags |= MCI_WAVE_SET_FORMATTAG;
       }

    ulrc = SetAudioDevice( ulpInstance,
                           &AmpSet,
                           ulSetFlags );

    if ( ulrc )
       {
       return ( ulrc );
       }

   if (ulParam1 &  MCI_WAVE_SET_AVGBYTESPERSEC)
     {
     if ( pSetParms->ulAvgBytesPerSec < ( ULONG ) ( AMPMIX.lSRate * ( AMPMIX.lBitsPerSRate / 8 ) * AMPMIX.sChannels ) )
        {
        return ( MCIERR_OUTOFRANGE );
        }
       else
        {
        ulpInstance->ulAverageBytesPerSec = pSetParms->ulAvgBytesPerSec;
        }
     } /* if average bytes per sec are passed in */

   else
     {
     ulpInstance->ulAverageBytesPerSec = AMPMIX.lSRate * ( AMPMIX.lBitsPerSRate / 8 ) * AMPMIX.sChannels;
     }

    STRMSTATE = STREAM_SET_STATE;

    } /* if a wave set is requested */


  /******************************************************************
  * If the caller has opened with an MCI_OPEN_MMIO or if a record
  * has been done, the file header must be updated.  In the case of
  * open mmio, applications such as the wave header are dependent
  * on the information in the file header being valid at all times.
  *******************************************************************/
  if ( ulpInstance->fOpenMMIO )
/*       ( AMPMIX.ulOperation == OPERATION_RECORD && STRMSTATE != MCI_RECORD ) ) */
       {
       /**************************************
       * If the card is in record mode an a
       * file has been loaded, then we can do
       * the set header.
       ***************************************/

       ulrc = SetAudioHeader (ulpInstance);
       } /* Not Recording */




  return (ulrc);

} /* MCISet */


