;*++
;
;Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   ixsysint.asm
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
;   Aleksey Bragin - Started Implementing - 11-May-2006
;   Alex Ionescu - Cleanup and implemented HalEndSystemInterupt - 25-Nov-06
;
;--*/
.586p

include callconv.inc
include ks386.inc

extrn KiI8259MaskTable:DWORD
extern SWInterruptLookUpTable:BYTE
extern SWInterruptHandlerTable2:DWORD

_DATA SEGMENT DWORD PUBLIC 'DATA'
;
; HalpSpecialDismissTable - a special dismiss table
;
public HalpSpecialDismissTable
HalpSpecialDismissTable label dword
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss2
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss3
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss1
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4
    dd offset HalpDismiss4

_DATA ENDS

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name HalBeginSystemInterrupt
; *
; * The HalBeginSystemInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalBeginSystemInterrupt, 3
BSI_Irql    equ 4
BSI_Vector  equ 8
BSI_OldIrql equ 0Ch

    ;
    ; Get the IRQ
    ;
    movzx ebx, byte ptr [esp+BSI_Vector]
    sub ebx, PRIMARY_VECTOR_BASE

    ;
    ; Assert it
    ;
if DBG
    cmp ebx, 23h
    jbe BSI_Begin
    int 3
endif

BSI_Begin:
    ;
    ; Jump to the appropriate dismiss entry
    ;
    jmp ds:HalpSpecialDismissTable[ebx*4]

HalpDismiss3:

    ;
    ; IRQ 15, check if spurious
    ;
    mov al, 0Bh
    out 0A0h, al
    jmp $+2
    in al, 0A0h
    test al, 80h
    jnz HalpDismiss1

    ;
    ; A real cascaded interrupt, dismiss it
    ;
    mov al, 62h
    out 20h, al
    mov eax, 0
    stdRET _HalBeginSystemInterrupt

HalpDismiss2:

    ;
    ; IRQ 7, check if spurious
    ;
    mov al, 0Bh
    out 020h, al
    jmp $+2
    in al, 020h
    test al, 80h
    jnz HalpDismiss1

    ;
    ; Dismiss it
    ;
    mov eax, 0
    stdRET _HalBeginSystemInterrupt

HalpDismiss1:

    ;
    ; Save current IRQL
    ;
    mov eax, [esp+BSI_OldIrql]
    mov ecx, PCR[PcIrql]
    mov [eax], cl

    ;
    ; Set new IRQL
    ;
    movzx eax, byte ptr [esp+BSI_Irql]
    mov PCR[PcIrql], eax

    ;
    ; Get the proper mask for this level
    ;
    mov eax, KiI8259MaskTable[eax*4]
    or eax, PCR[PcIDR]

    ;
    ; Set interrupt mask (stored in eax) for 8259
    ;
    out 021h, al
    shr eax, 8
    out 0A1h, al

    ;
    ; Check to which PIC the EOI was sent
    ;
    mov eax, ebx
    cmp eax, 8
    jnb BSI_1

    ;
    ; Write mask to master PIC
    ;
    or al, 60h
    out 20h, al
    jmp BSI_2

BSI_1:

    ;
    ; Write mask to slave PIC
    ;
    mov al, 20h
    out 0A0h, al
    mov al, 62h
    out 20h, al

BSI_2:

    ;
    ; Return interrupts and return TRUE
    ;
    in al, 21h
    sti
    mov eax, 1
    stdRET _HalBeginSystemInterrupt

align 4
HalpDismiss4:

    ;
    ; Dismiss it
    ;
    mov eax, 0
    stdRET  _HalBeginSystemInterrupt
stdENDP _HalBeginSystemInterrupt

;/*++
; * @name HalEndSystemInterrupt
; *
; * The HalEndSystemInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return KIRQL.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalEndSystemInterrupt, 2
ESI_Irql    equ 4

    ;
    ; Get the IRQL
    ;
    movzx ecx, byte ptr [esp+ESI_Irql]

    ;
    ; Check if we can do a simple lower
    ;
    cmp PCR[PcIrql], DISPATCH_LEVEL
    jbe PassiveApcDispatch

    ;
    ; Get the proper mask for this level
    ;
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[PcIDR]

    ;
    ; Set interrupt mask (stored in eax) for 8259
    ;
    out 021h, al
    shr eax, 8
    out 0A1h, al

PassiveApcDispatch:
    ;
    ; Set the new IRQL
    ;
    mov PCR[PcIrql], ecx

    ;
    ; Check the software mask we need and if it's required at this level
    ;
    mov eax, PCR[PcIRR]
    mov al, SWInterruptLookUpTable[eax]
    cmp al, cl
    ja CallSwHandler

    ;
    ; No need to call a handler, return
    ;
    stdRet _HalEndSystemInterrupt

CallSwHandler:
    ;
    ; Call the software handler for this interrupt
    ;
    add esp, 12
    jmp SWInterruptHandlerTable2[eax*4]
stdENDP _HalEndSystemInterrupt

;/*++
; * @name HalpEndSoftwareInterrupt
; *
; * The HalpEndSoftwareInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return KIRQL.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpEndSoftwareInterrupt, 1
ESI_Irql    equ 4

    ;
    ; Get the IRQL
    ;
    movzx ecx, byte ptr [esp+ESI_Irql]

    ;
    ; Check if we can do a simple lower
    ;
    cmp PCR[PcIrql], DISPATCH_LEVEL
    jbe PassiveApcDispatch_ESI

    ;
    ; Get the proper mask for this level
    ;
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[PcIDR]

    ;
    ; Set interrupt mask (stored in eax) for 8259
    ;
    out 021h, al
    shr eax, 8
    out 0A1h, al

PassiveApcDispatch_ESI:
    ;
    ; Set the new IRQL
    ;
    mov PCR[PcIrql], ecx

    ;
    ; Check the software mask we need and if it's required at this level
    ;
    mov eax, PCR[PcIRR]
    mov al, SWInterruptLookUpTable[eax]
    cmp al, cl
    ja CallSwHandler_ESI

    ;
    ; No need to call a handler, return
    ;
    stdRet _HalpEndSoftwareInterrupt

CallSwHandler_ESI:
    ;
    ; Call the software handler for this interrupt
    ;
    add esp, 8
    jmp SWInterruptHandlerTable2[eax*4]
stdENDP _HalpEndSoftwareInterrupt

_TEXT ENDS
END