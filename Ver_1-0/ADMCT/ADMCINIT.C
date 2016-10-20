/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME: admcinit.c
*
* DESCRIPTIVE NAME: Audio MCD DLL Intialization and termination
*                  functions.
*
* FUNCTION: Create Global heap, carry out per process intialization.
*
* NOTES:
*
* EXIT-NORMAL: Return Code 0.
*
* EXIT_ERROR:  Error Code.
*
*
* Global Vars Referenced:
*              hmtxProcSem, heap, UserCount,
*              hEventInitCompleted,
*
* INTERNAL REFERENCES:   ADMCEXIT () -- DLL Termination Routine.
*                        _DLL_InitTerm -- DLL Intialization Routine
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
#define HEAP_SIZE               64768
#define INTIALIZE               0
#define TERMINATE               1
#define FAILURE                 0L
#define SUCCESS                 1L

#include <os2.h>                        // OS/2 System Include
#include <hhpheap.h>                    // Heap Manager Definitions

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
*     LINKAGE:   CALL FAR
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
  UserCount--;
  /********************************************
  * Open the global mutex semaphore
  *********************************************/
  ulrc = DosOpenMutexSem ((ULONG)NULL, (PHMTX)&hmtxProcSem);

  if (ulrc)
      return (FAILURE);              /* Failed Sem Open */

  /*****************************************
  * Request the mutex semaphore.
  ******************************************/
  if ((ulrc = DosRequestMutexSem (hmtxProcSem,
                                 (ULONG)INDEFNITE_PERIOD)) != 0) {
      DosCloseMutexSem (hmtxProcSem);
      return (FAILURE);              /* Indicate failure to loader */
  }
  /*********************************************
  * The last process destroys the heap
  **********************************************/
  if (UserCount == 0) {
      HhpDestroyHeap (heap);    /* Destroy the heap */
  }

  /*********************************************
  * If not the last release access to heap
  **********************************************/
  if (UserCount > 0) {
      HhpReleaseHeap (heap, HhpGetPID());  /* Release access */
  }

  DosReleaseMutexSem (hmtxProcSem);
  DosCloseMutexSem (hmtxProcSem);

  /* De register Termination Routine */

  //DosExitList (EXLST_EXIT, (PFNEXITLIST)ADMCEXIT);

  return (SUCCESS);
}




/********************* START OF SPECIFICATIONS *******************************
*
* SUBROUTINE NAME: ADMCINIT.C
*
* DESCRIPTIVE NAME: Audio MCD DLL Initialization.
*
* FUNCTION: Intialize the DLL and provide mem access to multiple processes.
*
* NOTES: Access to The Global heap is provided using the heap manager
*        Routines. An Exit List is Registered at intialization Time.
*        The Routine is deregistered when the last Process that uses
*        this DLL Terminates.
*
* NOTES:  This routine utilizes a feature of the IBM C Set/2 Compiler
*         that frees programmers from having to write assembler stubs to
*         initialize their re-entrant DLLs.
*         C Set/2 provides an assembler initialization stub with the
*         compiler, and that stub sets the entry point of the DLL to
*         _DLL_InitTerm.  As long as the DLL entry point has this name,
*         the DLL can take advantage of the C SET/2 assembler stub instead
*         of writing one of it's own.
*
* ENTRY POINT: _DLL_InitTerm
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

#pragma linkage (_DLL_InitTerm, system)

unsigned long _DLL_InitTerm (unsigned long hModhandle, unsigned long Flag)
{

  extern int          UserCount;
  extern HHUGEHEAP    heap;
  extern HMTX         hmtxProcSem;
  extern HEV          hEventInitCompleted;
  extern ULONG        lPost;

  ULONG               ulrc;
  USHORT              HeapAllocated;
  USHORT              RCExit;
  INT                 rc;
  ulrc = hModhandle;      // Subdue the Warning for now
  ulrc = MCIERR_SUCCESS;
  HeapAllocated = FALSE;

  switch (Flag)
  {
  case INTIALIZE:

       {
       rc = _CRT_init();
       if (rc) {
          return (0L);
       }
       /*****************************************************************
       * Create The InitCompleted Event Sem. This semaphore is used
       * to block all entries in to the DLL prior to the completion of
       * intialization routine. On completion of the intialization
       * InitCompleted event is posted which frees up any blocked
       * thread at the entry point.
       *******************************************************************/

       if (hEventInitCompleted == (ULONG)NULL) {

           ulrc = DosCreateEventSem ((ULONG)NULL,
                                     (PHEV)&(hEventInitCompleted),
                                     DC_SEM_SHARED,
                                     FALSE);
           if (ulrc)
               return (FAILURE);
       }
       /*****************************************
       * Reset Intialization Complete Semaphore
       ******************************************/
       DosResetEventSem (hEventInitCompleted, &lPost);


       /**************************************
       * Create the Global Mutex Sem
       ***************************************/
       if ((hmtxProcSem == 0) && (UserCount == 0)) {
            ulrc = DosCreateMutexSem ((ULONG)NULL,
                                &hmtxProcSem,
                                DC_SEM_SHARED,
                                (ULONG)FALSE);
            if (ulrc)
                    return (FAILURE);  /* Failed Sem Create */
       }
       /***************************************
       * Open The Mutex Semaphore
       ****************************************/
       ulrc = DosOpenMutexSem ((ULONG)NULL, (PHMTX)&hmtxProcSem);

       if (ulrc)
            return (FAILURE);          /* Failed Sem Open */

       /****************************************
       * Acquire The Mutex Semaphore
       *****************************************/
       if ((ulrc = DosRequestMutexSem (hmtxProcSem,
                              (ULONG)INDEFNITE_PERIOD)) != 0) {

           DosCloseMutexSem (hmtxProcSem);
           return (FAILURE);
       }

       if (UserCount == 0) {

           heap = HhpCreateHeap ((ULONG)HEAP_SIZE, HH_SHARED);

           if (heap == (ULONG)NULL) {
               DosReleaseMutexSem (hmtxProcSem);
               DosCloseMutexSem (hmtxProcSem);
               return (FAILURE);
           }
           HeapAllocated = TRUE;
       }
       /*******************************************
       * Register ExitList Routine.
       ********************************************/
       // ulrc = DosExitList (0x6101, (PFNEXITLIST)ADMCEXIT);

       /*************************************************
       * If the user count is greater than one
       * then we just provide access to the global heap.
       **************************************************/
       if (UserCount != 0) {
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

       UserCount++;                         /* Increment Usage */
       //DosCloseMutexSem (hmtxProcSem);

       /**************************************
       * Post Intialization Complete Semaphore
       ****************************************/
       DosPostEventSem (hEventInitCompleted);

       /************************************
       * Init Complete, Indicate success
       * to the loader
       *************************************/
       return (SUCCESS);
       }
       break;

  case TERMINATE:
       {
          _CRT_term();
          RCExit = ADMCEXIT ();
          return (RCExit);
       }
       break;

  } /* Of switch statement */

} /* of Init Term Function */
