;*++
;
;Copyright (c) Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   hyprgate.asm
;
;Abstract:
;
;   This module provides access to x86 hardware through BIOS interrupts,
;   available and exported to the high level portable loader (TinyLoader)
;   through "HyperGate Functions", which drop into 16-bit real-mode to
;   perform the operation, then return the data requested.
;
;Environment:
;
;   16-bit real-mode
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

extrn _Tbx86SwitchToPaged:near
extrn _Tbx86SwitchToReal:near

public _HyperGateEnter
_HyperGateEnter proc near
    ;
    ; Save function pointer in ECX
    ;
    pop cx

    ;
    ; Save volatiles and stack
    ;
    push ebp
    push ebx
    push esi
    push edi
    mov esi, esp

    ;
    ; Switch to FLAT selectors
    ;
    mov ax, KGDT_TBX_DATA
    mov ds, ax
    mov ss, ax

    ;
    ; Switch to HyperGate Stack
    ;
    mov sp, 8000h - 2
    push esi

    ;
    ; Return to actual function code
    ;
    jmp cx
_HyperGateEnter endp

public _HyperGateExit
_HyperGateExit proc near
    ;
    ; Restore ESP
    ;
    pop esi

    ;
    ; Restore FLAT selectors and ESP
    ;
    mov dx, KGDT_R0_DATA
    mov ds, dx
    mov ss, dx
    mov es, dx
    mov esp, esi

    ;
    ; Restore volatiles and EBP
    ;
    pop edi
    pop esi
    pop ebx
    pop ebp

    ;
    ; Build jump frame and far jump
    ;
    pop edx
    push dword ptr KGDT_R0_CODE
    push edx
    db 66h
    retf
_HyperGateExit endp

public _HyperGateFrame
_HyperGateFrame proc near
    ;
    ; Save return pointer
    ;
    pop dx

    ;
    ; Save the size in dwords
    ;
    pop cx
    movzx ecx, cx
    sub sp, cx
    shr ecx, 2

    ;
    ; Save the frame pointer and make space for 4 volatiles and the return
    ;
    add esi, 20

    ;
    ; Switch to FLAT ds and DATA es
    ;
    push KGDT_R0_DATA
    pop ds
    push ss
    pop es

    ;
    ; Copy the data
    ;
    movzx edi, sp
    rep movs dword ptr [edi], dword ptr [esi]

    ;
    ; Restore DATA DS and return
    ;
    push es
    pop ds
    jmp dx
_HyperGateFrame endp

BUILD_HYPER_GATE Reboot
    ;
    ; Reboot
    ;
    int 19h
    jmp $+3
_Reboot endp

BUILD_HYPER_GATE DiskAccess, TRUE
    ;
    ; Setup the pointer into ES:BX
    ;
    mov eax, [bp].DiskBuffer
    mov bx, ax
    and bx, 0Fh
    shr eax, 4
    mov es, ax

    ;
    ; Add the 2 high bits from the cylinder number into the sector number
    ;
    mov cx, word ptr [bp].Track
    xchg ch, cl
    shl cl, 6
    add cl, byte ptr [bp].Sector

    ;
    ; Write the Function ID, and Sector/Head/Drive data into expected registers
    ;
    mov ah, byte ptr [bp].FunctionCode
    mov al, byte ptr [bp].SectorCount
    mov dh, byte ptr [bp].Head
    mov dl, byte ptr [bp].DriveNumber

    ;
    ; Call the disk interrupt and check for success
    ;
    int 13h
    jb Success

    ;
    ; We failed, so cleanup EAX
    ;
    xor eax, eax

Success:
    ;
    ; Suceess, make sure EAX only has valid information
    ;
    and eax, 0FFFFh

    ;
    ; Save low-word of ECX into high-word, and put dx into low word
    ;
    shl ecx, 16
    mov cx, dx
HYPERGATE_EXIT DiskAccess, TRUE

BUILD_HYPER_GATE GetChar
    ;
    ; Check if a key is in the buffer
    ;
    mov ax, 100h
    int 16h

    ;
    ; If not, then return 0
    ;
    jnz HaveChar
    mov eax, 0
    jmp GotChar

HaveChar:
    ;
    ; Request the key that's currently in the buffer, and clean up EAX
    ;
    mov ax, 0
    int 16h
    and eax, 0FFFFh
GotChar:
HYPERGATE_EXIT GetChar

BUILD_HYPER_GATE GetCounter
    ;
    ; Request counter value
    ;
    mov ah, 0
    int 1Ah

    ;
    ; Put cx into high-word of EAX and dx into the low word
    ;
    mov ax, cx
    shl eax, 16
    mov ax, dx
HYPERGATE_EXIT GetCounter

BUILD_HYPER_GATE HardwareCursor, TRUE
    ;
    ; Get coordinates from arguments
    ;
    mov eax, [bp].YOrd
    mov edx, [bp].XOrd

    ;
    ; Check if this is actually a special command being piggybacked
    ;
    cmp edx, 80000000h
    jne CursorRequest

    ;
    ; It is, get the VGA function code
    ;
    mov ebx, eax
    shr ebx, 16
    jmp CallVga

CursorRequest:
    ;
    ; This is a pure cursor request, so setup the registers for X and Y
    ;
    mov dh, al
    mov ah, 2
    mov bh, 0

CallVga:
    ;
    ; Call the VGA interrupt
    ;
    int 10h
HYPERGATE_EXIT HardwareCursor, TRUE

BUILD_HYPER_GATE NtDetect, TRUE, TRUE
    ;
    ; Jump to ntdetect.com by using a far return
    ;
    push cs
    push offset _TEXT:NtDetectReturn
    push 1000h
    push 0
    retf
NtDetectReturn:
HYPERGATE_EXIT NtDetect, TRUE, TRUE

BUILD_HYPER_GATE GetStallCounter
    ;
    ; Not yet implemented (only used in SCSI code)
    ;
    mov eax, 100
HYPERGATE_EXIT GetStallCounter

BUILD_HYPER_GATE ResetDisplay
    ;
    ; Call function 1112H to reset the VGA Font and Display
    ;
    mov ax, 1112h
    mov bx, 0
    int 10h
HYPERGATE_EXIT ResetDisplay

BUILD_HYPER_GATE GetMemoryDescriptor, TRUE
    ;
    ; Get the System Memory Descriptor Block pointer into BP
    ;
    mov eax, [bp].SystemMdBlock
    mov bp, ax
    and bp, 0Fh

    ;
    ; And setup the segment in ES
    ;
    shr eax, 4
    mov es, ax

    ;
    ; Put the link value and size into EBX/ECX where the function expects them
    ;
    mov ebx, es:[bp].Next
    mov ecx, es:[bp].BlockSize

    ;
    ; Point DI to the actual start of the Descriptor inside our Block
    ;
    lea di, [bp].BaseAddressLo

    ;
    ; Call INT 15 with the E820h function code and magic signature
    ;
    mov eax, 0E820h
    mov edx, 'SMAP'
    int 15h

    ;
    ; Update the link value and size so the caller can access them
    ;
    mov es:[bp].Next, ebx
    mov es:[bp].BlockSize, ecx

    ;
    ; Check for success based on carry flag, then match the signature
    ;
    sbb ecx, ecx
    sub eax, 'SMAP'
    or ecx, eax

    ;
    ; If both of these worked, ECX should be STATUS_SUCCESS
    ;
    mov es:[bp].Status, ecx
HYPERGATE_EXIT GetMemoryDescriptor, TRUE

BUILD_HYPER_GATE DetectExtendedInt13, TRUE
    ;
    ; Save volatiles
    ;
    push dx
    push bx
    push ds
    push si

    ;
    ; Set function 41 and set the magic number
    ;
    mov ah, 41h
    mov bx, 55AAh

    ;
    ; Set the Drive ID and call INT 13
    ;
    mov dl, byte ptr [bp].Int13DriveNumber
    int 13h

    ;
    ; Handle errors
    ;
    jb NotPresent
    cmp bx, 0AA55h
    jnz NotPresent
    test cl, 1
    jz NotPresent

    ;
    ; We survived, setup the pointer into DS:SI
    ;
    mov eax, [bp].DriveBuffer
    mov bx, ax
    and bx, 0Fh
    mov si, bx
    shr eax, 4
    mov ds, ax

    ;
    ; Set size for v1.0 structure
    ;
    mov word ptr [si], 1Ah

    ;
    ; Set drive number and call INT 13
    ;
    mov dl, byte ptr [bp].Int13DriveNumber
    mov ax, 48h
    int 13h

    ;
    ; Check for success
    ;
    jb NotPresent
    mov al, 1
    jnb Present

NotPresent:
    ;
    ; If we got here, then support for 48H is not available
    ;
    xor al, al

Present:
    ;
    ; Clear garbage in upper EAX
    ;
    movzx eax, al

    ;
    ; Restore volatiles
    ;
    pop si
    pop ds
    pop bx
    pop dx
HYPERGATE_EXIT DetectExtendedInt13, TRUE
_TEXT ends
end

