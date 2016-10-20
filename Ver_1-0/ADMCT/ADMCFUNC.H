/*static char *SCCSID = "@(#)admcfunc.h	13.6 92/04/15";*/
/********************** START OF SPECIFICATIONS ****************************
*
*   SOURCE FILE NAME:   ADMCFUNC.H
*
*   DESCRIPTIVE NAME: mci Waveform Driver include file.
*
*              Copyright (c) IBM Corporation  1991
*                        All Rights Reserved
*
*   STATUS: MM Extensions 1.0
*
*   FUNCTION: Component function Prototypes.
*
*
*   NOTES:
*
*       DEPENDENCIES:
*
*   EXTERNAL ENTRY POINTS: None.
*   INTERNAL ENTRY POINTS: None.
*
*   EXTERNAL REFERENCES (system):
*
*   MODIFICATION HISTORY:
*   DATE        DEVELOPER           CHANGE DESCRIPTION
*****************************************************************************/

/***********************************************
* MCI Message Support Functions
***********************************************/
RC   MCIStat (FUNCTION_PARM_BLOCK *);// Status
RC   MCIOpen (FUNCTION_PARM_BLOCK *);// Open
RC   MCICue  (FUNCTION_PARM_BLOCK *);// Cue
RC   MCIPlay (FUNCTION_PARM_BLOCK *);// Play
RC   MCIRecd (FUNCTION_PARM_BLOCK *);// Record
RC   MCIPaus (FUNCTION_PARM_BLOCK *);// Pause
RC   MCIClos (FUNCTION_PARM_BLOCK *);// Close
RC   MCISet  (FUNCTION_PARM_BLOCK *);// Set
RC   MCIScpt (FUNCTION_PARM_BLOCK *);// Set Cue Point
RC   MCISeek (FUNCTION_PARM_BLOCK *);// Seek
RC   MCIStop (FUNCTION_PARM_BLOCK *);// Stop
RC   MCILoad (FUNCTION_PARM_BLOCK *);// Load
RC   MCIInfo (FUNCTION_PARM_BLOCK *);// Info
RC   MCIResm (FUNCTION_PARM_BLOCK *);// Resume
RC   MCISave (FUNCTION_PARM_BLOCK *);// Save
RC   MCIRest (FUNCTION_PARM_BLOCK *);// Restore
RC   MCIMsvl (FUNCTION_PARM_BLOCK *);// Master Volume
RC   MCISync (FUNCTION_PARM_BLOCK *);// Sync Offset
RC   MCISavf (FUNCTION_PARM_BLOCK *);// Save File
RC   MCICnct (FUNCTION_PARM_BLOCK* );// Connectors
RC   MCIStpa (FUNCTION_PARM_BLOCK *);// Set Position Advise
RC   MCISaveFile (FUNCTION_PARM_BLOCK *);// Save Element
RC   MCICaps (FUNCTION_PARM_BLOCK *, ULONG);// Get Device Capabilities

/***************************************
* Post MCI Notification Messages
***************************************/

RC PostMDMMessage (ULONG ,USHORT,FUNCTION_PARM_BLOCK *);

/************************
* Element I/O
************************/
RC OpenFile (INSTANCE *, DWORD);      // Open Media Element

RC InstallIOProc (ULONG, MMIOINFO *); // Install IO Proc

RC GetAudioHeader (INSTANCE *);       // Get Audio Header

RC SetAudioHeader (INSTANCE *);       // Set Audio Header

/************************************************
* Streaming Support Functions
***********************************************/

RC CreateNAssocStream (HID,              /* Source Handler HID */
                       HID,              /* Target Handler HID */
                       HSTREAM *,        /* Stream Handle ()   */
                       INSTANCE *,       /* Instance Pointer   */
                       ULONG,            /* Play or Record     */
                       PEVFN);           /* Event Handler      */

RC DestroyStream (HSTREAM ); // Destroy Stream.

RC AssocMemPlayToAudioStrm (INSTANCE *,ULONG ); // PlayList Support

RC DoTillEvent (INSTANCE *, DWORD); // Enable Event

RC APIENTRY ReEventProc (MEVCB *);// Record Stream EventProc

RC APIENTRY PlEventProc (MEVCB *);// Play Stream EventProc

RC StartRecord (FUNCTION_PARM_BLOCK *); // Start Record Stream

RC StartPlay (FUNCTION_PARM_BLOCK *);   // Start Play Stream

RC StartSave (FUNCTION_PARM_BLOCK * ); // Start Save function

/******************************************
* Audio Device Support Functions
******************************************/

RC InitAudioDevice (INSTANCE *,ULONG ); // Open The Audio Device

RC SetAmpDefaults (INSTANCE *);//Set Amp/Mixer Default Values.

RC SetAudioDevice (INSTANCE *, LPMCI_WAVE_SET_PARMS, ULONG );// Change device

VOID SetWaveDeviceDefaults (INSTANCE *, ULONG);// Set Wave device defaults

VOID VSDInstToWaveSetParms (LPMCI_WAVE_SET_PARMS, INSTANCE *); // Swap Data

RC GetPDDName (ULONG , CHAR []);


/********************************************
* Instance Management, Parameter Checking..
********************************************/


RC CheckFlags (ULONG); // Check MCI Flags

RC CleanUp (PVOID); // Release Memory

DWORD CheckMem (PVOID, ULONG, ULONG); // Check  Valid Mem

RC AllocNCopyMessageParmMem (USHORT,FUNCTION_PARM_BLOCK *,
                                ULONG, USHORT);// Memory Allocation

RC ReportMMEErrors (USHORT ,FUNCTION_PARM_BLOCK *); // Error Checking


RC ReleaseInstanceMemory (FUNCTION_PARM_BLOCK *);  // Free Instance Mem

/*********************************
* Time Format Support
*********************************/

RC ConvertTimeUnits (INSTANCE *, DWORD *, DWORD); // To Current Base

RC ConvertToMM (INSTANCE *, DWORD *, DWORD); // To MMTIME

/*********************************
* Tasking Support
*********************************/
RC AbortWaitOperation (INSTANCE *); // Release Wait Block

DWORD MCD_EnterCrit (INSTANCE *);  // Acquire Mutex Sem

DWORD MCD_ExitCrit (INSTANCE *);  // Release Mutex Sem

DWORD AcquireProcSem ();  // Acquire Global Proc Sem

DWORD ReleaseProcSem ();  // Release Global Proc Sem

/***********************************
* Debug information
************************************/


/*****************************
* MCI Driver Entry Point
*****************************/
RC    EXPENTRY mciDriverEntry (LPTR,    /* Instance Pointer  */
                               USHORT,   /* MCI Message       */
                               ULONG,    /* MCI Flags         */
                               ULONG,    /* Data For This Msg */
                               USHORT);  /* User Parameter    */
