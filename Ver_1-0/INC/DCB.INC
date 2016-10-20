;***************************************************************************  
;                                                                             
;  Module Name: DCB.H                                                         
;                                                                             
;  OS/2 2.0 Multimedia Extensions Sync/Stream Manager Device Control          
;  Block defintitions.                                                        
;                                                                             
;  Copyright (c) International Business Machines Corporation 1991, 1992       
;                         All Rights Reserved                                 
;                                                                             
;***************************************************************************  
;**********************************************
;*
;* DCB - Common Device Control Block
;*               (*** PACKED STRUCTURE ***)
;*********************************************  

dcb	STRUC
dcb_ulDCBLen	DD	?                ;  length of structure                   
dcb_szDevName	DB	MAX_SPI_NAME DUP (?) ;  device driver name                    
dcb	ENDS
;**********************************************
;*
;* DCB_AUDIOSH - Device Control Block for the
;*               audio stream handler.
;*               (*** PACKED STRUCTURE ***)
;*********************************************  

dcb_audiosh	STRUC
dcb_audiosh_ulDCBLen	DD	?                ;  length of structure                   
dcb_audiosh_szDevName	DB	MAX_SPI_NAME DUP (?) ;  device driver name                    
dcb_audiosh_ulSysFileNum	DD	?            ;  File handle number                    
dcb_audiosh	ENDS
