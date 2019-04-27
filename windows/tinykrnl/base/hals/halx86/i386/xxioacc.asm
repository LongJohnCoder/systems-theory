;*++
;
;Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   xxioacc.asm
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
;   Alex Ionescu - Finished Implementation - 27-Dec-06
;
;--*/
.586p

include callconv.inc
include ks386.inc

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name _READ_PORT_UCHAR
; *
; * The _READ_PORT_UCHAR routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _READ_PORT_UCHAR, 1
Port    equ 4

    ;
    ; Clear return value and read it from the port
    ;
    xor eax, eax
    mov edx, [esp+Port]
    in al, dx
    stdRET _READ_PORT_UCHAR
stdENDP _READ_PORT_UCHAR

;/*++
; * @name _READ_PORT_USHORT
; *
; * The _READ_PORT_USHORT routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _READ_PORT_USHORT, 1
Port    equ 4

    ;
    ; Clear return value and read it from the port
    ;
    xor eax, eax
    mov edx, [esp+Port]
    in ax, dx
    stdRET _READ_PORT_USHORT
stdENDP _READ_PORT_USHORT

;/*++
; * @name _READ_PORT_ULONG
; *
; * The _READ_PORT_ULONG routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _READ_PORT_ULONG, 1
Port    equ 4

    ;
    ; Clear return value and read it from the port
    ;
    mov edx, [esp+Port]
    in ax, dx
    stdRET _READ_PORT_ULONG
stdENDP _READ_PORT_ULONG

;/*++
; * @name _READ_PORT_BUFFER_UCHAR
; *
; * The _READ_PORT_BUFFER_UCHAR routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _READ_PORT_BUFFER_UCHAR, 3
Port    equ 4
Buffer  equ 8
Count   equ 0Ch

    ;
    ; Save edi
    ;
    mov eax, edi

    ;
    ; Set registers for buffer read and do it
    ;
    mov edx, [esp+Port]
    mov edi, [esp+Buffer]
    mov ecx, [esp+Count]
    rep ins BYTE PTR es:[edi], dx

    ;
    ; Restore EDI and return
    ;
    mov edi, eax
    stdRET _READ_PORT_BUFFER_UCHAR
stdENDP _READ_PORT_BUFFER_UCHAR

;/*++
; * @name _READ_PORT_BUFFER_USHORT
; *
; * The _READ_PORT_BUFFER_USHORT routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _READ_PORT_BUFFER_USHORT, 3
Port    equ 4
Buffer  equ 8
Count   equ 0Ch

    ;
    ; Save edi
    ;
    mov eax, edi

    ;
    ; Set registers for buffer read and do it
    ;
    mov edx, [esp+Port]
    mov edi, [esp+Buffer]
    mov ecx, [esp+Count]
    rep ins WORD PTR es:[edi], dx

    ;
    ; Restore EDI and return
    ;
    mov edi, eax
    stdRET _READ_PORT_BUFFER_USHORT
stdENDP _READ_PORT_BUFFER_USHORT

;/*++
; * @name _READ_PORT_BUFFER_ULONG
; *
; * The _READ_PORT_BUFFER_ULONG routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _READ_PORT_BUFFER_ULONG, 3
Port    equ 4
Buffer  equ 8
Count   equ 0Ch

    ;
    ; Save edi
    ;
    mov eax, edi

    ;
    ; Set registers for buffer read and do it
    ;
    mov edx, [esp+Port]
    mov edi, [esp+Buffer]
    mov ecx, [esp+Count]
    rep ins DWORD PTR es:[edi], dx

    ;
    ; Restore EDI and return
    ;
    mov edi, eax
    stdRET _READ_PORT_BUFFER_ULONG
stdENDP _READ_PORT_BUFFER_ULONG

;/*++
; * @name _WRITE_PORT_UCHAR
; *
; * The _WRITE_PORT_UCHAR routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRITE_PORT_UCHAR, 2
Port    equ 4
Value   equ 8

    ;
    ; Write value to target port
    ;
    mov edx, [esp+Port]
    mov al, [esp+Value]
    out dx, al
    stdRET _WRITE_PORT_UCHAR
stdENDP _WRITE_PORT_UCHAR

;/*++
; * @name _WRITE_PORT_USHORT
; *
; * The _WRITE_PORT_USHORT routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRITE_PORT_USHORT, 2
Port    equ 4
Value   equ 8

    ;
    ; Write value to target port
    ;
    mov edx, [esp+Port]
    mov eax, DWORD PTR [esp+Value]
    out dx, ax
    stdRET _WRITE_PORT_USHORT
stdENDP _WRITE_PORT_USHORT

;/*++
; * @name _WRITE_PORT_ULONG
; *
; * The _WRITE_PORT_ULONG routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRITE_PORT_ULONG, 2
Port    equ 4
Value   equ 8

    ;
    ; Write value to target port
    ;
    mov edx, [esp+Port]
    mov eax, [esp+Value]
    out dx, eax
    stdRET _WRITE_PORT_ULONG
stdENDP _WRITE_PORT_ULONG

;/*++
; * @name _HalpIoDelay
; *
; * The _HalpIoDelay routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpIoDelay, 0

    ;
    ; Do a two-cycle delay
    ;
    jmp $+2
    jmp $+2
    stdRET _HalpIoDelay
stdENDP _HalpIoDelay

;/*++
; * @name _WRITE_PORT_BUFFER_UCHAR
; *
; * The _WRITE_PORT_BUFFER_UCHAR routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRITE_PORT_BUFFER_UCHAR, 3
Port    equ 4
Buffer  equ 8
Counter equ 0Ch

    ;
    ; Save esi
    ;
    mov eax, esi

    ;
    ; Set registers for buffer write and do it
    ;
    mov edx, [esp+Port]
    mov esi, [esp+Buffer]
    mov ecx, [esp+Count]
    rep outsb

    ;
    ; Restore esi
    ;
    mov esi, eax
    stdRET _WRITE_PORT_BUFFER_UCHAR
stdENDP _WRITE_PORT_BUFFER_UCHAR

;/*++
; * @name _WRITE_PORT_BUFFER_USHORT
; *
; * The _WRITE_PORT_BUFFER_USHORT routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRITE_PORT_BUFFER_USHORT, 3
Port    equ 4
Buffer  equ 8
Counter equ 0Ch

    ;
    ; Save esi
    ;
    mov eax, esi

    ;
    ; Set registers for buffer write and do it
    ;
    mov edx, [esp+Port]
    mov esi, [esp+Buffer]
    mov ecx, [esp+Count]
    rep outsw

    ;
    ; Restore esi
    ;
    mov esi, eax
    stdRET _WRITE_PORT_BUFFER_USHORT
stdENDP _WRITE_PORT_BUFFER_USHORT

;/*++
; * @name _WRITE_PORT_BUFFER_ULONG
; *
; * The _WRITE_PORT_BUFFER_ULONG routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRITE_PORT_BUFFER_ULONG, 3
Port    equ 4
Buffer  equ 8
Counter equ 0Ch

    ;
    ; Save esi
    ;
    mov eax, esi

    ;
    ; Set registers for buffer write and do it
    ;
    mov edx, [esp+Port]
    mov esi, [esp+Buffer]
    mov ecx, [esp+Count]
    rep outsd

    ;
    ; Restore esi
    ;
    mov esi, eax
    stdRET _WRITE_PORT_BUFFER_ULONG
stdENDP _WRITE_PORT_BUFFER_ULONG

;/*++
; * @name RDMSR
; *
; * The RDMSR routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall RDMSR, 1

    ;
    ; Read the MSR and return
    ;
    rdmsr
    fstRET RDMSR
fstENDP RDMSR

;/*++
; * @name RDMSR
; *
; * The RDMSR routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _WRMSR, 3

    ;
    ; Set proper registers
    ;
    mov ecx, [esp+4]
    mov eax, [esp+8]
    mov edx, [esp+12]

    ;
    ; Write the MSR and return
    ;
    wrmsr
    stdRET _WRMSR
stdENDP _WRMSR

_TEXT ENDS
END
