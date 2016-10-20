/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  AUDSUBS.h         (TEMPLATE SAMPLE)                   */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver subroutines header                 */
/*                                                                          */
/************************** END OF SPECIFICATIONS ***************************/

/*
** Reference file audsubs.c for information
** on these routines.
*/

void ProcessBuffer (void);


/*
** Updates stream position to account for completion
** of present operation and returns a pointer to
** present position (counter).
*/
PULONG GetStreamTime (PSTREAM pStream);

VOID SetStreamTime (PSTREAM pStream, ULONG ulSetPos);

void DevOpen            (void);

void DevClose           (void);

void DevIOCTL           (void);

void DevStart           (void);

void DevStop            (void);

void DevPause           (void);

void DevResume          (void);

void DevChange          (void);         // Change device type

void DevAudioBuffer     (void);

void DevIOCTLload       (void);

void DevIOCTLwait       (void);

void DevIOCTLstatus     (void);

void DevIOCTLhpi        (void);

void DestroyStreams (void);

void ReadDataFromCard   (void);         // Device dependent

void WriteDataToCard    (void);         // Device dependent

/*
** Routines for communication with OS/2 kernel
*/
void EOI                (void);         // Issue End-Of-Interrupt to 8259

void ReadStatus (void);

void WriteStatus (void);

void FlushInputBuffers (void);

void FlushOutputBuffers (void);

ULONG Enqueue_Event (HEVENT hEvent, ULONG ulCuePoint);

ULONG Dequeue_Event (HEVENT hEvent);
