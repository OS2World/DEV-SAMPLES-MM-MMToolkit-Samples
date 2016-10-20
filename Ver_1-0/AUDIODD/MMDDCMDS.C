/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  MMDDCMDS.C        (TEMPLATE SAMPLE)                   */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver IDC routines for MMPM/2            */
/*                                                                          */
/* LINKAGE: near calls                                                      */
/*                                                                          */
/* DESCRIPTION: Process IOCTLS received from the Amp Mixer and stream data  */
/*              to/from the stream handler.                                 */
/************************** END OF SPECIFICATIONS ***************************/


//****************************************************************************
//                     I N C L U D E S
//****************************************************************************

#define         INCL_NOPMAPI
#define         INCL_BASE
#define         INCL_DOS
#define         INCL_DOSDEVICES
#define         INCL_ERRORS
#include        <os2.h>

#include        <os2medef.h>
#include        <meerror.h>     // Multimedia error return codes
#include        <ssm.h>         // Sync Stream Manager
#include        <shdd.h>        // IDC interface
#include        <audio.h>
#include        "audiodd.h"
#include        "audsubs.h"
#include        "cdevhlp.h"

//**************************************************
//      IDC MMPM/2 routines
//**************************************************

VOID    ShdReportInt (ULONG);

RC  FAR DDCMDInternalEntryPoint(PDDCMDCOMMON pCommon);  // Called from stream handler
RC      DDCmdSetup( PDDCMDSETUP pSetup );
RC      DDCmdRead( PDDCMDREADWRITE pRead );
RC      DDCmdWrite( PDDCMDREADWRITE pWrite );
RC      DDCmdStatus( PDDCMDSTATUS pStatus );
RC      DDCmdControl( PDDCMDCONTROL pControl );
RC      DDCmdRegister( PDDCMDREGISTER pRegister );
RC      DDCmdDeRegister( PDDCMDDEREGISTER pDeRegister );
RC      GetStreamEntry(PSTREAM far *ppStream, HSTREAM hStream);
/*****************************************************************************
*                       E X T E R N S
******************************************************************************/

extern  GLOBAL GlobalTable;
extern  ULONG   operation[];    // Holds operation that we are prepared for
extern  ULONG trk_array[];      // Number of supported tracks is
extern  USHORT trk;             // device dependent field.
extern  SHORT mode[];

extern MCI_AUDIO_IOBUFFER xmitio;
extern MCI_AUDIO_IOBUFFER recio;
extern PROTOCOLTABLE ProtocolTable[NPROTOCOLS];

/*****************************************************************************
*                          D A T A
******************************************************************************/

//*******************************************************
// IDC Command function jump table                      *
//*******************************************************
ULONG   (*DDCMDFuncs[])(PVOID pCommon) = {
                DDCmdSetup,                         // 0
                DDCmdRead,                          // 1
                DDCmdWrite,                         // 2
                DDCmdStatus,                        // 3
                DDCmdControl,                       // 4
                DDCmdRegister,                      // 5
                DDCmdDeRegister                     // 6
                };
USHORT  MaxDDCMDFuncs = sizeof(DDCMDFuncs)/sizeof(USHORT);
/*****************************************************************************
*                          C O D E
******************************************************************************/


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          DDCMDInternalEntryPoint
*
* DESCRIPTIVE NAME: IDC Entry point enabling Audio Stream Handler to
*                   communicate with the Audio PDD.
*
* FUNCTION: To execute the command requested by Audio Stream Handler
*
* NOTES:    This routine is called from the IDCEntry routine located
*           in the  STARTUP.ASM  file.
*
* ENTRY POINTS:     DDCmdInternalEntryPoint()
*     LINKAGE:      CALL FAR
*
* INPUT:        Pointer to command-dependent parameter structure.
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_FUNCTION
*               ERROR_INVALID_BLOCK
* EFFECTS:
*
* INTERNAL REFERENCES: DDCMD functions
*
* EXTERNAL REFERENCES: None
*
*********************** END OF SPECIFICATIONS **********************/

RC  far DDCMDInternalEntryPoint( PDDCMDCOMMON pCommon )
{

        if (pCommon==NULL)
                return (ERROR_INVALID_BLOCK) ;

        //*********************************
        // Process DDCMD requested by stream handler
        //*********************************
        if (pCommon->ulFunction > (ULONG)MaxDDCMDFuncs)    // check for valid function
                return(ERROR_INVALID_FUNCTION);

        return(DDCMDFuncs[pCommon->ulFunction](pCommon));  // call DDCMD function


}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:              DDCmdSetup
*
* DESCRIPTIVE NAME:     Sets up the audio device
*
* NOTES:        Match hStream parameter that Stream Handler will pass.
*               Ensure that the PDD is set up with the
*               correct DSP module, using the operation field
                of the structure matching hStream.
*
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_STREAM
*               ERROR_INVALID_REQUEST
*               ERROR_INVALID_FUNCTION
*
* INPUTS:
*               ulFunction              DDCMD_SETUP
*               hStream                 Stream handle to set up
*               pSetupParm              Not used
*               ulSetupParmSize         Not used
*
*********************** END OF SPECIFICATIONS **********************/

RC      DDCmdSetup( PDDCMDSETUP pSetup )
{
    PSTREAM pStream;    // pointer to Stream table
    RC      rc;
    USHORT  i;

    //***********************************************
    // Search stream table for matching stream handle
    //***********************************************
    pStream = GlobalTable.paStream;
    if (rc = GetStreamEntry(&pStream, pSetup->hStream))
        return(rc);

    //***********************************************
    // Ensure that this stream opertaion matches the
    // operation that the PDD is set to perform.
    // Put another way.  If we were setup for a operation
    // different than that which the stream handler is
    // asking us to perform, then we are not prepared.
    // In this case, return error to stream handler.
    //***********************************************
    switch (operation[trk])     {

        case OPERATION_PLAY:
            if (pStream->ulOperation != STREAM_OPERATION_CONSUME)      {
                return (ERROR_INVALID_REQUEST) ;
            }

            break ;


        case OPERATION_RECORD:
            if (pStream->ulOperation != STREAM_OPERATION_PRODUCE)    {
                return (ERROR_INVALID_REQUEST) ;
            }

            break ;


        case PLAY_AND_RECORD:       /* Do nothing for now */
            break ;


        default:
            return (ERROR_INVALID_FUNCTION) ;       /* Stream not initialized */
    }

    /*
    ** Set up tables to have right information
    ** for the requested function.
    */
    for (i=0; i<GlobalTable.usMaxNumStreams; i++) {
            if (GlobalTable.paStream[i].hStream != -1) {                              // check for only valid streams
                    if ((GlobalTable.paStream[i].ulFlags & STREAM_STREAMING)          // is stream running?
                       && (GlobalTable.paStream[i].hStream != pStream->hStream)       // is running stream the stream to context switch?
                       && (GlobalTable.paStream[i].usTrackNum == pStream->usTrackNum))  // is running stream using the same channel as the context switch stream?
                            return(ERROR_STREAM_NOT_STOP);                        // yes, so return error
            }
    }

    //************************************************************************
    // Stream handler needs to adjust our CumTime when doing a setup, this is
    // because the stream handler might have been seeked and all events that he sends will
    // be relative to the seeked time.
    //*************************************************************************
    SetStreamTime (pStream, *((unsigned long far *)pSetup->pSetupParm) );

    return (NO_ERROR);
}

/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          DDCmdRead
*
* DESCRIPTIVE NAME:     Read from the audio device (Recording Mode)
*
* FUNCTION:     Allows the Audio Stream Handler to read buffers from the audio device.
*
* NOTES:
*               This routine chains the incoming buffers from the stream
*               handler into Iobuf packets and updates the next index buffer
*               packet.  The current index is only updated during
*               interrupt time in the get_next_rbuf routine.
*
*               This routine is called at interrupt and non-interrupt time.
*
*
* ENTRY POINTS: DDCmdRead()
*     LINKAGE:  near
*
* INPUT:        Parm pointer
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_STREAM
*               ERROR_INVALID_BLOCK
*               ERROR_STREAM_NOT_ACTIVE
*
*
* INPUTS:
*               ulFunction              DDCMD_READ
*               hStream                 Stream handle
*               pBuffer                 Address of buffer to record data to
*               ulBufferSize            Size of pBuffer
*
*********************** END OF SPECIFICATIONS **********************/

RC      DDCmdRead( PDDCMDREADWRITE pRead )
{
    USHORT  Current, Next, Previous;
    PSTREAM pStream;
    RC      rc;


    //*********************************
    // Validate data buffer pointer
    // that was passed in from stream handler
    //*********************************

    if (pRead->pBuffer==NULL)   {
            return (ERROR_INVALID_BLOCK) ;
    }

    pStream = GlobalTable.paStream;
    if (rc = GetStreamEntry(&pStream, pRead->hStream))
        return(rc);

    //************
    // Set indexes that point to IOBuff packets
    //************
    Current = pStream->usCurrIOBuffIndex;
    Next = pStream->usNextIOBuffIndex;

    /**************************************************
    **  Copy buffer pointer to "next"
    **
    **  pBufPhys - Physical address
    **
    **  Buffer   - Physical address to virtual
    **  Head     - beginning of buffer   (VIRTUAL)
    **  Tail     - end of buffer         (VIRTUAL)
    ***************************************************/

    //*************************************************
    // Fill IOBuff packet with data from pRead - size of data,
    // address, usRunFlags, and place it in the queue.
    // If there's a previous buffer in the queue, the
    // new packet will be filled in with the previous
    // buffer's values for position and delay.
    // Otherwise these values will be set to zero.
    //*************************************************

    pStream->IOBuff[Next].lSize   = pRead->ulBufferSize;
    pStream->IOBuff[Next].lCount  = 0;
    pStream->IOBuff[Next].pBuffer = pRead->pBuffer;
    pStream->IOBuff[Next].lSize   = pRead->ulBufferSize;
    pStream->IOBuff[Next].pHead   = pStream->IOBuff[Next].pBuffer;
    pStream->IOBuff[Next].pTail   =
       pStream->IOBuff[Next].pHead + pRead->ulBufferSize;
    pStream->IOBuff[Next].usRunFlags  =
       pStream->IOBuff[Current].usRunFlags & (USHORT)~IOB_OVERRUN;

    if (Current!=Next) {        /* There is a previous buffer */

        if (Next == 0)  Previous = MAXIOBUFFS - 1 ;
        else    Previous = Next - 1 ;

        pStream->IOBuff[Next].ulposition  = pStream->IOBuff[Previous].ulposition;
        pStream->IOBuff[Next].lDelay     = pStream->IOBuff[Previous].lDelay;
        // Do not set count when recording
    }

    else {                      /* Set ulposition, lDelay to 0 */

        pStream->IOBuff[Next].ulposition  = 0;
        pStream->IOBuff[Next].lDelay     = 0;
    }



    //***********************************************
    // Increment next buffer index and check for wrap
    //***********************************************
    if ((++Next == MAXIOBUFFS))
        Next = 0 ;
    pStream->usNextIOBuffIndex = Next;

    return( NO_ERROR );
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:              DDCmdWrite
*
* DESCRIPTIVE NAME:     Write to the audio device using AUDIO_HPI
*                       (Playback Mode)
*
* FUNCTION:     Allows the Audio Stream Handlerto write buffers to
*               the audio device by passing buffers (IOBuffs) to
*               the physical hardware device.
*
* NOTES:        StreamNumber is set at the IDC entry point
*               This routine chains the incoming buffers from the stream handler
*               into Iobuf packets and updates the next index buffer
*               packet.  The current index is only updated during
*               interrupt time in the get_next_xbuf routine.
*
*               This routine is called at interrupt and non-interrupt time.
*
* ENTRY POINTS: DDCmdWrite()
*     LINKAGE: near
*
* INPUT:       Parm pointer
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_STREAM
*               ERROR_INVALID_BLOCK
*
* INPUTS:
*               ulFunction              DDCMD_WRITE
*               hStream                 Stream handle
*               pBuffer                 Address of buffer to play data from
*               ulBufferSize            Size of pBuffer
*
*********************** END OF SPECIFICATIONS **********************/

RC  DDCmdWrite( PDDCMDREADWRITE pWrite )
{
    USHORT  Current, Next, Previous ;
    PSTREAM pStream;
    RC      rc;

    //**********************************
    // Validate data buffer pointer
    //**********************************

    if (pWrite->pBuffer==NULL)  {
            return (ERROR_INVALID_BLOCK) ;
    }

    //***********************************************
    // Search stream table for matching stream handle
    //***********************************************
    pStream = GlobalTable.paStream;
    if (rc = GetStreamEntry(&pStream, pWrite->hStream))
        return(rc);

    //*************
    // Set indexes
    //*************
    Current = pStream->usCurrIOBuffIndex;
    Next = pStream->usNextIOBuffIndex;

    /**************************************************
        Copy buffer pointer to "next"

        pBufPhys - Physical address is sent from stream handler

        Buffer   - Physical address to virtual
        Tail     - beginning of buffer   (VIRTUAL)
        Head     - end of buffer         (VIRTUAL)
     **************************************************/

    //**************************************************
    // Fill buffer with data from pWrite - size of data,
    // address, usRunFlags, and place it in the queue.
    // If there's a previous buffer in the queue, the
    // new buffer will be filled in with the previous
    // buffer's values for ulposition and lDelay. Other-
    // wise ulposition will be set to zero, lDelay to 1.
    //**************************************************
    pStream->IOBuff[Next].lSize   = pWrite->ulBufferSize;
    pStream->IOBuff[Next].lCount  = pWrite->ulBufferSize;
    pStream->IOBuff[Next].pBuffer = pWrite->pBuffer;
    pStream->IOBuff[Next].pTail      = pStream->IOBuff[Next].pBuffer;
    pStream->IOBuff[Next].pHead      =
            pStream->IOBuff[Next].pTail + pWrite->ulBufferSize;
    pStream->IOBuff[Next].usRunFlags  = pStream->IOBuff[Current].usRunFlags &
                                        (USHORT)~IOB_UNDERRUN;

    if (Current!=Next) {        /* There is a previous buffer */

        if (Next == 0)
                Previous = MAXIOBUFFS - 1 ;
        else
                Previous = Next - 1 ;

        pStream->IOBuff[Previous].lCount += pWrite->ulBufferSize;
    }

    pStream->IOBuff[Next].ulposition  = 0;
    pStream->IOBuff[Next].lDelay     = 1;


    //***********************************************
    // Increment next buffer index and check for wrap
    //***********************************************
    if ((++Next == MAXIOBUFFS))
        Next = 0 ;
    pStream->usNextIOBuffIndex = Next;

    return( NO_ERROR );
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          DDCmdStatus
*
* DESCRIPTIVE NAME:     Query current ulposition (stream time)
*
* FUNCTION:     Allows the Audio stream handler to get the current
*               stream time ulposition value in milliseconds.
*
* NOTES:
*               This routine will return the current real-time
*               "stream-time" for the stream.
*
*               This routine is called at non-interrupt time.
*
* ENTRY POINTS: DDCmdStatus()
*     LINKAGE: near
*
* INPUT:
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_STREAM
*
* INPUTS:
*               ulFunction              DDCMD_STATUS
*               hStream                 Stream handle
*
*********************** END OF SPECIFICATIONS **********************/

RC      DDCmdStatus(PDDCMDSTATUS pStatus )
{
    PSTREAM pStream;
    RC      rc;

    pStream = GlobalTable.paStream;
    if (rc = GetStreamEntry(&pStream, pStatus->hStream))
        return(rc);

    /* Get the current ulposition */
    GetStreamTime (pStream);

    //********************************************************
    // Return far pointer containing real-time position to stream handler
    //********************************************************
    pStatus->ulStatusSize = sizeof(recio.ulposition);
    pStatus->pStatus = &pStream->ulCumTime;

    return( NO_ERROR );
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          DDCmdControl
*
* DESCRIPTIVE NAME:
*
* FUNCTION:     Maps the AUDIO_CONTROL IDC commands to their IOCTL
*               equilivent.
*
* NOTES:    Since the entire routine uses the AUDIO_HPI calls,
*           Just set the current buffer's usRunFlags to the right command
*           Command from stream handler comes in pControl->ulCmd.
*           This routine sets flags, IOBuff and pStream structures.
*           These will be referenced at interrupt time for command execution.
*           For some operations, we will call stream handler directly.
*
* ENTRY POINTS: DDCmdControl()
*     LINKAGE: near
*
* INPUT:       Parm pointer
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_FUNCTION
*               ERROR_STREAM_NOT_STARTED
*               ERROR_INVALID_SEQUENCE
*               ERROR_INVALID_STREAM
*               ERROR_TOO_MANY_EVENTS
*
* INPUTS:
*               ulFunction              DDCMD_CONTROL
*               hStream                 Stream handle
*               ulCmd                   Specific control command
*               pParm                   Not used
*               ulParmSize              Not used
*
*********************** END OF SPECIFICATIONS **********************/

RC  DDCmdControl(PDDCMDCONTROL pControl)
{
    USHORT Current, i ;
    PSTREAM pStream;
    RC      rc;
    ULONG   ulCuePoint;


    pStream = GlobalTable.paStream;
    if (rc = GetStreamEntry(&pStream, pControl->hStream))
        return(rc);
    Current = pStream->usCurrIOBuffIndex;


    //****************************************
    // Perform specific DDCMD_CONTROL function
    //****************************************
    switch (pControl->ulCmd)    {
        case    DDCMD_START:

                //*********************************************
                // ... PAUSE - START ... is an invalid sequence
                //*********************************************
                if (pStream->IOBuff[Current].usRunFlags & IOB_PAUSED) {
                        return (ERROR_INVALID_SEQUENCE);
                }


                //*******************************************
                // stream handler told us to start and we have enough
                // full buffers, so send this buffer to the
                // audio card.  If operation is PLAY, the
                // routine will output the contents of the
                // buffer to the card.  If operation is RECORD
                // the routine will fill the buffer with data
                // from the card's input jack.
                //******************************************** */
                pStream->ulFlags |= STREAM_STREAMING;
                pStream->ulFlags &= ~STREAM_STOPPED;
                if (operation[trk] == OPERATION_PLAY) {
                        Audio_IOCTL_Hpi (pControl->pParm);
                }
                else    Audio_IOCTL_Hpi (pControl->pParm);


                //*************************
                // Set all IOBuffs to START
                //*************************
                for (i=0; i<MAXIOBUFFS; i++) {
                        pStream->IOBuff[i].usRunFlags |= IOB_STARTED;
                }
                break ;


           case DDCMD_STOP:
                //********************************************************
                // because we always write to the "next" buffer and play
                // from the "current" buffer, reset the indexes to point
                // at the same packet. So when a re-start comes in we will
                // not play a null buffer.
                //********************************************************

                Current = pStream->usCurrIOBuffIndex = pStream->usNextIOBuffIndex = 0;

                pStream->ulFlags |= STREAM_STOPPED;
                pStream->ulFlags &= ~STREAM_STREAMING;
                pStream->ulFlags &= ~STREAM_PAUSED;

                /************************************************
                 Do this so that PDD does not return an old buffer
                 to handler after handler received a STOP-DISCARD.
                 ************************************************/
                for ( i=0; i<MAXIOBUFFS; i++ )  {
                    pStream->IOBuff[i].usRunFlags &= ~IOB_RUNNING;
                    pStream->IOBuff[i].usRunFlags &= ~IOB_STARTED;
                    pStream->IOBuff[i].usRunFlags &= ~IOB_PAUSED;
                    pStream->IOBuff[i].usRunFlags |= IOB_STOPPED;
                    pStream->IOBuff[i].lSize      = 0;
                    pStream->IOBuff[i].pBuffer   = NULL;
                    pStream->IOBuff[i].lCount    = 0;
                    pStream->IOBuff[i].ulposition  = 0;
                    pStream->IOBuff[i].lDelay     = 0;
                    pStream->IOBuff[i].pBuffer    = NULL;
                }

                // Always return stream time when
                // stopping or pausing device.
                pControl->pParm = GetStreamTime(pStream);

                pControl->ulParmSize = sizeof(recio.ulposition);

                break ;


        case    DDCMD_PAUSE:

                //********************************************
                // trying to PAUSE a non-started stream = error
                //********************************************
                if (!(pStream->IOBuff[Current].usRunFlags & IOB_STARTED))     {
                        return (ERROR_STREAM_NOT_STARTED);
                }

                pStream->IOBuff[Current].usRunFlags |= IOB_PAUSED;
                pStream->ulFlags |= STREAM_PAUSED;
                pStream->ulFlags &= ~STREAM_STREAMING;

                // Always return stream time when
                // stopping or pausing device.
                pControl->pParm = GetStreamTime(pStream);
                pControl->ulParmSize = sizeof(recio.ulposition);
                break ;

        case    DDCMD_RESUME:

                //*********************************************
                // trying to RESUME a stoped/non-paused stream = error
                //*********************************************
                if (!(pStream->IOBuff[Current].usRunFlags & IOB_STARTED))    {
                        return (ERROR_INVALID_SEQUENCE);
                }

                if (operation[trk] == OPERATION_PLAY) {
                        Audio_IOCTL_Hpi (pControl->pParm);
                }
                else    Audio_IOCTL_Hpi (pControl->pParm);

                pStream->IOBuff[Current].usRunFlags &= ~IOB_PAUSED;
                pStream->ulFlags |= STREAM_STREAMING;

                break ;


        case    DDCMD_ENABLE_EVENT:
                // Create an event with specific cuepoint.
                // This PDD will detect the cuepoint and report
                // its occurance to the stream handler via
                // SHD_REPORT_EVENT

                ulCuePoint = *((unsigned long far *)pControl->pParm);
                rc = Enqueue_Event (pControl->hEvent, ulCuePoint);
                if (rc)
                   {
                   return (ERROR_TOO_MANY_EVENTS);
                   }
                break ;

        case    DDCMD_DISABLE_EVENT:

                rc = Dequeue_Event (pControl->hEvent);

                break ;

        default:
                return (ERROR_INVALID_FUNCTION) ;

    }     /* switch */

    return( NO_ERROR );
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          DDCmdRegister
*
* DESCRIPTIVE NAME: Register stream with this PDD
*
* FUNCTION: Initialize stream.
*
* NOTES:    Will receive the handler ID, handle of stream instance,
*           address of entry point from stream handler.
*           Set waiting flag (waiting for data)
*           Set usRunFlags = 0
*           Set No_circular_buffer flag (0x80)
*           Set protocol info for stream handler
*           Set Address type to Physical
*
* ENTRY POINTS:DDCmdRegister()
*     LINKAGE: near
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT-ERROR:   ERROR_INVALID_STREAM
*               ERROR_HNDLR_REGISTERED
*               ERROR_INVALID_FUNCTION
*               ERROR_INVALID_SPCBKEY
*
* INPUTS:
*               ulFunction              DDCMD_REG_STREAM
*               hStream                 Stream handle
*               ulSysFileNum            Device handle so PDD can map device instance to hStream
*               pSHDEntryPoint          Stream handler entry point
*               ulStreamOperation       Record (PRODUCE) or Play (CONSUME)
*               spcbkey                 Protocol info which determines outputs
*
* OUTPUTS:
*               ulBufSize               Buffer size in bytes for SPCB
*               ulNumBufs               # of buffers for SPCB
*               ulAddressType           Address type of data buffer
*               ulBytesPerUnit          Bytes per unit
*               mmtimePerUnit           MMTIME per unit
*
*********************** END OF SPECIFICATIONS **********************/

RC  DDCmdRegister(PDDCMDREGISTER pRegister)
{

    USHORT      i;
    PSTREAM     pStream;
    BOOL        bFound;
    RC          rc;

    if (pRegister->hStream == -1)
        return (ERROR_INVALID_STREAM) ;    /*   Stream handle invalid   */

    if (pRegister->ulSysFileNum == DATATYPE_NULL)
        return(ERROR_INITIALIZATION);

    if ((pRegister->ulStreamOperation != STREAM_OPERATION_CONSUME) &&
        (pRegister->ulStreamOperation != STREAM_OPERATION_PRODUCE))
                return (ERROR_INVALID_FUNCTION);


    //******************************************
    // Do protocol table lookup, using DataType,
    // DataSubType as keys and return ulBufSize,
    // ulNumBufs, ulAddressType, mmtimePerUnit;
    // Verify requested stream characteristics
    // match our capabilities.
    //******************************************
    for ( i=0; i<NPROTOCOLS; i++ )  {

        if (ProtocolTable[i].ulDataType == pRegister->spcbkey.ulDataType) {
            if (ProtocolTable[i].ulDataSubType == pRegister->spcbkey.ulDataSubType) {
                //**********************************************
                // We found a match for data type, data sub type
                //**********************************************
                break ;
            }
        }
    }


    //***************
    // No match found
    //***************
    if (i==NPROTOCOLS) {
        return (ERROR_INVALID_SPCBKEY) ;
    }

    //***************************
    // Match found:
    //   Pass back protocol ("stream characteristics")
    //   to stream handler.
    //***************************
    else {                                  /* Match found */
        pRegister->ulBufSize       = ProtocolTable[i].ulBufSize ;
        pRegister->ulNumBufs       = ProtocolTable[i].ulNumBufs ;
        pRegister->mmtimePerUnit   = 1;

        //****************************************************
        // Bytes per unit =
        //
        //  Samples     Channels     Bits      Byte     Sec
        //  -------  *           *  ------  *  ---- *  ------
        //    Sec                   Sample     Bits    MMTIME
        //****************************************************
        pRegister->ulBytesPerUnit  = ((ProtocolTable[i].ulSampleRate * ProtocolTable[i].usChannels
                                     * ProtocolTable[i].usBitsPerSample) / 8) /3;

    }
    //********************************************************************
    // Tell stream handler what type of data buffer pointer to reference
    //********************************************************************
    pRegister->ulAddressType = ADDRESS_TYPE_PHYSICAL ;

    bFound = FALSE;
    for (i=0; i<NUM_TRACKS; i++)
            if (trk_array[i] == pRegister->ulSysFileNum) {
                bFound = TRUE;
                trk = i;
            }
    if (!bFound)
        return(ERROR_INVALID_STREAM);

    //*************************************************************
    // Initialize pStream to point at the first entry for track trk
    //************************************************************* */
    pStream = GlobalTable.paStream;
    if (!(rc = GetStreamEntry(&pStream, pRegister->hStream)))
                return(ERROR_HNDLR_REGISTERED);
    pStream = GlobalTable.paStream;                         // no match, so create stream
    i=0;
    while(pStream->hStream != -1) {                     // find an empty stream entry
                if (++i >= GlobalTable.usMaxNumStreams)
                        return(ERROR_STREAM_CREATION);
                pStream++;
        }

    //**************************************
    // Found empty stream entry, so use it
    // Fill Stream structure
    //**************************************
    for ( i=0; i<MAXIOBUFFS; i++ )  {
        pStream->IOBuff[i].lSize      = 0;
        pStream->IOBuff[i].pHead      = NULL;
        pStream->IOBuff[i].pTail      = NULL;
        pStream->IOBuff[i].lCount     = 0;
        pStream->IOBuff[i].ulposition  = 0;
        pStream->IOBuff[i].lDelay     = 0;
        pStream->IOBuff[i].usRunFlags |= IOB_CHAIN_BUFFERS;
        pStream->IOBuff[i].pBuffer    = NULL;
    }

    //********************************
    // Save register info in structure
    //********************************
    pStream->hStream        = pRegister->hStream;
    pStream->ulFlags        = STREAM_REGISTERED;
    pStream->ulOperation    = pRegister->ulStreamOperation;
    pStream->ulSysFileNum   = trk_array[trk];
    pStream->usTrackNum     = trk;
    pStream->usCurrIOBuffIndex = 0;
    pStream->usNextIOBuffIndex = 0;
    (PSHDFN)pStream->ADSHEntry = pRegister->pSHDEntryPoint;

    return( NO_ERROR );
}


/********************* START OF SPECIFICATIONS *********************
*
* SUBROUTINE NAME:          DDCmdDeRegister
*
* DESCRIPTIVE NAME: Deregister this stream
*
* FUNCTION:     Remove stream instance from this PDD
*
* NOTES:        Done by setting table handle to value -1
*
* ENTRY POINTS:
*     LINKAGE:
*
* EXIT-NORMAL:  NO_ERROR
*
* EXIT_ERROR:   ERROR_INVALID_STREAM
*
* INPUTS:
*               ulFunction              DDCMD_DEREG_STREAM
*               hStream                 Stream handle
*
*********************** END OF SPECIFICATIONS **********************/

RC  DDCmdDeRegister(PDDCMDDEREGISTER pDeRegister)
{
    USHORT  i ;
    PSTREAM pStream;
    RC          rc;


    //**************************************************************
    // Check table to see if stream is registered and a valid handle
    //**************************************************************
    pStream = GlobalTable.paStream;
    if (rc = GetStreamEntry(&pStream, pDeRegister->hStream))
                return(rc);

    //************************
    // De-activate this stream
    // and clear all flags
    //************************

    pStream->hStream = -1;                      // make stream available
    pStream->ulFlags = 0;                       // clear flags

    for (i=0; i<MAXIOBUFFS; i++)    {
            pStream->IOBuff[i].usRunFlags = 0;
    }

    return( NO_ERROR );
}


/********************* START OF SPECIFICATIONS *********************
* SUBROUTINE NAME: GetStreamEntry
*
* DESCRIPTIVE NAME: Get the stream table entry.
*
* FUNCTION: To search the stream table finding a match with the given parm.
*
* NOTES: This routine is called internally.
*
* ENTRY POINTS:
*
*     LINKAGE:   CALL near
*
* INPUT: pointer to stream table, stream handle to find
*
* EXIT-NORMAL: NO_ERROR
*
* EXIT_ERROR: ERROR_INVALID_STREAM if stream not found in table
*
* INTERNAL REFERENCES: none
*
* EXTERNAL REFERENCES: none
*
*********************** END OF SPECIFICATIONS **********************/
RC      GetStreamEntry(PSTREAM far *ppStream, HSTREAM hStream)
{
        USHORT  i;
        PSTREAM pStream;

        i=0;
        pStream = *ppStream;
        while(pStream->hStream != hStream) {            // find stream entry
                if (++i >= GlobalTable.usMaxNumStreams)
                        return(ERROR_INVALID_STREAM);
                pStream++;
        };
        *ppStream = pStream;
        return(NO_ERROR);
}
