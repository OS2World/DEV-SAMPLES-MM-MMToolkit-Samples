/****************************************************************************/
/*                                                                          */
/* Module Name: OS2ME.H                                                     */
/*                                                                          */
/* OS/2 2.0 Multimedia Extensions top level include file that includes all  */
/* the files necessary for writing an OS/2 Multimedia application.          */
/*                                                                          */
/*        Copyright (c) International Business Machines Corporation         */
/*                          1991, 1992, 1993                                */
/*                        All Rights Reserved                               */
/*                                                                          */
/****************************************************************************/

#define OS2ME_INCLUDED

#ifndef INCL_32
  #ifndef INCL_16
    #define INCL_32
  #endif
#endif

/* OS/2 Type Headers for MCI and MMIO           */
#ifdef INCL_OS2MM
    #define INCL_MCIOS2
    #define INCL_MMIOOS2
#endif

/* Common definitions */
#include "os2medef.h"
#include "meerror.h"

#ifdef INCL_MACHDR
    #include "mciapi.h"
#endif

/* OS/2 SyncStream Manager Include File */
#include "ssm.h"

/* OS/2 Media Control Manager Include File */
#ifdef INCL_MCIOS2
  #include "mcios2.h"                   /* OS/2 MCI Application interface */
  #include "mmdrvos2.h"                 /* OS/2 MCI driver include file   */
#else
  #include "mmsystem.h"                 /* MCI Application interface */
  #include "mcidrv.h"                   /* MCI driver include file   */
#endif

/* OS/2 Multimedia IO Services Include File */
#ifdef INCL_MMIO
  #include "mmio.h"
  #ifdef INCL_MMIO_CODEC
     #include "codec.h"
  #endif
#else
  #ifdef INCL_MMIOOS2
     #include "mmioos2.h"
     #ifdef INCL_MMIO_CODEC
        #include "codec.h"
     #endif
  #endif
#endif

/* OS/2 Multimedia Secondary Windows Include File */
#ifdef INCL_SW
#define INCL_SECONDARYWINDOW
#define INCL_GRAPHICBUTTON
#define INCL_CIRCULARSLIDER

#include "sw.h"
#endif
