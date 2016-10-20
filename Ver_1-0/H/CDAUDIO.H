/****************************************************************************/
/*                                                                          */
/* Module Name:  CDAUDIO.H                                                  */
/*                                                                          */
/* FUNCTION:  This file contains the macro definition and common record     */
/*            structures used between the CD Audio MCI Driver and its       */
/*            VSDs (Vendor Specific Drivers).                               */
/* Copyright (c) International Business Machines Corporation 1991, 1992     */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/


/**********************************/
/* Macro Definitions              */
/**********************************/

#define UPC_SIZE              7           /* UPC code size, CD serial number */
#define CDMCD_CUEPOINT_MAX   20           /* maximum number of cuepoints */


/*************************************************************************/
/* Internal messages between the MCI Driver and the VSD.                 */
/*************************************************************************/

#define MCIDRV_REGISTER_DISC    MCI_INTERNAL_MESSAGES_START + 1
#define MCIDRV_REGISTER_DRIVE   MCI_INTERNAL_MESSAGES_START + 2
#define MCIDRV_REGISTER_TRACKS  MCI_INTERNAL_MESSAGES_START + 3
#define MCIDRV_CD_READ_LONG     MCI_INTERNAL_MESSAGES_START + 4
#define MCIDRV_CD_SET_VERIFY    MCI_INTERNAL_MESSAGES_START + 5
#define MCIDRV_CD_STATUS_CVOL   MCI_INTERNAL_MESSAGES_START + 6


/*************************************************************************/
/* Internal callback routine from the VSD to the MCI Driver.             */
/*************************************************************************/

typedef VOID (*PFNCDMCD) (DWORD, DWORD, DWORD);


/***********************************/
/* CDAudRegister record structures */
/***********************************/
#pragma pack(1)

typedef struct   /* ID a disc, used to verify a disc change                 */
{                /* must stay at 8 bytes to equal size of UPC               */
   BYTE   Mode;                           /* mode, 0=UPC, 1=ID              */
   WORD   wTrack1;                        /* address of track one in MMTIME */
   BYTE   NumTracks;                      /* number of tracks               */
   DWORD  dwLeadOut;                      /* address of lead out track      */
} MCI_CD_ID;

typedef struct /* CD-ROM Drive information, determines capabilities */
{
   WORD   wCaps;                          /* capabilities                   */
   DWORD  dwPrerollType;                  /* preroll type                   */
   DWORD  dwPrerollTime;                  /* preroll time                   */
   DWORD  dwMinStartTime;                 /* minimum starting time          */
   DWORD  dwCDMCDID;                      /* CD MCD ID for instance         */
   PFNCDMCD pCDMCDReturn;                 /* addr of CD MCD Return function */
} MCI_CD_REGDRIVE_PARMS;

/*************************************************/
/* These flags are valid for the wCaps Field     */
/*************************************************/
#define CDVSD_CAP_CAN_RECORD  0x0001    /* Can record audio               */
#define CDVSD_CAP_HAS_AUDIO   0x0002    /* Can play audio                 */
#define CDVSD_CAP_HAS_VIDEO   0x0004    /* Can play video                 */
#define CDVSD_CAP_CAN_EJECT   0x0010    /* Can eject disc                 */
#define CDVSD_CAP_CAN_STREAM  0x0020    /* Can stream                     */
#define CDVSD_CAP_HAS_DAC     0x0040    /* Can process internal           */
#define CDVSD_CAP_CAN_LOCK    0x0080    /* Can disable manual eject       */
#define CDVSD_CAP_CAN_VOLUME  0x0100    /* Can manipulate volume settings */
#define CDVSD_CAP_CAN_REVERSE 0x0200    /* Can play in reverse            */
#define CDVSD_CAP_CAN_V_SPEED 0x0400    /* Can vary play speed            */
#define CDVSD_CAP_CAN_CUE     0x0800    /* Can read sequent. after break  */
                                        /* Used for Cue, Pause, and Seek  */


typedef struct /* Information about the disc  */
{
   BYTE   LowestTrackNum;                 /* lowest track number  */
   BYTE   HighestTrackNum;                /* highest track number */
   BYTE   UPC[UPC_SIZE];                  /* upc, 13 BCD + 4bit 0 */
   MCI_CD_ID DiscID;                      /* Disc ID              */
} MCI_CD_REGDISC_PARMS;

typedef struct /* Information about each track */
{
   BYTE   TrackNum;                       /* track number               */
   DWORD  dwStartAddr;                    /* starting address in MMTIME */
   DWORD  dwEndAddr;                      /* ending address             */
   BYTE   TrackControl;                   /* track control information  */
} MCI_CD_REGTRACK_REC;

typedef struct /* Track information, used to verify address */
{
   MCI_CD_REGTRACK_REC *TrackRecArr;      /* ptr to array of track recs  */
   DWORD  dwBufSize;                      /* size of buffer              */
} MCI_CD_REGTRACKS_PARMS;


/********************************************/
/* Environment settings to save and restore */
/********************************************/

typedef struct
{
   DWORD dwPosition;                      /* current position in MMTIME       */
   DWORD dwEndPlay;                       /* end play position                */
   DWORD dwMode;                          /* Play mode: playing, paused, etc. */
   DWORD dwLevel;                         /* volume levels                    */
   DWORD dwVSDData;                       /* Extra VSD data storage area      */
} MCIDRV_CD_SAVE_PARMS;

/********************************************/
/* Read Long (2352-Byte) Sectors            */
/********************************************/

/**************************************************************************/
/* Flag for the MCIDRV_CD_READ_LONG message                               */
/*   Default addressing mode for the dwFrom field is Red Book.  Absolute  */
/*   sector addressing for ISO-9660, or High Sierra Group, requires flag. */
/**************************************************************************/

#define MCI_CD_READLONG_HSG          0x00000100L

typedef struct
{
   DWORD   dwFrom;                        /* read from this position   */
   WORD    wCount;                        /* Number of sectors to read */
   LPSTR   lpstrReturn;                   /* Pointer to return buffer  */
   DWORD   dwRetSize;                     /* Return buffer size        */
} MCI_CD_READLONG_PARMS;

#pragma pack()
