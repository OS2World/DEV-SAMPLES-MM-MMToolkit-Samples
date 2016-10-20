
/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  AUDIODAT.C        (TEMPLATE SAMPLE)                   */
/*                                                                          */
/* DISCRIPTIVE NAME: Audio device driver GLOBAL data                        */
/*                                                                          */
/************************** END OF SPECIFICATIONS ***************************/

#define         INCL_NOPMAPI
#define         INCL_DOS
#define         INCL_DOSDEVICES
#define         INCL_ERRORS
#include        <os2.h>

#include        <devhdr.h>              // attribute defines
#include        <devcmd.h>              // device driver strategy cmds
#include        <os2medef.h>
#include        <ssm.h>
#include        <audio.h>
#include        "audiodd.h"

/****************************************************************************/
/*                       E X T E R N S                                      */
/****************************************************************************/

//************************
// Both are in STARTUP.ASM
//************************
extern  Strategy();                     // IOCTL entry point
extern  DDCMDEntryPoint();
//extern RC FAR DDCMDEntryPoint(PVOID pCommon);
                                        // Device driver command entry point

/****************************************************************************/
/*                      D A T A                                             */
/****************************************************************************/

//*******************************************************
// Device driver header.                                *
//*******************************************************
DEVHDR  DevHdr = {(FPVOID) 0xFFFFFFFF,          // link (filled in by OS)
                  AUDIO_ATTRIB,                 // attribute
                  (NPVOID)Strategy,             // Offset of Strategy routine
                  (NPVOID)DDCMDEntryPoint,      // Offset of IDC routine
                  "AUDIOn$ ",                   // name where n=1-9
                  "\0\0\0\0\0\0\0\0"};          // 8 bytes reserved
FPVOID  DevHlp;


GLOBAL  GlobalTable = {
             NULL,                  // paStream dynamically allocated
             DEFAULTSTREAMS};       // number of streams


PROTOCOLTABLE ProtocolTable[NPROTOCOLS] =
  {
    { DATATYPE_MIDI,      SUBTYPE_NONE,        512L,  3, 0,     0, 0},
    { DATATYPE_ADPCM_AVC, ADPCM_AVC_VOICE,  8*1024L,  3, 0,     0, 0},
    { DATATYPE_ADPCM_AVC, ADPCM_AVC_MUSIC,  16*1024L, 3, 0,     0, 0},
    { DATATYPE_ADPCM_AVC, ADPCM_AVC_STEREO, 16*1024L, 3, 0,     0, 0},
    { DATATYPE_ADPCM_AVC, ADPCM_AVC_HQ,     32*1024L, 3, 0,     0, 0},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_1M08, 4*1024L,  3, 11025, 1, 8},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_1S08, 8*1024L,  3, 11025, 2, 8},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_1M16, 8*1024L,  4, 11025, 1, 16},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_1S16, 16*1024L, 4, 11025, 2, 16},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_2M08, 8*1024L,  3, 22050, 1, 8},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_2S08, 16*1024L, 4, 22050, 2, 8},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_2M16, 16*1024L, 4, 22050, 1, 16},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_2S16, 32*1024L, 4, 22050, 2, 16},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_4M08, 16*1024L, 4, 44100, 1, 8},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_4S08, 32*1024L, 4, 44100, 2, 8},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_4M16, 32*1024L, 4, 44100, 1, 16},
    { DATATYPE_WAVEFORM,  WAVE_FORMAT_4S16, 60*1024L, 6, 44100, 2, 16},
    { DATATYPE_MULAW,     MULAW_8B8KS,      60*1024L, 6, 8000,  2, 8},  /* 8k 8b Mu-Law   */
    { DATATYPE_MULAW,     MULAW_8B11KS,     60*1024L, 6, 11025, 2, 8},  /* 11K            */
    { DATATYPE_MULAW,     MULAW_8B22KS,     60*1024L, 6, 22050, 2, 8},  /* 22k 8b Mu-Law  */
    { DATATYPE_MULAW,     MULAW_8B44KS,     60*1024L, 6, 44100, 2, 8},  /* 44k 8b Mu-Law  */
    { DATATYPE_ALAW,      ALAW_8B8KS,       60*1024L, 6, 8000,  2, 8},  /* 8k 8b Mu-Law   */
    { DATATYPE_ALAW,      ALAW_8B11KS,      60*1024L, 6, 11025, 2, 8},  /* 11k 8b Mu-Law  */
    { DATATYPE_ALAW,      ALAW_8B22KS,      60*1024L, 6, 22050, 2, 8},  /* 22k 8b Mu-Law  */
    { DATATYPE_ALAW,      ALAW_8B44KS,      60*1024L, 6, 44100, 2, 8},  /* 44k 8b Mu-Law  */
    { DATATYPE_NULL,      SUBTYPE_NONE,     60*1024L, 6, 0,     2, 16}, /* Get hdwr       */
    { DATATYPE_SPV2,      SPV2_BPCM,        60*1024L, 6, 14700, 2, 16}, /* SPV/2 BCPCM    */
    { DATATYPE_SPV2,      SPV2_PCM,         60*1024L, 6, 14700, 2, 16}, /* SPV/2 16b PCM  */
    { DATATYPE_SPV2,      SPV2_NONE,        60*1024L, 6, 0,     2, 16}, /* SPV/2 NONE BRR30*/
    { DATATYPE_CDXA_AUDIO, CDXA_LEVELC,     39984L, 3, 18900, 2, 16},   /* CD ROM XA Lvl C BRR214*/
    { DATATYPE_CDXA_AUDIO, CDXA_LEVELB,     39984L, 3, 37800, 2,  16},  /* CD ROM XA Lvl B BRR214*/
    { DATATYPE_CDXA_AUDIO, CDXA_LEVELC_MONO,39984L, 3, 18900, 1, 16},   /* CD ROM XA Lvl C BRR214*/
    { DATATYPE_CDXA_AUDIO, CDXA_LEVELB_MONO,39984L, 3, 37800, 1,  16}   /* CD ROM XA Lvl B BRR214*/
  };


//*******************************************************
// IOCTL jump table                                     *
// Request packet function jump table                   *
//*******************************************************
ULONG   (*IOCTLFuncs[])(PREQPACKET rp) = {
                IOCTL_Init,                    // 0
                IOCTL_Invalid,                 // 1
                IOCTL_Invalid,                 // 2
                IOCTL_Input,                   // 3
                IOCTL_Read,                    // 4
                IOCTL_NondestructiveRead,      // 5
                IOCTL_ReadStatus,              // 6
                IOCTL_FlushInput,              // 7
                IOCTL_Write,                   // 8
                IOCTL_Invalid,                 // 9
                IOCTL_WriteStatus,             // 10
                IOCTL_FlushOutput,             // 11
                IOCTL_Output,                  // 12
                IOCTL_Open,                    // 13
                IOCTL_Close,                   // 14
                IOCTL_Invalid,                 // 15
                IOCTL_GenIoctl                 // 16
                };
USHORT  MaxIOCTLFuncs = sizeof(IOCTLFuncs)/sizeof(USHORT);

//*******************************************************
// AUDIO GENERIC IOCTL jump table                       *
//*******************************************************
ULONG   (*AudioIOCTLFuncs[])(PVOID pParm) = {  // Generic IOCTL function table
                Audio_IOCTL_Init,                     // 0
                Audio_IOCTL_Status,                   // 1
                Audio_IOCTL_Control,                  // 2
                Audio_IOCTL_Buffer,                   // 3
                Audio_IOCTL_Load,                     // 4
                Audio_IOCTL_Wait,                     // 5
                Audio_IOCTL_Hpi                       // 6
                };
USHORT  MaxAudioIOCTLFuncs = sizeof(AudioIOCTLFuncs)/sizeof(USHORT);

USHORT trk;                     // Tracks and number of supported tracks
                                // are device dependent parameters.
ULONG trk_array[2];             // 2 track device

MCI_AUDIO_IOBUFFER xmitio;
MCI_AUDIO_IOBUFFER recio;

USHORT  EndOfData;
