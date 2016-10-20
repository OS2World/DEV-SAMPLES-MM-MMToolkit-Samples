/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  AUDIODD.H         (TEMPLATE SAMPLE)                   */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver function                           */
/*                   prototypes and externs of global data.                 */
/*                                                                          */
/************************** END OF SPECIFICATIONS ***************************/

/****************************************************************************/
/*                       T Y P E  D E F S                                   */
/****************************************************************************/
typedef USHORT             OFFSET;              // Used for segment offsets
typedef void near *        NPVOID;
typedef void far *         FPVOID;
typedef int (pascal far    *FPFUNCTION) ();

/****************************************************************************/
/*                       D E F I N E S                                      */
/****************************************************************************/
#define DEFAULTSTREAMS  15
#define MAXIOBUFFS      16

#define NPROTOCOLS      33
#define NUM_TRACKS       2

/****************************************************************************/
/*                  D A T A   S T R U C T U R E S                           */
/****************************************************************************/

typedef struct _stream
   {
   HSTREAM         hStream;            // Unique stream handle
   ULONG           ulSysFileNum;       // id from kernel
                                       // PDDs have associated
                                       // file handle.
   ULONG           ulOperation;        // eg OPERATION_PLAY
   ULONG           ulFlags;            // State flags for this stream
   PVOID           ADSHEntry;          // IDC entrypoint into stream handler
   USHORT          usTrackNum;         // Which track does this stream use
   ULONG           ulCumTime;          // Current real-time stream position
   USHORT          usCurrIOBuffIndex;  // index into current iobuff struct
   USHORT          usNextIOBuffIndex;  // index into next iobuff struct
   MCI_AUDIO_IOBUFFER IOBuff[ MAXIOBUFFS ] ;
   } STREAM ;

typedef STREAM far *PSTREAM;

/************************
 *   STREAM ulFlags     *
 ************************/
#define     STREAM_STREAMING             0x0002L
#define     STREAM_PAUSED                0x0004L
#define     STREAM_STOPPED               0x0008L
#define     STREAM_REGISTERED            0x0010L

/***************************************
 *   IOBuff usRunFlags
 * These are for the IOBuff defined in
 * Audio.h. (MMPM/2 Toolkit header file)
 ***************************************/
#define     IOB_STOPPED             0x0002L
#define     IOB_PAUSED              0x0004L
#define     IOB_STARTED             0x0008L
#define     IOB_RUNNING             0x0010L
#define     IOB_OVERRUN             0x0020L
#define     IOB_UNDERRUN            0x0040L
#define     IOB_CHAIN_BUFFERS       0x0080L

typedef struct _global{
                PSTREAM         paStream;        // array ptr of stream structs
                unsigned short  usMaxNumStreams; // max # of streams from argument line
            } GLOBAL;

typedef GLOBAL *PGLOBAL;        // Pointer to dynamically allocated global
                                // table.  Pointer stored here in static memory


typedef struct  _protocol_table {
                        ULONG   ulDataType;       // Characteristics of
                        ULONG   ulDataSubType;    // stream types that this
                        ULONG   ulBufSize;        // PDD supports.
                        ULONG   ulNumBufs;
                        ULONG   ulSampleRate;
                        USHORT  usChannels;
                        USHORT  usBitsPerSample;
                        }PROTOCOLTABLE;

/*************************************************************************/
/* Device driver structures follow.                                      */
/*************************************************************************/

/*  Request Packet status bit values */
#define RPERR    0x8000 /*  error occurred, error code in RPstatuserr   */
#define RPDEV    0x4000 /*  also set ERR; error code defined by DD      */
#define RPBUSY   0x0200 /*  device is busy                              */
#define RPDONE   0x0100 /*  DD all done with request packet             */
#define RPBADCMD 0x0003 /*  DD recieved a bad command                   */

#define AUDIO_ATTRIB    0xC880                 // Attribute has IDC link

typedef struct  DeviceHdr {
                struct DeviceHdr far * DHnext;          // link
                USHORT  DHattrib;                       // attribute
                NPVOID  DHstrategy;                     // &Strategy routine
                NPVOID  DHidc;                          // &IDC routine
                UCHAR   DHname[8];                      // DD name
                UCHAR   DHreserved[8];                  // reserved
                }DEVHDR;

/*  AttachDD Inter-device driver communication data area */

typedef struct AttachArea {
        USHORT  realOFF;        /* real-mode offset of idc entry point  */
        USHORT  realCS;         /* real-mode CS of IDC entry point      */
        USHORT  realDS;         /* real-mode DS of IDC DD               */
        USHORT  protOFF;        /* protect-mode offset of entry point   */
        USHORT  protCS;         /* protect-mode CS of entry point       */
        USHORT  protDS;         /* protect-mode DS of other DD          */
        } ATTACHAREA;


/*  The Request Packet */

typedef struct ReqPacket {
        UCHAR   RPlength;       /* request packet length                */
        UCHAR   RPunit;         /* unit code for block DD only          */
        UCHAR   RPcommand;      /* command code                         */
        USHORT  RPstatus;       /* status word                          */
        UCHAR   RPreserved[4];  /* reserved bytes                       */
        ULONG   RPqlink;        /* queue linkage                        */
        union {                 /* command-specific data                */
            UCHAR   avail[19];  /* available space, in 32-byte packet   */

            struct {    /*  INIT Packet (one for entry, one for exit)   */
                   UCHAR      units;            /* number of units      */
                   FPFUNCTION DevHlp;           /* &DevHlp              */
                   char far  *args;             /* &args                */
                   UCHAR      drive;            /* drive #              */
                   } Init;                      /* available: 9 bytes   */
            struct {
                   UCHAR      units;            /* same as input        */
                   OFFSET     finalCS;          /* final code offset    */
                   OFFSET     finalDS;          /* final data offset    */
                   FPVOID BPBarray;             /* &BPB                 */
                   } InitExit;

            struct {    /*  READ, WRITE, WRITE_VERIFY                   */
                   UCHAR      media;            /* media descriptor     */
                   ULONG      buffer;           /* transfer address     */
                   USHORT     count;            /* bytes/sectors        */
                   ULONG      startsector;      /* starting sector#     */
                   USHORT     reserved;
                   } ReadWrite;                 /* available: 6 bytes   */

            struct {    /*  IOCTL                                       */
                   UCHAR      category;         /* category code        */
                   UCHAR      function;         /* function code        */
                   FPVOID parameters;           /* &parameters          */
                   FPVOID buffer;               /* &buffer              */
                   } IOCtl;                     /* available: 9 bytes   */

            struct {    /*  READ_NO_WAIT                                */
                   UCHAR      char_returned;    /* char to return       */
                   } ReadNoWait;                /* available: 18 bytes  */

            struct {    /*  MEDIA_CHECK                                 */
                   UCHAR      media;            /* media descriptor     */
                   UCHAR      return_code;      /* see #defines         */
                   FPVOID prev_volume;          /* &previous volume ID  */
                   } MediaCheck;                /* available: 13 bytes  */

            struct {    /*  BUILD_BPB                                   */
                   UCHAR      media;            /* media descriptor     */
                   FPVOID buffer;               /* 1-sector buffer FAT  */
                   FPVOID BPBarray;             /* &BPB array           */
                   UCHAR      drive;            /* drive #              */
                   } BuildBPB;                  /* available: 9 bytes   */

            struct {    /*  query PARTITIONABLE fixed disks             */
                   UCHAR      count;            /* 1-based # disks      */
                   ULONG      reserved;
                   } Partitionable;             /* available: 14 bytes  */

            struct {    /*  Get Fixed Disk/Logical Unit Map             */
                   ULONG      units;            /* units supported      */
                   ULONG      reserved;
                   } GetFixedMap;               /* available: 11 bytes  */

               } s;     /* command-Specific information */
        } REQPACKET;


typedef DEVHDR     near    *PDEVHDR;            // Ptr to device driver header
typedef ATTACHAREA near    *PATTACHAREA;        // Ptr to attachDD area
typedef REQPACKET  far     *PREQPACKET;         // ptr to request packet
typedef PREQPACKET far     *PPREQPACKET;
typedef PREQPACKET         QHEAD;               // Queue Head is &ReqPacket
typedef QHEAD      near    *PQHEAD;


/****************************************************************************/
/*              F U N C T I O N   P R O T O T Y P E S                       */
/****************************************************************************/

//**************************************************
//      Data Moving routines
//**************************************************
VOID    SetupPointers(void);
VOID    WriteDataToCard(void);
VOID    ReadDataFromCard(void);
VOID    RestorePointers(void);
VOID    EOI (void);
VOID    InterruptHandler(void);

//**************************************************
//      IOCTL  routines
//**************************************************
ULONG  Strategy_c(PREQPACKET rp);
ULONG  IOCTL_Init(PREQPACKET rp);
ULONG  IOCTL_Read(PREQPACKET rp);
ULONG  IOCTL_NondestructiveRead(PREQPACKET rp);
ULONG  IOCTL_ReadStatus(PREQPACKET rp);
ULONG  IOCTL_FlushInput(PREQPACKET rp);
ULONG  IOCTL_Write(PREQPACKET rp);
ULONG  IOCTL_WriteStatus(PREQPACKET rp);
ULONG  IOCTL_FlushOutput(PREQPACKET rp);
ULONG  IOCTL_Open(PREQPACKET rp);
ULONG  IOCTL_Close(PREQPACKET rp);
ULONG  IOCTL_Input(PREQPACKET rp);
ULONG  IOCTL_Invalid(PREQPACKET rp);
ULONG  IOCTL_Output(PREQPACKET rp);
ULONG  IOCTL_GenIoctl(PREQPACKET rp);

//**************************************************
//      STANDARD AUDIO IOCTL  routines
//**************************************************
ULONG   Audio_IOCTL_Init(PVOID pParm);
ULONG   Audio_IOCTL_Status(PVOID pParm);
ULONG   Audio_IOCTL_Control(PVOID pParm);
ULONG   Audio_IOCTL_Buffer(PVOID pParm);
ULONG   Audio_IOCTL_Load(PVOID pParm);
ULONG   Audio_IOCTL_Wait(PVOID pParm);
ULONG   Audio_IOCTL_Hpi(PVOID pParm);

ULONG   InitStreams(VOID);
