/****************************************************************************/
/*                                                                          */
/* Module Name: ACB.H                                                       */
/*                                                                          */
/* OS/2 2.0 Multimedia Extensions Sync/Stream Manager Associate             */
/* Control Block Definitions.                                               */
/*                                                                          */
/* Copyright (c) International Business Machines Corporation 1991, 1992     */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/

#define MAX_PORT_NAME         40           /* Max port name length         */
#define MAX_PORTS             16           /* Max number ports             */


/* XLATOFF */
#pragma pack(4)
/* XLATON */

/**********************************************
 *
 * ACB - Associate Control Block
 *
 *       Each stream handler must define the
 *       ACB for each data object type that
 *       expect ACB's from the application.
 *
 **********************************************/
typedef struct _acb { /* acb  -  Assoc. Control Block  */
   ULONG   ulACBLen;              /* length of structure                   */
   ULONG   ulObjType;
   ULONG   ulParm1;
   ULONG   ulParm2;
   } ACB;
typedef ACB  FAR  *PACB;              /* Ptr to an ACB        */


/******************************************************
 * FSSH - File system stream handler MMIO Object ACB
 ******************************************************/
#define ACBTYPE_MMIO          0x0001L  /* MMIO object                      */
typedef struct _acb_mmio   { /* acbmmio  - MMIO Assoc. Control Block       */
   ULONG   ulACBLen;              /* length of structure                   */
   ULONG   ulObjType;             /* ACBTYPE_MMIO                          */
   HMMIO   hmmio;                 /* Handle of media element mgr obj       */
   } ACB_MMIO;
typedef ACB_MMIO  FAR  *PACB_MMIO;       /* Ptr to an ACB  */


/******************************************************
 * MSH - Memory stream handler Playlist Object ACB
 ******************************************************/
#define ACBTYPE_MEM_PLAYL     0x0003L  /* Memory playlist object           */
typedef struct _acb_mem_playl { /* acbplyl - File system Assoc. Control Block */
   ULONG   ulACBLen;              /* length of structure                   */
   ULONG   ulObjType;             /* ACBTYPE_MEM_PLAYL                     */
   PVOID   pMemoryAddr;           /* Starting address of memory obj        */
   } ACB_MEM_PLAYL;
typedef ACB_MEM_PLAYL  FAR  *PACB_MEM_PLAYL;     /* Ptr to an ACB  */


/******************************************************
 * CDDASH - CD DA stream handler Object ACB
 ******************************************************/
#define ACBTYPE_CDDA          0x0004L  /*  Compact disk - digital audio obj*/

/* XLATOFF */
#pragma pack(1)
/* XLATON */

typedef struct _acb_CDDA   { /* acbcdda - CD Assoc. Control Block          */
   ULONG   ulACBLen;              /* length of structure                   */
   ULONG   ulObjType;             /* ACBTYPE_CDDA                          */
   CHAR    bCDDrive;              /* CD drive letter                       */
   } ACB_CDDA;
typedef ACB_CDDA  FAR  *PACB_CDDA;       /* Ptr to an ACB  */

/* XLATOFF */
#pragma pack()
/* XLATON */


/******************************************************
 * MISH - MIDI stream handler port-stream table ACB
 ******************************************************/
#define ACBTYPE_MISH          0x0005L      /* MIDI port-stream table       */
typedef struct _acb_MISH { /* acbmish - MIDI Assoc. Control Block          */
   ULONG     ulACBLen;                     /* length of structure          */
   ULONG     ulObjType;                    /* ACBTYPE_MISH                 */
   HSTREAM   hstreamDefault;               /* Default hstream to use when mapper */
                                           /* is turned off.               */
   ULONG     ulDeviceTypeID;               /* device type id               */
   ULONG     ulpMapperPorts;               /* pointer to mapper port table */
   ULONG     ulNumInStreams;
   HSTREAM   hstreamIn[MAX_PORTS];         /* Array of Input streams       */
   ULONG     ulNumOutStreams;
   HSTREAM   hstreamOut[MAX_PORTS];        /* Array of Output streams      */
                                           /* The index into the array is  */
                                           /* the source channel for that  */
                                           /* stream.                      */
   } ACB_MISH;
typedef ACB_MISH FAR *PACB_MISH;           /* Ptr to MIDI associate control block */


/******************************************************
 * MISH - MIDI stream handler SET ACB
 ******************************************************/
#define ACBTYPE_SET           0x0006L      /* MIDI set function            */
typedef struct _acb_set { /* acbset - Set Assoc. Control Block             */
   ULONG     ulACBLen;                     /* length of structure          */
   ULONG     ulObjType;                    /* ACBTYPE_SET                  */
   ULONG     ulFlags;                      /* Set flags                    */
   ULONG     ulReserved;                   /* Tempo change value (used only if */
                                           /*  MIDI_TEMPO_CHANGE)          */
   } ACB_SET;
typedef ACB_SET FAR *PACB_SET;             /* Ptr to set associate control block */

/* ulFlags defines:                                                        */
#define MIDI_MAP_ON           0x0000L      /* turn mapping function on in MISH   */
#define MIDI_MAP_OFF          0x0001L      /* turn mapping function off in MISH  */

/* XLATOFF */
#pragma pack()
/* XLATON */
