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
;   Master header file for the assembly (ASM) portion of TBX86.
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
.386

;
; From ks386.inc
;
CR0_PE              equ 1h
CR0_ET              equ 10h
CR0_PG              equ 80000000h
KGDT_R0_CODE        equ 8h
KGDT_R0_DATA        equ 10h
KGDT_TSS            equ 28h
KGDT_R0_PCR         equ 30h
KTRAP_FRAME_LENGTH  equ 0008CH
KGDT_TBX_CODE       equ 58h
KGDT_TBX_DATA       equ 60h
KGDT_TBX_VIDEO      equ 68h

;
; Builds a trap entry point
;
BUILD_TRAP macro Number
Public _Tbx86Trap&Number
_Tbx86Trap&Number proc near
    ;
    ; Save the trap number
    ;
    push 0
    push Number

    ;
    ; Jump to the frame builder
    ;
    jmp Tbx86TrapEnter
_Tbx86Trap&Number endp
endm

;
; Builds a HyperGate
;
BUILD_HYPER_GATE macro Name, HasArguments, NoFrame
_TEXT32 segment para use32 public 'CODE'
ASSUME CS:_TEXT32
Public _&Name
_&Name proc near
    ;
    ; Setup a gate from 32-bit CS to 16-bit CS
    ;
    push dword ptr KGDT_TBX_CODE
    push dword ptr (offset s_&Name)
    retf
_TEXT32 ends
ASSUME CS:_TEXT
s_&Name:
    ;
    ; Enter the HyperGate
    ;
    call _HyperGateEnter
ifnb <HasArguments>
    ;
    ; Setup the 16-bit stack
    ;
    push size Name&Args
    call _HyperGateFrame
endif
    ;
    ; Enter Real Mode
    ;
    call _Tbx86SwitchToReal
ifnb <HasArguments>
ifb <NoFrame>
    ;
    ; Setup stack frame
    ;
    push bp
    mov bp, sp
    add bp, 2
endif
endif
endm

;
; Does a HyperJump back to TinyLoader
;
HYPERGATE_EXIT macro Name, HasArguments, NoFrame
ifnb <HasArguments>
ifb <NoFrame>
    ;
    ; If this HyperGate Function had arguments, clean up the stack
    ;
    pop bp
endif
    add sp, size Name&Args
endif
    ;
    ; Re-enable Paging and Protected Mode
    ;
    push 1
    call _Tbx86SwitchToPaged

    ;
    ; Do a HyperJump back to TinyLoader
    ;
    add sp, 2
    jmp _HyperGateExit
_&Name endp
endm

;
; Structure for INT 15 function E820
;
E820Frame struc
    Status dd ?
    Next dd ?
    BlockSize dd ?
    BaseAddressLo dd ?
    BaseAddressHi dd ?
    LengthLo dd ?
    LengthHi dd ?
    MemType dd ?
E820Frame ends

;
; HyperGate Frames
;
GetMemoryDescriptorArgs struc
    SystemMdBlock dd ?
GetMemoryDescriptorArgs ends

DiskAccessArgs struc
    FunctionCode dd ?
    DriveNumber dd ?
    Head dd ?
    Track dd ?
    Sector dd ?
    SectorCount dd ?
    DiskBuffer dd ?
DiskAccessArgs ends

NtDetectArgs struc
    HeapBase dd ?
    HeapSize dd ?
    HwTree dd ?
    HeapUsed dd ?
    Options dd ?
    OptionsLength dd ?
NtDetectArgs ends

HardwareCursorArgs struc
    XOrd dd ?
    YOrd dd ?
HardwareCursorArgs ends

DetectExtendedInt13Args struc
    DriveBuffer dd ?
    Int13DriveNumber dd ?
DetectExtendedInt13Args ends

;
; Let the assembly code know of our C DATA section and its public variables
;
_DATA segment para use16 public 'DATA'
_DATA ends
extrn _Tbx86Init:near
extrn _Tbx86VideoBuffer:near
extrn _Tbx86TrapHandler:near
extrn _GdtDescriptor:fword
extrn _IdtDescriptor:fword
extrn _NullIdtDescriptor:fword
extrn _Tbx86Stack:word
extrn _BabyBlock:word
extrn _InitialDs:word

