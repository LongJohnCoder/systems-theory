/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ntstub.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 14-Apr-06

--*/
#include "precomp.h"

//
// FIXME: Find a better way to do this
//

#define SYSCALL_STUB(x) __asm               \
{                                           \
    __asm mov eax, x                        \
    __asm mov edx, KUSER_SHARED_SYSCALL     \
    __asm call dword ptr [edx]              \
}

NTSTATUS
NtAllocateVirtualMemory(IN HANDLE ProcessHandle,
                        IN OUT PVOID *BaseAddress,
                        IN ULONG ZeroBits,
                        IN OUT PSIZE_T RegionSize,
                        IN ULONG AllocationType,
                        IN ULONG Protect)
{
    SYSCALL_STUB(0x12);
}

NTSTATUS
ZwAllocateVirtualMemory(IN HANDLE ProcessHandle,
                        IN OUT PVOID *BaseAddress,
                        IN ULONG ZeroBits,
                        IN OUT PSIZE_T RegionSize,
                        IN ULONG AllocationType,
                        IN ULONG Protect)
{
    SYSCALL_STUB(0x12);
}

NTSTATUS
NtClose(IN HANDLE Handle)
{
    SYSCALL_STUB(0x1B);
}

NTSTATUS
ZwClose(IN HANDLE Handle)
{
    SYSCALL_STUB(0x1B);
}

NTSTATUS
NtQueryValueKey(IN HANDLE KeyHandle,
                IN PUNICODE_STRING ValueName,
                IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                OUT PVOID KeyValueInformation,
                IN ULONG Length,
                OUT PULONG ResultLength)
{
    SYSCALL_STUB(0xB9);
}

NTSTATUS
NTAPI
NtSetEventBoostPriority(IN HANDLE EventHandle)
{
    SYSCALL_STUB(0xE5);
}

NTSTATUS
NtReleaseKeyedEvent(IN HANDLE EventHandle,
                    IN PVOID Key,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    SYSCALL_STUB(0x123);
}

NTSTATUS
ZwSetInformationProcess(IN HANDLE ProcessHandle,
                        IN PROCESSINFOCLASS ProcessInformationClass,
                        IN PVOID ProcessInformation,
                        IN ULONG ProcessInformationLength)
{
    SYSCALL_STUB(0xED);
}

NTSTATUS
NtQueryObject(IN HANDLE ObjectHandle,
              IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
              OUT PVOID ObjectInformation,
              IN ULONG Length,
              OUT PULONG ResultLength OPTIONAL)
{
    SYSCALL_STUB(0xAA);
}

NTSTATUS
NtCreateEvent(OUT PHANDLE EventHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes,
              IN EVENT_TYPE EventType,
              IN BOOLEAN InitialState)
{
    SYSCALL_STUB(0x25);
}

NTSTATUS
ZwQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                      OUT PFILE_BASIC_INFORMATION FileInformation)
{
    SYSCALL_STUB(0x91);
}

NTSTATUS
NtQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
                         OUT PVOID SystemInformation,
                         IN SIZE_T SystemInformationLength,
                         OUT PSIZE_T ReturnLength)
{
    SYSCALL_STUB(0xB5);
}

NTSTATUS
ZwQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
                         OUT PVOID SystemInformation,
                         IN SIZE_T SystemInformationLength,
                         OUT PSIZE_T ReturnLength)
{
    SYSCALL_STUB(0xB5);
}

NTSTATUS
NtQueryVolumeInformationFile(IN HANDLE FileHandle,
                             OUT PIO_STATUS_BLOCK IoStatusBlock,
                             OUT PVOID FsInformation,
                             IN ULONG Length,
                             IN FS_INFORMATION_CLASS FsInformationClass)
{
    SYSCALL_STUB(0xBB);
}

NTSTATUS
ZwSetInformationObject(IN HANDLE ObjectHandle,
                       IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
                       IN PVOID ObjectInformation,
                       IN ULONG Length)
{
    SYSCALL_STUB(0xEC);
}

NTSTATUS
NtQueryPerformanceCounter(IN PLARGE_INTEGER Counter,
                          IN PLARGE_INTEGER Frequency)
{
    SYSCALL_STUB(0xAD);
}

NTSTATUS
ZwQuerySymbolicLinkObject(IN HANDLE SymLinkObjHandle,
                          OUT PUNICODE_STRING LinkTarget,
                          OUT PULONG DataWritten OPTIONAL)
{
    SYSCALL_STUB(0xB2);
}

NTSTATUS
NtOpenSymbolicLinkObject(OUT PHANDLE SymbolicLinkHandle,
                         IN ACCESS_MASK DesiredAccess,
                         IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    SYSCALL_STUB(0x85);
}

NTSTATUS
ZwOpenDirectoryObject(OUT PHANDLE FileHandle,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    SYSCALL_STUB(0x77);
}

NTSTATUS
NtSetInformationProcess(IN HANDLE ProcessHandle,
                        IN PROCESSINFOCLASS ProcessInformationClass,
                        IN PVOID ProcessInformation,
                        IN ULONG ProcessInformationLength)
{
    SYSCALL_STUB(0xED);
}

NTSTATUS
NtFlushInstructionCache(IN HANDLE ProcessHandle,
                        IN PVOID BaseAddress,
                        IN ULONG NumberOfBytesToFlush)
{
    SYSCALL_STUB(0x52);
}

NTSTATUS
NtProtectVirtualMemory(IN HANDLE ProcessHandle,
                       IN PVOID *BaseAddress,
                       IN ULONG *NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection)
{
    SYSCALL_STUB(0x8F);
}

NTSTATUS
NtMapViewOfSection(IN HANDLE SectionHandle,
                   IN HANDLE ProcessHandle,
                   IN OUT PVOID *BaseAddress,
                   IN ULONG ZeroBits,
                   IN ULONG CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                   IN OUT PSIZE_T ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType,
                   IN ULONG AccessProtection)
{
    SYSCALL_STUB(0x71);
}

NTSTATUS
ZwMapViewOfSection(IN HANDLE SectionHandle,
                   IN HANDLE ProcessHandle,
                   IN OUT PVOID *BaseAddress,
                   IN ULONG ZeroBits,
                   IN ULONG CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                   IN OUT PSIZE_T ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType,
                   IN ULONG AccessProtection)
{
    SYSCALL_STUB(0x71);
}

NTSTATUS
NtUnmapViewOfSection(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress)
{
    SYSCALL_STUB(0x115);
}

NTSTATUS
ZwUnmapViewOfSection(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress)
{
    SYSCALL_STUB(0x115);
}

NTSTATUS
NtQuerySection(IN HANDLE SectionHandle,
               IN SECTION_INFORMATION_CLASS SectionInformationClass,
               OUT PVOID SectionInformation,
               IN ULONG Length,
               OUT PULONG ResultLength)
{
    SYSCALL_STUB(0xAF);
}

NTSTATUS
NtCreateSection(OUT PHANDLE SectionHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                IN PLARGE_INTEGER MaximumSize OPTIONAL,
                IN ULONG SectionPageProtection OPTIONAL,
                IN ULONG AllocationAttributes,
                IN HANDLE FileHandle OPTIONAL)
{
    SYSCALL_STUB(0x34);
}

NTSTATUS
NtOpenFile(OUT PHANDLE FileHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN ULONG ShareAccess,
           IN ULONG OpenOptions)
{
    SYSCALL_STUB(0x27);
}

NTSTATUS
NtRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    SYSCALL_STUB(0xBE);
}

NTSTATUS
ZwAreMappedFilesTheSame(IN PVOID File1MappedAsAnImage,
                        IN PVOID File2MappedAsFile)
{
    SYSCALL_STUB(0x14);
}

NTSTATUS
NtQueryVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID Address,
                     IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                     OUT PVOID VirtualMemoryInformation,
                     IN ULONG Length,
                     OUT PULONG ResultLength)
{
    SYSCALL_STUB(0xBA);
}

NTSTATUS
NtOpenKeyedEvent(OUT PHANDLE EventHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    SYSCALL_STUB(0x122);
}

NTSTATUS
ZwCallbackReturn(PVOID Result,
                 ULONG ResultLength,
                 NTSTATUS Status)
{
    SYSCALL_STUB(0x16);
}

NTSTATUS
ZwContinue(IN PCONTEXT Context,
           IN BOOLEAN TestAlert)
{
    SYSCALL_STUB(0x22);
}

NTSTATUS
ZwDelayExecution(IN BOOLEAN Alertable,
                 IN LARGE_INTEGER *Interval)
{
    SYSCALL_STUB(0x3D);
}

NTSTATUS
NtQueryDebugFilterState(ULONG ComponentId,
                        ULONG Level)
{
    SYSCALL_STUB(0x94);
}

NTSTATUS
ZwQueryDebugFilterState(ULONG ComponentId,
                        ULONG Level)
{
    SYSCALL_STUB(0x94);
}

NTSTATUS
ZwQueryPerformanceCounter(IN PLARGE_INTEGER Counter,
                          IN PLARGE_INTEGER Frequency)
{
    SYSCALL_STUB(0xAD);
}

NTSTATUS
ZwQueryVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID Address,
                     IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                     OUT PVOID VirtualMemoryInformation,
                     IN ULONG Length,
                     OUT PULONG ResultLength)
{
    SYSCALL_STUB(0xBA);
}

NTSTATUS
ZwRaiseException(IN PEXCEPTION_RECORD ExceptionRecord,
                 IN PCONTEXT Context,
                 IN BOOLEAN SearchFrames)
{
    SYSCALL_STUB(0xBD);
}

NTSTATUS
ZwRaiseHardError(IN NTSTATUS ErrorStatus,
                 IN ULONG NumberOfParameters,
                 IN ULONG UnicodeStringParameterMask,
                 IN PULONG_PTR Parameters,
                 IN ULONG ValidResponseOptions,
                 OUT PULONG Response)
{
    SYSCALL_STUB(0xBE);
}

NTSTATUS
NtTerminateProcess(IN HANDLE ProcessHandle,
                   IN NTSTATUS ExitStatus)
{
    SYSCALL_STUB(0x10A);
}

NTSTATUS
ZwTerminateProcess(IN HANDLE ProcessHandle,
                   IN NTSTATUS ExitStatus)
{
    SYSCALL_STUB(0x10A);
}

NTSTATUS
ZwTestAlert(VOID)
{
    SYSCALL_STUB(0x10C);
}

NTSTATUS
ZwOpenKey(OUT PHANDLE KeyHandle,
          IN ACCESS_MASK DesiredAccess,
          IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    SYSCALL_STUB(0x7D);
}

NTSTATUS
NtOpenSection(OUT PHANDLE SectionHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    SYSCALL_STUB(0x83);
}

NTSTATUS
ZwQueryInformationProcess(IN HANDLE ProcessHandle,
                          IN PROCESSINFOCLASS ProcessInformationClass,
                          OUT PVOID ProcessInformation,
                          IN ULONG ProcessInformationLength,
                          OUT PULONG ReturnLength OPTIONAL)
{
    SYSCALL_STUB(0xA1);
}

NTSTATUS
NtOpenThreadToken(IN HANDLE ThreadHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN BOOLEAN OpenAsSelf,
                  OUT PHANDLE TokenHandle)
{
    SYSCALL_STUB(0x87);
}

NTSTATUS
ZwSetInformationToken(IN HANDLE TokenHandle,
                      IN TOKEN_INFORMATION_CLASS TokenInformationClass,
                      OUT PVOID TokenInformation,
                      IN ULONG TokenInformationLength)
{
    SYSCALL_STUB(0xEF);
}

NTSTATUS
ZwAdjustPrivilegesToken(IN HANDLE TokenHandle,
                        IN BOOLEAN DisableAllPrivileges,
                        IN PTOKEN_PRIVILEGES NewState,
                        IN ULONG BufferLength,
                        OUT PTOKEN_PRIVILEGES PreviousState,
                        OUT PULONG ReturnLength)
{
    SYSCALL_STUB(0xC);
}

NTSTATUS
NtDuplicateToken(IN HANDLE ExistingTokenHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN BOOLEAN EffectiveOnly,
                 IN TOKEN_TYPE TokenType,
                 OUT PHANDLE NewTokenHandle)
{
    SYSCALL_STUB(0x48);
}

NTSTATUS
NtFreeVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID *BaseAddress,
                    IN PSIZE_T RegionSize,
                    IN ULONG FreeType)
{
    SYSCALL_STUB(0x57);
}

NTSTATUS
ZwFreeVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID *BaseAddress,
                    IN PSIZE_T RegionSize,
                    IN ULONG FreeType)
{
    SYSCALL_STUB(0x57);
}

NTSTATUS
ZwOpenProcessToken(IN HANDLE ProcessHandle,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PHANDLE TokenHandle)
{
    SYSCALL_STUB(0x81);
}

NTSTATUS
ZwSetInformationThread(IN HANDLE ThreadHandle,
                       IN THREADINFOCLASS ThreadInformationClass,
                       IN PVOID ThreadInformation,
                       IN ULONG ThreadInformationLength)
{
    SYSCALL_STUB(0xEE);
}

NTSTATUS
NtWaitForKeyedEvent(IN HANDLE EventHandle,
                    IN PVOID Key,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    SYSCALL_STUB(0x124);
}

NTSTATUS
ZwWaitForSingleObject(IN HANDLE Handle,
                      IN BOOLEAN Alertable,
                      IN PLARGE_INTEGER Timeout OPTIONAL)
{
    SYSCALL_STUB(0x119);
}
