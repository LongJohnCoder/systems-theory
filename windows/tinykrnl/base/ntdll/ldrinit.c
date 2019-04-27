/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ldrinit.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 14-Apr-2006

--*/
#include "precomp.h"

//
// Initialization states
//
LONG LdrpFatalHardErrorCount;
LONG ProcessInitialized;
LONG LdrpProcessInitialized;
BOOLEAN LdrpInLdrInit;
BOOLEAN LdrpLdrDatabaseIsSetup;
BOOLEAN LdrpShutdownInProgress;

//
// Shim Engine Variables
//
BOOLEAN g_ShimsEnabled;
PSHIM_INSTALL g_pfnSE_InstallAfterInit;
PSHIM_INSTALL g_pfnSE_InstallBeforeInit;
PSHIM_DLL_LOADED g_pfnSE_DllLoaded;
PSHIM_DLL_UNLOADED g_pfnSE_DllUnloaded;
HANDLE g_pShimEngineModule;

//
// .NET Information
//
ULONG CorImageCount;
HANDLE Cor20DllHandle;
PCOR_IMAGE_UNLOAD CorImageUnloading;

//
// Debugging and Profiling Data
//
//#if DBG
PCHAR g_LdrFunction;
ULONG g_LdrLine;
BOOLEAN g_LdrBreakOnLdrpInitializeProcessFailure;
BOOLEAN LdrpDisplayLoadTime;
LARGE_INTEGER BeginTime, Interval, EndTime, ElapsedTime, InitbTime;
LONG LdrpCompareCount, LdrpSnapBypass, LdrpNormalSnap, LdrpSectionOpens;
LONG LdrpSectionCreates, LdrpSectionMaps, LdrpSectionRelocates;
BOOLEAN LdrpShouldCreateStackTraceDb;
BOOLEAN LdrpShowInitRoutines;
BOOLEAN LdrpBreakOnExceptions;
//#endif
BOOLEAN ShowSnaps;
BOOLEAN LdrpVerifyDlls;

//
// Loader lock variables
//
RTL_CRITICAL_SECTION_DEBUG LoaderLockDebug;
RTL_CRITICAL_SECTION LdrpLoaderLock =
{
    &LoaderLockDebug,
    -1,
    0,
};
BOOLEAN LoaderLockInitialized;

//
// Process state variables
//
PEB_LDR_DATA PebLdr;
PLDR_DATA_TABLE_ENTRY LdrpImageEntry, LdrpNtDllDataTableEntry;
UNICODE_STRING LdrpDefaultPath, LdrpKnownDllPath;
RTL_BITMAP FlsBitMap, TlsBitMap, TlsExpansionBitMap;
LIST_ENTRY LdrpHashTable[32];
RTL_CRITICAL_SECTION FastPebLock;
LONG LdrpNumberOfTlsEntries;
LIST_ENTRY LdrpTlsList;
BOOLEAN LdrpImageHasTls;

//
// Internal state variables
//
PVOID LdrpHeap;
PVOID NtDllBase;
UNICODE_STRING NtDllName =
    RTL_CONSTANT_STRING(L"ntdll.dll");
UNICODE_STRING SlashSystem32SlashString =
    RTL_CONSTANT_STRING(L"\\system32\\mscoree.dll");
UNICODE_STRING KnownDllString =
    RTL_CONSTANT_STRING(L"\\KnownDlls");
UNICODE_STRING KnownDllPathString =
    RTL_CONSTANT_STRING(L"KnownDllPath");
ULONG LdrpNumberOfProcessors;
ULONG NtdllBaseTag;
WCHAR FullMsCorePath[MAX_PATH];
WCHAR LdrpKnownDllPathBuffer[MAX_PATH];
HANDLE LdrpKnownDllObjectDirectory;
HKEY LdrpLargePageDLLKey;
PVOID RtlpUnhandledExceptionFilter;
HKEY Wow64ExecOptionsKey, ImageExecOptionsKey;
UNICODE_STRING ImageExecOptionsString = 
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT"
                        L"\\CurrentVersion\\Image File Execution Options\\");
UNICODE_STRING Wow64OptionsString = 
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft"
                        L"\\Windows NT\\CurrentVersion\\Image File Execution "
                        L"Options\\");

/*++
* @name RtlSetUnhandledExceptionFilter
*
* The RtlSetUnhandledExceptionFilter routine FILLMEIN
*
* @param TopLevelExceptionFilter
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlSetUnhandledExceptionFilter(IN PVOID TopLevelExceptionFilter)
{
    //
    // Encode the filter and save it, returning the encoded pointer
    //
    RtlpUnhandledExceptionFilter = RtlEncodePointer(TopLevelExceptionFilter);
    return RtlpUnhandledExceptionFilter;
}

/*++
* @name InitSecurityCookie
*
* The InitSecurityCookie routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
InitSecurityCookie(VOID)
{
    LARGE_INTEGER Timeout;

    //
    // Try to change the status and get the previous status value
    //
    if ((_InterlockedExchangeAdd(&SecurityCookieInitCount, 1) + 1) != 1)
    {
        //
        //
        // Set the timeout to 30 seconds
        //
        Timeout.QuadPart = Int32x32To64(30, -10000);

        //
        // Make sure the status hasn't changed
        //
        while (!SecurityCookieInitialized)
        {
            //
            // Do the wait
            //
            ZwDelayExecution(FALSE, &Timeout);
        }
    }
    else
    {
        //
        // We've initialized!
        //
        SecurityCookieInitialized = 1;
    }
}

/*++
* @name LdrpProcessInitializationComplete
*
* The LdrpProcessInitializationComplete routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpProcessInitializationComplete(VOID)
{
    //
    // Sanity check
    //
    ASSERT(LdrpProcessInitialized == 1);

    //
    // Set the process as Initialized
    //
    _InterlockedIncrement(&LdrpProcessInitialized);
}

/*++
* @name LdrpInitializationFailure
*
* The LdrpInitializationFailure routine FILLMEIN
*
* @param Status
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpInitializationFailure(IN NTSTATUS Status)
{
    ULONG_PTR ErrorParameter;
    ULONG ErrorResponse;

    //
    // Print our our last checkpoint
    //
    DbgPrint("LDR: Process initialization failure; NTSTATUS = %08lx\n"
             "     Function: %s\n"
             "     Line: %d\n",
             Status,
             g_LdrFunction,
             g_LdrLine);

    //
    // Make sure we haven't already done this once
    //
    if (!LdrpFatalHardErrorCount)
    {
        //
        // Setup the hard error
        //
        ZwRaiseHardError(STATUS_APP_INIT_FAILURE,
                         1,
                         0,
                         &ErrorParameter,
                         OptionOk,
                         &ErrorResponse);
    }
}

/*++
* @name LdrpInitializationException
*
* The LdrpInitializationException routine FILLMEIN
*
* @param ExceptionInfo
*        FILLMEIN
*
* @return LONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
LONG
LdrpInitializationException(IN PEXCEPTION_POINTERS ExceptionInfo)
{
    //
    // Print our our last checkpoint
    //
    DbgPrint("LDR: LdrpInitializeProcess() threw an exception: %lu (0x%08lx)\n"
             "     Exception record: .exr %p\n"
             "     Context record: .cxr %p\n",
             ExceptionInfo->ExceptionRecord->ExceptionCode,
             ExceptionInfo->ExceptionRecord->ExceptionCode,
             ExceptionInfo->ExceptionRecord,
             ExceptionInfo->ContextRecord);

    //
    // Print our last checkpoint
    //
    DbgPrint("     Last checkpoint: %s line %d\n",
             g_LdrFunction,
             g_LdrLine);

    //
    // Check if we should break and return
    //
    if (g_LdrBreakOnLdrpInitializeProcessFailure) DbgBreakPoint();
    return EXCEPTION_EXECUTE_HANDLER;
}

/*++
* @name LdrpCorValidateImage
*
* The LdrpCorValidateImage routine FILLMEIN
*
* @param ViewBase
*        FILLMEIN
*
* @param ImageName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpCorValidateImage(IN PVOID *ViewBase,
                     IN PWCHAR ImageName)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name LdrpCorUnloadImage
*
* The LdrpCorUnloadImage routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpCorUnloadImage(IN PVOID BaseAddress)
{
    //
    // Unload it
    //
    CorImageUnloading(BaseAddress);

    //
    // Check if it's the last one and unload mscoree.dll, if so
    //
    if (!(--CorImageCount)) LdrUnloadDll(Cor20DllHandle);
}

/*++
* @name LdrpGetShimEngineInterface
*
* The LdrpGetShimEngineInterface routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpGetShimEngineInterface(VOID)
{
    //
    // FIXME: TODO
    //
}

/*++
* @name LdrpRunShimEngineInitRoutine
*
* The LdrpRunShimEngineInitRoutine routine FILLMEIN
*
* @param Initialize
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrpRunShimEngineInitRoutine(IN BOOLEAN Initialize)
{
    //
    // FIXME: TODO
    //
    return FALSE;
}

/*++
* @name LdrpLoadShimEngine
*
* The LdrpLoadShimEngine routine FILLMEIN
*
* @param ShimEngineName
*        FILLMEIN
*
* @param ImageName
*        FILLMEIN
*
* @param ShimData
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpLoadShimEngine(IN PVOID ShimEngineName,
                   IN PUNICODE_STRING ImageName,
                   IN PVOID ShimData)
{
    UNICODE_STRING EngineName;
    NTSTATUS Status;
    PSHIM_INSTALL ShimInstall;

    //
    // Create the name
    //
    RtlInitUnicodeString(&EngineName, (PCWSTR)ShimEngineName);

    //
    // Load it
    //
    Status = LdrpLoadDll(0,
                         NULL,
                         NULL,
                         &EngineName,
                         &g_pShimEngineModule,
                         FALSE);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrint("LDR: Couldn't load the shim engine\n");
        return;
    }

    //
    // Get the interface pointers
    //
    LdrpGetShimEngineInterface();

    //
    // Call the Initialization routine
    //
    if (!LdrpRunShimEngineInitRoutine(TRUE))
    {
        //
        // Fail
        //
        DbgPrint("LDR: Couldn't load the shim engine\n");
        return;
    }

    //
    // If shims are enabled
    //
    if (g_ShimsEnabled)
    {
        //
        // Call the pre-init function
        //
        ShimInstall = RtlDecodeSystemPointer(g_pfnSE_InstallBeforeInit);
        ShimInstall(ImageName, ShimData);
    }
}

/*++
* @name LdrpUnloadShimEngine
*
* The LdrpUnloadShimEngine routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpUnloadShimEngine(VOID)
{
    //
    // Disable shims
    //
    g_ShimsEnabled = FALSE;

    //
    // Run the init routine
    //
    if (!LdrpRunShimEngineInitRoutine(FALSE))
    {
        //
        // Print out status message
        //
        DbgPrint("LDR: Failed to call init routine for shim engine\n");
    }

    //
    // Unload the Shim Engine
    //
    LdrUnloadDll(g_pShimEngineModule);
    g_pShimEngineModule = NULL;
}

/*++
* @name LdrpCheckForSecuROMImage
*
* The LdrpCheckForSecuROMImage routine FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrpCheckForSecuROMImage(IN PIMAGE_NT_HEADERS NtHeader)
{
    //
    // FIXME: TODO
    //
    return FALSE;
}

/*++
* @name LdrpCheckForNXEntryAddress
*
* The LdrpCheckForNXEntryAddress routine FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrpCheckForNXEntryAddress(IN PIMAGE_NT_HEADERS NtHeader)
{
    //
    // FIXME: TODO
    //
    return FALSE;
}

/*++
* @name LdrpCheckForSafeDiscImage
*
* The LdrpCheckForSafeDiscImage routine FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrpCheckForSafeDiscImage(IN PIMAGE_NT_HEADERS NtHeader)
{
    //
    // FIXME: TODO
    //
    return FALSE;
}

/*++
* @name LdrpForkProcess
*
* The LdrpForkProcess routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpForkProcess(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name LdrpTouchThreadStack
*
* The LdrpTouchThreadStack routine FILLMEIN
*
* @param MaximumStackCommit
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpTouchThreadStack(IN ULONG MaximumStackCommit)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name LdrOpenImageFileOptionsKey
*
* The LdrOpenImageFileOptionsKey routine FILLMEIN
*
* @param SubKey
*        FILLMEIN
*
* @param KeyType
*        FILLMEIN
*
* @param NewKeyHandle
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(IN PUNICODE_STRING SubKey,
                           IN ULONG KeyType,
                           OUT PHKEY NewKeyHandle)
{
    PHKEY RootKeyLocation;
    HKEY RootKey;
    UNICODE_STRING SubKeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWCHAR p1;
    NTSTATUS Status;

    //
    // Check which root key to open
    //
    if (KeyType)
    {
        //
        // Use the WOW64 Key
        //
        RootKeyLocation = &Wow64ExecOptionsKey;
    }
    else
    {
        //
        // Use the normal NT Key
        //
        RootKeyLocation = &ImageExecOptionsKey;
    }

    //
    // Get the current key
    //
    RootKey = *RootKeyLocation;

    //
    // Setup the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               KeyType ? 
                               &Wow64OptionsString : &ImageExecOptionsString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the root key
    //
    Status = ZwOpenKey(&RootKey, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        //
        // Write the key handle
        //
        if (_InterlockedCompareExchange((PLONG)RootKeyLocation,
                                        PtrToLong(RootKey),
                                        0))
        {
            //
            // Someone already opened it, use it instead
            //
            NtClose(RootKey);
            RootKey = *RootKeyLocation;
        }

        //
        // Extract the name
        //
        SubKeyString = *SubKey;
        p1 = (PWCHAR)((ULONG_PTR)SubKeyString.Buffer + SubKeyString.Length);

        //
        // Loop it
        //
        while (SubKeyString.Length)
        {
            //
            // Break out if we find a backslash
            //
            if ((*p1--) == OBJ_NAME_PATH_SEPARATOR) break;
            
            //
            // Advance in the string
            //
            p1--;
            SubKeyString.Length -= sizeof(WCHAR);
        }

        //
        // Set the name up until the slash
        //
        SubKeyString.Buffer = p1;
        SubKeyString.Length = SubKeyString.MaximumLength - SubKeyString.Length;

        //
        // Setup the object attributes
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyString,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKey,
                                   NULL);

        //
        // Open the setting key
        //
        Status = ZwOpenKey(NewKeyHandle, GENERIC_READ, &ObjectAttributes);
    }

    //
    // Return to caller
    //
    return Status;
}

/*++
* @name LdrQueryImageFileKeyOption
*
* The LdrQueryImageFileKeyOption routine FILLMEIN
*
* @param KeyHandle
*        FILLMEIN
*
* @param ValueName
*        FILLMEIN
*
* @param Type
*        FILLMEIN
*
* @param Buffer
*        FILLMEIN
*
* @param BufferSize
*        FILLMEIN
*
* @param ResultLength
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrQueryImageFileKeyOption(IN HKEY KeyHandle,
                           IN PCWSTR ValueName,
                           IN ULONG Type,
                           OUT PVOID Buffer,
                           IN ULONG BufferSize,
                           OUT PULONG ResultLength OPTIONAL)
{
    UCHAR KeyInfo[256];
    UNICODE_STRING ValueNameString;
    ULONG ResultSize;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation =
        (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;
    BOOLEAN FreeHeap = FALSE;
    NTSTATUS Status;
    ULONG KeyInfoSize;
    UNICODE_STRING IntegerString;

    //
    // Build a string for the value name
    //
    Status = RtlInitUnicodeStringEx(&ValueNameString, ValueName);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Query the value
    //
    Status = NtQueryValueKey(KeyHandle,
                             &ValueNameString,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyInfo),
                             &ResultSize);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        //
        // Our local buffer wasn't enough, allocate one
        //
        KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                      KeyValueInformation->DataLength;
        KeyValueInformation = RtlAllocateHeap(RtlGetProcessHeap(),
                                              0,
                                              KeyInfoSize);
        if (!KeyInfo) return STATUS_NO_MEMORY;

        //
        // Try again
        //
        Status = NtQueryValueKey(KeyHandle,
                                 &ValueNameString,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 KeyInfoSize,
                                 &ResultSize);
        FreeHeap = TRUE;
    }

    //
    // Check for success
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Handle binary data
        //
        if (KeyValueInformation->Type == REG_BINARY)
        {
            //
            // Check validity
            //
            if ((Buffer) && (KeyValueInformation->DataLength <= BufferSize))
            {
                //
                // Copy into buffer
                //
                RtlMoveMemory(Buffer,
                              &KeyValueInformation->Data,
                              KeyValueInformation->DataLength);
            }
            else
            {
                //
                // Fail
                //
                Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            // Copy the result length
            //
            if (ResultLength) *ResultLength = KeyValueInformation->DataLength;
        }
        else if (KeyValueInformation->Type == REG_DWORD)
        {
            //
            // Check for valid type
            //
            if (KeyValueInformation->Type != Type)
            {
                //
                // Error
                //
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else
            {
                //
                // Check validity
                //
                if ((Buffer) &&
                    (BufferSize == sizeof(ULONG)) &&
                    (KeyValueInformation->DataLength <= BufferSize))
                {
                    //
                    // Copy into buffer
                    //
                    RtlMoveMemory(Buffer,
                                  &KeyValueInformation->Data,
                                  KeyValueInformation->DataLength);
                }
                else
                {
                    //
                    // Fail
                    //
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                //
                // Copy the result length
                //
                if (ResultLength) *ResultLength = KeyValueInformation->DataLength;
            }
        }
        else if (KeyValueInformation->Type != REG_SZ)
        {
            //
            // We got something weird
            //
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else
        {
            //
            // String, check what you requested
            //
            if (Type == REG_DWORD)
            {
                //
                // Validate
                //
                if (BufferSize != sizeof(ULONG))
                {
                    //
                    // Invalid size
                    //
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    //
                    // OK, we know what you want...
                    //
                    IntegerString.Buffer = (PWCHAR)&KeyValueInformation->Data;
                    IntegerString.Length = (USHORT)KeyValueInformation->
                                            DataLength - sizeof(WCHAR);
                    IntegerString.MaximumLength = (USHORT)KeyValueInformation->
                                                          DataLength;
                    Status = RtlUnicodeStringToInteger(&IntegerString,
                                                       0,
                                                       Buffer);
                }
            }
            else
            {
                //
                // Validate
                //
                if (KeyValueInformation->DataLength > BufferSize)
                {
                    //
                    // Invalid
                    //
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    //
                    // Set the size
                    //
                    BufferSize = KeyValueInformation->DataLength;
                }

                //
                // Copy the string
                //
                RtlMoveMemory(Buffer, &KeyValueInformation->Data, BufferSize);
            }

            //
            // Copy the result length
            //
            if (ResultLength) *ResultLength = KeyValueInformation->DataLength;
        }
    }

    //
    // Check if buffer was in heap
    //
    if (FreeHeap) RtlFreeHeap(RtlGetProcessHeap(), 0, KeyValueInformation);

    //
    // Close key and return
    //
    NtClose(KeyHandle);
    return Status;
}

/*++
* @name LdrQueryImageFileExecutionOptionsEx
*
* The LdrQueryImageFileExecutionOptionsEx routine FILLMEIN
*
* @param SubKey
*        FILLMEIN
*
* @param ValueName
*        FILLMEIN
*
* @param ValueSize
*        FILLMEIN
*
* @param Buffer
*        FILLMEIN
*
* @param BufferSize
*        FILLMEIN
*
* @param ReturnedLength
*        FILLMEIN
*
* @param KeyType
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptionsEx(IN PUNICODE_STRING SubKey,
                                    IN PCWSTR ValueName,
                                    IN ULONG ValueSize,
                                    OUT PVOID Buffer,
                                    IN ULONG BufferSize,
                                    OUT PULONG ReturnedLength OPTIONAL,
                                    IN ULONG KeyType)
{
    NTSTATUS Status;
    HKEY KeyHandle;

    //
    // Open the key
    //
    Status = LdrOpenImageFileOptionsKey(SubKey, KeyType, &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        //
        // Query the option
        //
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            ValueName,
                                            ValueSize,
                                            Buffer,
                                            BufferSize,
                                            ReturnedLength);

        //
        // Close the handle
        //
        NtClose(KeyHandle);
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrQueryImageFileExecutionOptions
*
* The LdrQueryImageFileExecutionOptions routine FILLMEIN
*
* @param SubKey
*        FILLMEIN
*
* @param ValueName
*        FILLMEIN
*
* @param ValueSize
*        FILLMEIN
*
* @param Buffer
*        FILLMEIN
*
* @param BufferSize
*        FILLMEIN
*
* @param ReturnedLength
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(IN PUNICODE_STRING SubKey,
                                  IN PCWSTR ValueName,
                                  IN ULONG ValueSize,
                                  OUT PVOID Buffer,
                                  IN ULONG BufferSize,
                                  OUT PULONG ReturnedLength OPTIONAL)
{
    //
    // Call the newer function
    //
    return LdrQueryImageFileExecutionOptionsEx(SubKey,
                                               ValueName,
                                               ValueSize,
                                               Buffer,
                                               BufferSize,
                                               ReturnedLength,
                                               0);
}

/*++
* @name LdrpInitializeExecutionOptions
*
* The LdrpInitializeExecutionOptions routine FILLMEIN
*
* @param ImageName
*        FILLMEIN
*
* @param Peb
*        FILLMEIN
*
* @param OptionsKey
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpInitializeExecutionOptions(IN PUNICODE_STRING ImageName,
                               IN PPEB Peb,
                               IN PHKEY OptionsKey)
{
    //
    // FIXME: TODO
    //
    *OptionsKey = NULL;
    return STATUS_SUCCESS; // so we can still load
}

/*++
* @name LdrpAllocateTls
*
* The LdrpAllocateTls routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpAllocateTls(VOID)
{
    PTEB Teb = NtCurrentTeb();
    PLIST_ENTRY NextEntry, ListHead;
    PLDRP_TLS_DATA TlsData;
    PVOID *TlsVector;
    ULONG TlsDataSize;

    //
    // Check if we have any entries
    //
    if (LdrpNumberOfTlsEntries)
    {
        //
        // Allocate the vector array
        //
        TlsVector = RtlAllocateHeap(RtlGetProcessHeap(),
                                    0,
                                    LdrpNumberOfTlsEntries * sizeof(PVOID));
        if (!TlsVector) return STATUS_NO_MEMORY;
        Teb->ThreadLocalStoragePointer = TlsVector;

        //
        // Loop the TLS Array
        //
        ListHead = &LdrpTlsList;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            //
            // Get the entry
            //
            TlsData = CONTAINING_RECORD(NextEntry, LDRP_TLS_DATA, TlsLinks);
            NextEntry = NextEntry->Flink;

            //
            // Allocate this vector
            //
            TlsDataSize = TlsData->TlsDirectory.EndAddressOfRawData -
                          TlsData->TlsDirectory.StartAddressOfRawData;
            TlsVector[TlsData->TlsDirectory.Characteristics] =
                RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                TlsDataSize);
            if (!TlsVector[TlsData->TlsDirectory.Characteristics])
            {
                //
                // Out of memory
                //
                return STATUS_NO_MEMORY;
            }

            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: TlsVector %x Index %d = %x copied from %x to "
                         "%x\n",
                         TlsVector,
                         TlsData->TlsDirectory.Characteristics,
                         &TlsVector[TlsData->TlsDirectory.Characteristics],
                         TlsData->TlsDirectory.StartAddressOfRawData,
                         TlsVector[TlsData->TlsDirectory.Characteristics]);
            }

            //
            // Copy the data
            //
            RtlCopyMemory(TlsVector[TlsData->TlsDirectory.Characteristics],
                          UlongToPtr(TlsData->TlsDirectory.StartAddressOfRawData),
                          TlsDataSize);
        }
    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

/*++
* @name LdrpCallTlsInitializers
*
* The LdrpCallTlsInitializers routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param Reason
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpCallTlsInitializers(PVOID BaseAddress,
                        ULONG Reason)
{
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    PIMAGE_TLS_CALLBACK *Array, Callback;
    ULONG Size;

    //
    // Get the TLS Directory
    //
    TlsDirectory = RtlImageDirectoryEntryToData(BaseAddress,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_TLS,
                                                &Size);

    //
    // Protect against invalid pointers
    //
    __try
    {
        //
        // Make sure it's valid
        //
        if (TlsDirectory)
        {
            //
            // Get the array
            //
            Array = (PIMAGE_TLS_CALLBACK*)UlongToPtr(TlsDirectory->
                                                     AddressOfCallBacks);
            if (Array)
            {
                //
                // Display debug message
                //
                if (ShowSnaps)
                {
                    DbgPrint("LDR: Tls Callbacks Found. Imagebase %lx Tls %lx "
                             "CallBacks %lx\n",
                             BaseAddress,
                             TlsDirectory,
                             Array);
                }

                //
                // Loop the array
                //
                while (*Array)
                {
                    //
                    // Get the TLS Entrypoint
                    //
                    Callback = *Array++;

                    //
                    // Display debug message
                    //
                    if (ShowSnaps)
                    {
                        DbgPrint("LDR: Calling Tls Callback Imagebase %lx "
                                 "Function %lx\n",
                                 BaseAddress,
                                 Callback);
                    }

                    //
                    // Call it
                    //
                    LdrpCallInitRoutine(Callback, BaseAddress, Reason, 0);
                }
            }
        }
    }
    __except (LdrpGenericExceptionFilter(GetExceptionInformation(),
                                         __FUNCTION__))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - caught exception %08lx calling TLS callbacks\n",
                   __FUNCTION__,
                   GetExceptionCode());
    }
}

/*++
* @name LdrpInitializeTls
*
* The LdrpInitializeTls routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpInitializeTls(VOID)
{
    PLIST_ENTRY NextEntry, ListHead;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    PLDRP_TLS_DATA TlsData;
    ULONG Size;

    //
    // Initialize the TLS List
    //
    InitializeListHead(&LdrpTlsList);

    //
    // Loop all the modules
    //
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the entry
        //
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
        NextEntry = NextEntry->Flink;

        //
        // Get the TLS directory
        //
        TlsDirectory = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_TLS,
                                                    &Size);

        //
        // Check if we have a directory
        //
        if (TlsDirectory)
        {
            //
            // Check if the image has TLS
            //
            if (!LdrpImageHasTls) LdrpImageHasTls = TRUE;
            
            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: Tls Found in %wZ at %lx\n",
                         &LdrEntry->BaseDllName,
                         TlsDirectory);
            }

            //
            // Allocate an entry
            //
            TlsData = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      sizeof(LDRP_TLS_DATA));
            if (!TlsData) return STATUS_NO_MEMORY;

            //
            // Lock the DLL and mark it for TLS Usage
            //
            LdrEntry->LoadCount = -1;
            LdrEntry->TlsIndex = -1;

            //
            // Save the cached TLS data
            //
            TlsData->TlsDirectory = *TlsDirectory;
            InsertTailList(&LdrpTlsList, &TlsData->TlsLinks);

            //
            // Update the index
            //
            *(PLONG)UlongToPtr(TlsData->TlsDirectory.AddressOfIndex) =
                LdrpNumberOfTlsEntries;
            TlsData->TlsDirectory.Characteristics = LdrpNumberOfTlsEntries++;
        }
    }

    //
    // Done setting up TLS, allocate entries
    //
    return LdrpAllocateTls();
}

/*++
* @name LdrQueryApplicationCompatibilityGoo
*
* The LdrQueryApplicationCompatibilityGoo routine FILLMEIN
*
* @param OptionsKey
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrQueryApplicationCompatibilityGoo(IN HKEY OptionsKey)
{
    //
    // FIXME: TODO
    //
}

/*++
* @name LdrpInitializeProcess
*
* The LdrpInitializeProcess routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @param SystemArgument1
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpInitializeProcess(IN PCONTEXT Context,
                      IN PVOID SystemArgument1)
{
    ANSI_STRING FunctionName = RTL_CONSTANT_STRING("BaseQueryModuleData");
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = Teb->ProcessEnvironmentBlock;
    PWCHAR ImagePath;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING ImagePathName, ImageFileName, CommandLine, SystemRoot;
    NTSTATUS Status;
    HKEY OptionsKey;
    BOOLEAN IsDotNetImage;
    ULONG ComSectionSize = 0, ConfigSectionSize = 0;
    PVOID OldShimData;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    NLSTABLEINFO NlsTable;
    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig;
    RTL_HEAP_PARAMETERS HeapParameters, PrivateHeapParameters;
    ULONG HeapFlags;
    ULONG DebugProcessHeapOnly;
    UNICODE_STRING FullPath, CurrentDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SymLinkHandle;
    BOOLEAN CurDirWasAllocated;
    PLDR_DATA_TABLE_ENTRY NtLdrEntry;
    PLIST_ENTRY NextEntry, ListHead;
    PWCHAR BaseName, Current;
    ULONG ExecuteOptions;
    PSHIM_INSTALL ShimInstall;

    //
    // Set a NULL SEH Filter
    //
    NtCheckPoint();
    RtlSetUnhandledExceptionFilter(NULL);

    //
    // Get the image path
    //
    ImagePath = Peb->ProcessParameters->ImagePathName.Buffer;

    //
    // Check if it's normalized
    //
    if (Peb->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_NORMALIZED)
    {
        //
        // Normalize it
        //
        (ULONG_PTR)ImagePath += (ULONG_PTR)Peb->ProcessParameters;
    }

    //
    // Create a unicode string for the Image Path
    //
    ImagePathName.Length = Peb->ProcessParameters->ImagePathName.Length;
    ImagePathName.MaximumLength = ImagePathName.Length + sizeof(WCHAR);
    ImagePathName.Buffer = ImagePath;

    //
    // Get the NT Headers
    //
    NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);
    if (!NtHeader)
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failing because we were unable to map the image "
                   "base address (%p) to the PIMAGE_NT_HEADERS\n",
                   __FUNCTION__,
                   Peb->ImageBaseAddress);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Get the execution options
    //
    Status = LdrpInitializeExecutionOptions(&ImagePathName, Peb, &OptionsKey);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failing because we were unable to map apply the "
                   "execution options (ntstatus %x)\n",
                   __FUNCTION__,
                   Status);
        return Status;
    }

    //
    // Check if this is a .NET executable
    //
    if (RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                     IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                     TRUE,
                                     &ComSectionSize))
    {
        //
        // Remeber this for later
        //
        IsDotNetImage = TRUE;
    }

    //
    // Sanity check
    //
    NtCheckPoint();
    ASSERT(Peb->Ldr == NULL);

    //
    // Save the NTDLL Base address
    //
    NtDllBase = SystemArgument1;

    //
    // If this is a Native Image
    //
    if (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE)
    {
        //
        // Then do DLL Validation
        //
        LdrpVerifyDlls = TRUE;
    }

    //
    // Save the old Shim Data and clear it
    //
    OldShimData = Peb->pShimData;
    Peb->pShimData = NULL;

    //
    // Save the number of processors and CS Timeout
    //
    NtCheckPoint();
    LdrpNumberOfProcessors = Peb->NumberOfProcessors;
    RtlpTimeout = Peb->CriticalSectionTimeout;

    //
    // Normalize the parameters
    //
    if (ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters))
    {
        //
        // Save the Image and Command Line Names
        //
        ImageFileName = *(PUNICODE_STRING)&ProcessParameters->ImagePathName;
        CommandLine = *(PUNICODE_STRING)&ProcessParameters->CommandLine;
    }
    else
    {
        //
        // It failed, initialize empty strings
        //
        RtlInitEmptyUnicodeString(&ImageFileName, NULL, 0);
        RtlInitEmptyUnicodeString(&CommandLine, NULL, 0);
    }

    //
    // Initialize NLS data
    //
    RtlInitNlsTables(Peb->AnsiCodePageData,
                     Peb->OemCodePageData,
                     Peb->UnicodeCaseTableData,
                     &NlsTable);

    //
    // Reset NLS Translations
    //
    RtlResetRtlTranslations(&NlsTable);

    //
    // Get the Image Config Directory
    //
    LoadConfig = RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                              &ConfigSectionSize);

    //
    // Setup the Heap Parameters
    //
    RtlZeroMemory(&HeapParameters, sizeof(RTL_HEAP_PARAMETERS));
    HeapFlags = HEAP_GROWABLE;
    HeapParameters.Length = sizeof(RTL_HEAP_PARAMETERS);

    //
    // Check if we have Configuration Data
    //
    if ((LoadConfig) &&
        (ConfigSectionSize == sizeof(IMAGE_LOAD_CONFIG_DIRECTORY)))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check for custom affinity mask
    //
    NtCheckPoint();
    if (Peb->ImageProcessAffinityMask)
    {
        //
        // Set it
        //
        Status = NtSetInformationProcess(NtCurrentProcess(),
                                         ProcessAffinityMask,
                                         &Peb->ImageProcessAffinityMask,
                                         sizeof(KAFFINITY));
        if (NT_SUCCESS(Status))
        {
            //
            // Notify trace
            //
            DbgPrint("LDR: Using ProcessAffinityMask of 0x%Ix from image.\n",
                     Peb->ImageProcessAffinityMask);
        }
        else
        {
            //
            // Notify
            //
            DbgPrint("LDR: Failed to set ProcessAffinityMask of 0x%Ix from "
                     "image (Status == %08x)\n",
                     Peb->ImageProcessAffinityMask,
                     Status);
        }
    }

    //
    // Check if verbose debugging was requetsed
    //
    if (ShowSnaps = (BOOLEAN)(Peb->NtGlobalFlag & FLG_SHOW_LDR_SNAPS))
    {
        DbgPrint("LDR: PID: %0x%x started - '%wZ'\n",
                 Teb->Cid.UniqueProcess,
                 &CommandLine);
    }

    //
    // If the timeout is too long
    //
    NtCheckPoint()
    if (RtlpTimeout.QuadPart < Int32x32To64(3600, -10000000))
    {
        //
        // Then disable CS Timeout
        //
        RtlpTimeoutDisable = TRUE;
    }

    //
    // Initialize Critical Section Data
    //
    Status = RtlpInitDeferedCriticalSection();
    if (NT_SUCCESS(Status)) return Status;

    //
    // Set TLS/FLS Bitmap data
    //
    Peb->FlsBitmap = &FlsBitMap;
    Peb->TlsBitmap = &TlsBitMap;
    Peb->TlsExpansionBitmap = &TlsExpansionBitMap;

    //
    // Initialize FLS Bitmap
    //
    RtlInitializeBitMap(&FlsBitMap,
                        Peb->FlsBitmapBits,
                        FLS_MAXIMUM_AVAILABLE);
    RtlSetBit(&FlsBitMap, 0);

    //
    // Initialize TLS Bitmaps
    //
    RtlInitializeBitMap(&TlsBitMap,
                        Peb->TlsBitmapBits,
                        TLS_MINIMUM_AVAILABLE);
    RtlSetBit(&TlsBitMap, 0);
    RtlInitializeBitMap(&TlsExpansionBitMap,
                        Peb->TlsExpansionBitmapBits,
                        TLS_EXPANSION_SLOTS);
    RtlSetBit(&TlsBitMap, 0);

    //
    // Loop the Hash Table
    //
    for (i = 0; i < 32; i++)
    {
        //
        // Initialize each entry
        //
        InitializeListHead(&LdrpHashTable[i]);
    }

    //
    // Initialize the Loader Lock
    //
    InsertTailList(&RtlCriticalSectionList,
                   &LdrpLoaderLock.DebugInfo->ProcessLocksList);
    LdrpLoaderLock.DebugInfo->CriticalSection = &LdrpLoaderLock;
    LoaderLockInitialized = TRUE;

    //
    // Check if User Stack Trace Database support was requested
    //
    NtCheckPoint();
    if ((Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB) &&
        (LdrpShouldCreateStackTraceDb))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Setup Fast PEB Lock
    //
    RtlInitializeCriticalSection(&FastPebLock);

    //
    // Initialize Event Trace (Etw)
    //
    NtCheckPoint();
    //EtwpInitializeDll();
    Peb->FastPebLock = &FastPebLock;

    //
    // Setup VEH Callout Lock and Notification list
    //
    RtlInitializeCriticalSection(&RtlpCalloutEntryLock);
    InitializeListHead(&LdrpDllNotificationList);

    //
    // Initialize the Heap Manager
    //
    NtCheckPoint();
    RtlInitializeHeapManager();
    NtCheckPoint();

    //
    // Check for old executables (NT 3.51)
    //
    if ((NtHeader->OptionalHeader.MajorSubsystemVersion <= 3) &&
        (NtHeader->OptionalHeader.MinorSubsystemVersion < 51))
    {
        //
        // Use 16-byte aligned heap for compatibility
        //
        HeapFlags |= HEAP_CREATE_ALIGN_16;
    }

    //
    // Create the process heap
    //
    Peb->ProcessHeap = RtlCreateHeap(HeapFlags,
                                     NULL,
                                     NtHeader->OptionalHeader.SizeOfHeapReserve,
                                     NtHeader->OptionalHeader.SizeOfHeapCommit,
                                     NULL,
                                     &HeapParameters);
    if (!Peb->ProcessHeap)
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - unable to create process heap\n"
                   __FUNCTION__);
        return STATUS_NO_MEMORY;
    }

    //
    // Allocate an Activation Context Stack
    //
    Status = RtlAllocateActivationContextStack(&Teb->
                                               ActivationContextStackPointer);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Create the loader private heap
    //
    RtlZeroMemory(&PrivateHeapParameters, sizeof(RTL_HEAP_PARAMETERS));
    PrivateHeapParameters.Length = sizeof(RTL_HEAP_PARAMETERS);
    LdrpHeap = RtlCreateHeap(HEAP_GROWABLE | HEAP_CLASS_1,
                             NULL,
                             0x10000,
                             0x6000,
                             NULL,
                             &PrivateHeapParameters);
    if (!LdrpHeap)
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failing process initialization due to inability "
                   "to create loader private heap.\n"
                   __FUNCTION__);
        return STATUS_NO_MEMORY;
    }

    //
    // Create the Tag Heap
    //
    NtCheckPoint();
    NtdllBaseTag = RtlCreateTagHeap(Peb->ProcessHeap,
                                    0,
                                    L"NTDLL!",
                                    L"!Process\0"
                                    L"CSRSS Client\0"
                                    L"LDR Database\0"
                                    L"Current Directory\0"
                                    L"TLS Storage\0"
                                    L"DBGSS Client\0"
                                    L"SE Temporary\0"
                                    L"Temporary\0"
                                    L"LocalAtom\0");

    //
    // Initialize the atom package
    //
    RtlInitializeAtomPackage(HEAP_MAKE_TAG_FLAGS(NtdllBaseTag, 7));

    //
    // Check for Debug Heap
    //
    NtCheckPoint();
    if (OptionsKey)
    {
        //
        // Query the setting
        //
        Status = LdrQueryImageFileKeyOption(OptionsKey,
                                            L"DebugProcessHeapOnly",
                                            REG_DWORD,
                                            &DebugProcessHeapOnly,
                                            sizeof(ULONG),
                                            NULL);

        //
        // Check if we should enable the setting
        //
        if (NT_SUCCESS(Status) && RtlpDebugPageHeap && (DebugProcessHeapOnly))
        {
            //
            // Remove the global flag and remember the setting
            //
            RtlpDphGlobalFlags &= ~0x4000;
            RtlpDebugPageHeap = FALSE;
        }
    }

    //
    // Build the .NET Runtime Path
    //
    NtCheckPoint();
    FullPath.Buffer = FullMsCorePath;
    FullPath.Length = 0;
    FullPath.MaximumLength = sizeof(FullMsCorePath);
    RtlInitUnicodeString(&SystemRoot, SharedUserData->NtSystemRoot);
    RtlAppendUnicodeStringToString(&FullPath, &SystemRoot);
    RtlAppendUnicodeStringToString(&FullPath, &SlashSystem32SlashString);

    //
    // Open the Known DLLs directory
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &KnownDllString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&LdrpKnownDllObjectDirectory,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);

    //
    // Check if it exists
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // It doesn't, so assume System 32
        //
        LdrpKnownDllObjectDirectory = NULL;
        RtlInitUnicodeString(&LdrpKnownDllPath, FullPath.Buffer);
        LdrpKnownDllPath.Length -= sizeof(WCHAR);
    }
    else
    {
        //
        // Open the Known DLLs Path
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KnownDllPathString,
                                   OBJ_CASE_INSENSITIVE,
                                   LdrpKnownDllObjectDirectory,
                                   NULL);
        Status = NtOpenSymbolicLinkObject(&SymLinkHandle,
                                          SYMBOLIC_LINK_QUERY,
                                          &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            //
            // Query the path
            //
            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;
            Status = ZwQuerySymbolicLinkObject(SymLinkHandle,
                                               &LdrpKnownDllPath,
                                               NULL);
            NtClose(SymLinkHandle);
            if (!NT_SUCCESS(Status))
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - failed call to NtQuerySymbolicLinkObject with "
                           "status %x\n"
                           __FUNCTION__,
                           Status);
                return Status;
            }
        }
        else
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - failed call to NtOpenSymbolicLinkObject with "
                       "status %x\n"
                       __FUNCTION__,
                       Status);
            return Status;
        }
    }

    //
    // If we have process parameters, get the default path and current path
    //
    NtCheckPoint();
    if (ProcessParameters)
    {
        //
        // Check if we have a Dll Path
        //
        if (ProcessParameters->DllPath.Length)
        {
            //
            // Get the path
            //
            LdrpDefaultPath = *(PUNICODE_STRING)&ProcessParameters->DllPath;
        }
        else
        {
            //
            // We need a valid path
            //
            LdrpInitializationFailure(STATUS_INVALID_PARAMETER);
        }

        //
        // Set the current directory
        //
        CurrentDirectory = ProcessParameters->CurrentDirectory.DosPath;

        //
        // Check if it's empty or invalid
        //
        if ((!CurrentDirectory.Buffer) ||
            (CurrentDirectory.Buffer[0] == UNICODE_NULL) ||
            (!CurrentDirectory.Length))
        {
            //
            // Allocate space for the buffer
            //
            CurrentDirectory.Buffer = RtlAllocateHeap(Peb->ProcessHeap,
                                                      0,
                                                      3 * sizeof(WCHAR) +
                                                      sizeof(UNICODE_NULL));
            if (!CurrentDirectory.Buffer)
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - unable to allocate current working "
                           "directory buffer\n"
                           __FUNCTION__);
                return STATUS_NO_MEMORY;
            }

            //
            // Copy the drive of the system root
            //
            RtlMoveMemory(CurrentDirectory.Buffer,
                          SharedUserData->NtSystemRoot,
                          3 * sizeof(WCHAR));
            CurrentDirectory.Buffer[3] = UNICODE_NULL;
            CurrentDirectory.Length = 3 * sizeof(WCHAR);
            CurrentDirectory.MaximumLength = CurrentDirectory.Length +
                                             sizeof(WCHAR);

            //
            // Remember we were allocated
            //
            CurDirWasAllocated = TRUE;
        }
        else
        {
            //
            // Use the local buffer
            //
            CurrentDirectory.Length = SystemRoot.Length;
            CurrentDirectory.Buffer = SystemRoot.Buffer;
        }
    }

    //
    // Setup Loader Data
    //
    NtCheckPoint();
    Peb->Ldr = &PebLdr;
    PebLdr.Length = sizeof(PEB_LDR_DATA);
    PebLdr.Initialized = TRUE;

    //
    // Sanity checks
    //
    ASSERT(PebLdr.SsHandle == NULL);
    ASSERT(PebLdr.EntryInProgress == NULL);
    ASSERT(PebLdr.InLoadOrderModuleList.Flink == NULL);
    ASSERT(PebLdr.InLoadOrderModuleList.Blink == NULL);
    ASSERT(PebLdr.InMemoryOrderModuleList.Flink == NULL);
    ASSERT(PebLdr.InMemoryOrderModuleList.Blink == NULL);
    ASSERT(PebLdr.InInitializationOrderModuleList.Flink == NULL);
    ASSERT(PebLdr.InInitializationOrderModuleList.Blink == NULL);

    //
    // Initialize the list heads
    //
    InitializeListHead(&PebLdr.InLoadOrderModuleList);
    InitializeListHead(&PebLdr.InMemoryOrderModuleList);
    InitializeListHead(&PebLdr.InInitializationOrderModuleList);

    //
    // Allocate a data entry for the Image
    //
    LdrpImageEntry = LdrpAllocateDataTableEntry(Peb->ImageBaseAddress);
    NtLdrEntry = LdrpImageEntry;
    if (!NtLdrEntry)
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failing process initialization due to inability "
                   "to allocate \"%wZ\"'s LDR_DATA_TABLE_ENTRY\n"
                   __FUNCTION__,
                   &ImageFileName);

        //
        // Check if the current directory string was allocated and fail
        //
        if (CurDirWasAllocated) RtlFreeUnicodeString(&CurrentDirectory);
        return STATUS_NO_MEMORY;
    }

    //
    // Set it up
    //
    NtLdrEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(NtLdrEntry->DllBase);
    NtLdrEntry->LoadCount = -1;
    NtLdrEntry->EntryPointActivationContext = 0;
    NtLdrEntry->FullDllName.Length = ImageFileName.Length;
    NtLdrEntry->FullDllName.Buffer = ImageFileName.Buffer;
    if (IsDotNetImage) NtLdrEntry->Flags = LDRP_COR_IMAGE;

    //
    // Check if the name is empty
    //
    if (!ImageFileName.Buffer[0])
    {
        //
        // Use the same base name
        //
        NtLdrEntry->BaseDllName = NtLdrEntry->FullDllName;
    }
    else
    {
        //
        // Find a slash and parse to get the name
        //
        BaseName = UNICODE_NULL;
        Current = ImageFileName.Buffer;
        while (*Current) if (*Current++ == '\\') BaseName = Current;

        //
        // Did we find anything this time?
        //
        if (!BaseName)
        {
            //
            // No, use the same base name
            //
            NtLdrEntry->BaseDllName = NtLdrEntry->FullDllName;
        }
        else
        {
            //
            // Setup the name
            //
            NtLdrEntry->BaseDllName.Length = (USHORT)((ULONG_PTR)Current -
                                                      (ULONG_PTR)BaseName);
            NtLdrEntry->BaseDllName.MaximumLength = NtLdrEntry->BaseDllName.
                                                    Length + sizeof(WCHAR);
            NtLdrEntry->BaseDllName.Buffer = NtLdrEntry->FullDllName.Buffer -
                                             NtLdrEntry->BaseDllName.Length +
                                             NtLdrEntry->FullDllName.Length;
        }
    }

    //
    // Processing done, insert it
    //
    NtLdrEntry->Flags |= LDRP_ENTRY_PROCESSED;
    LdrpInsertMemoryTableEntry(NtLdrEntry);

    //
    // Now add an entry for NTDLL
    //
    NtLdrEntry = LdrpAllocateDataTableEntry(SystemArgument1);
    if (!NtLdrEntry)
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failing process initialization due to inability "
                   "to allocate NTDLL's LDR_DATA_TABLE_ENTRY\n"
                   __FUNCTION__);

        //
        // Check if the current directory string was allocated and fail
        //
        if (CurDirWasAllocated) RtlFreeUnicodeString(&CurrentDirectory);
        return STATUS_NO_MEMORY;
    }

    //
    // Now set it up
    //
    NtLdrEntry->Flags = LDRP_IMAGE_DLL;
    NtLdrEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(NtLdrEntry->DllBase);
    NtLdrEntry->LoadCount = -1;
    NtLdrEntry->EntryPointActivationContext = 0;
    NtLdrEntry->BaseDllName.Length = SystemRoot.Length;
    RtlAppendUnicodeStringToString(&SystemRoot, &NtDllName);
    NtLdrEntry->BaseDllName.Length = NtDllName.Length;
    NtLdrEntry->BaseDllName.MaximumLength = NtDllName.MaximumLength;
    NtLdrEntry->BaseDllName.Buffer = NtDllName.Buffer;

    //
    // Processing done, insert it
    //
    LdrpNtDllDataTableEntry = NtLdrEntry;
    LdrpInsertMemoryTableEntry(NtLdrEntry);

    //
    // Let the world know
    //
    if (ShowSnaps)
    {
        DbgPrint("LDR: NEW PROCESS\n");
        DbgPrint("     Image Path: %wZ (%wZ)\n",
                 &LdrpImageEntry->FullDllName,
                 &LdrpImageEntry->BaseDllName);
        DbgPrint("     Current Directory: %wZ\n", &CurrentDirectory);
        DbgPrint("     Search Path: %wZ\n", &LdrpDefaultPath);
    }

    //
    // Link the Load Order List
    //
    InsertHeadList(&Peb->Ldr->InLoadOrderModuleList,
                   &LdrpNtDllDataTableEntry->InLoadOrderLinks);

    //
    // Set the current directory
    //
    NtCheckPoint();
    Status = RtlSetCurrentDirectory_U(&CurrentDirectory);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - unable to set current directory to \"%wZ\"; "
                   "status = %x\n"
                   __FUNCTION__,
                   &CurrentDirectory,
                   Status);

        //
        // We failed, check if we should free it
        //
        if (CurDirWasAllocated) RtlFreeUnicodeString(&CurrentDirectory);

        //
        // Set it to the NT Root
        //
        CurrentDirectory = SystemRoot;
        Status = RtlSetCurrentDirectory_U(&CurrentDirectory);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - unable to set current directory to "
                       "NtSystemRoot; status = %x\n"
                       __FUNCTION__,
                       &CurrentDirectory,
                       Status);
        }
    }
    else
    {
        //
        // We're done with it, free it
        //
        if (CurDirWasAllocated) RtlFreeUnicodeString(&CurrentDirectory);
    }

    //
    // Check if we should look for a .local file
    //
    if (ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_LOCAL_DLL_PATH)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if the Application Verifier was enabled
    //
    if (Peb->NtGlobalFlag & FLG_POOL_ENABLE_TAIL_CHECK)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if this is a .NET image
    //
    NtCheckPoint();
    if (IsDotNetImage)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if this is a Win32 Subsystem Image
    //
    NtCheckPoint();
    if ((NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
        (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Walk the IAT and load all the DLLs
    //
    NtCheckPoint();
    Status = LdrpWalkImportDescriptor(LdrpDefaultPath.Buffer, LdrpImageEntry);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - call to LdrpWalkImportDescriptor failed with "
                   "status %x\n"
                   __FUNCTION__,
                   Status);
        return Status;
    }

    //
    // Check if relocation is needed
    //
    NtCheckPoint();
    if ((ULONG_PTR)Peb->ImageBaseAddress != NtHeader->OptionalHeader.ImageBase)
    {
        //
        // Set protection so that we can relocate
        //
        Status = LdrpSetProtection(Peb->ImageBaseAddress, FALSE);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - call to LdrpSetProtection(%p, FALSE) failed "
                       "with status %x\n"
                       __FUNCTION__,
                       Peb->ImageBaseAddress,
                       Status);
            return Status;
        }

        //
        // Now relocate the image
        //
        Status = LdrRelocateImage(Peb->ImageBaseAddress,
                                  "LDR",
                                  STATUS_SUCCESS,
                                  STATUS_CONFLICTING_ADDRESSES,
                                  STATUS_INVALID_IMAGE_FORMAT);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - call to LdrRelocateImage failed with status "
                       "%x\n"
                       __FUNCTION__,
                       Status);
            return Status;
        }

        //
        // Check if we have a context, and we're not a .NET image
        //
        if ((Context) && !(IsDotNetImage))
        {
            //
            // Also relocate our start context
            //
            LdrpRelocateStartContext(Context,
                                     (ULONG_PTR)Peb->ImageBaseAddress -
                                     NtHeader->OptionalHeader.ImageBase);
        }

        //
        // Restore protection
        //
        Status = LdrpSetProtection(Peb->ImageBaseAddress, TRUE);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - call to LdrpSetProtection(%p, TRUE) failed "
                       "with status %x\n"
                       __FUNCTION__,
                       Peb->ImageBaseAddress,
                       Status);
            return Status;
        }
    }

    //
    // Loop the DLLs
    //
    NtCheckPoint();
    ListHead = &Peb->Ldr->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get this entry
        //
        NtLdrEntry = CONTAINING_RECORD(NextEntry,
                                       LDR_DATA_TABLE_ENTRY,
                                       InLoadOrderLinks);

        //
        // Lock it
        //
        NtLdrEntry->LoadCount = -1;

        //
        // Move to the next one
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Phase 0 is done
    //
    LdrpLdrDatabaseIsSetup = TRUE;

    //
    // Initialize TLS
    //
    NtCheckPoint();
    Status = LdrpInitializeTls();
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failed to initialize TLS slots; status %x\n"
                   __FUNCTION__,
                   Status);
        return Status;
    }

    //
    // Loop the DLLs
    //
    NtCheckPoint();
    ListHead = &PebLdr.InMemoryOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get this entry
        //
        NtLdrEntry = CONTAINING_RECORD(NextEntry,
                                       LDR_DATA_TABLE_ENTRY,
                                       InMemoryOrderModuleList);

        //
        // Mark its range
        //
        RtlpStkMarkDllRange(NtLdrEntry);

        //
        // Move to the next one
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we are being debugged
    //
    if (Peb->BeingDebugged)
    {
        //
        // Break into the debugger
        //
        DbgBreakPoint();

        //
        // Check if we should show snaps now
        //
        ShowSnaps = (BOOLEAN)(Peb->NtGlobalFlag & FLG_SHOW_LDR_SNAPS);
    }

    //
    // Validate the Image for MP Usage
    //
    if (LdrpNumberOfProcessors > 1) LdrpValidateImageForMp(LdrpImageEntry);

    //
    // Check if we are profiling
    //
    if (LdrpDisplayLoadTime) NtQueryPerformanceCounter(&InitbTime, NULL);

    //
    // Check NX Options
    //
    if (SharedUserData->NXSupportPolicy == 1)
    {
        //
        // Enable NX
        //
        ExecuteOptions = 0xD;
    }
    else if (!SharedUserData->NXSupportPolicy)
    {
        //
        // Disable NX
        //
        ExecuteOptions = 0xA;
    }

    //
    // Let the Memory Manager know
    //
    ZwSetInformationProcess(NtCurrentProcess(),
                            ProcessExecuteFlags,
                            &ExecuteOptions,
                            sizeof(ExecuteOptions));

    //
    // Check if we had Shim Data
    //
    if (OldShimData)
    {
        //
        // Load the Shim Engine
        //
        Peb->AppCompatInfo = NULL;
        LdrpLoadShimEngine(OldShimData, &ImagePathName, OldShimData);
    }
    else
    {
        //
        // Check for Application Compatibility Goo
        //
        LdrQueryApplicationCompatibilityGoo(OptionsKey);
    }

    //
    // Check more NX Options
    //
    ExecuteOptions = 0;
    if ((SharedUserData->NXSupportPolicy == 2) &&
        (NtHeader->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE) &&
        (ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_NX))
    {
        //
        // Set execute options
        //
        ExecuteOptions = 2;
    }

    //
    // Check for Safedisk, SecuROM or NXEntry
    //
    if ((LdrpCheckForSecuROMImage(NtHeader)) ||
        (LdrpCheckForNXEntryAddress(NtHeader)) ||
        (LdrpCheckForSafeDiscImage(NtHeader)))
    {
        //
        // Check if we haven't already set them
        //
        if (!ExecuteOptions)
        {
            //
            // Set the options
            //
            ExecuteOptions = 2;
        }
    }

    //
    // Check if we have an options to set
    //
    if (ExecuteOptions)
    {
        //
        // Set them
        //
        ZwSetInformationProcess(NtCurrentProcess(),
                                ProcessExecuteFlags,
                                &ExecuteOptions,
                                sizeof(ExecuteOptions));
    }

    //
    // Now call the Init Routines
    //
    NtCheckPoint()
    Status = LdrpRunInitializeRoutines(Context);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - Failed running initialization routines; status "
                   "%x\n",
                   __FUNCTION__,
                   Status);
        return Status;
    }

    //
    // Check if shims are enabled
    //
    if (g_ShimsEnabled)
    {
        //
        // Decode the shim installation function and call it
        //
        ShimInstall = RtlDecodeSystemPointer(g_pfnSE_InstallAfterInit);
        if (!ShimInstall(&ImagePathName, OldShimData))
        {
            //
            // We failed; unload the shim engine
            //
            LdrpUnloadShimEngine();
        }
    }

    //
    // Check if we have a user-defined Post Process Routine and call it
    //
    if (Peb->PostProcessInitRoutine) Peb->PostProcessInitRoutine();

    //
    // Close the options key if we have one opened
    //
    if (OptionsKey) NtClose(OptionsKey);

    //
    // Return status
    //
    NtCheckPoint();
    return Status;
}

/*++
* @name LdrpInitializeThread
*
* The LdrpInitializeThread routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpInitializeThread(IN PCONTEXT Context)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name _LdrpInitialize
*
* The _LdrpInitialize routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @param SystemArgument1
*        FILLMEIN
*
* @param SystemArgument2
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
_LdrpInitialize(PCONTEXT Context,
                PVOID SystemArgument1,
                PVOID SystemArgument2)
{
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status, LoaderStatus;
    MEMORY_BASIC_INFORMATION MemoryBasicInfo;
    LARGE_INTEGER Timeout;

    //
    // Check if we have a deallocation stack
    //
    __debugbreak();
    NtCheckPoint();
    if (!Teb->DeallocationStack)
    {
        //
        // We don't, set one
        //
        NtCheckPoint();
        Status = ZwQueryVirtualMemory(NtCurrentProcess(),
                                      Teb->Tib.StackLimit,
                                      MemoryBasicInformation,
                                      &MemoryBasicInfo,
                                      sizeof(MEMORY_BASIC_INFORMATION),
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - Call to NtQueryVirtualMemory failed with "
                       "ntstatus %lx\n",
                       __FUNCTION__,
                       Status);
            LdrpInitializationFailure(Status);
            RtlRaiseStatus(Status);
        }

        //
        // Set the stack
        //
        Teb->DeallocationStack = MemoryBasicInfo.AllocationBase;
    }

    //
    // Loop for init lock
    //
    while (TRUE)
    {
        //
        // Try to change the status and get the previous status value
        //
        ProcessInitialized = _InterlockedCompareExchange(&LdrpProcessInitialized,
                                                         1,
                                                         0);

        //
        // If it was already initialized, then try to acquire the lock
        //
        if (ProcessInitialized == 1)
        {
            //
            //
            // Set the timeout to 30 seconds
            //
            Timeout.QuadPart = Int32x32To64(30, -10000);

            //
            // Make sure the status hasn't changed
            //
            while (!LdrpProcessInitialized)
            {
                //
                // Do the wait
                //
                Status = ZwDelayExecution(FALSE, &Timeout);
                if (!NT_SUCCESS(Status))
                {
                    DbgPrintEx(DPFLTR_LDR_ID,
                               0,
                               "LDR: ***NONFATAL*** %s - call to NtDelayExecution "
                               "waiting on loader lock failed; ntstatus = %x\n",
                               __FUNCTION__,
                               Status);
                }
            }
        }
        else
        {
            //
            // Sanity check
            //
            ASSERT((ProcessInitialized == 0) || (ProcessInitialized == 2));
            break;
        }
    }

    //
    // Check if we have already setup LDR data
    //
    if (!Peb->Ldr)
    {
        //
        // Setup the Loader Lock
        //
        Peb->LoaderLock = &LdrpLoaderLock;

        //
        // Let other code know we're initializing
        //
        LdrpInLdrInit = TRUE;

        //
        // Check if profiling was enabled
        //
        if (LdrpDisplayLoadTime) ZwQueryPerformanceCounter(&BeginTime, NULL);

        //
        // Initialize VEH Call lists
        //
        NtCheckPoint();
        InitializeListHead(&RtlpCalloutEntryList);
        InitializeListHead(&RtlpCallbackEntryList);

        //
        // Protect with SEH
        //
        __try
        {
            //
            // Initialize the Process
            //
            LoaderStatus = LdrpInitializeProcess(Context,
                                                 SystemArgument1);
            if (!NT_SUCCESS(LoaderStatus))
            {
                //
                // We've failed
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - call to LdrpInitializeProcess() failed "
                           "with ntstatus = %x\n",
                           __FUNCTION__,
                           Status);
            }
            else if (Peb->MinimumStackCommit)
            {
                //
                // We need to enforce the stack limit
                //
                LoaderStatus = LdrpTouchThreadStack(Peb->MinimumStackCommit);
            }
            NtCheckPoint();
        }
        __except (LdrpInitializationException(GetExceptionInformation()))
        {
            //
            // Get the exception code
            //
            LoaderStatus = GetExceptionCode();
        }

        //
        // We're not initializing anymore
        //
        LdrpInLdrInit = FALSE;

        //
        // Check if profiling was enabled
        //
        if (LdrpDisplayLoadTime)
        {
            //
            // Calculate the end and elapsed times
            //
            ZwQueryPerformanceCounter(&EndTime, NULL);
            ZwQueryPerformanceCounter(&ElapsedTime, &Interval);
            ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;

            //
            // Display the load time
            //
            DbgPrint("\nLoadTime %ld In units of %ld cycles/second \n",
                     ElapsedTime.LowPart,
                     Interval.LowPart);

            //
            // Display the init time
            //
            ElapsedTime.QuadPart = EndTime.QuadPart - InitbTime.QuadPart;
            DbgPrint("InitTime %ld\n", ElapsedTime.LowPart);

            //
            // Display all other statistics
            //
            DbgPrint("Compares %d Bypasses %d Normal Snaps %d\n"
                     "SecOpens %d SecCreates %d Maps %d Relocates %d\n",
                     LdrpCompareCount,
                     LdrpSnapBypass,
                     LdrpNormalSnap,
                     LdrpSectionOpens,
                     LdrpSectionCreates,
                     LdrpSectionMaps,
                     LdrpSectionRelocates);
        }

        //
        // Check if init worked
        //
        if (NT_SUCCESS(LoaderStatus)) LdrpProcessInitializationComplete();
    }
    else
    {
        //
        // Loader data is there... is this a fork()?
        //
        if (Peb->InheritedAddressSpace)
        {
            //
            // Handle the fork()
            //
            LoaderStatus = LdrpForkProcess();
        }
        else
        {
            //
            // This is a new thread initializing
            //
            LdrpInitializeThread(Context);
        }
    }

    //
    // All done, test alert the thread
    //
    ZwTestAlert();

    //
    // Check success status
    //
    if (!NT_SUCCESS(LoaderStatus))
    {
        //
        // Fail
        //
        LdrpInitializationFailure(LoaderStatus);
        RtlRaiseStatus(LoaderStatus);
    }
}

/*++
* @name LdrpInitialize
*
* The LdrpInitialize routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @param SystemArgument1
*        FILLMEIN
*
* @param SystemArgument2
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpInitialize(PCONTEXT Context,
               PVOID SystemArgument1,
               PVOID SystemArgument2)
{
    //
    // Initialize the security cookie
    //
    if (!SecurityCookieInitialized) InitSecurityCookie();

    //
    // Call the main initializer
    //
    _LdrpInitialize(Context, SystemArgument1, SystemArgument2);
}

