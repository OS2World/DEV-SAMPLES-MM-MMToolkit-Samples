/****************************************************************************/
/*                                                                          */
/* Module Name: MCIDRV.H                                                    */
/*                                                                          */
/* This is the multimedia include file that has the typedefs, defines and   */
/* function prototyes for MCI drivers.                                      */
/*                                                                          */
/* Copyright (c) International Business Machines Corporation 1991, 1992     */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/

typedef    USHORT   GID;

/****************************************************************************/
/*             MCI Driver  message identifiers                              */
/****************************************************************************/
#define MCIDRV_SAVE                 900
#define MCIDRV_RESTORE              901
#define MCIDRV_SYNC                 902
#define MCIDRV_CHANGERESOURCE       903

/*********************************************************************/
/* The following range of message ID's are reserved for internal use */
/*  by MCI drivers                                                   */
/*********************************************************************/
#define MCI_INTERNAL_MESSAGES_START    1000
#define MCI_INTERNAL_MESSAGES_END      1999

/*********************************************************************/
/* Flags used by the string parser for command tables                */
/* NOTE: All flags must have an "L" suffix in order to be parsed as  */
/* DWORDs by the resource compiler                                   */
/*********************************************************************/
#define MCI_COMMAND_HEAD                 0x00000001L
#define MCI_END_COMMAND                  0x00000002L
#define MCI_END_COMMAND_LIST             0x00000003L
#define MCI_RETURN                       0x00000004L
#define MCI_STRING                       0x00000005L
#define MCI_FLAG                         0x00000006L
#define MCI_INTEGER                      0x00000007L
#define MCI_CONSTANT                     0x00000008L
#define MCI_CONSTANT_INTEGER             0x00000009L
#define MCI_CONSTANT_STRING              0x0000000AL
#define MCI_END_CONSTANT                 0x0000000BL
#define MCI_DEFAULT_STRING               0x0000000CL
#define MCI_DEFAULT_INTEGER              0x0000000DL
#define MCI_RETURN_TYPE                  0x0000000EL
#define MCI_RETURN_TYPE_STRING           0x0000000FL
#define MCI_END_RETURN_TYPE              0x00000010L
#define MCI_CONSTANT_2                   0x00000011L
#define MCI_END_CONSTANT_2               0x00000012L
#define MCI_CONSTANT_PAIR                0x00000013L
#define MCI_END_CONSTANT_PAIR            0x00000014L
#define MCI_CONSTANT_INTEGER_PAIR        0x00000015L
#define MCI_CONSTANT_STRING_PAIR         0x00000016L

/*********************************************************************/
/* Return types supported by mciSendString                           */
/*********************************************************************/
#define MCI_INTEGER_RETURNED                     0x1000
#define MCI_COLONIZED2_RETURN                    0x2000
#define MCI_COLONIZED3_RETURN                    0x3000
#define MCI_COLONIZED4_RETURN                    0x4000
#define MCI_TRUE_FALSE_RETURN                    0x5000
#define MCI_ON_OFF_RETURN                        0x6000
#define MCI_DEVICENAME_RETURN                    0x7000
#define MCI_TIME_FORMAT_RETURN                   0x8000
#define MCI_SPEED_FORMAT_RETURN                  0x9000
#define MCI_MODE_RETURN                          0xA000
#define MCI_MEDIA_TYPE_RETURN                    0xB000
#define MCI_TRACK_TYPE_RETURN                    0xC000
#define MCI_CONNECTOR_TYPE_RETURN                0xD000
#define MCI_CDXA_CHANNEL_DESTINATION_RETURN      0xE000
#define MCI_PREROLL_TYPE_RETURN                  0xF000
#define MCI_FORMAT_TAG_RETURN                    0xF100
#define MCI_SEQ_SYNCHRONIZATION_RETURN           0xF200

/*********************************************************************/
/* End of msg text used by mciGetErrorString                         */
/*********************************************************************/
#define MCIERR_MSG_TABLE_END    MCIERR_BASE + 7000

/*********************************************************************/
/*                                                                   */
/*  MCI DEVICE shareability categories                               */
/*                                                                   */
/*********************************************************************/
#define FIXEDSINGLECONTEXT            0x0001
#define DYNAMICSINGLECONTEXT          0x0002
#define LIMITEDMULTIPLECONTEXT        0x0003
#define UNLIMITEDMULTIPLECONTEXT      0x0004

/*********************************************************************/
/* MCI driver flag for close during exit list processing             */
/*********************************************************************/
#define MCI_CLOSE_EXIT                 0x10000000L

/*********************************************************************/
/* MCI driver specific error table resource number base              */
/*********************************************************************/
#define MMERROR_TABLE_BASE      500

/*********************************************************************/
/*                                                                   */
/*  MCIDRV_CHANGERESOURCE  message flags                             */
/*                                                                   */
/*********************************************************************/
#pragma pack(1)

typedef struct {
   PVOID      pInstance;             /* pointer to device instance   */
   USHORT     usResourceUnits;       /* required resource units      */
   USHORT     usResourceClass;       /* resource class               */
   USHORT     usResourcePriority;    /* resource priority            */
} MCIDRV_CHANGERESOURCE_PARMS;
typedef MCIDRV_CHANGERESOURCE_PARMS  *PMCIDRV_CHANGERESOURCE_PARMS;

/*********************************************************************/
/*                                                                   */
/*  MCIDRV_SYNC  message flags                                       */
/*                                                                   */
/*********************************************************************/
#define MCIDRV_SYNC_ENABLE           0x00000100L
#define MCIDRV_SYNC_DISABLE          0x00000200L
#define MCIDRV_SYNC_REC_PULSE        0x00000400L
#define MCIDRV_SYNC_MASTER           0x00000800L

typedef struct {
    PVOID   pInstance;       /* pointer to the instance structure    */
    GID     GroupID;         /* pointer to the instance structure    */
    MMTIME  mmTime;          /* Synch pulse time                     */
    HSTREAM hStream;         /* Stream handle                        */
} MCIDRV_SYNC_PARMS;
typedef MCIDRV_SYNC_PARMS *PMCIDRV_SYNC_PARMS;


/*********************************************************************/
/* contains information for open message for MCI drivers             */
/*********************************************************************/
typedef struct {
    DWORD   dwCallback;    /* call back handle                       */
    WORD    wDeviceID;     /* The device ID assigned to this instance*/
    USHORT  usDeviceType;  /* The device type number                 */
    USHORT  usDeviceOrd;   /* The device ordinal number              */
    PVOID   pInstance;     /* pointer to the instance structure allocated */
                           /* initialized by the driver. The MCI driver   */
                           /* will fill in this parameter.                */
    CHAR    szDevDLLName[260]; /* Character string containing the device  */
                               /* specific DLL name to call for the open. */
                               /* (ie. ACPA.DLL)                          */
    LPSTR   lpstrElementName;/* typically a file name or NULL             */
    USHORT  usDevParmLen;  /* Device parameters data block length.        */
    PVOID   pDevParm;      /* Device parameters data block. This data     */
                           /* block is unique to each type of device.     */
                           /* (ie. LVD "COM1 9600 N 7 1").                */
    PVOID   Reserved0;
    USHORT  usResourceUnitsRequired;  /* number of resource units this instance */
                                      /* requires.                              */
    USHORT  usResourceClass;          /* resource class this instance belongs to*/
    USHORT  usResourcePriority;       /* resource priority for this instance    */
    DWORD   dwParam2;                 /* Pointer to MCI_OPEN structure          */
} MMDRV_OPEN_PARMS;
typedef MMDRV_OPEN_PARMS  *PMMDRV_OPEN_PARMS;


#pragma pack()
DWORD APIENTRY mdmDriverNotify (WORD    wDeviceID,
                                HWND    hwnd,
                                WORD    wMsgType,
                                WORD    wUserParm,
                                DWORD   dwMsgParm);

DWORD APIENTRY mdmSyncNotify   (PVOID  pInstance,
                                GID    GroupID,
                                ULONG  ulSyncTime,
                                DWORD  dwSyncFlags);

