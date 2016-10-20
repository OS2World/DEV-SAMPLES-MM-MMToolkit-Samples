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
;***************************************************************************
;*
;* DCB - Common Device Control Block
;*               (*** PACKED STRUCTURE ***)
;**************************************************************************  

DCB	STRUC
dcb_ulDCBLen	DD	?                ;  length of structure                   
dcb_szDevName	DB	MAX_SPI_NAME DUP (?) ;  device driver name                    
DCB	ENDS
;***************************************************************************
;*
;* E_DCB - Extended Device Control Block
;*
;*         This structure will grow over time as new fields are added
;*         to the end of the structure.  If you manipulate the structure,
;*         be sure to check the length field.
;*
;*               (*** PACKED STRUCTURE ***)
;**************************************************************************  

E_DCB	STRUC
e_dcb_ulDCBLen	DD	?                ;  length of structure                   
e_dcb_szDevName	DB	MAX_SPI_NAME DUP (?) ;  device driver name                    
e_dcb_ulSysFileNum	DD	?            ;  file handle number                    
   ;     new fields will be added here                                       
E_DCB	ENDS
