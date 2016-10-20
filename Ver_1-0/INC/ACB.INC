;***************************************************************************  
;                                                                             
;  Module Name: ACB.H                                                         
;                                                                             
;  OS/2 2.0 Multimedia Extensions Sync/Stream Manager Associate               
;  Control Block Definitions.                                                 
;                                                                             
;  Copyright (c) International Business Machines Corporation 1991, 1992       
;                         All Rights Reserved                                 
;                                                                             
;***************************************************************************  
MAX_PORT_NAME	EQU	40            ;  Max port name length           
MAX_PORTS	EQU	16            ;  Max number ports               
;**********************************************
;*
;* ACB - Associate Control Block
;*
;*       Each stream handler must define the
;*       ACB for each data object type that
;*       expect ACB's from the application.
;*
;*********************************************  

acb	STRUC
acb_ulACBLen	DD	?              ;  length of structure                     
acb_ulObjType	DD	?
acb_ulParm1	DD	?
acb_ulParm2	DD	?
acb	ENDS
;******************************************************
;* FSSH - File system stream handler MMIO Object ACB
;*****************************************************  
ACBTYPE_MMIO	EQU	0001H   ;  MMIO object                        

acb_mmio	STRUC
acbmmio_ulACBLen	DD	?              ;  length of structure                     
acbmmio_ulObjType	DD	?             ;  ACBTYPE_MMIO                            
acbmmio_hmmio	DD	?                 ;  Handle of media element mgr obj         
acb_mmio	ENDS
;******************************************************
;* MSH - Memory stream handler Playlist Object ACB
;*****************************************************  
ACBTYPE_MEM_PLAYL	EQU	0003H   ;  Memory playlist object             

acb_mem_playl	STRUC
acbplyl_ulACBLen	DD	?              ;  length of structure                     
acbplyl_ulObjType	DD	?             ;  ACBTYPE_MEM_PLAYL                       
acbplyl_pMemoryAddr	DD	?           ;  Starting address of memory obj          
acb_mem_playl	ENDS
;******************************************************
;* CDDASH - CD DA stream handler Object ACB
;*****************************************************  
ACBTYPE_CDDA	EQU	0004H   ;   Compact disk - digital audio obj  

acb_CDDA	STRUC
acbcdda_ulACBLen	DD	?              ;  length of structure                     
acbcdda_ulObjType	DD	?             ;  ACBTYPE_CDDA                            
acbcdda_bCDDrive	DB	?              ;  CD drive letter                         
acb_CDDA	ENDS
;******************************************************
;* MISH - MIDI stream handler port-stream table ACB
;*****************************************************  
ACBTYPE_MISH	EQU	0005H       ;  MIDI port-stream table         

acb_MISH	STRUC
acbmish_ulACBLen	DD	?                     ;  length of structure            
acbmish_ulObjType	DD	?                    ;  ACBTYPE_MISH                   
acbmish_hstreamDefault	DD	?               ;  Default hstream to use when mapper   
                                           ;  is turned off.                 
acbmish_ulDeviceTypeID	DD	?               ;  device type id                 
acbmish_ulpMapperPorts	DD	?               ;  pointer to mapper port table   
acbmish_ulNumInStreams	DD	?
acbmish_hstreamIn	DD	MAX_PORTS DUP (?)         ;  Array of Input streams         
acbmish_ulNumOutStreams	DD	?
acbmish_hstreamOut	DD	MAX_PORTS DUP (?)        ;  Array of Output streams        
                                           ;  The index into the array is    
                                           ;  the source channel for that    
                                           ;  stream.                        
acb_MISH	ENDS
;******************************************************
;* MISH - MIDI stream handler SET ACB
;*****************************************************  
ACBTYPE_SET	EQU	0006H       ;  MIDI set function              

acb_set	STRUC
acbset_ulACBLen	DD	?                     ;  length of structure            
acbset_ulObjType	DD	?                    ;  ACBTYPE_SET                    
acbset_ulFlags	DD	?                      ;  Set flags                      
acbset_ulReserved	DD	?                   ;  Tempo change value (used only if   
                                           ;   MIDI_TEMPO_CHANGE)            
acb_set	ENDS
;  ulFlags defines:                                                          
MIDI_MAP_ON	EQU	0000H       ;  turn mapping function on in MISH     
MIDI_MAP_OFF	EQU	0001H       ;  turn mapping function off in MISH    
