/****************************************************************************/
/*                                                                          */
/* Module Name: OS2ME.H                                                     */
/*                                                                          */
/* OS/2 2.0 Multimedia Extensions top level include file that includes all  */
/* the files necessary for writing an OS/2 Multimedia application.          */
/*                                                                          */
/* Copyright (c) International Business Machines Corporation 1991, 1992     */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/

#define OS2ME_INCLUDED

#ifndef INCL_32
  #ifndef INCL_16
    #define INCL_32
  #endif
#endif

/* Common definitions */
#include "os2medef.h"
#include "meerror.h"

/* OS/2 SyncStream Manager Include File */
#include "ssm.h"

/* OS/2 Media Control Manager Include File */
#include "mmsystem.h"

#include "mcidrv.h"                   /* OS/2 MCI driver include file */

/* OS/2 Multimedia IO Services Include File */
#ifdef INCL_MMIO
#include "mmio.h"
#endif

/* OS/2 Multimedia Secondary Windows Include File */
#ifdef INCL_SW
#define INCL_SECONDARYWINDOW
#define INCL_GRAPHICBUTTON
#define INCL_CIRCULARSLIDER

#include "sw.h"
#endif
