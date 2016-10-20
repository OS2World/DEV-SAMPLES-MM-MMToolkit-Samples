/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  AUDIODD.C         (TEMPLATE SAMPLE)                   */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver strategy and IOCTL routines        */
/*                                                                          */
/* LINKAGE: Called from Startup.asm                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/*    This audio device driver is an OS/2 16-bit Physical Device driver     */
/*    designed to demonstrate audio device communication with the           */
/*    MMPM/2 stream handler.                                                */
/*    Information on OS/2 Physical Device Drivers architecture and          */
/*    programming is in the "OS/2 2.0 Physical Device Driver reference"     */
/*                                                                          */
/*    MMPM/2 streaming architecture is outlined in                          */
/*    "MMPM/2 Programmers Workbook"                                         */
/*       - Data Streaming & synchronization chapter                         */
/*    Specifix information on interdevice communication is in the           */
/*    same document in the "Stream Handler" Chapter in the section on       */
/*    "Interdevice-driver-communication                                     */
/************************** END OF SPECIFICATIONS ***************************/


#define NUMGDTS 1L
#define  INCL_DOSINFOSEG
#include <os2.h>

#include <os2medef.h>
#include <ssm.h>
#include <audio.h>
#include "audiodd.h"
#include "audsubs.h"
#include "cdevhlp.h"

extern ULONG   (*IOCTLFuncs[])(PREQPACKET rp);
extern MaxIOCTLFuncs;
extern ULONG   (*AudioIOCTLFuncs[])(PVOID pParm);
extern MaxAudioIOCTLFuncs;
extern FPVOID  DevHlp;
extern EndOfData;
extern GLOBAL GlobalTable;

int   mode = MIDI;
unsigned long flags = 0;
long  srate;
long  bits_per_sample;
long  bsize;
int   channels;
unsigned long  operation;
int   position_type;


/*********************************************************************/
/* STRATEGY_C                                                        */
/* DD strategy control after entry from assembler Strategy routine.  */
/*********************************************************************/

ULONG   Strategy_c(PREQPACKET rp)
{
        if (rp->RPcommand > (UCHAR)MaxIOCTLFuncs) // check for valid function
                return(RPDONE | RPERR | RPBADCMD);

        return(IOCTLFuncs[rp->RPcommand](rp));  // call request function
                                                // then return its rc
}


/*******************************************************************************/
/*                         INVALID REQUEST                                     */
/*******************************************************************************/
ULONG   IOCTL_Invalid(PREQPACKET rp)
{
        return(RPDONE | RPERR | RPBADCMD);
}

/*****************************************************************************/
/*                      Hardware communication routines                      */
/* Implementation of these functions is dependant on interfaces and physical */
/* characteristics of the card.                                              */
/* IOCTLs listed below are OS/2 defined IOCTLs.  Additional IOCTLs arrive    */
/* from applications and MMPM/2 through the OS/2 generic IOCTL.              */
/*****************************************************************************/

/*****************************************************************************/
/*                              READ                                         */
/* Stub, read from device.  Implemention dependant on hardware architecture. */
/*****************************************************************************/
ULONG   IOCTL_Read(PREQPACKET rp)
{
        ReadDataFromCard();     // Stub routine
        return(RPDONE);
}

/*****************************************************************************/
/*                        NONDESTRUCTIVE READ                                */
/* Stub, routine performing nondestructive read operation.                   */
/* This read operation reads data from a buffer without removing that data.  */
/*****************************************************************************/
ULONG   IOCTL_NondestructiveRead(PREQPACKET rp)
{
        return(RPDONE);
}

/*****************************************************************************/
/*                           READ STATUS                                     */
/* Stub routine                                                              */
/*****************************************************************************/
ULONG   IOCTL_ReadStatus(PREQPACKET rp)
{
        ReadStatus();
        return(RPDONE);
}

/*****************************************************************************/
/*                            FLUSH INPUT                                    */
/*****************************************************************************/
ULONG   IOCTL_FlushInput(PREQPACKET rp)
{
        FlushInputBuffers();
        return(RPDONE);
}

/*****************************************************************************/
/*                          WRITE                                            */
/* Stub, write to device.                                                    */
/*****************************************************************************/
ULONG   IOCTL_Write(PREQPACKET rp)
{
        WriteDataToCard();
        return(RPDONE);
}

/*****************************************************************************/
/*                         WRITE  STATUS                                     */
/* Stub routine                                                              */
/*****************************************************************************/
ULONG   IOCTL_WriteStatus(PREQPACKET rp)
{
        WriteStatus();
        return (RPDONE);
}

/*****************************************************************************/
/*                        FLUSH OUTPUT                                       */
/*****************************************************************************/
ULONG   IOCTL_FlushOutput(PREQPACKET rp)
{
        FlushOutputBuffers();
        return (RPDONE);
}

/*****************************************************************************/
/*                        OPEN                                               */
/* Prepare device for operations.                                            */
/*****************************************************************************/
ULONG   IOCTL_Open(PREQPACKET rp)
{
        /*****************************
        ** Open device, hook interrupts
        **
        ** For this sample, no interrupts will be
        ** hooked, generated or received as there
        ** is no hardare.
        ** Still, AUDINTR.C contains a sample
        ** interrupt handler demonstrating
        ** communication with the MMPM/2 stream handler.
        ******************************/
        DevOpen();
        return (RPDONE);
}

/*****************************************************************************/
/*                          CLOSE                                            */
/* Opposite of open.  Clean up system resources.  If any activity is in      */
/* process, it needs to be ended.                                            */
/*****************************************************************************/
ULONG   IOCTL_Close(PREQPACKET rp)
{
        //*****************************
        // Destroy streams, turn off
        // any hung notes, close device
        //*****************************
        DestroyStreams();
        DevClose();
        return(RPDONE);
}


/*****************************************************************************/
/*                      IOCTL INPUT/OUTPUT                                   */
/*****************************************************************************/
ULONG   IOCTL_Input(PREQPACKET rp)
{
        return(RPDONE);
}

ULONG   IOCTL_Output(PREQPACKET rp)
{
        return(RPDONE);
}



/*****************************************************************************/
/*                       GENERIC IOCTL                                       */
/*****************************************************************************/
ULONG   IOCTL_GenIOCTL(PREQPACKET rp)                   // GENERAL IOCTL (OS/2)
{
        PVOID   pParm;
        //*****************************
        // Valid category : 0x80
        // Valid functions: 0x40 - 0x5f
        //*****************************
        if (rp->RPcommand > (UCHAR)MaxAudioIOCTLFuncs)  // Is function invalid?
                return(RPDONE | RPERR | RPBADCMD);

        pParm = rp->s.IOCtl.parameters;
        AudioIOCTLFuncs[rp->RPcommand](pParm);         // call request function
                                                       // using table of funcs
                                                       // set up at compile
                                                       // time.
        return (RPDONE);
}




/*****************************************************************************/
/*                     AUDIO_INIT IOCTL                                      */
/*****************************************************************************/
ULONG   Audio_IOCTL_Init(PVOID pParm)
{
        MCI_AUDIO_INIT FAR *pInit;

        pInit = (MCI_AUDIO_INIT FAR *)pParm;

        //****************************************
        // Copy parameters to our global variables
        // in case any of them have changed.
        //****************************************
        operation = pInit->ulOperation;
        flags = pInit->ulFlags;
        mode = pInit->sMode;
        srate = pInit->lSRate;
        bits_per_sample = pInit->lBitsPerSRate;
        bsize = pInit->lBsize;
        channels = pInit->sChannels;
        return(RPDONE);
}


/*****************************************************************************/
/*                    AUDIO_STATUS IOCTL                                     */
/* Query status of audio device in accordance with stream handler spec.      */
/*****************************************************************************/
ULONG   Audio_IOCTL_Status(PREQPACKET rp)
{
        DevIOCTLstatus();
        return(RPDONE);
}


/*****************************************************************************/
/*                    AUDIO_CONTROL IOCTL                                    */
/*****************************************************************************/
ULONG   Audio_IOCTL_Control(PREQPACKET rp)
{
        switch(rp->RPcommand)
        {
           /****************/
           /* AUDIO_CHANGE */
           /****************/
           case AUDIO_CHANGE:                  /* Change adapter  */
                   DevChange ();               /* characteristics */
                   break;

           /***************/
           /* AUDIO_START */
           /***************/
           case AUDIO_START:                   /* Start new operation        */
                   DevStart();
                   break;

           /**************/
           /* AUDIO_STOP */
           /**************/
           case AUDIO_STOP:                 /* Stop current operation        */
                   DevStop();
                   break;

           /***************/
           /* AUDIO_PAUSE */
           /***************/
           case AUDIO_PAUSE:                /* suspend current operation     */
                   DevPause();
                   break;

           /****************/
           /* AUDIO_RESUME */
           /****************/
           case AUDIO_RESUME:               /* resume a suspended operation  */
                   DevResume();
                   break;

           default:                         /* Unknown control               */
                   return (-1);             /* return an error */
                   break;
        }
        return(RPDONE);
}

/*****************************************************************************/
/*                     AUDIO_BUFFER IOCTL                                    */
/*****************************************************************************/
ULONG   Audio_IOCTL_Buffer(PREQPACKET rp)   /* AUDIO_BUFFER IOCTL */
{
        PVOID   pParm;
        pParm = rp->s.IOCtl.parameters;
        DevAudioBuffer();
        return(RPDONE);
}


/*****************************************************************************/
/*                      AUDIO_LOAD IOCTL                                     */
/*****************************************************************************/
ULONG   Audio_IOCTL_Load(PREQPACKET rp)
{
        PVOID   pParm;
        pParm = rp->s.IOCtl.parameters;
        DevIOCTLload();
        return(RPDONE);
}


/*****************************************************************************/
/*                           AUDIO_WAIT IOCTL                                */
/*****************************************************************************/
ULONG   Audio_IOCTL_Wait(PREQPACKET rp)   /* AUDIO_WAIT */
{
        PVOID   pParm;
        pParm = rp->s.IOCtl.parameters;
        DevIOCTLwait();
        return(RPDONE);
}



/*****************************************************************************/
/*                     HIGH PERFORMANCE INTERFACE IOCTL                      */
/*****************************************************************************/
ULONG   Audio_IOCTL_Hpi(PREQPACKET rp)
{
        PVOID   pParm;
        pParm = rp->s.IOCtl.parameters;

        DevIOCTLhpi ();
        return(RPDONE);
}


/*****************************************************************************/
/*                      PDD INITIALIZATION CODE                              */
/*****************************************************************************/

/*****************************************************************************/
/*                                INIT                                       */
/*****************************************************************************/
ULONG   IOCTL_Init(PREQPACKET rp)
{
        /*
        ** Set varable in data segement to point to the
        ** OS/2 DevHlp entry point.  The address of this
        ** routine is in the DevHdr data structure.
        ** This address will be referenced on later calls
        ** to DevHlp.
        */
        DevHlp = (char far *) rp->s.Init.DevHlp;

        /*
        ** As return values, tell the operating system the
        ** address (offset) of the end of the code and data segments.
        */
        rp->s.InitExit.finalCS = (OFFSET)&InitStreams;
        rp->s.InitExit.finalDS = (OFFSET)&EndOfData;

        /*
        ** Call routine to get streaming initialized with MMPM/2
        ** and then return to the kernel via our assembler code.
        */
        return(InitStreams());
}

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          InitStreams
*
* DESCRIPTIVE NAME:
*
* FUNCTION:         Initializes the stream table at device init time.
*
* NOTES: This routine is removed from the code segment after boot time.
*
*
* ENTRY POINTS:
*     LINKAGE:  Near from IOCTL_Init()
*
* INPUT:
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:
*
* EFFECTS:
*
* INTERNAL REFERENCES: none
*
* EXTERNAL REFERENCES: DevHlps
*
*********************** END OF SPECIFICATIONS **********************/


ULONG InitStreams(VOID)
{
        USHORT          i, j, BlkSize;
        USHORT          usGDT[NUMGDTS];
        PVOID           PhysAddress;
        ULONG           rc;
        BOOL            bErrFlg = FALSE;
        PSTREAM         pStream;

        /**********************************/
        /* alloc memory for stream table, */
        /**********************************/

        BlkSize = sizeof(STREAM) * GlobalTable.usMaxNumStreams;

        // Allocate in high memory first.  If it fails allocate in low mem

        if ( DevHlp_AllocPhys(BlkSize, 0, &PhysAddress) ) // Allocate high
           {
              // If that fails, allocate low
              rc = DevHlp_AllocPhys(BlkSize, 1, &PhysAddress);
              // If that fails, installation fails
              if (rc)
                 return(RPDONE | RPERR);
           }

        /*********************************************************/
        /* allocate GDTs                                         */
        /* The GDT addresses are copied from the local variable  */
        /* into the GlobalTable so they can be used when running */
        /* at ring 0 (after initialization)                      */
        /*********************************************************/
        rc = DevHlp_AllocGDTSelector(NUMGDTS, &usGDT[0]);

        /*********************************************************/
        /* Set up a temporary virtual address.                   */
        /* Note, cannot use GDT at init time as init is ring 3.  */
        /* The GDT is allocated during init, but cannot be used  */
        /* until executing at ring 0.                            */
        /*********************************************************/
        rc = DevHlp_PhysToVirt(PhysAddress,
                               BlkSize,
                               &GlobalTable.paStream);
        if (rc)
                return(RPDONE | RPERR);

        //*********************
        // Initialize stream table
        //*********************
        pStream = GlobalTable.paStream;
        for (i=0; i<GlobalTable.usMaxNumStreams; i++)
        {
                pStream->hStream = -1;
                pStream->ulFlags = 0;

                for (j=0; j<MAXIOBUFFS; j++)
                {
                       pStream->IOBuff[j].usRunFlags = 0;
                       pStream->IOBuff[j].lCount  = 1;
                       pStream->IOBuff[j].pBuffer = NULL;
                }
                pStream++;
        }

        //***********************************************
        // Map to GDT selector to address of stream table
        //***********************************************
        if (rc = DevHlp_PhysToGDTSelector(PhysAddress,  // Physical address
                                      BlkSize,          // Length
                                      usGDT[0]))        // Selector to map
                bErrFlg = TRUE;

        else
                GlobalTable.paStream =
                   MAKEP(usGDT[0],0);  // set to virtual GDT pointer

        if (bErrFlg)
                return(RPERR | RPDONE);
        else
                return(RPDONE);
}
