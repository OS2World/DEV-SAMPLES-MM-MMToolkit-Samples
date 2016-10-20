/*static char *SCCSID = "@(#)cdaudutl.c	13.7 92/04/23";*/
/****************************************************************************/
/*                                                                          */
/* SOURCE FILE NAME:  CDAUDUTL.C                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD AUDIO MCI DRIVER UTILITIES                         */
/*                                                                          */
/* COPYRIGHT:  (c) IBM Corp. 1991, 1992                                     */
/*                                                                          */
/* FUNCTION:  This file contains the hardware independent code that         */
/*            supplement the PROCESS commands in CDAUDPRO.C for the         */
/*            CD Audio MCI Driver uses.  The entry point to the DLL is      */
/*            in CDAUDIO.C                                                  */
/*                                                                          */
/* NOTES:  The hardware dependent code is found in file IBMCDROM.C.         */
/*                                                                          */
/* OTHER FUNCTIONS:                                                         */
/*       SetAudio      - Set audio information from MCI_SET.                */
/*       SetConnector  - Enable or disable connection.                      */
/*       SetCuePoint   - Enable cue point.                                  */
/*       StatusMCD     - Get status from MCD information.                   */
/*       StatusMCDDef  - Get status from MCD information.                   */
/*       StatusVSD     - Get status from VSD information.                   */
/*       DisableEvents - Disable cuepoints and position advise.             */
/*       GetTimeAddr   - Convert a time format to/from MMTIME               */
/*       GetTimeAddrRC - Colinize return code to time format.               */
/*       GetTrackInfo  - Get Track info for specified track or address.     */
/*       ValAddress    - validate address for play and seek commands.       */
/*       ValState      - validate state, has disc and not suspended.        */
/*       vsdResponse   - process VSD Response.                              */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#define INCL_DOSERRORS
#define INCL_DOSPROCESS
#define INCL_DOSMEMMGR
#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#include <string.h>
#include <os2me.h>
#include <mcd.h>
#include <cdaudio.h>
#include "cdaudibm.h"
#include <ctype.h>
#include <hhpheap.h>

extern PVOID          CDMC_hHeap;

/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  SetAudio                                               */
/*                                                                          */
/* DESCRIPTIVE NAME:  Set Audio                                             */
/*                                                                          */
/* FUNCTION:  Set Audio information when MCI_SET_AUDIO flag is used with    */
/*            MCI_SET command.  This function is called by ProcSet().       */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD *pParam1 -- flag for this message.                            */
/*      MCI_SET_PARMS *dwParam2 -- pointer to local SET structure.          */
/*      ULONG *pulMode  -- mode flags.                                      */
/*      DWORD *pdwLevel -- volume level.                                    */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_FLAGS_NOT_COMPATIBLE -- Flags not compatible.                */
/*      MCIERR_INVALID_AUDIO_FLAG   -- Unknown audio flag specified.        */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/

DWORD SetAudio(PINST pInst, DWORD *pParam1, MCI_SET_PARMS *dwParam2,
               ULONG *pulMode, DWORD *pdwLevel)
{
   DWORD rc = MCIERR_SUCCESS;          // assume the best
   USHORT usFlag = 0;
   ULONG  ulChan = 0;
   WORD left, right;

   if (*pParam1 & MCI_SET_ON && *pParam1 & MCI_SET_OFF)
      rc = MCIERR_FLAGS_NOT_COMPATIBLE;
   else
      switch (dwParam2->dwAudio)
      {
         case MCI_SET_AUDIO_LEFT :
            ulChan = CDMC_LFT_CH;
            break;
         case MCI_SET_AUDIO_RIGHT :
            ulChan = CDMC_RGT_CH;
            break;
         case MCI_SET_AUDIO_ALL :
            ulChan = CDMC_ALL_CH;
            break;
         default : rc = MCIERR_INVALID_AUDIO_FLAG;
      }  /* of switch */


   if (!rc)
   {
      *pParam1 ^= MCI_SET_AUDIO;       // remove non VSD flag
      if (*pParam1 & MCI_OVER)
         *pParam1 ^= MCI_OVER;         // vectored volume not supported

      if (*pParam1 & MCI_SET_ON)
      {
         usFlag = 1;
         *pParam1 ^= MCI_SET_ON;       // remove non VSD flag
         *pulMode |= ulChan;
      }  /* of else if ON */
      else if (*pParam1 & MCI_SET_OFF)
      {
         usFlag = 1;
         *pParam1 ^= MCI_SET_OFF;      // remove non VSD flag
         if (ulChan & CDMC_LFT_CH)
            *pulMode &= CHAN_MODE_SET | CDMC_RGT_CH;
         if (ulChan & CDMC_RGT_CH)
            *pulMode &= CHAN_MODE_SET | CDMC_LFT_CH;
      }  /* of else if OFF */

      if (*pParam1 & MCI_SET_VOLUME)
      {
         usFlag = 1;
         /* get requested volume levels */
         left  = (WORD) dwParam2->dwLevel;
         right = (WORD) dwParam2->dwLevel;

         if (ulChan & CDMC_LFT_CH)
            if (left > 100)
               VOL_LEFT(*pdwLevel) = 100;
            else
               VOL_LEFT(*pdwLevel) = left;
         if (ulChan & CDMC_RGT_CH)
            if (right > 100)
               VOL_RIGHT(*pdwLevel) = 100;
            else
               VOL_RIGHT(*pdwLevel) = right;
      }  /* of setting volume */

      if (usFlag)                // CHANGE, SET VOLUME
      {
         /* Go ahead and prepare for call to VSD */
         if (*pulMode & CDMC_LFT_CH && pInst->ulMode & CDMC_HEADPHONE)
            left = (WORD)((DWORD)VOL_LEFT(*pdwLevel) *
                   pInst->dwMasterVolume / 100);
         else           /* else channel is muted */
            left = 0;
         if (*pulMode & CDMC_RGT_CH && pInst->ulMode & CDMC_HEADPHONE)
            right = (WORD)((DWORD)VOL_RIGHT(*pdwLevel) *
                   pInst->dwMasterVolume / 100);
         else           /* else channel is muted */
            right = 0;
         dwParam2->dwLevel = MAKEULONG(left, right);
         *pParam1 |= MCI_SET_VOLUME;      //make sure VSD flag is set

      }  /* of if change */
      else
         rc = MCIERR_MISSING_FLAG;
   }  /* of if no error with flags */

   return(rc);

}  /* of SetAudio() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  SetConnector                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:  Set Connector                                         */
/*                                                                          */
/* FUNCTION:  Enable or disable connectors from the MCI_CONNECTOR command.  */
/*            This function is called by ProcConnector().                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwParam1 -- flag for this message.                            */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_DEVICE_NOT_READY  -- No Disc is present.                     */
/*      MCIERR_UNSUPPORTED_FLAG     -- cannot process constant.             */
/*      MCIERR_FLAGS_NOT_COMPATIBLE -- Invalid flag combinations.           */
/*      MCIERR_INVALID_FLAG         -- Unknown or unsupported flag.         */
/*      MCIERR_CANNOT_LOAD_DRIVER -- Unable to load VSD.                    */
/*                                                                          */
/* NOTES:   ProcConnector() calls this function only when                   */
/*          MCI_ENABLE_CONNECTOR or MCI_DISABLE_CONNECTOR flags were used.  */
/*                                                                          */
/****************************************************************************/

DWORD SetConnector(PINST pInst, DWORD dwParam1)
{
   DWORD rsp, rc = MCIERR_SUCCESS;          // assume the best
   MCI_STATUS_PARMS recStatus;

   /* get current position -- will be placed in pInst->dwCur_pos */
   /* don't use recStatus.dwReturn, it will be adjusted with     */
   /*   sync offset and current time format setting              */
   recStatus.dwItem = MCI_STATUS_POSITION;
   rsp = ProcStatus(pInst, MCI_STATUS_ITEM, &recStatus);

   if (dwParam1 & MCI_ENABLE_CONNECTOR)          // *** ENABLE DAC ***
   {
      if (pInst->ulMode & CDMC_CAN_DAC)
      {
         /* do it if not already playing internally */
         if (!(pInst->ulMode & CDMC_INTDAC))
            pInst->ulMode = pInst->ulMode & STREAM_MODE_SET | CDMC_INTDAC;

      }  /* of if can enable DAC */
      else
         rc = MCIERR_UNSUPPORTED_FLAG;           //no internal DAC
   }  /* of if enable */
   else                             // *** DISABLE DAC ***
   {
      if (pInst->ulMode & CDMC_INTDAC)
      {
         ProcStop(pInst, MCI_WAIT);
         pInst->ulMode ^= CDMC_INTDAC;
      }   /* of if playing internal DAC */
   }  /* of if disable */

   return(rc);

}  /* of SetConnector() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  SetCuePoint                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  Set Cue Point                                         */
/*                                                                          */
/* FUNCTION:  Set Cue Point from the MCI_SET_CUEPOINT command.              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwParam1 -- flag for this message.                            */
/*      DWORD dwParam2 -- pointer to structure (message dependent).         */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_OUTOFRANGE -- invalid cuepoint position.                     */
/*      MCIERR_DEVICE_NOT_READY -- No Disc is present.                      */
/*      MCIERR_CUEPOINT_LIMIT_REACHED --  no more room to add events.       */
/*      MCIERR_DUPLICATE_CUEPOINT     --  duplicate cuepoint.               */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/

DWORD SetCuePoint(PINST pInst, MCI_CUEPOINT_PARMS *dwParam2)
{
   DWORD rc, dwTime, dwFlags;
   int i, found = -1;
   MCI_CUEPOINT_PARMS recCuePoint;

   /* validate time */
   dwTime = GetTimeAddr(pInst, dwParam2->dwCuepoint, TRUE);
   if (dwTime == (DWORD) -1L)
      rc = MCIERR_OUTOFRANGE;         // invalid track number
   else
   {
      dwTime += pInst->dwOffset;
      if (dwTime < pInst->dwStart_disk ||
          dwTime > pInst->dwEnd_disk)
         rc = MCIERR_OUTOFRANGE;
      else
         rc = MCIERR_SUCCESS;
   }

   /* make sure cuepoint is unique */
   if (!rc)
   {
      for (i=0; i < CDMCD_CUEPOINT_MAX; i++)
      {
         if (pInst->arrCuePoint[i] == (DWORD) -1L)
         {
            if (found < 0)
               found = i;      // save first available entry
         }
         else
            if (pInst->arrCuePoint[i] == dwTime)
            {
               rc = MCIERR_DUPLICATE_CUEPOINT;
               break;               // error end for loop
            }
      }  /* of for loop */

      if (!rc)
         if (found < 0)
            rc = MCIERR_CUEPOINT_LIMIT_REACHED;
   }  /* of if no error, test uniqueness */

   /* init record and relay message */
   if (!rc)
   {
      pInst->arrCuePoint[found] = dwTime;
      /* copy record so as not to change original from application */
      memcpy(&recCuePoint, dwParam2, sizeof(MCI_CUEPOINT_PARMS));
      dwParam2->dwCuepoint = dwTime;
      dwFlags = MCI_SET_CUEPOINT_ON | MCI_WAIT;
      rc = pInst->pMCIDriver(pInst->hHWMCID, MCI_SET_CUEPOINT,
                             (DWORD) &dwFlags, (DWORD) dwParam2, 0);

      if (rc)                            // clear entry
         pInst->arrCuePoint[found] = -1L;

   }  /* of if no error, init record */

   return(rc);

}  /* of SetCuePoint() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  StatusMCD                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Status MCD                                            */
/*                                                                          */
/* FUNCTION:  Query the Status of MCD flags.  This function is called       */
/*            by ProcStatus().                                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst     -- pointer to instance.                             */
/*      DWORD dwParam1  -- flag for this message.                           */
/*      DWORD dwParam2  -- pointer to structure (message dependent).        */
/*      DWORD rsp       -- state response.                                  */
/*      USHORT usIgnore -- ignore invalid media type.                       */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_OUTOFRANGE -- invalid track supplied.                        */
/*      MCIERR_FLAGS_NOT_COMPATIBLE -- Flags not compatible.                */
/*      MCIERR_UNSUPPORTED_FUNCTION -- Unknown flag or value used.          */
/*      MCIERR_DEVICE_NOT_READY   -- device was not ready, no disc.         */
/*      MCIERR_INVALID_MEDIA_TYPE -- No audio tracks were found.            */
/*                                                                          */
/* NOTES: dwParam1 = MCI_STATUS_ITEM or MCI_STATUS_ITEM | MCI_TRACK         */
/*                                   or MCI_STATUS_ITEM | MCI_STATUS_START  */
/*                                                                          */
/****************************************************************************/

DWORD StatusMCD(PINST pInst, DWORD dwParam1, MCI_STATUS_PARMS *dwParam2,
                DWORD rsp, USHORT usIgnore)
{
   DWORD rc;
   DWORD dwAddr, dwTrack, dwFlags;
   MCI_CD_REGTRACK_REC *ptr1, *ptr2;
   MCI_STATUS_PARMS recStatus;
   ULONG ulMode;

   /* Make sure disc is present before you rely on cache */
   if (rsp)
   {
      if (rsp == MCIERR_INSTANCE_INACTIVE)
         if (pInst->usStatus - SUSPEND == REGTRACK)
            rc = MCIERR_INVALID_MEDIA_TYPE;
         else if (pInst->usStatus - SUSPEND <= NODISC)
            rc = MCIERR_DEVICE_NOT_READY;
         else
         {
            rc = MCIERR_SUCCESS;    // process inactive command
            recStatus.dwReturn = pInst->dwCur_pos;
         }
      else
         rc = rsp;
   }  /* of if known error */
   else
   {
      recStatus.dwItem = MCI_STATUS_POSITION;
      dwFlags = MCI_STATUS_ITEM | MCI_WAIT;
      rc = pInst->pMCIDriver(pInst->hHWMCID, MCI_STATUS,
                             (DWORD) &dwFlags, (DWORD) &recStatus, 0);
      if (DWORD_LOWD(rc))
      {
         rc = vsdResponse(pInst, rc);
         if (!rc)                    //Disc Change, get starting address
            recStatus.dwReturn = pInst->dwStart_disk;
      }  /* of if error */

      if (!DWORD_LOWD(rc))
         pInst->dwCur_pos = recStatus.dwReturn;    // valid value, keep it
   }  /* of else no error so check disc */

   if (rc == MCIERR_INVALID_MEDIA_TYPE && usIgnore)
      rc = MCIERR_SUCCESS;

   if (DWORD_LOWD(rc))
      return(rc);

   /*----------- End of checking presents of Disc -----------------*/

   switch (dwParam1)
   {
      case MCI_STATUS_ITEM | MCI_TRACK :
         rc = StatusMCDDef(pInst, dwParam1, dwParam2, recStatus.dwReturn);
         break;
      case MCI_STATUS_ITEM | MCI_STATUS_START :
         /* item must be MCI_STATUS_POSITION from ProcStatus() */
         dwParam2->dwReturn =  GetTimeAddr(pInst, pInst->dwStart_disk, FALSE);
         rc = GetTimeAddrRC(pInst, rc);
         break;
      default :                            // must be MCI_STATUS_ITEM alone
         switch(dwParam2->dwItem)
         {
            case MCI_STATUS_POSITION :
               dwParam2->dwReturn =  GetTimeAddr(pInst,
                         (DWORD)(recStatus.dwReturn - pInst->dwOffset), FALSE);
               rc = GetTimeAddrRC(pInst, rc);
               break;
            case MCI_STATUS_LENGTH :
               dwTrack = pInst->recTrack.dwBufSize /
                         sizeof(MCI_CD_REGTRACK_REC);
               dwAddr = (pInst->recTrack.TrackRecArr + dwTrack-1)->dwEndAddr -
                         pInst->recTrack.TrackRecArr->dwStartAddr;
               /* return MSF format for length if in TMSF format */
               if ((pInst->ulMode & TIME_MODE) == CDMC_TMSF)
               {
                  ulMode = pInst->ulMode;
                  pInst->ulMode = pInst->ulMode & TIME_MODE_SET | CDMC_REDBOOK;
                  dwParam2->dwReturn = GetTimeAddr(pInst, dwAddr, FALSE);
                  rc = GetTimeAddrRC(pInst, rc);
                  pInst->ulMode = ulMode;
               }
               else
               {
                  dwParam2->dwReturn = GetTimeAddr(pInst, dwAddr, FALSE);
                  rc = GetTimeAddrRC(pInst, rc);
               }
               break;
            case MCI_STATUS_NUMBER_OF_TRACKS :
               ptr1 = GetTrackInfo(pInst, 0, pInst->dwStart_disk);
               ptr2 = GetTrackInfo(pInst, 0, pInst->dwEnd_disk - 1);
               if (ptr1 && ptr2)         //if both pointers are valid
                  dwParam2->dwReturn = ptr2->TrackNum - ptr1->TrackNum + 1;
               else
                  dwParam2->dwReturn = 0L;
               DWORD_HIWD(rc) = MCI_INTEGER_RETURNED;
               break;
            default :
               rc = StatusMCDDef(pInst, dwParam1, dwParam2, recStatus.dwReturn);

         }  /* of switch, known items */
   }  /* of switch, flags */

   return(rc);

}  /* of StatusMCD() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  StatusMCDDef                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:  Status MCD Default                                    */
/*                                                                          */
/* FUNCTION:  Query the Status of MCD default flags.  This function is      */
/*            called by ProcStatus().                                       */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwParam1 -- flag for this message.                            */
/*      DWORD dwParam2 -- pointer to structure (message dependent).         */
/*      DWORD dwAddr   -- current address.                                  */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_OUTOFRANGE -- invalid track supplied.                        */
/*      MCIERR_FLAGS_NOT_COMPATIBLE -- Flags not compatible.                */
/*      MCIERR_UNSUPPORTED_FUNCTION -- Unknown flag or value used.          */
/*      MCIERR_DEVICE_NOT_READY   -- device was not ready, no disc.         */
/*      MCIERR_INVALID_MEDIA_TYPE -- No audio tracks were found.            */
/*                                                                          */
/* NOTES: dwParam1 = MCI_STATUS_ITEM or MCI_STATUS_ITEM | MCI_TRACK         */
/*                                   or MCI_STATUS_ITEM | MCI_STATUS_START  */
/*                                                                          */
/****************************************************************************/

DWORD StatusMCDDef(PINST pInst, DWORD dwParam1,
                   MCI_STATUS_PARMS *dwParam2, DWORD dwAddr)
{
   DWORD rc = MCIERR_SUCCESS;
   DWORD dwTrack;
   ULONG ulMode;
   MCI_CD_REGTRACK_REC *ptr1;


   /****************** get track number and info ********************/

   if (dwParam1 & MCI_TRACK)
   {
      dwTrack = dwParam2->dwTrack;
      ptr1 = GetTrackInfo(pInst, (BYTE) dwTrack, 0);
      if (ptr1 == NULL)
         rc = MCIERR_OUTOFRANGE;
   }
   else   /* get current track */
   {
      ptr1 = GetTrackInfo(pInst, 0, dwAddr);   //valid addr from drive
      dwTrack = ptr1->TrackNum;
   }  /* of else get current track */

   if (!DWORD_LOWD(rc))
      switch(dwParam2->dwItem)
      {
         case MCI_STATUS_NUMBER_OF_TRACKS : // with MCI_TRACK, w/o already done
            rc = MCIERR_FLAGS_NOT_COMPATIBLE;
         case MCI_STATUS_POSITION :     // with MCI_TRACK, w/o already done
            dwParam2->dwReturn = GetTimeAddr(pInst,
                 (DWORD)(ptr1->dwStartAddr - pInst->dwOffset), FALSE);
            rc = GetTimeAddrRC(pInst, rc);
            break;
         case MCI_STATUS_POSITION_IN_TRACK :
            if (dwParam1 & MCI_TRACK)
               rc = MCIERR_FLAGS_NOT_COMPATIBLE;
            else
            {
               dwAddr -= ptr1->dwStartAddr + pInst->dwOffset;
               dwParam2->dwReturn = GetTimeAddr(pInst, dwAddr, FALSE);
               rc = GetTimeAddrRC(pInst, rc);
            }
            break;
         case MCI_STATUS_LENGTH :       // with MCI_TRACK, w/o already done
            dwAddr = ptr1->dwEndAddr - ptr1->dwStartAddr;
            /* return MSF format for length if in TMSF format */
            if ((pInst->ulMode & TIME_MODE) == CDMC_TMSF)
            {
               ulMode = pInst->ulMode;
               pInst->ulMode = (pInst->ulMode & TIME_MODE_SET) | CDMC_REDBOOK;
               dwParam2->dwReturn = GetTimeAddr(pInst, dwAddr, FALSE);
               rc = GetTimeAddrRC(pInst, rc);
               pInst->ulMode = ulMode;
            }
            else
            {
               dwParam2->dwReturn = GetTimeAddr(pInst, dwAddr, FALSE);
               rc = GetTimeAddrRC(pInst, rc);
            }
            break;
         case MCI_STATUS_CURRENT_TRACK :
            if (dwParam1 & MCI_TRACK)
               rc = MCIERR_FLAGS_NOT_COMPATIBLE;
            else
            {
               dwParam2->dwReturn = dwTrack;
               DWORD_HIWD(rc) = MCI_INTEGER_RETURNED;
            }
            break;
         case MCI_CD_STATUS_TRACK_TYPE :
            if (!(ptr1->TrackControl & IS_DATA_TRK))
               dwParam2->dwReturn = MCI_CD_TRACK_AUDIO;
            else if (ptr1->TrackControl & IS_OTHER_TRK)
               dwParam2->dwReturn = MCI_CD_TRACK_OTHER;
            else
               dwParam2->dwReturn = MCI_CD_TRACK_DATA;
            DWORD_HIWD(rc) = MCI_TRACK_TYPE_RETURN;
            break;
         case MCI_CD_STATUS_TRACK_COPYPERMITTED :
            if (ptr1->TrackControl & IS_COPYABLE)
               dwParam2->dwReturn = MCI_TRUE;
            else
               dwParam2->dwReturn = MCI_FALSE;
            DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
            break;
         case MCI_CD_STATUS_TRACK_CHANNELS :
            if (!(ptr1->TrackControl & IS_DATA_TRK))      /* Is Audio ? */
               if (ptr1->TrackControl & HAS_4_CHANS)
                  dwParam2->dwReturn = 4L;
               else
                  dwParam2->dwReturn = 2L;
            else                                          /* Data */
               dwParam2->dwReturn = 0L;
            DWORD_HIWD(rc) = MCI_INTEGER_RETURNED;
            break;
         case MCI_CD_STATUS_TRACK_PREEMPHASIS :
            if (!(ptr1->TrackControl & IS_DATA_TRK))      /* Is Audio ? */
               if (ptr1->TrackControl & IS_PRE_EMPH)
                  dwParam2->dwReturn = MCI_TRUE;
               else
                  dwParam2->dwReturn = MCI_FALSE;
            else                                          /* Data */
               dwParam2->dwReturn = MCI_FALSE;
            DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
            break;
         /* no default value, all values were filtered in procStatus() */
      }  /* of switch, track info needed */

   return(rc);

}  /* of StatusMCDDef() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  StatusVSD                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Status VSD                                            */
/*                                                                          */
/* FUNCTION:  Query the Status of VSD only flags.  This function is called  */
/*            by ProcStatus().                                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwParam1 -- flag for this message.                            */
/*      DWORD dwParam2 -- pointer to structure (message dependent).         */
/*      DWORD rc       -- initial state, inactive, not ready, etc.          */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_DEVICE_NOT_READY   -- device was not ready, no disc.         */
/*      MCIERR_INVALID_MEDIA_TYPE -- No audio tracks were found.            */
/*                                                                          */
/* NOTES: dwParam1 = MCI_STATUS_ITEM or MCI_STATUS_ITEM | MCI_TRACK         */
/*                                                                          */
/****************************************************************************/

DWORD StatusVSD(PINST pInst, DWORD dwParam1, MCI_STATUS_PARMS *dwParam2,
                DWORD rc)
{
   ULONG ulVol;
   USHORT usStatus;
   DWORD dwFlags, rsp = 0L;
   MCI_STATUS_PARMS recStatus;

   if (!rc)
   {
      /* Query position to release any Disc Changes in VSD */
      recStatus.dwItem = MCI_STATUS_POSITION;
      dwFlags = MCI_STATUS_ITEM | MCI_WAIT;
      rc = pInst->pMCIDriver(pInst->hHWMCID, MCI_STATUS,
                             (DWORD) &dwFlags, (DWORD) &recStatus, 0);
      if (DWORD_LOWD(rc))
         rc = vsdResponse(pInst, rc);
   }  /* of if no error */

   if (rc == MCIERR_INVALID_MEDIA_TYPE)
   {
      rsp = MCIERR_INVALID_MEDIA_TYPE;
      rc = MCIERR_SUCCESS;
   }

   if (!rc)
   {
      dwParam1 |= MCI_WAIT;
      rc = pInst->pMCIDriver(pInst->hHWMCID, MCI_STATUS, (DWORD) &dwParam1,
                         (DWORD) dwParam2, 0);
      if (DWORD_LOWD(rc))
         rc = vsdResponse(pInst, rc);       //if error, process response
   }  /* of if no error */

   if (!(dwParam1 & MCI_TRACK))          //process known combinations flags
      switch(dwParam2->dwItem)
      {
         case MCI_STATUS_MEDIA_PRESENT :
            switch DWORD_LOWD(rc)
            {
               case MCIERR_SUCCESS :                //vsd already set dwReturn
                  DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
                  if (dwParam2->dwReturn == MCI_FALSE)
                     pInst->usStatus = NODISC;
                  else if (rsp)
                     rc = rsp;                     //report invalid media
                  break;
               case MCIERR_INSTANCE_INACTIVE :
                  rc = MCIERR_SUCCESS;
                  DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
                  usStatus = (USHORT)(pInst->usStatus - SUSPEND);
                  if (usStatus == REGTRACK)
                     rc = MCIERR_INVALID_MEDIA_TYPE;
                  else if (usStatus <= NODISC)
                     dwParam2->dwReturn = MCI_FALSE;
                  else
                     dwParam2->dwReturn = MCI_TRUE;
                  break;
               default :                            //return not present
                  rc = MCIERR_SUCCESS;
                  DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
                  dwParam2->dwReturn = MCI_FALSE;
            }  /* of switch */
            DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
            break;
         case MCI_STATUS_MODE :
            if (DWORD_LOWD(rc))
            {
               dwParam2->dwReturn = MCI_MODE_NOT_READY;
               rc = MCIERR_SUCCESS;
            }
            else
               if (dwParam2->dwReturn == MCI_MODE_STOP)
               {
                  if (pInst->usStatus == PAUSED)
                     dwParam2->dwReturn = MCI_MODE_PAUSE;
               }
               else
                  if (dwParam2->dwReturn == MCI_MODE_NOT_READY)
                     pInst->usStatus = NODISC;

            DWORD_HIWD(rc) = MCI_MODE_RETURN;
            break;
         case MCI_STATUS_READY :
            if (DWORD_LOWD(rc))
            {
               dwParam2->dwReturn = MCI_FALSE;
               rc = MCIERR_SUCCESS;
            }
            else
               if (dwParam2->dwReturn == MCI_FALSE)
                  pInst->usStatus = NODISC;

            DWORD_HIWD(rc) = MCI_TRUE_FALSE_RETURN;
            break;
         case MCI_STATUS_VOLUME :
            if (DWORD_LOWD(rc))          // if error
               rsp = TRUE;
            else
            {
               rsp = FALSE;
               DWORD_HIWD(rc) = MCI_COLONIZED2_RETURN;

               /* adjust device volume to component volume */
               if (pInst->dwMasterVolume && pInst->ulMode & CDMC_HEADPHONE)
               {
                  ulVol = (DWORD)VOL_LEFT(dwParam2->dwReturn) *
                        100 / pInst->dwMasterVolume;
                  if (ulVol > 100)
                     VOL_LEFT(dwParam2->dwReturn) = 100;
                  else
                     VOL_LEFT(dwParam2->dwReturn) = (BYTE)ulVol;

                  ulVol = (DWORD)VOL_RIGHT(dwParam2->dwReturn) *
                           100 / pInst->dwMasterVolume;
                  if (ulVol > 100)
                     VOL_RIGHT(dwParam2->dwReturn) = 100;
                  else
                     VOL_RIGHT(dwParam2->dwReturn) = (BYTE)ulVol;

               }  /* of if non-zero */
               else
                  /* master volume is 0%, return requested volume */
                  rsp = TRUE;

            }  /* of else no error */

            if (rsp)  //unknown volume, ask VSD what component volume should be
            {
               if (rc == MCIERR_INSTANCE_INACTIVE)
                  dwParam2->dwReturn = pInst->recSave.dwLevel;
               else
               {
                  dwFlags = MCI_WAIT;
                  recStatus.dwReturn = pInst->dwLevel;
                  pInst->pMCIDriver(pInst->hHWMCID, MCIDRV_CD_STATUS_CVOL,
                                    (DWORD) &dwFlags, (DWORD) &recStatus, 0);
                  dwParam2->dwReturn = recStatus.dwReturn;
               }
               rc = (DWORD) MAKEULONG(MCIERR_SUCCESS, MCI_COLONIZED2_RETURN);
            }

            break;
      }  /* of switch */

   return(rc);

}  /* of StatusVSD() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  DisableEvents             DEVELOPER:  Garry Lewis      */
/*                                                                          */
/* DESCRIPTIVE NAME:  Disable Events                                        */
/*                                                                          */
/* FUNCTION:  Disable cuepoint and position advise events from the VSD.     */
/*            This is a requirement when the disc is changed.               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*                                                                          */
/* EXIT CODES:  None                                                        */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/

VOID DisableEvents(PINST pInst)
{
   int i;
   DWORD dwParam1;
   MCI_CUEPOINT_PARMS recCuePoint;

   /* disable cuepoints */
   dwParam1 = MCI_SET_CUEPOINT_OFF | MCI_WAIT;
   for (i=0; i < CDMCD_CUEPOINT_MAX; i++)
   {
      if (pInst->arrCuePoint[i] != -1L)
      {
         recCuePoint.dwCuepoint = pInst->arrCuePoint[i];
         pInst->pMCIDriver(pInst->hHWMCID, MCI_SET_CUEPOINT,
                           (DWORD) &dwParam1, (DWORD) &recCuePoint, 0);
         pInst->arrCuePoint[i] = (DWORD) -1L;
      }  /* of if cuepoint is set */
   }  /* of for loop of cuepoints */

   /* disable position advise */
   dwParam1 = MCI_SET_POSITION_ADVISE_OFF | MCI_WAIT;
   pInst->pMCIDriver(pInst->hHWMCID, MCI_SET_POSITION_ADVISE,
                     (DWORD) &dwParam1, 0L, 0);

}  /* of DisableEvents() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  GetTimeAddr                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  Get Time Address                                      */
/*                                                                          */
/* FUNCTION:  Convert to and from MMTime and other time formats.            */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwAddr   -- input address.                                    */
/*      USHORT usTO    -- TRUE if to MMTime, FALSE if from MMTime.          */
/*                                                                          */
/* EXIT CODES:  DWORD  dwReturn -- the converted address.                   */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

DWORD GetTimeAddr(PINST pInst, DWORD dwAddr, USHORT usTO)
{
   DWORD dwReturn;
   MCI_CD_REGTRACK_REC *ptr;

   switch (pInst->ulMode & TIME_MODE)
   {
      case CDMC_MILLSEC :                          // milliseconds
         if (usTO)
            dwReturn = MSECTOMM(dwAddr);
         else
            dwReturn = MSECFROMMM(dwAddr);
         break;
      case CDMC_REDBOOK :                          // Red Book / MSF
         if (usTO)
            dwReturn = REDBOOKTOMM(dwAddr);
         else
            dwReturn = REDBOOKFROMMM(dwAddr);
         break;
      case CDMC_TMSF :                             // Track:Min:Sec:Frame
         if (usTO)
         {
            ptr = GetTrackInfo(pInst, TMSF_TRACK(dwAddr), 0L);
            if (ptr == NULL)
               dwReturn = (DWORD) -1L;            // invalid track number
            else
            {
               dwAddr = (dwAddr >> 8) & 0x00FFFFFF;     //TMSF -> MSF
               dwReturn = ptr->dwStartAddr + REDBOOKTOMM(dwAddr);
            }
         }  /* of TO MMTIME */
         else
         {
            ptr = GetTrackInfo(pInst, 0, dwAddr);
            if (ptr == NULL)
               dwReturn = (DWORD) -1L;            // invalid track number
            else
            {
               dwAddr -= ptr->dwStartAddr;
               dwReturn = (REDBOOKFROMMM(dwAddr) << 8) | (DWORD) ptr->TrackNum;
            }
         }  /* of else FROM MMTIME */
         break;
      case CDMC_MMTIME :                          // Multimedia Time
      default :
         dwReturn = dwAddr;
   }  /* on switch */

   return(dwReturn);

}  /* of GetTimeAddr() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  GetTimeAddrRC                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  Get Time Address Return Code                          */
/*                                                                          */
/* FUNCTION:  Return correct time format in the colinized code,             */
/*            high word of the return code.                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwAddr   -- input address.                                    */
/*      USHORT usTO    -- TRUE if to MMTime, FALSE if from MMTime.          */
/*                                                                          */
/* EXIT CODES:  DWORD  rc  -- the converted return code.                    */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

DWORD GetTimeAddrRC(PINST pInst, DWORD rc)
{
   switch (pInst->ulMode & TIME_MODE)
   {
      case CDMC_REDBOOK :                          // Red Book / MSF
         DWORD_HIWD(rc) = MCI_COLONIZED3_RETURN;
         break;
      case CDMC_TMSF :                             // Track:Min:Sec:Frame
         DWORD_HIWD(rc) = MCI_COLONIZED4_RETURN;
         break;
      case CDMC_MILLSEC :                          // milliseconds
      case CDMC_MMTIME :                           // Multimedia Time
      default :
         DWORD_HIWD(rc) = MCI_INTEGER_RETURNED;
   }  /* on switch */

   return(rc);

}  /* of GetTimeAddrRC() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  GetTrackInfo                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:  Get Track Information                                 */
/*                                                                          */
/* FUNCTION:  Find the Track entry for the given track or absolute address. */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      BYTE  Track    -- specified track number.                           */
/*      DWORD dwAddr   -- specified absolute address, ignored if dwTrack    */
/*                        is non-zero.                                      */
/*                                                                          */
/* EXIT CODES:  MCI_CD_REGTRACK_REC *pRet -- return pointer of track info,  */
/*                                           NULL if error (invalid input). */
/* NOTES:                                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

MCI_CD_REGTRACK_REC * GetTrackInfo(PINST pInst, BYTE Track, DWORD dwAddr)
{
   MCI_CD_REGTRACK_REC *pRet;
   USHORT i, usOffset;                  //Number of Tracks - 1

   if (Track)                                         // if a track was used
      if (pInst->recDisc.LowestTrackNum <= Track &&      // and is valid
          pInst->recDisc.HighestTrackNum >= Track)
         pRet = pInst->recTrack.TrackRecArr +            // get track info
                  (Track - pInst->recDisc.LowestTrackNum);
      else
         pRet = NULL;                                    // else error
   else                                                // otherwise an address
   {
      usOffset = pInst->recDisc.HighestTrackNum - pInst->recDisc.LowestTrackNum;

      /* check if address is in a valid range */
      if (pInst->recTrack.TrackRecArr->dwStartAddr <= dwAddr &&
          (pInst->recTrack.TrackRecArr + usOffset)->dwEndAddr >= dwAddr)
      {
         /* for every track, check address bounds until dwAddr is found */
         /* if not found it must be the last track since the addr is valid */
         for (i= 0; i < usOffset; i++)
            if ((pInst->recTrack.TrackRecArr + i)->dwStartAddr <= dwAddr &&
                (pInst->recTrack.TrackRecArr + i)->dwEndAddr > dwAddr)
               break;
         pRet = pInst->recTrack.TrackRecArr + i;

      }  /* of if a valid address */
      else
         pRet = NULL;                                    // else error
   }  /* of else an address is used */

   return(pRet);

}  /* of GetTrackInfo() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  ValAddress                                             */
/*                                                                          */
/* DESCRIPTIVE NAME:  Validate Address                                      */
/*                                                                          */
/* FUNCTION:  Validate Redbook Audio address in MMTIME format.              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD *pwFrom  -- address to be tested and returned.                */
/*      DWORD *pwTo    -- address to be tested and returned.                */
/*      USHORT isPlay  -- type of check (TRUE = play, FALSE - seek).        */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_OUTOFRANGE -- cannot play in reverse.                        */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

DWORD ValAddress(PINST pInst, DWORD *pdwFrom, DWORD *pdwTo, USHORT isPlay)
{
   DWORD rc;

   rc = MCIERR_SUCCESS;

   /* validate TO */
   if (*pdwTo < pInst->dwStart_disk)
      rc = MCIERR_OUTOFRANGE;
   else
      if (isPlay)
      {
         if (*pdwTo > pInst->dwEnd_disk)
            rc = MCIERR_OUTOFRANGE;
      }
      else
         /* dwEnd_disk is really the starting frame or sector of the next */
         /* track, usually the Lead Out Track.  Valid seeks should be     */
         /* contained within playable tracks.                             */
         if (*pdwTo > pInst->dwEnd_disk - MMT_FRAME)
            rc = MCIERR_OUTOFRANGE;

   /* Validate from address if needed */
   if (!rc && isPlay)
   {
      if (*pdwFrom < pInst->dwStart_disk)
         rc = MCIERR_OUTOFRANGE;
      else if (*pdwTo > pInst->dwEnd_disk)
         rc = MCIERR_OUTOFRANGE;

      /* validate direction */
      /* can only go forward if HW can't play in reverse or if streaming */
      if (*pdwFrom > *pdwTo &&
             (!(pInst->wCaps & CDVSD_CAP_CAN_REVERSE) ||
               (pInst->ulMode & CDMC_STREAM)))
         rc = MCIERR_OUTOFRANGE;

   }  /* of if validating FROM */

   return(rc);

}  /* of ValAddress() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  ValState                                               */
/*                                                                          */
/* DESCRIPTIVE NAME:  Validate State                                        */
/*                                                                          */
/* FUNCTION:  Validate State of an Instance.  Most commands need to have    */
/*            a disc and not be suspended.                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst     -- pointer to instance.                             */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_DEVICE_NOT_READY -- No Disc is present.                      */
/*      MCIERR_INSTANCE_INACTIVE -- Instance is suspended.                  */
/*      MCIERR_INVALID_MEDIA_TYPE -- No audio tracks were found.            */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

DWORD ValState(PINST pInst)
{
   DWORD rc;

   /* if tracks were registered then we should be stopped, */
   /*    unless no audio tracks exists.                    */
   if (pInst->usStatus == REGTRACK)
      rc = MCIERR_INVALID_MEDIA_TYPE;
   else if (pInst->usStatus <= NODISC)
      rc = MCIERR_DEVICE_NOT_READY;
   else if (pInst->usStatus >= SUSPEND)
      rc = MCIERR_INSTANCE_INACTIVE;
   else
      rc = MCIERR_SUCCESS;

   return(rc);

}  /* of ValState() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  vsdResponse                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  Vendor Supplied Driver Response                       */
/*                                                                          */
/* FUNCTION:  Process the response from a VSD.                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst    -- pointer to instance.                              */
/*      DWORD dwResponse -- received response.                              */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_DEVICE_NOT_READY   -- device was not ready, no disc.         */
/*      MCIERR_INVALID_MEDIA_TYPE -- No audio tracks were found.            */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

DWORD vsdResponse(PINST pInst, DWORD dwResponse)
{
   DWORD rc;

   switch (DWORD_LOWD(dwResponse))
   {
      case MCIERR_DEVICE_NOT_READY :
         pInst->usStatus = NODISC;
         rc = dwResponse;
         break;
      case MCIERR_MEDIA_CHANGED :
         rc = ReRegister(pInst);
         break;
      default :
         rc = dwResponse;
   }   /* of switch */

   return(rc);

}  /* of vsdResponse() */


