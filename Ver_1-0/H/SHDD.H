/***************************************************************************\
*
* Module Name: SHDD.H
*
* OS/2 2.0 Multimedia Extensions Stream Handler Device Driver Interfaces
* Block defintions.
*
* Copyright (c) International Business Machines Corporation 1990,1991
*                         All Rights Reserved
*
\***************************************************************************/

/****************************************************************************/
/*                         D E F I N E S                                    */
/****************************************************************************/

/* XLATOFF */
typedef RC (FAR *PSHDFN)        (PVOID pParmIn);
typedef RC (FAR *PDDCMDFN)      (PVOID pParmIn);
/* XLATON */
#ifdef XXXXX                                 /* Picked up by H2INC only    */
typedef PVOID PSHDFN;                        /* Because previous function  */
typedef PVOID PDDCMDFN;                      /* prototypes generate errors */
                                             /* when parsed by H2INC.      */
#endif                                       /*                            */



/* DDCMD's:                                                                */

#define     DDCMD_SETUP         0L
#define     DDCMD_READ          1L
#define     DDCMD_WRITE         2L
#define     DDCMD_STATUS        3L
#define     DDCMD_CONTROL       4L
#define     DDCMD_REG_STREAM    5L
#define     DDCMD_DEREG_STREAM  6L


/* SHD's:                                                                   */
#define     SHD_REPORT_INT      0L      /* PDD reports interrupt has arrived*/
#define     SHD_REPORT_EVENT    1L      /* PDD reports cuetime has arrived  */



/****************************************************************************/
/*                       S T R U C T U R E S                                */
/****************************************************************************/

/* XLATOFF */
#pragma pack(4)
/* XLATON */

/**************************************/
/****   DDCMD's                      **/
/**************************************/
typedef struct  _ddcmd_common_parm {            /* comm  - common to all DD cmds */
                        ULONG   ulFunction;     /* Function requested by SH */
                        HSTREAM hStream;        /* data stream instance     */
                      } DDCMDCOMMON;

typedef struct  _ddcmd_setup_parm {             /* setu */
                        ULONG   ulFunction;     /* Function requested by SH */
                        HSTREAM hStream;
                        PVOID   pSetupParm;     /* pass stream time to PDD  */
                        ULONG   ulSetupParmSize;
                      } DDCMDSETUP;

typedef struct  _ddcmd_readwrite_parm {         /* read */
                        ULONG   ulFunction;     /* Function requested by SH */
                        HSTREAM hStream;
                        PVOID   pBuffer;
                        ULONG   ulBufferSize;
                      } DDCMDREADWRITE;

typedef struct  _ddcmd_status_parm {            /* stat */
                        ULONG   ulFunction;     /* Function requested by SH */
                        HSTREAM hStream;
                        PVOID   pStatus;        /* (output)ptr to current position time */
                        ULONG   ulStatusSize;   /* (output)size of position time        */
                      } DDCMDSTATUS;

typedef struct  _ddcmd_control_parm {           /* cont */
                        ULONG   ulFunction;     /* Function requested by SH */
                        HSTREAM hStream;
                        HEVENT  hEvent;         /* used for EVENTS only                 */
                        ULONG   ulCmd;
                        PVOID   pParm;          /* (input/output) ptr to (ulong)cuetime when ulCmd is ENABLE_EVENT */
                        ULONG   ulParmSize;     /* (input/output) size of cuetime or returning current time        */
                      } DDCMDCONTROL;

/**********************************************/
/* ulCmd defines                              */
/**********************************************/
                                          /* CHANGE and WAIT not implemented                   */
#define DDCMD_START       1               /* start device                                      */
#define DDCMD_STOP        2               /* stop device and return current position in pParm  */
#define DDCMD_PAUSE       3               /* pause device and return current position in pParm */
#define DDCMD_RESUME      4               /* resume device                                     */
#define DDCMD_ENABLE_EVENT  5             /* ask PDD to create this event                      */
#define DDCMD_DISABLE_EVENT 6             /* ask PDD to purge this event                       */

typedef struct  _ddcmd_register_parm {          /* regi */
                        ULONG   ulFunction;     /* Function requested by SH                    */
                        HSTREAM hStream;        /* Stream handle needed @ interrupt time       */
                        ULONG   ulSysFileNum;   /* Device Handle so pdd can map device instance to hstream */
                        PSHDFN  pSHDEntryPoint; /* Stream handler entry point                  */
                        ULONG   ulStreamOperation; /* SH input Record or play                  */
                        SPCBKEY spcbkey;
                        ULONG   ulBufSize;      /* PDD output (optional) buffer size in bytes for SPCB */
                        ULONG   ulNumBufs;      /* PDD output (optional) # of buffers for SPCB         */
                        ULONG   ulAddressType;  /* PDD output (required) addr ptr type to data buffer  */
                        ULONG   ulBytesPerUnit; /*PDD output (required)                                */
                        MMTIME  mmtimePerUnit;  /*PDD output (required)                                */
                      } DDCMDREGISTER;

/*****************************/
/* ulStreamOperation  defines*/
/*****************************/
#define STREAM_OPERATION_MASK     0xC000
#define STREAM_OPERATION_CONSUME  0x8000
#define STREAM_OPERATION_PRODUCE  0xC000

/*******************************************************************************/
/* ulAddressType  defines                                                      */
/* The PDD will tell the SH what type of address ptr it expects the data buffer*/
/* to be.  The SH will then request this address type to the SSM, so that the  */
/* SSM will send the correct type of each buffer request.                      */
/*******************************************************************************/
#define ADDRESS_TYPE_VIRTUAL    0L
#define ADDRESS_TYPE_PHYSICAL   1L              /*default                  */
#define ADDRESS_TYPE_LINEAR     2L


typedef struct  _ddcmd_deregister_parm {        /* dere */
                        ULONG   ulFunction;     /* Function requested by SH */
                        HSTREAM hStream;        /* Stream handle needed @ interrupt time */
                      } DDCMDDEREGISTER;



typedef DDCMDCOMMON    FAR     *PDDCMDCOMMON;
typedef DDCMDSETUP     FAR     *PDDCMDSETUP;
typedef DDCMDREADWRITE FAR     *PDDCMDREADWRITE;
typedef DDCMDSTATUS    FAR     *PDDCMDSTATUS;
typedef DDCMDCONTROL   FAR     *PDDCMDCONTROL;
typedef DDCMDREGISTER  FAR     *PDDCMDREGISTER;
typedef DDCMDDEREGISTER FAR    *PDDCMDDEREGISTER;


/**************************************/
/****   SHD's                        **/
/**************************************/
typedef struct  _shd_common_parm {              /* shdc */
                        ULONG   ulFunction;     /* Function requested by PDD               */
                        HSTREAM hStream;        /* Stream handle needed @ interrupt time   */
                        }SHD_COMMON;

typedef struct  _shd_reportint_parm {           /* repi */
                        ULONG   ulFunction;     /* Function requested by PDD*/
                        HSTREAM hStream;        /* so SH knows which stream to process     */
                        PVOID   pBuffer;        /* return ptr to last used buffer          */
                        ULONG   ulFlag;         /* reason for interrupt     */
                        ULONG   ulStatus;       /* rc or bytes read/written */
                        ULONG   ulStreamTime;   /* time in milliseconds of stream position  */
                      } SHD_REPORTINT;
/* ulFlag settings */
#define ERROR               0x80L
#define SHD_READ_COMPLETE   1L
#define SHD_WRITE_COMPLETE  2L

typedef struct  _shd_reportevent_parm {         /* repe */
                        ULONG   ulFunction;     /* Function requested by PDD*/
                        HSTREAM hStream;        /* so SH knows which stream to process     */
                        HEVENT  hEvent;         /* event handle pass back to stream handler*/
                        ULONG   ulStreamTime;   /* time in milliseconds of stream position  */
                      } SHD_REPORTEVENT;

typedef SHD_COMMON      FAR  *PSHD_COMMON;
typedef SHD_REPORTINT   FAR  *PSHD_REPORTINT;
typedef SHD_REPORTEVENT FAR  *PSHD_REPORTEVENT;


/* XLATOFF */
#pragma pack()
/* XLATON */

/****************************************************************************/
/*              F U N C T I O N   P R O T O T Y P E S                       */
/****************************************************************************/

/* XLATOFF */
RC  FAR         SHDEntryPoint(PSHD_COMMON pCommon);
RC  FAR         DDCMDEntryPoint(PDDCMDCOMMON pCommon);
/* XLATON  */



