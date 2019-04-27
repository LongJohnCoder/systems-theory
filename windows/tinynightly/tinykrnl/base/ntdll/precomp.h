/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The NT Layer DLL provides access the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 14-Apr-06

--*/
#define WIN32_NO_STATUS
#define _NTSYSTEM_
#include "stdio.h"
#include "excpt.h"
#include "windef.h"
#include "winnt.h"
#include "ntndk.h"
#include "dpfilter.h"

#define NtCheckPoint()              \
{                                   \
    g_LdrFunction = __FUNCTION__;   \
    g_LdrLine = __LINE__;           \
}

#define NtUnhandled()                           \
{                                               \
    DbgPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                            \
}

//
// Creates the DLL Name Hash
//
#define LDR_GET_HASH_ENTRY(x) \
    (RtlUpcaseUnicodeChar(x - 'A') & 31)

//
// Ldr Count Update Flags
//
#define LDRP_REFCOUNT       0x0001
#define LDRP_DEREFCOUNT     0x0002
#define LDRP_PIN            0x0003

//
// Security Cookie
//
#define PI                  3.141592654
#define LdrpDefaultCookie   (ULONG)(PI  * 1000000000)
#define LdrpDefaultCookie16 (LdrpDefaultCookie >> 16)

//
// Internal TLS Data
//
typedef struct _LDRP_TLS_DATA
{
    LIST_ENTRY TlsLinks;
    IMAGE_TLS_DIRECTORY TlsDirectory;
} LDRP_TLS_DATA, *PLDRP_TLS_DATA;

//
// Internal Current Directory Reference
//
typedef struct _RTLP_CURDIR_REF
{
    LONG Flags;
    HANDLE DirectoryHandle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

//
// Prototypes
//
NTSTATUS
RtlpInitDeferedCriticalSection(
    VOID
);

NTSTATUS
RtlpSecMemFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN PSIZE_T RegionSize,
    IN ULONG FreeType
);

NTSTATUS
RtlInitializeAtomPackage(
    IN ULONG Tag
);

VOID
RtlInitializeHeapManager(
    VOID
);

NTSTATUS
RtlpSecMemFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN PSIZE_T RegionSize,
    IN ULONG FreeType
);

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry(
    IN PVOID BaseAddress
);

VOID
LdrpInsertMemoryTableEntry(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

PVOID
LdrpFetchAddressOfEntryPoint(
    IN PVOID BaseAddress
);

NTSTATUS
LdrpSetProtection(
    IN PVOID BaseAddress,
    IN BOOLEAN ReProtect
);

VOID
LdrpRelocateStartContext(
    IN PCONTEXT Context,
    IN ULONG_PTR Diff
);

VOID
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

VOID
RtlpStkMarkDllRange(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

NTSTATUS
LdrpRunInitializeRoutines(
    IN PCONTEXT Context OPTIONAL
);

VOID
LdrpEnsureLoaderLockIsHeld(
    VOID
);

VOID
LdrpCallTlsInitializers(
    IN PVOID BaseAddress,
    IN ULONG Reason
);

BOOLEAN
LdrpCallInitRoutine(
    IN PVOID InitRoutine,
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
);

LONG
LdrpGenericExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN PCHAR FunctionName
);

VOID
LdrpCheckNXCompatibility(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

NTSTATUS
LdrpWalkImportDescriptor(
    IN LPWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

NTSTATUS
AVrfPageHeapDllNotification(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

NTSTATUS
AVrfDllLoadNotification(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

VOID
AVrfDllUnloadNotification(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

NTSTATUS
LdrpSnapThunk(
    IN PVOID ExportBase,
    IN PVOID ImportBase,
    IN PIMAGE_THUNK_DATA OriginalThunk,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportEntry,
    IN ULONG ExportSize,
    IN BOOLEAN Static,
    IN LPSTR DllName
);

NTSTATUS
LdrpGetProcedureAddress(
    IN PVOID BaseAddress,
    IN PANSI_STRING Name,
    IN ULONG Ordinal,
    OUT PVOID *ProcedureAddress,
    IN BOOLEAN ExecuteInit
);

BOOLEAN
LdrpCheckForLoadedDllHandle(
    IN PVOID Base,
    OUT PLDR_DATA_TABLE_ENTRY *LdrEntry
);

ULONG
LdrpClearLoadInProgress(
    VOID
);

VOID
LdrpUpdateLoadCount2(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
    IN ULONG Flags
);

NTSTATUS
LdrpLoadDll(
    IN BOOLEAN IsolationFlags,
    IN PWCHAR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *BaseAddress,
    IN BOOLEAN CallInit
);

NTSTATUS
LdrpMapDll(
    IN PWCHAR SearchPath OPTIONAL,
    IN PWCHAR *DllPath2,
    IN PWCHAR DllName OPTIONAL,
    IN PULONG DllCharacteristics,
    IN BOOLEAN Static,
    IN BOOLEAN Redirect,
    OUT PLDR_DATA_TABLE_ENTRY *LdrEntry
);

NTSTATUS
LdrQueryImageFileKeyOption(
    IN HKEY KeyHandle,
    IN PCWSTR ValueName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
LdrpAllocateUnicodeString(
    IN OUT PUNICODE_STRING StringOut,
    IN ULONG Length
);

NTSTATUS
LdrpCopyUnicodeString(
    IN OUT PUNICODE_STRING StringOut,
    IN PUNICODE_STRING StringIn
);

VOID
LdrpFreeUnicodeString(
    IN PUNICODE_STRING StringIn
);

NTSTATUS
LdrpCorValidateImage(
    IN PVOID *ViewBase,
    IN PWCHAR ImageName
);

VOID
LdrpCorUnloadImage(
    IN PVOID BaseAddress
);

VOID
LdrpSendDllNotifications(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
    IN BOOLEAN Type,
    IN ULONG Flags
);

BOOLEAN
LdrpCheckForLoadedDll(
    IN PWCHAR SearchPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN Static,
    IN BOOLEAN Relocated,
    OUT PLDR_DATA_TABLE_ENTRY *LdrEntry
);

VOID
LdrpFinalizeAndDeallocateDataTableEntry(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

//
// FIXME: Should go to shim header
//
typedef
BOOLEAN
(*PSHIM_INSTALL)(
     IN PUNICODE_STRING ImageName,
     IN PVOID ShimData
);

typedef
VOID
(*PSHIM_DLL_LOADED)(
     IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

typedef
VOID
(*PSHIM_DLL_UNLOADED)(
     IN PLDR_DATA_TABLE_ENTRY LdrEntry
);

typedef
NTSTATUS
(*PKERNEL32_POST_IMPORT_FUNCTION)(
    VOID
);

typedef
VOID
(*PCOR_IMAGE_UNLOAD)(
     IN PVOID BaseAddress
);

//
// Externals
//
extern PCHAR g_LdrFunction;
extern ULONG g_LdrLine;
extern LIST_ENTRY RtlpCalloutEntryList, RtlpCallbackEntryList;
extern BOOLEAN SecurityCookieInitialized;
extern LONG SecurityCookieInitCount;
extern LIST_ENTRY LdrpDllNotificationList;
extern RTL_CRITICAL_SECTION RtlpCalloutEntryLock;
extern LARGE_INTEGER RtlpTimeout;
extern BOOLEAN RtlpTimeoutDisable;
extern LIST_ENTRY RtlCriticalSectionList;
extern PVOID LdrpHeap;
extern LIST_ENTRY LdrpHashTable[32];
extern PEB_LDR_DATA PebLdr;
extern ULONG LdrpNumberOfProcessors;
extern BOOLEAN LdrpInLdrInit;
extern LONG LdrpFatalHardErrorCount;
extern BOOLEAN ShowSnaps;
extern BOOLEAN LdrpShowInitRoutines;
extern BOOLEAN LdrpImageHasTls;
extern PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
extern BOOLEAN LdrpShutdownInProgress;
extern RTL_CRITICAL_SECTION LdrpLoaderLock;
extern BOOLEAN LdrpBreakOnExceptions;
extern PVOID Kernel32BaseQueryModuleData;
extern LONG LdrpNormalSnap, LdrpSectionCreates, LdrpSectionMaps;
extern LONG LdrpSectionRelocates, LdrpCompareCount, LdrpSectionOpens;
extern BOOLEAN LdrpDisplayLoadTime;
extern PSHIM_DLL_UNLOADED g_pfnSE_DllUnloaded;
extern PSHIM_DLL_LOADED g_pfnSE_DllLoaded;
extern BOOLEAN g_ShimsEnabled;
extern BOOLEAN LdrpLdrDatabaseIsSetup;
extern HANDLE LdrpKnownDllObjectDirectory;
extern UNICODE_STRING LdrpDefaultPath;
extern HKEY LdrpLargePageDLLKey;
extern LARGE_INTEGER BeginTime;
extern UNICODE_STRING LdrpKnownDllPath;
extern RTL_CRITICAL_SECTION FastPebLock;
extern BOOLEAN RtlpDebugPageHeap;
extern ULONG RtlpDphGlobalFlags;
extern PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;
extern HANDLE LdrpShutdownThreadId;