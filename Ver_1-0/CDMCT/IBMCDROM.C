/*static char *SCCSID = "@(#)ibmcdrom.c	13.8 92/04/23";*/
/****************************************************************************/
/*                                                                          */
/* SOURCE FILE NAME:  IBMCDROM.C                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  IBM CD-ROMs                                           */
/*                                                                          */
/* COPYRIGHT:  (c) IBM Corp. 1990 - 1992                                    */
/*                                                                          */
/* FUNCTION:  This file contains the device dependent code for the CD Audio */
/*            Vendor Specific Driver (VSD).  This file is called by         */
/*            CDAUDIO.DLL  -- the generic CD Audio MCI Driver.              */
/*                                                                          */
/* NOTES:  The hardware independent code is found in file CDAUDIO.C.  As    */
/*         more and more CD ROM drives are supported, hardware dependent    */
/*         files can be made into DLLs and register be registered in the    */
/*         MMPM2.INI file upon installation to be recognized.               */
/*                                                                          */
/* ENTRY POINTS:                                                            */
/*       vsdDriverEntry     - entry point to the VSD                        */
/*                                                                          */
/*                                                                          */
/* OTHER FUNCTIONS:                                                         */
/*       process_msg        - Process the requested command message.        */
/*       CDAudErrRecov      - Error recovery routine.                       */
/*       CDAudClose         - Close an instance.                            */
/*       CDAudInfo          - Returns information about the component.      */
/*       CDAudOpen          - Open an instance.                             */
/*       CDAudRegDisc       - Register a disc for the multimedia component. */
/*       CDAudRegDrive      - Register a drive for the multimedia component.*/
/*       CDAudSet           - Set various attributes of the device.         */
/*       CDAudSetVerify     - Tests flags for the set command.              */
/*       CDAudStatus        - Returns the requested attribute.              */
/*       CDAudStatCVol      - Returns mapped volume levels for volume given.*/
/*       CD01_Cue           - Preroll a drive.                              */
/*       CD01_CuePoint      - Set up a cue point.                           */
/*       CD01_GetCaps       - Get device capabilities.                      */
/*       CD01_GetDiscInfo   - Get status info of the disc.                  */
/*       CD01_GetID         - Get the CD ID from the disc.                  */
/*       CD01_GetPosition   - Get the position of the head.                 */
/*       CD01_GetState      - Get the state of the device.                  */
/*       CD01_GetTOC        - Returns table of contents (MMTOC form) of CD. */
/*       CD01_GetVolume     - Get the volume settings of the drive.         */
/*       CD01_LockDoor      - Lock/Unlock the drive door.                   */
/*       CD01_Open          - Open specified device/drive.                  */
/*       CD01_Play          - Initiate a play operation.                    */
/*       CD01_PlayCont      - Continue a play operation.                    */
/*       CD01_PosAdvise     - Set up a position advise command.             */
/*       CD01_RegTracks     - Register tracks on the disc.                  */
/*       CD01_Restore       - Restore the saved instance.                   */
/*       CD01_Resume        - Unpause a CD Play operation.                  */
/*       CD01_Save          - Save the current instance.                    */
/*       CD01_Seek          - Seek to a particular redbook address.         */
/*       CD01_SetVolume     - Set the volume of the drive.                  */
/*       CD01_StartPlay     - Start the play operation.                     */
/*       CD01_Stop          - Stop a CD Play operation.                     */
/*       CD01_Sync          - Sync to MDM request.                          */
/*       CD01_Timer         - Timer routine for play operation.             */
/*       CD01_TimerNotify   - Timer routine to setup and notify events.     */
/*       CallIOCtl          - Call the hardware via IOCTLs.                 */
/*                                                                          */
/*       NOTE:  CD01_... refers to commands that are compatible with the    */
/*              IBM 3510 CD-ROM drive.  CD02_... may refers to commands     */
/*              that are compatible with the CD02 drives, which are         */
/*              not compatible with the CD01_... commands.  This way        */
/*              different hardware can share the same VSD.                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#define INCL_DOSERRORS
#define INCL_DOSDEVICES
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <os2me.h>
#include <cdaudio.h>
#include "ibmcdrom.h"
#include <hhpheap.h>

extern PVOID          CDMC_hHeap;

/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  vsdDriverEntry                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  Hardware specific code entry point.                   */
/*                                                                          */
/* FUNCTION:  Receive command message from the generic MCI Driver for CD    */
/*            Audio (CDAUDIO.DLL).                                          */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      LPTR  lpInstance -- Pointer to device handle.                       */
/*      WORD  wMessage   -- Command message.                                */
/*      DWORD dwParam1   -- Pointer to flag for this message.               */
/*      DWORD dwParam2   -- Pointer to data record structure.               */
/*      WORD  wUserParm  -- User Parameter.                                 */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_INI_FILE   -- corrupted INI file, drive is not CD-ROM drive. */
/*      MCIERR_DEVICE_NOT_READY      -- device was closed from an error.    */
/*      MCIERR_MEDIA_CHANGED         -- device was reopened, waiting for    */
/*                                      MCIDRV_REGISTER message.            */
/*      MCIERR_FLAGS_NOT_COMPATIBLE  -- missing or multiple flags.          */
/*      MCIERR_UNRECOGNIZED_COMMAND  -- unknown command.                    */
/*      MCIERR_UNSUPPORTED_FUNCTION  -- unsupported command.                */
/*      MCIERR_UNSUPPORTED_FLAG      -- unsupported flag.                   */
/*      MCIERR_INVALID_FLAG          -- flag not supported by this VSD.     */
/*      MCIERR_OUT_OF_MEMORY         -- couldn't allocate instance.         */
/*      MCIERR_INVALID_ITEM_FLAG     -- Unknown item specified.             */
/*      MCIERR_INVALID_MEDIA_TYPE    -- No audio tracks were found.         */
/*      MCIERR_CUEPOINT_LIMIT_REACHED -- no more room to add events.        */
/*      MCIERR_INVALID_CUEPOINT       -- unable to locate event.            */
/*      MCIERR_DEVICE_LOCKED -- CD-ROM drive, previously opened exclusively.*/
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
DWORD APIENTRY vsdDriverEntry(LPTR lpInstance, WORD wMessage, DWORD dwParam1,
                              DWORD dwParam2, WORD wUserParm)
{
   DWORD  rc, dwP1Temp = MCI_WAIT;
   USHORT try = 1;

   if (dwParam1 == 0L)
      dwParam1 = (DWORD) &dwP1Temp;

   /* check to see if the drive is open, unless it is an Open message */
   if (wMessage == MCI_OPEN)
      rc = CDAudOpen(*(DWORD *)dwParam1, (MMDRV_OPEN_PARMS *)dwParam2);
   else
   {
      /* if the device is closed try reopening it unless you are closing */
      if (((PINST) lpInstance)->hDrive == 0 && wMessage != MCI_CLOSE)
      {
         rc = CD01_Open((PINST) lpInstance);
         /* Clear commands not needing an open hardware device */
         if (rc == MCIERR_DEVICE_NOT_READY)
            if ((wMessage == MCI_GETDEVCAPS) ||
                (wMessage == MCI_INFO) ||
                (wMessage == MCIDRV_REGISTER_DRIVE) ||
                (wMessage == MCI_SET_CUEPOINT) ||
                (wMessage == MCI_SET_POSITION_ADVISE) ||
                (wMessage == MCIDRV_CD_STATUS_CVOL) ||
                (wMessage == MCIDRV_SYNC &&
                   !(*(DWORD *)dwParam1 & MCIDRV_SYNC_REC_PULSE)))
               rc = MCIERR_SUCCESS;

      }  /* of if drive needs to be open */
      else      /* drive was opened */
         rc = MCIERR_SUCCESS;

      if (!rc)
         do
         {
            /* process message */
            rc = process_msg((PINST) lpInstance, wMessage,
                              (DWORD *)dwParam1, dwParam2, wUserParm);

            if (rc == MCIERR_DEVICE_NOT_READY ||       /* ERROR RECOVERY */
                rc == MCIERR_MEDIA_CHANGED)
            {
               if (((PINST)lpInstance)->Drive == '0')     /* drive is closed */
               {                                   /* don't reissue commands */
                  rc = MCIERR_SUCCESS;
                  break;
               }
               else
                  if (try == 2)
                     break;                         /* quit after 2 tries. */
                  else
                  {
                     rc = CDAudErrRecov((PINST) lpInstance);
                     if (rc)                   /* error is still there, exit */
                        break;
                     else
                        try++;
                  }  /* of else only tried the command once (try == 1) */

            }  /* of if the drive was not ready */
            else
               break;                          /* clear flag to exit */

         } while (try);  /* end of do loop and if no open error */

   } /* of else command was not MCI_OPEN */

   return(rc);

}  /* of vsdDriverEntry() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  process_msg                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  Process message command.                              */
/*                                                                          */
/* FUNCTION:  Process the command message received by vsdDriverEntry().     */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Pointer to instance structure.                  */
/*      WORD  wMessage   -- Command message.                                */
/*      DWORD *pParam1   -- Pointer to flag for this message.               */
/*      DWORD dwParam2   -- Pointer to data record structure.               */
/*      WORD  wUserParm  -- User Parameter.                                 */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS    -- action completed without error.                */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_MEDIA_CHANGED         -- device was reopened, waiting for    */
/*                                      MCIDRV_REGISTER message.            */
/*      MCIERR_FLAGS_NOT_COMPATIBLE  -- missing or multiple flags.          */
/*      MCIERR_UNRECOGNIZED_COMMAND  -- unknown command.                    */
/*      MCIERR_UNSUPPORTED_FUNCTION  -- unsupported command.                */
/*      MCIERR_UNSUPPORTED_FLAG      -- unsupported flag.                   */
/*      MCIERR_INVALID_BUFFER        -- Buffer size was too small.          */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*      MCIERR_INVALID_MEDIA_TYPE    -- No audio tracks were found.         */
/*      ERROR_TOO_MANY_EVENTS --  no more room to add events.               */
/*      ERROR_INVALID_EVENT   --  unable to locate event.                   */
/*      ERROR_INVALID_MMTIME  --  duplicate cuepoint.                       */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD process_msg(PINST pInst, WORD wMessage,
                         DWORD *pParam1, DWORD dwParam2, WORD wUserParm)
{
   DWORD rc;

   DosRequestMutexSem(pInst->hInstSem, WAIT_FOREVER);
   if (wMessage != MCI_PLAY)
      DosReleaseMutexSem(pInst->hInstSem);       // No protection needed

   /* process message */
   switch(wMessage)
   {
      case MCI_CLOSE :
         rc = CDAudClose(pInst, *pParam1);
         break;
      case MCI_CUE :                           /* Pre-roll */
         rc = CD01_Cue(pInst);
         break;
      case MCI_GETDEVCAPS :                    /* Get Device Capabilities */
         rc = CD01_GetCaps(*pParam1, (MCI_GETDEVCAPS_PARMS *)dwParam2);
         break;
      case MCI_GETTOC :                        /* Get Table of Contents */
         if (*pParam1 & WAIT_NOTIFY_MASK)
            rc = MCIERR_INVALID_FLAG;
         else
            rc = CD01_GetTOC(pInst, (MCI_TOC_PARMS *)dwParam2);
         break;
      case MCI_INFO :
         rc = CDAudInfo(pInst, *pParam1, (MCI_INFO_PARMS *)dwParam2);
         break;
      /* case MCI_OPEN :          open was already done in vsdDriverEntry() */

      case MCI_PAUSE :
         rc = CD01_Stop(pInst, TIMER_PLAY_SUSPEND);
         break;
      case MCI_PLAY :
         rc = CD01_Play(pInst, pParam1,
                        ((MCI_PLAY_PARMS *)dwParam2)->dwFrom,
                        ((MCI_PLAY_PARMS *)dwParam2)->dwTo, wUserParm,
                        (HWND) ((MCI_PLAY_PARMS *)dwParam2)->dwCallback);
         break;
      case MCIDRV_REGISTER_DISC :                  /* Register Disc */
         rc = CDAudRegDisc(pInst, REG_BOTH, (MCI_CD_REGDISC_PARMS *)dwParam2);
         break;
      case MCIDRV_REGISTER_DRIVE :                 /* Register Drive */
         rc = CDAudRegDrive(pInst, (MCI_CD_REGDRIVE_PARMS *)dwParam2);
         break;
      case MCIDRV_REGISTER_TRACKS :                /* Register Tracks */
         rc = CD01_RegTracks(pInst, (MCI_CD_REGTRACKS_PARMS *)dwParam2);
         break;
      case MCIDRV_RESTORE :
         rc = CD01_Restore(pInst, (MCIDRV_CD_SAVE_PARMS *)dwParam2);
         break;
      case MCI_RESUME :                            /* Unpause */
         rc = CD01_Resume(pInst);
         break;
      case MCIDRV_SAVE :
         rc = CD01_Save(pInst, (MCIDRV_CD_SAVE_PARMS *)dwParam2);
         break;
      case MCI_SEEK :
         rc = CD01_Seek(pInst, ((MCI_SEEK_PARMS *)dwParam2)->dwTo);
         break;
      case MCI_SET :
         rc = CDAudSet(pInst, pParam1, (MCI_SET_PARMS *)dwParam2);
         break;
      case MCI_SET_CUEPOINT :
         rc = CD01_CuePoint(pInst, *pParam1, (MCI_CUEPOINT_PARMS *)dwParam2);
         break;
      case MCI_SET_POSITION_ADVISE :
         rc = CD01_PosAdvise(pInst, *pParam1, (MCI_POSITION_PARMS *)dwParam2);
         break;
      case MCIDRV_CD_SET_VERIFY :
         rc = CDAudSetVerify(*pParam1);
         break;
      case MCI_STATUS :
         rc = CDAudStatus(pInst, *pParam1, (MCI_STATUS_PARMS *)dwParam2);
         break;
      case MCIDRV_CD_STATUS_CVOL :
         rc = CDAudStatCVol(&((MCI_STATUS_PARMS *)dwParam2)->dwReturn);
         break;
      case MCI_STOP :
         rc = CD01_Stop(pInst, TIMER_EXIT_ABORTED);
         break;
      case MCIDRV_SYNC :
         rc = CD01_Sync(pInst, *pParam1, (MCIDRV_SYNC_PARMS *)dwParam2);
         break;

      /* List unsupported functions */

      case MCI_ACQUIREDEVICE :  case MCI_CONNECTION :     case MCI_CONNECTOR :
      case MCI_CONNECTORINFO :  case MCI_DEVICESETTINGS :
      case MCI_DEFAULT_CONNECTION:                        case MCI_ESCAPE :
      case MCI_LOAD:            case MCI_MASTERAUDIO :    case MCI_RECORD :
      case MCI_RELEASEDEVICE :  case MCI_SAVE :           case MCI_SPIN :
      case MCI_STEP :           case MCI_SYSINFO :        case MCI_UPDATE :
         rc = MCIERR_UNSUPPORTED_FUNCTION;
         break;
      default : rc = MCIERR_UNRECOGNIZED_COMMAND;

   }  /* of switch */

   return(rc);

}  /* of process_msg() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudErrRecov                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Information.                                 */
/*                                                                          */
/* FUNCTION:  Returns string information from a device driver.              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_INI_FILE  -- corrupted INI file, drive is not CD-ROM drive.  */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_MEDIA_CHANGED         -- device was reopened.                */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudErrRecov(PINST pInst)
{
   DWORD rc;

   /* Close the device */
   DosClose(pInst->hDrive);
   pInst->hDrive = 0;                    /* mark the instance structure */

   /* Try to reopen the device */
   rc = CD01_Open(pInst);

   return(rc);

}  /* of CDAudErrRecov() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudClose                                             */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Close                                        */
/*                                                                          */
/* FUNCTION:  Close a device dependent instance.                            */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      MMDRV_OPEN_PARMS *pParam2 -- Pointer to data record structure.      */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_INVALID_FLAG -- flag not supported by this VSD.              */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudClose(PINST pInst, DWORD dwParam1)
{
   DWORD rc;

   if (dwParam1 & WAIT_NOTIFY_MASK)
      rc = MCIERR_INVALID_FLAG;
   else
   {
      pInst->Drive = '0';              /* destroy drive to prevent reopening */

      /* stop play & timer if one is going */
      switch(pInst->usPlayFlag)
      {
         case TIMER_PLAY_SUSPEND :                 //suspended, stop only timer
         case TIMER_PLAY_SUSP_2  :
            if (DosWaitThread(&pInst->ulPlayTID, 1L) ==
                                         ERROR_THREAD_NOT_TERMINATED)
            {
               pInst->usPlayFlag = TIMER_EXIT_ABORTED;
               DosPostEventSem(pInst->hTimeLoopSem);  //continue timer loop
            }
            else
               pInst->usPlayFlag = TIMER_AVAIL;
            break;
         case TIMER_PLAYING :                      //playing, stop device
            if (DosWaitThread(&pInst->ulPlayTID, 1L) !=
                                         ERROR_THREAD_NOT_TERMINATED)
               pInst->usPlayFlag = TIMER_AVAIL;     //play thread doesn't exist
            CD01_Stop(pInst, TIMER_EXIT_ABORTED);
            break;
      }  /* of switch() */

      /* wait for commands to terminate */
      while (pInst->usPlayFlag != TIMER_AVAIL)
         DosSleep(HALF_TIME_MIN);

      DosClose(pInst->hDrive);
      DosCloseMutexSem(pInst->hIOSem);
      DosCloseMutexSem(pInst->hInstSem);
      DosCloseEventSem(pInst->hTimeLoopSem);
      DosCloseEventSem(pInst->hReturnSem);
      HhpFreeMem(CDMC_hHeap, pInst);
      rc = MCIERR_SUCCESS;                  /* assume all will work right */
   }  /* of else flags were okay */

   return(rc);

}  /* of CDAudClose() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudInfo                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Information.                                 */
/*                                                                          */
/* FUNCTION:  Returns string information from a device driver.              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD dwParam1   -- Flag for this message.                          */
/*      MCI_INFO_PARMS *pParam2 -- Pointer to data record structure.        */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_FLAGS_NOT_COMPATIBLE  -- Mis-match in flags.                 */
/*      MCIERR_INVALID_BUFFER        -- Buffer too small.                   */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudInfo(PINST pInst, DWORD dwParam1, MCI_INFO_PARMS *pParam2)
{
   DWORD rc, dwLen;
   CHAR szInfo[INFO_SIZE];
   USHORT i;

   dwParam1 &= WAIT_NOTIFY_MASK;

   if (dwParam1 == MCI_INFO_PRODUCT)
   {
      if (pParam2->dwRetSize >= INFO_SIZE)
      {
         dwLen = INFO_SIZE;
         rc = MCIERR_SUCCESS;
      }
      else
      {
         dwLen = pParam2->dwRetSize;
         rc = MCIERR_INVALID_BUFFER;
      }

      /* fill in product information */

      if (pInst->usHWType / HW_TYPE < HW_TYPE)
         strcpy(szInfo, "IBMCD0");
      else
         strcpy(szInfo, "IBMCD");

      /* complete string with ID number */
      i = (USHORT) strlen(szInfo);
      _itoa((int)pInst->usHWType, (PSZ)&szInfo[i], 10);

      if (dwLen)   //if 0, it may be an invalid address
         strncpy((CHAR *)pParam2->lpstrReturn, szInfo, dwLen);
      pParam2->dwRetSize = INFO_SIZE;

   }
   else
      rc = MCIERR_INVALID_FLAG;

   if (rc && rc != MCIERR_INVALID_BUFFER)
      pParam2->dwRetSize = 0L;

   return(rc);

}  /* of CDAudInfo() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudOpen                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Open.                                        */
/*                                                                          */
/* FUNCTION:  Open a device dependent instance.                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      DWORD dwParam1   -- Flag for this message.                          */
/*      MMDRV_OPEN_PARMS *pParam2 -- Pointer to data record structure.      */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_INI_FILE  -- corrupted INI file, drive is not CD-ROM drive.  */
/*      MCIERR_OUT_OF_MEMORY       -- couldn't allocate memory for instance.*/
/*      MCIERR_INVALID_FLAG -- flag not supported by this VSD.              */
/*      MCIERR_DEVICE_LOCKED -- CD-ROM drive, previously opened exclusively.*/
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudOpen(DWORD dwParam1, MMDRV_OPEN_PARMS *pParam2)
{
   DWORD rc;
   PINST pInst;
   BYTE i;
   ULONG ulDevParmLen;
   PVOID pVSDParm;
   CHAR *pDrive, *pModel;

   /* validate flags */
   /* this VSD will only support standard flags */
   dwParam1 &= WAIT_NOTIFY_MASK;
   if (dwParam1)
      rc = MCIERR_INVALID_FLAG;
   else
   {
      /* get memory */
      pInst = HhpAllocMem(CDMC_hHeap, sizeof(struct instance_state));
      if (pInst == NULL)
         rc = MCIERR_OUT_OF_MEMORY;
      else
      {
         /* create semaphore so only one IOCLT call will be made at a time */
         rc = DosCreateMutexSem((PSZ) NULL, &pInst->hIOSem, 0L, FALSE);
         if (!rc)
            rc = DosCreateEventSem((PSZ) NULL, &pInst->hTimeLoopSem, 0L, FALSE);
         if (!rc)
            rc = DosCreateMutexSem((PSZ) NULL, &pInst->hInstSem, 0L, FALSE);
         if (!rc)
            rc = DosCreateEventSem((PSZ) NULL, &pInst->hReturnSem, 0L, FALSE);
         if (rc)
         {
            DosCloseMutexSem(pInst->hIOSem);
            DosCloseMutexSem(pInst->hInstSem);
            DosCloseEventSem(pInst->hTimeLoopSem);
            DosCloseEventSem(pInst->hReturnSem);
            rc = MCIERR_OUT_OF_MEMORY;

         }  /* of if error creating semaphores */
      }  /* else no error creating instance structure's memory */

      /* init instance */
      if (!rc)
      {  /* instance entry created */
         /* init instance */
         pInst->hDrive = 0;                     // drive handle
         pInst->dwCurPos = 0L;                  // set current position;
         pInst->wDeviceID = pParam2->wDeviceID;
         for (i=0; i < CDMCD_CUEPOINT_MAX; i++)          // init arrays
            pInst->arrCuePoint[i].dwEvent = (DWORD) -1L;

         pInst->qptPosAdvise.dwEvent = 0L;
         pInst->StreamMaster = FALSE;

         /* validate and get drive & hardware type */
         /* copy string so that it doesn't get disturb */
         ulDevParmLen = strlen(pParam2->pDevParm) + 1;
         pVSDParm = HhpAllocMem(CDMC_hHeap, ulDevParmLen);
         strcpy(pVSDParm, pParam2->pDevParm);
         parse_DevParm((CHAR *)pVSDParm, &pDrive, &pModel);
         pInst->Drive = *pDrive;

         if (stricmp(pModel, "IBMCD010"))
            pInst->usHWType = IBMCD019;       /* assume 3510 */
         else
            pInst->usHWType = IBMCD010;

         HhpFreeMem(CDMC_hHeap, pVSDParm);

         pInst->usPlayFlag = TIMER_AVAIL;
         pInst->ulPlayTID  = 0L;
         pInst->DiscID.dwLeadOut = 0L;

         /* Try opening the device.  Ignore things like disc not present, */
         /* but report errors involving the INI file or that the drive    */
         /* was previously opened in an exclusive mode.                   */
         rc = CD01_Open(pInst);
         if (rc != MCIERR_INI_FILE && rc != MCIERR_DEVICE_LOCKED)
            rc = MCIERR_SUCCESS;        /* pretend that all went well */

      }  /* of if no errors getting memory */

      if (rc)                                  /* if error, free memory */
         HhpFreeMem(CDMC_hHeap, pInst);
      else
         pParam2->pInstance = pInst;           /* pass pointer back */

   }  /* of else no invalid flags */

   return(rc);

}  /* of CDAudOpen() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudRegDisc                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Register Compact Disc.                       */
/*                                                                          */
/* FUNCTION:  Register a CD media so that the disc may be recognized by     */
/*            CDAUDIO.DLL.                                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      USHORT usFlag    -- Action Flag to copy into pParam2.               */
/*      MCI_CD_REGDISC_PARMS *pParam2 -- Pointer to data record.            */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudRegDisc(PINST pInst, USHORT usFlag,
                          MCI_CD_REGDISC_PARMS *pParam2)
{
   DWORD rc;
   USHORT i;
   BYTE lowest, highest;
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = STANDRD_PMAX;
   MCI_CD_ID DiscID;

   memcpy(&DiscID, &pInst->DiscID, sizeof(MCI_CD_ID));  //save original

   if (usFlag == REG_BOTH || usFlag == REG_INST)
   {   /* register with the internal instance */
      rc = CD01_GetID(pInst, &pInst->DiscID, &lowest, &highest);
      if (!rc && usFlag == REG_BOTH)
         /* copy information to external source */
         memcpy(&pParam2->DiscID, &pInst->DiscID, sizeof(MCI_CD_ID));
   }   /* of if update instance structure */
   else      /* the flag is REG_PARAM2 */
      rc = CD01_GetID(pInst, &pParam2->DiscID, &lowest, &highest);

   /* complete registration for param2 */
   if (!rc && (usFlag == REG_BOTH || usFlag == REG_PARAM2))
   {
      /* get low and high track numbers */
      pParam2->LowestTrackNum  = lowest;
      pParam2->HighestTrackNum = highest;

      /* get upc code -- device cannot do this so set this to zero */
      for (i=0; i < UPC_SIZE; i++)
         pParam2->UPC[i] = 0;

   }  /* of if need to complete Param2 */

   return(rc);

}  /* of CDAudRegDisc() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudRegDrive                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Register CD-ROM Drive and its capabilities.  */
/*                                                                          */
/* FUNCTION:  Register a CD media so that the disc may be recognized by     */
/*            CDAUDIO.DLL.                                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      MCI_CD_REGDRIVE_PARMS *pParam2 -- Pointer to data record.           */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudRegDrive(PINST pInst, MCI_CD_REGDRIVE_PARMS *pParam2)
{
   /* get the CD MCD return function information */
   pInst->dwCDMCDID    = pParam2->dwCDMCDID;
   pInst->pCDMCDReturn = pParam2->pCDMCDReturn;

   /* fill in capabilities */
   pParam2->wCaps = CD01_CAPS;        //This sample supports on the IBM 3510

   /* fill in preroll information */
   pParam2->dwPrerollType  = MCI_PREROLL_NONE;
   pParam2->dwPrerollTime  = 0L;
   pParam2->dwMinStartTime = SPEC_START;

   return(MCIERR_SUCCESS);
}  /* of CDAudRegDrive() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudSet                                               */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Set.                                         */
/*                                                                          */
/* FUNCTION:  Set features and attributes of the CD-ROM drive.              */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD *pParam1   -- Flag for this message.                          */
/*      MCI_SET_PARMS *pParam2   -- Pointer to data record structure.       */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_UNSUPPORTED_FLAG      -- unsupported flag.                   */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:  Mutual Exclusive flags (LOCK/UNLOCK & OPEN/CLOSE DOOR) are       */
/*         tested in the MCD.                                               */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudSet(PINST pInst, DWORD *pParam1, MCI_SET_PARMS *pParam2)
{
   DWORD rc;
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = STANDRD_PMAX;
   ULONG ulType;

   /* validate flags */
   rc = CDAudSetVerify(*pParam1);

   if (!rc)
   {
      ulType = *pParam1 & WAIT_NOTIFY_MASK;

      if (ulType & MCI_SET_DOOR_OPEN)
      {
         /* hardware does not support eject if playing, so stop it. */
         if (pInst->usPlayFlag == TIMER_PLAYING)
            rc = CD01_Stop(pInst, TIMER_EXIT_ABORTED);

         if (!rc)             // no error, proceed, else disc is already out
         {
            /* make sure that door is unlocked so eject can be successful */
            CD01_LockDoor(pInst, MCI_FALSE);
            CallIOCtl(pInst, CDDRIVE_CAT, EJECT__DISK,
                       "CD01", ulParamLen, &ulParamLen,
                        NULL,  ulDataLen,  &ulDataLen);
         }
         rc = MCIERR_SUCCESS;

      }  /* of if EJECTing disc */

      if (ulType & MCI_SET_DOOR_LOCK)
         rc = CD01_LockDoor(pInst, MCI_TRUE);
      else if (ulType & MCI_SET_DOOR_UNLOCK)
         rc = CD01_LockDoor(pInst, MCI_FALSE);

      if (ulType & MCI_SET_VOLUME)
         /* Hardware for example cannot support vectored volume */
         rc = CD01_SetVolume(pInst, pParam2->dwLevel);

   }  /* of if no error */

   return(rc);

}  /* of CDAudSet() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudSetVerify                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Set Verify                                   */
/*                                                                          */
/* FUNCTION:  Verify flags for SET command.  This function is called by     */
/*            the MCD to validate flags prior to calling other components,  */
/*            like the Amp/Mixer or SSM, to process their flags.            */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      DWORD dwParam1   -- Flag for this message.                          */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_UNSUPPORTED_FLAG      -- unsupported flag.                   */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*                                                                          */
/* NOTES:  Mutual Exclusive flags (LOCK/UNLOCK & OPEN/CLOSE DOOR) are       */
/*         tested in the MCD.                                               */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudSetVerify(DWORD dwParam1)
{
   DWORD rc = MCIERR_SUCCESS;

   /* check unsupported flags */
   if (dwParam1 & MCI_SET_DOOR_CLOSED)
      rc = MCIERR_UNSUPPORTED_FLAG;
   else
      /* check unknown flags */
      if (dwParam1 & ~(MCI_SET_DOOR_OPEN   | MCI_SET_DOOR_LOCK |
                       MCI_SET_DOOR_UNLOCK | MCI_SET_VOLUME | MCI_OVER |
                       MCI_NOTIFY | MCI_WAIT))
         rc = MCIERR_INVALID_FLAG;

   return(rc);

}  /* of CDAudSetVerify() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudStatus                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Status.                                      */
/*                                                                          */
/* FUNCTION:  Query and return the status of the features and attributes    */
/*            of the CD-ROM drive.                                          */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD dwParam1   -- Flag for this message.                          */
/*      MCI_STATUS_PARMS *pParam2   -- Pointer to data record structure.    */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_FLAGS_NOT_COMPATIBLE  -- Mis-match in flags.                 */
/*      MCIERR_UNSUPPORTED_FUNCTION  -- Flags not supported.                */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*      MCIERR_INVALID_ITEM_FLAG     -- Unknown item specified.             */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CDAudStatus(PINST pInst,
                         DWORD dwParam1, MCI_STATUS_PARMS *pParam2)
{
   DWORD rc;

   dwParam1 &= WAIT_NOTIFY_MASK;

   /* verify flags */
   if (dwParam1 & MCI_STATUS_ITEM)
      if (dwParam1 != MCI_STATUS_ITEM)
         if (dwParam1 & MCI_STATUS_START && dwParam1 & MCI_TRACK)
            rc = MCIERR_FLAGS_NOT_COMPATIBLE;
         else
            rc = MCIERR_INVALID_FLAG;
      else
         rc = MCIERR_SUCCESS;
   else                                      // no ITEM flag
      if (dwParam1 & MCI_TRACK)
         rc = MCIERR_MISSING_FLAG;
      else if (dwParam1 & MCI_STATUS_START)
         rc = MCIERR_MISSING_FLAG;
      else
         rc = MCIERR_INVALID_FLAG;

   if (!rc)                         // dwParam1 == MCI_STATUS_ITEM only
   {
      switch(pParam2->dwItem)
      {
         case MCI_STATUS_POSITION :
            rc = CD01_GetPosition(pInst, &pParam2->dwReturn);
            break;
         case MCI_STATUS_VOLUME :
            rc = CD01_GetVolume(pInst, &pParam2->dwReturn);
            break;
         case MCI_STATUS_MEDIA_PRESENT :
         case MCI_STATUS_MODE :
         case MCI_STATUS_READY :
            rc = CD01_GetDiscInfo(pInst, pParam2->dwItem, pParam2);
            break;
         default :
            rc = MCIERR_INVALID_ITEM_FLAG;

      }  /* of item switch */
   }  /* of if valid flag */

   return(rc);

}  /* of CDAudStatus() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CDAudStatCVol                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Status Component Volume.                     */
/*                                                                          */
/* FUNCTION:  Remaps the component volume to what it would be from the      */
/*            hardware if master audio was at 100%.                         */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      DWORD *dwLevel   -- Pointer to volume level.                        */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
DWORD CDAudStatCVol(DWORD *dwLevel)
{
   WORD wLeft, wRight;

   wLeft  = LOUSHORT(*dwLevel);
   wRight = HIUSHORT(*dwLevel);

   if (wLeft)  wLeft  = 100; else wLeft  = 0;
   if (wRight) wRight = 100; else wRight = 0;

   /* Adjust for hardware limitations, VOL = MAX(Left, Right) */
   if (wLeft && wRight)
      if (wLeft > wRight)
         wRight = wLeft;
      else
         wLeft = wRight;

   *dwLevel = MAKEULONG(wLeft, wRight);

   return(MCIERR_SUCCESS);

}  /* of CDAudStatCVol() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Cue                                               */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Cue.                                               */
/*                                                                          */
/* FUNCTION:  Cue up or preroll the drive.  To do this we seek to the       */
/*            current position.                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Cue(PINST pInst)
{
   DWORD rc, dwPos;

   /* test is disk is still present and get current position */
   rc = CD01_GetPosition(pInst, &dwPos);

   /* seek to current position to spin disc */
   if (!rc)                              // if no error
      rc = CD01_Seek(pInst, dwPos);

   return(rc);

}  /* of CD01_Cue() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_CuePoint                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Cue Point                                          */
/*                                                                          */
/* FUNCTION:  Set up the desired cuepoint.  To do this the cue point        */
/*            arrays will need to be updated.                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD dwParam1   -- Flag set for this message.                      */
/*      MCI_CUEPOINT_PARMS *pParam2 -- Pointer to data record structure.    */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_CUEPOINT_LIMIT_REACHED -- no more room to add events.        */
/*      MCIERR_INVALID_CUEPOINT       -- unable to locate event.            */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_CuePoint(PINST pInst,
                           DWORD dwParam1, MCI_CUEPOINT_PARMS *pParam2)
{
   DWORD rc = MCIERR_SUCCESS;
   int i;

   if (dwParam1 & MCI_SET_CUEPOINT_ON)
   {
      /* valid cuepoint, find first available entry,  */
      /* CD Audio MCD will make sure that we have room  */
      /* and that cuepoint is unique */
      for (i=0; i < CDMCD_CUEPOINT_MAX; i++)
         if (pInst->arrCuePoint[i].dwEvent == (DWORD) -1L)
            break;

      if (i == CDMCD_CUEPOINT_MAX)
         rc = MCIERR_CUEPOINT_LIMIT_REACHED;
      else
      {
         pInst->arrCuePoint[i].dwEvent = pParam2->dwCuepoint;
         pInst->arrCuePoint[i].dwCallback = pParam2->dwCallback;
         pInst->arrCuePoint[i].wUserParm = pParam2->wUserParm;
      }
   }  /* of if setting cuepoint */
   else
   {
      for (i=0; i < CDMCD_CUEPOINT_MAX; i++)
      {
         if (pInst->arrCuePoint[i].dwEvent == pParam2->dwCuepoint)
            break;
      }
      if (i == CDMCD_CUEPOINT_MAX)
         rc = MCIERR_INVALID_CUEPOINT;
      else
         pInst->arrCuePoint[i].dwEvent = (DWORD) -1L;

   }   /* of else MCI_SET_CUEPOINT_OFF */

   return(rc);

}  /* of CD01_CuePoint() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetCaps                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get Capabilities.                                  */
/*                                                                          */
/* FUNCTION:  Get information about the capabilities of the device and the  */
/*            VSD.                                                          */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      DWORD dwParam1   -- Flag set for this message.                      */
/*      MCI_GETDEVCAPS_PARMS *pParam2   -- Pointer to data record structure.*/
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_FLAGS_NOT_COMPATIBLE  -- Mis-match in flags.                 */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*      MCIERR_INVALID_ITEM_FLAG     -- Unknown item specified.             */
/*                                                                          */
/* NOTES:  Most of the capabilities are processed in the general MCI Driver.*/
/*      The messages supported need to be listed in the VSD because VSDs    */
/*      may support different messages.                                     */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetCaps(DWORD dwParam1, MCI_GETDEVCAPS_PARMS *pParam2)
{
   DWORD rc = MCIERR_SUCCESS;

   dwParam1 &= WAIT_NOTIFY_MASK;

   switch (dwParam1)
   {
      case MCI_GETDEVCAPS_MESSAGE :
         switch (pParam2->wMessage)
         {
            case MCI_CLOSE :                  case MCI_CUE :
            case MCI_GETDEVCAPS :             case MCI_GETTOC :
            case MCI_INFO :                   case MCI_OPEN :
            case MCI_PAUSE :                  case MCI_PLAY :
                                              case MCI_RESUME :
            case MCI_SEEK :                   case MCI_SET :
            case MCI_SET_CUEPOINT :           case MCI_SET_POSITION_ADVISE :
            case MCI_STATUS :                 case MCI_STOP :
            case MCIDRV_SAVE :                case MCIDRV_RESTORE :
            case MCIDRV_REGISTER_DISC :       case MCIDRV_REGISTER_DRIVE :
            case MCIDRV_REGISTER_TRACKS :     case MCIDRV_CD_STATUS_CVOL :
               pParam2->dwReturn = MCI_TRUE;
               break;
            default :
               pParam2->dwReturn = MCI_FALSE;
         }  /* of switch */
         break;
      case MCI_GETDEVCAPS_ITEM :            // MCD does all that VSD supports
         rc = MCIERR_INVALID_ITEM_FLAG;
         break;
      default :
         rc = MCIERR_INVALID_FLAG;
         break;
   }   /* of switch */

   return(rc);

}  /* of CD01_GetCaps() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetDiscInfo                                       */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get Disc Information                               */
/*                                                                          */
/* FUNCTION:  Get information contained on the compact disc.                */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      ULONG ulType     -- Type flag for this message.                     */
/*      MCI_STATUS_PARMS *pParam2   -- Pointer to data record structure.    */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_FLAGS_NOT_COMPATIBLE  -- Mis-match in flags.                 */
/*                                                                          */
/* NOTES:  The function supports the MCI_STATUS_MEDIA_PRESENT,              */
/*      MCI_STATUS_MODE and MCI_STATUS_READY flags for the MCI_STATUS       */
/*      command message.                                                    */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetDiscInfo(PINST pInst, ULONG ulType,
                               MCI_STATUS_PARMS *pParam2)
{
   DWORD rc = MCIERR_SUCCESS;
   BYTE data[SEEKSTATDMAX];
   ULONG ulDataLen = SEEKSTATDMAX, ulParamLen = STANDRD_PMAX;

   switch(ulType)
   {
      case MCI_STATUS_MEDIA_PRESENT :
         if (CallIOCtl(pInst, CDDRIVE_CAT, DEV__STATUS,
                      "CD01", ulParamLen, &ulParamLen,
                       data,  ulDataLen,  &ulDataLen))
            pParam2->dwReturn = MCI_FALSE;
         else
            if (data[STATAUDFLD] & NO_MEDIA)     // if No disc is present
               pParam2->dwReturn = MCI_FALSE;
            else
               pParam2->dwReturn = MCI_TRUE;
         break;
      case MCI_STATUS_MODE :
         CD01_GetState(pInst, pParam2);
         break;
      case MCI_STATUS_READY :
         CD01_GetState(pInst, pParam2);
         if (pParam2->dwReturn == MCI_MODE_NOT_READY)
            pParam2->dwReturn = MCI_FALSE;
         else
            pParam2->dwReturn = MCI_TRUE;
         break;
      default : rc = MCIERR_FLAGS_NOT_COMPATIBLE;

   }  /* of switch */

   return(rc);

}  /* of CD01_GetDiscInfo() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetID                                             */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get Disc ID.                                       */
/*                                                                          */
/* FUNCTION:  Get the Disc ID.  The disc ID is 8 bytes and format is:       */
/*            ÚÄÄÄÄÄÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄ¿                     */
/*            ³   01   ³Addr of³Num of³Address of the ³                     */
/*            ³UPC = 00³Track 1³Tracks³Lead Out Track ³                     */
/*            ÀÄÄÄÄÄÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÙ                     */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      MCI_CD_DISC_ID *pDiscID  -- Pointer to data record structure.       */
/*      BYTE  *LowTrack  -- Lowest track number.                            */
/*      BYTE  *HighTrack -- Highest track number.                           */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetID(PINST pInst, MCI_CD_ID *pDiscID,
                 BYTE *LowTrack, BYTE *HighTrack)
{
   DWORD rc;
   BYTE data[DSKTRCK_DMAX], param[DSKTRCK_PMAX] = "CD01";
   ULONG ulDataLen = DSKTRCK_DMAX, ulParamLen = STANDRD_PMAX;

   /* set mode */
   pDiscID->Mode = 1;

   /* get rest of data */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, DISK___INFO,
                   "CD01", ulParamLen, &ulParamLen,
                    data,  ulDataLen,  &ulDataLen);

   if (!rc)
   {
      /* set number of tracks */
      pDiscID->NumTracks = data[TRCKHI_FLD];
      *HighTrack = data[TRCKHI_FLD];

      /* set leadout track */
      pDiscID->dwLeadOut = REDBOOK2TOMM(*(DWORD *)&data[TRCKENDADR]);

      /* set first track */
      param[TRACKFIELD] = data[TRCKLOWFLD];
      *LowTrack = data[TRCKLOWFLD];

      /* get track information to get address of the first track */
      ulDataLen = DSKTRCK_DMAX;
      ulParamLen = DSKTRCK_PMAX;
      rc = CallIOCtl(pInst, CDAUDIO_CAT, TRACK__INFO,
                      param, ulParamLen, &ulParamLen,
                      data,  ulDataLen,  &ulDataLen);

      if (!rc)
         pDiscID->wTrack1 = (WORD) REDBOOK2TOMM(*(DWORD *)&data[TRACKFFFLD]);

   }   /* of if no error on IOCTL */

   return(rc);

}  /* of CD01_GetID() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetPosition                                       */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get Position.                                      */
/*                                                                          */
/* FUNCTION:  Get the current position of the CD-ROM drive.                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD *dwPosition -- ptr of the current position.                   */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_MEDIA_CHANGED         -- A disc change was reported.         */
/*                                                                          */
/* NOTES:  It is important to return the above three return codes because   */
/*         MCDs and Applications may rely on MCI_STATUS POSITION to verify  */
/*         that the same disc is still there.                               */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetPosition(PINST pInst, DWORD *dwPosition)
{
   DWORD rc;
   BYTE data[LOCATON_DMAX], param[LOCATON_PMAX] = {'C', 'D', '0', '1', RBMODE};
   ULONG ulDataLen = LOCATON_DMAX, ulParamLen = LOCATON_PMAX;

   /* get location of head in disc */
   rc = CallIOCtl(pInst, CDDRIVE_CAT, Q__LOCATION,
                   param, ulParamLen, &ulParamLen,
                   data,  ulDataLen,  &ulDataLen);

   if (!rc)
      if (pInst->usPlayFlag == TIMER_PLAYING)
      {
         *dwPosition = REDBOOK2TOMM(*(DWORD *)data);
         pInst->dwCurPos = *dwPosition;
      }  /* of if playing */
      else           /* if not playing then use logical location */
         *dwPosition = pInst->dwCurPos;

   return(rc);

}  /* of CD01_GetPosition() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetState                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get State.                                         */
/*                                                                          */
/* FUNCTION:  Get the state (playing, stopped, paused, etc.) of the device. */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      MCI_STATUS_PARMS *pParam2   -- Pointer to data record structure.    */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetState(PINST pInst, MCI_STATUS_PARMS *pParam2)
{
   DWORD rc;
   BYTE data[AUDSTAT_DMAX];      //values for the audio status command
   ULONG ulDataLen = AUDSTAT_DMAX, ulParamLen = STANDRD_PMAX;

   /* query the status */
   rc = CallIOCtl(pInst, CDDRIVE_CAT, DEV__STATUS,
                   "CD01", ulParamLen, &ulParamLen,
                   data,   ulDataLen,  &ulDataLen);

   if (rc)    /* error, manual eject */
      pParam2->dwReturn = MCI_MODE_NOT_READY;
   else
   {
      if (data[STATAUDFLD] & NO_MEDIA)               // if no disc
         pParam2->dwReturn = MCI_MODE_NOT_READY;
      else if (data[STATAUDFLD] & IS_PLAYING)        // if Playing
         pParam2->dwReturn = MCI_MODE_PLAY;
      else
         pParam2->dwReturn = MCI_MODE_STOP;

   }  /* of else no IOCTL error */

   return(rc);

}  /* of CD01_GetState() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetTOC                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get Table of Contents.                             */
/*                                                                          */
/* FUNCTION:  Get the Table of Contents of the playable portion of the CD.  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      MCI_TOC_PARMS *pParam2   -- Pointer to data record structure.       */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_INVALID_BUFFER        -- Buffer too small.                   */
/*      MCIERR_INVALID_MEDIA_TYPE    -- No audio tracks were found.         */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetTOC(PINST pInst, MCI_TOC_PARMS *pParam2)
{
   DWORD rc;
   BYTE data[DSKTRCK_DMAX], param[DSKTRCK_PMAX] = "CD01";
   BYTE low, hi;             // low and high track values
   BYTE cnt = 0, check = 0;  // audio track counters
   DWORD end_adr, cur_adr;
   ULONG ulDataLen = DSKTRCK_DMAX, ulParamLen = DSKTRCK_PMAX;

   /*** GET DISK INFORMATION ***/
   /* get disk information */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, DISK___INFO,
                    param, ulParamLen, &ulParamLen,
                    data,  ulDataLen,  &ulDataLen);

   if (!rc)
      if (pParam2->dwBufSize < sizeof(MCI_TOC_REC))
         rc = MCIERR_INVALID_BUFFER;

   low = data[TRCKLOWFLD];
   hi  = data[TRCKHI_FLD];

   if (!rc)
   {                                            // okay, extract data
      /* get ending address */
      end_adr = REDBOOK2TOMM(*(DWORD *)&data[TRENDFFFLD]);

      /*** LOOP TO FILL TABLE ***/
      /******************************************************************
       *  CNT is a counter of audio tracks, LOW starts with the lowest  *
       *  track number and increments with each track, CHECK is a flag  *
       *  marking the previous audio track so that the ending address   *
       *  may be assigned.                                              *
       ******************************************************************/

      for (; low <= hi; low++)
      {
         param[TRACKFIELD] = low;

         ulDataLen = DSKTRCK_DMAX;
         ulParamLen = DSKTRCK_PMAX;
         rc = CallIOCtl(pInst, CDAUDIO_CAT, TRACK__INFO,
                   param, ulParamLen, &ulParamLen,
                   data,  ulDataLen,  &ulDataLen);

         if (rc)
            break;

         cur_adr = REDBOOK2TOMM(*(DWORD *)&data[TRACKFFFLD]);

         /* Enter address as ending address of previous track */
         if (cnt != check)                     // skip first and data tracks
         {                                     // add ending address
            (pParam2->lpBuf + cnt-1)->dwEndAddr = cur_adr;
            check = cnt;
         }

         if (!(data[TRKSTATFLD] & IS_DATA_TRK))         // if audio track
         {  /* audio track has been found, is there room for it in buffer */
            /* check for valid buffer size */
            if (pParam2->dwBufSize < (DWORD)(cnt + 1) * sizeof(MCI_TOC_REC))
            {
               rc = MCIERR_INVALID_BUFFER;
               break;
            }

            (pParam2->lpBuf + cnt)->TrackNum = low;          // track number
            (pParam2->lpBuf + cnt)->dwStartAddr = cur_adr;   // add start loc
            (pParam2->lpBuf + cnt)->Control = data[TRKSTATFLD];   // Control
            (pParam2->lpBuf + cnt)->wCountry = 0;                 // Country
            (pParam2->lpBuf + cnt)->dwOwner = 0L;                 // Owner
            (pParam2->lpBuf + cnt)->dwSerialNum = 0L;             // S/N

            if (low == hi)                       // last addr if last track
               (pParam2->lpBuf + cnt)->dwEndAddr = end_adr;
            cnt++;

         }  /* of if not data track */

      }   /* of for loop */

      if (!cnt)
         rc = MCIERR_INVALID_MEDIA_TYPE;     // No audio tracks were found.

   }   /* of if no error getting audio disc information */

   /* find needed buffer size */
   if (rc == MCIERR_INVALID_BUFFER)
   {
      for (; low <= hi; low++)
      {
         param[TRACKFIELD] = low;

         ulDataLen = DSKTRCK_DMAX;
         ulParamLen = DSKTRCK_PMAX;
         if (CallIOCtl(pInst, CDAUDIO_CAT, TRACK__INFO,
                   param, ulParamLen, &ulParamLen,
                   data,  ulDataLen,  &ulDataLen))
         {
            rc = MCIERR_DEVICE_NOT_READY;
            break;
         }

         if (!(data[TRKSTATFLD] & IS_DATA_TRK))         // if audio track
            cnt++;

      }   /* of for loop */

   }  /* of if buffer too small */

   /* return buffer size */
   pParam2->dwBufSize = sizeof(MCI_TOC_REC) * cnt;

   return(rc);

}  /* of CD01_GetTOC() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_GetVolume                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Get Volume for the IBM 3510.                       */
/*                                                                          */
/* FUNCTION:  Get the left and right volume levels.                         */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD *dwVolume  -- Retrieved volume.                               */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_GetVolume(PINST pInst, DWORD *dwVolume)
{
   DWORD rc;
   BYTE data[QVOLUME_DMAX];
   ULONG ulDataLen = QVOLUME_DMAX, ulParamLen = STANDRD_PMAX;

   /* audio channel information */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, AUD_CH_INFO,
                    "CD01", ulParamLen, &ulParamLen,
                    data,  ulDataLen,  &ulDataLen);

   if (!rc)
      /* convert volume levels from 0-FF to 0-100% */
      *dwVolume = (data[VOL_LT_FLD] * 100 / 0xFF) |        // Left Channel
                  (data[VOL_RT_FLD] * 100 / 0xFF << 16);   // Right Chan

   return(rc);

}  /* of CD01_GetVolume() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_LockDoor                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Lock Door                                          */
/*                                                                          */
/* FUNCTION:  Disable or enable the manual eject button.                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      USHORT Lockit    -- Is To Be LOCK flag.                             */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_LockDoor(PINST pInst, USHORT LockIt)
{
   DWORD rc;
   BYTE param[LOCKDOR_PMAX] = "CD01";
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = LOCKDOR_PMAX;

   if (LockIt == MCI_TRUE)
      param[LOCKDORFLD] = 1;             //Eject button is disabled
   else
      param[LOCKDORFLD] = 0;             //Eject button is enabled

   rc = CallIOCtl(pInst, CDDRIVE_CAT, LOCK___DOOR,
                   param, ulParamLen, &ulParamLen,
                   NULL,  ulDataLen,  &ulDataLen);

   /* IBM device driver error, lock or unlock twice */
   ulDataLen = STANDRD_DMAX;
   ulParamLen = LOCKDOR_PMAX;

   rc = CallIOCtl(pInst, CDDRIVE_CAT, LOCK___DOOR,
                   param, ulParamLen, &ulParamLen,
                   NULL,  ulDataLen,  &ulDataLen);


   return(rc);

}  /* of CD01_LockDoor() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Open                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Open.                                              */
/*                                                                          */
/* FUNCTION:  Open the CD-ROM drive.                                        */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_INI_FILE  -- corrupted INI file, drive is not CD-ROM drive.  */
/*      MCIERR_DEVICE_LOCKED -- CD-ROM drive, previously opened exclusively.*/
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_MEDIA_CHANGED         -- different disc was inserted.        */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Open(PINST pInst)
{
   DWORD rc;

   CHAR drive[] = "A:";
   ULONG ulAction;
   HFILE hDev;
   MCI_CD_REGDISC_PARMS dwParam2;
   BYTE data[LOCATON_DMAX], param[LOCATON_PMAX] = {'C', 'D', '0', '1', RBMODE};
   ULONG ulDataLen, ulParamLen;

   drive[0] = pInst->Drive;               /* Get drive letter */

   /* open device */
   rc = DosOpen(drive, &hDev, &ulAction, 0L, 0L, OPENFLAG, OPENMODE, 0L);

   if (rc)
   {
      switch (rc)
      {
         case ERROR_PATH_NOT_FOUND :     //network drive
         case ERROR_INVALID_DRIVE :      //invalid drive, not reg in boot-up(?)
            rc = MCIERR_INI_FILE;
            break;
         case ERROR_SHARING_VIOLATION :  //drive opened exclusively
            rc = MCIERR_DEVICE_LOCKED;
            break;
         case ERROR_NOT_READY :          //disc not in drive, drive powered off
         default :
            /* resume timer loop so that it can exit */
            DosPostEventSem(pInst->hTimeLoopSem);
            /* make play completed before returning */
            while (pInst->usPlayFlag != TIMER_AVAIL)
               DosSleep(HALF_TIME_MIN);
            rc = MCIERR_DEVICE_NOT_READY;
      }  /* of switch() */
   }  /* of if error */
   else                                   /* open was successful */
   {
      pInst->hDrive = hDev;

      if (pInst->DiscID.dwLeadOut == 0L)            // if New Open
      {
         /* test to see if drive is really a CD-ROM Drive */
         ulDataLen  = IDCDROM_DMAX;
         ulParamLen = STANDRD_PMAX;
         if (CallIOCtl(pInst, CDDRIVE_CAT, ID___CD_ROM,
                        param, ulParamLen, &ulParamLen,
                        data,  ulDataLen,  &ulDataLen))
            rc = MCIERR_INI_FILE;
         else
            if (memcmp(data, "CD01", IDCDROM_DMAX))
               rc = MCIERR_INI_FILE;
            else
               rc = MCIERR_SUCCESS;

         if (!rc)
         {  /** Get current position and let the registration *
              * insert information to the instance structure. **/
            ulDataLen  = LOCATON_DMAX;
            ulParamLen = LOCATON_PMAX;
            rc = CallIOCtl(pInst, CDDRIVE_CAT, Q__LOCATION,
                           param, ulParamLen, &ulParamLen,
                           data,  ulDataLen,  &ulDataLen);
            pInst->dwCurPos = REDBOOK2TOMM(*(DWORD *)data);

            rc = CDAudRegDisc(pInst, REG_INST, NULL);
         }   /* of if no error identifying drive */
      }
      else
      {
         /** Previous Open, register separately and compare with old  *
           * disc.  If the same, pretend nothing happened, otherwise  *
           * return code to make general MCI Driver do a re-register  */
         rc = CDAudRegDisc(pInst, REG_PARAM2, &dwParam2);

         if (!rc)
            if (memcmp(&pInst->DiscID, &dwParam2.DiscID, sizeof(MCI_CD_ID)))
            {
               CD01_Stop(pInst, TIMER_EXIT_CHANGED);
               rc = MCIERR_MEDIA_CHANGED;           /* Different disc */
            }
      }  /* of else not a new open */
   }  /* of else device is opened */

   return(rc);

}  /* of CD01_Open() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Play                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Play.                                              */
/*                                                                          */
/* FUNCTION:  Initiate playing audio data to internal DAC(s).               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst       -- Instance pointer.                              */
/*      DWORD *pParam1    -- Flag for this message.                         */
/*      DWORD dwFrom      -- From address.                                  */
/*      DWORD dwTo        -- To address in MMTIME.                          */
/*      WORD  wUserParm   -- User Parameter.                                */
/*      HWND  hCallback   -- Call back handle.                              */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY  -- device was not ready, no disc.          */
/*      MCIERR_MEDIA_CHANGED     -- Disc changed.                           */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Play(PINST pInst, DWORD *pParam1, DWORD dwFrom, DWORD dwTo,
                       WORD wUserParm, HWND hCallback)
{
   DWORD rc;
   DWORD dwThreadID;
   ULONG cnt;

   /* Stop drive before issuing next play command */
   if ((pInst->usPlayFlag == TIMER_PLAYING) ||
       (pInst->usPlayFlag == TIMER_PLAY_SUSPEND) ||
       (pInst->usPlayFlag == TIMER_PLAY_SUSP_2))
      if (*pParam1 & MCI_WAIT)
         CD01_Stop(pInst, TIMER_EXIT_ABORTED);
      else
         CD01_Stop(pInst, TIMER_EXIT_SUPER);

   /* prepare for play call */
   pInst->dwCurPos = dwFrom;
   pInst->dwEndPos = dwTo;
   pInst->wPlayNotify = (WORD)(*pParam1 & (MCI_WAIT | MCI_NOTIFY));
   if (*pParam1 & MCI_NOTIFY)
   {
      pInst->wPlayUserParm = wUserParm;
      pInst->hPlayCallback = hCallback;
      *pParam1 ^= MCI_NOTIFY;
   }  /* notify flag was used */

   if (*pParam1 & MCI_WAIT)
      rc = CD01_Timer(pInst);      /* returns when play commands end */
   else
   {
      DosResetEventSem(pInst->hReturnSem, &cnt);  //force a wait
      rc = DosCreateThread(&dwThreadID, (PFNTHREAD)CD01_Timer,
                   (DWORD)pInst, 0L, THREAD_STACK_SZ);
      if (rc)
      {
         rc = MCIERR_OUT_OF_MEMORY;
         DosPostEventSem(pInst->hReturnSem);
      }
      else  /* wait for new thread to process enough */
      {
         /* Let MCD know not to send notification by returning wait */
         *pParam1 = (*pParam1 & ~MCI_NOTIFY) | MCI_WAIT;

         /* wait for new thread to process enough */
         DosWaitEventSem(pInst->hReturnSem, WAIT_FOREVER);
      }

      DosReleaseMutexSem(pInst->hInstSem);

   }  /* else no wait flag was used */

   return(rc);

}  /* of CD01_Play() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_PlayCont                                          */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Play Continue.                                     */
/*                                                                          */
/* FUNCTION:  Continue to play audio data to internal DAC(s) from a         */
/*            MCIDRV_RESTORE or MCIDRV_SYNC command.                        */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst       -- Instance pointer.                              */
/*      DWORD dwFrom      -- From address.                                  */
/*      DWORD dwTo        -- To address in MMTIME.                          */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY  -- device was not ready, no disc.          */
/*      MCIERR_MEDIA_CHANGED     -- Disc changed.                           */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_PlayCont(PINST pInst, DWORD dwFrom, DWORD dwTo)
{
   DWORD rc;
   BYTE param[PLAYAUD_PMAX] = {'C', 'D', '0', '1', RBMODE};
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = PLAYAUD_PMAX;

   /* convert starting MM Time into Redbook 2 format */
   * (DWORD *)&param[STARTFFFLD] = REDBOOK2FROMMM(dwFrom);

   /* convert ending MM Time into Redbook 2 format */
   * (DWORD *)&param[END_FF_FLD] = REDBOOK2FROMMM(dwTo);

   /* Stop drive before issuing next play command */
   CD01_Stop(pInst, TIMER_PLAY_SUSPEND);

   /* play drive */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, PLAY__AUDIO,
                  param, ulParamLen, &ulParamLen,
                  NULL,  ulDataLen,  &ulDataLen);

   if (!rc)
      pInst->dwCurPos = dwFrom;

   /* if Timer was stopped, continue timer loop */
   if (pInst->usPlayFlag == TIMER_PLAY_SUSPEND)
      pInst->usPlayFlag = TIMER_PLAYING;
   DosPostEventSem(pInst->hTimeLoopSem);

   return(rc);

}  /* of CD01_PlayCont() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_PosAdvise                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Position Advise                                    */
/*                                                                          */
/* FUNCTION:  Set up the desired position advise.                           */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD dwParam1   -- Flag set for this message.                      */
/*      MCI_POSITION_PARMS *pParam2 -- Pointer to data record structure.    */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_FLAGS_NOT_COMPATIBLE -- Flags are mutually exclusive.        */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_PosAdvise(PINST pInst, DWORD dwParam1,
                            MCI_POSITION_PARMS *pParam2)
{
   DWORD rc = MCIERR_SUCCESS;

   if (dwParam1 & MCI_SET_POSITION_ADVISE_ON)
   {
      if (dwParam1 & MCI_SET_POSITION_ADVISE_OFF)
         rc = MCIERR_FLAGS_NOT_COMPATIBLE;
      else
      {
         pInst->qptPosAdvise.dwEvent = pParam2->dwUnits;
         pInst->qptPosAdvise.dwCallback = pParam2->dwCallback;
         pInst->qptPosAdvise.wUserParm = pParam2->wUserParm;
      }
   }  /* of if on */
   else
      if (dwParam1 & MCI_SET_POSITION_ADVISE_OFF)
         pInst->qptPosAdvise.dwEvent = 0L;

   return(rc);

}  /* of CD01_PosAdvise() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_RegTracks                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Audio Register Compact Disc.                       */
/*                                                                          */
/* FUNCTION:  Register a CD media so that the disc may be recognized by     */
/*            CDAUDIO.DLL.                                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      MCI_CD_REGTRACKS_PARMS *pParam2 -- Pointer to data record.          */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_INVALID_BUFFER        -- Buffer size is too small.           */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_RegTracks(PINST pInst, MCI_CD_REGTRACKS_PARMS *pParam2)
{
   DWORD rc;
   BYTE data[DSKTRCK_DMAX], param[DSKTRCK_PMAX] = "CD01";
   BYTE low, hi, cnt;       // low and high track values
   DWORD end_adr, cur_adr;
   ULONG ulDataLen = DSKTRCK_DMAX, ulParamLen = DSKTRCK_PMAX;


   /*** GET DISK INFORMATION ***/

   /* get disk information */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, DISK___INFO,
                   param, ulParamLen, &ulParamLen,
                   data,  ulDataLen,  &ulDataLen);

   if (!rc)
   {                                            // okay, extract data
      low = data[TRCKLOWFLD];
      hi  = data[TRCKHI_FLD];

      /* get ending address */
      end_adr = REDBOOK2TOMM(*(DWORD *)&data[TRENDFFFLD]);

      /* check for valid buffer size */
      if (pParam2->dwBufSize <
             (DWORD)(hi - low + 1) * sizeof(MCI_CD_REGTRACK_REC))
         rc = MCIERR_INVALID_BUFFER;
      else
      {
         rc = MCIERR_SUCCESS;

         /*** LOOP TO FILL TABLE ***/

         for (cnt = 0; low <= hi; cnt++, low++)
         {
            param[TRACKFIELD] = low;

            ulDataLen  = DSKTRCK_DMAX;
            ulParamLen = DSKTRCK_PMAX;
            if (CallIOCtl(pInst, CDAUDIO_CAT, TRACK__INFO,
                            param, ulParamLen, &ulParamLen,
                            data,  ulDataLen,  &ulDataLen))
            {
               rc = MCIERR_DEVICE_NOT_READY;
               break;
            }

            /* get track number */
            (pParam2->TrackRecArr + cnt)->TrackNum = low;

            /* get control byte */
            (pParam2->TrackRecArr + cnt)->TrackControl = data[TRKSTATFLD];

            /* get starting address */
            cur_adr = REDBOOK2TOMM(*(DWORD *)&data[TRACKFFFLD]);
            (pParam2->TrackRecArr + cnt)->dwStartAddr = cur_adr;

            if (cnt)
               /* get ending address */
               (pParam2->TrackRecArr + cnt-1)->dwEndAddr = cur_adr;

            /* get ending address if it is the last track */
            if (low == hi)
               (pParam2->TrackRecArr + cnt)->dwEndAddr = end_adr;

         }   /* of for loop */
      }   /* of else valid buffer size */
   }   /* of if no error getting audio disc information */

   /* return buffer size */
   if (rc)
      pParam2->dwBufSize = 0;
   else
      pParam2->dwBufSize = sizeof(MCI_CD_REGTRACK_REC) * cnt;

   return(rc);

}  /* of CD01_RegTracks() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Restore                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Restore.                                           */
/*                                                                          */
/* FUNCTION:  The device context or instance is being restored.  Restore    */
/*            the drive as listed in pParams.                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Pointer to instance.                            */
/*      MCIDRV_CD_SAVE_PARMS *pParam2 -- pointer to save structure.         */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed.                               */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Restore(PINST pInst, MCIDRV_CD_SAVE_PARMS *pParam2)
{
   DWORD rc;

   /* reset volume */
   rc = CD01_SetVolume(pInst, pParam2->dwLevel);

   /* reset position and mode */
   if (!rc)
   {
      switch (pParam2->dwMode)
      {
         case MCI_MODE_STOP :
            rc = CD01_Seek(pInst, pParam2->dwPosition);
            break;
         case MCI_MODE_PLAY :
            rc = CD01_PlayCont(pInst, pParam2->dwPosition, pParam2->dwEndPlay);
            DosPostEventSem(pInst->hTimeLoopSem);  //continue timer loop
            break;
         case MCI_MODE_PAUSE :
            rc = CD01_Stop(pInst, TIMER_PLAY_SUSP_2);
            if (!rc)
            {
               pInst->dwCurPos = pParam2->dwPosition;
               pInst->dwEndPos = pParam2->dwEndPlay;
            }
            break;
         case MCI_MODE_NOT_READY :                /* disc changed on restore */
            rc = CD01_Stop(pInst, TIMER_EXIT_CHANGED);
            break;
         default :   /* error in saved state, try to accept current position */
            break;
      }  /* of switch */
   }  /* if no error setting volume */

   return(rc);

}  /* of CD01_Restore() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Resume                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Resume Playing.                                    */
/*                                                                          */
/* FUNCTION:  Unpause or resume the play command.                           */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Resume(PINST pInst)
{
   DWORD rc;
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = STANDRD_PMAX;

   if (pInst->usPlayFlag == TIMER_PLAY_SUSP_2)
   {
      rc = CD01_PlayCont(pInst, pInst->dwCurPos, pInst->dwEndPos);
      DosPostEventSem(pInst->hTimeLoopSem);
   }
   else
   {
      /* resume audio */
      rc = CallIOCtl(pInst, CDAUDIO_CAT, RESUMEAUDIO,
                     "CD01", ulParamLen, &ulParamLen,
                      NULL,  ulDataLen,  &ulDataLen);

      /* check if resume failed, a disc was ejected and reinserted */
      if (rc == MCIERR_OUTOFRANGE)
         rc = CD01_PlayCont(pInst, pInst->dwCurPos, pInst->dwEndPos);

      if (!rc)
      {
         if (pInst->usPlayFlag == TIMER_PLAY_SUSPEND)
            pInst->usPlayFlag = TIMER_PLAYING;
         DosPostEventSem(pInst->hTimeLoopSem);
      }
   }   /* of else resuming paused audio */

   return(rc);

}  /* of CD01_Resume() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Save                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Save.                                              */
/*                                                                          */
/* FUNCTION:  The device context or instance is being saved.  Stop the      */
/*            drive and the play timer if needed.  Save the current state.  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Pointer to instance.                            */
/*      MCIDRV_CD_SAVE_PARMS *pParam2 -- pointer to save structure.         */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed.                               */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Save(PINST pInst, MCIDRV_CD_SAVE_PARMS *pParam2)
{
   DWORD rc;

   /* cheek to see if it is playing. If it is, stop drive */
   if (pInst->usPlayFlag == TIMER_PLAYING)
      CD01_Stop(pInst, TIMER_PLAY_SUSPEND);     // Don't care about return code
   else
      if (pInst->usPlayFlag == TIMER_PLAY_SUSP_2)
         pInst->usPlayFlag = TIMER_PLAY_SUSPEND;

   /* get volume */
   CDAudStatCVol(&pParam2->dwLevel);

   /* get position */
   rc = CD01_GetPosition(pInst, &pParam2->dwPosition);

   return(rc);

}  /* of CD01_Save() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Seek                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Seek.                                              */
/*                                                                          */
/* FUNCTION:  Seek to the specified position.                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance pointer.                               */
/*      DWORD dwTo       -- address to seek.                                */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Seek(PINST pInst, DWORD dwTo)
{
   DWORD rc;
   BYTE data[SEEKSTATDMAX], param[SEEKSTATPMAX] = {'C', 'D', '0', '1', RBMODE};
   ULONG ulDataLen = SEEKSTATDMAX, ulParamLen = SEEKSTATPMAX;

   /* Stop drive before issuing SEEK command,         *
    *    drive will not SEEK Command if it is playing */
   rc = CallIOCtl(pInst, CDDRIVE_CAT, DEV__STATUS,
                   "CD01", ulParamLen, &ulParamLen,
                    data,  ulDataLen,  &ulDataLen);

   if (!rc)        /* stop drive even if it thinks that its playing */
      if ((data[STATAUDFLD] & IS_PLAYING) ||
          (pInst->usPlayFlag == TIMER_PLAYING) ||
          (pInst->usPlayFlag == TIMER_PLAY_SUSPEND) ||
          (pInst->usPlayFlag == TIMER_PLAY_SUSP_2))
         rc = CD01_Stop(pInst, TIMER_EXIT_ABORTED);

   if (!rc)
   {
      /* convert MM Time into Redbook 2 format */
      * (DWORD *)&param[SEEK_FFFLD] = REDBOOK2FROMMM(dwTo);

      /* Seek to new location */
      ulDataLen  = STANDRD_DMAX;
      ulParamLen = SEEKSTATPMAX;
      rc = CallIOCtl(pInst, CDDRIVE_CAT, SEEK_POSITN,
                      param, ulParamLen, &ulParamLen,
                      NULL,  ulDataLen,  &ulDataLen);

      if (!rc)
         pInst->dwCurPos = dwTo;

   }  /* of if no error preparing disk for seek */

   return(rc);

}  /* of CD01_Seek() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_SetVolume                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Set Volume.                                        */
/*                                                                          */
/* FUNCTION:  Set the left and right volume levels on the CD-ROM drive.     */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance structure.                             */
/*      DWORD dwVolume   -- Volume Level.                                   */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_SetVolume(PINST pInst, DWORD dwVolume)
{

   DWORD rc;
   BYTE data[QVOLUME_DMAX], left, right;
   ULONG ulDataLen = QVOLUME_DMAX, ulParamLen = STANDRD_PMAX;

   /* get current audio channel information */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, AUD_CH_INFO,
                    "CD01", ulParamLen, &ulParamLen,
                    data,   ulDataLen,  &ulDataLen);

   /* get requested volume levels */
   left  = (BYTE) dwVolume;
   right = (BYTE) (dwVolume >> 16);

   if (!rc)
   {
      /* Since this is the IBM 3510, reset left and right to 0 or FF */
      if (left)
         left = 0xFF;

      if (right)
         right = 0xFF;

      /* Save an IOCLT call by only resetting if a change occurred */

      if (data[VOL_LT_FLD] != left || data[VOL_RT_FLD] != right)
      {
         data[VOL_LT_FLD] = left;
         data[VOL_RT_FLD] = right;
         /* set volume */
         ulDataLen = QVOLUME_DMAX;
         ulParamLen = STANDRD_PMAX;
         rc = CallIOCtl(pInst, CDAUDIO_CAT, AUD_CH_CTRL,
                  "CD01", ulParamLen, &ulParamLen,
                   data,  ulDataLen,  &ulDataLen);

      } /* of if volume had changed */

   }  /* of else Audio Channel Info DevIOCtl worked */

   return(rc);

}  /* of CD01_SetVolume() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_StartPlay                                         */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Start Play.                                        */
/*                                                                          */
/* FUNCTION:  Start playing audio data to internal DAC(s).                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst       -- Instance pointer.                              */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY  -- device was not ready, no disc.          */
/*      MCIERR_MEDIA_CHANGED     -- Disc changed.                           */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_StartPlay(PINST pInst)
{
   DWORD rc;
   BYTE param[PLAYAUD_PMAX] = {'C', 'D', '0', '1', RBMODE};
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = PLAYAUD_PMAX;
   ULONG cnt;
   PTIB ptib;
   PPIB ppib;

   /* convert MM Time into Redbook 2 format */
   * (DWORD *)&param[STARTFFFLD] = REDBOOK2FROMMM(pInst->dwCurPos);
   * (DWORD *)&param[END_FF_FLD] = REDBOOK2FROMMM(pInst->dwEndPos);

   /* play drive */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, PLAY__AUDIO,
                    param, ulParamLen, &ulParamLen,
                    NULL,  ulDataLen,  &ulDataLen);

   if (!rc)
   {
      /* set timer play flag */
      pInst->usPlayFlag = TIMER_PLAYING;
      DosGetInfoBlocks(&ptib, &ppib);
      pInst->ulPlayTID = ptib->tib_ptib2->tib2_ultid;
      DosResetEventSem(pInst->hTimeLoopSem, &cnt);  // force a wait in timer

   }  /* if no error */

   /* original thread owns Mutex sem,               */
   /* if not WAIT tell original thead to release it */
   if (pInst->wPlayNotify == MCI_WAIT)
      DosReleaseMutexSem(pInst->hInstSem);
   else
      DosPostEventSem(pInst->hReturnSem);

   return(rc);

}  /* of CD01_StartPlay() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Stop                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Stop.                                              */
/*                                                                          */
/* FUNCTION:  Stop the play command.                                        */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst        -- Instance pointer.                             */
/*      USHORT usTimerFlag -- Set timer to this flag, if playing.           */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Stop(PINST pInst, USHORT usTimerFlag)
{
   DWORD rc;
   USHORT StopTimer = TRUE;
   ULONG ulDataLen = STANDRD_DMAX, ulParamLen = STANDRD_PMAX;

   /* stop timer if one is going */
   if (pInst->usPlayFlag == TIMER_PLAYING ||
       pInst->usPlayFlag == TIMER_PLAY_SUSPEND ||
       pInst->usPlayFlag == TIMER_PLAY_SUSP_2)
   {
      pInst->usPlayFlag = usTimerFlag;

      switch(usTimerFlag)
      {
         case TIMER_PLAY_SUSPEND :
         case TIMER_PLAY_SUSP_2 :
            StopTimer = FALSE;
      }  /* of switch */

      if (StopTimer)
      {
         /* resume timer loop so that it can exit */
         DosPostEventSem(pInst->hTimeLoopSem);
         while (pInst->usPlayFlag != TIMER_AVAIL)  //make play completed before
            DosSleep(HALF_TIME_MIN);               //returning, race condition.
      }
   }  /* of if timer is being used */

   /* stop drive */
   rc = CallIOCtl(pInst, CDAUDIO_CAT, STOP__AUDIO,
                   "CD01", ulParamLen, &ulParamLen,
                    NULL,  ulDataLen,  &ulDataLen);

   return(rc);

}  /* of CD01_Stop() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Sync                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Synchronization.                                   */
/*                                                                          */
/* FUNCTION:  Process SYNC messages from MCIDRV_SYNC message.               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance pointer.                               */
/*      DWORD dwParam1   -- Flag for this message.                          */
/*      MCIDRV_SYNC_PARMS *pParam2 -- pointer to record structure.          */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*      MCIERR_INVALID_FLAG          -- Unknown flag.                       */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Sync(PINST pInst, DWORD dwParam1, MCIDRV_SYNC_PARMS *pParam2)
{
   DWORD rc = MCIERR_SUCCESS;
   DWORD dwPosition;
   LONG  lDelta;

   dwParam1 &= WAIT_NOTIFY_MASK;

   switch (dwParam1)
   {
      case MCIDRV_SYNC_ENABLE | MCIDRV_SYNC_MASTER :
         pInst->StreamMaster = TRUE;        // continue on
      case MCIDRV_SYNC_ENABLE :
         pInst->pSyncInst = pParam2->pInstance;
         pInst->GroupID = pParam2->GroupID;
         break;
      case MCIDRV_SYNC_DISABLE :
         pInst->StreamMaster = FALSE;
         break;
      case MCIDRV_SYNC_REC_PULSE :
         /* get current position */
         CD01_GetPosition(pInst, &dwPosition);

         lDelta = dwPosition - pParam2->mmTime - SYNC_LATENCY;
         if (lDelta < -SYNC_TOLERANCE || lDelta > SYNC_TOLERANCE)
            rc = CD01_PlayCont(pInst, (DWORD) pParam2->mmTime, pInst->dwEndPos);
         break;
      default :
         rc = MCIERR_INVALID_FLAG;
   }  /* of switch */

   return(rc);

}  /* of CD01_Sync() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_Timer                                             */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Timer                                              */
/*                                                                          */
/* FUNCTION:  Queries the device once a second to determine is a play       */
/*            command is still going on.                                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance pointer.                               */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY  -- device was not ready, no disc.          */
/*      MCIERR_MEDIA_CHANGED     -- Disc changed on restore as reported by  */
/*                                  MCD.  Return error on play thread.      */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_Timer(PINST pInst)
{
   DWORD rc;
   DWORD dwDelay, dwPrevLimit;
   DWORD dwPosAdvise = 0L, dwSyncPos = 0L, dwOldPosAdvise = 0L;
   BYTE dataAud[AUDSTAT_DMAX];    //values for the audio status command
   BYTE dataPos[LOCATON_DMAX],
          param[LOCATON_PMAX] = {'C', 'D', '0', '1', RBMODE};
   ULONG ulDataLen, ulParamLen;
   ULONG cnt, DoLoop = TRUE;
   WORD wNotifyType = MCI_NOTIFY_SUCCESSFUL;

   /* start playing */
   rc = CD01_StartPlay(pInst);
   dwPrevLimit = pInst->dwCurPos;

   if (!rc)
   do
   {
      /* check position advise before calling notification routine. */
      /* event must be multiple address relative to address 0.      */
      if (pInst->qptPosAdvise.dwEvent != dwOldPosAdvise)
      {
         dwOldPosAdvise = pInst->qptPosAdvise.dwEvent;
         if (dwOldPosAdvise)
            if (pInst->dwCurPos % dwOldPosAdvise)    //if past, get next one
               dwPosAdvise = (pInst->dwCurPos / dwOldPosAdvise + 1)
                              * dwOldPosAdvise;
            else
               dwPosAdvise = pInst->dwCurPos;
         else
            dwPosAdvise = 0L;
      }  /* of if position advise changed */

      dwDelay = CD01_TimerNotify(pInst, &dwPosAdvise, &dwSyncPos, &dwPrevLimit);

      /********************************************************************
       * wait on the semaphore.  If the instance is PAUSED or being SAVED *
       * (no longer active) then wait until RESUME or RESTORE clears the  *
       * semaphore.  If a destructive STOP is received, the wait will     *
       * terminate immediately.  Otherwise, wait as long as dwDelay.      *
       ********************************************************************/

      if (!DosWaitEventSem(pInst->hTimeLoopSem, (ULONG) dwDelay))
         /* semaphore is cleared.  Reset it incase we don't exit loop */
         DosResetEventSem(pInst->hTimeLoopSem, &cnt);

      /* get new position */
      ulDataLen  = LOCATON_DMAX;
      ulParamLen = LOCATON_PMAX;
      rc = CallIOCtl(pInst, CDDRIVE_CAT, Q__LOCATION,
                   param, ulParamLen, &ulParamLen,
                   dataPos,  ulDataLen,  &ulDataLen);

      /* The only way to receive a media change rc is when closing   *
       * an inactive instance after a disc change, process the abort */
      if (rc)
         if (rc == MCIERR_MEDIA_CHANGED)
            rc = MCIERR_SUCCESS;
         else   // an error occurred, break out of the loop
            break;

      switch (pInst->usPlayFlag)
      {
         case TIMER_EXIT_SUPER :
            wNotifyType = MCI_NOTIFY_SUPERSEDED;
            DoLoop = FALSE;
            break;
         case TIMER_EXIT_ABORTED :
            wNotifyType = MCI_NOTIFY_ABORTED;
            DoLoop = FALSE;
            break;
         case TIMER_EXIT_CHANGED :
            rc = MCIERR_MEDIA_CHANGED;
            DoLoop = FALSE;
            break;
         case TIMER_AVAIL :
            rc = MCIERR_DRIVER_INTERNAL;       // abondon ship
            DoLoop = FALSE;
            break;
         case TIMER_PLAY_SUSPEND :             // do nothing
         case TIMER_PLAY_SUSP_2  :
            break;
         case TIMER_PLAYING :
            /* update current location */
            pInst->dwCurPos = REDBOOK2TOMM(*(DWORD *)dataPos);

            DosRequestMutexSem(pInst->hIOSem, (ULONG)-1L);

            /* check if drive is still playing */
            ulDataLen  = AUDSTAT_DMAX;
            ulParamLen = STANDRD_PMAX;
            if (DosDevIOCtl(pInst->hDrive, CDDRIVE_CAT, DEV__STATUS,
                            "CD01", ulParamLen, &ulParamLen,
                            dataAud,   ulDataLen,  &ulDataLen))
            {                         // Error
               rc = MCIERR_DEVICE_NOT_READY;
               DoLoop = FALSE;
            }  /* of if error calling device */
            else
            {
               if (!(dataAud[STATAUDFLD] & IS_PLAYING))      // if NOT Playing
               {
                  /* check to see if PLAY terminated or was STOPPED/PAUSED */
                  ulDataLen  = AUDSTAT_DMAX;
                  ulParamLen = STANDRD_PMAX;
                  if (DosDevIOCtl(pInst->hDrive, CDAUDIO_CAT, AUD__STATUS,
                                  "CD01", ulParamLen, &ulParamLen,
                                   dataAud,   ulDataLen,  &ulDataLen))
                  {
                     rc = MCIERR_DEVICE_NOT_READY;
                     DoLoop = FALSE;
                  }  /* of if error calling device */
                  else
                     if (!(dataAud[AUDSTATFLD] & WAS_STOPPED))
                     {     /* PLAY command finished */
                        rc = MCIERR_SUCCESS;
                        DoLoop = FALSE;
                     }  /* of else PLAY command terminated itself (finished) */
                     /* else it is paused, continue the clock */

               }  /* of if NOT Playing */
            }  /* of else no error querying status */

            DosReleaseMutexSem(pInst->hIOSem);
            break;
         default :
            rc = MCIERR_DRIVER_INTERNAL;       // unknown play flag
            DoLoop = FALSE;
            break;

      }   /* of switch on play flag */
   } while(DoLoop);  /* of do loop */

   if (!rc)         // if no error, report any events
      CD01_TimerNotify(pInst, &dwPosAdvise, &dwSyncPos, &dwPrevLimit);

   pInst->ulPlayTID  = 0L;                     // clear before exit or return

   if (pInst->wPlayNotify != MCI_WAIT)
   {
      if (rc)
         wNotifyType = (WORD) rc;

      /* inform MDM that play has terminated */
      if (pInst->wPlayNotify == MCI_NOTIFY)
         mdmDriverNotify(pInst->wDeviceID, pInst->hPlayCallback, MM_MCINOTIFY,
                      pInst->wPlayUserParm, MAKEULONG(MCI_PLAY, wNotifyType));

      /* inform MCD that play has terminated */
      pInst->pCDMCDReturn(pInst->dwCDMCDID, MAKEULONG(MCI_PLAY, wNotifyType),
                          0L);               //MCD ignores 3rd field for PLAY
      pInst->usPlayFlag = TIMER_AVAIL;
      DosExit(0L, 0L);
   }  /* of if notify was used */

   if (!rc && wNotifyType == MCI_NOTIFY_SUCCESSFUL)
      pInst->dwCurPos = pInst->dwEndPos;

   pInst->usPlayFlag = TIMER_AVAIL;

   return(rc);

}  /* of CD01_Timer() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CD01_TimerNotify                                       */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD Timer Notify                                       */
/*                                                                          */
/* FUNCTION:  Sets up the wait value to suspend the timer thread based on   */
/*            upcoming events.  It also sends notification of past or soon  */
/*            to pass events.                                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst       -- Instance pointer.                              */
/*      DWORD *pPosAdvise -- Position for next Position Advise Event.       */
/*      DWORD *pSyncPos   -- Position for next Sync Pulse if master.        */
/*      DWORD *pPrevLimit -- Last range checked.                            */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:  It is important to note that events are in MM Time and delay     */
/*         factors are in milliseconds.                                     */
/*                                                                          */
/****************************************************************************/
static DWORD CD01_TimerNotify(PINST pInst, DWORD *pPosAdvise,
                              DWORD *pSyncPos, DWORD *pPrevLimit)
{
   DWORD dwDelay, dwUpperLimit, dwEvent;
   LONG lTemp;
   int i;

   /* find wait value */
   if (pInst->usPlayFlag == TIMER_PLAY_SUSPEND ||
       pInst->usPlayFlag == TIMER_PLAY_SUSP_2)
      dwDelay = (DWORD) WAIT_FOREVER;
   else
   {  /* find minimum event */
      dwDelay = WAIT_TIME_MAX;
      dwUpperLimit = pInst->dwCurPos + HALF_TIME_MIN * 3; // *3 is ms to mmtime

      /* process cuepoints */
      for (i=0; i < CDMCD_CUEPOINT_MAX; i++)
      {
         dwEvent = pInst->arrCuePoint[i].dwEvent;

         /* skip if no event */
         if (dwEvent == (DWORD) -1L)
            continue;

         /* check to see if event is in window */
         if (dwEvent >= *pPrevLimit && dwEvent < dwUpperLimit)
         {
            mdmDriverNotify(pInst->wDeviceID,
                            (HWND)pInst->arrCuePoint[i].dwCallback,
                            MM_MCICUEPOINT,
                            pInst->arrCuePoint[i].wUserParm, dwEvent);
            continue;
         }
                                             // there is an implied ELSE here
         lTemp = (LONG)(dwEvent - pInst->dwCurPos) / 3;
         if (lTemp > 0 && (DWORD)lTemp < dwDelay)
            dwDelay = (DWORD)lTemp;

      }  /* of cuepoint for loop */

      /* process position advise */
      if (*pPosAdvise)
      {
         /* loop for small advise frequencies */
         do
         {
            lTemp = (LONG)(*pPosAdvise - pInst->dwCurPos) / 3;
            if (lTemp < HALF_TIME_MIN)
            {
               mdmDriverNotify(pInst->wDeviceID,
                               (HWND)pInst->qptPosAdvise.dwCallback,
                               MM_MCIPOSITIONCHANGE,
                               pInst->qptPosAdvise.wUserParm,
                               (DWORD) *pPosAdvise);
               *pPosAdvise += pInst->qptPosAdvise.dwEvent;
            }
         }  while (lTemp < HALF_TIME_MIN);

         if ((DWORD)lTemp < dwDelay)
            dwDelay = (DWORD)lTemp;
      }  /* of if process advise event pending */

      /* process sync pulse */
      if (pInst->StreamMaster)
      {
         if (*pSyncPos)
         {
            lTemp = (LONG)(*pSyncPos - pInst->dwCurPos) / 3;
            if (lTemp < HALF_TIME_MIN)
            {
               mdmSyncNotify(pInst->pSyncInst, pInst->GroupID,
                             pInst->dwCurPos, 0);
               *pSyncPos = pInst->dwCurPos + SYNC_TOLERANCE * 2 * 3;
            }
         }
         else      // new event just started
            *pSyncPos = pInst->dwCurPos + SYNC_TOLERANCE * 2 * 3;
      }  /* of if position advise is set */
      else
         *pSyncPos = 0L;

      *pPrevLimit = dwUpperLimit;

   }  /* of else find minimum time to wait */

   return(dwDelay);

}  /* of CD01_TimerNotify() */



/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  CallIOCtl                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Call DosDevIOCTL()                                    */
/*                                                                          */
/* FUNCTION:  Call the DosDevIOCTLM Time to Redbook address.                */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      PINST pInst      -- Instance pointer.                               */
/*      ULONG ulCat      -- Category.                                       */
/*      ULONG ulFunction -- Function number.                                */
/*      PVOID pParam     -- pointer to Parameter array.                     */
/*      ULONG ulPLen     -- Parameter array length.                         */
/*      ULONG *pPLen     -- pointer to Parameter array length.              */
/*      PVOID pData      -- pointer to Data array.                          */
/*      ULONG ulDLen     -- Data array length.                              */
/*      ULONG *pDLen     -- pointer to Data array length.                   */
/*                                                                          */
/* EXIT CODES:                                                              */
/*      MCIERR_SUCCESS   -- action completed without error.                 */
/*      MCIERR_DEVICE_NOT_READY      -- device was not ready, no disc.      */
/*                                                                          */
/* NOTES:  DosDevIOCTL sometimes fail when called by a second thread.  To   */
/*      avoid this, a generated thread should have at least a 16K stack     */
/*      size.                                                               */
/*                                                                          */
/****************************************************************************/
static DWORD CallIOCtl(PINST pInst,  ULONG ulCat,  ULONG ulFunction,
                       PVOID pParam, ULONG ulPLen, ULONG *pPLen,
                       PVOID pData,  ULONG ulDLen, ULONG *pDLen)
{
   DWORD rc;

   DosRequestMutexSem(pInst->hIOSem, (ULONG)-1L);
   rc = DosDevIOCtl(pInst->hDrive, ulCat, ulFunction,
                    pParam, ulPLen, pPLen,
                    pData,  ulDLen, pDLen);
   DosReleaseMutexSem(pInst->hIOSem);

   switch(rc)
   {
      case 0L :
         rc = MCIERR_SUCCESS;
         break;
      case 0xFF06 :
      case 0xFF08 :
         rc = MCIERR_OUTOFRANGE;
         break;
      case 0xFF10 :
         rc = MCIERR_MEDIA_CHANGED;
         break;
      default :
         rc = MCIERR_DEVICE_NOT_READY;
   }

   return(rc);

}  /* of CallIOCtl() */



