/***************************************************************************\
*
* Module Name: AUDIOMCD.H
*
* OS/2 2.0 Multimedia Extensions Audio MCI Driver Instance Data
* Block defintions.
*
* Copyright (c) International Business Machines Corporation 1990,1991
*                         All Rights Reserved
*
\***************************************************************************/

/*
#ifdef _TORONTO
    typedef  ULONG   NULL;
#endif
*/
/*************************************/
  // Definitons of sizes
/************************************/
#define  MAX_ERROR_LENGTH         128
#define  MAX_MODULE_NAME          10
#define  MAX_FILE_NAME            16
#define  HANDLER_NAMES_BUFLEN     128
#define  NOTIFY_THREAD_STACKSIZE  32768
#define  HEAP_SIZE                4096
#define  INDEFNITE_PERIOD          -1
#define  ONE_MINUTE               60000
#define  MAX_CUEPOINTS            20

/************************************************
* Default Wave attributes
************************************************/
#define DEFAULT_SAMPLERATE                   22050
#define DEFAULT_BYTESPERSEC                  22050
#define DEFAULT_BITSPERSAMPLE                16
#define DEFAULT_MODE                         DATATYPE_WAVEFORM
#define DEFAULT_CHANNELS                     1
#define DEFAULT_SOURCE_HANDLER_NAME         "FSSH"
#define MEM_PLAYLIST_SH                     "MEMSH"
#define DEFAULT_SOURCE_PDD_NAME             " "
#define DEFAULT_TARGET_HANDLER_NAME         "AUDIOSH$"
#define DEFAULT_DEVDLL_NAME                 "AUDIOIF"
#define DEFAULT_BLOCK_ALIGN                  1

/*********************************************************************
*   Modified EVCB just has an Implicit Evcb and MCD Instance
*   in a Single Block.
*********************************************************************/
typedef struct MEVCB {
      IMPL_EVCB           evcb;                /* Implicit EVCB           */
      ULONG               ulpInstance;         /* Instance Ptr            */
      USHORT              usUserParm;          /* Returned on Cue Points  */
      } MEVCB; /* Modified EVCB */

/********************************************************************
*   Modified Time EVCB Block Contains a Time EVCB with MCD Instance,
*   and other useful Parameters
********************************************************************/
typedef struct MTIMEEVCB {
    TIME_EVCB            evcb;                /* Time EVCB                */
    WORD                 wDeviceID;           /* Cue Point Notifications  */
    HWND                 dwCallback;          /* Cue Point Call back Hndl */
    ULONG                ulpInstance;         /* Instance Ptr             */
    USHORT               usCueUsrParm;        /* CuePt User Parm          */
    MMTIME               mmCuePt;             /* CuePt in MMTIME          */
    } MTIME_EVCB;  /* Modified Event Control Block */

/*****************************************************************
*   Streaming Instance is a MCI Driver Layer Data Block
*   defining information needed to Use SSM SPIs. This structure
*   is Instance based. This Structure defines Streaming Info only.
*****************************************************************/
typedef struct _strInstance {
    HID         hidASource;                    /* Source Handler ID         */
    HID         hidATarget;                    /* STarget Handler ID        */
    HID         hidBSource;                    /* Target as Source ID       */
    HID         hidBTarget;                    /* Target as Target ID       */
    char        pszAHandlerName[15];           /* Source Handler Name       */
    char        pszBHandlerName[15];           /* Target Handler Name       */
    HEVENT      hEvent;                        /* Implicit Event Handle     */
    HEVENT      hPosEvent;                     /* Position Event Handle     */
    HEVENT      hPlayToEvent;                  /* Cue Point Event handle    */
    MMTIME      mmStreamTime;                  /* Stream Time               */
    ULONG       ulState;                       /* Stream Pause,Play etc     */
    HSTREAM     hStream;                       /* Address of stream handle  */
    MEVCB       Evcb;                          /* Pointer to ECB            */
    MTIME_EVCB  TimeEvcb;                      /* Cue Point Time EVCB       */
    MTIME_EVCB  PosAdvEvcb;                    /* Time Events for PosAdvise */
    MTIME_EVCB  MCuePtEvcb[MAX_CUEPOINTS];     /* Array of CuePt Evcbs      */
    HEVENT      HCuePtHndl[MAX_CUEPOINTS];     /* Array of CuePt Event hndls*/
    USHORT      usCuePtIndex;                  /* Index into above          */
    PVOID       EventRtn;                      /* Event handler entry point */
    SPCBKEY     SpcbKey;                       /* SPCB key with Datatype    */
    ACB_MMIO    acbmmio;                       /* Associate Control Block   */
    ACB_MEM_PLAYL acbPlayList;                 /* Associate Control Block   */
    DCB_AUDIOSH AudioDCB;                      /* AudioSH Device Control Bl */
    } STRINSTANCE;                             /* Streaming Instance Struct */

/********************************************************************
*   This Structure defines the data items that are needed to be
*   maintained per MCI Device instance. The Instance structure
*   has a pointer to the Streaming Information Structure defined
*   above. Memory For This data structure is allocated dynamically
*   off a Global heap.
********************************************************************/

typedef struct _Instance {
    ULONG               ulInstanceSignature;       /* Instance Signature   */
    WORD                wAmpDeviceID;              /* Amp Mixer Device ID  */
    WORD                wWaveDeviceID;             /* Wave Device ID       */
    HEV                 hEventSem;                 /* Event Sem  Handle    */
    HEV                 hThreadSem;                /* Thread Control Sem   */
    HMTX                hmtxDataAccess;            /* Instance Data Access */
    HMTX                hmtxNotifyAccess;          /* Notify status access */
    HMODULE             hModHandle;                /* Handle of above      */
    HMODULE             hModIOProc;                /* Custom IO Proc Hndl  */
    HMMIO               hmmio;                     /* MMIO file ID         */
    HFILE               hTempFile;                 /* Temp File Handle */
    MMCKINFO            mmckinfo;                  /* Riff Chunk Info      */
    TID                 PlayThreadID;              /* Play Thread ID       */
    TID                 RecdThreadID;              /* Recd Thread ID       */
    TID                 SaveThreadID;              /* Save Thread ID       */
    char                lpstrAudioFile[128];       /* WAV File Name        */
    char                szDevDLL[128];             /* Dev Spcfc DLL        */
    char                szAudioDevName[9];         /* Audio Device Name    */
    ULONG               ulNotifyAborted;
    ULONG               ulOperation;               /* Play, Record, pause  */
    ULONG               ulTimeUnits;               /* Current Time Units   */
    ULONG               ulSyncOffset;              /* Synchronizatn offst  */
    ULONG               ulSecondsPerByte;          /* Bytes Per Sec        */
    ULONG               ulCreateFlag;              /* Flag for Strm Crtn   */
    ULONG               ulCreatedName;             /* Indicates we created name */
    BOOL                usRecordFlags;             /* Flag for Recording   */
    BOOL                usFileExists;              /* Flag for File Crtn   */
    ULONG               StreamEvent;               /* Stream Event         */
    BOOL                usNotifyPending;           /* Notify Pending Flag  */
    USHORT              usNotPendingMsg;           /* Msg for above        */
    BOOL                usVSDInit;                 /* Device Init Flag     */
    USHORT              usModeIndex;               /* Mode Table Index     */
    DWORD               dwCallback;                /* Call Back Hndl       */
    DWORD               dwOpenCallBack;
    BOOL                PlayTo;                    /* Play Till Flag       */
    USHORT              usUserParm;                /* User Parameter       */
    BOOL                usSaveFlag;                /* Drv Save             */
    BOOL                usRestFlag;                /* Drv rest             */
    STRINSTANCE         StreamInfo;                /* Streaming Info       */
    BOOL                usMediaPresent;            /* Did we Specify a fil */
    MCI_AMP_INSTANCE    AmpInstance;               /* VSDI driver Struct   */
    PVOID               pPlayList;                 /* Pointer to Play List */
    BOOL                usPlayLstStrm;             /* Flag for stream Crtn */
    BOOL                usPosAdvise;               /* Position Advise Flag */
    BOOL                usCuePt;                   /* CuePoint On Flag     */
    BOOL                usRecdOK;                  /* Okay To Wait For RECD*/
    DWORD               dwMMFileLength;            /* Length of File in MM */
    MMAUDIOHEADER       mmAudioHeader;             /* DCR 96               */
    USHORT              usPosUserParm;             /* User Parm Returned   */
    BOOL                mmioHndlPrvd;              /* mmioHandle Given Flag*/
    BOOL                usRecdInsert;              /* Record Insert Flag   */
    DWORD               dwmmioOpenFlag;            /* mmioOpen Flags       */
    BOOL                usWaitPending;             /* Pending Wait Flag    */
    USHORT              usWaitMsg;                 /* Pending Wait Msg     */
    ULONG               ulAbortedNotify;           /* indicates notify aborted */
    ULONG               ulCanInsert;               /* Insertion  possible  */
    ULONG               ulCanSave;                 /* Save is possible     */
    ULONG               ulUsingTemp;               /* Using temp files     */
    ULONG               ulOpenTemp;                /* Open with temp files */
    ULONG               ulCanRecord;               /* Recording is possible*/
    ULONG               ulBlockAlignment;          /* Blk Algnment for wave*/
    ULONG               ulAverageBytesPerSec;      /* Holds average bps    */
    ULONG               ulNoSaveWithoutName;       /* save w/o name flag   */
    USHORT              usAsyncMessage;            /* Asynchronous message */
    ULONG               ulOldStreamPos;            /* holds destroyed pos  */
    LONG (* APIENTRY pfnVSD) (LPMCI_AMP_INSTANCE, LONG, LONG, LONG, LONG);
    } INSTANCE;         /* Audio MCD MCI Instance Block */


/*********************************************************************
* Function Parameter Block defines the parameters needed by each MCI
* Message processing function to process the message.
**********************************************************************/
typedef struct {
    LPTR        ulpInstance;       /*  Pointer to Instance               */
    USHORT      usMessage;         /*  The Message                       */
    ULONG       ulParam1;          /*  Flags for the Message             */
    ULONG       ulParam2;          /*  Data for The Message              */
    USHORT      usUserParm;        /*  User Parameter Returned on Notify */
    ULONG       ulNotify;          /*  Notification flag                 */
    INSTANCE    *pInstance;        /*  Shall be Modified later           */
    DWORD       dwCallback;        /*  Window Handle                     */
    }FUNCTION_PARM_BLOCK;          /*  Audio MCD Function Paramter Block */


/************************************************/
        // MASKS for MCI_WAIT and MCI_NOTIFY
/************************************************/
#define NOTIFY_MASK     ~(MCI_NOTIFY | MCI_WAIT)
#define WAIT_MASK       0xffffff00L
#define   VSDI          "AUDIOIFDriverEntry"

/************************************
* Streaming STATE defines
*************************************/
#define CREATE_STATE        0x080
#define PREROLL_STATE       0x081
#define SAVED_STATE         0x082
#define RESTOR_STATE        0x083
#define ACTIVE              0x084
#define UNUSED              0x085
#define STREAM_STOPPED      0x086
#define PLAY_STREAM         0x087
#define RECORD_STREAM       0x088
#define DESTROYED_STATE     0x089
#define CUEPLAY_STATE       0x090
#define CUERECD_STATE       0x091
#define SAVEPAUS_STATE      0x09a
#define OPEN_FATAL          0x09b
#define EVENT_ENABLED       0x09c
#define ASYNCH_MESSAGE      0x09d
#define STREAM_SET_STATE    0x09e
#define STOP_PAUSED         0x09f
#define NOT_INTIALIZED      -1
#define FILE_LENGTH         -1
#define WAVE_IO_PROC        01
#define AVC_IO_PROC         02

/************************************
* Current Time Unit TAGS
************************************/
#define lMMTIME         01
#define lMILLISECONDS   02
#define lBYTES          03
#define lSAMPLES        04

/************************************
*  Internal Error States
************************************/
#define MCI_WAVEERR_BASE        6000
#define NULLPOINTER             (MCI_WAVEERR_BASE +51)
#define NUM_DATATYPES           32
#define SCALE_FACTOR            100

#define XWAVHDR      (ulpInstance->mmAudioHeader.mmXWAVHeader.XWAVHeaderInfo)
#define WAVEHDR      (ulpInstance->mmAudioHeader.mmXWAVHeader.WAVEHeader)
#define CUEPTINDX    (ulpInstance->StreamInfo.usCuePtIndex)
#define CUEPTEVCB    (ulpInstance->StreamInfo.MCuePtEvcb)
#define POSEVCB      (ulpInstance->StreamInfo.PosAdvEvcb)
#define TIMEEVCB     (ulpInstance->StreamInfo.TimeEvcb)
#define STRMSTATE    (ulpInstance->StreamInfo.ulState)
#define STREAM       (ulpInstance->StreamInfo)
#define AMPMIX       (ulpInstance->AmpInstance)
