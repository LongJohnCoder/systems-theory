;*++
;
;Copyright (c) Aleksey Bragin.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   pmrtc.asm
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
;   Alex Ionescu - Started Implementing - 23-Nov-2006
;
;--*/
.586p

include callconv.inc
include ks386.inc
include acpi_mp.inc

extrn _HalpFixedAcpiDescTable:DWORD
extrn _HalpCmosCenturyOffset:DWORD

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name HalpInitializeCmos
; *
; * The HalpInitializeCmos routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpInitializeCmos, 0

    ;
    ; Set the century offset from the FADT
    ;
    movzx eax, byte ptr [_HalpFixedAcpiDescTable+FaCenturyAlarmIndex]
    or al, al
    jnz SetOffset

    ;
    ; Couldn't find one, use the default
    ;
    mov eax, 32h

    ;
    ; Save the offset
    ;
SetOffset:
    mov _HalpCmosCenturyOffset, eax
    stdRET _HalpInitializeCmos
stdENDP _HalpInitializeCmos

_TEXT ENDS
END

