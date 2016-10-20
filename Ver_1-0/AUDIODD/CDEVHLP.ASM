

        PAGE    80,132
        .386p

        TITLE   CDEVHLP - 'C' INTERFACE FOR DEVHELP CALLS
        SUBTTL  'C' DEVHLP
        NAME    DEVHLPs

;********************** START OF SPECIFICATIONS *********************
;*
;* SOURCE FILE NAME:  CDEVHLP.ASM
;*
;* DESCRIPTIVE NAME:  'C' interface for OS/2 devhelp calls
;*
;* FUNCTION: This module provides the interface mechanism to translate a 'C'
;*           stacked based devhelp call to a MASM register based OS/2 devhelp
;*           call.
;*
;* NOTES:
;*    DEPENDENCIES: NONE
;*    RESTRICTIONS: Use assmebly language only.
;*
;* ENTRY POINTS:    _DevHlp_AllocGDTSelector
;*                  _DevHlp_AllocPhys
;*                  _DevHlp_PhysToGDTSelector
;*                  _DevHlp_PhysToVirt
;*                  _DevHlp_VirtToPhys
;*
;* EXTERNAL REFERENCES (system):  Far calls to OS/2 kernal DevHlp routines.
;*
;*********************** END OF SPECIFICATIONS **********************
;******************************************************************************
;                       I N C L U D E S
;******************************************************************************

.xlist
include devhlp.inc
.list

;******************************************************************************
;                       E X T E R N S
;******************************************************************************
_DATA   SEGMENT WORD PUBLIC USE16 'DATA'
        EXTRN   _DevHlp:DWORD
_DATA   ENDS

;******************************************************************************
;                   D A T A   D E C L A R A T I O N S
;******************************************************************************

;******************************************************************************
;                           C O D E
;******************************************************************************

_TEXT   SEGMENT WORD PUBLIC USE16 'CODE'
        ASSUME cs:_TEXT

;********************** START OF SPECIFICATIONS *********************
;*
;* SUBROUTINE NAME: _DevHlp_AllocGDTSelector
;*
;* DESCRIPTIVE NAME:
;*
;* FUNCTION: Call the devhelper routine with the 'C' parameters passed in the
;*           registers.
;*
;* NOTES:
;*
;* ENTRY POINTS:  _DevHlp_AllocGDTSelector
;*     LINKAGE:   CALL NEAR
;*
;* INPUT: ULONG block size in bytes
;*
;* EXIT-NORMAL:  Refer to OS/2 devhelp api document ES:DI contains locaton of array
;*               of words of GDT's.
;*
;* EXIT_ERROR: 'Carry flag set' and AX=error code
;*
;* EFFECTS:  Registers:
;*
;* INTERNAL REFERENCES: none
;*
;* EXTERNAL REFERENCES: DevHlp_AllocGDTSelector
;*
;*********************** END OF SPECIFICATIONS **********************

_DevHlp_AllocGDTSelector proc near
        ASSUME cs:_TEXT,ds:_DATA,es:NOTHING,ss:NOTHING
NumGDTSelectors EQU     <[bp+4]>
paGDTSel        EQU     <[bp+8]>        ; seg:off of GDT array
        push    bp
        mov     bp,sp                   ; allocate NULL stack frame
        push    cx
        push    dx
        push    di
        push    es

IFDEF   DEVHLP_DEBUG
        int     3
ENDIF
        mov     es,WORD PTR paGDTSel+2          ; get seg.
        mov     di,WORD PTR paGDTSel            ; get off.
        mov     cx,NumGDTSelectors
        mov     dl,DevHlp_AllocGDTSelector
        call    [_DevHlp]
        jc      allocgdt
        xor     ax,ax                           ; clear for no error
allocgdt:
        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bp
        ret
_DevHlp_AllocGDTSelector endp

;********************** START OF SPECIFICATIONS *********************
;*
;* SUBROUTINE NAME: _DevHlp_AllocPhys
;*
;* DESCRIPTIVE NAME:
;*
;* FUNCTION: Call the devhelper routine with the 'C' parameters passed in the
;*           registers.
;*
;* NOTES:
;*
;* ENTRY POINTS:  _DevHlp_AllocPhys
;*     LINKAGE:   CALL NEAR
;*
;* INPUT: ULONG block size in bytes
;*
;* EXIT-NORMAL:  Refer to OS/2 devhelp api document ax:bx = 32 bit physical addr
;*               AX = 0
;*
;* EXIT_ERROR: AX=error code
;*
;* EFFECTS:  Registers:
;*
;* INTERNAL REFERENCES: none
;*
;* EXTERNAL REFERENCES: DevHlp_AllocPhys
;*
;*********************** END OF SPECIFICATIONS **********************

_DevHlp_AllocPhys proc near
        ASSUME cs:_TEXT,ds:_DATA,es:NOTHING,ss:NOTHING
BytesLo         EQU     <[bp+4]>        ; memory block size
BytesHi         EQU     <[bp+6]>
Location        EQU     <[bp+8]>        ; above or below 1MB
pRetAddress     EQU     <[bp+10]>       ; 32bit address to allocated memory blk

        push    bp
        mov     bp,sp                   ; allocate NULL stack frame
        push    bx
        push    dx
        push    es

IFDEF   DEVHLP_DEBUG
        int     3
ENDIF
        mov     bx,BytesLo
        mov     ax,BytesHi
        mov     dh,BYTE PTR Location            ; 0=above 1MB
        mov     dl,DevHlp_AllocPhys             ; 1=below 1MB
        call    [_DevHlp]
        jc      allocphys

        mov     dx,bx                           ; Need bx for addressing
        les     bx,pRetAddress                  ; Dereference address of
                                                ; callers variable
        mov     es:[bx+2],ax                    ; Return high 16 bits and
        mov     es:[bx],dx                      ; Low 16 bits (flat pointer)

        xor     ax,ax                           ; clear ax => no error
allocphys:
        pop     es
        pop     dx
        pop     bx
        pop     bp                              ; restore callers frame
        ret
_DevHlp_AllocPhys endp

;********************** START OF SPECIFICATIONS *********************
;*
;* SUBROUTINE NAME: _DevHlp_PhysToGDTSelector
;*
;* DESCRIPTIVE NAME:
;*
;* FUNCTION: Call the devhelper routine with the 'C' parameters passed in the
;*           registers.
;*
;* NOTES:
;*
;* ENTRY POINTS:  _DevHlp_PhysToGDTSelector
;*     LINKAGE:   CALL NEAR
;*
;* INPUT: 32bit physical address, length of segment, selector to setup
;*
;* EXIT-NORMAL: Refer to OS/2 devhelp api document
;*              ptr that contained 32bit physaddrr is now a sel:off.
;*              AX = 0
;*
;* EXIT_ERROR: ax=error code
;*
;* EFFECTS:  Registers:
;*
;* INTERNAL REFERENCES:
;*
;* EXTERNAL REFERENCES: DevHlp_PhysToGDTSelector
;*
;*********************** END OF SPECIFICATIONS **********************

_DevHlp_PhysToGDTSelector proc near
        ASSUME cs:_TEXT,ds:_DATA,es:NOTHING,ss:NOTHING
PhysAddress     EQU     <[bp+4]>
ByteLength      EQU     <[bp+8]>
Selector        EQU     <[bp+10]>
        push    bp
        mov     bp,sp                   ; allocate NULL stack frame
        push    bx
        push    cx
        push    dx
        push    si

IFDEF   DEVHLP_DEBUG
        int     3
ENDIF
        mov     ax,WORD PTR PhysAddress+2
        mov     bx,WORD PTR PhysAddress
        mov     cx,ByteLength
        mov     si,Selector
        mov     dl,DevHlp_PhysToGDTSelector
        call    DWORD PTR [_DevHlp]      ; return error in ax if CF
        jc      physToGDT
        xor     ax,ax
physToGDT:
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     bp
        ret
_DevHlp_PhysToGDTSelector endp



;********************** START OF SPECIFICATIONS *********************
;*
;* SUBROUTINE NAME: _DevHlp_PhysToVirt
;*
;* DESCRIPTIVE NAME:
;*
;* FUNCTION: Call the devhelper routine with the 'C' parameters passed in the
;*           registers.
;*
;* NOTES:
;*
;* ENTRY POINTS:  _DevHlp_PhysVirt
;*     LINKAGE:   CALL NEAR
;*
;* INPUT: ULONG block size in bytes
;*
;* EXIT-NORMAL: Refer to OS/2 devhelp api document ax:bx = 32 bit physical addr
;*              AX = 0
;*
;* EXIT_ERROR: ax=error code
;*
;* EFFECTS:  Registers:
;*
;* INTERNAL REFERENCES: none
;*
;* EXTERNAL REFERENCES: DevHlp_PhysToVirt
;*
;*********************** END OF SPECIFICATIONS **********************

_DevHlp_PhysToVirt proc near
        ASSUME cs:_TEXT,ds:_DATA,es:NOTHING,ss:NOTHING
PhysAddress     EQU     <[bp+4]>        ; physical address
BlockSize       EQU     <[bp+8]>        ; block size
pRetAddress     EQU     <[bp+12]>       ; address of returned virtual pointer
        push    bp
        mov     bp,sp                   ; allocate NULL stack frame
        push    dx
        push    es
        push    di
        push    ds
        push    si

IFDEF   DEVHLP_DEBUG
        int     3
ENDIF
        mov     ax,WORD PTR PhysAddress+2
        mov     bx,WORD PTR PhysAddress
        mov     cx,BlockSize
        xor     dh,dh                           ; set DS:SI
        mov     dl,DevHlp_PhysToVirt
        call    [_DevHlp]
        jc      physvirterr

        les     di,pRetAddress                  ; de-reference pointer
        mov     es:[di+2],ds                    ; hi word (return ds:si)
        mov     es:[di],si                      ; lo word
        xor     ax,ax                           ; clear for no error

physvirterr:
        pop     si
        pop     ds
        pop     di
        pop     es
        pop     dx
        pop     bp
        ret
_DevHlp_PhysToVirt endp

;********************** START OF SPECIFICATIONS *********************
;*
;* SUBROUTINE NAME: _DevHlp_VirtToPhys
;*
;* DESCRIPTIVE NAME:
;*
;* FUNCTION: Call the devhelper routine with the 'C' parameters passed in the
;*           registers.
;*
;* NOTES:
;*
;* ENTRY POINTS:  _DevHlp_VirtToPhys
;*     LINKAGE:   CALL NEAR
;*
;* INPUT:
;*
;* EXIT-NORMAL: Refer to OS/2 devhelp api document
;*              AX = 0
;*
;* EXIT_ERROR: ax = -1
;*
;* EFFECTS:  Registers:
;*
;* INTERNAL REFERENCES:
;*
;* EXTERNAL REFERENCES: DevHlp_VirtToPhys
;*
;*********************** END OF SPECIFICATIONS **********************

_DevHlp_VirtToPhys proc near
        ASSUME cs:_TEXT,ds:_DATA,es:NOTHING,ss:NOTHING
VirtAddress     EQU     <[bp+4]>        ; dword
pRetPhysAddress EQU     <[bp+8]>        ; dword
        push    bp
        mov     bp,sp                   ; allocate NULL stack frame
        push    si
        push    dx
        push    ds

IFDEF   DEVHLP_DEBUG
        int     3
ENDIF
        lds     si,VirtAddress
        mov     dl,DevHlp_VirtToPhys
        call    DWORD PTR [_DevHlp]     ; return error in ax if CF
        jc      VirtPhysErr

        les     si,pRetPhysAddress      ; de-reference pointer
        mov     es:[si+2],ax            ; Return high word
        mov     es:[si],bx              ; Return low word
        xor     ax, ax                  ; Success, RC = zero
        jmp     VirtPhysDone

VirtPhysErr:
        mov     ax,-1

VirtPhysDone:
        pop     ds
        pop     dx
        pop     si
        pop     bp
        ret
_DevHlp_VirtToPhys endp

_TEXT   ENDS
        END
