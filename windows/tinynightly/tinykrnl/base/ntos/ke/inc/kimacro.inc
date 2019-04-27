;*++
;
;Copyright (c) Alex Ionescu.  All rights reserved.
;
;   THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;   PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;   kimacro.inc
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
;   Alex Ionescu - Started Implementation - 21-Dec-06
;
;--*/
.586p

;/*++
; * @name SET_TF_DEBUG_HEADER
; *
; * The SET_TF_DEBUG_HEADER macro FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this macro needs to be completed.
; *
; *--*/
SET_TF_DEBUG_HEADER macro
    ;
    ; Get the Debug Trap Frame EBP/EIP
    ;
    mov ebx, [ebp+TsEbp]
    mov edi, [ebp+TsEip]

    ;
    ; Write the debug data
    ;
    mov [ebp+TsDbgArgPointer], edx
    mov dword ptr [ebp+TsDbgArgMark], 0BADB0D00h
    mov [ebp+TsDbgEbp], ebx
    mov [ebp+TsDbgEip], edi
endm

;/*++
; * @name V86_TRAP_FIXUP
; *
; * The V86_TRAP_FIXUP macro FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this macro needs to be completed.
; *
; *--*/
V86_TRAP_FIXUP macro EnterLabel, ExitLabel
V86_&EnterLabel:
    ;
    ; Get V86 segment registers
    ;
    mov eax, [ebp+TsV86Fs]
    mov ebx, [ebp+TsV86Gs]
    mov ecx, [ebp+TsV86Es]
    mov edx, [ebp+TsV86Ds]

    ;
    ; Restore them into Protected Mode trap frame
    ;
    mov [ebp+TsSegFs], ax
    mov [ebp+TsSegGs], bx
    mov [ebp+TsSegEs], cx
    mov [ebp+TsSegDs], dx

    ;
    ; Go back to mainline code
    ;
    jmp V86_&ExitLabel
endm

;/*++
; * @name DR_TRAP_FIXUP
; *
; * The DR_TRAP_FIXUP macro FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this macro needs to be completed.
; *
; *--*/
DR_TRAP_FIXUP macro EnterLabel, ExitLabel
Dr_&EnterLabel:
    ;
    ; Check if this was V86 mode
    ;
    test dword ptr [ebp+TsEFlags], EFLAGS_V86_MASK
    jnz @f

    ;
    ; Check if it was user mode
    ;
    test byte ptr [ebp+TsSegCs], MODE_MASK
    jz Dr_&ExitLabel

@@:
    ;
    ; Get DR0, 1, 2
    ;
    mov ebx, dr0
    mov ecx, dr1
    mov edi, dr2

    ;
    ; Save them
    ;
    mov [ebp+TsDr0], ebx
    mov [ebp+TsDr1], ecx
    mov [ebp+TsDr2], edi

    ;
    ; Get DR3, 6, 7
    ;
    mov ebx, dr0
    mov ecx, dr1
    mov edi, dr2

    ;
    ; Save them
    ;
    mov [ebp+TsDr3], ebx
    mov [ebp+TsDr6], ecx
    mov [ebp+TsDr7], edi

    ;
    ; Clear DR7
    ;
    xor ebx, ebx
    mov dr7, ebx

    ;
    ; Get the PRCB
    ;
    mov edi, PCR[PcPrcb]

    ;
    ; Get DR0, 1
    ;
    mov ebx, [edi+PbProcessorState+PsSpecialRegisters+SrKernelDr0]
    mov ecx, [edi+PbProcessorState+PsSpecialRegisters+SrKernelDr1]

    ;
    ; Set them
    ;
    mov dr0, ebx
    mov dr1, ecx

    ;
    ; Get DR2, 3
    ;
    mov ebx, [edi+PbProcessorState+PsSpecialRegisters+SrKernelDr2]
    mov ecx, [edi+PbProcessorState+PsSpecialRegisters+SrKernelDr3]

    ;
    ; Set them
    ;
    mov dr2, ebx
    mov dr3, ecx

    ;
    ; Get DR6, 7
    ;
    mov ebx, [edi+PbProcessorState.PsSpecialRegisters.SrKernelDr6]
    mov ecx, [edi+PbProcessorState.PsSpecialRegisters.SrKernelDr7]

    ;
    ; Set them
    ;
    mov dr6, ebx
    mov dr7, ecx
    jmp Dr_&ExitLabel
endm

;/*++
; * @name INT_PROLOG
; *
; * The INT_PROLOG macro FILLMEIN
; *
; * @param None.
; *
; * @return None.
; *
; * @remarks Documentation for this macro needs to be completed.
; *
; *--*/
INT_PROLOG macro Label, Return, FakeErrorCode
ifb <FakeErrorCode>
    ;
    ; Save fake error code
    ;
    push esp
endif

    ;
    ; Save the non-volatiles
    ;
    push ebp
    push ebx
    push esi
    push edi

    ;
    ; Skip debug registers and other stuff
    ;
    sub esp, TsEdi

    ;
    ; Set up frame
    ;
    mov ebp, esp

    ;
    ; Save volatiles
    ;
    mov [esp+TsEax], eax
    mov [esp+TsEcx], ecx
    mov [esp+TsEdx], edx
    mov dword ptr [esp+TsPreviousPreviousMode], -1

    ;
    ; Check if this was from V86 Mode
    ;
    test dword ptr [ebp+TsEflags], EFLAGS_V86_MASK
    jnz V86_&Label

    ;
    ; Check if this was kernel mode
    ;
    cmp word ptr [esp+TsSegCs], KGDT_R0_CODE
    jz @f

    ;
    ; Set segments
    ;
    mov [esp+TsSegFs], fs
    mov [esp+TsSegDs], ds
    mov [esp+TsSegEs], es
    mov [esp+TsSegGs], gs

    ;
    ; Load the segment registers (use OVERRIDE (0x66))
    ;
V86_&Return:
    mov ebx, KGDT_R0_PCR
    mov eax, KGDT_R3_DATA OR RPL_MASK
    mov fs, bx
    mov ds, ax
    mov es, ax

@@:
    ;
    ; Get the previous exception list
    ;
    mov ebx, [fs:PcExceptionList]

    ;
    ; Set the exception handler chain terminator
    ;
    mov [fs:PcExceptionList], -1

    ;
    ; Save the previous exception list
    ;
    mov [esp+PcExceptionList], ebx

ifnb <FakeErrorCode>
    ;
    ; Setup the 16-bit stack
    ;
    lea eax, [esp+TsErrCode]
    lea ecx, [esp+TsEip]
    mov ebx, ss:[eax]
    mov ss:[eax], ecx
endif

    ;
    ; FIXME: Check if this is the ABIOS stack
    ;
    ; cmp esp, 0x10000
    ; jb Abios_Label

    ;
    ; Delete error code
    ;
    and dword ptr [esp+TsErrCode], 0

    ;
    ; Get the current thread and clear direction flag
    ;
    mov ecx, [fs:PcPrcbData+PbCurrentThread]
    cld

    ;
    ; Flush DR7
    ;
    and dword ptr [ebp+TsDr7], 0

    ;
    ; Check if the thread was being debugged
    ;
    test byte ptr [ecx+ThDebugActive], 0FFh

ifnb <FakeErrorCode>
    ;
    ; Push parameter
    ;
    push ebx
endif

    ;
    ; Save DR registers if needed
    ;
    jnz Dr_&Label

Dr_&Return:
    ;
    ; Set the trap frame debug header
    ;
    SET_TF_DEBUG_HEADER
endm

