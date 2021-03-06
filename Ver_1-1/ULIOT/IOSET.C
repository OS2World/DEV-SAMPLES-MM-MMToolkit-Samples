/*************************START OF SPECIFICATIONS *************************/
/* SOURCE FILE NAME:  IOSET.C                                             */
/*                                                                        */
/* DESCRIPTIVE NAME: File Format IO Proc routine for MMIOM_SET            */
/*                                                                        */
/* COPYRIGHT:     IBM - International Business Machines                   */
/*            Copyright (c) IBM Corporation  1991, 1992, 1993             */
/*                        All Rights Reserved                             */
/*                                                                        */
/* STATUS: OS/2 Release 2.0                                               */
/*                                                                        */
/* FUNCTION: This source module contains the set functions.               */
/* NOTES:                                                                 */
/*    DEPENDENCIES: none                                                  */
/*    RESTRICTIONS: Runs in 32 bit protect mode (OS/2 2.0)                */
/*                                                                        */
/* ENTRY POINTS:                                                          */
/*              IOProcSet                                                 */
/*                                                                        */
/************************* END OF SPECIFICATIONS **************************/


#include        <stdio.h>
#include        <string.h>
#include        <stdlib.h>
#include        <memory.h>

#define         INCL_DOS                        /* #define  INCL_DOSPROCESS.*/
#define         INCL_ERRORS
#define         INCL_WIN
#define         INCL_GPI
#include        <os2.h>                         /* OS/2 headers.            */
#include        <pmbitmap.h>

#define         INCL_OS2MM
#define         INCL_MMIO_CODEC
#define         INCL_MMIO_DOSIOPROC

#include        <os2me.h>                     /* Multi-Media IO extensions. */
#include        <hhpheap.h>
#include        <ioi.h>


/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: IOProcSet                                               */
/*                                                                          */
/* DESCRIPTIVE NAME: Set various conditions in IO Proc                      */
/*                                                                          */
/* FUNCTION:                                                                */
/*                                                                          */
/* NOTES: None                                                              */
/*                                                                          */
/* ENTRY POINT: IOProcSet                                                   */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*   PMMIOINFO             pmmioinfo          - ptr to instance structure.  */
/*   LONG                  lParam1  - first parameter                       */
/*   LONG                  lParam2  - Second parameter                      */
/*                                                                          */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              MMIO_SUCCESS                                                */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*              MMIO_ERROR                                                  */
/*                                                                          */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
LONG IOProcSet ( PMMIOINFO pmmioinfo,
                 LONG lParam1,
                 LONG lParam2 )

{
   PINSTANCE       pinstance;
   LONG            rc = MMIO_SUCCESS;
   PMMEXTENDINFO   pmmextendinfo = (PMMEXTENDINFO)lParam1;
   PCCB            pccb;
   ULONG           ulCCBCount;
   PCODECASSOC     pcodecassoc;
   ULONG           ulSize;
   PVOID           PtrNextAvail;

   if (rc = ioGetPtrInstance(pmmioinfo,&pinstance))
      return(rc);

   switch(lParam2){
      /*************************/
      /* SET INFO              */
      /*************************/
      case MMIO_SET_EXTENDEDINFO:   /* Set extended information */
         if (pmmextendinfo) {     /* error check */

            /********************/
            /* Set active track */
            /********************/
            if (pmmextendinfo->ulFlags & MMIO_TRACK) {

               if (pmmextendinfo->ulTrackID == (ULONG)MMIO_RESETTRACKS) {
                  pinstance->lCurrentTrack = pmmextendinfo->ulTrackID;
                  }
               else {
                  if (pinstance->ulFlags & OPENED_READONLY) {
                     if (ioFindTracki(pinstance,pmmextendinfo->ulTrackID)) {
                        pinstance->lCurrentTrack = pmmextendinfo->ulTrackID;
                        }
                     else {
                        pmmioinfo->ulErrorRet = MMIOERR_INVALID_PARAMETER;
                        rc = MMIO_ERROR;
                        break;
                        }
                     }

                  else if (pinstance->ulFlags &  
                          (OPENED_READWRITE | OPENED_WRITECREATE)) {
                     pinstance->lCurrentTrack = pmmextendinfo->ulTrackID;
                     }

                  else {
                     pmmioinfo->ulErrorRet = MMIOERR_INVALID_PARAMETER;
                     rc = MMIO_ERROR;
                     break;
                     }
                  } /* else */
               }  /* MMIO_TRACK */

            /****************************************************************/
            /* Reset all Non-normal reading modes.  All request audio and   */
            /* video frames are returned.                                   */
            /****************************************************************/
            if (pmmextendinfo->ulFlags & MMIO_NORMAL_READ) {
               pinstance->ulMode = MODE_NORMALREAD;
               } /* MMIO_NORMAL_READ */

            /****************************************************************/
            /* Set io proc into SCAN mode for the active track. Reading     */
            /* will now be done, but only Key frames are returned for video */
            /****************************************************************/
            else if (pmmextendinfo->ulFlags & MMIO_SCAN_READ) {
               pinstance->ulMode = MODE_SCANREAD;
               } /* MMIO_SCAN_READ */

            /****************************************************************/
            /* Set io proc into REVERSE mode for the active track. Reading  */
            /* will now be done, but only Key frames are returned for video */
            /****************************************************************/
            else if (pmmextendinfo->ulFlags & MMIO_REVERSE_READ) {
               pinstance->ulMode = MODE_REVERSEREAD;
               } /* MMIO_REVERSE_READ */

            /****************************************************************/
            /* Associate CODEC information for recording                    */
            /****************************************************************/
            if (pmmextendinfo->ulFlags & MMIO_CODEC_ASSOC) {

               /* Don't alow a CODEC association for read only files */
               if (pinstance->ulFlags & OPENED_READONLY) {
                  pmmioinfo->ulErrorRet = MMIOERR_INVALID_PARAMETER;
                  rc = MMIO_ERROR;
                  break;
                  }

               /* Can only associate 1 CODEC for record */
               if (pmmextendinfo->ulNumCODECs == 1) {
                  if (rc = ioAssociateCodec(pmmioinfo,
                                            pinstance,
                                            pmmextendinfo->pCODECAssoc)) {
                     pmmioinfo->ulErrorRet = rc;
                     rc = MMIO_ERROR;
                     }
                  }
               else {
                  pmmioinfo->ulErrorRet = MMIOERR_INVALID_PARAMETER;
                  rc = MMIO_ERROR;
                  }
               } /* MMIO_CODEC_ASSOC */
            } /* pmmextendedinfo */

         else { /* error - data structure missing */
            pmmioinfo->ulErrorRet = MMIOERR_INVALID_PARAMETER;
            rc = MMIO_ERROR;
            }
         break;

      /********************************/
      /* QUERY BASE AND CODEC INFO    */
      /********************************/
      case MMIO_QUERY_EXTENDEDINFO_ALL: /* Query Also CODEC associated info */

         /* Create the array of codecassoc structures to return to caller */
         pcodecassoc = pmmextendinfo->pCODECAssoc;  /* Point to beginning */
         for (pccb = pinstance->pccbList; pccb; pccb = pccb->pccbNext) {
            pcodecassoc->pCodecOpen = NULL;
            pcodecassoc->pCODECIniFileInfo = NULL;
            pcodecassoc++;
            }
         PtrNextAvail = (PVOID)pcodecassoc;

         /* Fill in pointers to the CODECIniFileInfo structures to follow */
         ulSize = 0L;
         pcodecassoc = pmmextendinfo->pCODECAssoc;  /* Point to beginning */
         for (pccb = pinstance->pccbList; pccb; pccb = pccb->pccbNext) {

            /* Create and copy CODECINIFILEINFO structure */
            pcodecassoc->pCODECIniFileInfo = (PCODECINIFILEINFO)PtrNextAvail;
            memcpy(pcodecassoc->pCODECIniFileInfo,&pccb->cifi,sizeof(CODECINIFILEINFO));
            PtrNextAvail = (PVOID) (((ULONG)PtrNextAvail) + sizeof(CODECINIFILEINFO));

            /* Create and copy CODECOPEN structure */
            pcodecassoc->pCodecOpen = PtrNextAvail;
            memcpy(pcodecassoc->pCodecOpen,&pccb->codecopen,sizeof(CODECOPEN));
            PtrNextAvail = (PVOID) (((ULONG)PtrNextAvail) + sizeof(CODECOPEN));

            /* Create and copy Pointers to structures in CODECOPEN structure */
            if (pccb->codecopen.pControlHdr) {
               ulSize = *((PULONG)pccb->codecopen.pControlHdr);
               ((PCODECOPEN)pcodecassoc->pCodecOpen)->pControlHdr = (PVOID)PtrNextAvail;
               memcpy(((PCODECOPEN)pcodecassoc->pCodecOpen)->pControlHdr,
                      pccb->codecopen.pControlHdr,
                      ulSize);
               PtrNextAvail = (PVOID) (((ULONG)PtrNextAvail) + ulSize);
               }

            if (pccb->codecopen.pSrcHdr) {
               ulSize = *((PULONG)pccb->codecopen.pSrcHdr);
               ((PCODECOPEN)pcodecassoc->pCodecOpen)->pSrcHdr = PtrNextAvail;
               memcpy(((PCODECOPEN)pcodecassoc->pCodecOpen)->pSrcHdr,
                      pccb->codecopen.pSrcHdr,
                      ulSize);
               PtrNextAvail = (PVOID) (((ULONG)PtrNextAvail) + ulSize);
               }

            if (pccb->codecopen.pDstHdr) {
               ulSize = *((PULONG)pccb->codecopen.pDstHdr);
               ((PCODECOPEN)pcodecassoc->pCodecOpen)->pDstHdr = PtrNextAvail;
               memcpy(((PCODECOPEN)pcodecassoc->pCodecOpen)->pDstHdr,
                      pccb->codecopen.pDstHdr,
                      ulSize);
               PtrNextAvail = (PVOID) (((ULONG)PtrNextAvail) + ulSize);
               }

            if (pccb->codecopen.pOtherInfo) {
               ulSize = *((PULONG)pccb->codecopen.pOtherInfo);
               ((PCODECOPEN)pcodecassoc->pCodecOpen)->pOtherInfo = PtrNextAvail;
               memcpy(((PCODECOPEN)pcodecassoc->pCodecOpen)->pOtherInfo,
                      pccb->codecopen.pOtherInfo,
                      ulSize);
               PtrNextAvail = (PVOID) (((ULONG)PtrNextAvail) + ulSize);
               }
            pcodecassoc++;
            }

      /*********************************************************/
      /* QUERY BASE INFO  (NOTE:Fall thru from previous case!) */
      /*********************************************************/
      case MMIO_QUERY_EXTENDEDINFO_BASE: /* Query only MMEXTENDINFO info */
         pmmextendinfo->ulStructLen = sizeof(MMEXTENDINFO);
         pmmextendinfo->ulTrackID = (ULONG)pinstance->lCurrentTrack;
         /* pmmextendinfo->pCODECAssoc = NULL;  */

         /* Compute ulBufSize for complete information return */
         ulSize = 0L;
         for (pccb = pinstance->pccbList, ulCCBCount = 0;  /* Count CCB's */
              pccb;
              ulCCBCount++, pccb = pccb->pccbNext) {
            ulSize += sizeof(CODECASSOC)+sizeof(CODECOPEN)+sizeof(CODECINIFILEINFO); /* static stuff */

            /* extract ulStructLen as first field of structure that ptr points to */
            if (pccb->codecopen.pControlHdr) {
               ulSize += *((PULONG)pccb->codecopen.pControlHdr);
               }
            if (pccb->codecopen.pSrcHdr) {
               ulSize += *((PULONG)pccb->codecopen.pSrcHdr);
               }
            if (pccb->codecopen.pDstHdr) {
               ulSize += *((PULONG)pccb->codecopen.pDstHdr);
               }
            if (pccb->codecopen.pOtherInfo) {
               ulSize += *((PULONG)pccb->codecopen.pOtherInfo);
               }
            }

         pmmextendinfo->ulNumCODECs = ulCCBCount;
         pmmextendinfo->ulBufSize = ulSize;
         break;

      /*********/
      /* ERROR */
      /*********/
      default:
         pmmioinfo->ulErrorRet = MMIOERR_INVALID_PARAMETER;
         rc = MMIO_ERROR;
         break;
      }/* end switch */

   return(rc);
}
