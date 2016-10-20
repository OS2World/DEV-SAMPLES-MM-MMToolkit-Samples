/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHSTOP.C                                               */
/*                                                                          */
/* DESCRIPTIVE NAME:  Stream Handler Stop stream routine                    */
/*                                                                          */
/* FUNCTION: This function stops a stream instance.                         */
/*                                                                          */
/* ENTRY POINTS: ShcStop                                                    */
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
/* SUBROUTINE NAME: ShcStop                                                 */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Command Stop stream routine             */
/*                                                                          */
/* FUNCTION: This stops the specified stream.  The flags passed indicate    */
/*           if this should discard all buffers or not.                     */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShcStop                                                     */
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

RC ShcStop(pstparm)
PPARM_STOP pstparm;

{ /* Start of ShcStop */

RC    rc = NO_ERROR;                    // local return code
PSIB  psib;                             // Stream instance block
ULONG ulPostCount;                      // dummy

  /*
   * Find our Stream Instance Block
   */
  if (!(rc = ShFindSib(&psib, pstparm->hstream, pstparm->hid)))
    { /* We have SIB */
      //
      // Don't bother checking most states since the stream manager does this
      // for us before the command gets here.
      //
      // If this is stop Flush and we are the target then we will stop when we
      // run out of buffers.  So ignore the stop command.
      //
      if (!((pstparm->ulFlags & SPI_STOP_FLUSH) && (pstparm->hid == TgtHandlerID)))
        { /* not flush and target handler */
          if (pstparm->ulFlags & SPI_STOP_DISCARD+SPI_STOP_FLUSH)
            { /* We must release all buffers */
              if (psib->ThreadID)
                {
                  //
                  //  Update action flags to stop the independent thread, and wait on the
                  //  command sync semaphore to wait until it gets the command.
                  //
                  DosResetEventSem(psib->hevCmdSync, &ulPostCount);
                  if (pstparm->ulFlags & SPI_STOP_DISCARD)
                    psib->ulActionFlags |= SIBACTFLG_STOP_DISCARD;
                  if (pstparm->ulFlags & SPI_STOP_FLUSH)
                    psib->ulActionFlags |= SIBACTFLG_STOP_FLUSH;

                  DosPostEventSem(psib->hevStop);
                  if (psib->ulStatus == SIB_PAUSED)
                    {
                      psib->ulStatus = SIB_RUNNING;
                      DosResumeThread(psib->ThreadID);
                    }
                  if (!(psib->ulActionFlags & SIBACTFLG_THREAD_DEAD))
                    DosWaitEventSem(psib->hevCmdSync, SEM_INDEFINITE_WAIT);
                }
              else
                {
                  if (pstparm->ulFlags & SPI_STOP_DISCARD)
                    psib->ulActionFlags |= SIBACTFLG_STOP_DISCARD;
                  if (pstparm->ulFlags & SPI_STOP_FLUSH)
                    psib->ulActionFlags |= SIBACTFLG_STOP_FLUSH;
                }
            } /* We must release all buffers */
          else
            { /* Assume stop pause command*/

              if (psib->ulStatus == SIB_RUNNING)
                {
                  psib->ulStatus = SIB_PAUSED;
                  //
                  // Check for NULL threadid (filter stream handlers)
                  //
                  if ((psib->ThreadID) &&
                      !(psib->ulActionFlags & SIBACTFLG_THREAD_DEAD))
                    {
                      rc = DosSuspendThread(psib->ThreadID);
                    }
                }
            } /* Assume stop pause command*/
        } /* not flush and target handler */
      else
        { /* flush and target */

          if (psib->ulStatus == SIB_PAUSED)
            {
              psib->ulStatus = SIB_RUNNING;
              DosResumeThread(psib->ThreadID);
            }
        } /* flush and target */
    } /* We have SIB */

  return(rc);

} /* End of ShcStop */
