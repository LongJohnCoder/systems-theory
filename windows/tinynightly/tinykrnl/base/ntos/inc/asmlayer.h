#ifndef _ASMLAYER_
#define _ASMLAYER_

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4733)               // re-enable below

//
// Inlined Assembly Function to halt the CPU
//
FORCEINLINE
VOID
NTAPI
HaltCpu(VOID)
{
    for (;;) __asm hlt;
}

//
// Inlined Assembly Functions to play with the stack
//
FORCEINLINE
VOID
NTAPI
ClearEbp(VOID)
{
    __asm xor ebp, ebp
}

FORCEINLINE
VOID
NTAPI
SubstractEsp(IN ULONG Value)
{
    __asm sub esp, Value
}

FORCEINLINE
VOID
NTAPI
PushToStack(IN Value)
{
    __asm push Value;
}

FORCEINLINE
VOID
NTAPI
PushEdiToStack(VOID)
{
    __asm push edi;
}

FORCEINLINE
VOID
NTAPI
PushExceptionListToStack(VOID)
{
    __asm push fs:[KPCR_EXCEPTION_LIST];
}

FORCEINLINE
VOID
NTAPI
PopExceptionListFromStack(VOID)
{
    __asm pop fs:[KPCR_EXCEPTION_LIST];
}

FORCEINLINE
VOID
NTAPI
SetExceptionHandler(VOID)
{
    __asm mov fs:[KPCR_EXCEPTION_LIST], esp;
}

FORCEINLINE
VOID
NTAPI
RemoveExceptionHandler(VOID)
{
    __asm mov esp, fs:[KPCR_EXCEPTION_LIST];
}

FORCEINLINE
VOID
NTAPI
PushEsiToStack(VOID)
{
    __asm push esi;
}

FORCEINLINE
VOID
NTAPI
PushEbxToStack(VOID)
{
    __asm push ebx;
}

FORCEINLINE
VOID
NTAPI
PushEbpToStack(VOID)
{
    __asm push ebp;
}

FORCEINLINE
VOID
NTAPI
PushFsToStack(VOID)
{
    __asm push fs;
}

FORCEINLINE
VOID
NTAPI
PushEaxToStack(VOID)
{
    __asm push eax;
}

FORCEINLINE
VOID
NTAPI
PushEcxToStack(VOID)
{
    __asm push ecx;
}

FORCEINLINE
VOID
NTAPI
PushDsToStack(VOID)
{
    __asm push ds;
}

FORCEINLINE
VOID
NTAPI
PushEsToStack(VOID)
{
    __asm push es;
}

FORCEINLINE
VOID
NTAPI
PushGsToStack(VOID)
{
    __asm push gs;
}

FORCEINLINE
VOID
NTAPI
PushEdxToStack(VOID)
{
    __asm push edx;
}

FORCEINLINE
VOID
NTAPI
SetStack(IN ULONG Value)
{
    __asm mov ebp, Value;
}

FORCEINLINE
USHORT
NTAPI
GetEbp(VOID)
{
    __asm mov eax, ebp
}

FORCEINLINE
VOID
FASTCALL
SetEspStack(IN ULONG Value)
{
    __asm mov esp, Value;
}

FORCEINLINE
VOID
FASTCALL
SetEspToEbp(VOID)
{
    __asm mov esp, ebp;
}

FORCEINLINE
ULONG
NTAPI
GetStack(VOID)
{
    __asm mov eax, esp;
}

FORCEINLINE
VOID
NTAPI
RetFromInterrupt(VOID)
{
    __asm iretd;
}

//
// Inlined Assembly Functions to get/set EFLAGS
//
FORCEINLINE
VOID
NTAPI
SetEFlags(IN ULONG _Flags)
{
    __asm
    {
        pushfd
        mov eax, _Flags
        and [esp], eax
        popfd
    }
}

//
// Inlined Assembly Functions to get/set TSS, GDT and IDT
//
#if defined(NTOS_KERNEL_RUNTIME) || defined(_BLDR_)
FORCEINLINE
ULONG
NTAPI
GetTssSelector(VOID)
{
    __asm
    {
        xor eax, eax
        str ax
    }
}

FORCEINLINE
PKGDTENTRY
NTAPI
GetGdt(VOID)
{
    KGDTENTRY Gdt;
    __asm sgdt Gdt
    return (PKGDTENTRY)*((PULONG_PTR)&Gdt.BaseLow);
}

FORCEINLINE
VOID
NTAPI
GetGdtr(IN PKDESCRIPTOR Gdtr)
{
    __asm mov eax, Gdtr
    __asm sgdt [eax]
}

FORCEINLINE
VOID
NTAPI
GetIdtr(IN PKDESCRIPTOR Idtr)
{
    __asm mov eax, Idtr
    __asm sidt [eax]
}

FORCEINLINE
VOID
NTAPI
GetTr(IN USHORT Tr)
{
    __asm str Tr
}

FORCEINLINE
VOID
NTAPI
GetLdtr(IN USHORT Ldtr)
{
    __asm sldt Ldtr
}

FORCEINLINE
PKIDTENTRY
NTAPI
GetIdt(VOID)
{
    KGDTENTRY Idt;
    __asm sidt Idt
    return (PKIDTENTRY)*((PULONG_PTR)&Idt.BaseLow);
}

FORCEINLINE
VOID
FASTCALL
SetTr(IN USHORT Selector)
{
    __asm ltr Selector
}

FORCEINLINE
VOID
FASTCALL
SetLdtr(IN USHORT Selector)
{
    __asm lldt Selector
}

FORCEINLINE
VOID
FASTCALL
SetGdtr(IN PKDESCRIPTOR Descriptor)
{
    __asm mov eax, Descriptor
    __asm lgdt [eax]
}

FORCEINLINE
VOID
FASTCALL
SetIdtr(IN PKDESCRIPTOR Descriptor)
{
    __asm mov eax, Descriptor
    __asm lidt [eax]
}
#endif

//
// Inlined Assembly Functions to get/set segment registers
//
FORCEINLINE
USHORT
NTAPI
GetCs(VOID)
{
    __asm
    {
        xor eax, eax
        mov ax, cs
    }
}

FORCEINLINE
USHORT
NTAPI
GetDs(VOID)
{
    __asm
    {
        xor eax, eax
        mov ax, ds
    }
}

FORCEINLINE
USHORT
NTAPI
GetEs(VOID)
{
    __asm
    {
        xor eax, eax
        mov ax, es
    }
}

FORCEINLINE
USHORT
NTAPI
GetFs(VOID)
{
    __asm
    {
        xor eax, eax
        mov ax, fs
    }
}

FORCEINLINE
USHORT
NTAPI
GetGs(VOID)
{
    __asm
    {
        xor eax, eax
        mov ax, gs
    }
}

FORCEINLINE
USHORT
NTAPI
GetSs(VOID)
{
    __asm
    {
        xor eax, eax
        mov ax, ss
    }
}

FORCEINLINE
VOID
NTAPI
SetFs(ULONG _Fs)
{
    __asm
    {
        mov ebx, _Fs
        mov fs, bx
    }
}

FORCEINLINE
VOID
NTAPI
SetDs(ULONG _Ds)
{
    __asm
    {
        mov eax, _Ds
        mov ds, ax
    }
}

FORCEINLINE
VOID
NTAPI
SetEs(ULONG _Es)
{
    __asm
    {
        mov eax, _Es
        mov es, ax
    }
}

//
// Inlined Assembly Functions to get/set DR registers
//
FORCEINLINE
VOID
NTAPI
SetDr0(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov dr0, eax
    }
}

FORCEINLINE
VOID
NTAPI
SetDr1(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov dr1, eax
    }
}

FORCEINLINE
VOID
NTAPI
__writecr2(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov cr2, eax
    }
}

FORCEINLINE
VOID
NTAPI
SetDr2(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov dr2, eax
    }
}

FORCEINLINE
VOID
NTAPI
SetDr3(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov dr3, eax
    }
}

FORCEINLINE
VOID
NTAPI
SetDr6(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov dr6, eax
    }
}

FORCEINLINE
VOID
NTAPI
SetDr7(ULONG Value)
{
    __asm
    {
        mov eax, Value
        mov dr7, eax
    }
}

FORCEINLINE
ULONG
NTAPI
GetDr0(VOID)
{
    __asm mov eax, dr0
}

FORCEINLINE
ULONG
NTAPI
GetDr1(VOID)
{
    __asm mov eax, dr1
}

FORCEINLINE
ULONG
NTAPI
GetDr2(VOID)
{
    __asm mov eax, dr2
}

FORCEINLINE
ULONG
NTAPI
GetDr3(VOID)
{
    __asm mov eax, dr3
}

FORCEINLINE
ULONG
NTAPI
GetDr6(VOID)
{
    __asm mov eax, dr6
}

FORCEINLINE
ULONG
NTAPI
GetDr7(VOID)
{
    __asm  mov eax, dr7
}

//
// For returning from a trap/interrupt
//
FORCEINLINE
VOID
NTAPI
ReturnFromInterrupt(VOID)
{
    __asm iret;
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
#endif
