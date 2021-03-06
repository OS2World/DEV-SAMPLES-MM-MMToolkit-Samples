/********************** START OF SPECIFICATIONS ****************************/
/*                                                                         */
/* Module Name: MInstall.h                                                 */
/*                                                                         */
/* DESCRIPTIVE NAME: Defines for creation of Installation Device Specific  */
/*                   Dlls.                                                 */
/*                                                                         */
/* ABSTRACT: This file contains information necessary for the creation of  */
/*           device specific Dlls. A message based interface exists so     */
/*           that theses Dlls can do such things as change config.sys,     */
/*           update the MME.INI file, and create WPS Objects.              */
/*                                                                         */
/* Copyright (c) International Business Machines Corporation 1990,1991     */
/*                         All Rights Reserved                             */
/*                                                                         */
/*********************** END OF SPECIFICATIONS *****************************/

#ifndef __MINSTALL_H_
#define __MINSTALL_H_

#define MAX_SETUP_STRING      1000     /* Longest setup string including   */
                                       /*   expanded macros                */

#define MAX_FOLDER_NAME       150      /* Longest desktop folder name      */

#define MAX_RESPONSE_LENGTH   256      /* Max device specific DLL response */

/* XLATOFF */
#pragma pack(4)
/* XLATON */

/***************************************************************************/
/* This is a function pointer prototype that describes the entry point     */
/* into the device specific DLL.                                           */
/*                                                                         */
/* Parameters:  HWND - Owner handle.                                       */
/*              PSZ  - Source path.                                        */
/*              PSZ  - Destination Drive (drive letter, colon - eg.  "X:") */
/*              PSZ  - Dll input parms in script file (DLL specific)       */
/*              HWND - Object window that receives message to do MCI and   */
/*                     CONFIG.SYS work.                                    */
/*              PSZ  - PSZ used for response file. (Is a CHAR [256] and    */
/*                     used for IN/OUT)                                    */
/***************************************************************************/
typedef ULONG APIENTRY VENDORFUNCTION(HWND,
                                          PSZ,
                                          PSZ,
                                          PSZ,
                                          HWND,
                                          PSZ);

typedef VENDORFUNCTION  * PVENDORFUNCTION;

/***************************************************************************/
/* MM_DISPATCHVARS and MM_DISPATCHMESSAGES() are two macros used by Device */
/* specific DLL's to keep the user interface responsive while they are     */
/* doing updates to MME.INI, config.sys, etc . . . This is necessary       */
/* because the device specific dll's are executing in install's msg queue  */
/* thread (thread 1).                                                      */
/*                                                                         */
/* MM_DISPATCHVARS should be declared in the scope required for the        */
/* MM_DISPATCHMESSAGES() macro to use them.                                */
/*                                                                         */
/* MM_DISPATHMESSAGES() should be used before and after every I/O intensive*/
/* operation.                                                              */
/*                                                                         */
/* An example would be:                                                    */
/*                                                                         */
/* void IOIntensive()                                                      */
/*    {                                                                    */
/*    MM_DISPATCHVARS;                                                     */
/*                                                                         */
/*    MM_DISPATHMESSAGES();                                                */
/*    DiskIO();                                                            */
/*    MM_DISPATHMESSAGES();                                                */
/*    }                                                                    */
/*                                                                         */
/* Please set the mouse pointer to SPTR_WAIT during operations lasting     */
/* than 1 second.                                                          */
/***************************************************************************/
#define MM_DISPATCHVARS        HAB MM_hab_ = WinQueryAnchorBlock(HWND_DESKTOP);\
                               QMSG MM_qmsg_

#define MM_DISPATCHMESSAGES()  while (WinPeekMsg(MM_hab_,                   \
                                                 &MM_qmsg_,                 \
                                                 (HWND)NULL,                \
                                                 0,                         \
                                                 0,                         \
                                                 PM_NOREMOVE))              \
                                  {                                         \
                                  WinGetMsg(MM_hab_, &MM_qmsg_, NULL, 0, 0);\
                                  WinDispatchMsg(MM_hab_, &MM_qmsg_);       \
                                  }

/***************************************************************************/
/* IM_MCI_EXTENDED_SYSINFO - Maps to MCI_SYSINFO.                          */
/*                                                                         */
/* mp1 = msg;                                                              */
/* mp2 = pvoid;                                                            */
/*                                                                         */
/* This will map to:                                                       */
/*         sysinfo.dwItem       = (ULONG)LONGFROMMP(mp1);                  */
/*         sysinfo.pSysInfoParm = PVOIDFROMMP(mp2);                        */
/*         mciSendCommand((WORD)0,                                         */
/*                        (WORD)MCI_SYSINFO,                               */
/*                        (DWORD)MCI_SYSINFO_ITEM,                         */
/*                        (DWORD)&sysinfo,                                 */
/*                        (WORD)0);                                        */
/***************************************************************************/
#define IM_MCI_EXTENDED_SYSINFO     0x057E

/* XLATOFF */
#pragma pack(1)
/* XLATON */

typedef struct _INSTIOPROC
   {
   CHAR  fccIOProc[5];
   CHAR  szDLLName[CCHMAXPATH];
   CHAR  szProcName[128];
   } INSTIOPROC;

typedef INSTIOPROC *PINSTIOPROC;

/* XLATOFF */
#pragma pack( )
/* XLATON */

/***************************************************************************/
/* IM_MMIO_INSTALL - Install an IO-Proc                                    */
/*                                                                         */
/* mp1 = 0;                        ** Not used                             */
/* mp2 = (PINSTIOPROC)&instioproc; ** pointer to and INSTIOPROC struct     */
/***************************************************************************/
#define IM_MMIO_INSTALL    0x057F

/***************************************************************************/
/* IM_SPI_INSTALL - Install stream protocol information                    */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = (PSZ)pszDllPath;  ** Fully qualified path of a SPI resource DLL   */
/***************************************************************************/
#define IM_SPI_INSTALL     0x0580

#define INST_MAXCLASSNAME   256
#define INST_MAXTITLE       256
#define INST_MAXSETUPSTRING 1024
#define INST_MAXLOCATION    256


typedef struct _INSTOBJECTDATA
   {
   CHAR  achClassName[INST_MAXCLASSNAME];
   CHAR  achTitle[INST_MAXTITLE];
   CHAR  achSetupString[INST_MAXSETUPSTRING];
   CHAR  achLocation[INST_MAXLOCATION];
   ULONG ulFlags;
   } INSTOBJECTDATA;

typedef INSTOBJECTDATA *PINSTOBJECTDATA;

/***************************************************************************/
/* IM_CREATE_WPS_OBJECT - Install a folder and its contents                */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = MPFROMP(pinstOBJECTdata);                                         */
/***************************************************************************/
#define IM_CREATE_WPS_OBJECT  0x0571

/***************************************************************************/
/* IM_DESTROY_WPS_OBJECT - Install a folder and its contents               */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = MPFROMP(HOBJECT); ** or "<OBJECT_ID>"                             */
/***************************************************************************/
#define IM_DESTROY_WPS_OBJECT  0x0572

/***************************************************************************/
/* IM_LOG_ERROR - Write a message to the log file.                         */
/*                                                                         */
/* mp1 = (PSZ)pszStatement;   ** Should end with a '\n' and be NULL        */
/*                            **  terminated                               */
/* mp2 = 0;                   ** Unused                                    */
/***************************************************************************/
#define IM_LOG_ERROR       0x0573

/***************************************************************************/
/* CONFIGDATA - Pointer to this structure is passed in mp1 in all CONFIG   */
/*              messages.                                                  */
/***************************************************************************/
typedef struct _CONFIGDATA
   {
   LONG lLine;
   LONG lBufferLen;
   PSZ  pszBuffer;
   BOOL fAdd;
   } CONFIGDATA;

typedef CONFIGDATA *PCONFIGDATA;

/***************************************************************************/
/* IM_CONFIGENUMERATE - Get a line from the config.sys file.               */
/*                                                                         */
/* mp1 = (PCONFIGDATA)pconfdata;                                           */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGENUMERATE 0x0574

/***************************************************************************/
/* IM_CONFIGUPDATE - Update an entry in the config.sys file.               */
/*                                                                         */
/* mp1 = (PCONFIGDATA)pconfdata;                                           */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGUPDATE    0x0575

/***************************************************************************/
/* IM_CONFIGMERGE - Merge data into an existing config.sys entry.          */
/*                                                                         */
/* mp1 = (PCONFIGDATA)pconfdata;                                           */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGMERGE     0x0576

/***************************************************************************/
/* IM_CONFIGEREPLACE - Replace an entry that exists in config.sys.         */
/*                                                                         */
/* mp1 = (PCONFIGDATA)pconfdata;                                           */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGREPLACE   0x0577

/***************************************************************************/
/* IM_CONFIGNEW - Add a line to the config.sys file.                       */
/*                                                                         */
/* mp1 = (PCONFIGDATA)pconfdata;                                           */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGNEW       0x0578

/***************************************************************************/
/* IM_CONFIGDELETE - Delete a line (REM it out) from the config.sys file.  */
/*                                                                         */
/* mp1 = (PCONFIGDATA)pconfdata;                                           */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGDELETE    0x0579

/***************************************************************************/
/* IM_CONFIGQUERYCHANGED - Returns TRUE if config.sys has changed else     */
/*                         FALSE.                                          */
/*                                                                         */
/* mp1 = 0                                                                 */
/* mp2 = 0                                                                 */
/***************************************************************************/
#define IM_CONFIGQUERYCHANGED    0x057A


typedef struct _MIDIMAPINSTALLDATA
   {
   CHAR  achIniName[CCHMAXPATH];
   CHAR  achAppName[1024];
   CHAR  achKeyName[1024];
   CHAR  achDllName[CCHMAXPATH];
   ULONG ulResourceID;
   } MIDIMAPINSTALLDATA;

typedef MIDIMAPINSTALLDATA *PMIDIMAPINSTALLDATA;

/***************************************************************************/
/* IM_MIDIMAP_INSTALL - Install a midi map.                                */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = MPFROMP(pmidimapinstalldata);                                     */
/*                                                                         */
/* Return 0 on success, else error occurred.                               */
/***************************************************************************/
#define IM_MIDIMAP_INSTALL 0x057B

typedef struct _INSTMCISENDCOMMAND
   {
   USHORT wDeviceID;
   USHORT wMessage;
   ULONG  dwParam1;
   ULONG  dwParam2;
   USHORT wUserParm;
   } INSTMCISENDCOMMAND;

typedef INSTMCISENDCOMMAND *PINSTMCISENDCOMMAND;

/***************************************************************************/
/* IM_MCI_SEND_COMMAND - Send an MCI command                               */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = MPFROMP(PINSTMCISENDCOMMAND);                                     */
/*                                                                         */
/* Return mciSendCommand result.                                           */
/***************************************************************************/
#define IM_MCI_SEND_COMMAND         0x057C

typedef struct _INSTEAJOIN
   {
   CHAR achFileName[CCHMAXPATH];
   CHAR achEAFileName[CCHMAXPATH];
   } INSTEAJOIN;

typedef INSTEAJOIN *PINSTEAJOIN;

/***************************************************************************/
/* IM_EA_JOIN                                                              */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = MPFROMP(PINSTEAJOIN);                                             */
/*                                                                         */
/* Return EA join result.                                                  */
/***************************************************************************/
#define IM_EA_JOIN                  0x057D

typedef struct _INSTEALONGNAMEJOIN
   {
   CHAR achLongName[CCHMAXPATH];
   CHAR achLongFileName[CCHMAXPATH];
   CHAR achEALongFileName[CCHMAXPATH];
   } INSTEALONGNAMEJOIN;

typedef INSTEALONGNAMEJOIN *PINSTEALONGNAMEJOIN;

/***************************************************************************/
/* IM_EA_LONG_NAME_JOIN                                                    */
/*                                                                         */
/* mp1 = 0;                ** Not used                                     */
/* mp2 = MPFROMP(PINSTEALONGNAMEJOIN);                                     */
/*                                                                         */
/* Return EA long name creation result.                                    */
/***************************************************************************/
#define IM_EA_LONG_NAME_JOIN                  0x0581

#ifndef NO_ERROR
#define NO_ERROR 0
#endif

#define CFGERR_BASE                    0xFF00
#define CFGERR_START                   (CFGERR_BASE + 1)

#define CFGERR_OPENFAILED              (CFGERR_START +  1)
#define CFGERR_OUTOFMEMORY             (CFGERR_START +  2)
#define CFGERR_INVALIDPARAMETER        (CFGERR_START +  3)
#define CFGERR_INVALIDENUMFLAG         (CFGERR_START +  4)
#define CFGERR_ATTOP                   (CFGERR_START +  5)
#define CFGERR_ATBOTTOM                (CFGERR_START +  6)
#define CFGERR_BUFFEROVERFLOW          (CFGERR_START +  7)
#define CFGERR_LINENOTFOUND            (CFGERR_START +  8)
#define CFGERR_CANTPARSEBUFFER         (CFGERR_START +  9)
#define CFGERR_VARIABLENOTFOUND        (CFGERR_START + 10)

/* Line positions */
#define CONFIG_TOP      -1
#define CONFIG_BOTTOM   -2
#define CONFIG_NEXT     -3
#define CONFIG_PREV     -4
#define CONFIG_CURRENT  -5

/* XLATOFF */
#pragma pack( )
/* XLATON */

#endif /* #ifndef __MINSTALL_H_ */

