
/*static char *SCCSID = "@(#)mcd.h	13.35 92/04/13";*/
/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* Module NAME:  MCD.H                                                      */
/*                                                                          */
/*                                                                          */
/* OS/2 2.0 Multimedia Extensions MCD structures and definitions            */
/*                                                                          */
/* Copyright (c) International Business Machines Corporation 1990,1991      */
/*                         All Rights Reserved                              */
/************************** END OF SPECIFICATIONS ***************************/

#define INCL_SEMAPHORES
#define LIST_LEN           8
#pragma pack(4)
typedef struct _portlist {   /* devid */
                       ULONG ulDevType;
                       ULONG  ulDevNum;
                      } MCI_PORT_LIST;
#pragma pack()


/*-------------------------------------------------------------------------*
* Amp/Mixer instance structure declaration
*-------------------------------------------------------------------------*/

#pragma pack(4)

typedef struct _MCI_AMP_INSTANCE
        {
        CHAR szDeviceName[128];
        CHAR szDriverName[15];
        HFILE hFile;
        USHORT usDeviceID;
        ULONG ulGlobalFile;

        USHORT usMasterVolume;
        LONG lLeftVolume;
        LONG lRightVolume;
        LONG lVolumeDelay;

        LONG lBalance;

        LONG lBalanceDelay;

        LONG lBass;

        LONG lTreble;

        LONG lPitch;

        LONG lSRate;
        LONG lBitsPerSRate;
        LONG lBsize;
        SHORT sMode;
        SHORT sChannels;
        ULONG ulFlags;
        LONG lMonitor;
        LONG lInput;
        LONG lOutput;
        ULONG ulOperation;
        ULONG ulGainLevel;


        ULONG ulStoreRVolume;
        ULONG ulStoreLVolume;


        ULONG ulDosOpened;
        ULONG ulInitOpen;
        ULONG ulMasterVolume;
        ULONG ulActive;
        LONG  lStoreSRate;
        LONG  lStoreBitsPerSRate;
        ULONG ulStoreOperation;
        SHORT sStoreMode;
        SHORT sStoreChannels;
        ULONG ulSetAmpDefaults;
        ULONG ulDefaultsSet;

        ULONG ulOutputDev;
        ULONG ulInputDev;
        ULONG ulResourcesUsed;
        ULONG ulRestoreNotify;
        DWORD dwCallback;
        SHORT sUserParm;
        LONG  lNotifyFlag;
        LONG  ( * APIENTRY pfnAUDIOIF) ( PVOID, LONG, LONG, LONG, LONG);
        HMTX  hmtxInstanceSem;
        ULONG ulClass;
        ULONG ulBlockAlignment;
        DWORD ulHardwareMode;
        MCI_PORT_LIST  rInputList [ LIST_LEN ];
        MCI_PORT_LIST  rOutputList[ LIST_LEN ];
        ULONG ulNumInputs;
        ULONG ulNumOutputs;


        } MCI_AMP_INSTANCE;



typedef MCI_AMP_INSTANCE FAR *LPMCI_AMP_INSTANCE;
#pragma pack()
