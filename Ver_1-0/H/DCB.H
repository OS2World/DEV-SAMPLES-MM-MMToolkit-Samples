/****************************************************************************/
/*                                                                          */
/* Module Name: DCB.H                                                       */
/*                                                                          */
/* OS/2 2.0 Multimedia Extensions Sync/Stream Manager Device Control        */
/* Block defintitions.                                                      */
/*                                                                          */
/* Copyright (c) International Business Machines Corporation 1991, 1992     */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/

/* XLATOFF */
#pragma pack(1)
/* XLATON */

/**********************************************
 *
 * DCB - Common Device Control Block
 *               (*** PACKED STRUCTURE ***)
 **********************************************/
typedef struct _dcb { /* dcb  -  Device Control Block                      */
   ULONG   ulDCBLen;                /* length of structure                 */
   SZ      szDevName[MAX_SPI_NAME]; /* device driver name                  */
   } DCB;
typedef DCB  FAR *PDCB;


/**********************************************
 *
 * DCB_AUDIOSH - Device Control Block for the
 *               audio stream handler.
 *               (*** PACKED STRUCTURE ***)
 **********************************************/
typedef struct _dcb_audiosh { /* dcb_audiosh  -  Device Control Block      */
   ULONG   ulDCBLen;                /* length of structure                 */
   SZ      szDevName[MAX_SPI_NAME]; /* device driver name                  */
   ULONG   ulSysFileNum;            /* File handle number                  */
   } DCB_AUDIOSH;
typedef DCB_AUDIOSH  FAR  *PDCB_AUDIOSH;

/* XLATOFF */
#pragma pack()
/* XLATON */
