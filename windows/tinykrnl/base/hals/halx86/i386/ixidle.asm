;*++
;
;Copyright (c) Aleksey Bragin.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   idle.asm
;
;Abstract:
;
; FILLME
;
;Environment:
;
;   Kernel mode
;
;Revision History:
;
;   Aleksey Bragin - Started Implementing - 14-Jun-2006
;
;--*/
.586p

include callconv.inc
include ks386.inc

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name _HalProcessorIdle
; *
; * The _HalProcessorIdle routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalProcessorIdle, 0

    ;
    ; Enable interrupts and do a halt
    ;
    sti
    hlt
    stdRET _HalProcessorIdle
stdENDP _HalProcessorIdle

_TEXT ENDS
END
