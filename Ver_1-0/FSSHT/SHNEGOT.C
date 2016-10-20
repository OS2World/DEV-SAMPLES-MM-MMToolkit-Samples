/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHNEGOT.C                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Stream Handler Negotiate Results routine              */
/*                                                                          */
/* FUNCTION: This function starts a stream instance.                        */
/*                                                                          */
/* ENTRY POINTS: ShcNegotiateResult                                         */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/
#define  INCL_NOPMAPI                  /* no PM include files required */
#define  INCL_DOSSEMAPHORES
#define  INCL_DOSPROCESS
#define  INCL_DOSERRORS
#include <os2.h>
#include <os2me.h>
#include <hhpheap.h>
#include <shi.h>

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ShcNegotiateResult                                      */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Command Negotiate results               */
/*                                                                          */
/* FUNCTION:                                                                */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShcNegotiateResult                                          */
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

RC ShcNegotiateResult(pnrparm)
PARM_NEGOTIATE* pnrparm;

{ /* Start of ShcNegotiateResult */

RC rc = NO_ERROR;                       // local return code
PSIB psib;                              // Stream instance block

  /*
   * Find our Stream Instance Block
   */
  if (!(rc = ShFindSib(&psib, pnrparm->hstream, pnrparm->hid)))
    { /* We have SIB */
      /* error if we don't have something to stream */
      if (psib->ulStatus != SIB_INITIALIZED)
        {
          rc = ERROR_INVALID_FUNCTION;
        }
      else
        {
          psib->espcb.spcb = *pnrparm->pspcb;
          psib->ulStatus = SIB_NEGOTIATED;
        }
    } /* We have SIB */

  return(rc);

} /* End of ShcNegotiateResult */
