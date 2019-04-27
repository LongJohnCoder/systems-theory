;*++
;
;Copyright (c) Aleksey Bragin.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   ixipi.asm
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

extrn _HalpRegisterKdSupportFunctions@4:NEAR

extrn _HalpActiveProcessors:DWORD
extrn _HalpDefaultInterruptAffinity:DWORD

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name _HalInitializeProcessor
; *
; * The _HalInitializeProcessor routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalInitializeProcessor, 2
ProcessorNumber equ 4
LoaderBlock     equ 8

    ; Set default interrupt mask and stall factor
    mov PCR[PcIDR], 0FFFFFFFBh
    mov PCR[PcStallScaleFactor], INITIAL_STALL_COUNT

    ; Update interrupt affinity and active masks
    mov eax, [esp+ProcessorNumber]
    lock bts _HalpDefaultInterruptAffinity, eax
    lock bts _HalpActiveProcessors, eax

    ; Register KD functions
    stdCall _HalpRegisterKdSupportFunctions, <[esp+LoaderBlock]>

    stdRET  _HalInitializeProcessor
stdENDP _HalInitializeProcessor

;/*++
; * @name _HalRequestIpi
; *
; * The _HalRequestIpi routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalRequestIpi, 1
    ; Nothing to do here for UP
    stdRET  _HalRequestIpi
stdENDP _HalRequestIpi

_TEXT ENDS
END