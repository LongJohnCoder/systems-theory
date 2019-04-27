.586p

include ks386.inc
include callconv.inc

_TEXT SEGMENT DWORD PUBLIC 'CODE'
ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

public @ExInterlockedPopEntrySList@4
@ExInterlockedPopEntrySList@4:

    ;
    ; Save nonvolatiles
    ;
    push ebx
    push ebp

    ;
    ; Save the listhead in EBP
    ;
    mov ebp, ecx

public ExpInterlockedPopEntrySListResume
ExpInterlockedPopEntrySListResume:
    ;
    ; Get the next sequence number and flink in EDX and EAX
    ;
    mov edx, [ebp+4]
    mov eax, [ebp]

    ;
    ; Check if the list is empty
    ;
    or eax, eax
    jz EmptyList

    ;
    ; Copy the depth and decrease it
    ;
    lea ecx, [edx-1]

public ExpInterlockedPopEntrySListFault
ExpInterlockedPopEntrySListFault:

    ;
    ; Get the next entry
    ;
    mov ebx, [eax]

    ;
    ; Do the pop
    ;
public ExpInterlockedPopEntrySListEnd
ExpInterlockedPopEntrySListEnd:
    lock cmpxchg8b qword ptr [ebp]

    ;
    ; If we failed, try again
    ;
    jnz ExpInterlockedPopEntrySListResume

EmptyList:
    ;
    ; Restore nonvolatiles
    ;
    pop ebp
    pop ebx
    ret
_TEXT ENDS
END
