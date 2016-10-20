/*************************START OF SPECIFICATIONS *************************/
/* SOURCE FILE NAME:  IOOPEN.C                                            */
/*                                                                        */
/* DESCRIPTIVE NAME: File Format IO Proc routine for MMIOM_OPEN.          */
/*                                                                        */
/* COPYRIGHT:     IBM - International Business Machines                   */
/*            Copyright (c) IBM Corporation  1991, 1992, 1993             */
/*                        All Rights Reserved                             */
/*                                                                        */
/* STATUS: OS/2 Release 2.0                                               */
/*                                                                        */
/* FUNCTION: This source module contains the open routine, and other      */
/*           routines that support IOProcOpen.                            */
/* NOTES:                                                                 */
/*    DEPENDENCIES: none                                                  */
/*    RESTRICTIONS: Runs in 32 bit protect mode (OS/2 2.0)                */
/*                                                                        */
/* ENTRY POINTS:                                                          */
/*     IOProcOpen                                                         */
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
#include        <os2me.h>                      /* Multi-Media IO extensions.*/
#include        <hhpheap.h>
#include        <ioi.h>

   extern HHUGEHEAP hheap;
   extern HMTX hmtxGlobalHeap;

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME:  IOProcOpen                                             */
/*                                                                          */
/* DESCRIPTIVE NAME: open Digital Video file.                               */
/*                                                                          */
/* FUNCTION: This function opens digital video file, allocates instance     */
/*           structure, reads header, calls file format specific open       */
/*           routine to init the track information.                         */
/*                                                                          */
/* NOTES: None                                                              */
/*                                                                          */
/* ENTRY POINT: IOProcOpen                                                  */
/*   LINKAGE:   CALL FAR (00:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*              PMMIOINFO pmmioinfo - Pointer to MMIOINFO staus structure.  */
/*              PSZ     pszFileName - Name of file to be opened.            */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*              MMIO_SUCCESS                                                */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*              MMIO_ERROR                                                  */
/*              MMIOERR_INVALID_ACCESS_FLAG                                 */
/*              MMIOERR_OUTOFMEMORY                                         */
/*              io proc specific error                                      */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

LONG IOProcOpen (PMMIOINFO pmmioinfo, PSZ pszFileName) {
   LONG            rc = MMIO_SUCCESS;               /* Return code.         */
   LONG            lFilePosition;                   /* Logical file position*/
   MMIOINFO        Localmmioinfo;                   /* For locally used.    */
   PINSTANCE       pinstance;                       /* Local work structure.*/

   if (pmmioinfo == NULL) return MMIO_ERROR;

   if (CheckMem((PVOID)pmmioinfo, sizeof(MMIOINFO), PAG_WRITE))
      return MMIO_ERROR;

   /**************************************************************************/
   /* Validate the open flags for this File Format IO Proc                   */
   /* (INVALID_OPEN_FLAGS should be defined in the ff.h - file format        */
   /*  specific header file.)                                                */
   /**************************************************************************/

   if (pmmioinfo->ulFlags  & INVALID_OPEN_FLAGS) {
       pmmioinfo->ulErrorRet = MMIOERR_INVALID_ACCESS_FLAG;
       return(MMIO_ERROR);
   }

   ENTERCRITX;
   if ((pinstance = (PINSTANCE)HhpAllocMem(hheap,sizeof(INSTANCE))) == NULL) {
       EXITCRIT;
       pmmioinfo->ulErrorRet = MMIOERR_OUTOFMEMORY;
       return(MMIO_ERROR);                         /* Allocate work struct. */
   }
   EXITCRIT;

   pmmioinfo->pExtraInfoStruct = (PVOID) pinstance;
   pmmioinfo->fccIOProc = HEX_FOURCC_FFIO;            /* Make sure this is set for codec loading */
   ioInstanceInit(pinstance);

   // Validate read flags before doing read initialization

   if (( pmmioinfo->ulFlags  & MMIO_READ ) &&
                !( pmmioinfo->ulFlags & INVALID_READ_FLAGS )) {

      // IOProc identifies Storage System

      memcpy (&Localmmioinfo, pmmioinfo, sizeof(MMIOINFO));
      Localmmioinfo.pIOProc = NULL;
      Localmmioinfo.fccIOProc = pmmioinfo->fccChildIOProc;
      Localmmioinfo.ulFlags |= MMIO_NOIDENTIFY; // Eliminate callbacks
      Localmmioinfo.ulFlags &= ~MMIO_ALLOCBUF;  // Force non-buffered open

      rc = ioIdentifyStorageSystem(&Localmmioinfo,pszFileName);

      if (rc != MMIO_SUCCESS) {        /* if error,                  */
         ioCleanUp(pmmioinfo);
         return(rc);
      }

      /****************************************************************************/
      /* Allocate memory for pTempBuffer which is used when IOProcReadInterLeaved */
      /* is called.                                                               */
      /****************************************************************************/

      if (ENTERCRIT(rc)) {
         ioCleanUp(pmmioinfo);
         return(rc);
      }

      if ((pinstance->pTempBuffer = HhpAllocMem(hheap, DEFAULTBUFFERSIZE)) == NULL) {
         EXITCRIT;
         ioCleanUp(pmmioinfo);
         return(MMIOERR_OUTOFMEMORY);
      }
      EXITCRIT;
      pinstance->ulTempBufferSize = DEFAULTBUFFERSIZE;

      /**************************************************************************/
      /* Open Movie file                                                        */
      /**************************************************************************/

      if ( pmmioinfo->fccChildIOProc != FOURCC_MEM ) {
         Localmmioinfo.cchBuffer = 0;
         Localmmioinfo.pchBuffer = NULL;
      }
      pinstance->hmmioFileHandle = mmioOpen(pszFileName,&Localmmioinfo,MMIO_NOIDENTIFY);
      if (pinstance->hmmioFileHandle <= (HMMIO)0L) {    /* Test file open error.*/
         rc = Localmmioinfo.ulErrorRet;
      }

      /**************************************************************************/
      /* Call file format specific open routine                                 */
      /**************************************************************************/

      else if (!(rc = ffOpenRead(pmmioinfo, pinstance))) {
         if(!(rc = ioAddTracksToMovieHeader(pinstance))) {

            /**************************************************************************/
            /* Set lLogicalFilePos to a position pass the header block to allow       */
            /* read occurring at the first byte of non-header data.                   */
            /**************************************************************************/
            lFilePosition = ffSeekToDataBegin(pmmioinfo,pinstance);
            if (lFilePosition < MMIO_SUCCESS)
               rc = MMIO_ERROR;
            else
               pinstance->lFileCurrentPosition = lFilePosition;
         }
      }

      if (rc) {
         ioCleanUp(pmmioinfo);
         return(rc);
      }
   }

   // Validate Write flags before doing initialization

#ifndef WORKSHOP

   if ((pmmioinfo->ulFlags & (MMIO_READWRITE | MMIO_WRITE)) &&
                !(pmmioinfo->ulFlags & INVALID_WRITE_FLAGS)) {

      // Open the movie file

      memset (&Localmmioinfo, '\0', sizeof(MMIOINFO));
      Localmmioinfo.pIOProc   = NULL;
      Localmmioinfo.fccIOProc = pmmioinfo->fccChildIOProc;

      if (pmmioinfo->fccChildIOProc != FOURCC_MEM) {
         Localmmioinfo.cchBuffer = 0;
         Localmmioinfo.pchBuffer = NULL;
      }

      Localmmioinfo.ulFlags |= MMIO_NOIDENTIFY; // Eliminate callbacks
      Localmmioinfo.ulFlags &= ~MMIO_ALLOCBUF;  // Force non-buffered open.  MMIO May do buffering.

      pinstance->hmmioFileHandle = mmioOpen(pszFileName, &Localmmioinfo,
                                            MMIO_READWRITE | MMIO_NOIDENTIFY);

      if (pinstance->hmmioFileHandle <= (HMMIO)0L) // Test file open error.
         rc = Localmmioinfo.ulErrorRet;
      else
         rc = ffOpenWrite(pmmioinfo, pinstance);   // call file format specific open routine

      if (rc != 0) {
         ioCleanUp(pmmioinfo);
         return(rc);
      }
   }

#else    // WORKSHOP next

   if ((pmmioinfo->ulFlags & (MMIO_READWRITE | MMIO_WRITE)) &&
                !(pmmioinfo->ulFlags & INVALID_WRITE_FLAGS)) {

      // Open the movie file

      memset (&Localmmioinfo, '\0', sizeof(MMIOINFO));
      Localmmioinfo.pIOProc   = NULL;
      Localmmioinfo.fccIOProc = pmmioinfo->fccChildIOProc;
      Localmmioinfo.ulFlags  = pmmioinfo->ulFlags;
      Localmmioinfo.ulFlags |= MMIO_NOIDENTIFY; // Eliminate callbacks
      Localmmioinfo.ulFlags &= ~MMIO_ALLOCBUF;  // Force non-buffered open.  MMIO May do buffering.

      if (!(pmmioinfo->ulFlags & MMIO_CREATE)) {
         rc = ioIdentifyStorageSystem(&Localmmioinfo, pszFileName);

         if (rc != MMIO_SUCCESS) {        // if error
            pmmioinfo->ulErrorRet = rc;   // see IdentifyStorageSystem
            ioCleanUp(pmmioinfo);
            return(MMIO_ERROR);
         }

         // Allocate memory for pTempBuffer which is used when
         // IOProcReadInterLeaved is called.

         if (ENTERCRIT(rc)) {
            ioCleanUp(pmmioinfo);
            return MMIO_ERROR;
         }

         pinstance->pTempBuffer = HhpAllocMem(hheap, DEFAULTBUFFERSIZE);
         if (pinstance->pTempBuffer == NULL) {
            EXITCRIT;
            pmmioinfo->ulErrorRet = MMIOERR_OUTOFMEMORY;
            ioCleanUp(pmmioinfo);
            return MMIO_ERROR;
         }
         EXITCRIT;

         pinstance->ulTempBufferSize = DEFAULTBUFFERSIZE;
      }

      pinstance->lFileCurrentPosition = 0;

      pinstance->hmmioFileHandle = mmioOpen(pszFileName, &Localmmioinfo, Localmmioinfo.ulFlags);

      if (pinstance->hmmioFileHandle <= (HMMIO)0L) // Test file open error.
         rc = Localmmioinfo.ulErrorRet;
      else {
         rc = ffOpenWrite(pmmioinfo, pinstance);   // call file format specific open routine

         if (rc == 0) {
            if (!(pmmioinfo->ulFlags & MMIO_CREATE)) {
               rc = ioAddTracksToMovieHeader(pinstance);

               if (rc == 0) {

                  // Set lLogicalFilePos to a position pass the header block to allow
                  // read occurring at the first byte of non-header data.

                  lFilePosition = ffSeekToDataBegin(pmmioinfo, pinstance);
                  if (lFilePosition < MMIO_SUCCESS) rc = MMIO_ERROR;
                  else pinstance->lFileCurrentPosition = lFilePosition;
               }
            }
         }
      }

      if (rc != 0) {
         pmmioinfo->ulErrorRet = rc;
         ioCleanUp(pmmioinfo);
         return MMIO_ERROR;
      }
   }

   // set up the pathname in the instance structure

   if (strlen(pszFileName) < CCHMAXPATH) {
      strcpy((PSZ)&(pinstance->szFileName), pszFileName);
      if ((pinstance->szFileName)[1] == ':')
         pinstance->ulEditFlags |= FULLY_QUALIFIED_PATH;
   }

#endif

   return MMIO_SUCCESS;
}
