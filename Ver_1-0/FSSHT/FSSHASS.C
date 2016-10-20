/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: FSSHASS.C                                              */
/*                                                                          */
/* DESCRIPTIVE NAME:  File System Stream Handler Associate routine          */
/*                                                                          */
/* FUNCTION: This function is associates the given object with the          */
/*           specified stream.                                              */
/*                                                                          */
/* ENTRY POINTS: ShcAssociate                                               */
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
/* SUBROUTINE NAME: ShcAssociate                                            */
/*                                                                          */
/* DESCRIPTIVE NAME: Stream Handler Command Associate Object routine        */
/*                                                                          */
/* FUNCTION: Associates the object passed with the specified stream         */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* ENTRY POINT: ShcAssociate                                                */
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

RC ShcAssociate(pasparm)
PPARM_ASSOC pasparm;

{ /* Start of ShcAssociate */

RC rc = NO_ERROR;                       // local return code
PSIB psib;                              // Stream instance block

  /*
   * Find our Stream Instance Block
   */
  if (!(rc = ShFindSib(&psib, pasparm->hstream, pasparm->hid)))
    { /* We have SIB */
      if (!(psib->ulActionFlags & SIBACTFLG_THREAD_DEAD))
        { /* validate ObjType */
          if (pasparm->pacb->ulObjType != ACBTYPE_MMIO)
            {
              rc = ERROR_INVALID_OBJTYPE;
            }
          else
            {
              if (pasparm->pacb->ulACBLen < sizeof(ACB_MMIO))
                {
                  rc = ERROR_INVALID_BUFFER_SIZE;
                }
              else
                {
                  psib->ulAssocP1 = pasparm->pacb->ulParm1;
                  psib->ulStatus = SIB_RUNNING;
                }
            }
        } /* validate ObjType */
      else
        {
          rc = ERROR_INVALID_SEQUENCE;
        }
    } /* We have SIB */

  return(rc);

} /* End of ShcAssociate */
