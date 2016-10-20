;***************************************************************************  
;                                                                             
;  Module Name: OS2MEDEF.H                                                    
;                                                                             
;  OS/2 2.0 Multimedia Extensions Common Definitions file                     
;                                                                             
;  Copyright (c) International Business Machines Corporation 1991, 1992       
;                         All Rights Reserved                                 
;                                                                             
;***************************************************************************  
ifndef  _OS2MEDEF_H
_OS2MEDEF_H	EQU	1

ifdef XXXXX
endif
;***********************  
;  Sampling Rate  
;***********************  
HZ_8000	EQU	8000             ;  8.0 kHz  
HZ_11025	EQU	11025            ;  11.025 kHz  
HZ_14700	EQU	14700            ;  14.700 kHz (SPV/2)  
HZ_18900	EQU	18900            ;  18.900 kHz (CD/XA LVL C)  
HZ_22050	EQU	22050            ;  22.050 kHz  
HZ_37800	EQU	37800            ;  37.800 kHz (CD/XA LVL B)  
HZ_44100	EQU	44100            ;  44.100 kHz  
;***********************  
;  Bits per Sample  
;***********************  
BPS_4	EQU	4                ;  4 bits/sample (ADPCM)  
BPS_8	EQU	8                ;  8 bits/sample (PCM)  
BPS_16	EQU	16               ;  16 bits/sample(PCM)  
;***********************  
;  Channels  
;***********************  
CH_1	EQU	1                ;  Mono  
CH_2	EQU	2                ;  Stereo  
CH_4	EQU	4                ;  Quad  
;*************************************************************************
;* Multimedia Datatypes (spcbkey values)
;************************************************************************  
;****************
;* NULL datatype
;***************  
DATATYPE_NULL	EQU	0000H      ;  NULL stream data type (NULL)  
SUBTYPE_NONE	EQU	0000H    ;  No subtype for this data type  
;*********************
;* WAVEFORM datatypes
;********************  
DATATYPE_WAVEFORM	EQU	0001H      ;  WAVEFORM audio (PCM)  
;  Subtypes for DATATYPE_WAVEFORM:  
WAVE_FORMAT_1M08	EQU	0001H    ;  11.025kHz, Mono  , 8-bit  
WAVE_FORMAT_1S08	EQU	0002H    ;  11.025kHz, Stereo, 8-bit  
WAVE_FORMAT_1M16	EQU	0003H    ;  11.025kHz, Mono  , 16-bit  
WAVE_FORMAT_1S16	EQU	0004H    ;  11.025kHz, Stereo, 16-bit  
WAVE_FORMAT_2M08	EQU	0005H    ;  22.05kHz , Mono  , 8-bit  
WAVE_FORMAT_2S08	EQU	0006H    ;  22.05kHz , Stereo, 8-bit  
WAVE_FORMAT_2M16	EQU	0007H    ;  22.05kHz , Mono  , 16-bit  
WAVE_FORMAT_2S16	EQU	0008H    ;  22.05kHz , Stereo, 16-bit  
WAVE_FORMAT_4M08	EQU	0009H    ;  44.1kHz  , Mono  , 8-bit  
WAVE_FORMAT_4S08	EQU	000aH    ;  44.1kHz  , Stereo, 8-bit  
WAVE_FORMAT_4M16	EQU	000bH    ;  44.1kHz  , Mono  , 16-bit  
WAVE_FORMAT_4S16	EQU	000cH    ;  44.1kHz  , Stereo, 16-bit  
;******************
;* MuLaw datatypes
;*****************  
DATATYPE_MULAW	EQU	0101H      ;  MuLaw  
;  Subtypes for DATATYPE_MULAW:  
MULAW_8B8KS	EQU	0001H    ;  8bit 8kHz stereo  
MULAW_8B11KS	EQU	0002H    ;  8bit 11kHz stereo  
MULAW_8B22KS	EQU	0003H    ;  8bit 22kHz stereo  
MULAW_8B44KS	EQU	0004H    ;  8bit 44kHz stereo  
;*****************
;* ALaw datatypes
;****************  
DATATYPE_ALAW	EQU	0102H      ;  ALaw  
;  Subtypes for DATATYPE_ALAW:  
ALAW_8B8KS	EQU	0001H    ;  8bit 8kHz stereo  
ALAW_8B11KS	EQU	0002H    ;  8bit 11kHz stereo  
ALAW_8B22KS	EQU	0003H    ;  8bit 22kHz stereo  
ALAW_8B44KS	EQU	0004H    ;  8bit 44kHz stereo  
;*******************************
;* AVC ADPCM WAVEFORM datatypes
;******************************  
DATATYPE_ADPCM_AVC	EQU	0103H      ;  ADPCM audio  
;  Subtypes for DATATYPE_ADPCM_AVC:  
ADPCM_AVC_VOICE	EQU	0001H 
ADPCM_AVC_MUSIC	EQU	0002H 
ADPCM_AVC_STEREO	EQU	0003H 
ADPCM_AVC_HQ	EQU	0004H 
;****************
;* MIDI datatype
;***************  
DATATYPE_MIDI	EQU	0201H      ;  MIDI  
;          SUBTYPE_NONE            0x0000L      No subtype for this data type  
;********************
;* GENERIC datatypes
;*******************  
DATATYPE_GENERIC	EQU	0202H      ;  Generic for files / memory. This  
                                             ;  datatype will match any other  
                                             ;  datatype at stream creation time.  
;**************************
;* Speech Viewer datatypes
;*************************  
DATATYPE_SPV2	EQU	0203H      ;  Speech Viewer  
;  Subtypes for DATATYPE_SPV2:  
SPV2_BPCM	EQU	0001H 
SPV2_PCM	EQU	0002H 
SPV2_NONE	EQU	0003H 
;******************
;* CD-XA datatypes
;*****************  
DATATYPE_CDXA_VIDEO	EQU	0204H      ;  CD-XA Video datatype  
DATATYPE_CDXA_DATA	EQU	0205H      ;  CD-XA sub-band data datatype  
DATATYPE_CDXA_AUDIO	EQU	0206H      ;  CD-XA ADPCM Audio datatype  
DATATYPE_CDXA_AUDIO_HD	EQU	0207H      ;  CD-XA ADPCM Audio with Headers  
;  Subtypes for DATATYPE_CDXA_AUDIO:  
CDXA_LEVELB	EQU	0000H  ;  LEVEL B Audio Stereo  
CDXA_LEVELC	EQU	0001H  ;  LEVEL C Audio Stereo  
CDXA_LEVELB_MONO	EQU	0002H  ;  LEVEL B Audio Mono  
CDXA_LEVELC_MONO	EQU	0003H  ;  LEVEL C Audio Mono  
endif
