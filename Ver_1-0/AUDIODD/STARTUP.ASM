        PAGE    80,132

        TITLE  Audio OS/2 Device Driver
        .seq                                      ; Use segments in order listed
        .286p                                     ; Allow priviledged instruct

;********************** START OF SPECIFICATIONS *********************
;*
;* SOURCE FILE NAME:  STARTUP.ASM
;*
;* DESCRIPTIVE NAME:  Audio device driver PDD 'ASM' startup module for 'C'.
;*
;* FUNCTION: Provides the user's "main" routine with the address of
;*           the request packet.
;*
;*           Satisfies the C EXTRN for __acrtused, which prevents the startup
;*           logic from being included from the standard C library.
;*
;*********************** END OF SPECIFICATIONS **********************

;******************************************************************************
;                            S E G M E N T S
;******************************************************************************

_DATA   SEGMENT WORD PUBLIC 'DATA'
_DATA   ENDS

_BSS    SEGMENT WORD PUBLIC 'BSS'
_BSS    ENDS

CONST   SEGMENT WORD PUBLIC 'CONST'
CONST   ENDS

_ENDDATA   SEGMENT WORD PUBLIC 'ENDDATA'
_ENDDATA   ENDS

_TEXT   SEGMENT WORD PUBLIC 'CODE'
        EXTRN  _Strategy_c:NEAR
        EXTRN  _DDCmdInternalEntryPoint:far
        PUBLIC  __acrtused
        PUBLIC _Strategy
        PUBLIC _DDCMDEntryPoint
_TEXT   ENDS

_ENDTEXT SEGMENT WORD PUBLIC 'ENDCODE'
_ENDTEXT   ENDS


DGROUP  GROUP   CONST, _BSS, _DATA, _ENDDATA
CGRP    GROUP   _TEXT, _ENDTEXT

;******************************************************************************
;  C O D E   S E G M E N T
;******************************************************************************
_TEXT   SEGMENT WORD PUBLIC 'CODE'
;;        ASSUME cs:_TEXT, ds:DGROUP, es:NOTHING, ss:NOTHING
        ASSUME ds:DGROUP, es:NOTHING, ss:NOTHING        ; MASM 6.00



;********************** START OF SPECIFICATIONS *********************
;*
;* SUBROUTINE NAME: Strategy
;*
;* DESCRIPTIVE NAME: Audio PDD Strategy Routine
;*
;* ENTRY POINTS:  Strategy
;*
;* INPUT:  DS    = DD Data Segment
;*         ES:BX -> Pointer to the Request Packet
;*
;*            The Request Packet:
;*            ------------------
;*            HEADER:                 ; Request Header
;*              BYTE    .PktLen       ;   length of request packet
;*              BYTE                  ;   Block Device unit code (unused)
;*              BYTE    .PktCmd       ;   command code
;*              WORD    .PktStatus    ;   return status
;*              DWORD   .PktDevLink   ;   ReqPkt queue linkage
;*                                    ;
;*            DATA:                   ;
;*              ????                  ; Depends on function requested
;*
;* EXIT-NORMAL: .PktStatus  = 0100h ( No Error              )
;*                            0300h ( Device Busy, No Error )
;*
;* EXIT-ERROR:  .PktStatus  = 8101h ( Unknown Unit          )
;*                            8102h ( Device Not Ready      )
;*                            8103h ( Unknown command       )
;*                            810Ch ( General Failure       )
;*                            8111h ( Char_Call_Interrupted )
;*                            8112h ( No Monitor Support    )
;*                            8113h ( Invalid Parameter     )
;*
;*********************** END OF SPECIFICATIONS **********************
_Strategy proc far
__acrtused label byte                           ; Label for linker, eliminates
                                                ; inclusion of C runtime startup
        push    es                              ; Push address of request
        push    bx                              ; packet (parm to C routine)
        call    _Strategy_c
        pop     bx                              ; Restore stack frame and
        pop     es                              ; restore pointer to request
                                                ; packet.
        mov     WORD PTR es:[bx+3],ax           ; plug in status word
        ret                                     ; Return to kernel
_Strategy endp


;***************************************
; DDCMDEntryPoint - Enables Audio stream
; handler to communicate with Audio PDD
;***************************************

_DDCMDEntryPoint proc far

        push   bp                               ; Save stack pointer
        mov    bp, sp

        push   ds                               ; Create own data segment
        mov    ax, DGROUP
        mov    ds, ax


        push   [bp+8]                           ; high address
        push   [bp+6]                           ; low address
                                                ; +4[bp] has old CS register

        call   _DDCmdInternalEntryPoint         ; C routine to process DDCMDs

        add    sp, 4                            ; toss calling address

        pop    ds                               ; restore registers ds, bp
        pop    bp
        ret

_DDCMDEntryPoint endp


_TEXT   ENDS
        END
