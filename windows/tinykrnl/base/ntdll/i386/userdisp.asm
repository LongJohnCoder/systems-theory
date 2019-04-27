.386p

include ks386.inc
include callconv.inc

EXTRNP _ZwCallbackReturn, 3
EXTRNP _ZwContinue, 2
EXTRNP _RtlDispatchException, 2
EXTRNP _RtlRaiseStatus, 1
EXTRNP _ZwRaiseException, 3
EXTRNP _RtlRaiseException, 1
EXTRNP _ZwTestAlert, 0
EXTRNP _LdrpInitialize, 3

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

_KiUserApcExceptionHandler:
    ;
    ; Put the exception record in ECX and check the Flags
    ;
    mov ecx, [esp+4]
    test dword ptr [ecx+ErExceptionFlags], EXCEPTION_UNWIND
    jz dispreturn

    ; Test alert the thread
    stdCall _ZwTestAlert

dispreturn:
    ;
    ; We'll continue
    ;
    mov eax, ExceptionContinueSearch
    ret 16

_KiUserCallbackExceptionHandler:
    ;
    ; Put the exception record in ECX and check the Flags
    ;
    mov ecx, [esp+4]
    test dword ptr [ecx+ErExceptionFlags], EXCEPTION_UNWIND
    jz disp2return

    ; Return to the kernel
    stdCall _ZwCallbackReturn, <0, 0, STATUS_CALLBACK_POP_STACK>

disp2return:
    ;
    ; We'll continue
    ;
    mov eax, ExceptionContinueSearch
    ret 16

cPublicProc _KiUserApcDispatcher ,5
    ;
    ; Get pointer to stack
    ;
    lea eax, [esp+2DCh]

    ;
    ; Set SEH
    ;
    mov ecx, fs:PcExceptionList
    mov edx, _KiUserApcExceptionHandler
    mov [eax], ecx
    mov [eax+4], edx
    mov fs:PcExceptionList, eax

    ;
    ; Now get the APC Stack and call the APC routine
    ;
    pop eax
    lea edi, [esp+0Ch]
    call eax

    ;
    ; Remove SEH
    ;
    mov ecx, [edi+2CCh]
    mov fs:PcExceptionList, ecx

    ;
    ; Go back to kernel mode
    ;
    stdCall _ZwContinue, <edi, 1>
    mov esi, eax

apcloop:
    stdCall _RtlRaiseStatus, <esi>
    jmp apcloop
stdENDP _KiUserApcDispatcher

cPublicProc _KiUserCallbackDispatcher, 3

    ;
    ; Set SEH
    ;
    mov ecx, fs:PcExceptionList
    mov edx, _KiUserCallbackExceptionHandler
    lea eax, [esp+10h]
    mov [esp+10h], ecx
    mov [esp+14h], edx
    mov fs:PcExceptionList, eax

    ;
    ; Get function ID
    ;
    add esp, 4
    pop edx

    ;
    ; Call the callback
    ;
    mov eax,fs:[TebPeb]
    mov eax,[eax+PeKernelCallbackTable]
    call dword ptr [eax+edx*4]

    ;
    ; Return to kernel
    ;
    stdCall _ZwCallbackReturn, <0, 0, eax>
    mov esi, eax

userloop:
    stdCall _RtlRaiseStatus, <esi>
    jmp userloop
stdENDP _KiUserCallbackDispatcher

cPublicProc _KiUserExceptionDispatcher, 2
    ;
    ; Get arguments and call the exception dispatcher
    ;
    mov ecx, [esp+4]
    mov ebx, [esp]
    stdCall _RtlDispatchException, <ebx, ecx>

    ;
    ; Check if should raise the exception
    ;
    or al, al
    je short raise

    ;
    ; Restore EBX and EBX, then return to kernel
    ;
    pop ebx
    pop ecx
    stdCall _ZwContinue, <ecx, 0>
    jmp short raise2

raise:
    pop ebx
    pop ecx
    stdCall _ZwRaiseException, <ebx, ecx, 0>

raise2:
    ;
    ; Make space for an exception record
    ;
    add esp, -14

    ;
    ; Fill it out
    ;
    mov [esp+ErExceptionCode], eax
    mov dword ptr [esp+ErExceptionFlags], EXCEPTION_NONCONTINUABLE
    mov [esp+ErExceptionRecord],ebx
    mov dword ptr [esp+ErNumberParameters], 0

    ;
    ; Raise the exception
    ;
    stdCall _RtlRaiseException, <esp>
stdRET _KiUserExceptionDispatcher
stdENDP _KiUserExceptionDispatcher

cPublicProc _KiRaiseUserExceptionDispatcher
    ;
    ; Make room on the stack for an exception record
    ;
    push ebp
    mov ebp, esp
    sub esp, ExceptionRecordLength

    ;
    ; Save exception address and code
    ;
    mov [esp+ErExceptionAddress], eax
    mov eax, fs:[TebPeb]
    mov eax, [eax+TbExceptionCode]
    mov [esp+ErExceptionCode], eax

    ;
    ; Save exception flags, record and number of parameters
    ;
    mov dword ptr [esp+ErExceptionFlags], 0
    mov dword ptr [esp+ErExceptionRecord], 0
    mov dword ptr [esp+ErNumberParameters], 0

    ;
    ; Raise the exception, and return the code
    ;
    stdCall _RtlRaiseException, <esp>
    mov eax, [esp+ErExceptionCode]

    ;
    ; Cleanup stack and return
    ;
    mov esp, ebp
    pop ebp
stdRET _KiRaiseUserExceptionDispatcher
stdENDP _KiRaiseUserExceptionDispatcher

.686p
cPublicProc _KiFastSystemCall
    ;
    ; Save stack in EDX and do a SYSENTER
    ;
    mov edx, esp
    sysenter
stdENDP _KiFastSystemCall

cPublicProc _KiFastSystemCallRet
    ;
    ; Just return
    ;
stdRET _KiFastSystemCallREt
stdENDP _KiFastSystemCallREt

cPublicProc _KiIntSystemCall
    ;
    ; Save stack in EDX and do an INT 2E
    ;
    lea edx, [esp+8]
    int 2Eh
stdRET _KiIntSystemCall
stdENDP _KiIntSystemCall

cPublicProc _LdrInitializeThunk
    ;
    ; Put context into function argument
    ;
    lea eax, [esp+10h]
    mov [esp+4], eax

    ;
    ; Clear stack and jump into C code
    ;
    xor ebp, ebp
    jmp _LdrpInitialize@12
stdENDP _LdrInitializeThunk

cPublicProc _LdrpCallInitRoutine, 4
    ;
    ; Setup frame
    ;
    push ebp
    mov ebp, esp

    ;
    ; Save volatiles
    ;
    push esi
    push edi
    push ebx

    ;
    ; Save stack
    mov esi, esp

    ;
    ; Call the init routine
    ;
    push [ebp+14h]
    push [ebp+10h]
    push [ebp+0Ch]
    call dword ptr [ebp+8]

    ;
    ; Restore stack and volatiles
    ;
    mov esp, esi
    pop ebx
    pop edi
    pop esi
    pop ebp
stdRET _LdrpCallInitRoutine
stdENDP _LdrpCallInitRoutine

_TEXT ENDS
END
