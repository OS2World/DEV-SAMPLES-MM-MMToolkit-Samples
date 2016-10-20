/****************************************************************************/
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1992                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME: SHI.H                                                  */
/*                                                                          */
/* This file contains the stream handler internal definitions and types.    */
/*                                                                          */
/****************************************************************************/

/*
 *   Miscellaneous defines
 */

#define EXTLSTADD_ORDER       0x6201   // Exit list order for us
#define CREATE_SUSPENDED      1L       // DosCreateThread suspended
#define STACKSIZE             4096L    // Stack size for new thread
#define STACKSIZE32           1024*32L // Stack size for new thread
#define HEAPSIZE              4096L    // Heap size for control blocks
#define SPI_SEEK_MODE_MASK    (SPI_SEEK_ABSOLUTE+SPI_SEEK_RELATIVE+SPI_SEEK_FROMEND)
#define TEMPORARY_ERROR       0L
#define RECOVERABLE_ERROR     1L
#define NONRECOVERABLE_ERROR  2L
#define DO_SLEEP              0L
#define DONT_SLEEP            5L
#define DONT_SLEEP_FLUSH      6L

/*
 *   Structure declarations
 */

/*
 *   Extended Stream Protocol Control Block (ESPCB)
 */
typedef struct espcb {
          struct espcb *pnxtESPCB;  // rtn for events of this datatype
          SPCB         spcb;        // Stream Protocol Control Block
        } ESPCB, *PESPCB;

/*
 * Stream Instance Block
 */

typedef struct sib {                   // Stream Instance Block
           struct sib *pnxtSIB;        // Pointer to next SIB
           ESPCB       espcb;          // Extended SPCB
           HSTREAM     hStream;        // Stream handle for this instance
           HID         HandlerID;      // Handler ID (src or tgt)
           ULONG       ulStatus;       // Status of this instance
           ULONG       ulActionFlags;  // Flags that indicate that the
                                       //  read/write thread needs to do
                                       //  some special action.
           TID         ThreadID;       // ID of I/O thread
           PID         ProcessID;      // Process ID that created the stream
           ULONG       ulAssocP1;      // Associate Parm 1
           ULONG       ulAssocP2;      // Associate Parm 2
           ULONG       aulInfo[5];     // Array for misc information
           MMTIME      mmtCurrentTime; // Current mmtime of the stream
           HEV         hevStop;        //
           HEV         hevCmdSync;     // sem for synchronizing command
                                       // with the IO thread
        }  SIB, *PSIB;

//
//   SIB ulStatus States
//

#define SIB_INITIALIZED          1L
#define SIB_NEGOTIATED           2L
#define SIB_READY_TO_RUN         3L
#define SIB_RUNNING              4L
#define SIB_PAUSED               5L

//
// SIB ulActionFlag bit meanings
//

#define SIBACTFLG_KILL         0x0001L     // Kill the read/write thread
#define SIBACTFLG_STOP_FLUSH   0x0002L     // Stop and flush all filled buffers.
                                           //  This flag is the same value as the
                                           //  Stop stream flag bit value.
#define SIBACTFLG_STOP_DISCARD 0x0004L     // Stop and discard all filled buffers.
                                           //  This flag is the same value as the
                                           //  Stop stream flag bit value.
#define SIBACTFLG_STOP_PAUSE   0x0008L     // Stop Pause the IO thread
                                           //  This is only used for stream handlers that the IO
                                           //  thread gets the global data mutex sem.
#define SIBACTFLG_THREAD_DEAD  0x0010L     // The IO Thread has exited.  This could be due
                                           //  to several reasons including exit list processing.
                                           //  The synchronous commands should not wait on the
                                           //  io thread to post an event if this flag is set.

/*
 *   External Global Data Definitions
 */

extern PESPCB pESPCB_ListHead;         // Pointer to list of Extended SPCB's
extern ULONG ulProcessCount;           // # of processes using this stream
                                       //  handler
extern PSIB pSIB_list_head;            // Pointer to list of SIB's in use
                                       //  for this process.
extern PSZ pszHandlerName;             // Name of this stream handler
extern ULONG ulHandlerVersion;         // Version of this handler
extern HID SrcHandlerID;               // Handler ID returned from Register
extern HID TgtHandlerID;               // Handler ID returned from Register
extern HHUGEHEAP hHeap;                // Handle of heap for SIB's and EVCB's
extern PSZ pszProcCntMutexName;        // Name of semaphore to control the
                                       //  usProcessCount variable.
extern HMTX hmtxProcCnt;               // Handle of semaphore to control the
                                       //  usProcessCount variable.
extern HMTX hmtxGlobalData;            // Handle of semaphore to control the
                                       //  Global Data structures.
extern PFN ShWriteRtn;                 // variable to hold @ of write routine
extern PFN ShReadRtn;                  // variable to hold @ or read routine.

/*
 * Prototype statements
 */

/* Shc commands */

RC ShcCreate(PPARM_CREATE);
RC ShcAssociate(PPARM_ASSOC);
RC ShcStart(PPARM_START);
RC ShcStop(PPARM_STOP);
RC ShcDestroy(PPARM_DESTROY);
RC ShcSeek(PPARM_SEEK);
RC ShcGetProtocol(PPARM_GPROT);
RC ShcInstallProtocol(PPARM_INSTPROT);
RC ShcEnumerateProtocols(PPARM_ENUMPROT);
RC ShcNegotiateResult(PPARM_NEGOTIATE);
RC ShcEnableSync(PPARM_ENSYNC);
RC ShcDisableSync(PPARM_DISSYNC);

/* Utility routines */
VOID APIENTRY ShExitList(VOID);
  /* In shmisc.c */
SHCFN ShcRouter;
RC ShFindSib(PSIB*, HSTREAM, HID);
PESPCB ShFindEspcb(SPCBKEY);
  /* In shioutil.c */
RC ReportEvent(PSIB, RC, ULONG, ULONG, ULONG);
RC CheckNSleep(PSIB);
RC SleepNCheck(PSIB, PULONG);
RC ShIOError(PSIB, PARM_NOTIFY, RC);

