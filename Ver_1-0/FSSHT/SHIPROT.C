/*static char *SCCSID = "@(#)shiprot.c	13.2 92/05/01";*/
/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHIPROT.C                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Stream Handler Install Protocol Routine               */
/*                                                                          */
/* NOTES: This is a DLL file. This function is mainly for tuning streams    */
/*        performance by changing the buffer attributes.                    */
/*                                                                          */
/* ENTRY POINTS: ShcInstallProtocol                                         */
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
/* SUBROUTINE NAME: ShcInstallProtocol                                      */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Command Install Protocol                */
/*                                                                          */
/* FUNCTION: This updates the specified spcb with new values                */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShcInstallProtocol                                          */
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

RC ShcInstallProtocol(pipparm)
PPARM_INSTPROT pipparm;

{ /* Start of ShcInstallProtocol */

RC rc = NO_ERROR;                       // local return code
int notfound = TRUE;
PESPCB pTempEspcb;
PESPCB pPrevEspcb;

  //
  // the ESPCB list is under semphore control
  //
  if (!(rc = DosRequestMutexSem(hmtxGlobalData, SEM_INDEFINITE_WAIT)))
    { /* obtained semaphore */
      if (pipparm->ulFlags & SPI_DEINSTALL_PROTOCOL)
        { /* DeInstall */
          //
          // To Deinstall, Find the spcb,
          //               Take it off the espcb chain,
          //               Free the espcb memory allocated
          //
          rc = ERROR_INVALID_SPCBKEY;
          pPrevEspcb = NULL;
          pTempEspcb = pESPCB_ListHead;
          while (pTempEspcb && notfound)
            { /* Loop thru espcbs */
              if ((pipparm->spcbkey.ulDataType == pTempEspcb->spcb.spcbkey.ulDataType) &&
                  (pipparm->spcbkey.ulDataSubType == pTempEspcb->spcb.spcbkey.ulDataSubType) &&
                  (pipparm->spcbkey.ulIntKey == pTempEspcb->spcb.spcbkey.ulIntKey))
                { /* found match */
                  notfound = FALSE;
                  rc = NO_ERROR;
                  //
                  // Take the espcb off the chain
                  //
                  if (pPrevEspcb)
                    {
                      pPrevEspcb->pnxtESPCB = pTempEspcb->pnxtESPCB;
                    }
                  else
                    {
                      pESPCB_ListHead = pTempEspcb->pnxtESPCB;
                    }

                  HhpFreeMem(hHeap, pTempEspcb);

                } /* found match */
              else
                { /* try the next espcb in the chain */

                  pPrevEspcb = pTempEspcb;
                  pTempEspcb = pTempEspcb->pnxtESPCB;

                } /* try the next espcb in the chain */
            } /* Loop thru espcbs */
        } /* DeInstall */
      else
        { /* Install */
          //
          // If the spcb already exists then error
          //
          if (ShFindEspcb(pipparm->spcbkey))
            {
              rc = ERROR_INVALID_SPCBKEY;
            }
          else
            { /* Ok to add spcb */
              //
              // Allocate the espcb and put it on the front of the chain
              //
              pTempEspcb = (PESPCB)HhpAllocMem(hHeap, sizeof(ESPCB));
              if (pTempEspcb)
                {
                  pTempEspcb->spcb = *(pipparm->pspcb);
                  pTempEspcb->pnxtESPCB = pESPCB_ListHead;
                  pESPCB_ListHead = pTempEspcb;
                }
              else
                {
                  rc = ERROR_ALLOC_RESOURCES;
                }
            } /* Ok to add spcb */

        } /* Install */

      DosReleaseMutexSem(hmtxGlobalData);

    } /* obtained semaphore */

  return(rc);

} /* End of ShcInstallProtocol */
