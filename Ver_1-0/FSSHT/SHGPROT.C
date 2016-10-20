/*static char *SCCSID = "@(#)shgprot.c	13.2 92/05/01";*/
/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHGPROT.C                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Stream Handler Get Protocol routine                   */
/*                                                                          */
/* FUNCTION: Get the protocols installed for this stream handler.           */
/*                                                                          */
/* ENTRY POINTS: ShcGetProtocol                                             */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
#define  INCL_NOPMAPI                  /* no PM include files required */
#define  INCL_DOSSEMAPHORES
#define  INCL_DOSPROCESS
#include <os2.h>
#include <os2me.h>
#include <hhpheap.h>
#include <shi.h>

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ShcGetProtocol                                          */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Command Get Protocol                    */
/*                                                                          */
/* FUNCTION: This returns a SPCB indicated by the key passed in.            */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShcGetProtocol                                              */
/*   LINKAGE:   CALL NEAR (0:32)                                            */
/*                                                                          */
/* INPUT:                                                                   */
/*                                                                          */
/* EXIT-NORMAL:                                                             */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*                                                                          */
/* INTERNAL REFERENCES:                                                     */
/*        ROUTINES:                                                         */
/*                                                                          */
/* EXTERNAL REFERENCES:                                                     */
/*        ROUTINES:                                                         */
/*        DATA STRUCTURES:                                                  */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

RC ShcGetProtocol(pgpparm)
PPARM_GPROT pgpparm;

{ /* Start of ShcGetProtocol */

RC rc = NO_ERROR;                       // local return code
PESPCB pTempEspcb;

  //
  // the ESPCB list is under semphore control
  //
  if (!(rc = DosRequestMutexSem(hmtxGlobalData, SEM_INDEFINITE_WAIT)))
    { /* obtained semaphore */
      /*
       * Locate the spcb and return it
       */
      pTempEspcb = ShFindEspcb(pgpparm->spcbkey);
      if (pTempEspcb)
        {
          *pgpparm->pspcb = pTempEspcb->spcb;
        }
      else
        {
          rc = ERROR_INVALID_SPCBKEY;
        }

      DosReleaseMutexSem(hmtxGlobalData);

    } /* obtained semaphore */

  return(rc);

} /* End of ShcGetProtocol */
