/*static char *SCCSID = "@(#)mcd.h      13.35 92/04/13";*/
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

#define LINE_IN_ON                              0x00000001
#define MIC_ON                                  0x00000002
#define LINE_OUT_ON                             0x00000004


#define VSD_SET_VOLUME          0x00000001
#define VSD_SET_TREBLE          0x00000002
#define VSD_SET_BASS            0x00000004
#define VSD_SET_PITCH           0x00000008
#define VSD_SET_GAIN            0x00000010
#define VSD_SET_CONN            0x00000020
#define VSD_SET_MONITOR         0x00000040
#define VSD_SET_BALANCE         0x00000080
#define VSD_SET_ALL             0xffffffff


/*-------------------------------------------------------------------------*
* RIFF to ACPA Mode conversion Table
*-------------------------------------------------------------------------*/

typedef struct
        {
        ULONG  ulDataType;
        ULONG  ulAUDIODDMode;
        } RIFFTABLE;



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
        ULONG ulCallback;
        SHORT sUserParm;
        LONG  lNotifyFlag;
        LONG  ( * APIENTRY pfnAUDIOIF) ( PVOID, LONG, LONG, LONG, LONG);
        HMTX  hmtxInstanceSem;
        ULONG ulClass;
        ULONG ulBlockAlignment;
        ULONG ulHardwareMode;
        MCI_PORT_LIST  rInputList [ LIST_LEN ];
        MCI_PORT_LIST  rOutputList[ LIST_LEN ];
        ULONG ulNumInputs;
        ULONG ulNumOutputs;

        ULONG ulDataType;
        ULONG ulSubType;
        ULONG ulBytesPerBlock;
        ULONG ulMMTimePerBlock;
        ULONG ulCanRecord;
        ULONG ulVolControl;

// new resource management stuff


        PVOID    pAudioModeData;     // pointer to dev. specific resource data
        CHAR     pResourceDLL[ 128 ];

        PSZ        pProdInfo;          // pointer to the product name
        ULONG      ulDeviceID;         // pointer to the device id.
        ULONG      ulNumMappingRows;   // number of RIFF->AUDIODD mapping modes
        ULONG      ulMatch;
        RIFFTABLE  *pMapTable;        // additional RIFF->AUDIODD mappings

        ULONG      ulNumDataTypes;   // number of datatypes to check
        PVOID      pDataTypeTable;
        BOOL       fMute;
        ULONG      ulResourceID;     // offset in the resource dll to use
        ULONG      ulConnDefaults;   // Default values for connectors

        } MCI_AMP_INSTANCE;



typedef MCI_AMP_INSTANCE FAR *PMCI_AMP_INSTANCE;
#pragma pack()




#pragma pack(4)

typedef struct _MCI_OLD_AMP_INSTANCE
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
        ULONG ulCallback;
        SHORT sUserParm;
        LONG  lNotifyFlag;
        LONG  ( * APIENTRY pfnAUDIOIF) ( PVOID, LONG, LONG, LONG, LONG);
        HMTX  hmtxInstanceSem;
        ULONG ulClass;
        ULONG ulBlockAlignment;
        ULONG ulHardwareMode;
        MCI_PORT_LIST  rInputList [ LIST_LEN ];
        MCI_PORT_LIST  rOutputList[ LIST_LEN ];
        ULONG ulNumInputs;
        ULONG ulNumOutputs;


        } MCI_OLD_AMP_INSTANCE;

#pragma pack()



