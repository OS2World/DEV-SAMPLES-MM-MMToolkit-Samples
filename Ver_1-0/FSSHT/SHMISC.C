/*static char *SCCSID = "@(#)shmisc.c	13.2 92/05/01";*/
/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHMISC.C                                               */
/*                                                                          */
/* DESCRIPTIVE NAME:  Stream Handler Misc. routines                         */
/*                                                                          */
/* FUNCTION: This file contains the supporting routines for the stream      */
/*           handlers.                                                      */
/*                                                                          */
/* ENTRY POINTS: ShFindSib                                                  */
/*               ShFindEspcb                                                */
/*************************** END OF SPECIFICATIONS **************************/
#define  INCL_NOPMAPI                  /* no PM include files required */
#define  INCL_DOSSEMAPHORES
#define  INCL_DOSERRORS
#define  INCL_DOSPROCESS
#include <os2.h>
#include <os2me.h>
#include <hhpheap.h>
#include <shi.h>

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ShFindSib                                               */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Find Stream Instance Block              */
/*                                                                          */
/* FUNCTION: Finds the SIB associated with the stream handle, and handler   */
/*           ID passed.                                                     */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShFindSib                                                   */
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

RC ShFindSib(psib, hstream, hid)
PSIB *psib;                               // Output - pointer to SIB
HSTREAM hstream;                          // Input - stream handle
HID hid;                                  // Handler ID to identify
                                          //  source or target

{ /* Start of ShFindSib */

RC rc = NO_ERROR;                       // local return code
PSIB psibtemp;                          // Stream instance block

  /*
   * Find our Stream Instance Block by searching the SIB list.  It is
   *  under semaphore control, so get it first.
   */
  rc = DosRequestMutexSem(hmtxGlobalData, SEM_INDEFINITE_WAIT);
  if (!rc)
    { /* Search SIB list */
      psibtemp = pSIB_list_head;
      while ((psibtemp) &&
             !((psibtemp->hStream == hstream) && (psibtemp->HandlerID == hid)))
        {
          psibtemp = psibtemp->pnxtSIB;
        }
      if (psibtemp)
        {
          *psib = psibtemp;
        }
      else
        {
          rc = ERROR_INVALID_STREAM;
        }

      DosReleaseMutexSem(hmtxGlobalData);

    } /* Search SIB list */

  return(rc);

} /* End of ShFindSib */

/************************** START OF SPECIFICATIONS *************************/
/*                                                                          */
/* SUBROUTINE NAME: ShFindEspcb                                             */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Find Extended Stream Protocol Control   */
/*                   Block                                                  */
/*                                                                          */
/* FUNCTION: Finds the ESPCB described by the spcbkey passed.               */
/*                                                                          */
/* NOTES: The hmtxGlobalData semaphore must be obtained before calling      */
/*        this routine.                                                     */
/*                                                                          */
/* ENTRY POINT: ShFindEspcb                                                 */
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

PESPCB ShFindEspcb(spcbkey)
SPCBKEY spcbkey;                          // Input - key of espcb to find

{ /* Start of ShFindEspcb */

int notfound = TRUE;
PESPCB pTempEspcb;

  /*
   * Find our Extended SPCB by searching the ESPCB list.
   */

    { /* Search ESPCB list */

      pTempEspcb = pESPCB_ListHead;
      while (pTempEspcb && notfound)
        { /* Loop thru espcbs */
          if ((spcbkey.ulDataType == pTempEspcb->spcb.spcbkey.ulDataType) &&
              (spcbkey.ulDataSubType == pTempEspcb->spcb.spcbkey.ulDataSubType) &&
              (spcbkey.ulIntKey == pTempEspcb->spcb.spcbkey.ulIntKey))
            {
              notfound = FALSE;
            }
          else
            {
              pTempEspcb = pTempEspcb->pnxtESPCB;
            }
        } /* Loop thru espcbs */

    } /* Search ESPCB list */

  return(pTempEspcb);

} /* End of ShFindEspcb */
