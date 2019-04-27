;*++
;
;Copyright (c) Alex Ionescu.  All rights reserved.
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

EXTRNP _KeBugCheckEx, 5, IMPORT
EXTRNP _DbgBreakPoint, 0, IMPORT
extern _HalpSystemHardwareLock:DWORD

public _HalpRebootNow, _HalpCmosCenturyOffset

_DATA SEGMENT DWORD PUBLIC 'DATA'

_HalpHardwareLockFlags dd 0
_HalpRebootNow dd 0
_HalpCmosCenturyOffset dd ?

_DATA ENDS

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;/*++
; * @name HalpAcquireCmosSpinLock
; *
; * The HalpAcquireCmosSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpAcquireSystemHardwareSpinLock, 0

    ;
    ; Save EAX
    ;
    push eax

AcquireCmosLock:

    ;
    ; Disable interrupts
    ;
    pushfd
    cli

    ;
    ; Acquire the hardware lock
    ;
    lea eax, _HalpSystemHardwareLock
    lock bts [eax], 0
    jb GotCmosLock

    ;
    ; Save lock owner
    ;
    push edi
    mov edi, PCR[PcPrcb]
    mov edi, [edi+PbCurrentThread]
    or edi, 1
    mov [eax], edi
    pop edi

    ;
    ; Save the flags for release and restore eax
    ;
    pop _HalpHardwareLockFlags
    pop eax
    stdRET _HalpAcquireSystemHardwareSpinLock

GotCmosLock:

    ;
    ; Restore interrupts
    ;
    popfd

CheckSpinLock:

    ;
    ; Tell the CPU we're spinning
    ;
    pause

ifndef NT_UP ; This is only possible on SMP, if another thread requested reboot
    ;
    ; Check if we are due for a reboot
    ;
    cmp _HalpRebootNow, 0
    jnz DoReboot
endif

    ;
    ; Check the spinlock value
    ;
    cmp [eax], 0
    jnz CheckSpinLock
    jmp AcquireCmosLock

ifndef NT_UP ; This is only possible on SMP, if another thread requested reboot
DoReboot:
    ;
    ; Reboot the machine
    ;
    mov eax, _HalpRebootNow
    call eax
    int 3
endif
stdENDP _HalpAcquireSystemHardwareSpinLock

;/*++
; * @name HalpReleaseCmosSpinLock
; *
; * The HalpReleaseCmosSpinLock routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpReleaseCmosSpinLock, 0

    ;
    ; Save EAX
    ;
    push eax

    ;
    ; Restore EFLAGS on stack
    ;
    push _HalpHardwareLockFlags

    ;
    ; Get lock address
    ;
    lea eax, _HalpSystemHardwareLock

    ;
    ; Make sure this is the same lock owner
    ;
    push edi
    mov edi, PCR[PcPrcb]
    mov edi, [edi+PbCurrentThread]
    or edi, 1
    cmp edi, [eax]
    pop edi
    jz CmosOwnerOk

    ;
    ; Invalid release
    ;
    stdCall _KeBugCheckEx, <SPIN_LOCK_NOT_OWNED, eax, 0, 0, 0>

CmosOwnerOk:
    ;
    ; Release lock
    ;
    lock and dword ptr [eax], 0

    ;
    ; Restore flags (and interrupts)
    ;
    popfd 
    pop eax
    stdRET _HalpReleaseCmosSpinLock
stdENDP _HalpReleaseCmosSpinLock

;/*++
; * @name HalpFlushTLB
; *
; * The HalpFlushTLB routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpFlushTLB, 0

    ;
    ; Save flags and volatiles, disable interrupts
    ;
    pushfd
    push ebx
    push esi
    cli

    ;
    ; Save current Page Directory Base
    ;
    mov esi, cr3

    ;
    ; Check if we support CPUID
    ;
    mov ecx, PCR[PcPrcb]
    cmp byte ptr [ecx].PbCpuID, 0
    jz short ResetCr3

    ;
    ; Get feature bits
    ;
    mov eax, 1
    cpuid

    ;
    ; Check if global flush is supported
    ;
    test edx, 2000h
    jz short ResetCr3

    ;
    ; Do a global fush by disabling global bit in CR4
    ;
    mov ecx, cr4
    mov edx, ecx
    and ecx, not CR4_PGE
    mov cr4, ecx

    ;
    ; Restore CR3 to flush
    ;
    mov cr3, esi

    ;
    ; Now restore CR4 (With the global bit if it was there)
    ;
    mov cr4, edx
    jmp short Return

ResetCr3:
    ;
    ; Restore CR3
    ;
    mov cr3, esi

Return:
    ;
    ; Restore volatiles and interrupts
    ;
    pop esi
    pop ebx
    popfd
    stdRET _HalpFlushTLB
stdENDP _HalpFlushTLB

;/*++
; * @name HalpCpuID
; *
; * The HalpCpuID routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpCpuID, 5

    ;
    ; Save Volatiles
    ;
    push ebx
    push esi

    ;
    ; Do requested CPUID
    ;
    mov eax, [esp+12]
    cpuid

    ;
    ; Return CPUID Register Data
    ;
    mov esi, [esp+16]
    mov [esi], eax
    mov esi, [esp+20]
    mov [esi], ebx
    mov esi, [esp+24]
    mov [esi], ecx
    mov esi, [esp+28]
    mov [esi], edx

    ;
    ; Restore volatiles
    ;
    pop esi
    pop ebx
    stdRET _HalpCpuID
stdENDP _HalpCpuID

;/*++
; * @name HalpReadCmosTime
; *
; * The HalpReadCmosTime routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpGetCmosCenturyByte, 0

    ;
    ; Check if we can directly write the offset
    ;
    mov eax, _HalpCmosCenturyOffset
    test eax, 100h
    jnz @f

    ;
    ; Read century and return
    ;
    mov ah, [esp+4]
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    stdRET _HalpGetCmosCenturyByte

@@:
    ;
    ; Read register A
    ;
    mov edx, eax
    mov al, 0Ah
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2

    ;
    ; Set update byte
    ;
    mov dh, al
    or al, 10h
    mov ah, al
    mov al, 0Ah
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Read century offset
    ;
    mov al, dl
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    mov dl, al

    ;
    ; Restore register A
    ;
    mov ah, dh
    mov al, 0Ah
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Return offset
    ;
    mov al, dl
    stdRET _HalpGetCmosCenturyByte
stdENDP _HalpGetCmosCenturyByte

;/*++
; * @name HalpReadCmosTime
; *
; * The HalpReadCmosTime routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpSetCmosCenturyByte, 1

    ;
    ; Check if we can directly write the offset
    ;
    mov eax, _HalpCmosCenturyOffset
    test eax, 100h
    jnz @f

    ;
    ; Update century and return
    ;
    mov ah, [esp+4]
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2
    stdRET _HalpSetCmosCenturyByte

@@:
    ;
    ; Read register A
    ;
    mov edx, eax
    mov al, 0Ah
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2

    ;
    ; Set update byte
    ;
    mov dh, al
    or al, 10h
    mov ah, al
    mov al, 0Ah
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Write century
    ;
    mov ah, [esp+4]
    mov al, dl
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Restore register A
    ;
    mov ah, dh
    mov al, 0Ah
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2
    stdRET _HalpSetCmosCenturyByte
stdENDP _HalpSetCmosCenturyByte

;/*++
; * @name HalpReadCmosTime
; *
; * The HalpReadCmosTime routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpReadCmosTime, 1

    ;
    ; Set retry count
    ;
    mov ecx, 100

AcquireLock:

    ;
    ; Acquire the CMOS lock
    ;
    push ecx
    stdCall _HalpAcquireSystemHardwareSpinLock
    mov ecx, 100

CmosRead:
    ;
    ; Read register A
    ;
    mov al, 0Ah
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2

    ;
    ; Check if an update is in progress
    ;
    test al, 80h
    jz NoUpdate
    loop CmosRead

    ;
    ; Couldn't get CMOS, release the lock
    ;
    stdCall _HalpReleaseCmosSpinLock

    ;
    ; Do a breakpoint since this scenario is unlikely
    ;
    pop ecx
    loop AcquireLock
    stdCall _DbgBreakPoint
    jmp short _HalpReadCmosTime@4

NoUpdate:
    ;
    ; Pop the retry count off the stack
    ;
    pop ecx

    ;
    ; Get TIME_FIELDS and clear EAX
    ;
    mov edx, [esp+4]
    xor eax, eax

    ;
    ; Set 500 milliseconds
    ;
    mov [edx+TfMilliseconds], 500

    ;
    ; Read seconds
    ;
    mov al, 0
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    mov [edx+TfSecond], ax

    ;
    ; Read minutes
    ;
    mov al, 2
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    mov [edx+TfMinute], ax

    ;
    ; Read hours
    ;
    mov al, 4
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    mov [edx+TfHour], ax

    ;
    ; Read days
    ;
    mov al, 6
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    mov [edx+TfWeekday], ax

    ;
    ; Read weeks
    ;
    mov al, 7
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    mov [edx+TfDay], ax

    ;
    ; Read months
    ;
    mov al, 8
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    mov [edx+TfMonth], ax

    ;
    ; Read years
    ;
    mov al, 9
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad

    ;
    ; Save years and TIME_FIELDS
    ;
    push eax
    push edx

    ;
    ; Get century
    ;
    call _HalpGetCmosCenturyByte
    xor ah, ah
    rol ax, 4
    ror al, 4
    aad
    pop edx

    ;
    ; Multiply century by 100 and calculate full year
    ;
    mov ah, 100
    mul ah
    pop ecx
    add ax, cx

    ;
    ; Check if the year is after 1900 and before 1920
    ;
    cmp ax, 1900
    jb NoAdd
    cmp ax, 1920
    jae NoAdd

    ;
    ; It is, so add 100 years
    ;
    add ax, 100

NoAdd:
    ;
    ; Set years
    ;
    mov [edx+TfYear], ax

    ;
    ; Release the lock and return
    ;
    stdCall _HalpReleaseCmosSpinLock
    stdRET _HalpReadCmosTime
stdENDP _HalpReadCmosTime

;/*++
; * @name HalpReadCmosTime
; *
; * The HalpReadCmosTime routine FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this routine needs to be completed.
; *
; *--*/
cPublicProc _HalpWriteCmosTime, 1

    ;
    ; Set retry count
    ;
    mov ecx, 100

AcquireLockW:

    ;
    ; Acquire the CMOS lock
    ;
    push ecx
    stdCall _HalpAcquireSystemHardwareSpinLock
    mov ecx, 100

CmosReadW:
    ;
    ; Read register A
    ;
    mov al, 0Ah
    out 70h, al
    jmp $+2
    in al, 71h
    jmp $+2

    ;
    ; Check if an update is in progress
    ;
    test al, 80h
    jz NoUpdateW
    loop CmosReadW

    ;
    ; Couldn't get CMOS, release the lock
    ;
    stdCall _HalpReleaseCmosSpinLock

    ;
    ; Do a breakpoint since this scenario is unlikely
    ;
    pop ecx
    loop AcquireLockW
    stdCall _DbgBreakPoint
    jmp short _HalpWriteCmosTime@4

NoUpdateW:
    ;
    ; Pop the retry count off the stack and get TIME_FIELDS
    ;
    pop ecx
    mov edx, [esp+4]

    ;
    ; Write seconds
    ;
    mov al, [edx+TfSecond]
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 0
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Write minutes
    ;
    mov al, [edx+TfMinute]
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 2
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Write hours
    ;
    mov al, [edx+TfHour]
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 4
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Write week
    ;
    mov al, [edx+TfWeekday]
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 6
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Write day
    ;
    mov al, [edx+TfDay]
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 7
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Write months
    ;
    mov al, [edx+TfMonth]
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 8
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Getyear and normalize it
    ;
    mov al, [edx+TfYear]
    cmp ax, 9999
    jbe NoMax
    mov ax, 9999

NoMax:
    ;
    ; Get century
    ;
    mov cl, 100
    div cl
    push eax
    aam
    rol al, 4
    ror ax, 4

    ;
    ; Set century byte
    ;
    push eax
    call _HalpSetCmosCenturyByte

    ;
    ; Get the year and write it
    ;
    pop eax
    mov al, ah
    aam
    rol al, 4
    ror ax, 4
    mov ah, al
    mov al, 9
    out 70h, al
    jmp $+2
    out 71h, al
    jmp $+2

    ;
    ; Release the lock and return
    ;
    stdCall _HalpReleaseCmosSpinLock
    stdRET _HalpWriteCmosTime
stdENDP _HalpWriteCmosTime

_TEXT ENDS
END
