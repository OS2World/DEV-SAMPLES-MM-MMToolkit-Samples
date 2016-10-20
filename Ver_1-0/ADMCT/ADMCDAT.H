/***************************************************************************\
*
* Module Name: ADMCDAT.h
*
* OS/2 2.0 Multimedia Extensions Audio MCI Driver MMPM DataFormat Structure
* defintions.
*
* Copyright (c) International Business Machines Corporation 1990,1991
*                         All Rights Reserved
*
\***************************************************************************/

#define DATATYPE        (DataFormat[ulpInstance->usModeIndex])
    /*****************************************
    * Waveform Table DataStructure
    *****************************************/

typedef struct _mode_data_typ{
   ULONG      ulDataType;        /* PCM, ADPCM, etc..       */
   ULONG      ulDataSubType;     /* Freq, bits/sam,..       */
   LONG       srate;             /* 11,22,44 Khz            */
   LONG       bits_per_sample;   /* 8 or 16 bits/sam        */
   SHORT      channels;          /* Mono or Stereo          */
   ULONG      ulBytes;           /* Scaled for Time Calcs    */
   ULONG      ulMMTime;          /* Scaled for Time Calcs    */
   }MODE_DATA;

MODE_DATA DataFormat[] = {
// ALANSO-- no need for this data type
{DATATYPE_MIDI,      SUBTYPE_NONE, 8, 1,              0,      },
// alanso

{DATATYPE_ADPCM_AVC, ADPCM_AVC_VOICE,  11025, 16, 1,  576, 300,  },
{DATATYPE_ADPCM_AVC, ADPCM_AVC_MUSIC,  22050, 16, 1,  1128, 300,  },
{DATATYPE_ADPCM_AVC, ADPCM_AVC_STEREO, 22050, 16, 2,  2256, 300,  },
{DATATYPE_ADPCM_AVC, ADPCM_AVC_HQ,     44100, 16, 1,  1128, 150,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_1M08, 11025, 8,  1,  147, 40,   },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_2M08, 22050, 8,  1,  147, 20,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_4M08, 44100, 8,  1,  147, 10,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_1M16, 11025, 16, 1,  294, 40,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_2M16, 22050, 16, 1,  294, 20,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_4M16, 44100, 16, 1,  294, 10,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_1S08, 11025, 8,  2,  294, 40,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_2S08, 22050, 8,  2,  294, 20,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_4S08, 44100, 8,  2,  294, 10,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_1S16, 11025, 16, 2,  588, 40,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_2S16, 22050, 16, 2,  588, 20,  },
{DATATYPE_WAVEFORM,  WAVE_FORMAT_4S16, 44100, 16, 2,  588, 10, },
{DATATYPE_NULL,      SUBTYPE_NONE,      0,    0,  0,     0, },
{DATATYPE_MULAW,     MULAW_8B8KS,      8000,  8,  2,     0, },
{DATATYPE_MULAW,     MULAW_8B11KS,     11025, 8,  2,     0, },
{DATATYPE_MULAW,     MULAW_8B22KS,     22050, 8,  2,     0, },
{DATATYPE_MULAW,     MULAW_8B44KS,     44100, 8,  2,     0, },
{DATATYPE_ALAW,      ALAW_8B8KS,       8000,  8,  2,     0, },
{DATATYPE_ALAW,      ALAW_8B11KS,      11025, 8,  2,     0, },
{DATATYPE_ALAW,      ALAW_8B22KS,      22050, 8,  2,     0, },
{DATATYPE_ALAW,      ALAW_8B44KS,      44100, 8,  2,     0, },
{DATATYPE_NULL,      SUBTYPE_NONE,     0,     0,  0,     0, },
{DATATYPE_SPV2,      SPV2_BPCM,        14700, 16, 1,     0, },
{DATATYPE_SPV2,      SPV2_PCM,         14700, 16, 1,     0, },
{DATATYPE_SPV2,      SPV2_NONE,        0,     1,         0, },
};
