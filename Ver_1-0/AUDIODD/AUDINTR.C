/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  AUDINTR.C    (TEMPLATE SAMPLE)                        */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver interrupt handler                  */
/*                                                                          */
/* LINKAGE: near calls                                                      */
/*                                                                          */
/* DESCRIPTION:                                                             */
/*    Processes data stream and calls stream handler via                    */
/*    IDC (Inter Device Communication) to get/give data buffers.            */
/*                                                                          */
/************************** END OF SPECIFICATIONS ***************************/

#define  INCL_DOS
#define  INCL_DOSINFOSEG
#include <os2.h>

#include <os2medef.h>
#include <ssm.h>         // Sync Stream Manager
#include <shdd.h>
#include <audio.h>
#include "audiodd.h"

extern ULONG operation;

//****************************************************
// This procedure is called at device interrupt time
//****************************************************
VOID InterruptHandler()
{
        SHD_REPORTINT   ShdInt ;
        PSTREAM         pStream;

        // Set up the pStream pointer
        // Code omitted - there are no streams

        /*
        ** Do not call stream handler OR get any new buffers
        ** if stream was/is stopped by handler
        **
        ** Definitions:
        **    Overrun  - Happens during "record" if the DSP (card)
        **               does not have an empty buffer to place data.
        **               This is a data loss condition.
        **    Underrun - Happsn during "playback" if the DSP (card)
        **               has no more buffers available to read.
        **               (ie No data available to write to hardware).
        **               No data loss, but will experience audio interuption.
        **
        ** So, it is the hardware that detects the error condition.
        ** The method of relaying this information to the PDD is device
        ** specific.  A common method is for the hardware to address flags
        ** in the PDD (writeable addresses set up at initialization time).
        ** Those flags would be interrogated here to determine if all is well.
        ** If things are not well, that information would be passed on to
        ** the stream handler.
        */

        // Code omitted - test overrun/underrun and report to stream handler


        if (pStream->ulFlags & STREAM_STOPPED)
          {
          EOI(); // Reset interrupt controller
          return;
          }

        //*************************************
        // If operation is PLAY, write data out
        // to the card.  If operation is RECORD
        // get data from the card's input jack
        //*************************************
        if (operation == OPERATION_PLAY)
           {
           WriteDataToCard();
           }
        else
           {
           ReadDataFromCard(); // Operation is record
           }

        //*****************************************
        // Report interrupt to audio stream handler
        //*****************************************

        ShdInt.ulFunction = SHD_REPORT_INT ;
        ShdInt.hStream    = pStream->hStream;
        ShdInt.ulFlag     = SHD_WRITE_COMPLETE ;

        //*****************************************
        // If overrun or underrun flags are set,
        // return error code to stream handler, set ERROR flag
        //*****************************************

        //************************************
        // If pBuffer is NULL, report UNDERRUN
        // to ADSH, so he can shut me down
        //************************************

        // IDC call to Stream Handler
        ((PSHDFN)pStream->ADSHEntry)(&ShdInt);

        //***************************
        // Reset interrupt controller
        //***************************
        EOI();

        return;
}
