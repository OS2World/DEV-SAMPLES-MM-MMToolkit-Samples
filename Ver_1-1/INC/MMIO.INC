;****************************************************************************
; Module Name: MMIO.H
;
; OS/2 2.0 Multimedia Extensions Input/Output Manager Definitions file
;  NOTE:  All structures are UNPACKED!!
;
; Copyright (c) International Business Machines Corporation 1990,1991,1992
;                         All Rights Reserved
;
;
;***************************************************************************  
ifndef _MMIO_H_
_MMIO_H_	EQU	1

; 
;* MMIO symbols.
;  
ifndef _OS2MEDEF_H
endif
; 
;* MS compat defines.
;  
ifndef _OS2MEDEF_H
LPSTR	EQU	PSZ
endif
HPSTR	EQU	PSZ
; 
;* Define MMIO public data structures.
;  

MMCKINFO	STRUC
mmckinfo_ckid	DD	?             ;  Chunk id (FOURCC)                      
mmckinfo_ckSize	DD	?           ;  Chunk Size (bytes)                     
mmckinfo_fccType	DD	?          ;  FourCC Type (if ckid RIFF or LIST)     
mmckinfo_dwDataOffset	DD	?     ;  File Offset of data portion of chunk   
mmckinfo_dwFlags	DD	?          ;  MMIO_DIRTY (if new chunk)              
MMCKINFO	ENDS
LPMMCKINFO	EQU	PMMCKINFO     ;  MS compat define   
LPMMIOPROC	EQU	PMMIOPROC     ;  MS compat define   

MMIOINFO	STRUC
mmioinfo_dwFlags	DD	?          ;  Open flags                    
mmioinfo_fccIOProc	DD	?        ;  FOURCC of the IOProc to use   
mmioinfo_pIOProc	DD	?          ;  Function Pointer to IOProc to use   
mmioinfo_dwErrorRet	DD	?       ;  Extended Error return code    
mmioinfo_cchBuffer	DD	?        ;  I/O buff size (if used), Fsize if MEM   
mmioinfo_pchBuffer	DD	?        ;  Start of I/O buff             
mmioinfo_pchNext	DD	?          ;  Next char to read or write in buff   
mmioinfo_pchEndRead	DD	?       ;  Last char in buff can be read + 1    
mmioinfo_pchEndWrite	DD	?      ;  Last char in buff can be written + 1   
mmioinfo_lBufOffset	DD	?       ;  Offset in buff to pchNext   
mmioinfo_lDiskOffset	DD	?      ;  Disk offset in file         
mmioinfo_adwInfo	DD	4 DUP (?)       ;  IOProc specific fields      
mmioinfo_lLogicalFilePos	DD	?  ;  Actual file position, buffered or not   
mmioinfo_dwTranslate	DD	?      ;  Translation field           
mmioinfo_fccChildIOProc	DD	?   ;  FOURCC of Child IOProc      
mmioinfo_pExtraInfoStruct	DD	? ;  Pointer to a structure of related data   
mmioinfo_hmmio	DD	?            ;  Handle to media element     
MMIOINFO	ENDS
LPMMIOINFO	EQU	PMMIOINFO     ;  MS compat define   

MMCFINFO	STRUC
mmcfinfo_dwHeaderSize	DD	?        ;  CTOC header size            
mmcfinfo_dwEntriesTotal	DD	?      ;  Num of CTOC table entries   
mmcfinfo_dwEntriesDeleted	DD	?    ;  Num of CTOC table entries deleted   
mmcfinfo_dwEntriesUnused	DD	?     ;  Num of unused CTOC entries   
mmcfinfo_dwBytesTotal	DD	?        ;  Combined byte size of all CGRP elements   
mmcfinfo_dwBytesDeleted	DD	?      ;  Byte size of all deleted CGRP elements   
mmcfinfo_dwHeaderFlags	DD	?       ;  Info about entire compound file (CF)    
mmcfinfo_wEntrySize	DW	?          ;  Size of each CTOC table entry   
mmcfinfo_wNameSize	DW	?           ;  Size of name field in entry, default 13   
mmcfinfo_wExHdrFields	DW	?        ;  Num CTOC header extra fields   
mmcfinfo_wExEntFields	DW	?        ;  Num CTOC entry extra fields    
MMCFINFO	ENDS
; 
;* Note:  The are variable length DWORD arrays that may be present
;*        at the end of the MMCFINFO structure.
;  
;    DWORD    (*adwExHdrFldUsage)[];  /* Array of header extra usage fields   
;    DWORD    (*adwExtEntFldUsage)[]; /* Array of entry extra usage fields    
;    DWORD    (*adwExHdrField)[];     /* Array of header extra fields         
LPMMCFINFO	EQU	PMMCFINFO     ;  MS compat define   

MMCTOCENTRY	STRUC
mmctocentry_dwOffset	DD	?            ;  Offset of element within CGRP   
mmctocentry_dwSize	DD	?              ;  Size of element                 
mmctocentry_dwMedType	DD	?           ;  Fourcc of element               
mmctocentry_dwMedUsage	DD	?          ;  Possible sub type               
mmctocentry_dwCompressTech	DD	?      ;  Compression technique used      
mmctocentry_dwUncompressBytes	DD	?   ;  Actual size of uncompressed element   
MMCTOCENTRY	ENDS
; 
;* Note:  A variable length name field and possibly a DWORD array may
;*        be present at the end of the MMCTOCENTRY structure.
;  
;    DWORD    (*adwExEntField)[];  /* Array of entry extra fields   
;    PSZ      pszElementName[];    /* Name of element, variable length   
LPMMCTOCENTRY	EQU	PMMCTOCENTRY ;  MS compat define   
; 
;* Conversion Flags/Data Structures:
;  
MMIO_MEDIATYPE_IMAGE	EQU	00000001H   ;  Image media   
MMIO_MEDIATYPE_AUDIO	EQU	00000002H   ;  Audio media   
MMIO_MEDIATYPE_MIDI	EQU	00000004H   ;  MIDI media    
MMIO_MEDIATYPE_COMPOUND	EQU	00000008H   ;  Cmpd media    
MMIO_MEDIATYPE_OTHER	EQU	00000010H   ;  Other media   
MMIO_MEDIATYPE_UNKNOWN	EQU	00000020H   ;  Unknown media   
MMIO_MEDIATYPE_DIGITALVIDEO	EQU	00000040H   ;  Digital Video   
MMIO_MEDIATYPE_ANIMATION	EQU	00000080H   ;  Future: Not Supported   
MMIO_MEDIATYPE_MOVIE	EQU	00000100H   ;  Movie File   
MMIO_CANREADTRANSLATED	EQU	00000001H  ; IOProc Cpbilty Flgs  
MMIO_CANWRITETRANSLATED	EQU	00000002H  ;  "       "       "   
MMIO_CANREADWRITETRANSLATED	EQU	00000004H  ;  "       "       "   
MMIO_CANREADUNTRANSLATED	EQU	00000008H  ;  "       "       "   
MMIO_CANWRITEUNTRANSLATED	EQU	00000010H  ;  "       "       "   
MMIO_CANREADWRITEUNTRANSLATED	EQU	00000020H  ;  "       "       "   
MMIO_CANSEEKTRANSLATED	EQU	00000040H  ;  "       "       "   
MMIO_CANSEEKUNTRANSLATED	EQU	00000080H  ;  "       "       "   
MMIO_CANINSERTUNTRANSLATED	EQU	00000100H  ;  "       "       "   
MMIO_CANSAVEUNTRANSLATED	EQU	00000200H  ;  "       "       "   
MMIO_CANINSERTTRANSLATED	EQU	00000400H  ;  "       "       "   
MMIO_CANSAVETRANSLATED	EQU	00000800H  ;  "       "       "   
MMIO_CANMULTITRACKREADTRANSLATED	EQU	00001000H  ;  "       "       "   
MMIO_CANMULTITRACKREADUNTRANSLATED	EQU	00002000H  ;  "       "       "   
MMIO_CANMULTITRACKWRITETRANSLATED	EQU	00004000H  ;  "       "       "   
MMIO_CANMULTITRACKWRITEUNTRANSLATED	EQU	00008000H  ;  "       "       "   
MMIO_CANTRACKSEEKTRANSLATED	EQU	00010000H  ;  "       "       "   
MMIO_CANTRACKSEEKUNTRANSLATED	EQU	00020000H  ;  "       "       "   
MMIO_CANTRACKREADTRANSLATED	EQU	00040000H  ;  "       "       "   
MMIO_CANTRACKREADUNTRANSLATED	EQU	00080000H  ;  "       "       "   
MMIO_CANTRACKWRITETRANSLATED	EQU	00100000H  ;  "       "       "   
MMIO_CANTRACKWRITEUNTRANSLATED	EQU	00200000H  ;  "       "       "   
MMIO_IOPROC_STORAGESYSTEM	EQU	00000001H   ;  IOProc Types   
MMIO_IOPROC_FILEFORMAT	EQU	00000002H 
MMIO_IOPROC_DATAFORMAT	EQU	00000004H 

MMFORMATINFO	STRUC
mmformatinfo_dwStructLen	DD	?               ;  Length of this structure   
mmformatinfo_fccIOProc	DD	?                 ;  IOProc identifier          
mmformatinfo_dwIOProcType	DD	?              ;  Type of IOProc             
mmformatinfo_dwMediaType	DD	?               ;  Media Type                 
mmformatinfo_dwFlags	DD	?                   ;  IOProc capability flags    
mmformatinfo_achDefaultFormatExt	DB	SIZE   FOURCC  + 1 DUP (?)
                                       ;  Default extension 4 + null   
mmformatinfo_dwCodePage	DD	?                ;  Code Page                    
mmformatinfo_dwLanguage	DD	?                ;  Language                     
mmformatinfo_lNameLength	DD	?               ;  length of identifier string   
MMFORMATINFO	ENDS
MMIO_IMAGE_UNKNOWN	EQU	00000000H   ;  Unknown image content   
MMIO_IMAGE_DRAWING	EQU	00000001H   ;  Simple drawing          
MMIO_IMAGE_GRAPH	EQU	00000002H   ;  Graphs & Cartoons       
MMIO_IMAGE_PHOTO	EQU	00000004H   ;  Varying Color & Shades   
ifdef INCL_GPI
ifdef INCL_GPIBITMAPS
MAX_PALETTE	EQU	256 

XDIBHDR_PREFIX	STRUC
XDIB_ulMemSize	DD	?                          ;  Length of bitmap     
XDIB_ulPelFormat	DD	?
XDIB_usTransType	DW	?
XDIB_ulTransVal	DD	?
XDIBHDR_PREFIX	ENDS

MMXDIBHEADER	STRUC
XDIB_XDIBHeaderPrefix	DB	SIZE XDIBHDR_PREFIX DUP (?)
XDIB_BMPInfoHeader2	DB	SIZE BITMAPINFOHEADER2 DUP (?)
MMXDIBHEADER	ENDS

MMIMAGEHEADER	STRUC
mmimageheader_dwHeaderLength	DD	?         ;  Length in Bytes   
mmimageheader_dwContentType	DD	?          ;  Image content     
mmimageheader_dwMediaType	DD	?            ;  Media Type        
mmimageheader_mmXDIBHeader	DB	SIZE MMXDIBHEADER DUP (?)           ;  OS/2 2.0 PM compat header   
mmimageheader_bmiColors	DB	SIZE RGB2 * MAX_PALETTE DUP (?) ;  PM compatible palette   
MMIMAGEHEADER	ENDS
endif
endif
MMIO_AUDIO_UNKNOWN	EQU	00000000H   ;  Unknown image content    
MMIO_AUDIO_VOICE	EQU	00000001H   ;  Limited Range            
MMIO_AUDIO_MUSIC	EQU	00000002H   ;  FM Radio or equivalent   
MMIO_AUDIO_HIFI	EQU	00000004H   ;  High quality recording   

WAVE_HEADER	STRUC
waveheader_usFormatTag	DW	?              ;  Type of wave format   
waveheader_usChannels	DW	?               ;  Number of channels    
waveheader_ulSamplesPerSec	DD	?          ;  Sampling rate         
waveheader_ulAvgBytesPerSec	DD	?         ;  Avg bytes per sec     
waveheader_usBlockAlign	DW	?             ;  Block Alignment in bytes   
waveheader_usBitsPerSample	DW	?          ;  Bits per sample       
WAVE_HEADER	ENDS

XWAV_HEADERINFO	STRUC
xwaveheader_ulAudioLengthInMS	DD	?        ;  Audio data in millisecs   
xwaveheader_ulAudioLengthInBytes	DD	?     ;  Audio data in bytes       
xwaveheader_ulReserved	DD	?               ;  reserved                  
XWAV_HEADERINFO	ENDS

MMXWAV_HEADER	STRUC
mmxwaveheader_WAVEHeader	DB	SIZE WAVE_HEADER DUP (?)               ;  Per RIFF WAVE Definition   
mmxwaveheader_XWAVHeaderInfo	DB	SIZE XWAV_HEADERINFO DUP (?)           ;  Extended wave definition   
MMXWAV_HEADER	ENDS

MMAUDIOHEADER	STRUC
mmaudioheader_dwHeaderLength	DD	?           ;  Length in Bytes   
mmaudioheader_dwContentType	DD	?            ;  Image content     
mmaudioheader_dwMediaType	DD	?              ;  Media Type        
mmaudioheader_mmXWAVHeader	DB	SIZE MMXWAV_HEADER DUP (?)             ;  header            
MMAUDIOHEADER	ENDS
MMIO_MIDI_UNKNOWN	EQU	00000000H   ;  Unknown midi content     
MMIO_MIDI_VOICE	EQU	00000001H   ;  Limited Range            
MMIO_MIDI_MUSIC	EQU	00000002H   ;  FM Radio or equivalent   
MMIO_MIDI_HIFI	EQU	00000004H   ;  High quality recording   
; 
;* MMPMMMIO.INI file structure and definitions.
;  
DLLNAME_SIZE	EQU	CCHMAXPATH
PROCNAME_SIZE	EQU	32 
MAX_EXTENSION_NAME	EQU	4 

MMINIFILEINFO	STRUC
mminifileinfo_fccIOProc	DD	?                     ;  IOProc identifier        
mminifileinfo_szDLLName	DB	DLLNAME_SIZE DUP (?)       ;  DLL name string          
mminifileinfo_szProcName	DB	PROCNAME_SIZE DUP (?)     ;  Procedure name string    
mminifileinfo_dwFlags	DD	?                       ;  Flags for Preload        
mminifileinfo_dwExtendLen	DD	?                   ;  Length of ext fields     
mminifileinfo_dwMediaType	DD	?                   ;  Media type               
mminifileinfo_dwIOProcType	DD	?                  ;  Type of IOProc           
mminifileinfo_szDefExt	DB	SIZE   MAX_EXTENSION_NAME  DUP (?)
                                              ;  Default file extension   
MMINIFILEINFO	ENDS
; 
;* CODEC Structures and type definitions for Rel. 1.1
;  
CODEC_INFO_SIZE	EQU	8 
CODEC_HW_NAME_SIZE	EQU	32 
ifdef INCL_MMIO_CODEC

CODECINIFILEINFO	STRUC
codecinifileinfo_ulStructLen	DD	?                  ;  length of this structure      
codecinifileinfo_fcc	DD	?                          ;  File Format ID                
codecinifileinfo_szDLLName	DB	DLLNAME_SIZE DUP (?)      ;  DLL name string               
codecinifileinfo_szProcName	DB	PROCNAME_SIZE DUP (?)    ;  Procedure name string         
codecinifileinfo_ulCompressType	DD	?               ;  Compression Type              
codecinifileinfo_ulCompressSubType	DD	?            ;  Compression SubType           
codecinifileinfo_ulMediaType	DD	?                  ;  Media type                    
codecinifileinfo_ulCapsFlags	DD	?                  ;  capabilities flags            
codecinifileinfo_ulFlags	DD	?                      ;  flags                         
codecinifileinfo_szHWID	DB	CODEC_HW_NAME_SIZE DUP (?)   ;  specific information          
codecinifileinfo_ulMaxSrcBufLen	DD	?               ;  max source buffer length      
codecinifileinfo_ulSyncMethod	DD	?                 ;  Synchronization method        
codecinifileinfo_ulReserved1	DD	?                  ;  Reserved1                     
codecinifileinfo_ulXalignment	DD	?                 ;  x alignment - video only      
codecinifileinfo_ulYalignment	DD	?                 ;  y alignment - video only      
codecinifileinfo_ulSpecInfo	DD	CODEC_INFO_SIZE DUP (?)  ;  specific information          
CODECINIFILEINFO	ENDS
; 
;* CODECINIFILEINFO synchronization method (ulSyncMethod) values.
;  
CODEC_SYNC_METHOD_NO_DROP_FRAMES	EQU	0 
CODEC_SYNC_METHOD_DROP_FRAMES_IMMEDIATELY	EQU	1 
CODEC_SYNC_METHOD_DROP_FRAMES_PRECEDING_KEY	EQU	2 
; 
;* CODECINIFILEINFO capabilities (ulCapsFlags) values.
;  
CODEC_COMPRESS	EQU	00000001H 
CODEC_DECOMPRESS	EQU	00000002H 
CODEC_WINDOW_CLIPPING	EQU	00000004H 
CODEC_PALETTE_TRANS	EQU	00000008H 
CODEC_SELFHEAL	EQU	00000010H 
CODEC_SCALE_PEL_DOUBLE	EQU	00000020H 
CODEC_SCALE_PEL_HALVED	EQU	00000040H 
CODEC_SCALE_CONTINUOUS	EQU	00000080H 
CODEC_MULAPERTURE	EQU	00000100H 
CODEC_4_BIT_COLOR	EQU	00000200H 
CODEC_8_BIT_COLOR	EQU	00000400H 
CODEC_16_BIT_COLOR	EQU	00000800H 
CODEC_24_BIT_COLOR	EQU	00001000H 
CODEC_HARDWARE	EQU	00002000H 
CODEC_SYMMETRIC	EQU	00004000H 
CODEC_ASYMMETRIC	EQU	00008000H 
CODEC_DIRECT_DISPLAY	EQU	00010000H 
CODEC_DEFAULT	EQU	00020000H 
CODEC_ORIGIN_LOWERLEFT	EQU	00040000H 
CODEC_ORIGIN_UPPERLEFT	EQU	00080000H 
CODEC_SET_QUALITY	EQU	00100000H   ;  quality level is settable   
CODEC_DATA_CONSTRAINT	EQU	00200000H   ;  data constraint supported   
; 
;* CODECINIFILEINFO Flag Values (ulFlags) values.
;  
;  Bit definitions for mmioSet()   
MMIO_SET_EXTENDEDINFO	EQU	0001H 
MMIO_QUERY_EXTENDEDINFO_BASE	EQU	0002H 
MMIO_QUERY_EXTENDEDINFO_ALL	EQU	0004H 
;  CODECASSOC structure   

CODECASSOC	STRUC
codec_pCodecOpen	DD	?                    ;  codec specific open header   
codec_pCODECIniFileInfo	DD	?  ;  codecinifileinfo             
CODECASSOC	ENDS
;  PCODECASSOC definition   
;  MMEXTENDINFO structure   

MMEXTENDINFO	STRUC
extended_ulStructLen	DD	?              ;  length of this structure    
extended_ulBufSize	DD	?                ;  total buffer size   
extended_ulFlags	DD	?                  ;  flags   
extended_ulTrackID	DD	?                ;  track ID   
extended_ulNumCODECs	DD	?              ;  number of codec entries   
extended_pCODECAssoc	DD	?              ;  pointer to codec info array   
MMEXTENDINFO	ENDS
;  PMMEXTENDINFO definition   
; 
;* MMEXTENDINFO operation (ulFlags) values.
;  
MMIO_TRACK	EQU	00000001H 
MMIO_NORMAL_READ	EQU	00000002H 
MMIO_SCAN_READ	EQU	00000004H 
MMIO_REVERSE_READ	EQU	00000008H 
MMIO_CODEC_ASSOC	EQU	00000100H 
MMIO_RESETTRACKS	EQU	-1        ;   Turns off the active track number.   
CODEC_START	EQU	0EC0H 
CODEC_END	EQU	0EFFH 
MMIOM_CODEC_CLOSE	EQU	CODEC_START + 1 
MMIOM_CODEC_OPEN	EQU	CODEC_START + 2 
MMIOM_CODEC_QUERYNAME	EQU	CODEC_START + 3 
MMIOM_CODEC_QUERYNAMELENGTH	EQU	CODEC_START + 4 
MMIOM_CODEC_COMPRESS	EQU	CODEC_START + 5 
MMIOM_CODEC_DECOMPRESS	EQU	CODEC_START + 6 
endif
;  End CODEC include   
; 
;* Flags:
;  
MMIO_CREATE	EQU	00000001H        ;  Open   
MMIO_CTOCFIRST	EQU	00000002H        ;  Open   
MMIO_READ	EQU	00000004H        ;  Open   
MMIO_WRITE	EQU	00000008H        ;  Open   
MMIO_READWRITE	EQU	00000010H        ;  Open   
MMIO_COMPAT	EQU	00000020H        ;  Open   
MMIO_EXCLUSIVE	EQU	00000040H        ;  Open   
MMIO_DENYWRITE	EQU	00000080H        ;  Open   
MMIO_DENYREAD	EQU	00000100H        ;  Open   
MMIO_DENYNONE	EQU	00000200H        ;  Open   
MMIO_ALLOCBUF	EQU	00000400H        ;  Open   
MMIO_DELETE	EQU	00000800H        ;  Open   
MMIO_USE_TEMP	EQU	00001000H        ;  Open/Close/Save   
MMIO_INSERTON	EQU	00000001H        ;  Insert   
MMIO_INSERTOFF	EQU	00000002H        ;  Insert   
MMIO_RWMODE	EQU	00001000H        ;  Open   
MMIO_SHAREMODE	EQU	00002000H        ;  Open   
MMIO_DIRTY	EQU	00004000H        ;  Write   
MMIO_VERTBAR	EQU	00008000H        ;  Open   
MMIO_BUFSHARED	EQU	00010000H        ;  Open   
MMIO_APPEND	EQU	00020000H        ;  Open   
MMIO_NOIDENTIFY	EQU	00040000H        ;  Open   
MMIO_FINDFIRST	EQU	00000001H        ;  CF Find Entry   
MMIO_FINDNEXT	EQU	00000002H        ;  CF Find Entry   
MMIO_FINDUNUSED	EQU	00000004H        ;  CF Find Entry   
MMIO_FINDDELETED	EQU	00000008H        ;  CF Find Entry   
MMIO_CHANGEDELETED	EQU	0001H          ;  CF Change Entry   
MMIO_CF_FQNAME	EQU	0001H          ;  CF Compact        
MMIO_FHOPEN	EQU	0001H            ;  Close   
MMIO_EMPTYBUF	EQU	0001H            ;  Flush   
MMIO_CREATERIFF	EQU	0001H            ;  CreateChunk   
MMIO_CREATELIST	EQU	0002H            ;  CreateChunk   
MMIO_FINDCHUNK	EQU	0004H            ;  Descend       
MMIO_FINDRIFF	EQU	0008H            ;  Descend       
MMIO_FINDLIST	EQU	0010H            ;  Descend       
CTOC_HF_SEQUENTIAL	EQU	00000001H     ;  CTOC dwHeaderFlags   
CTOC_HF_MEDSUBTYPE	EQU	00000002H     ;  CTOC dwHeaderFlags   
CTOC_EFU_UNUSED	EQU	00000000H  ;  CTOC extra usage code   
CTOC_EFU_LASTMODTIME	EQU	00000001H  ;  CTOC extra usage code   
CTOC_EFU_CODEPAGE	EQU	00000002H  ;  CTOC extra usage code   
CTOC_EFU_LANGUAGE	EQU	00000003H  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM0	EQU	00000005H  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM1	EQU	00000006H  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM2	EQU	00000007H  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM3	EQU	00000008H  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM4	EQU	00000009H  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM5	EQU	0000000AH  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM6	EQU	0000000BH  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM7	EQU	0000000CH  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM8	EQU	0000000DH  ;  CTOC extra usage code   
CTOC_EFU_COMPRESSPARAM9	EQU	0000000EH  ;  CTOC extra usage code   
CTOC_CHARSET_STANDARD	EQU	00000000H  ;  CTOC charset value      
MMIO_INSTALLPROC	EQU	00000001H  ;  Install IO Proc   
MMIO_REMOVEPROC	EQU	00000002H  ;  Install IO Proc   
MMIO_FINDPROC	EQU	00000004H  ;  Install IO Proc   
MMIO_MATCHFIRST	EQU	00000010H  ;  Ini File Handler   
MMIO_MATCHNEXT	EQU	00000020H  ;  Ini File Handler   
MMIO_MATCHFOURCC	EQU	00000040H  ;  Ini File Handler   
MMIO_MATCHDLL	EQU	00000080H  ;  Ini File Handler   
MMIO_MATCHPROCEDURENAME	EQU	00000100H  ;  Ini File Handler   
MMIO_FULLPATH	EQU	00000200H  ;  Ini File Handler   
MMIO_NOVERIFY	EQU	00000400H  ;  Ini File Handler   
MMIO_MATCHCOMPRESSTYPE	EQU	00000800H  ;  Ini File Handler   
MMIO_EXTENDED_STRUCT	EQU	00001000H  ;  Ini File ulFlags   
MMIO_MATCHCOMPRESSSUBTYPE	EQU	00002000H  ;  Ini File Handler   
MMIO_MATCHHWID	EQU	00004000H  ;  Ini File Handler   
MMIO_MATCHCAPSFLAGS	EQU	00008000H  ;  Ini File Handler   
MMIO_SKIPMATCH	EQU	00010000H  ;  Ini/Load Handler   
MMIO_TOUPPER	EQU	0001H       ;  StringToFourcc   
MMIO_CF_ENTRY_EXISTS	EQU	00000001H  ;  Add CGRP element   
MMIO_FORCE_IDENTIFY_SS	EQU	00000001H  ;  Identify   
MMIO_FORCE_IDENTIFY_FF	EQU	00000002H  ;  Identify   
MMIO_NOTRANSLATE	EQU	00000000H  ;  Translation   
MMIO_TRANSLATEDATA	EQU	00000001H  ;  Translation   
MMIO_TRANSLATEHEADER	EQU	00000002H  ;  Translation   
MMIO_DECOMPRESS	EQU	00000004H  ;  CODEC Decompress   
MMIO_DEFAULTBUFFER	EQU	8192     ;  two pages under OS/2 2.0   
MMIO_SEEK_IFRAME	EQU	00010000H   ;  Seek to nearest previous IFRAME   
; 
;* Messages :
;  
MMIOM_START	EQU	0E00H 
MMIOM_END	EQU	0EFFH 
MMIOM_GETCF	EQU	MMIOM_START + 1 
MMIOM_GETCFENTRY	EQU	MMIOM_START + 2 
MMIOM_CLOSE	EQU	MMIOM_START + 3 
MMIOM_OPEN	EQU	MMIOM_START + 4 
MMIOM_READ	EQU	MMIOM_START + 5 
MMIOM_SEEK	EQU	MMIOM_START + 6 
MMIOM_WRITE	EQU	MMIOM_START + 7 
MMIOM_IDENTIFYFILE	EQU	MMIOM_START + 8 
MMIOM_GETHEADER	EQU	MMIOM_START + 9 
MMIOM_SETHEADER	EQU	MMIOM_START + 10 
MMIOM_QUERYHEADERLENGTH	EQU	MMIOM_START + 11 
MMIOM_GETFORMATNAME	EQU	MMIOM_START + 12 
MMIOM_GETFORMATINFO	EQU	MMIOM_START + 13 
MMIOM_SEEKBYTIME	EQU	MMIOM_START + 14 
MMIOM_TEMPCHANGE	EQU	MMIOM_START + 15 
MMIOM_BEGININSERT	EQU	MMIOM_START + 16 
MMIOM_ENDINSERT	EQU	MMIOM_START + 17 
MMIOM_SAVE	EQU	MMIOM_START + 18 
MMIOM_SET	EQU	MMIOM_START + 19 
MMIOM_COMPRESS	EQU	MMIOM_START + 20 
MMIOM_DECOMPRESS	EQU	MMIOM_START + 21 
MMIOM_MULTITRACKREAD	EQU	MMIOM_START + 22 
MMIOM_MULTITRACKWRITE	EQU	MMIOM_START + 23 
MMIOM_DELETE	EQU	MMIOM_START + 24 
MMIOM_BEGINGROUP	EQU	MMIOM_START + 25 
MMIOM_ENDGROUP	EQU	MMIOM_START + 26 
MMIOM_UNDO	EQU	MMIOM_START + 27 
MMIOM_REDO	EQU	MMIOM_START + 28 
MMIOM_BEGINSTREAM	EQU	MMIOM_START + 29 
MMIOM_ENDSTREAM	EQU	MMIOM_START + 30 
MMIOM_USER	EQU	0F00H 
MMIOM_USER_END	EQU	0FFFH 
; 
;* Include error codes for MMIO only.
;  
ifndef _MEERROR_H_
INCL_MMIO_ERRORS	EQU	1

INCLUDE meerror.INC
endif
MMIO_SUCCESS	EQU	0 
MMIO_WARNING	EQU	2 
MMIO_ERROR	EQU	-1 
MMIOERR_UNSUPPORTED_MESSAGE	EQU	-2 
MMIO_CF_SUCCESS	EQU	0 
MMIO_CF_FAILURE	EQU	1 
FOURCC_RIFF	EQU	mmioFOURCC( 'R', 'I', 'F', 'F' )
FOURCC_LIST	EQU	mmioFOURCC( 'L', 'I', 'S', 'T' )
FOURCC_MEM	EQU	mmioFOURCC( 'M', 'E', 'M', ' ' )
FOURCC_DOS	EQU	mmioFOURCC( 'D', 'O', 'S', ' ' )
FOURCC_BND	EQU	mmioFOURCC( 'B', 'N', 'D', ' ' )
FOURCC_FREE	EQU	mmioFOURCC( 'F', 'R', 'E', 'E' )
FOURCC_DEL	EQU	mmioFOURCC( 'D', 'E', 'L', ' ' )
FOURCC_CTOC	EQU	mmioFOURCC( 'C', 'T', 'O', 'C' )
FOURCC_CGRP	EQU	mmioFOURCC( 'C', 'G', 'R', 'P' )
FOURCC_CF	EQU	mmioFOURCC( 'C', 'F', ' ', ' ' )
MMIO_NLS_CHARSET_INFO	EQU	8000    ;  RCDATA Name ID for NLS            
MMIO_IOPROC_NAME_TABLE	EQU	8500    ;  RCDATA Name ID for string table   
MMIO_CODEC_NAME_TABLE	EQU	9000    ;  RCDATA Name ID for Codec  table   
; 
;* Numeric equivalents of fourcc's.  These are needed for the resource
;* compiler.
;  
HEX_FOURCC_DOS	EQU	20534f44H 
HEX_FOURCC_MEM	EQU	204d454dH 
HEX_FOURCC_BND	EQU	20444e42H 
HEX_FOURCC_CF	EQU	20204643H 
; 
;* Country codes (CC), languages (LC), and dialects (DC).
;  
MMIO_DEFAULT_CODE_PAGE	EQU	437 
MMIO_CC_NONE	EQU	00Q 
MMIO_CC_USA	EQU	01Q 
MMIO_CC_CANADA	EQU	02Q 
MMIO_CC_LATIN_AMERICA	EQU	03Q 
MMIO_CC_GREECE	EQU	30Q 
MMIO_CC_NETHERLANDS	EQU	31Q 
MMIO_CC_BELGIUM	EQU	32Q 
MMIO_CC_FRANCE	EQU	33Q 
MMIO_CC_SPAIN	EQU	34Q 
MMIO_CC_ITALY	EQU	3Q 9 
MMIO_CC_SWITZERLAND	EQU	41Q 
MMIO_CC_AUSTRIA	EQU	43Q 
MMIO_CC_UNITED_KINGDOM	EQU	44Q 
MMIO_CC_DENMARK	EQU	45Q 
MMIO_CC_SWEDEN	EQU	46Q 
MMIO_CC_NORWAY	EQU	47Q 
MMIO_CC_WEST_GERMANY	EQU	4Q 9 
MMIO_CC_MEXICO	EQU	52Q 
MMIO_CC_BRAZIL	EQU	55Q 
MMIO_CC_AUSTRALIA	EQU	61Q 
MMIO_CC_NEW_ZEALAND	EQU	64Q 
MMIO_CC_JAPAN	EQU	081 
MMIO_CC_KOREA	EQU	082 
MMIO_CC_CHINA	EQU	086 
MMIO_CC_TAIWAN	EQU	088 
MMIO_CC_TURKEY	EQU	090 
MMIO_CC_PORTUGAL	EQU	351 
MMIO_CC_LUXEMBOURG	EQU	352 
MMIO_CC_ICELAND	EQU	354 
MMIO_CC_FINLAND	EQU	358 
MMIO_LC_NONE	EQU	0 
MMIO_DC_NONE	EQU	0 
MMIO_LC_ARABIC	EQU	1 
MMIO_DC_ARABIC	EQU	1 
MMIO_LC_BULGARIAN	EQU	2 
MMIO_DC_BULGARIAN	EQU	1 
MMIO_LC_CATALAN	EQU	3 
MMIO_DC_CATALAN	EQU	1 
MMIO_LC_TRADITIONAL_CHINESE	EQU	4 
MMIO_DC_TRADITIONAL_CHINESE	EQU	1 
MMIO_LC_SIMPLE_CHINESE	EQU	4 
MMIO_DC_SIMPLE_CHINESE	EQU	2 
MMIO_LC_CZECH	EQU	5 
MMIO_DC_CZECH	EQU	1 
MMIO_LC_DANISH	EQU	6 
MMIO_DC_DANISH	EQU	1 
MMIO_LC_GERMAN	EQU	7 
MMIO_DC_GERMAN	EQU	1 
MMIO_LC_SWISS_GERMAN	EQU	7 
MMIO_DC_SWISS_GERMAN	EQU	2 
MMIO_LC_GREEK	EQU	8 
MMIO_DC_GREEK	EQU	1 
MMIO_LC_US_ENGLISH	EQU	9 
MMIO_DC_US_ENGLISH	EQU	1 
MMIO_LC_UK_ENGLISH	EQU	9 
MMIO_DC_UK_ENGLISH	EQU	2 
MMIO_LC_SPANISH	EQU	10 
MMIO_DC_SPANISH	EQU	1 
MMIO_LC_SPANISH_MEXICAN	EQU	10 
MMIO_DC_SPANISH_MEXICAN	EQU	2 
MMIO_LC_FINNISH	EQU	11 
MMIO_DC_FINNISH	EQU	1 
MMIO_LC_FRENCH	EQU	12 
MMIO_DC_FRENCH	EQU	1 
MMIO_LC_BELGIAN_FRENCH	EQU	12 
MMIO_DC_BELGIAN_FRENCH	EQU	2 
MMIO_LC_CANADIAN_FRENCH	EQU	12 
MMIO_DC_CANADIAN_FRENCH	EQU	3 
MMIO_LC_SWISS_FRENCH	EQU	12 
MMIO_DC_SWISS_FRENCH	EQU	4 
MMIO_LC_HEBREW	EQU	13 
MMIO_DC_HEBREW	EQU	1 
MMIO_LC_HUNGARIAN	EQU	14 
MMIO_DC_HUNGARIAN	EQU	1 
MMIO_LC_ICELANDIC	EQU	15 
MMIO_DC_ICELANDIC	EQU	1 
MMIO_LC_ITALIAN	EQU	16 
MMIO_DC_ITALIAN	EQU	1 
MMIO_LC_SWISS_ITALIAN	EQU	16 
MMIO_DC_SWISS_ITALIAN	EQU	2 
MMIO_LC_JAPANESE	EQU	17 
MMIO_DC_JAPANESE	EQU	1 
MMIO_LC_KOREAN	EQU	18 
MMIO_DC_KOREAN	EQU	1 
MMIO_LC_DUTCH	EQU	19 
MMIO_DC_DUTCH	EQU	1 
MMIO_LC_BELGIAN_DUTCH	EQU	19 
MMIO_DC_BELGIAN_DUTCH	EQU	2 
MMIO_LC_NORWEGIAN_BOKMAL	EQU	20 
MMIO_DC_NORWEGIAN_BOKMAL	EQU	1 
MMIO_LC_NORWEGIAN_NYNORSK	EQU	20 
MMIO_DC_NORWEGIAN_NYNORSK	EQU	2 
MMIO_LC_POLISH	EQU	21 
MMIO_DC_POLISH	EQU	1 
MMIO_LC_BRAZILIAN_PORTUGUESE	EQU	22 
MMIO_DC_BRAZILIAN_PORTUGUESE	EQU	1 
MMIO_LC_PORTUGUESE	EQU	22 
MMIO_DC_PORTUGUESE	EQU	2 
MMIO_LC_RHAETO_ROMANIC	EQU	23 
MMIO_DC_RHAETO_ROMANIC	EQU	1 
MMIO_LC_ROMANIAN	EQU	24 
MMIO_DC_ROMANIAN	EQU	1 
MMIO_LC_RUSSIAN	EQU	25 
MMIO_DC_RUSSIAN	EQU	1 
MMIO_LC_SERBO_CROATIAN_LATIN	EQU	26 
MMIO_DC_SERBO_CROATIAN_LATIN	EQU	1 
MMIO_LC_SERBO_CROATIAN_CYRILLIC	EQU	26 
MMIO_DC_SERBO_CROATIAN_CYRILLIC	EQU	2 
MMIO_LC_SLOVAK	EQU	27 
MMIO_DC_SLOVAK	EQU	1 
MMIO_LC_ALBANIAN	EQU	28 
MMIO_DC_ALBANIAN	EQU	1 
MMIO_LC_SWEDISH	EQU	29 
MMIO_DC_SWEDISH	EQU	1 
MMIO_LC_THAI	EQU	30 
MMIO_DC_THAI	EQU	1 
MMIO_LC_TURKISH	EQU	31 
MMIO_DC_TURKISH	EQU	1 
MMIO_LC_URDU	EQU	32 
MMIO_DC_URDU	EQU	1 
MMIO_LC_BAHASA	EQU	33 
MMIO_DC_BAHASA	EQU	1 
; 
;* Base function prototypes:
;  
; 
;* Compound File function prototypes:
;  
; 
;* Conversion Utility function prototypes:
;  
; 
;* MMPMMMIO.INI file migration utility
;  
; 
;* MMIO CODEC APIs
;  
ifdef INCL_MMIO_CODEC
endif
endif ;  _MMIO_H_   
