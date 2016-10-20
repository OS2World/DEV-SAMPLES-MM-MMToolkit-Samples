;***************************************************************************  
;                                                                             
;  Module Name: SSM.H                                                         
;                                                                             
;  OS/2 2.0 Multimedia Extensions Sync/Stream Manager Definitions file        
;                                                                             
;  Copyright (c) International Business Machines Corporation 1991, 1992       
;                         All Rights Reserved                                 
;                                                                             
;***************************************************************************  
MAX_SPI_NAME	EQU	9       ;  Maximum ASCIIZ stream handler name/class    
                             ;  and device driver name                      
;***************************************************************************  
;                                                                             
;   Forward Declarations                                                      
;                                                                             
;***************************************************************************  
;***************************************************************************  
;                                                                             
;   Global Type Declarations                                                  
;                                                                             
;***************************************************************************  

lockh	STRUC
Memory_lock	DB	16 DUP (?)
lockh	ENDS
RC	EQU	ULONG                      ;  Used for all return codes            

slave	STRUC
slave_hstreamSlave	DD	?          ;  Handle for this slave              
slave_mmtimeStart	DD	?           ;  Offset in master stream time to start   
slave	ENDS

hand	STRUC
hand_szHandlerClass	DB	MAX_SPI_NAME DUP (?) ;  Stream handler class        
hand_szHandlerName	DB	MAX_SPI_NAME DUP (?)  ;  Stream handler name         
hand	ENDS
INCLUDE spcb.INC
INCLUDE evcb.INC
INCLUDE acb.INC
INCLUDE dcb.INC
;***************************************************************************  
;  INI file support defines                                                   
;***************************************************************************  
;  RCDATA defines:                                                            
SPI_RESOURCE	EQU	1                ;  RCDATA resource number              
SH_DLL_TYPE	EQU	0                ;  DLL type stream handler             
SH_PDD_TYPE	EQU	1                ;  PDD type stream handler             
SPCBSIZE	EQU	72              ;  SPCB length, the Resource compiler  
                                       ;    doesn't allow sizeof(SPCB).       
ifdef XXXXX
endif
;***************************************************************************  
;                                                                             
;   Function Prototypes for SPI functions                                     
;   All use Pure C calling convention                                         
;   Six major categories:                                                     
;      - stream creation                                                      
;      - stream control                                                       
;      - event handling                                                       
;      - sync handling                                                        
;      - configuration                                                        
;      - stream protocol programming                                          
;***************************************************************************  
;  SpiStopStream() ulFlags defines:                                           
SPI_STOP_STREAM	EQU	0000H          ;  Only stop this one stream       
                                           ;  The default is to pause the data            
                                           ;   stream. No change to data buffers.         
                                           ;   synchronous call (no event notification)   
SPI_STOP_SLAVES	EQU	0001H          ;  Stop this master and any slaves             
SPI_STOP_FLUSH	EQU	0002H          ;  Stop and flush all filled buffers.          
                                           ;   Stops the Src & Tgt SH, lets the           
                                           ;   Tgt SH run until out of buffers            
                                           ;   thereby, "flushing" all buffers.           
                                           ;   Tgt SH stops when out of buffers           
                                           ;   (This is a ASYNCHRONOUS call, event        
                                           ;    notification sent to appl when done.)     
SPI_STOP_DISCARD	EQU	0004H          ;  Stop and discard all filled buffers.        
                                           ;   Stops the Src & Tgt SH                     
                                           ;   then discards all buffers current for      
                                           ;   this stream.                               
                                           ;   (This is a ASYNCHRONOUS call, event        
                                           ;    notification sent to appl when done.)     
;***************************************************************************  
;                                                                             
;   Function Prototypes for SMH functions                                     
;   All use Pure C calling convention                                         
;  NOTE ABOUT POINTERS:                                                       
;   0:32 linear addresses for ring 3 (DLL) stream handlers                    
;   16:16 selector/offset addresses for ring 0 (PDD) stream handlers          
;***************************************************************************  

smh_common	STRUC
smhfunc_ulFunction	DD	?        ;  SMH command function                     
smhfunc_hid	DD	?               ;  handler ID                               
smh_common	ENDS
;***************************************************************************  
SMH_REGISTER	EQU	0               ;  (Valid only from ring 3 stream   
                                               ;   handler)                   

parm_reg	STRUC
smhreg_ulFunction	DD	?                  ;  SMH command function        
smhreg_pszSHName	DD	?                   ;  Max size = 8 characters     
smhreg_phidSrc	DD	?                     ;  (9 including null)          
smhreg_phidTgt	DD	?
smhreg_pshcfnEntry	DD	?                 ;  Handler entry point for SHC's    
                                               ;   Only used for Ring 0 handlers.  
smhreg_ulFlags	DD	?
smhreg_ulMaxNumStreams	DD	?             ;  Max # of streams            
smhreg_ulMaxNumEvents	DD	?              ;  Max # of events/stream      
parm_reg	ENDS
REGISTER_TGT_HNDLR	EQU	0001H          ;  This handler is Target.     
REGISTER_SRC_HNDLR	EQU	0002H          ;  This handler is Source.     
                                               ;  Handlr can be both.         
REGISTER_NONSTREAMING	EQU	0004H          ;  Handler is non-streaming.   
VALIDREGISTERFLAGS	EQU	(REGISTER_TGT_HNDLR  OR  REGISTER_SRC_HNDLR  OR  REGISTER_NONSTREAMING)
;  Notes:                                                                       
;   - Since ring 0 (DD) stream handlers can not use the IDC interface to the    
;     sync/stream manager during DD initialization time, it must use the IOCTL  
;     function to perform the SMH_REGISTER function. The addresses are 16:16    
;     for this IOCTl instead of 0:32, because it comes form a device driver.    
SSMDD_CATEGORY	EQU	0081H     ;  Category (For ring 0 stream handler)   
IOCTL_FUNC_SMH_REGISTER	EQU	0040H     ;  Function (For ring 0 stream handler)   
;***************************************************************************  
SMH_DEREGISTER	EQU	1               ;  SMH_Deregister()            

parm_dereg	STRUC
smhder_ulFunction	DD	?                  ;  SMH command function        
smhder_pszSHName	DD	?                   ;  Max size = 8 char, 9 includes   
parm_dereg	ENDS
;***************************************************************************  
SMH_REPORTEVENT	EQU	2               ;  SMH_ReportEvent()           

parm_event	STRUC
smhrev_ulFunction	DD	?                  ;  SMH command function        
smhrev_hid	DD	?
smhrev_hevent	DD	?                      ;  Used only for explicit events     
                                               ;   Must be 0 for sync and implicit  
                                               ;   events (i.e. error events)       
smhrev_pevcbEvent	DD	?                  ;  Event status & HSTREAM            
parm_event	ENDS
;***************************************************************************  
SMH_NOTIFY	EQU	3               ;  SMH_Notify()                

parm_notify	STRUC
smhnot_ulFunction	DD	?       ;  Input - SMH command function           
smhnot_hid	DD	?              ;  Input                                  
smhnot_hstream	DD	?          ;  Input                                  
smhnot_ulFlags	DD	?          ;  Input/Output                           
smhnot_ulGetNumEntries	DD	?  ;  Input/Output - # of entries in table   
smhnot_ulRetNumEntries	DD	?  ;  Input/Output - # of entries in table   
smhnot_pGetBufTab	DD	?       ;  Output - Ptr to buffer/record table    
smhnot_pRetBufTab	DD	?       ;  Input - Ptr to buffer/record table     
parm_notify	ENDS

srcbuftab	STRUC
srcbuftab_pBuffer	DD	?      ;  Ptr to buffer                               
srcbuftab_pRecord	DD	?      ;  Ptr to record in buffer (split-streams only)    
srcbuftab_ulLength	DD	?     ;  Max buf length on GetEmpty, Filled          
                               ;  (actual) rec/buf length on ReturnFull.      
srcbuftab_ulMessageParm	DD	? ;  Message to passed to application           
srcbuftab_mmtimeOffset	DD	? ;  mmtime offset from beginning of buffer      
srcbuftab	ENDS

tgtbuftab	STRUC
tgtbuftab_pBuffer	DD	?      ;  Ptr to buffer                               
tgtbuftab_ulBufId	DD	?      ;  Buffer ID. Passed to SH on GetFull, must    
                               ;  be passed back to SSM on ReturnEmpty.       
tgtbuftab_ulLength	DD	?     ;  Filled (actual) buffer length on GetFull,   
                               ;  unused on ReturnEmpty.                      
tgtbuftab_ulMessageParm	DD	? ;  Message to passed to application           
tgtbuftab_mmtimeOffset	DD	? ;  mmtime offset from beginning of buffer      
tgtbuftab	ENDS
; SMH_ulFlags:                                                                
BUF_GETEMPTY	EQU	00000001H    ;  Ptr returned in SMH_pGetBuffer    
BUF_RETURNFULL	EQU	00000002H    ;  Ptr passed in SMH_pRetBuffer      
BUF_GETFULL	EQU	00000004H    ;  Ptr returned in SMH_pGetBuffer    
BUF_RETURNEMPTY	EQU	00000008H    ;  Ptr passed in SMH_pRetBuffer      
BUF_GIVEBUF	EQU	00000010H    ;  Ptr passed in SMH_pRetBuffer      
BUF_EOS	EQU	00000100H    ;  End of stream. Source sets this on   
                                         ;   last buffer. Target sets it after   
                                         ;   consuming the laset buffer.         
BUF_RECORDS	EQU	00004000H    ;  One or more records (Split-stream)   
BUF_LASTRECORD	EQU	00008000H    ;  (BUF_RETURNFULL only) Mark           
                                         ;  these record(s) as being the         
                                         ;  last in the buffer. So the SSM       
                                         ;  knows the maximum # of records       
                                         ;  produced for this buffer.            
                                         ;  (Split stream only)                  
; SMH_ulFlags: (Available at ring 0 only)                                        
BUF_LINEAR	EQU	00010000H    ;  Ptrs are Global linear (Ring 0 only)   
BUF_PHYSICAL	EQU	00020000H    ;  Ptrs are physical (Ring 0 only)        
                                         ;  (valid only for phyically              
                                         ;   contiguous buffers)                   
BUF_RESERVED	EQU	00040000H    ;  Reserved                               
VALIDSOURCEFLAGS	EQU	(BUF_RETURNFULL OR BUF_GETEMPTY OR BUF_EOS OR BUF_LASTRECORD OR BUF_PHYSICAL OR BUF_LINEAR OR BUF_RECORDS OR BUF_GIVEBUF)
VALIDTARGETFLAGS	EQU	(BUF_RETURNEMPTY OR BUF_GETFULL OR BUF_PHYSICAL OR BUF_LINEAR OR BUF_EOS)
;***************************************************************************  
SMH_LOCKMEM	EQU	4               ;  SMH_LockMem()               

parm_lockm	STRUC
smhloc_ulFunction	DD	?                  ;  SMH command function        
smhloc_pBuffer	DD	?
smhloc_ulBufSize	DD	?
smhloc_plockh	DD	?
smhloc_ulFlags	DD	?
parm_lockm	ENDS
; SMH_ulFlags:                                                                
SSM_LOCKMEM	EQU	0000H          ;  Lock Memory.                
SSM_UNLOCKMEM	EQU	0001H          ;  Unlock memory.              
SSM_CONTIGLOCK	EQU	0002H          ;  Lock memory contiguously.   
;***************************************************************************  
;***************************************************************************  
;                                                                             
;   Function Prototypes for SHC functions (Stream Handler Functions)          
;   All use Pure C calling convention                                         
;                                                                             
;  NOTE ABOUT POINTERS:                                                       
;   0:32 linear addresses for ring 3 (DLL) stream handlers                    
;   16:16 selector/offset addresses for ring 0 (PDD) stream handlers          
;***************************************************************************  

shc_common	STRUC
shcfunc_ulFunction	DD	?        ;  Handler command function                 
shcfunc_hid	DD	?               ;  handler ID                               
shc_common	ENDS
;***************************************************************************  
SHC_ASSOCIATE	EQU	0 

parm_assoc	STRUC
shcass_ulFunction	DD	?        ;  Handler command function               
shcass_hid	DD	?               ;  handler ID                             
shcass_hstream	DD	?           ;  handle of stream instance              
shcass_pacb	DD	?              ;  pointer to associate control block     
parm_assoc	ENDS
;***************************************************************************  
SHC_CLOSE	EQU	1 

parm_close	STRUC
shcclo_ulFunction	DD	?        ;  Handler command function               
shcclo_hid	DD	?               ;  handler ID                             
parm_close	ENDS
;***************************************************************************  
SHC_CREATE	EQU	2 

parm_create	STRUC
shccrt_ulFunction	DD	?        ;  Handler command function               
shccrt_hid	DD	?               ;  handler ID                             
shccrt_hstream	DD	?           ;  handle of stream instance              
shccrt_spcbkey	DB	SIZE SPCBKEY DUP (?)           ;  type of stream to create               
shccrt_pspcb	DD	?             ;  Pointer to SPCB from handler           
shccrt_hstreamBuf	DD	?        ;  Used for split streams or to associate  
                                    ;  another stream with this stream.       
shccrt_pdcb	DD	?              ;  pointer to device control block        
parm_create	ENDS
;***************************************************************************  
SHC_DESTROY	EQU	3 

parm_destroy	STRUC
shcdes_ulFunction	DD	?        ;  Handler command function               
shcdes_hid	DD	?               ;  handler ID                             
shcdes_hstream	DD	?           ;  handle of stream instance              
parm_destroy	ENDS
;***************************************************************************  
SHC_START	EQU	4 

parm_start	STRUC
shcstr_ulFunction	DD	?        ;  Handler command function               
shcstr_hid	DD	?               ;  handler ID                             
shcstr_hstream	DD	?           ;  handle of stream instance              
shcstr_ulFlags	DD	?           ;  Start flags                            
parm_start	ENDS
;  ulFlags:                                                                   
FLUSH_START	EQU	0001H     ;  This start is a result of a Flush Stop command.   
;***************************************************************************  
SHC_STOP	EQU	5 

parm_stop	STRUC
shcstp_ulFunction	DD	?        ;  Handler command function               
shcstp_hid	DD	?               ;  handler ID                             
shcstp_hstream	DD	?           ;  handle of stream instance              
shcstp_ulFlags	DD	?           ;  Stop flags (refer to ulFlags defines under   
                                    ;   SpiStopStream() call.                 
parm_stop	ENDS
;***************************************************************************  
SHC_SEEK	EQU	6 

parm_seek	STRUC
shcsek_ulFunction	DD	?        ;  Handler command function               
shcsek_hid	DD	?               ;  handler ID                             
shcsek_hstream	DD	?           ;  handle of stream instance              
shcsek_ulFlags	DD	?           ;  Seek flag (refer to ulFlags defines    
                                    ;   under SpiSeekStream() call).          
shcsek_lSeekPoint	DD	?        ;  seek to point , mmtime or other        
parm_seek	ENDS
;***************************************************************************  
SHC_ENABLE_EVENT	EQU	7 

parm_enevent	STRUC
shceev_ulFunction	DD	?        ;  Handler command function               
shceev_hid	DD	?               ;  handler ID                             
shceev_hevent	DD	?            ;  handle of event to enable              
shceev_pevcbUser	DD	?         ;  User event information (e.g. hstream   
                                    ;   EventType, EventSubType)              
parm_enevent	ENDS
;***************************************************************************  
SHC_DISABLE_EVENT	EQU	8 

parm_disevent	STRUC
shcdev_ulFunction	DD	?        ;  Handler command function               
shcdev_hid	DD	?               ;  handler ID                             
shcdev_hstream	DD	?           ;  handle to stream for this event        
shcdev_hevent	DD	?            ;  handle of event to disable             
parm_disevent	ENDS
;***************************************************************************  
SHC_ENABLE_SYNC	EQU	9 

parm_ensync	STRUC
shcesy_ulFunction	DD	?        ;  Handler command function               
shcesy_hid	DD	?               ;  handler ID                             
shcesy_hstream	DD	?           ;  handle of stream instance              
shcesy_ulFlags	DD	?           ;  Sync flags                             
shcesy_mmtimeSync	DD	?        ;  granularity of sync interval (if null, use default)   
shcesy_pevcbSyncPulse	DD	?  ;  Sync pulse EVCB information            
shcesy_ulSyncPulseSem	DD	?    ;  Optional 16 bit system                 
                                    ;  Semaphore for handler                  
parm_ensync	ENDS
;  ulFlags:                                                                   
SYNC_MASTER	EQU	0000H     ;  The handler will be a master for this sync  
SYNC_SLAVE	EQU	0001H     ;  The handler will be a slave for this sync   
;***************************************************************************  
SHC_DISABLE_SYNC	EQU	10 

parm_dissync	STRUC
shcdsy_ulFunction	DD	?        ;  Handler command function               
shcdsy_hid	DD	?               ;  handler ID                             
shcdsy_hstream	DD	?           ;  handle of stream instance              
parm_dissync	ENDS
;***************************************************************************  
SHC_GET_TIME	EQU	11 

parm_gtime	STRUC
shcgti_ulFunction	DD	?        ;  Handler command function               
shcgti_hid	DD	?               ;  handler ID                             
shcgti_hstream	DD	?           ;  handle of stream instance              
shcgti_mmtimeCurrent	DD	?     ;  Returns current stream time            
parm_gtime	ENDS
;***************************************************************************  
SHC_GET_PROTOCOL	EQU	12 

parm_gprot	STRUC
shcgpt_ulFunction	DD	?        ;  Handler command function               
shcgpt_hid	DD	?               ;  handler ID                             
shcgpt_spcbkey	DB	SIZE SPCBKEY DUP (?)           ;  key of SPCB                            
shcgpt_pspcb	DD	?             ;  ptr to SPCB (output)                   
parm_gprot	ENDS
;***************************************************************************  
SHC_INSTALL_PROTOCOL	EQU	13 

parm_instprot	STRUC
shcipt_ulFunction	DD	?        ;  Handler command function               
shcipt_hid	DD	?               ;  handler ID                             
shcipt_spcbkey	DB	SIZE SPCBKEY DUP (?)           ;  key of SPCB                            
shcipt_pspcb	DD	?             ;  ptr to SPCB to install                 
shcipt_ulFlags	DD	?           ;  Install / Deinstall flags              
parm_instprot	ENDS
;***************************************************************************  
SHC_ENUMERATE_PROTOCOLS	EQU	14 

parm_enumprot	STRUC
shcqpi_ulFunction	DD	?        ;  Handler command function               
shcqpi_hid	DD	?               ;  handler ID                             
shcqpi_paSPCBKeys	DD	?        ;  ptr to buffer to fill with SPCB keys   
shcqpi_pulNumSPCBKeys	DD	?    ;  # of entries buffer on input,          
                                    ;   # of SPCB key on output.              
parm_enumprot	ENDS
;***************************************************************************  
SHC_NEGOTIATE_RESULT	EQU	15 

parm_negotiate	STRUC
shcneg_ulFunction	DD	?        ;  Handler command function               
shcneg_hid	DD	?               ;  handler ID                             
shcneg_hstream	DD	?           ;  handle of stream instance              
shcneg_pspcb	DD	?             ;  Ptr to negotiated SPCB                 
shcneg_ulErrorStatus	DD	?     ;  On error, indicates which field in the  
parm_negotiate	ENDS
;***************************************************************************  
;   Reserved - SHC command       16L                                          
;***************************************************************************  
