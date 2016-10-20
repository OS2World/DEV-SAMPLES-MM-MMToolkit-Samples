/*************************START OF SPECIFICATIONS *************************/
/* SOURCE FILE NAME:  IOCODEC.C                                           */
/*                                                                        */
/* DESCRIPTIVE NAME: File Format IO Proc codec interfaces                 */
/*                                                                        */
/* COPYRIGHT:     IBM - International Business Machines                   */
/*            Copyright (c) IBM Corporation  1991, 1992, 1993             */
/*                        All Rights Reserved                             */
/*                                                                        */
/* STATUS: OS/2 Release 2.0                                               */
/*                                                                        */
/* FUNCTION: This source module contains the routine.                     */
/*                                                                        */
/* NOTES:                                                                 */
/*    DEPENDENCIES: none                                                  */
/*    RESTRICTIONS: Runs in 32 bit protect mode (OS/2 2.0)                */
/*                                                                        */
/* ENTRY POINTS:                                                          */
/*     ioDetermineCodec                                                   */
/*     ioLoadCodecDLL                                                     */
/*     ioLoadCodec                                                        */
/*     ioFindCodec                                                        */
/*     ioCloseCodec                                                       */
/*     ioInitCodecopen                                                    */
/*     ioAssociateCodec                                                   */
/*                                                                        */
/************************* END OF SPECIFICATIONS **************************/


#include        <stdio.h>
#include        <string.h>
#include        <stdlib.h>
#include        <memory.h>

#define         INCL_DOS                      /* #define  INCL_DOSPROCESS  */
#define         INCL_ERRORS
#define         INCL_WIN
#define         INCL_GPI
#include        <os2.h>                       /* OS/2 headers              */
#include        <pmbitmap.h>

#define         INCL_OS2MM
#define         INCL_MMIO_CODEC
#define         INCL_MMIO_DOSIOPROC
#include        <os2me.h>                     /* Multi-Media IO extensions */
#include        <hhpheap.h>
#include        <ioi.h>


   extern HHUGEHEAP hheap;                        /* Heap of memory.        */
   extern CODECINIFILEINFO acifiTable[];
   extern ULONG    ulNumColors;                   /* added to correct multiinstance problem */
   extern HMTX hmtxGlobalHeap;


/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ioDetermineCodec                                        */
/*                                                                          */
/* DESCRIPTIVE NAME: Determine which codec should be loaded for this        */
/*                   FOURCC, COMPRESSTYPE, CAPSFLAGS, ColorDepth,           */
/*                   COMPRESS or DECOMPRESS.                                */
/*                                                                          */
/* FUNCTION: This function Determines the CODEC to load.                    */
/*                                                                          */
/* NOTES: Picks the default if there is one.                                */
/*                                                                          */
/* ENTRY POINT: ioDetermineCodec                                            */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*              PINSTANCE  pinstance                                        */
/*              ULONG      ulSearchFlags                                    */
/*              PCODECINIFILEINFO pcifi                                     */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              pcifi  contains found codec info                            */
/*              rc = MMIO_SUCCESS                                           */
/* EXIT-ERROR:                                                              */
/*              MMIO_ERROR                                                  */
/*              MMIOERR_CODEC_NOT_SUPPORTED                                 */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
LONG ioDetermineCodec ( PINSTANCE pinstance,
                        ULONG ulSearchFlags,
                        PCODECINIFILEINFO pcifi )

{
   LONG              rc = MMIO_SUCCESS;  /* Return code of IOProc's call. */
   USHORT            i;                  /* Loop index.       */
   ULONG             ulFlags;
   HPS               hps;                /* only used to query color support */
   HAB               hab;                /* anchor block */
   HMQ               hmq;                /* anchor block */


   if (pcifi->ulCapsFlags & CODEC_DECOMPRESS) {
      /* Query the display mode */
      if (ulNumColors == 0) {       /* Get this info once per process */
         hab  = WinInitialize(0);
//       hmq  = WinCreateMsgQueue( hab, 0L );

         hps  = WinGetPS(HWND_DESKTOP);
         DevQueryCaps ( GpiQueryDevice(hps),
                        CAPS_COLORS,
                        1L,
                        (PLONG)&ulNumColors);

         WinReleasePS (hps);
//       WinDestroyMsgQueue( hmq );
         WinTerminate (hab);
         }

      /* Set the color depth for the CODEC we want */
      if (ulNumColors == 16)
         pcifi->ulCapsFlags |= CODEC_4_BIT_COLOR;
      else if (ulNumColors > 256)
         pcifi->ulCapsFlags |= CODEC_16_BIT_COLOR;
      else  /* 256 and anything else */
         pcifi->ulCapsFlags |= CODEC_8_BIT_COLOR;
      }

   /***************************************************************************/
   /* Search for the DEFAULT codec of this type from the MMIO INI file        */
   /***************************************************************************/
   pcifi->ulCapsFlags |= CODEC_DEFAULT;   /* Pick default */
   ulFlags = ulSearchFlags |
             MMIO_MATCHFOURCC |
             MMIO_MATCHCOMPRESSTYPE |
             MMIO_MATCHCAPSFLAGS |
             MMIO_MATCHFIRST |
             MMIO_FINDPROC;

   if (!(rc = mmioIniFileCODEC(pcifi,ulFlags))) {
      return(MMIO_SUCCESS);
      }

   /***************************************************************************/
   /* If no default, find first one and use it from the MMIO INI file         */
   /***************************************************************************/
   pcifi->ulCapsFlags &= ~CODEC_DEFAULT;
   ulFlags = ulSearchFlags |
             MMIO_MATCHFOURCC |
             MMIO_MATCHCOMPRESSTYPE |
             MMIO_MATCHCAPSFLAGS |
             MMIO_MATCHFIRST |
             MMIO_FINDPROC;

   /* Match the fourcc, compress type, caps flags */
   if (!(rc = mmioIniFileCODEC(pcifi,ulFlags))) {
      return(MMIO_SUCCESS);
      }

   /***************************************************************************/
   /* Search any internal CODEC tables for the necessary CODEC to load.       */
   /* Note: This is used for debugging new CODEC's that have not been added   */
   /*       to the mmpmmmio.ini file.                                         */
   /***************************************************************************/
   for (i = 0; i < NUMBER_CODEC_TABLE; i++) {

      if ((acifiTable[i].ulCompressType == pcifi->ulCompressType) &&
          ((acifiTable[i].ulCapsFlags & pcifi->ulCapsFlags) == pcifi->ulCapsFlags)) {

         *pcifi = acifiTable[i];         /* Copy contents */
         return(MMIO_SUCCESS);
         }
      }


   return(MMIOERR_CODEC_NOT_SUPPORTED);
}



/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME:  ioLoadCodecDLL                                         */
/*                                                                          */
/* DESCRIPTIVE NAME: Load a CODEC IO Proc and add it the PCCB list          */
/*                                                                          */
/* FUNCTION: This function loads a CODEC IO Proc and adds it to the linked  */
/*           list of CODECs currently loaded for this movie instance.       */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ioLoadCodecDLL                                              */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*             PINSTANCE pinstance     - Instant structure.                 */
/*             PCODECINIFILEINFO pcifi - CODEC ini file information         */
/*             PULONG phCodec          - Returns hCodec of any sibling codec*/
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              pccb                                                        */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*              0L                                                          */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
PCCB ioLoadCodecDLL ( PINSTANCE pinstance,
                      PCODECINIFILEINFO pcifi,
                      PULONG phCodec )

{
   LONG      rc = MMIO_SUCCESS;           /* Return code of IOProc's call. */
   SZ        szLoadError[260];            /* Error returns.          */
   PCCB      pccbNew;
   PCCB      pccb;
   HMODULE   hmod = 0L;
   PMMIOPROC pmmioproc;
   ULONG     hCodec = 0L;


   /**************************************************************************/
   /* Search if the CCB entry has been created for the passed in             */
   /* pszDLLName and pszProcName,if yes, then sets pccb    pointer of        */
   /* ptracki to that node.(different track may share a same CCB)            */
   /**************************************************************************/
   for (pccb = pinstance->pccbList; pccb; pccb = pccb->pccbNext) {

      if (!stricmp(pcifi->szDLLName,pccb->cifi.szDLLName)) {
         hCodec = pccb->hCodec;

         if (!stricmp(pcifi->szProcName,pccb->cifi.szProcName)) {
            /* Proc entry names match */
            return(pccb);
            }
         }
      } /* end loop */


   /**************************************************************************/
   /* the above searching can't find the DCIO node, if a same DLLName was    */
   /* found, query IOProc address directly, else load module then query I/O  */
   /* address before allocates a new pccb node.                              */
   /**************************************************************************/
   if (DosLoadModule(szLoadError,
                     (ULONG)sizeof(szLoadError),
                     pcifi->szDLLName,
                     &hmod))   {
      /* Load Error - MMIOERR_INVALID_DLLNAME */
      return(NULL);
      }

   if (DosQueryProcAddr(hmod,
                        0L,
                        pcifi->szProcName,
                        (PFN *)&pmmioproc))   {
      /* Query Error - MMIOERR_INVALID_PROCEDURENAME */
      return(NULL);
      }

   /**************************************************************************/
   /* The above loading and querying was successful, then create a new node  */
   /* for the DCIO. If HhpAllocMem fails, call DosFreeModule to free DCIO    */
   /* module before returning error.                                         */
   /**************************************************************************/
   if (ENTERCRIT(rc)) {
      return(NULL);
      }

   pccbNew = (PCCB)HhpAllocMem(hheap,(ULONG)sizeof(CCB));

   EXITCRIT;

   if(!pccbNew) {
      DosFreeModule(hmod);
      /* Allocate error - MMIOERR_OUTOFMEMORY */
      return(NULL);
      }

   /**************************************************************************/
   /* Assigns the Decompress IOProc information, which is in record map      */
   /* table, to the new DCIO node.                                           */
   /**************************************************************************/
   pccbNew->cifi = *pcifi;
   pccbNew->hmodule = hmod;
   pccbNew->pmmioproc = pmmioproc;
   pccbNew->hCodec = 0L;
   pccbNew->ulLastSrcBuf = 0L;
   pccbNew->ulMisc1 = 0L;
   pccbNew->ulMisc2 = 0L;

   /**************************************************************************/
   /* adds new node to the beginning of ccb list.                            */
   /**************************************************************************/
   pccbNew->pccbNext = pinstance->pccbList;
   pinstance->pccbList = pccbNew;

   *phCodec = hCodec;
   return(pccbNew);
}



/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME:  ioLoadCodec                                            */
/*                                                                          */
/* DESCRIPTIVE NAME: Load a CODEC IO Proc and add it the PCCB list          */
/*                                                                          */
/* FUNCTION: This function loads a CODEC IO Proc and adds it to the linked  */
/*           list of CODECs currently loaded for this movie instance.       */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ioLoadCodec                                                 */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*             PINSTANCE pinstance     - Instant structure.                 */
/*             PTRACKI ptracki         - Track specific information         */
/*             PCODECINIFILEINFO pcifi - CODEC ini file information         */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              pccb                                                        */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*              0L                                                          */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
PCCB ioLoadCodec ( PINSTANCE pinstance,
                   PTRACKI ptracki,
                   PCODECINIFILEINFO pcifi )

{
   LONG      rc = MMIO_SUCCESS;           /* Return code of IOProc's call. */
   PCCB      pccbNew;
   ULONG     hCodec = 0L;



   if (pccbNew = ioLoadCodecDLL(pinstance,pcifi,&hCodec)) {

      /**************************************************************************/
      /* Open the Codec IO Proc and save the hCodec in the pccb structure       */
      /**************************************************************************/
      if (rc = ffOpenCodec(pinstance, pccbNew, hCodec, ptracki)) {

         pinstance->pccbList = pccbNew->pccbNext;   /* unlink from list */
         ioCloseCodec(pccbNew);
         pccbNew = NULL;  /* return error condition */
         }
      }

   return(pccbNew);
}



/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME:  ioFindCodec                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:                                                        */
/*                                                                          */
/* FUNCTION: This function finds a compression/decompression control block  */
/*           for this compression type.                                     */
/*                                                                          */
/* NOTES: None                                                              */
/*                                                                          */
/* ENTRY POINT: ioFindCodec                                                 */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*           PINSTANCE   pinstance  - Movie instance structure              */
/*           ULONG       ulCompressType - Compression type                  */
/*                                                                          */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              pccb                                                        */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*              NULL - 0L                                                   */
/*                                                                          */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
PCCB ioFindCodec ( PINSTANCE pinstance,
                   ULONG ulCompressType )

{
   PCCB   pccb;

   for (pccb = pinstance->pccbList; pccb; pccb = pccb->pccbNext) {
      if (pccb->cifi.ulCompressType == ulCompressType)
         return(pccb);
      }
   return(NULL);  /* not found */
}



/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME:  ioCloseCodec                                           */
/*                                                                          */
/* DESCRIPTIVE NAME:                                                        */
/*                                                                          */
/* FUNCTION: This function Closes a codec instance for a movie instance.    */
/*           This is called upon a unrecoverable error or on movie close.   */
/*                                                                          */
/* NOTES: None                                                              */
/*                                                                          */
/* ENTRY POINT: ioCloseCodec                                                */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*           PCCB        pccb       - Pointer to codec control structure.   */
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
LONG ioCloseCodec ( PCCB pccb )

{
   LONG       rc = MMIO_SUCCESS;           /* Return code of IOProc's call. */

   ENTERCRITX;
   if (pccb->codecopen.pSrcHdr) {
      HhpFreeMem(hheap,(PVOID)pccb->codecopen.pSrcHdr);
      }

   if (pccb->codecopen.pDstHdr) {
      HhpFreeMem(hheap,(PVOID)pccb->codecopen.pDstHdr);
      }

   if (pccb->codecopen.pControlHdr) {
      HhpFreeMem(hheap,(PVOID)pccb->codecopen.pControlHdr);
      }

   if (pccb->codecopen.pOtherInfo) {
      HhpFreeMem(hheap,(PVOID)pccb->codecopen.pOtherInfo);
      }

   if (pccb->hCodec) {
      rc = pccb->pmmioproc(&pccb->hCodec,
                           MMIOM_CODEC_CLOSE,
                           0L,
                           0L);

      if (!rc) {
         pccb->hCodec = 0L;
         }
      }

   if (pccb->hmodule) {
//----DosFreeModule(pccb->hmodule);
      pccb->hmodule = 0;
      }

   HhpFreeMem(hheap,(PVOID)pccb);
   pccb = NULL;
   EXITCRIT;

   return(rc);
}


/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ioInitCodecopen                                         */
/*                                                                          */
/* DESCRIPTIVE NAME: Allocate and initialize a CODECOPEN structure to be    */
/*                   saved in the CCB. Copy info from input CODECOPEN.      */
/*                                                                          */
/* FUNCTION: This function allocates a CODECOPEN structure.                 */
/*                                                                          */
/* NOTES: None                                                              */
/*                                                                          */
/* ENTRY POINT: ioInitCodecopen                                             */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*              PCCB          pccb                                          */
/*              PCODECOPEN    pcodecopen                                    */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              rc = MMIO_SUCCESS                                           */
/* EXIT-ERROR:                                                              */
/*              MMIO_ERROR                                                  */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
LONG ioInitCodecopen ( PCCB pccb,
                       PCODECOPEN pcodecopen)

{
   ULONG             ulSize;


   ENTERCRITX;
   pccb->codecopen.ulFlags = pcodecopen->ulFlags;

   /* Create and copy Pointers to structures in CODECOPEN structure */
   if (pcodecopen->pControlHdr) {
      ulSize = *((PULONG)pcodecopen->pControlHdr);
      if (!(pccb->codecopen.pControlHdr = (PVOID)HhpAllocMem(hheap,ulSize))) {
         return(MMIO_ERROR);
         }
      memcpy(pccb->codecopen.pControlHdr, pcodecopen->pControlHdr, ulSize);
      }

   if (pcodecopen->pSrcHdr) {
      ulSize = *((PULONG)pcodecopen->pSrcHdr);
      if (!(pccb->codecopen.pSrcHdr = (PVOID)HhpAllocMem(hheap,ulSize))) {
         return(MMIO_ERROR);
         }
      memcpy(pccb->codecopen.pSrcHdr, pcodecopen->pSrcHdr, ulSize);
      }

   if (pcodecopen->pDstHdr) {
      ulSize = *((PULONG)pcodecopen->pDstHdr);
      if (!(pccb->codecopen.pDstHdr = (PVOID)HhpAllocMem(hheap,ulSize))) {
         return(MMIO_ERROR);
         }
      memcpy(pccb->codecopen.pDstHdr, pcodecopen->pDstHdr, ulSize);
      }

   if (pcodecopen->pOtherInfo) {
      ulSize = *((PULONG)pcodecopen->pOtherInfo);
      if (!(pccb->codecopen.pOtherInfo = (PVOID)HhpAllocMem(hheap,ulSize))) {
         return(MMIO_ERROR);
         }
      memcpy(pccb->codecopen.pOtherInfo, pcodecopen->pOtherInfo, ulSize);
      }

   EXITCRIT;
   return(MMIO_SUCCESS);
}


/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ioAssociateCodec                                        */
/*                                                                          */
/* DESCRIPTIVE NAME: Associate CODECs with a file(track). Use this codec    */
/*                   to compress frames to be written to a file.            */
/*                                                                          */
/* FUNCTION: This function associates a CODEC with a file (track).          */
/*                                                                          */
/* NOTES: None                                                              */
/*                                                                          */
/* ENTRY POINT: ioAssociateCodec                                            */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*              PINSTANCE  pinstance                                        */
/*              PMMEXTENDEDINFO pmmextendedinfo                             */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              rc = MMIO_SUCCESS                                           */
/* EXIT-ERROR:                                                              */
/*              MMIO_ERROR                                                  */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*              Codec is added to CCB link-list and the codecs are opened.  */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
LONG ioAssociateCodec ( PMMIOINFO pmmioinfo,
                        PINSTANCE pinstance,
                        PCODECASSOC pcodecassoc )

{
   LONG              rc = MMIO_SUCCESS;  /* Return code of IOProc's call. */
   PCCB              pccb;
   ULONG             hCodec;  /* Possibly returned from ioLoadCodecDLL */

   /* Check for NULL pointers */
   if (!pcodecassoc->pCodecOpen || !pcodecassoc->pCODECIniFileInfo) {
      return (MMIOERR_INVALID_PARAMETER);
      }

   /* Force the correct values into the codecinfileinfo structure */
   pcodecassoc->pCODECIniFileInfo->ulStructLen = sizeof(CODECINIFILEINFO);
   pcodecassoc->pCODECIniFileInfo->fcc = pmmioinfo->fccIOProc;
   pcodecassoc->pCODECIniFileInfo->ulCapsFlags |= CODEC_COMPRESS;       /* Set this one */

   /* Find the codec to load */
   if (rc = ioDetermineCodec(pinstance, 0, pcodecassoc->pCODECIniFileInfo)) {
      return(rc); /* return error */
      }

   else { /* load and open the compression codec */

      /***********************************************/
      /* Check for previously installed codecs.      */
      /* de-installed any loaded, load new one       */
      /* allows only 1 codec to be loaded at a time  */
      /***********************************************/
      if (pinstance->pccbList) {
         pccb = pinstance->pccbList;
         pinstance->pccbList = pccb->pccbNext;   /* unlink from list */
         ioCloseCodec(pccb);
         }

      /* Load the codec dll */
      if (pccb = ioLoadCodecDLL(pinstance,
                                pcodecassoc->pCODECIniFileInfo,
                                &hCodec)) {

         /* Save the codec open information in the ccb */
         ((PCODECOPEN)pcodecassoc->pCodecOpen)->ulFlags |= CODEC_COMPRESS;    /* Force open of compressor */

         if (!(rc = ioInitCodecopen(pccb,(PCODECOPEN)pcodecassoc->pCodecOpen))) {

            /* Open the codec */
            if (!(rc = pccb->pmmioproc(&hCodec,
                                       MMIOM_CODEC_OPEN,
                                       (LONG)&pccb->codecopen,
                                       0L)))  {
               pccb->hCodec = hCodec;       /* save handle to codec */
               }
            }

         /* handle error conditions */
         if (rc) {
            pinstance->pccbList = pccb->pccbNext;   /* unlink from list */
            ioCloseCodec(pccb);
            }
         }
      else {
         rc = MMIO_ERROR;
         }
      }
   return(rc);
}
