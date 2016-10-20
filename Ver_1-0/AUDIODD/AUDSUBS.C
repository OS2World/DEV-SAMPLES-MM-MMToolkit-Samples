/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  AUDSUBS.C         (TEMPLATE SAMPLE)                   */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver subroutines.                       */
/*                                                                          */
/* DESCRIPTION:  Provides routines to assist execution of the primary       */
/*               logic of the audio device driver (audiodd.c).              */
/*               The routines in this sample are callable, but being stubs, */
/*               don't do anything.  Still, the structure is useful.        */
/*                                                                          */
/*               Most of the routines equate directly to OS/2 2.0           */
/*               kernel <-> PDD interfaces.  Accordingly, the OS/2 2.0      */
/*               Physical Device Driver reference should be referenced for  */
/*               the routines that serve the purpose of kernel communication*/
/*                                                                          */
/*               Other routines are specific to the Audio Device Driver.    */
/************************** END OF SPECIFICATIONS ***************************/

#define  INCL_DOSINFOSEG
#include <os2.h>

#include <os2medef.h>
#include <meerror.h>
#include <ssm.h>
#include <audio.h>
#include "audiodd.h"
#include "audsubs.h"    // Function declarations

extern MCI_AUDIO_IOBUFFER recio;
extern MCI_AUDIO_IOBUFFER xmitio;

/*
** Stub subroutines specific to the audio device driver
*/
void ProcessBuffer (void)
{
}

/*
** Updates stream position to account for completion
** of present operation and returns a pointer to
** present position (counter).
*/
PULONG GetStreamTime (PSTREAM pStream)  // What is present time in stream
{
   if (pStream->ulOperation == OPERATION_RECORD)
      {
      pStream->ulCumTime += recio.ulposition;
      recio.ulposition = 0;
      }
   else
      { /* assume operation == OPERATION_PLAY */
      pStream->ulCumTime += xmitio.ulposition;
      xmitio.ulposition = 0;
      } /* endif */
   return (&pStream->ulCumTime);
}

VOID SetStreamTime (PSTREAM pStream, ULONG ulSetPos)
{
   pStream->ulCumTime = ulSetPos;
}

/*
** Hardware specific operations to open this device
*/
void DevOpen            (void)
{
}

/*
** Hardware specific operations to close this device
*/
void DevClose           (void)
{
}

void DevIOCTL           (void)
{
}

void DevStart           (void)
{
}

void DevStop            (void)
{
}

void DevPause           (void)
{
}

void DevResume          (void)
{
}

void DevChange          (void)          // Change device type
{
}

void DevAudioBuffer     (void)
{
}

void DevIOCTLload       (void)
{
}

void DevIOCTLwait       (void)          // Mapped to OS/2 service
{
}

void DevIOCTLstatus     (void)
{
}

void DevIOCTLhpi        (void)
{
}

void DestroyStreams     (void)
{
}

/*
** Hardware specific operations
*/
void ReadDataFromCard   (void)
{
}

/*
** Hardware specific operations
*/
void WriteDataToCard    (void)
{
}

/*
** Issue End-Of-Interrupt to 8259
** interrupt controller (OS/2 DevHlp)
*/
void EOI (void)
{
}

void ReadStatus (void)
{
}

void WriteStatus (void)
{
}

void FlushInputBuffers (void)
{
}

void FlushOutputBuffers (void)
{
}

/*
** When processing data, the PDD watches for
** specific events.  For example, when data at
** certain stream position is processed, the
** PDD tells the stream handler of the encountered
** event (eg setpositionadvise on every ...)
** The PDD has to have a mechanish of noticing
** these events.
*/
ULONG Enqueue_Event (HEVENT hEvent, ULONG ulCuePoint)
{
   // Generate new event in queue
   // If no room, return error
   // Set event
   return (0);
}

ULONG Dequeue_Event (HEVENT hEvent)
{
   // Search queue for specified event
   // If not found, return error
   // Fix pointers to bypass this node.
   // Free the node memory
   return (0);
}
