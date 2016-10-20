/*static char *SCCSID = "@(#)fssh.h     13.1 92/02/15";*/
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/*                    Copyright (c) IBM Corporation 1991                    */
/*                           All Rights Reserved                            */
/*                                                                          */
/* SOURCE FILE NAME:  FSSH.H                                                */
/*                                                                          */
/* DESCRIPTIVE NAME:  File System Stream Handler definitions                */
/*                                                                          */
/* STATUS:  MM Release 1.00                                                 */
/*                                                                          */
/* FUNCTION: This file defines any values and external varaibles specific   */
/*   to the file system stream handler.                                     */
/*                                                                          */
/* NOTES:                                                                   */
/*                                                                          */
/* DEPENDENCIES:                                                            */
/*   SSM.H for RC typedef                                                   */
/*                                                                          */
/* Change History:                                                          */
/* Ptm     Date     Pgmr Comments                                           */
/* ======= ======== ===  ================================================== */
/*         11/15/90 MJP  Initial code                                       */
/* mm00056 05/24/91 MJP  Change FSSH_NUM_ESCB to 5 from 2 (waveform, adpcm, */
/*                       and video now supported datatypes for install prot.*/
/* mm00264 08/08/91 MJP  Delete FSSH_NUM_ESCB, changing to having linked    */
/*                       list of ESPCB's since install protocol is changed. */
/* mm00264 08/16/91 MJP  Add MMIOM_SEEKBYTIME for temp fix                  */
/* mm00??? 09/28/91 MJP  Take out the special SEEKBYTIME since it is now    */
/*                       supported by the standard mmio.h.  Also add more   */
/*                       comments since this is a toolkit sample.           */
/*************************** END OF SPECIFICATIONS **************************/
#define FSSH_HANDLER_NAME  "FSSH"
#define FSSH_VERSION       100
#define FSSH_PROCCNT_MTX   "\\SEM32\\FSSH"  // Name of 32bit Named Mutex
                                            // Semaphore used to gaurd access
                                            // to the process count variable.

extern RC FsshWrite();    // Write routine for recording to disk
extern RC FsshRead();     // Read routine to read from disk
