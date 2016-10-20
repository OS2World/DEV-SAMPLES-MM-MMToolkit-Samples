/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: ADMCINIT.c
*
*              Copyright (c) IBM Corporation  1991, 1993
*                        All Rights Reserved
*
* DESCRIPTIVE NAME: Audio MCD DLL Intialization and termination
*                  functions.
*

* FUNCTION: Create Global heap, carry out per process intialization.
*
* NOTES:
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
* EFFECTS:
*
* Global Vars Referenced:
*              hmtxProcSem, heap, UserCount,
*              hEventInitCompleted,
*
* INTERNAL REFERENCES:   ADMCEXIT () -- DLL Termination Routine.
*                        InitADMC () -- DLL Intialization Routine
*
* EXTERNAL REFERENCES:   DosResetEventSem ()        - OS/2 API
*                        DosPostEventSem  ()        - OS/2 API
*                        DosCreateMutexSem()        - OS/2 API
*                        HhpCreateHeap    ()        - MME  API
*                        HhpDestroyHeap   ()        - MME  API
*                        HhpGetPID()      ()        - MME  API
*
*********************** END OF SPECIFICATIONS **********************/

#define INCL_BASE
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_ERRORS

#define MCIERR_SUCCESS          0
#define INDEFNITE_PERIOD       -1
#define INTIALIZE               0
#define TERMINATE               1
#define HEAP_SIZE               8192
#define FAILURE                 0L
#define SUCCESS                 1L

#include <os2.h>                        // OS/2 System Include
#include <hhpheap.h>                    // Heap Manager Definitions
#include <admcres.h>

//MRESULT EXPENTRY ClipboardProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ADMEXIT
*
* DESCRIPTIVE NAME: AudioMCD DLL Temination Routine
*
* FUNCTION: Cleanup and deallocate resources used.
*
*
* ENTRY POINTS:
*
* NOTES: The following concepts are illustrated in this file:
*  A. How to keep track of the number of copies of a dll which
*     are loaded in this system.
*  B. How to allocate and deallocate heaps.
*  C. How to use semaphores accross processes.
*
* INPUT:
*
* EXIT-NORMAL: Return Code 1 to the System Loader.
*
* EXIT_ERROR:  Failure To Load This Module.
*
* EFFECTS:
*
* INTERNAL REFERENCES: heap management Routines.
*
*               Global Variables referenced:
*                      heap.
*                      hmtxProcSem.
*                      UserCount.
*
* EXTERNAL REFERENCES: DosCreateMutexSem  ()    -  OS/2 API
*                      DosOpenMutexSem    ()    -  OS/2 API
*                      DosReleaseMutexSem ()    -  OS/2 API
*                      DosCloseMutexSem   ()    -  OS/2 API
*                      DosExitList        ()    -  OS/2 API -- (DCR ed )
*
*********************** END OF SPECIFICATIONS ********************************/

USHORT ADMCEXIT ()                    /* AudioMCD Exit Routine    */
{

  extern int          UserCount;   // Number of Dynamic links
  extern HHUGEHEAP    heap;        // Global Heap
  extern HMTX         hmtxProcSem; // Global Mutex semaphore
  ULONG               ulrc;        // Return Code

  ulrc = MCIERR_SUCCESS;

  /*************************************************
  * Since we are terminating, there is one less
  * copy of our dll alive, so reduce the global
  * memory count of the number of dll's alive by
  * one.  This count is important, since if we ever
  * are reduced to 0 copies of the dll, we should
  * free the heap memory that we allocatd.
  *************************************************/

  UserCount--;



  /*****************************************
  * Request the mutex semaphore.
  ******************************************/
  if ((ulrc = DosRequestMutexSem ( hmtxProcSem,
                                   INDEFNITE_PERIOD)) != 0)
    {
    DosCloseMutexSem (hmtxProcSem);
    return (FAILURE);
    }


  /*********************************************
  * If there are no more dll's alive, destroy the
  * heap.
  **********************************************/

  if (UserCount == 0)
     {
     HhpDestroyHeap (heap);    /* Destroy the heap */
     }

  /*********************************************
  * If not the last dll, then release access to heap
  **********************************************/

  if (UserCount > 0)
    {
    HhpReleaseHeap (heap, HhpGetPID());  /* Release access */
    }

  DosReleaseMutexSem (hmtxProcSem);
  DosCloseMutexSem (hmtxProcSem);

  return (SUCCESS);
} /* ADMCExit */




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ADMCINIT.C
*
* DESCRIPTIVE NAME: AudioMCD DLL Ibtialization.
*
* FUNCTION: Intialize the DLL and provide mem access to multiple processes.
*
* NOTES: Access to The Global heap is provided using the heap manager
*        Routines. The DLL_InitTerm function is the first function
*        called by the OS/2 DLL Loader.  If the flag variable is set to
*        0, then this is DLL initialization, else it will be termination.
*
*        With the global variable ulUserCount, we keep track of how many
*        times this dll is loaded (i.e. how many process have loaded the
*        DLL).  This variable is used to monitor access to the global
*        memory heap that all instances of the dll maintain.
*
*
* ENTRY POINTS:
*
* INPUT:
*
* EXIT-NORMAL: Return Code 1 to the System Loader.
*
* EXIT_ERROR:  Failure To Load This Module.
*
* EFFECTS:
*
* INTERNAL REFERENCES: heap management Routines,
*
*               Global Variables referenced:
*                      heap.
*                      hmtxProcSem.
*                      UserCount.
*
* EXTERNAL REFERENCES: DosCreateMutexSem  ()    -  OS/2 API
*                      DosOpenMutexSem    ()    -  OS/2 API
*                      DosReleaseMutexSem ()    -  OS/2 API
*                      DosCloseMutexSem   ()    -  OS/2 API
*                      DosExitList        ()    -  OS/2 API
*
*********************** END OF SPECIFICATIONS ********************************/

/* COVCC_!INSTR */
#pragma linkage (_DLL_InitTerm, system)

/* COVCC_!INSTR */
unsigned long _DLL_InitTerm (unsigned long hModhandle, unsigned long Flag)


{

  extern int          UserCount;
  extern HHUGEHEAP    heap;
  extern HMTX         hmtxProcSem;

  extern ULONG        hModuleHandle;

//  extern   HAB        hab;
//  extern   HMQ        hmq;
//  extern   QMSG       qmsg;
//  extern   HWND       hwndClipWin;               /* Clipboard win handle */



  ULONG               ulrc;
  USHORT              RCExit;
  ulrc = hModhandle;      // Subdue the Warning for now
  ulrc = MCIERR_SUCCESS;

  /*********************************************
  * Store the module handle for future reference
  * (i.e. GpiLoadBitmap)
  **********************************************/


  hModuleHandle = hModhandle;

  if (Flag == INTIALIZE )
       {

       /**************************************
       * Create a semaphore which will be used
       * by all instances of this dll to prevent
       * one copy of the dll from overwriting
       * another's memory etc.
       ***************************************/

       if ( UserCount == 0 )
          {
            ulrc = DosCreateMutexSem ( (ULONG)NULL,
                                       &hmtxProcSem,
                                       DC_SEM_SHARED,
                                       FALSE );
            if (ulrc)
               {
               return (FAILURE);  /* Failed Sem Create */
               }

          } /* if this is the first instance of the dll */

       _CRT_init();

       /***************************************
       * Open the Mutex Semaphore for process control
       ****************************************/

       ulrc = DosOpenMutexSem ((ULONG)NULL, &hmtxProcSem);

       if (ulrc)
          {
          return (FAILURE);
          }

       /****************************************
       * Acquire The Mutex Semaphore
       *****************************************/

       if ((ulrc = DosRequestMutexSem ( hmtxProcSem,
                                        INDEFNITE_PERIOD)) != 0)
         {

         DosCloseMutexSem (hmtxProcSem);
         return (FAILURE);
         }

       /**************************************
       * If we are the first copy of the dll
       * currently loaded, then create a heap
       * where all of our memory will be
       * allocated from.
       ***************************************/

       if (UserCount == 0)
          {

           heap = HhpCreateHeap (HEAP_SIZE, HH_SHARED);

           if (heap == (ULONG)NULL)
              {
              DosReleaseMutexSem (hmtxProcSem);
              DosCloseMutexSem (hmtxProcSem);
              return (FAILURE);
              }

          }

       /*************************************************
       * If the user count is greater than one
       * then we just provide access to the global heap
       * which was created on the load of the initial
       * copy of this dll.
       **************************************************/

       if (UserCount != 0)
          {
          /**********************************
          * Provide Shared Access to heap
          ***********************************/
          ulrc = HhpAccessHeap (heap, HhpGetPID());
          if (ulrc)
               return (FAILURE);
          }

       /******************************************
       * Release The Mutex Semaphore
       *******************************************/

       if ((ulrc = DosReleaseMutexSem (hmtxProcSem)) != 0L)
          {
          if (UserCount != 0)
                DosCloseMutexSem (hmtxProcSem);

          return (FAILURE);
          }


       /*****************************************
       * The stream handlers, but only for the
       * first DLL
       *****************************************/

       ulrc = StreamSetup ( );

       if ( ulrc )
          {
          return ( ulrc );
          }

       UserCount++;                         /* Increment Usage */


       /***********************************************
       * Init Complete, Indicate success to the loader
       ************************************************/

       return (SUCCESS);

       } /* if this is DLL initialization */

    /********************************************
    * If the flag is anything but initialization
    * then it MUST be termination.
    *********************************************/

    else
       {
          RCExit = ADMCEXIT ();
          return (RCExit);
       } /* else this must be termination */

} /* of Init Term Function */

/* COVCC_INSTR */
