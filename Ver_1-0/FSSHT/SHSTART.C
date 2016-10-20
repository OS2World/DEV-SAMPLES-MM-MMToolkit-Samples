/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHSTART.C                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  Stream Handler Start stream routine                   */
/*                                                                          */
/* FUNCTION: This function starts a stream instance.                        */
/*                                                                          */
/* ENTRY POINTS: ShcStart                                                   */
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
/* SUBROUTINE NAME: ShcStart                                                */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Command Start stream routine            */
/*                                                                          */
/* FUNCTION: This routine kick starts the IO thread to start reading or     */
/*   writing.  It may be called indirectly from SpiStartStream, or by the   */
/*   stream manager buffer control code.  If we had to block because no     */
/*   buffers were available, then the buffer control code will send a start.*/
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShcStart                                                    */
/*   LINKAGE:   CALL NEAR (0:32)                                            */
/*                                                                          */
/* INPUT: Pointer to shc start parameter block (PARM_START) containing:     */
/*   ULONG   ulFunction  Handler command function SHC_START                 */
/*   HID     hid         handler ID                                         */
/*   HSTREAM hstream     handle of stream instance                          */
/*                                                                          */
/* EXIT-NORMAL: NO_ERROR (0)                                                */
/*                                                                          */
/* EXIT-ERROR:                                                              */
/*   ERROR_INVALID_SPCBKEY                                                  */
/*   ERROR_ALLOC_RESOURCES                                                  */
/*   Errors from the external routines:                                     */
/*     DosRequestMutexSem                                                   */
/*                                                                          */
/* SIDE EFFECTS:                                                            */
/*   The IO thread is unblocked.                                            */
/*                                                                          */
/* EXTERNAL REFERENCES:                                                     */
/*   ROUTINES:                                                              */
/*     ShFindSib                                                            */
/*     DosResumeThread                                                      */
/*                                                                          */
/*   DATA STRUCTURES:                                                       */
/*                                                                          */
/*************************** END OF SPECIFICATIONS **************************/

RC ShcStart(pstparm)
PPARM_START pstparm;

{ /* Start of ShcStart */

RC rc = NO_ERROR;                       // local return code
PSIB psib;                              // Stream instance block
BOOL bPaused;                           // Is stream paused?

  /*
   * Find our Stream Instance Block
   */
  if (!(rc = ShFindSib(&psib, pstparm->hstream, pstparm->hid)))
    { /* We have SIB */
      /* error if we don't have something to stream */
      if (psib->ulStatus < SIB_RUNNING)
        {
          rc = ERROR_DATA_ITEM_NOT_SPECIFIED;
        }
      else
        {
          if (psib->ulActionFlags & SIBACTFLG_THREAD_DEAD)
            {
              rc = ERROR_INVALID_SEQUENCE;
            }
          else
            { /* command sequence ok */

              bPaused = (psib->ulStatus == SIB_PAUSED);
              psib->ulStatus = SIB_RUNNING;
              if (psib->ThreadID)
                {
                  DosPostEventSem(psib->hevStop);
                  if (bPaused)
                    {
                      DosResumeThread(psib->ThreadID);
                    }
                }
            } /* command sequence ok */
        }
    } /* We have SIB */

  return(rc);

} /* End of ShcStart */
