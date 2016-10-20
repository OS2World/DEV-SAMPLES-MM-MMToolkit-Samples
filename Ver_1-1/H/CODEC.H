/****************************************************************************/
/*                                                                          */
/* Module Name: CODEC.H                                                     */
/*                                                                          */
/* OS/2 2.0 Multimedia Extensions CODEC  Definitions file                   */
/*                                                                          */
/* Copyright (c) International Business Machines Corporation 1992, 1993,    */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/

#ifndef _CODEC_H_
#define _CODEC_H_

/* XLATOFF */
#pragma pack(4)
/* XLATON */

/******************
 * Defines:
 ******************/

/* Ultimotion CODEC type for CODECINIFILEINFO ulCompressType */
#define  FOURCC_ULTI       mmioFOURCC('U', 'L', 'T', 'I')
#define  HEX_FOURCC_ULTI   0x49544C55L     /* ITLU */

/* Indeo CODEC type for CODECINIFILEINFO ulCompressType */
#define  FOURCC_RT21       mmioFOURCC('R', 'T', '2', '1')
#define  HEX_FOURCC_RT21   0x31325452L     /* 12TR */



/**********************************************
 *
 * GENPAL - Generic Header Palette
 *
 **********************************************/
typedef struct _GENPAL {          /* genpal */
   ULONG       ulStartIndex;      /* starting RGB index */
   ULONG       ulNumColors;       /* number of following entries */
   PRGB2       prgb2Entries;      /* 256 RGB entries */
   } GENPAL;
typedef GENPAL *PGENPAL;          /* Ptr to a generic palette */


/**********************************************
 *
 * RECTL1 - Rectangle Structure (TEMP - h2inc problem)
 *
 **********************************************/
typedef struct _RECTL1 {          /* rcl1 */
    LONG  xLeft;
    LONG  yBottom;
    LONG  xRight;
    LONG  yTop;
} RECTL1;
typedef RECTL1 *PRECTL1;


/**********************************************
 *
 * CODECVIDEOHEADER - CODEC video Header
 *
 **********************************************/
typedef struct _CODECVIDEOHEADER {  /* codecvidhdr */
   ULONG  ulStructLen;
   ULONG  cx;
   ULONG  cy;
   USHORT cPlanes;
   USHORT cBitCount;
   ULONG  ulColorEncoding;
   GENPAL genpal;
} CODECVIDEOHEADER;
typedef CODECVIDEOHEADER *PCODECVIDEOHEADER;

/* ulColorEncoding defines: */
#define  MMIO_RGB_5_6_5    0x0001L  /* Each pixel is a RGB_5_6_5 datatype */
#define  MMIO_RGB_24       0x0002L  /* Each pixel is a RGB_24 datatype */
#define  MMIO_YUV_4_1_1    0x0004L  /* Each pixel is a YUV_4_1_1 datatype */
#define  MMIO_COMPRESSED   0x0008L  /* The data is compressed */
#define  MMIO_YUV_24       0x0010L  /* Each pixel is a YUV_24 datatype */
#define  MMIO_PALETTIZED   0x0020L  /* The data is palettized */


/**********************************************
 *
 * MMVIDEOOPEN - Video Open Structure
 *
 * This structure is passed on the CODEC open
 * message when video compression is being done
 * to indicate information such as quality,
 * frame rate, data rate, and key frame rate.
 *
 * Quality:
 *
 * The ulQuality field specifies a scalar value
 * in the range 0 - 10000, where 0 is the lowest
 * quality and 10000 is the highest quality. A
 * value of -1 specifies the default quality level,
 * and the default quality level (e.g. 5000) is
 * returned in the ulQuality field.
 *
 *
 * Key Frame rate:
 *
 * The ulKeyFrameRate structure specifies the key
 * frame (aka I-frame, reference frame) frequency.
 * Every Nth frame is a key frame as specified.
 * A value of zero specifies that no periodic key
 * are to be compressed.  Additional key frames may
 * be inserted at any point by specifying
 * MMIO_IS_KEY_FRAME in the MMCOMPRESS structure.
 *
 * example:  ulKeyFrameRate = 5  results in:
 *
 *    key delta delta delta delta key delta delta delta delta key delta...
 *
 *
 * Frame rate:
 *
 * Rate = number of time units per second
 * Scale = number of time units per frame
 *
 * examples:  Rate = 30  Scale = 1     =>    30 FPS
 *            Rate = 15  Scale = 1     =>    15 FPS
 *            Rate = 25  Scale = 2     =>    12.5 FPS
 *
 *
 * Data Constraint:
 *
 * Compressors which are capable of constraining the
 * resultant compressed video data rate use the
 * information in the ulDataConstraint and
 * ulConstraintInterval fields.  A non-zero value
 * in ulDataConstraint specifies the number of bytes
 * which is not to be exceeded over an interval of
 * frames in the output data stream, regardless of
 * the requested quality level.  This value only
 * considers video data, i.e. audio data and file format
 * overhead must be considered seperately when determining
 * the final output file data rate.  The interval of
 * frames over which the data is constrained is specified
 * in ulConstraintInterval.  A value of zero for
 * ulDataContraint specifies that the data rate is not
 * to be constrained and is to compressed according to
 * the requested quality level.
 *
 * example 1:  ulDataConstraint = 150000   ulConstraintInterval = 15
 *
 *             This results in an output stream wherein the sizes of any 15
 *             consecutive frames does not exceed 150000 bytes.  If the
 *             frame rate is 15 FPS, the resultant data rate will not
 *             exceed 150000 bytes per second.
 *
 * example 2:  ulDataConstraint = 10000    ulConstraintInterval = 1
 *
 *             This results in an output stream wherein any single frame
 *             does not exceed 10000 bytes.  If the frame rate is 15 FPS,
 *             the resultant data rate will not exceed 150000 bytes per
 *             second.  Note the difference between this case and example 1
 *             where individual frames may exceed 10000 bytes (the average)
 *             so long other frames in any 15 frame sequence are sufficiently
 *             smaller to satisfy the constraint within the constraint interval.
 *
 **********************************************/
typedef struct _MMVIDEOOPEN  {      /* mmvidopen */
   ULONG       ulStructLen;
   ULONG       ulQuality;
   ULONG       ulKeyFrameRate;
   ULONG       ulScale;
   ULONG       ulRate;
   ULONG       ulDataConstraint;
   ULONG       ulConstraintInterval;
   } MMVIDEOOPEN;
typedef MMVIDEOOPEN *PMMVIDEOOPEN;


/**********************************************
 *
 * CODECOPEN - CODEC open structure
 *
 **********************************************/
typedef struct _CODECOPEN  {        /* codecopen */
   ULONG       ulFlags;             /* flags & events - Refer to ulCapsFlags in CODECINIFILEINFO */
   PVOID       pControlHdr;         /* control header - (codec specific) */
   PVOID       pSrcHdr;             /* source header - Ptr CODECVIDEOHEADER */
   PVOID       pDstHdr;             /* destination header - Ptr CODECVIDEOHEADER */
   PVOID       pOtherInfo;          /* other information - Ptr MMVIDEOOPEN */
   } CODECOPEN;
typedef CODECOPEN *PCODECOPEN;

#define VALID_CODECOPEN_INPUTFLAGS   (CODEC_DECOMPRESS |       \
                                      CODEC_WINDOW_CLIPPING |  \
                                      CODEC_PALETTE_TRANS |    \
                                      CODEC_SELFHEAL |         \
                                      CODEC_SCALE_PEL_DOUBLE | \
                                      CODEC_SCALE_PEL_HALVED | \
                                      CODEC_SCALE_CONTINUOUS | \
                                      CODEC_MULAPERTURE |      \
                                      CODEC_HARDWARE |         \
                                      CODEC_DIRECT_DISPLAY)


/**********************************************
 *
 * MMCOMPRESS - Compress structure
 *
 **********************************************/
typedef struct _MMCOMPRESS  {     /* mmcomp */
   ULONG       ulStructLen;       /* length of this structure */
   ULONG       ulFlags;           /* command and status flags */
   ULONG       ulSrcBufLen;       /* source buffer size */
   PVOID       pSrcBuf;           /* source buffer */
   ULONG       ulDstBufLen;       /* destination buffer length */
   PVOID       pDstBuf;           /* destination buffer */
   PVOID       pRunTimeInfo;      /* control information - Ptr MMVIDEOCOMPRESS */
   } MMCOMPRESS;
typedef MMCOMPRESS *PMMCOMPRESS;

/* ulFlags defines: (Defined below) */
/* #define  MMIO_IS_KEY_FRAME 0x0001L */ /* This bit is set by the application */
                                         /* to instruct the IOProc to compress */
                                         /* the pSrcBuf into a key or reference */
                                         /* frame. If the bit is not set, a delta */
                                         /* frame is compressed. */
/* #define  MMIO_IS_PALETTE    0x0002L */ /* A video palette is provided.  This is */
                                          /* set by the application. */


/**********************************************
 *
 * MMVIDEOCOMPRESS - Video Compress structure
 *
 **********************************************/
typedef struct _MMVIDEOCOMPRESS { /* mmvidcomp */
   ULONG       ulStructLen;       /* Structure length */
   GENPAL      genpalVideo;       /* Video stream palette */
   PVOID       pControlHdr;       /* control header (codec specific) */
   } MMVIDEOCOMPRESS;
typedef MMVIDEOCOMPRESS *PMMVIDEOCOMPRESS;


/**********************************************
 *
 * MMDECOMPRESS - Decompress Structure
 *
 **********************************************/
typedef struct _MMDECOMPRESS  {   /* mmdec */
   ULONG       ulStructLen;       /* length of this structure */
   ULONG       ulFlags;           /* command and status flags */
   ULONG       ulSrcBufLen;       /* source buffer size */
   PVOID       pSrcBuf;           /* source buffer */
   ULONG       ulDstBufLen;       /* destination buffer length */
   PVOID       pDstBuf;           /* destination buffer */
   PVOID       pRunTimeInfo;      /* control information Ptr to MMVIDEODECOMPRESS */
   } MMDECOMPRESS;
typedef MMDECOMPRESS *PMMDECOMPRESS;

/* ulFlags defines: */
#define MMIO_DROP_DELTA_FRAME 0x0001L /* Input/Output - Tells the IOProc to drop the delta */
                                      /* frame if the pSrcBuf contains a delta */
                                      /* frame.  On return, the bit is reset */
                                      /* if the delta frame is dropped. */
#define MMIO_IS_KEY_FRAME     0x0002L /* Output - This bit is set by the IOProc when */
                                      /* the data contained in the pSrcBuf is */
                                      /* a key or reference frame. */
#define MMIO_IS_PALETTE       0x0004L /* Output - A video palette has been found. */
                                      /* This is set by the IOProc. */
#define MMIO_PALETTE_CHANGE   0x0008L /* Input - The physical palette has been changed */
                                      /* in...  This is set by the application. */
#define MMIO_ORIGIN_LOWERLEFT 0x0010L /* Input - The video frame origin */
#define MMIO_RECTL_CHANGE     0x0020L /* Input - The valid rectl list has changed. */
#define MMIO_ORIGIN_UPPERLEFT 0x0040L /* Input - The video frame origin */
#define MMIO_DROP_FRAME_DECODE 0x0080L /* Input - Tells the IOProc to drop decoding  */
                                      /* of the frame.                         */

#define VALID_DECOMPRESS_INPUTFLAGS   (MMIO_DROP_DELTA_FRAME | \
                                       MMIO_PALETTE_CHANGE |   \
                                       MMIO_ORIGIN_LOWERLEFT | \
                                       MMIO_RECTL_CHANGE |     \
                                       MMIO_DROP_FRAME_DECODE | \
                                       MMIO_ORIGIN_UPPERLEFT)


/**********************************************
 *
 * MMVIDEODECOMPRESS - Video Decompress structure
 *
 **********************************************/
typedef struct _MMVIDEODECOMPRESS {    /* mmviddec */
   ULONG       ulStructLen;            /* Structure length */
   ULONG       ulRectlCount;           /* Valid rectangle count - for clipping */
   PRECTL1     prectl;                 /* Valid rectangle array - for clipping */
   ULONG       ulSkipLength;           /* Skipped line length */
   ULONG       ulDecodeLines;          /* Num of lines to decompress */
   GENPAL      genpalPhysical;         /* Physical palette */
   GENPAL      genpalVideo;            /* Video stream palette */
   RECTL1      rectlSrc;               /* Source window rectangle */
   RECTL1      rectlDst;               /* Destination window rectangle */
   } MMVIDEODECOMPRESS;
typedef MMVIDEODECOMPRESS *PMMVIDEODECOMPRESS;


/*************************************************
 *
 * RECORDTAB - Record table
 *
 * NOTE: This structure maps to ESRCBUFTAB in ssm.h
 *************************************************/
typedef struct _RECORDTAB {       /* recordtab */
   ULONG       ulReserved1;       /* reserved for system */
   PVOID       pRecord;           /* ptr to record in buffer */
   ULONG       ulLength;          /* length of record */
   ULONG       ulReserved2;       /* reserved for system */
   ULONG       ulReserved3;       /* reserved for system */
   ULONG       ulParm1;           /* Record specific data */
   ULONG       ulParm2;           /* Record specific data */
   } RECORDTAB;
typedef RECORDTAB FAR *PRECORDTAB;/* Ptr to a buffer entry  */


/* ulParm1 Return values for MULTITRACK_READ only: */
#define MMIO_INVISIBLE_FRAME   0x1000L


/* ulParm1 Input values for MULTITRACK_WRITE only: */
/* #define  MMIO_IS_KEY_FRAME  0x0001L */ /* Frame is a Key frame */
/* #define  MMIO_IS_PALETTE    0x0002L */ /* Buffer contains a video palette  */



/************************************************
 *
 *  TRACKMAP - This structure maps a track to
 *             a record table.
 *
 ************************************************/
typedef struct _TRACKMAP {        /* trackmap */
   ULONG       ulTrackID;         /* Input - track ID */
   ULONG       ulNumEntries;      /* Input - number of record entries */
   PRECORDTAB  pRecordTabList;    /* Input/Output - Ptr to a record table */
   } TRACKMAP;
typedef TRACKMAP *PTRACKMAP;      /* Ptr to a track map table entry */


/**********************************************
 *
 * MMMULTITRACKREAD - Multiple Track Read
 *
 **********************************************/
typedef struct _MMMULTITRACKREAD  {   /* mtread */
   ULONG       ulLength;          /* Input - length of read buffer */
   PVOID       pBuffer;           /* Input - ptr to read buffer */
   ULONG       ulFlags;           /* Output - read flags */
   ULONG       ulNumTracks;       /* Input - number of track entries */
   PTRACKMAP   pTrackMapList;     /* Input - ptr to track-to-record list */
   } MMMULTITRACKREAD;
typedef MMMULTITRACKREAD *PMMMULTITRACKREAD;

/* ulFlags Defines: */
#define MULTITRACKREAD_NOTDONE        0x0001L /* Read is not done. Another read of the same */
                                              /* buffer is necessary. There were not enough record */
                                              /* entries in the record table passed to this api. */
#define MULTITRACKREAD_EOF            0x0002L /* End of File. Used because # bytes read may not */
                                              /* match the length of the buffer in cases of a */
                                              /* record that spans in the next buffer. */


/**********************************************
 *
 * MMMULTITRACKWRITE - Multiple Track Write
 *
 **********************************************/
typedef struct _MMMULTITRACKWRITE  {   /* mtwrite */
   ULONG       ulNumTracks;       /* number of track entries */
   PTRACKMAP   pTrackMapList;     /* ptr to track-to-record list */
   } MMMULTITRACKWRITE;
typedef MMMULTITRACKWRITE *PMMMULTITRACKWRITE;


/**********************************************
 *
 * MMMOVIEHEADER - standard movie header data
 *
 **********************************************/
typedef struct _MMMOVIEHEADER {   /* mmhdr */
   ULONG         ulStructLen;       /* length of this structure */
   ULONG         ulReserved;        /* Reserved for future use */
   ULONG         ulMediaType;       /* video media type */
   ULONG         ulMovieCapsFlags;  /* capabilities */
   ULONG         ulMaxBytesPerSec;  /* maximum transfer rate */
   ULONG         ulPaddingGranularity;/* pad to a multiple of this size */
   ULONG         ulSuggestedBufferSize;
   ULONG         ulStart;
   ULONG         ulLength;
   ULONG         ulNextTrackID;     /* next available track id */
   ULONG         ulNumEntries;      /* number of track entries */
   PMMTRACKINFO  pmmTrackInfoList;  /* track information */
   PSZ           pszMovieTitle;     /* movie title */
   ULONG         ulCountry;         /* country code for the title string */
   ULONG         ulCodePage;        /* country code page the title string */
   } MMMOVIEHEADER;
typedef MMMOVIEHEADER *PMMMOVIEHEADER;

/* ulContentType Defines: */
#define MMIO_VIDEO_UNKNOWN          0x00000000L  /* Unknown video content */
#define MMIO_VIDEO_DATA             0x00000001L  /* Video                 */

/* ulMovieCapsFlags Defines: */
#define  MOVIE_HAS_VIDEO         0x0001L  /* The movie contains video. */
#define  MOVIE_HAS_AUDIO         0x0002L  /* The movie contains audio. */
#define  MOVIE_CAN_SEEK          0x0004L  /* The movie can seek. */
#define  MOVIE_CAN_SCAN          0x0008L  /* The movie can fast scan. */
#define  MOVIE_HAS_COPYRIGHT     0x0010L  /* The movie contains copyrighted data. */
#define  MOVIE_WAS_CAPTUREFILE   0x0020L  /* The movie is a specially allocated  */
                                          /* file used for capturing real-time */
                                          /* video.  Applications should warn  */
                                          /* the user before writing over a file */
                                          /* with this flag set because the user  */
                                          /* probably defragmented this file. */

/**********************************************
 *
 * MMVIDEOHEADER - Movie Video Track Header
 *
 **********************************************/
typedef struct _MMVIDEOHEADER {   /* mmvhdr */
   ULONG         ulStructLen;       /* length of this structure */
   ULONG         ulContentType;     /* video content type */
   ULONG         ulMediaType;       /* video media type */
   ULONG         ulVideoCapsFlags;  /* capabilities */
   ULONG         ulWidth;           /* video width in pels */
   ULONG         ulHeight;          /* video height in pels */
   ULONG         ulScale;
   ULONG         ulRate;            /* Rate / Scale == frames/second */
   ULONG         ulStart;
   ULONG         ulLength;
   ULONG         ulTotalFrames;     /* total number of video frames */
   ULONG         ulInitialFrames;
   MMTIME        mmtimePerFrame;    /* frame display time or 0L */
   ULONG         ulSuggestedBufferSize;
   GENPAL        genpalVideo;       /* palette */
   PMMXDIBHEADER pmmXDIBHeader;     /* windows DIB compatible header */
   } MMVIDEOHEADER;
typedef MMVIDEOHEADER *PMMVIDEOHEADER;

/* ulContentType Defines: */
#define MMIO_VIDEO_UNKNOWN          0x00000000L  /* Unknown video content */
#define MMIO_VIDEO_DATA             0x00000001L  /* Video                 */

/* XLATOFF */
#pragma pack()
/* XLATON */

#endif /* _CODEC_H_ */
