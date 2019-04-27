;*++
;
;Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   ixswint.asm
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
;   Alex Ionescu - Re-implemented and cleanedup - 25-Nov-06
;
;--*/
.586p

include callconv.inc
include ks386.inc
include kimacro.inc

EXTRNP _HalpEndSoftwareInterrupt, 1
EXTRNP _KiDispatchInterrupt, 0, IMPORT
EXTRNP _KiDeliverApc, 3, IMPORT
EXTRNP _HalBeginSystemInterrupt, 3
EXTRNP Kei386EoiHelper, 0, IMPORT
EXTRNP _KeSetTimeIncrement, 2, IMPORT
EXTRNP _KeUpdateSystemTime, 0
extrn SWInterruptLookUpTable:BYTE
extrn SWInterruptHandlerTable:DWORD
extern KiI8259MaskTable:DWORD
extern _HalpCurrentRollOver:DWORD
extern _HalpCurrentTimeIncrement:DWORD
extern _HalpClockSetMSRate:DWORD

_DATA SEGMENT DWORD PUBLIC 'DATA'

public HalpPerfCounterLow, HalpPerfCounterHigh
public HalpLastPerfCounterLow, HalpLastPerfCounterHigh
HalpPerfCounterLow dd 0
HalpPerfCounterHigh dd 0
HalpLastPerfCounterLow dd 0
HalpLastPerfCounterHigh dd 0

_DATA ends

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name HalDisableSystemInterrupt
; *
; * The HalDisableSystemInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalDisableSystemInterrupt, 2
DSI_Vector equ 4

    ;
    ; Get the IRQ
    ;
    movzx ecx, byte ptr [esp+DSI_Vector]
    sub ecx, PRIMARY_VECTOR_BASE

    ;
    ; Generate mask and disable interrupts
    ;
    mov edx, 1
    shl edx, cl
    cli

    ;
    ; Set the new mask in the IDR
    ;
    or PCR[PcIDR], edx

    ;
    ; Get the current mask
    ;
    xor eax, eax
    in al, 0A1h
    shl eax, 8
    in al, 21h

    ;
    ; Mask this interrupt off and write the new mask
    ;
    or eax, edx
    out 21h, al
    shr eax, 8
    out 0A1h, al

    ;
    ; Re-eanble interrupts and return
    ;
    in al, 0A1h
    sti
    stdRET _HalDisableSystemInterrupt
stdENDP _HalDisableSystemInterrupt

;/*++
; * @name HalDisableSystemInterrupt
; *
; * The HalDisableSystemInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalEnableSystemInterrupt, 3
ESI_Vector equ 4
ESI_Latched equ 12

    ;
    ; Get the vector and validate it
    ;
    movzx ecx, byte ptr [esp+ESI_Vector]
    sub ecx, PRIMARY_VECTOR_BASE
    jb Invalid
    cmp ecx, CLOCK2_LEVEL
    jnb Invalid

    ;
    ; Get the current PCI Edge/Level control registers
    ;
    mov edx, 4D1h
    in al, dx
    shl ax, 8
    mov eax, 4D0h
    in al, dx
    mov dx, 1
    shl dx, cl

    ;
    ; Check if this is a latched interrupt
    ;
    cmp dword ptr [esp+ESI_Latched], 0
    jnz Latched

    ;
    ; Use OR for edge interrupt
    ;
    or ax, dx
    jmp AfterMask

Latched:

    ;
    ; Mask it out for level interrupt
    ;
    not dx
    and ax, dx

AfterMask:

    ;
    ; Set the PCI Edge/Level control registers
    ;
    mov edx, 4D0h
    out dx, al
    shr ax, 8
    mov edx, 4D1h
    out dx, al

    ;
    ; Calculate the new IDR
    ;
    mov eax, 1
    shl eax, cl
    not eax

    ;
    ; Disable interrupts and set new IDR
    ;
    cli
    and PCR[PcIDR], eax

    ;
    ; Get the current IRQL and mask the IRQs in the PIC
    ;
    movzx eax, byte ptr PCR[PcIrql]
    mov eax, KiI8259MaskTable[eax*4]
    or eax, PCR[PcIDR]
    out 21h, al
    shr eax, 8
    out 0A1h, al

    ;
    ; Enable interrupts and return TRUE
    ;
    sti
    mov eax, 1
    stdRET _HalEnableSystemInterrupt

Invalid:

    ;
    ; Fail, invalid IRQ
    ;
    xor eax, eax
    stdRET _HalEnableSystemInterrupt
stdENDP _HalEnableSystemInterrupt

;/*++
; * @name HalRequestSoftwareInterrupt
; *
; * The HalRequestSoftwareInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall HalRequestSoftwareInterrupt, 1

    ;
    ; Create a mask from the passed Irql (in cl)
    ;
    mov eax, 1
    shl eax, cl

    ;
    ; Store flags and disable interrupts
    ;
    pushfd
    cli

    ;
    ; Request the sw interrupt (by ORing the mask with PcIRR)
    ;
    or PCR[PcIRR], eax

    ;
    ; Get Irql into ecx and updated IRR in eax
    ;
    mov ecx, PCR[PcIrql]
    mov eax, PCR[PcIRR]

    ;
    ; Mask off hw interrupts
    ;
    and eax, 0111b

    ;
    ; Get the sw interrupt
    ;
    xor edx, edx
    mov dl, SWInterruptLookUpTable[eax]

    ;
    ; Check if it's >= Irql, if yes - just return
    ;
    cmp dl, cl
    jbe irql_higher

    ;
    ; Call appropriate sw interrupt's handler
    ;
    call SWInterruptHandlerTable[edx*4]

irql_higher:
    ;
    ; Restore flags and return
    ;
    popfd
    fstRET HalRequestSoftwareInterrupt
fstENDP HalRequestSoftwareInterrupt

;/*++
; * @name HalClearSoftwareInterrupt
; *
; * The HalClearSoftwareInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall HalClearSoftwareInterrupt, 1

    ;
    ; We need to create a mask from the passed Irql (in cl)
    ; To do this, we shift the bit (eax) to the left by cl
    ;
    mov eax, 1
    shl eax, cl

    ;
    ; Then we must clear the that bit in PcIRR: invert eax and & it with PcIRR
    ;
    not eax
    and PCR[PcIRR], eax
    fstRET HalClearSoftwareInterrupt
fstENDP HalClearSoftwareInterrupt

;/*++
; * @name HalpDispatchInterrupt
; *
; * The HalpDispatchInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
DR_TRAP_FIXUP hdpi_a, hdpi_t
V86_TRAP_FIXUP hdpi_a, hdpi_t
public _HalpDispatchInterrupt
_HalpDispatchInterrupt proc

    ;
    ; Create fake interrupt stack
    ;
    pop eax
    pushfd
    push cs
    push eax

    ;
    ; Do interrupt prolog
    ;
    INT_PROLOG hdpi_a, hdpi_t

public _HalpDispatchInterrupt2ndEntry
_HalpDispatchInterrupt2ndEntry:

    ;
    ; Save IRQL and change to dispatch
    ;
    push PCR[PcIrql]
    mov byte ptr PCR[PcIrql], DISPATCH_LEVEL

    ;
    ; Update IRR and enable interrupts
    ;
    and dword ptr PCR[PcIRR], not (1 shl DISPATCH_LEVEL)
    sti

    ;
    ; Cal the DPC handler
    ;
    mov ecx, ebp
    stdCall _KiDispatchInterrupt

    ;
    ; Exit the interrupt
    ;
    cli
    call _HalpEndSoftwareInterrupt@4
    jmp dword ptr [__imp_Kei386EoiHelper@0]
_HalpDispatchInterrupt endp

;/*++
; * @name HalpApcInterrupt
; *
; * The HalpApcInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
DR_TRAP_FIXUP hapc_a, hapc_t
V86_TRAP_FIXUP hapc_a, hapc_t
public _HalpApcInterrupt
_HalpApcInterrupt proc

    ;
    ; Create fake interrupt stack
    ;
    pop eax
    pushfd
    push cs
    push eax

    ;
    ; Do interrupt prolog
    ;
    INT_PROLOG hapc_a, hapc_t

public _HalpApcInterrupt2ndEntry
_HalpApcInterrupt2ndEntry:

    ;
    ; Save IRQL and change to APC
    ;
    push PCR[PcIrql]
    mov byte ptr PCR[PcIrql], APC_LEVEL

    ;
    ; Update IRR and enable interrupts
    ;
    and dword ptr PCR[PcIRR], not (1 shl APC_LEVEL)
    sti

    ;
    ; Check if we came from kernel mode
    ;
    mov eax, [ebp+TsSegCs]
    and eax, MODE_MASK

    ;
    ; Test if we came from V86 mode and fake user-mode if so
    ;
    test dword ptr [ebp+TsEFlags], EFLAGS_V86_MASK
    jz @f
    or eax, MODE_MASK

    ;
    ; Call the APC handler
    ;
@@:
    stdCall _KiDeliverApc, <eax, 0, ebp>

    ;
    ; Exit the interrupt
    ;
    cli
    call _HalpEndSoftwareInterrupt@4
    jmp dword ptr [__imp_Kei386EoiHelper@0]
_HalpApcInterrupt endp

;/*++
; * @name HalpClockInterrupt
; *
; * The HalpClockInterrupt routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
DR_TRAP_FIXUP hci_a, hci_t
V86_TRAP_FIXUP hci_a, hci_t
cPublicProc _HalpClockInterrupt, 0
    ;
    ; Enter trap
    ;
    INT_PROLOG hci_a, hci_t

    ;
    ; Push vector and make stack for IRQL
    ;
    push PRIMARY_VECTOR_BASE
    sub esp, 4

    ;
    ; Begin the interrupt
    ;
    stdCall _HalBeginSystemInterrupt, <CLOCK2_LEVEL, PRIMARY_VECTOR_BASE, esp>

    ;
    ; Check if it's spurious
    ;
    or al, al
    jz Spurious

    ;
    ; Update the performance counter
    ;
    xor ebx, ebx
    mov eax, _HalpCurrentRollOver
    add HalpPerfCounterLow, eax
    adc HalpPerfCounterHigh, ebx

    ;
    ; Get the time increment and check if someone changed the clock rate
    ;
    mov eax, _HalpCurrentTimeIncrement
    cmp _HalpClockSetMSRate, ebx
    jz _KeUpdateSystemTime@0

    ;
    ; FIXME: Someone did!
    ;
    mov eax, 0924Ah
    jmp $

Spurious:
    ;
    ; Exit the interrupt
    ;
    add esp, 8
    jmp dword ptr [__imp_Kei386EoiHelper@0]
_HalpClockInterrupt endp
_TEXT ENDS
END
