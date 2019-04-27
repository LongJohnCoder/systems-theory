;*++
;
;Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   ixirql.asm
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
;   Aleksey Bragin - Started Implementing - 09-May-2006
;   Alex Ionescu - Fixed implementation of most functions- 23-Nov-2006
;
;--*/
.586p

include callconv.inc
include ks386.inc

EXTRNP _KeBugCheckEx, 5, IMPORT
if DBG
EXTRNP _Kii386SpinOnSpinLock, 2, IMPORT
endif
extrn _KiUnexpectedInterrupt:NEAR
extrn _HalpApcInterrupt:NEAR
extrn _HalpApcInterrupt2ndEntry:NEAR
extrn _HalpDispatchInterrupt:NEAR
extrn _HalpDispatchInterrupt2ndEntry:NEAR
extrn _HalpBusType:DWORD

_DATA SEGMENT DWORD PUBLIC 'DATA'

;
; KiI8259MaskTable - mask table for all Irqls
;
public KiI8259MaskTable
KiI8259MaskTable label dword
    dd 000000000h ; 0
    dd 000000000h ; 1
    dd 000000000h ; 2
    dd 000000000h ; 3
    dd 0FF800000h ; 4
    dd 0FFC00000h ; 5
    dd 0FFE00000h ; 6
    dd 0FFF00000h ; 7
    dd 0FFF80000h ; 8
    dd 0FFFC0000h ; 9
    dd 0FFFE0000h ; 10
    dd 0FFFF0000h ; 11
    dd 0FFFF8000h ; 12
    dd 0FFFFC000h ; 13
    dd 0FFFFE000h ; 14
    dd 0FFFFF000h ; 15
    dd 0FFFFF800h ; 16
    dd 0FFFFFC00h ; 17
    dd 0FFFFFE00h ; 18
    dd 0FFFFFE00h ; 19
    dd 0FFFFFE80h ; 20
    dd 0FFFFFEC0h ; 21
    dd 0FFFFFEE0h ; 22
    dd 0FFFFFEF0h ; 23
    dd 0FFFFFEF8h ; 24
    dd 0FFFFFEF8h ; 25
    dd 0FFFFFEFAh ; 26
    dd 0FFFFFFFAh ; 27
    dd 0FFFFFFFBh ; 28
    dd 0FFFFFFFBh ; 29
    dd 0FFFFFFFBh ; 30
    dd 0FFFFFFFBh ; 31

;
; FIXME: Convert these to better representation with defines
;
PS2PICsInitializationString:
    dw 020h
    db 019h
    db 030h
    db 004h
    db 001h
    dw 0A0h
    db 019h
    db 038h
    db 002h
    db 001h
    dw 000h

PICsInitializationString:
    dw 020h
    db 011h
    db 030h
    db 004h
    db 001h
    dw 0A0h
    db 011h
    db 038h
    db 002h
    db 001h
    dw 000h

public SWInterruptLookUpTable
SWInterruptLookUpTable label byte
    db 0
    db 0
    db 1
    db 1
    db 2
    db 2
    db 2
    db 2

public SWInterruptHandlerTable
SWInterruptHandlerTable label dword
    dd offset _KiUnexpectedInterrupt
    dd offset _HalpApcInterrupt
    dd offset _HalpDispatchInterrupt

public SWInterruptHandlerTable2
SWInterruptHandlerTable2 label dword
    dd offset _KiUnexpectedInterrupt
    dd offset _HalpApcInterrupt2ndEntry
    dd offset _HalpDispatchInterrupt2ndEntry

_DATA ENDS

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name KfRaiseIrql
; *
; * The KfRaiseIrql routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KfRaiseIrql, 1

    ;
    ; Get current IRQL
    ;
    mov eax, PCR[PcIrql]

    ;
    ; Get requested IRQL and compare
    ;
    movzx ecx, cl
    cmp eax, ecx
    ja IrqlHigher

    ;
    ; Check if we can do a simple raise
    ;
    cmp ecx, DISPATCH_LEVEL
    jbe PassiveApcDispatch

    ;
    ; Otherwise, save IRQL in EDX and disable interrupts
    ;
    mov edx, eax
    pushfd
    cli

    ;
    ; Set the new IRQL and get the proper mask for this level
    ;
    mov PCR[PcIrql], cl
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[PcIDR]

    ;
    ; Set interrupt mask (stored in eax) for 8259
    ;
    out 021h, al
    shr eax, 8
    out 0A1h, al

    ;
    ; Restore interrupts and return the previous IRQL
    ;
    popfd
    mov eax, edx
    fstRET KfRaiseIrql

PassiveApcDispatch:
    ;
    ; If it's raising to an irql not higher than
    ; DISPATCH, then it's just pretty simple
    ;
    mov PCR[PcIrql], ecx
    fstRET KfRaiseIrql

IrqlHigher:
    ;
    ; Set Irql to PASSIVE_LEVEL and bugcheck
    ;
    mov PCR[PcIrql], PASSIVE_LEVEL
    stdCall _KeBugCheckEx, <IRQL_NOT_GREATER_OR_EQUAL, eax, ecx, 0, 9>
    fstRET KfRaiseIrql
fstENDP KfRaiseIrql

;/*++
; * @name KfLowerIrql
; *
; * The KfLowerIrql routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KfLowerIrql, 1

    ;
    ; Save flags to kill interrupts later
    ;
    pushfd

    ;
    ; Verify IRQL
    ;
    movzx ecx, cl
    cmp ecx, PCR[PcIrql]
    ja IrqlHigher_L

    ;
    ; Check if we can do a simple raise and kill interrupts
    ;
    cmp PCR[PcIrql], DISPATCH_LEVEL
    cli
    jbe PassiveApcDispatch_L

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

PassiveApcDispatch_L:
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
    popfd
    fstRET KfLowerIrql

IrqlHigher_L:
    ;
    ; Set Irql to HIGH_LEVEL and bugcheck
    ;
    mov eax, PCR[PcIrql]
    mov PCR[PcIrql], HIGH_LEVEL
    stdCall _KeBugCheckEx, <IRQL_NOT_LESS_OR_EQUAL, eax, ecx, 0, 3>

CallSwHandler:
    ;
    ; Call the software handler for this interrupt and restore interrupts
    ;
    call SWInterruptHandlerTable[eax*4]
    popfd
    fstRET KfLowerIrql
fstENDP KfLowerIrql

;/*++
; * @name KeGetCurrentIrql
; *
; * The KeGetCurrentIrql routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _KeGetCurrentIrql, 0

    ;
    ; Save Irql from PCR into eax and return
    ;
    mov eax, PCR[PcIrql]
    stdRET _KeGetCurrentIrql
stdENDP _KeGetCurrentIrql

;/*++
; * @name _HalpInitializePICs
; *
; * The _HalpInitializePICs routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpInitializePICs, 1

    ;
    ; Save esi and flags in the stack and forbid all interrupts
    ;
    push esi
    pushf
    cli

    ;
    ; Load PICs initialization string address into esi
    ;
    lea esi, PICsInitializationString

    ;
    ; Check bus type for EISA
    ;
    test _HalpBusType, MACHINE_TYPE_MCA
    jz BusType2

    ;
    ; FIXME: TODO
    ;
    mov eax, 1234h
    jmp $

BusType2:
    ; Load Port Address
    lodsw

SendString:
    ;
    ; Send ICW1 and wait
    ;
    movzx edx, ax
    outsb
    jmp $+2

    ;
    ; Send ICW2 and wait
    ;
    inc edx
    outsb
    jmp $+2

    ;
    ; Send ICW3 and wait
    ;
    outsb
    jmp $+2

    ;
    ; Send ICW4 and wait
    ;
    outsb
    jmp $+2

    ;
    ; Mask out all 8259 irqs
    ;
    mov al, 0FFh
    out dx, al

    ;
    ; Check if we reached the end of our initialization string
    ;
    lodsw
    cmp ax, 0
    jnz SendString

    ;
    ; Check if we have to force interrupts on
    ;
    mov al, [esp+0Ch]
    or al, al
    jz DontEnable
    or dword ptr [esp], EFLAGS_INTERRUPT_MASK

DontEnable:
    popf
    pop esi
    stdRET _HalpInitializePICs
stdENDP _HalpInitializePICs

;/*++
; * @name KfAcquireSpinLock
; *
; * The KfAcquireSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KfAcquireSpinLock, 1

    ;
    ; Simply raise Irql to dispatch (UP code only)
    ;
    mov eax, PCR[PcIrql]
    mov PCR[PcIrql], DISPATCH_LEVEL
    fstRET KfAcquireSpinLock
fstENDP KfAcquireSpinLock

;/*++
; * @name KfReleaseSpinLock
; *
; * The KfReleaseSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KfReleaseSpinLock, 2

    ;
    ; Save flags to kill interrupts later
    ;
    pushfd

    ;
    ; Put the IRQL in ECX
    ;
    movzx ecx, dl

    ;
    ; FIXME: Verify IRQL
    ;

    ;
    ; Check if we can do a simple lower and kill interrupts
    ;
    cmp PCR[PcIrql], DISPATCH_LEVEL
    cli
    jbe PassiveApcDispatch_RS

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

PassiveApcDispatch_RS:
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
    ja CallSwHandler_RS

    ;
    ; No need to call a handler, return
    ;
    popfd
    fstRET KfReleaseSpinLock

CallSwHandler_RS:
    ;
    ; Call the software handler for this interrupt and restore interrupts
    ;
    call SWInterruptHandlerTable[eax*4]
    popfd
    fstRET KfReleaseSpinLock
fstENDP KfReleaseSpinLock

;/*++
; * @name KeAcquireQueuedSpinLock
; *
; * The KeAcquireQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeAcquireQueuedSpinLock, 1

    ;
    ; Set IRQL to dispatch (UP code only)
    ;
    mov eax, PCR[PcIrql]
    mov PCR[PcIrql], DISPATCH_LEVEL
    fstRET KeAcquireQueuedSpinLock
fstENDP KeAcquireQueuedSpinLock

;/*++
; * @name KeAcquireInStackQueuedSpinLock
; *
; * The KeAcquireInStackQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeAcquireInStackQueuedSpinLock, 2

    ;
    ; Set IRQL to dispatch (UP code only)
    ;
    push DISPATCH_LEVEL

AcquireStackQSL:
    ;
    ; Update the IRQL
    ;
    pop ecx
    push edx
    call @KfRaiseIrql@4

    ;
    ; Update the Queued Spin Lock
    ;
    pop edx
    mov [edx+LqhOldIrql], al
    fstRET KeAcquireInStackQueuedSpinLock
fstENDP KeAcquireInStackQueuedSpinLock

;/*++
; * @name KeAcquireInStackQueuedSpinLockRaiseToSynch
; *
; * The KeAcquireInStackQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeAcquireInStackQueuedSpinLockRaiseToSynch, 2

    ;
    ; Use DISPATCH_LEVEL as IRQL (UP Code only)
    ;
    push DISPATCH_LEVEL
    jmp AcquireStackQSL
fstENDP KeAcquireInStackQueuedSpinLockRaiseToSynch

;/*++
; * @name KeAcquireQueuedSpinLockRaiseToSynch
; *
; * The KeAcquireQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeAcquireQueuedSpinLockRaiseToSynch, 2

    ;
    ; Use DISPATCH_LEVEL for IRQL (Up code only)
    ;
    push ecx
    mov ecx, DISPATCH_LEVEL
    call @KfRaiseIrql@4
    pop ecx
    fstRET KeAcquireQueuedSpinLockRaiseToSynch
fstENDP KeAcquireQueuedSpinLockRaiseToSynch

;/*++
; * @name KeAcquireSpinLockRaiseToSynch
; *
; * The KeAcquireSpinLockRaiseToSynch routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeAcquireSpinLockRaiseToSynch, 1

    ;
    ; Simply raise IRQL to dispatch (UP code only)
    ;
    push ecx
    mov ecx, DISPATCH_LEVEL
    call @KfRaiseIrql@4
    pop ecx
    fstRET KeAcquireSpinLockRaiseToSynch
fstENDP KeAcquireSpinLockRaiseToSynch

;/*++
; * @name KeReleaseQueuedSpinLock
; *
; * The KeReleaseQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeReleaseQueuedSpinLock, 2

    ;
    ; Just lower IRQL
    ;
    movzx ecx, dl
    jmp @KfLowerIrql@4
fstENDP KeReleaseQueuedSpinLock

;/*++
; * @name KeReleaseInStackQueuedSpinLock
; *
; * The KeReleaseInStackQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeReleaseInStackQueuedSpinLock, 1

    ;
    ; Just lower IRQL
    ;
    movzx ecx, byte ptr [ecx+LqhOldIrql]
    jmp @KfLowerIrql@4
fstENDP KeReleaseInStackQueuedSpinLock

;/*++
; * @name KeTryToAcquireQueuedSpinLockRaiseToSynch
; *
; * The KeTryToAcquireQueuedSpinLockRaiseToSynch routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeTryToAcquireQueuedSpinLockRaiseToSynch, 1

    ;
    ; Use DISPATCH_LEVEL for IRQL (UP Code only)
    ;
    push edx
    push DISPATCH_LEVEL
    jmp UpdateQsl
fstENDP KeTryToAcquireQueuedSpinLockRaiseToSynch

;/*++
; * @name KeTryToAcquireQueuedSpinLock
; *
; * The KeTryToAcquireQueuedSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicFastCall KeTryToAcquireQueuedSpinLock, 2

    ;
    ; Set IRQL to dispatch (UP code only)
    ;
    push edx
    push DISPATCH_LEVEL

UpdateQsl:
    ;
    ; Update the IRQL
    ;
    pop ecx
    call @KfRaiseIrql@4
    pop edx

    ;
    ; Update the Queued Spin Lock and return TRUE
    ;
    mov [edx], al
    xor eax, eax
    or eax, 1
    fstRET KeTryToAcquireQueuedSpinLock
fstENDP KeTryToAcquireQueuedSpinLock

;/*++
; * @name KeRaiseIrqlToDpcLevel
; *
; * The KeRaiseIrqlToDpcLevel routine FILLMEIN
; *
; * @param None.
; *
; * @return KIRQL.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _KeRaiseIrqlToDpcLevel, 0

    ;
    ; Simply raise IRQL to dispatch (UP code only)
    ;
    mov eax, PCR[PcIrql]
    mov PCR[PcIrql], DISPATCH_LEVEL
    stdRET _KeRaiseIrqlToDpcLevel
stdENDP _KeRaiseIrqlToDpcLevel

;/*++
; * @name KeRaiseIrqlToSynchLevel
; *
; * The KeRaiseIrqlToSynchLevel routine FILLMEIN
; *
; * @param None.
; *
; * @return KIRQL.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _KeRaiseIrqlToSynchLevel, 0

    ;
    ; Simply raise IRQL to dispatch (UP code only)
    ;
    mov eax, PCR[PcIrql]
    mov PCR[PcIrql], DISPATCH_LEVEL
    stdRET _KeRaiseIrqlToSynchLevel
stdENDP _KeRaiseIrqlToSynchLevel

_TEXT ENDS
END