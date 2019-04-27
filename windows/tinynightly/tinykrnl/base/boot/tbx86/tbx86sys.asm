;*++
;
;Copyright (c) Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   tbx86sys.asm
;
;Abstract:
;
;   This module provides the initial entrypoint called by the boot record code,
;   as well as various low-level system routines for entering and exiting both
;   real mode as well as protected mode, the generic trap handler, and routines
;   for enabling the A20 Line and for memory manipulation.
;
;Environment:
;
;   16-bit real-mode and 32-bit protected mode.
;
;Revision History:
;
;   Alex Ionescu - Implemented - 11-Apr-2006
;
;--*/
include tbx86.inc

_TEXT segment para use16 public 'CODE'
ASSUME CS:_TEXT, DS:_DATA, SS:_DATA
.386p

Start proc near
    ;
    ; Get offset to DATA
    ;
    mov bx, offset _TEXT:_DATA
    mov di, bx
    shr bx, 4

    ;
    ; Get CS Selector and add the offset
    ;
    mov ax, cs
    add ax, bx

    ;
    ; Set the segments and stack
    ;
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, offset _DATA:_Tbx86Stack

    ;
    ; Call the C entrypoint
    ;
    push ds
    push di
    push dx
    call _Tbx86Init
Start endp

public _Tbx86SwitchToPaged
_Tbx86SwitchToPaged proc near
    ;
    ; Clean up the flags
    ;
    push dword ptr 0
    popfd

    ;
    ; Save the stack in BX and check our argument
    ;
    mov bx, sp
    mov dx, [bx+2]

    ;
    ; Clean up GS and ES
    ;
    xor bx, bx
    mov gs, bx
    mov es, bx

    ;
    ; Set the PCR Selector in FS
    ;
    push KGDT_R0_PCR
    pop fs

    ;
    ; Load the GDT and IDT
    ;
    cli
    lgdt fword ptr [_GdtDescriptor]
    lidt fword ptr [_IdtDescriptor]

    ;
    ; Save the Video Selector
    ;
    mov si, offset _Tbx86VideoBuffer
    mov word ptr [si+2], KGDT_TBX_VIDEO

    ;
    ; Save CR0 so we can modify it
    ;
    mov ebx, cr0

    ;
    ; Check DX, where we saved our argument. If the argument specifies that
    ; we only want protection (this happens when we first load), then skip
    ; all the paging code, since that's TinyLoader's job
    ;
    or dx, dx
    jz SkipPaging

    ;
    ; OK, so we're enabling paging too
    ;
    or ebx, CR0_PE + CR0_PG
    mov cr0, ebx

    ;
    ; Do a jump to flush the prefetcher queue
    ;
    ALIGN 4
    jmp flush

SkipPaging:
    ;
    ; We're only enabling protected mode
    ;
    or ebx, CR0_PE
    mov cr0, ebx

    ;
    ; Do a jump to flush the prefetcher queue
    ;
    ALIGN 4
    jmp flush

flush:
    ;
    ; Reload CS and do a far return so we enter protected mode
    ;
    push KGDT_TBX_CODE
    push offset cs:restart
    retf

restart:
    ;
    ; Load DS and SS with our data selector now
    ;
    mov bx, KGDT_TBX_DATA
    mov ds, bx
    mov ss, bx

    ;
    ; Clear LDT
    ;
    xor bx, bx
    lldt bx

    ;
    ; If this is our first time loading, then load TR too
    ;
    or dx, dx
    jnz SkipTr
    mov bx, KGDT_TSS
    ltr bx

SkipTr:
    ;
    ; Return
    ;
    ret
_Tbx86SwitchToPaged endp

public _Tbx86SwitchToReal
_Tbx86SwitchToReal proc near
    ; Save the GDT and IDT
    sgdt fword ptr [_GdtDescriptor]
    sidt fword ptr [_IdtDescriptor]

    ;
    ; Save DS and switch to FLAT
    ;
    push [_InitialDs]
    mov ax, KGDT_TBX_DATA
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;
    ; Disable paging and protected mode
    ;
    mov eax, cr0
    and eax, not (CR0_PG + CR0_PE)
    mov cr0, eax

    ;
    ; Flush the prefetch queue
    ;
    jmp far ptr Continue

Continue:
    ;
    ; Flush the TLB
    ;
    mov eax, cr3
    nop
    nop
    nop
    nop
    mov cr3, eax

    ;
    ; Jump to real mode
    ;
    db 0EAh
    dw offset _TEXT:RealContinue
    dw 02000h

RealContinue:
    ;
    ; Restore DS and SS
    ;
    pop ax
    mov ds, ax
    mov ss, ax

    ;
    ; Restore Video Buffer
    ;
    mov si, offset _Tbx86VideoBuffer
    mov word ptr [si+2], 0b800h

    ;
    ; Load the empty IDT
    ;
    lidt fword ptr [_NullIdtDescriptor]

    ;
    ; Enable interrupts and return
    ;
    sti
    ret
_Tbx86SwitchToReal endp

public _Tbx86EnterTinyLoader
_Tbx86EnterTinyLoader proc near
    ;
    ; Get the entrypoint and restore DS
    ;
    mov ebx, dword ptr [esp+2]
    xor eax, eax
    mov ax, [_InitialDs]

    ;
    ; Set the stack for TinyLoader
    ;
    mov cx, KGDT_R0_DATA
    mov ss, cx
    mov esp, 62000h - 4

    ;
    ; Load DS and ES
    ;
    mov ds, cx
    mov es, cx

    ;
    ; Get the offset to the Boot Context structure, and turn it into a pointer
    ;
    shl eax, 4
    xor ecx, ecx
    mov cx, offset _BabyBlock
    add eax, ecx

    ;
    ; Push it to the loader, and push two dummy addresses for the INT frame
    ;
    push eax
    push 1010h
    push 1010h

    ;
    ; Push the far address of the loader
    ;
    db 66h
    push KGDT_R0_CODE
    push ebx

    ;
    ; Do a far return to jump into the loader
    ;
    db 66h
    retf
_Tbx86EnterTinyLoader endp

public _Tbx86GetBiosMemoryMap
_Tbx86GetBiosMemoryMap proc near
    ;
    ; Setup stack frame
    ;
    push ebp
    mov bp, sp

    ;
    ; Set the E820 frame in BP and save volatiles
    ;
    mov bp, [bp+6]
    push es
    push edi
    push esi
    push ebx

    ;
    ; Set ES == SS
    ;
    push ss
    pop es

    ;
    ; Set the frame data
    ;
    mov ebx, [bp].Next
    mov ecx, [bp].BlockSize
    lea di, [bp].BaseAddressLo

    ;
    ; Set the function code and magic, then call it
    ;
    mov eax, 0E820h
    mov edx, 'SMAP'
    int 15h

    ;
    ; Update key and size
    ;
    mov [bp].Next, ebx
    mov [bp].BlockSize, ecx

    ;
    ; Check for success and if the signature matched, set 0 for success
    ;
    sbb ecx, ecx
    sub eax, 'SMAP'
    or ecx, eax

    ;
    ; Return error code
    ;
    mov [bp].Status, ecx

    ;
    ; Restore volatiles
    ;
    pop ebx
    pop esi
    pop edi
    pop es
    pop ebp
    ret
_Tbx86GetBiosMemoryMap endp

public _Tbx86EnableA20Line
_Tbx86EnableA20Line proc near
    ;
    ; Enable the A20 line
    ;
    call Empty8042
    mov al, 0D1h
    out 64h, al
    call Empty8042
    mov al, 0DFh
    out 60h, al
    call Empty8042
    ret
_Tbx86EnableA20Line endp

public Empty8042
Empty8042 proc near
    ;
    ; Flush the buffer
    ;
    in al, 64h
    jmp $+2
    jmp $+2
    jmp $+2
    jmp $+2
    test al, 2
    jnz Empty8042
    ret
Empty8042 endp

Tbx86TrapEnter proc near
    ;
    ; Start building the trap frame
    ;
    push ebp
    push ebx
    push esi
    push edi
    push 0
    push fs
    sub esp, 8
    push eax
    push ecx
    push edx
    push 0
    push ds
    push 0
    push es
    push 0
    push gs

    ;
    ; Switch to safe segments and skip all the debug stuff
    ;
    mov ax, KGDT_TBX_DATA
    sub esp, 30h
    mov ds, ax
    mov es, ax

    ;
    ; Set the stack frame to point to us
    ;
    mov ebp, esp

    ;
    ; Fill out Debug Frame (FIXME: Do only for DBG)
    ;
    ;mov ebx, [ebp+60h]
    ;mov edi, [ebp+68h]
    ;mov dword ptr [ebp+0Ch], edx
    mov dword ptr [ebp+8], 0BADB0D00h
    ;mov dword ptr [ebp], ebx
    ;mov dword ptr [ebp+4], edi

    ;
    ; Is the exception frame on a 16bit or 32bit stack?
    ;
    mov ax, ss
    cmp ax, KGDT_R0_DATA
    jne Tbx86Trap32

    ;
    ; FIXME: Handle 32-bit traps
    ;

Tbx86Trap32:
    ;
    ; Call the trap handler
    ;
    push ebp
    call _Tbx86TrapHandler
Tbx86TrapEnter endp

public _Tbx86MemMove
_Tbx86MemMove proc near
    ;
    ; Setup the stack frame and save volatiles
    ;
    enter 0, 0
    push ds
    push es

    ;
    ; Set destination and origin pointers
    ;
    mov esi, dword ptr [bp+4]
    mov edi, dword ptr [bp+8]

    ;
    ; Set size in ULONGs
    ;
    mov ecx, dword ptr [bp+12]
    shr ecx, 2

    ;
    ; Set DS and ES
    ;
    mov ax, KGDT_R0_DATA
    mov ds, ax
    mov es, ax

    ;
    ; Do the move
    ;
    cld
    rep movs dword ptr [edi], dword ptr [esi]

    ;
    ; Modulo the bytes left and copy those too
    ;
    mov ecx, dword ptr [bp+12]
    and ecx, 3
    rep movs byte ptr [edi], byte ptr [esi]

    ;
    ; Restore stack and volatiles
    ;
    pop es
    pop ds
    leave
    ret
_Tbx86MemMove endp

public _Tbx86MemZero
_Tbx86MemZero proc near
    ;
    ; Setup the stack frame and save volatiles
    ;
    enter 0, 0
    push es

    ;
    ; Set destination pointer
    ;
    mov edi, dword ptr [bp+4]

    ;
    ; Set size in ULONGs
    ;
    mov ecx, dword ptr [bp+8]
    shr ecx,2

    ;
    ; Set ES
    ;
    mov ax, KGDT_R0_DATA
    mov es, ax

    ;
    ; Do the erase
    ;
    xor eax, eax
    cld
    rep stos dword ptr [edi]

    ;
    ; Modulo the bytes left and zero those too
    ;
    mov ecx, dword ptr [bp+8]
    and ecx, 3
    rep stos byte ptr [edi]

    ;
    ; Restore stack and volatiles
    ;
    pop es
    leave
    ret
_Tbx86MemZero endp

BUILD_TRAP 0
BUILD_TRAP 1
BUILD_TRAP 2
BUILD_TRAP 3
BUILD_TRAP 4
BUILD_TRAP 5
BUILD_TRAP 6
BUILD_TRAP 7
BUILD_TRAP 8
BUILD_TRAP 9
BUILD_TRAP 10
BUILD_TRAP 11
BUILD_TRAP 12
BUILD_TRAP 13
BUILD_TRAP 14
BUILD_TRAP 15

_TEXT ends
end Start
