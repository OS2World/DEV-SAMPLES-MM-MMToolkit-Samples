/*static char *SCCSID = "@(#)cdmcinit.c	13.4 92/04/30";*/
/******************************STATE OF SPECIFICATIONS**********************/
/*                                                                         */
/*      SOURCE FILE NAME : CDMCINIT.C                                      */
/*                                                                         */
/*      DESCRIPTIVE NAME : CDMC DATA INITIALIZATION  DLL                   */
/*                                                                         */
/*      COPYRIGHT :                                                        */
/*                    COPYRIGHT (C) IBM CORPORATION 1990 - 1992            */
/*                          ALL RIGHTS RESERVED                            */
/*                                                                         */
/*                                                                         */
/*      FUNCTION :                                                         */
/*                Reads the MMPM2.INI file and loads all information for   */
/*                device management into the CDMC_Main_CB structure.       */
/*                                                                         */
/*                                                                         */
/*      NOTES :                                                            */
/*                                                                         */
/*******************************END OF SPECIFICATIONS***********************/


#define INCL_BASE
#define INCL_PM
#define INCL_WINWINDOWMGR
#define INCL_MODULEMGR


#include <stdio.h>
#include <os2.h>
#include <stddef.h>
#include <string.h>
#include "os2me.h"
#include "hhpheap.h"

int            CDMCInitialization(void);
USHORT         CDMC_Exit();

PVOID          CDMC_hHeap;
DWORD          AccessSem = 0;
USHORT         CDMC_Init = 0;

/*
 * NOTE:   This routine utilizes a feature of the IBM C Set/2 Compiler
 *         that frees programmers from having to write assembler stubs to
 *         initialize their re-entrant DLLs.
 *         C Set/2 provides an assembler initialization stub with the
 *         compiler, and that stub sets the entry point of the DLL to
 *         _DLL_InitTerm.  As long as the DLL entry point has this name,
 *         the DLL can take advantage of the C SET/2 assembler stub instead
 *         of writing one of it's own.
 */

#pragma linkage (_DLL_InitTerm, system)

unsigned long _DLL_InitTerm (unsigned long hModhandle, unsigned long ulTerm)
   {

   DWORD SemWait = 7000;
   USHORT InitCompleted;
   INT rc;

   /*
    * Checking this parameter will insure that this routine will only
    * be run on an actual initialization.  Return success from the
    * termination.
    */

   if (ulTerm)
      return (1L);


   rc = _CRT_init();
   if (rc) {
      return (0L);
   }

   InitCompleted = TRUE;   // Set InitCompleted = True;

   /**************************************************************************/
   /* Increment CDMC_Init. If this is the first call to CDMCAttach, then     */
   /* ++CDMC_Init will be 1 , AccessSem will be 0. This causes the semaphore */
   /* to be created, requested and CDMCInitialization called. If ++CDMC_Init */
   /* is >1 or AccessSem != 0, or DosCreateMutexSem fails, then              */
   /* Initialization was previously completed, and HhpAccessHeap will be     */
   /* called                                                                 */
   /**************************************************************************/
   if ( ((++CDMC_Init) == 1) && (!AccessSem) )
      {

      /**********************************************************************
      * If DosCreateMutexSem fails, then Init was previously completed.    **
      **********************************************************************/
      if (!(DosCreateMutexSem(NULL,&AccessSem,DC_SEM_SHARED,FALSE)))
         {

         /******************************************************************/
         /* Set InitCompleted to false (not Previously initialized)        */
         /******************************************************************/
         InitCompleted = 0;

         /******************************************************************/
         /* request the Mutex semaphore                                    */
         /******************************************************************/
         if (DosRequestMutexSem(AccessSem,SemWait))
            {

            /***************************************************************/
            /* close the Mutex semaphore                                   */
            /***************************************************************/
            DosCloseMutexSem(AccessSem);
            return(0);
            }

         /******************************************************************/
         /*   Initialize the CDMC_Main_CB structure                        */
         /******************************************************************/
         if (CDMCInitialization())
            {

            /***************************************************************/
            /* CDMCInitialize failed! Restore The CDMC_Init count          */
            /***************************************************************/
            --CDMC_Init;

            /***************************************************************/
            /*  release the Mutex semaphore                                */
            /***************************************************************/
            DosReleaseMutexSem(AccessSem);

            /***************************************************************/
            /* close the Mutex semaphore                                   */
            /***************************************************************/
            DosCloseMutexSem(AccessSem);
            return(0);
            } /* end if */
         } /* end if */
      } /* end if */


   if (InitCompleted)
   /*********************************************************************/
   /* If CDMCInitialization was called by a previous process:           */
   /* Open and request the semaphore and then Provide shared access to  */
   /* the CDMC_Main_CB structure.                                       */
   /*********************************************************************/
      {
      if ((DosOpenMutexSem(NULL,&AccessSem)))
         return(0);

      if ((DosRequestMutexSem(AccessSem,SemWait)) )
         {

         /****************************************************************/
         /* If request fails, close the Mutex and return an error        */
         /****************************************************************/
         DosCloseMutexSem(AccessSem);
         return(0);
         }

      /*****************************************************************/
      /* Provide access to the heap. CDMC_Main_CB has was initialized  */
      /* before this call.                                             */
      /*****************************************************************/
      if (HhpAccessHeap(CDMC_hHeap,HhpGetPID()))
         {
         DosReleaseMutexSem(AccessSem);
         DosCloseMutexSem(AccessSem);
         return(0);
         } /* end if */

      } /* end if */

    /******************************************************************/
    /* Release the semaphore                                          */
    /******************************************************************/
   if (DosReleaseMutexSem(AccessSem))
      return(0);

   /*************************************/
   /* Successful initialization of DLL  */
   /*************************************/
   return(1);
   }



/****************************************************************************
**  CDMCInitialization                                                     **
*****************************************************************************
*
*  Arguments: no arguments
*
*  Return:
****************************************************************************/


int CDMCInitialization(void)
{

  /***********************************************************************/
  /* Obtain initial heap from memory management routines                 */
  /*   store as pHeap for easy reference                                 */
  /***********************************************************************/
  CDMC_hHeap  = HhpCreateHeap(4096L, HH_SHARED);

  return(0);
}

/****************************************************************************
**  CDMC_Exit                                                              **
*****************************************************************************
*
*  Arguments: no arguments
*
*  Return:
*          0 Success
*
*  Description: Cleans up instances, CDMC_Init count and allocated memory
*               after a process terminates.
*
*  Global Vars referenced:
*               CDMC_Init
*               CDMC_Main_CB
*
*  Global Vars Modified:
*               CDMC_Init
*               CDMC_Main_CB
*
****************************************************************************/


USHORT CDMC_Exit()

   {


   --CDMC_Init;

   if (CDMC_Init == 0)
      {
      if (DosCloseMutexSem(AccessSem) )
         return(1);

      if(HhpDestroyHeap(CDMC_hHeap) )
         return(1);

      }

   DosExitList(EXLST_EXIT,(PFNEXITLIST) CDMC_Exit);
   }

