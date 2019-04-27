/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ldrsnap.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 15-Apr-06

--*/
#include "precomp.h"

//
// Process state
//
PTEB LdrpTopLevelDllBeingLoadedTeb;
PLDR_DATA_TABLE_ENTRY LdrpCurrentDllInitializer, LdrpLoadedDllHandleCache;

//
// Callbacks
//
PKERNEL32_POST_IMPORT_FUNCTION Kernel32ProcessInitPostImportFunction;
PVOID LdrpManifestProberRoutine, LdrpAppCompatDllRedirectionCallbackFunction;

//
// Static strings
//
UNICODE_STRING DefaultExtension = RTL_CONSTANT_STRING(L".DLL");
UNICODE_STRING User32String = RTL_CONSTANT_STRING(L"user32.dll");
UNICODE_STRING Kernel32String = RTL_CONSTANT_STRING(L"kernel32.dll");
UNICODE_STRING DynamicString = RTL_CONSTANT_STRING(L"Dymically Allocated Memory");
UNICODE_STRING MsCoreeDllString = RTL_CONSTANT_STRING(L"\\system32\\mscoree.dll");
UNICODE_STRING RtlNtPathSeparatorString = RTL_CONSTANT_STRING(L"\\");
UNICODE_STRING RtlDosPathSeparatorsString = RTL_CONSTANT_STRING(L"\\/");

//
// Buffers
//
WCHAR LocalBuffer[MAX_PATH], RealBuffer[MAX_PATH];
WCHAR LocalBuffer2[MAX_PATH], RealBuffer2[MAX_PATH];

//
// Profiling data
//
LARGE_INTEGER MapBeginTime, MapEndTime, MapElapsedTime;

/*++
* @name LdrpAllocateDataTableEntry
*
* The LdrpAllocateDataTableEntry routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @return PLDR_DATA_TABLE_ENTRY
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry(IN PVOID BaseAddress)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(BaseAddress);

    //
    // Make sure the header is valid
    //
    if (NtHeader)
    {
        //
        // Allocate an entry
        //
        LdrEntry = RtlAllocateHeap(LdrpHeap,
                                   HEAP_ZERO_MEMORY,
                                   sizeof(LDR_DATA_TABLE_ENTRY));
        if (LdrEntry)
        {
            //
            // Set it up
            //
            LdrEntry->DllBase = BaseAddress;
            LdrEntry->SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
            LdrEntry->PatchInformation = NULL;
            LdrEntry->TimeDateStamp = NtHeader->FileHeader.TimeDateStamp;
        }
    }

    //
    // Return the entry
    //
    return LdrEntry;
}

/*++
* @name LdrpInsertMemoryTableEntry
*
* The LdrpInsertMemoryTableEntry routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpInsertMemoryTableEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG i;

    //
    // Get the Hash entry
    //
    i = LDR_GET_HASH_ENTRY(LdrEntry->BaseDllName.Buffer[0]);
    InsertTailList(&LdrpHashTable[i], &LdrEntry->HashLinks);

    //
    // Insert it into the other lists
    //
    InsertTailList(&PebLdr.InLoadOrderModuleList, &LdrEntry->InLoadOrderLinks);
    InsertTailList(&PebLdr.InMemoryOrderModuleList,
                   &LdrEntry->InMemoryOrderModuleList);
}

/*++
* @name LdrpFetchAddressOfSecurityCookie
*
* The LdrpFetchAddressOfSecurityCookie routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @return PULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PULONG
LdrpFetchAddressOfSecurityCookie(IN PVOID BaseAddress,
                                 IN ULONG Size)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig;
    ULONG ConfigSize;

    //
    // Get the header
    //
    NtHeader = RtlImageNtHeader(BaseAddress);
    if (!NtHeader) return NULL;

    //
    // Get the Load Config directory
    //
    LoadConfig = RtlImageDirectoryEntryToData(BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                              &ConfigSize);

    //
    // Make sure we got one
    //
    if (!(LoadConfig) || !(ConfigSize)) return NULL;

    //
    // Check if it's an old-style (non SAFESEH) one
    //
    if (ConfigSize != FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY,
                                   SEHandlerTable))
    {
        //
        // It's not. Make sure it corresponds to what it says it is
        //
        if (ConfigSize != LoadConfig->Size) return NULL;
    }

    //
    // Make sure it's a SAFESEH compatible one
    //
    if (LoadConfig->Size >= FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY,
                                         SEHandlerCount) + sizeof(ULONG))
    {
        //
        // Make sure the cookie is in the image
        //
        if ((LoadConfig->SecurityCookie > PtrToUlong(BaseAddress)) &&
            (LoadConfig->SecurityCookie < ((ULONG_PTR)BaseAddress + Size)))
        {
            //
            // It is, return it
            //
            return UlongToPtr(LoadConfig->SecurityCookie);
        }
    }

    //
    // Fail
    //
    return NULL;
}

/*++
* @name LdrpInitSecurityCookie
*
* The LdrpInitSecurityCookie routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpInitSecurityCookie(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PULONG Cookie;
    ULONG Key;
    ULONG Key2 = 0x1234; // fixme
    LARGE_INTEGER Counter;

    //
    // Get the address of the cookie
    //
    Cookie = LdrpFetchAddressOfSecurityCookie(LdrEntry->DllBase,
                                              LdrEntry->SizeOfImage);
    if (!Cookie) return;

    //
    // Check if we have a non-initialized cookie
    //
    if ((*Cookie != LdrpDefaultCookie) &&
        (*Cookie != LdrpDefaultCookie16))
    {
        //
        // Invalid cookie, quit
        //
        return;
    }

    //
    // Start by XORing the system time and Client ID
    //
    Key = SharedUserData->SystemTime.LowPart ^
          SharedUserData->SystemTime.High1Time;
    Key ^= ((ULONG_PTR)NtCurrentTeb()->Cid.UniqueThread ^
            (ULONG_PTR)NtCurrentTeb()->Cid.UniqueProcess);

    //
    // Query the time
    //
    NtQueryPerformanceCounter(&Counter, NULL);

    //
    // XOR it into the key
    //
    Key2 ^= Counter.HighPart;
    Key2 ^= Counter.LowPart;
    Key ^= Key2;

    //
    // Check if we had the 16-bit cookie and mask our key if so
    //
    if (*Cookie == LdrpDefaultCookie16) Key &= 0xFFFF;

    //
    // Make sure we have a key generated
    //
    if (!(Key) || (Key == *Cookie))
    {
        //
        // We don't have a key, or we generated the same one
        //
        Key = (*Cookie == LdrpDefaultCookie16) ?
               LdrpDefaultCookie16 + 1 : LdrpDefaultCookie + 1;
    }

    //
    // Save the cookie
    //
    *Cookie = Key;
}

/*++
* @name LdrpFetchAddressOfEntryPoint
*
* The LdrpFetchAddressOfEntryPoint routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
LdrpFetchAddressOfEntryPoint(IN PVOID BaseAddress)
{
    PIMAGE_NT_HEADERS NtHeader;
    ULONG_PTR EntryPoint = 0;

    //
    // Get the header and entrypoint
    //
    NtHeader = RtlImageNtHeader(BaseAddress);
    EntryPoint = NtHeader->OptionalHeader.AddressOfEntryPoint;
    if (EntryPoint) EntryPoint += (ULONG_PTR)BaseAddress;

    //
    // Return it
    //
    return (PVOID)EntryPoint;
}

/*++
* @name LdrpSetProtection
*
* The LdrpSetProtection routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param ReProtect
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpSetProtection(IN PVOID BaseAddress,
                  IN BOOLEAN ReProtect)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG i;
    PVOID Address;
    SIZE_T Size;
    NTSTATUS Status;
    ULONG NewAccess, OldAccess;

    //
    // Get the NT Headers
    //
    NtHeader = RtlImageNtHeader(BaseAddress);
    if (!NtHeader) return STATUS_INVALID_IMAGE_FORMAT;

    //
    // Get the section header
    //
    SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

    //
    // Loop every section
    //
    for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
    {
        //
        // Check if this section isn't RW and has data
        //
        if (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) &&
             (SectionHeader->SizeOfRawData))
        {
            //
            // Check if we are re-protecting
            //
            if (ReProtect)
            {
                //
                // Check if the section was also executable
                //
                if (SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                {
                    //
                    // Then we'll need E access
                    //
                    NewAccess = PAGE_EXECUTE;
                }
                else
                {
                    //
                    // Otherwise, only RO is needed
                    //
                    NewAccess = PAGE_READONLY;
                }

                //
                // Take into account caching as well
                //
                NewAccess |= (SectionHeader->Characteristics &
                              IMAGE_SCN_MEM_NOT_CACHED) ? PAGE_NOCACHE : 0;
            }
            else
            {
                //
                // Make the section RW
                //
                NewAccess = PAGE_READWRITE;
            }

            //
            // Get the address and size of the section
            //
            Address = (PVOID)((ULONG_PTR)BaseAddress +
                              SectionHeader->VirtualAddress);
            Size = SectionHeader->SizeOfRawData;
            if (Size)
            {
                //
                // Set the protection value
                //
                Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                &Address,
                                                &Size,
                                                NewAccess,
                                                &OldAccess);
                if (!NT_SUCCESS(Status)) return Status;
            }
        }

        //
        // Move to the next section
        //
        SectionHeader++;
    }

    //
    // Flush the ICACHE if needed, and return success
    //
    if (ReProtect) NtFlushInstructionCache(NtCurrentProcess(), NULL, 0);
    return STATUS_SUCCESS;
}

/*++
* @name LdrpValidateImageForMp
*
* The LdrpValidateImageForMp routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpValidateImageForMp(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PIMAGE_LOAD_CONFIG_DIRECTORY ConfigDirectory;
    ULONG ConfigSize;
    PUCHAR *Prefix;
    ULONG_PTR Parameters;
    ULONG Response;

    //
    // Get the configuration directory, check if it's valid and if it has a
    // lock prefix table
    //
    ConfigDirectory = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                   &ConfigSize);
    if ((ConfigDirectory) &&
        (ConfigSize == sizeof(IMAGE_LOAD_CONFIG_DIRECTORY)) &&
        (ConfigDirectory->LockPrefixTable))
    {
        //
        // Get the first prefix
        //
        Prefix = (PUCHAR*)UlongToPtr(ConfigDirectory->LockPrefixTable);
        while (*Prefix)
        {
            //
            // Check if this prefix is a NOP (0x90)
            //
            if (**Prefix == 0x90)
            {
                //
                // Check if we have more then one CPU
                //
                if (LdrpNumberOfProcessors > 1)
                {
                    //
                    // Set the DLL Name as the parameter and taise the error
                    //
                    Parameters = (ULONG_PTR)&LdrEntry->BaseDllName;
                    ZwRaiseHardError(STATUS_IMAGE_MP_UP_MISMATCH,
                                     1,
                                     1,
                                     &Parameters,
                                     OptionOk,
                                     &Response);

                    //
                    // If we're initializing, increase the count of errors
                    //
                    if (LdrpInLdrInit) LdrpFatalHardErrorCount++;
                }
            }

            //
            // Move to the next prefix
            //
            Prefix++;
        }
    }
}

/*++
* @name LdrpClearLoadInProgress
*
* The LdrpClearLoadInProgress routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
LdrpClearLoadInProgress(VOID)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG i = 0;

    //
    // Start looping
    //
    ListHead = &PebLdr.InInitializationOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the entry and remove the load flag
        //
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderModuleList);
        LdrEntry->Flags &= ~LDRP_LOAD_IN_PROGRESS;

        //
        // Check if it hasn't been processed and has an entrypoint
        if (!(LdrEntry->Flags & LDRP_ENTRY_PROCESSED) &&
            (LdrEntry->EntryPoint))
        {
            //
            // Increase count
            //
            i++;
        }

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Return count
    //
    return i;
}

/*++
* @name LdrpResolveDllNameForAppPrivateRedirection
*
* The LdrpResolveDllNameForAppPrivateRedirection routine FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param PrivateName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpResolveDllNameForAppPrivateRedirection(IN PUNICODE_STRING DllName,
                                           OUT PUNICODE_STRING PrivateName)
{
    UNICODE_STRING RealName, LocalName;
    NTSTATUS Status;

    //
    // Initialize the strings
    //
    RtlInitEmptyUnicodeString(&RealName, RealBuffer2, sizeof(RealBuffer2));
    RtlInitEmptyUnicodeString(&LocalName, LocalBuffer2, sizeof(LocalBuffer2));

    //
    // Make sure we have a private name
    //
    if (!PrivateName)
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    //
    // Initialize it
    //
    RtlInitEmptyUnicodeString(PrivateName, NULL, 0);

    //
    // Make sure we have a DLL name
    //
    if (!DllName)
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    //
    // Validate it
    //
    Status = RtlValidateUnicodeString(0, DllName);
    if (!NT_SUCCESS(Status)) goto Quickie;

    //
    // Check if we should compute the privatized name
    //
    if (((NtCurrentPeb()->ProcessParameters) &&
         (NtCurrentPeb()->ProcessParameters->Flags &
          RTL_USER_PROCESS_PARAMETERS_PRIVATE_DLL_PATH)) &&
          (DllName->Length))
    {
        //
        // Query it
        //
        Status = RtlComputePrivatizedDllName_U(DllName,
                                               &LocalName,
                                               &RealName);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s call to RtlComputePrivatizedDllName_U()"
                       "failed with status %lx\n",
                       __FUNCTION__,
                       Status);
            goto Quickie;
        }

        //
        // Check if the local name exists
        //
        if (RtlDoesFileExists_U(LocalName.Buffer))
        {
            //
            // Copy it
            //
            Status = LdrpCopyUnicodeString(PrivateName, &LocalName);
            if (!NT_SUCCESS(Status))
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s calling LdrpCopyUnicodeString() failed; "
                           "exiting with status %lx\n",
                           __FUNCTION__,
                           Status);
                goto Quickie;
            }
        }
        else if (RtlDoesFileExists_U(RealName.Buffer))
        {
            //
            // Copy it
            //
            Status = LdrpCopyUnicodeString(PrivateName, &RealName);
            if (!NT_SUCCESS(Status))
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s calling LdrpCopyUnicodeString() failed; "
                           "exiting with status %lx\n",
                           __FUNCTION__,
                           Status);
                goto Quickie;
            }
        }

        //
        // If we got here, set success
        //
        Status = STATUS_SUCCESS;
    }

Quickie:
    //
    // Check if the real name wasn't buffer-allocated
    //
    if (RealName.Buffer != RealBuffer2)
    {
        //
        // Free it
        //
        RtlFreeStringRoutine(RealName.Buffer);
    }

    //
    // Check if the local name wasn't buffer-allocated
    //
    if (LocalName.Buffer != LocalBuffer2)
    {
        //
        // Free it
        //
        RtlFreeStringRoutine(LocalName.Buffer);
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpResolveFullName
*
* The LdrpResolveFullName routine FILLMEIN
*
* @param OriginalName
*        FILLMEIN
*
* @param PathName
*        FILLMEIN
*
* @param FullPathName
*        FILLMEIN
*
* @param ExpandedName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpResolveFullName(IN PUNICODE_STRING OriginalName,
                    IN PUNICODE_STRING PathName,
                    IN PUNICODE_STRING FullPathName,
                    IN PUNICODE_STRING *ExpandedName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_PATH_TYPE PathType;
    BOOLEAN InvalidName;
    ULONG Length;

    //
    // Check if we should display
    //
    if (ShowSnaps)
    {
        //
        // Print a message
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - Expanding full name of %wZ\n",
                   __FUNCTION__,
                   OriginalName);
    }

    //
    // Lock the PEB
    //
    RtlEnterCriticalSection(&FastPebLock);

    //
    // Get the path name
    //
    Length = RtlGetFullPathName_Ustr(OriginalName,
                                     PathName->Length,
                                     PathName->Buffer,
                                     NULL,
                                     &InvalidName,
                                     &PathType);
    if (!(Length) || (Length > UNICODE_STRING_MAX_BYTES))
    {
        //
        // Fail
        //
        Status = STATUS_NAME_TOO_LONG;
        goto Quickie;
    }

    //
    // Check if the length hasn't changed
    //
    if (Length <= PathName->Length)
    {
        //
        // Return the same thing
        //
        *ExpandedName = PathName;
        PathName->Length = (USHORT)Length;
        goto Quickie;
    }

    //
    // Sanity check
    //
    ASSERT(Length >= sizeof(WCHAR));

    //
    // Allocate a string
    //
    Status = LdrpAllocateUnicodeString(FullPathName, Length - sizeof(WCHAR));
    if (!NT_SUCCESS(Status)) goto Quickie;

    //
    // Now get the full path again
    //
    Length = RtlGetFullPathName_Ustr(OriginalName,
                                     FullPathName->Length,
                                     FullPathName->Buffer,
                                     NULL,
                                     &InvalidName,
                                     &PathType);
    if (!(Length) || (Length > FullPathName->Length))
    {
        //
        // Fail
        //
        LdrpFreeUnicodeString(FullPathName);
        Status = STATUS_NAME_TOO_LONG;
    }
    else
    {
        //
        // Return the expanded name
        //
        *ExpandedName = FullPathName;
        FullPathName->Length = (USHORT)Length;
    }

Quickie:
    //
    // Unlock the PEB
    //
    RtlLeaveCriticalSection(&FastPebLock);

    //
    // Check if we should display
    //
    if (ShowSnaps)
    {
        //
        // Check if we got here through success
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // Print a message
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - Expanded to %wZ\n",
                       __FUNCTION__,
                       *ExpandedName);
        }
        else
        {
            //
            // Print a message
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                        0,
                        "LDR: %s - Failed to expand %wZ; 0x%08x\n",
                        __FUNCTION__,
                        OriginalName,
                        Status);
        }
    }

    //
    // If we failed, return NULL
    //
    if (!NT_SUCCESS(Status)) *ExpandedName = NULL;

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpSearchPath
*
* The LdrpSearchPath routine FILLMEIN
*
* @param SearchPath
*        FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param PathName
*        FILLMEIN
*
* @param FullPathName
*        FILLMEIN
*
* @param ExpandedName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpSearchPath(IN PWCHAR *SearchPath,
               IN PWCHAR DllName,
               IN PUNICODE_STRING PathName,
               IN PUNICODE_STRING FullPathName,
               IN PUNICODE_STRING *ExpandedName)
{
    BOOLEAN TryAgain = FALSE;
    PWCHAR ActualSearchPath = *SearchPath;
    UNICODE_STRING TestName;
    NTSTATUS Status;
    PWCHAR Buffer, BufEnd;
    ULONG Length = 0;
    WCHAR p;
    PWCHAR pp;

    //
    // Check if we don't have a search path
    //
    if (!ActualSearchPath) *SearchPath = LdrpDefaultPath.Buffer;

    //
    // Check if we should display
    //
    if (ShowSnaps)
    {
        //
        // Print a message
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - Looking for %ws in %ws\n",
                   __FUNCTION__,
                   DllName,
                   *SearchPath);
    }

    //
    // Check if we're dealing with a relative path
    //
    if (RtlDetermineDosPathNameType_U(DllName) != RtlPathTypeRelative)
    {
        //
        // Good, we're not. Create the name string
        //
        Status = RtlInitUnicodeStringEx(&TestName, DllName);
        if (!NT_SUCCESS(Status)) goto Quickie;

        //
        // Make sure it exists
        //
        if (!RtlDoesFileExists_UstrEx(&TestName, TRUE))
        {
            //
            // It doesn't, fail
            //
            Status = STATUS_DLL_NOT_FOUND;
            goto Quickie;
        }

        //
        // Resolve the full name
        //
        Status = LdrpResolveFullName(&TestName,
                                     PathName,
                                     FullPathName,
                                     ExpandedName);
        goto Quickie;
    }

    //
    // FIXME: Handle relative case semicolon-lookup here
    //

    //
    // Calculate length
    //
    Length += (ULONG)wcslen(DllName) + sizeof(UNICODE_NULL);
    if (Length > UNICODE_STRING_MAX_CHARS)
    {
        //
        // Too long, fail
        //
        Status = STATUS_NAME_TOO_LONG;
        goto Quickie;
    }

    //
    // Allocate buffer
    //
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
    if (!Buffer)
    {
        //
        // Fail
        //
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    //
    // FIXME: Setup TestName here
    //
    Status = STATUS_NOT_FOUND;

    //
    // Start loop
    //
    do
    {
        //
        // Get character
        //
        p = *ActualSearchPath;
        if (!(p) && (p == ';'))
        {
            //
            // We don't have a character, or is a semicolon.
            // FIXME: Handle this case
            //

            //
            // Check if we should display
            //
            if (ShowSnaps)
            {
                //
                // Print a message
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - Looking for %ws\n",
                           __FUNCTION__,
                           Buffer);
            }

            //
            // Sanity check
            //
            TestName.Length = (USHORT)ALIGN_DOWN((BufEnd - Buffer), WCHAR);
            ASSERT(TestName.Length < TestName.MaximumLength);

            //
            // Check if the file exists
            //
            if (RtlDoesFileExists_UstrEx(&TestName, FALSE))
            {
                //
                // It does. Reallocate the buffer
                //
                TestName.MaximumLength = (USHORT)ALIGN_DOWN((BufEnd - Buffer),
                                                            WCHAR) +
                                         sizeof(WCHAR);
                TestName.Buffer = RtlReAllocateHeap(RtlGetProcessHeap(),
                                                    0,
                                                    Buffer,
                                                    TestName.MaximumLength);
                if (!TestName.Buffer)
                {
                    //
                    // Keep the old one
                    //
                    TestName.Buffer = Buffer;
                }
                else
                {
                    //
                    // Update buffer
                    //
                    Buffer = TestName.Buffer;
                }

                //
                // Make sure we have a buffer at least
                //
                ASSERT(TestName.Buffer);

                //
                // Resolve the name
                //
                *SearchPath = ActualSearchPath++;
                Status = LdrpResolveFullName(&TestName,
                                             PathName,
                                             FullPathName,
                                             ExpandedName);
                break;
            }

            //
            // Update buffer end
            //
            BufEnd = Buffer;

//SkipIt:
            //
            // Update string position
            //
            pp = ActualSearchPath++;
        }
        else
        {
            //
            // Otherwise, write the character
            //
            *BufEnd = p;
            BufEnd++;
        }

        //
        // Check if the string is empty, meaning we're done
        //
        if (!(*ActualSearchPath)) TryAgain = TRUE;

        //
        // Advance in the string
        //
        ActualSearchPath++;
    } while (!TryAgain);

    //
    // Check if we had a buffer and free it
    //
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

Quickie:
    //
    // Check if we got here through failure
    //
    if (!NT_SUCCESS(Status)) *ExpandedName = NULL;

    //
    // Check if we should display
    //
    if (ShowSnaps)
    {
        //
        // Check for success
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Print a message
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - Returning %wZ\n",
                       __FUNCTION__,
                       *ExpandedName);
        }
        else
        {
            //
            // Print a message
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s -  Unable to locate %ws in %ws: 0x%08x\n",
                       __FUNCTION__,
                       DllName,
                       ActualSearchPath,
                       Status);
        }
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpResolveDllName
*
* The LdrpResolveDllName routine FILLMEIN
*
* @param SearchPath
*        FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param Redirected
*        FILLMEIN
*
* @param FullDllName
*        FILLMEIN
*
* @param BaseDllName
*        FILLMEIN
*
* @param DllHandle
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpResolveDllName(IN PWCHAR SearchPath OPTIONAL,
                   OUT PWCHAR *DllPath,
                   IN PWCHAR DllName,
                   IN BOOLEAN Redirected,
                   OUT PUNICODE_STRING FullDllName,
                   OUT PUNICODE_STRING BaseDllName,
                   OUT HANDLE *DllHandle)
{
    UNICODE_STRING DllNameString, PrivateName;
    USHORT Position = 0;
    PWCHAR ActualSearchPath;
    WCHAR NameBuffer[40];
    UNICODE_STRING NameString, PathString;
    PUNICODE_STRING ExpandedName = NULL;
    NTSTATUS Status;
    USHORT Delta;

    //
    // Initialize the name string
    //
    RtlInitEmptyUnicodeString(&DllNameString, NULL, 0);

    //
    // Check if we have a path, if not, use the default one
    //
    ActualSearchPath = (SearchPath) ? SearchPath : LdrpDefaultPath.Buffer;

    //
    // Assume NULL for the caller
    //
    if (DllPath) *DllPath = NULL;

    //
    // Initialize the name string and path string
    //
    RtlInitEmptyUnicodeString(&NameString, NameBuffer, sizeof(NameBuffer));
    RtlInitUnicodeString(&PathString, NameBuffer);

    //
    // Assume NULL for the caller
    //
    if (DllHandle) *DllHandle = NULL;

    //
    // Initialize Dll name strings
    //
    if (FullDllName) RtlInitEmptyUnicodeString(FullDllName, NULL, 0);
    if (BaseDllName) RtlInitEmptyUnicodeString(BaseDllName, NULL, 0);

    //
    // Make sure all our parameters are there
    //
    if (!(FullDllName) || !(BaseDllName) || !(DllHandle))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the DLL Name String
    //
    RtlInitUnicodeString(&DllNameString, DllName);

    //
    // Resolve the name
    //
    Status = LdrpResolveDllNameForAppPrivateRedirection(&DllNameString,
                                                        &PrivateName);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s failed calling LdrpResolveDllNameFor"
                   "AppPrivateRedirection with status %lx\n",
                   __FUNCTION__,
                   Status);
        return Status;
    }

    //
    // Check if we have a private name
    //
    if (PrivateName.Length)
    {
        //
        // Return it
        //
        FullDllName->Length = PrivateName.Length;
        FullDllName->Buffer = PrivateName.Buffer;
    }
    else
    {
        //
        // Check if we are redirected
        //
        if (Redirected)
        {
            //
            // We are, so we already have a full path. Copy it
            //
            Status = LdrpCopyUnicodeString(FullDllName, &DllNameString);
            if (!NT_SUCCESS(Status))
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s failed call to LdrpCopyUnicodeString() in"
                           "redirected case; status = %lx\n",
                           __FUNCTION__,
                           Status);
                return Status;
            }
        }
        else
        {
            //
            // Do a path lookup
            //
            Status = LdrpSearchPath(&ActualSearchPath,
                                    DllName,
                                    &NameString,
                                    &PathString,
                                    &ExpandedName);
            if (!NT_SUCCESS(Status)) return STATUS_DLL_NOT_FOUND;

            //
            // If the path was requested and we have one, return it
            //
            if ((DllPath) && (*SearchPath)) *DllPath = SearchPath;

            //
            // Check if the strings match
            //
            if (ExpandedName != &NameString)
            {
                //
                // Copy the expanded name
                //
                Status = LdrpCopyUnicodeString(FullDllName, ExpandedName);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // Fail
                    //
                    DbgPrintEx(DPFLTR_LDR_ID,
                               0,
                               "LDR: %s failed call to LdrpCopyUnicodeString()"
                               " in redirected case; status = %lx\n",
                               __FUNCTION__,
                               Status);
                    return Status;
                }
            }
            else
            {
                //
                // Just update the string data
                //
                FullDllName->Length = ExpandedName->Length;
                FullDllName->MaximumLength = ExpandedName->MaximumLength;
                FullDllName->Buffer = ExpandedName->Buffer;
            }
        }
    }

    //
    // Try to find a path separator
    //
    Status = RtlFindCharInUnicodeString(1,
                                        FullDllName,
                                        &RtlDosPathSeparatorsString,
                                        &Position);
    if (Status == STATUS_NOT_FOUND)
    {
        //
        // No separator found: we have a clean base name. Return it
        //
        BaseDllName->Length = FullDllName->Length;
        BaseDllName->MaximumLength = FullDllName->MaximumLength;
        BaseDllName->Buffer = FullDllName->Buffer;
    }
    else if (!NT_SUCCESS(Status))
    {
        //
        // Display message
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s failing because RtlFindCharInUnicodeString"
                   "failed with status %x\n",
                   __FUNCTION__,
                   Status);

        //
        // Free the string
        //
        LdrpFreeUnicodeString(FullDllName);

        //
        // Clear it
        //
        RtlInitEmptyUnicodeString(FullDllName, NULL, 0);
        return Status;
    }

    //
    // Separate the base name from the full name
    //
    Delta = Position + sizeof(WCHAR);
    BaseDllName->Length = FullDllName->Length - Delta;
    BaseDllName->MaximumLength = FullDllName->MaximumLength - Delta;
    BaseDllName->Buffer = (PWCHAR)((ULONG_PTR)FullDllName->Buffer + Delta);
    return STATUS_SUCCESS;
}

/*++
* @name LdrpCheckForLoadedDll
*
* The LdrpCheckForLoadedDll routine FILLMEIN
*
* @param SearchPath
*        FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param Static
*        FILLMEIN
*
* @param Redirected
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrpCheckForLoadedDll(IN PWCHAR SearchPath OPTIONAL,
                      IN PUNICODE_STRING DllName,
                      IN BOOLEAN Static,
                      IN BOOLEAN Redirected,
                      OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    WCHAR NameBuffer[40];
    UNICODE_STRING Unknown, DllNameString;
    ULONG Chars;
    BOOLEAN Result;
    PWCHAR p;
    ULONG HashIndex;
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY Entry;
    PUNICODE_STRING CompareName;
    BOOLEAN FullPath = FALSE;
    PUNICODE_STRING ExpandedName;
    UNICODE_STRING NtPathName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DllHandle, SectionHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize = 0;
    PIMAGE_NT_HEADERS NtHeader, NtHeader2;

    //
    // Initalize name/path strings
    //
    RtlInitUnicodeString(&Unknown, NULL);
    RtlInitEmptyUnicodeString(&DllNameString, NameBuffer, sizeof(NameBuffer));

    //
    // Make sure we have a name
    //
    if (!(DllName->Buffer) || !(DllName->Buffer[0])) return FALSE;

    //
    // Check if this was a static link
    //
    if (Static)
    {
StaticLookup:
        //
        // Check if we are redirected
        //
        if (Redirected)
        {
            //
            // Find out how many chars to loop
            //
            Chars = DllName->Length / sizeof(WCHAR);
            if (DllName->Length) Chars--;

            //
            // Loop the DLL Name
            //
            p = &DllName->Buffer[Chars * sizeof(WCHAR)];
            while (p != DllName->Buffer)
            {
                //
                // Check if it's a path separator
                //
                if ((*p == '\\') || (*p == '/')) break;
                p--;
            }

            //
            // Check if our loop worked
            //
            ASSERTMSG(p != DllName->Buffer,
                      "Redirected DLL name does not have full path; either "
                      "caller lied or redirection info is in error");
            if (p == DllName->Buffer)
            {
                //
                // Check if we should show a debug message
                //
                if (ShowSnaps)
                {
                    //
                    // Notify debugger
                    //
                    DbgPrint("LDR: Failing LdrpCheckForLoadedDll because "
                             "redirected DLL name %wZ does not include a "
                             "slash\n");
                }

                //
                // Fail
                //
                Result = FALSE;
                goto Quickie;
            }

            //
            // Get the Hash Index, skipping the name's slash
            //
            HashIndex = LDR_GET_HASH_ENTRY(DllName->Buffer[1]);
        }
        else
        {
            //
            // Get the Hash Index
            //
            HashIndex = LDR_GET_HASH_ENTRY(DllName->Buffer[0]);
        }

        //
        // Loop the hash table
        //
        ListHead = &LdrpHashTable[HashIndex];
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            //
            // Get the entry
            //
            Entry = CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      HashLinks);

            //
            // Increase profiling data
            //
            LdrpCompareCount++;

            //
            // Check if the DLL is redirected
            //
            if (Redirected)
            {
                //
                // Check if this entry is redirected
                //
                if (Entry->Flags & LDRP_REDIRECTED)
                {
                    //
                    // It is, so compare the entire name
                    //
                    CompareName = &Entry->FullDllName;
                }
            }
            else
            {
                //
                // Check if this entry is redirected
                //
                if (!(Entry->Flags & LDRP_REDIRECTED))
                {
                    //
                    // It's not, compare the base name
                    //
                    CompareName = &Entry->BaseDllName;
                }
            }

            //
            // Make sure we have something to compare (this means that the
            // Loader's Data Table Entry's redirection flag matches our callers
            // flag).
            //
            if (CompareName)
            {
                //
                // Compare the names
                //
                if (RtlEqualUnicodeString(DllName, CompareName, TRUE))
                {
                    //
                    // Found it. Return success
                    //
                    *LdrEntry = Entry;
                    Result = TRUE;
                    goto Quickie;
                }
            }

            //
            // Move to the next one
            //
            NextEntry = NextEntry->Flink;
        }

        //
        // All done here
        //
        Result = FALSE;
        goto Quickie;
    }

    //
    // This is a dynamic lookup. Check if we're redirected
    //
    if (Redirected)
    {
        //
        // We have a full path for sure
        //
        FullPath = TRUE;

        //
        // Prepare the name string
        //
        DllNameString.Length = DllName->Length;
        DllNameString.MaximumLength = DllName->MaximumLength;
        DllNameString.Buffer = DllName->Buffer;
        ExpandedName = &DllNameString;
    }
    else
    {
        //
        // Loop the DLL Name
        //
        p = DllName->Buffer;
        while (*p)
        {
            //
            // Get a charachter, and check if it's a path separator
            //
            p++;
            if ((*p == '\\') || (*p == '/'))
            {
                //
                // Found one. Remember that this name contains the full path
                //
                FullPath = TRUE;

                //
                // Do a path search
                //
                Status = LdrpSearchPath(&SearchPath,
                                        DllName->Buffer,
                                        &DllNameString,
                                        &Unknown,
                                        &ExpandedName);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // We've failed, or the path was too long
                    //
                    if (ShowSnaps)
                    {
                        //
                        // Display debug message
                        //
                        DbgPrint("LDR: LdrpCheckForLoadedDll - Unable To "
                                 "located %ws: 0x%08x\n",
                                 DllName->Buffer,
                                 Status);
                    }

                    //
                    // Fail
                    //
                    Result = FALSE;
                    goto Quickie;
                }
            }
        }
    }

    //
    // Check if we we got a DLL name without a path
    //
    if (!FullPath)
    {
        //
        // Check if we found an activation context for this name
        //
        Status = RtlFindActivationContextSectionString(NULL,
                                                       NULL,
                                                       2,
                                                       DllName,
                                                       NULL);
        if (NT_SUCCESS(Status))
        {
            //
            // We did, which means we can't do a static lookup
            //
            Result = FALSE;
            goto Quickie;
        }

        //
        // Do a static lookup instead
        //
        goto StaticLookup;
    }

    //
    // Loop the loaded DLLs
    //
    Result = FALSE;
    ListHead = &PebLdr.InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the current entry, and move to the next
        //
        Entry = CONTAINING_RECORD(NextEntry,
                                  LDR_DATA_TABLE_ENTRY,
                                  InLoadOrderLinks);
        NextEntry = NextEntry->Flink;

        //
        // Skip this entry if it's not linked in memory anymore
        //
        if (!Entry->InMemoryOrderModuleList.Flink) continue;

        //
        // Compare the names
        //
        if (RtlEqualUnicodeString(ExpandedName, &Entry->FullDllName, TRUE))
        {
            //
            // Found it, break out and return it
            //
            *LdrEntry = Entry;
            Result = TRUE;
            goto Quickie;
        }
    }

    //
    // We couldn't find the DLL, so we'll try loading it. Get its NT name
    //
    if (!RtlDosPathNameToNtPathName_U(ExpandedName->Buffer,
                                      &NtPathName,
                                      NULL,
                                      NULL))
    {
        //
        // We've failed
        //
        Result = FALSE;
        goto Quickie;
    }

    //
    // Initialize the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the DLL
    //
    Status = NtOpenFile(&DllHandle,
                        SYNCHRONIZE | FILE_EXECUTE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE |
                        FILE_SYNCHRONOUS_IO_NONALERT);

    //
    // Free the NT Name
    //
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail if we couldn't open the DLL
        //
        Result = FALSE;
        goto Quickie;
    }

    //
    // Create a section for it
    //
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_READ |
                             SECTION_MAP_EXECUTE |
                             SECTION_MAP_WRITE,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_COMMIT,
                             DllHandle);
    NtClose(DllHandle);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail if we couldn't create the section
        //
        Result = FALSE;
        goto Quickie;
    }

    //
    // Map it
    //
    Status = ZwMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_EXECUTE);
    NtClose(SectionHandle);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail if we couldn't map the section
        //
        Result = FALSE;
        goto Quickie;
    }

    //
    // Get the NT Header
    //
    Status = RtlImageNtHeaderEx(0,
                                ViewBase,
                                (ULONGLONG)ViewSize,
                                &NtHeader);
    if (!(NT_SUCCESS(Status)) || !(NtHeader))
    {
        //
        // Fail
        //
        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
        Result = FALSE;
        goto Quickie;
    }

    //
    // Loop the loaded DLLs
    //
    ListHead = &PebLdr.InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the current entry, and move to the next
        //
        Entry = CONTAINING_RECORD(NextEntry,
                                  LDR_DATA_TABLE_ENTRY,
                                  InLoadOrderLinks);
        NextEntry = NextEntry->Flink;

        //
        // Skip this entry if it's not linked in memory anymore
        //
        if (!Entry->InMemoryOrderModuleList.Flink) continue;

        //
        // Check if this entry matches this file's timestamp and size
        //
        if ((Entry->TimeDateStamp == NtHeader->FileHeader.TimeDateStamp) &&
            (Entry->SizeOfImage == NtHeader->OptionalHeader.SizeOfImage))
        {
            //
            // It matches, get the entry's NT Header and match it entirely
            //
            NtHeader2 = RtlImageNtHeader(Entry->DllBase);
            if (RtlEqualMemory(NtHeader2, NtHeader, sizeof(IMAGE_NT_HEADERS)))
            {
                //
                // Even the headers match. Now ask the kernel for a full check
                //
                Status = ZwAreMappedFilesTheSame(Entry->DllBase, ViewBase);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // We got really close, but they don't actually match!
                    //
                    continue;
                }
                else
                {
                    //
                    // We found it! Return its pointer and jump to exit
                    //
                    *LdrEntry = Entry;
                    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
                    Result = TRUE;
                    goto Quickie;
                }
            }
        }
    }

    //
    // Unmap the image
    //
    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);

Quickie:
    //
    // Free the ??? string and return
    //
    LdrpFreeUnicodeString(&Unknown);
    return Result;
}

/*++
* @name LdrpCheckForLoadedDllHandle
*
* The LdrpCheckForLoadedDllHandle routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrpCheckForLoadedDllHandle(IN PVOID BaseAddress,
                            OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    PLDR_DATA_TABLE_ENTRY Current;
    PLIST_ENTRY ListHead, NextEntry;

    //
    // Check the cache first
    //
    if ((LdrpLoadedDllHandleCache) &&
        (LdrpLoadedDllHandleCache->DllBase == BaseAddress))
    {
        //
        // We got lucky, return the cached entry
        //
        *LdrEntry = LdrpLoadedDllHandleCache;
        return TRUE;
    }

    //
    // Do a lookup
    //
    ListHead = &PebLdr.InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while(NextEntry != ListHead)
    {
        //
        // Get the current entry
        //
        Current =  CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        //
        // Make sure it's not unloading and check for a match
        //
        if ((Current->InMemoryOrderModuleList.Flink) &&
            (BaseAddress = Current->DllBase))
        {
            //
            // Save in cache
            //
            LdrpLoadedDllHandleCache = Current;

            //
            // Return it
            //
            *LdrEntry = Current;
            return TRUE;
        }

        //
        // Move to the next one
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Nothing found
    //
    return FALSE;
}

/*++
* @name LdrpUpdateLoadCount3
*
* The LdrpUpdateLoadCount3 routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param UpdateString
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
NTAPI
LdrpUpdateLoadCount3(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                     IN ULONG Flags,
                     OUT PUNICODE_STRING UpdateString)
{
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PIMAGE_IMPORT_DESCRIPTOR IatEntry;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry;
    PIMAGE_BOUND_FORWARDER_REF BoundForwarder;
    LPSTR ImportName, BoundStringBase;
    ULONG i, ImportSize, BoundSize;
    ANSI_STRING TempString;
    PUNICODE_STRING ImportNameString;
    PLDR_DATA_TABLE_ENTRY DllEntry;
    NTSTATUS Status;
    BOOLEAN RedirectedDll = FALSE, RedirectedDll2 = FALSE, RedirectedDll3 = FALSE;
    PIMAGE_THUNK_DATA FirstThunk;
    UNICODE_STRING DllString1;
    PUNICODE_STRING DllString2;
    LPSTR DebugString;

    //
    // Set up the Activation Context
    //
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format =
        RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
    RtlZeroMemory(&ActCtx, sizeof(ActCtx));

    //
    // Activate it
    //
    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           LdrEntry->
                                           EntryPointActivationContext);

    //
    // Check the flags
    //
    if (Flags = LDRP_REFCOUNT)
    {
        //
        // Is it already loading?
        //
        if (LdrEntry->Flags & LDRP_LOAD_IN_PROGRESS)
        {
            //
            // Return
            //
            goto Quickie;
        }
        else
        {
            //
            // Mark it in loading
            //
            LdrEntry->Flags |= LDRP_LOAD_IN_PROGRESS;
        }
    }
    else if (Flags = LDRP_DEREFCOUNT)
    {
        //
        // Is it unloading
        //
        if (LdrEntry->Flags & LDRP_UNLOAD_IN_PROGRESS)
        {
            //
            // Return
            //
            goto Quickie;
        }
        else
        {
            //
            // Mark it unloading
            //
            LdrEntry->Flags |= LDRP_UNLOAD_IN_PROGRESS;
        }
    }
    else if (Flags = LDRP_PIN)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check for .NET Image
    //
    if (LdrEntry->Flags & LDRP_COR_IMAGE)
    {
        //
        // Make sure mscore is loaded
        //
        DllString2 = &MsCoreeDllString;
        if (!LdrpCheckForLoadedDll(NULL,
                                   DllString2,
                                   TRUE,
                                   FALSE,
                                   &DllEntry))
        {
            //
            // It's not, return
            //
            goto Quickie;
        }

        //
        // Check if it's locked
        //
        if (DllEntry->LoadCount != -1)
        {
            //
            // See what we're supposed to do with it
            //
            if (Flags == LDRP_REFCOUNT)
            {
                //
                // We're referencing
                //
                DllEntry->LoadCount++;
                DebugString = "Refcount";
            }
            else if (Flags == LDRP_DEREFCOUNT)
            {
                //
                // We're dereferencing
                //
                DllEntry->LoadCount--;
                DebugString = "Derefcount";
            }
            else
            {
                //
                // We're pinning
                //
                DllEntry->LoadCount |= -1;
                DebugString = "Pin";
            }

            //
            // Display debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: %s %wZ (%lx)\n", 
                         DebugString,
                         DllString2,
                         DllEntry->LoadCount);
            }
        }

        //
        // Update the load count of this DLL
        //
        LdrpUpdateLoadCount3(DllEntry, Flags, UpdateString);
        goto Quickie;
    }

    //
    // Use our TEB String as a buffer
    //
    ImportNameString = &NtCurrentTeb()->StaticUnicodeString;

    //
    // First try to handle Bound IAT
    //
    BoundEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                              &BoundSize);
    if (BoundEntry)
    {
        //
        // Check which operation we're doing
        //
        if ((Flags == LDRP_REFCOUNT) || (Flags == LDRP_PIN))
        {
            //
            // Mark it as loading
            //
            LdrEntry->Flags |= LDRP_LOAD_IN_PROGRESS;
        }
        else if (Flags == LDRP_DEREFCOUNT)
        {
            //
            // Mark it as unloading
            //
            LdrEntry->Flags |= LDRP_UNLOAD_IN_PROGRESS;
        }

        //
        // The base address of the strings will never change, so save it
        //
        BoundStringBase = (LPSTR)BoundEntry;

        //
        // Make sure we have a name, and loop
        //
        while (BoundEntry->OffsetModuleName)
        {
            //
            // Get the name VA
            //
            ImportName = BoundStringBase + BoundEntry->OffsetModuleName;

            //
            // Convert it to a Unicode String
            //
            RtlInitAnsiString(&TempString, ImportName);
            Status = RtlAnsiStringToUnicodeString(ImportNameString,
                                                  &TempString,
                                                  FALSE);

            //
            // If we failed, skip it
            //
            if (!NT_SUCCESS(Status)) goto SkipBoundEntry;

            //
            // Initialize the strings
            //
            DllString2 = ImportNameString;
            RtlInitEmptyUnicodeString(&DllString1, NULL, 0);

            //
            // Check if the SxS Assemblies specify another file
            //
            Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                              ImportNameString,
                                                              &DefaultExtension,
                                                              UpdateString,
                                                              &DllString1,
                                                              &DllString2,
                                                              NULL,
                                                              NULL,
                                                              NULL);
            //
            // Check success
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Remember this for later
                //
                RedirectedDll = TRUE;
            }
            else if (Status != STATUS_SXS_KEY_NOT_FOUND)
            {
                //
                // Unrecoverable SxS failure;
                //
                goto SkipBoundEntry;
            }

            //
            // Set success
            //
            Status = STATUS_SUCCESS;

            //
            // Now check if this DLL is loaded
            //
            if (LdrpCheckForLoadedDll(NULL,
                                      DllString2,
                                      TRUE,
                                      RedirectedDll,
                                      &DllEntry))
            {
                //
                // Check if it's locked
                //
                if (DllEntry->LoadCount != -1)
                {
                    //
                    // See what we're supposed to do with it
                    //
                    if (Flags == LDRP_REFCOUNT)
                    {
                        //
                        // We're referencing
                        //
                        DllEntry->LoadCount++;
                        DebugString = "Refcount";
                    }
                    else if (Flags == LDRP_DEREFCOUNT)
                    {
                        //
                        // We're dereferencing
                        //
                        DllEntry->LoadCount--;
                        DebugString = "Derefcount";
                    }
                    else
                    {
                        //
                        // We're pinning
                        //
                        DllEntry->LoadCount |= -1;
                        DebugString = "Pin";
                    }

                    //
                    // Display debug message
                    //
                    if (ShowSnaps)
                    {
                        DbgPrint("LDR: %s %wZ (%lx)\n", 
                                 DebugString,
                                 DllString2,
                                 DllEntry->LoadCount);
                    }
                }

                //
                // Update the load count of this DLL
                //
                LdrpUpdateLoadCount3(DllEntry, Flags, UpdateString);
            }

            //
            // Check if we have a string to free
            //
            if (DllString1.Buffer) RtlFreeUnicodeString(&DllString1);

SkipBoundEntry:
            //
            // Get the forwarder
            //
            BoundForwarder = (PIMAGE_BOUND_FORWARDER_REF)(BoundEntry + 1);

            //
            // Loop it
            //
            for (i = 0; i < BoundEntry->NumberOfModuleForwarderRefs; i++)
            {
                //
                // Get the name VA 
                //
                ImportName = BoundStringBase +
                             BoundForwarder->OffsetModuleName;

                //
                // Convert it to a Unicode String
                //
                RtlInitAnsiString(&TempString, ImportName);
                Status = RtlAnsiStringToUnicodeString(ImportNameString,
                                                      &TempString,
                                                      FALSE);

                //
                // If we failed, skip it
                //
                if (!NT_SUCCESS(Status)) goto SkipForwarderEntry;

                //
                // Initialize the strings
                //
                DllString2 = ImportNameString;
                RtlInitEmptyUnicodeString(&DllString1, NULL, 0);

                //
                // Check if the SxS Assemblies specify another file
                //
                Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                                  ImportNameString,
                                                                  &DefaultExtension,
                                                                  UpdateString,
                                                                  &DllString1,
                                                                  &DllString2,
                                                                  NULL,
                                                                  NULL,
                                                                  NULL);
                //
                // Check success
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // Remember this for later
                    //
                    RedirectedDll2 = TRUE;
                }
                else if (Status != STATUS_SXS_KEY_NOT_FOUND)
                {
                    //
                    // Unrecoverable SxS failure;
                    //
                    goto SkipForwarderEntry;
                }

                //
                // Set success
                //
                Status = STATUS_SUCCESS;

                //.
                // Now check if this DLL is loaded
                //
                if (LdrpCheckForLoadedDll(NULL,
                                          DllString2,
                                          TRUE,
                                          RedirectedDll2,
                                          &DllEntry))
                {
                    //
                    // Check if it's locked
                    //
                    if (DllEntry->LoadCount != -1)
                    {
                        //
                        // See what we're supposed to do with it
                        //
                        if (Flags == LDRP_REFCOUNT)
                        {
                            //
                            // We're referencing
                            //
                            DllEntry->LoadCount++;
                            DebugString = "Refcount";
                        }
                        else if (Flags == LDRP_DEREFCOUNT)
                        {
                            //
                            // We're dereferencing
                            //
                            DllEntry->LoadCount--;
                            DebugString = "Derefcount";
                        }
                        else
                        {
                            //
                            // We're pinning
                            //
                            DllEntry->LoadCount |= -1;
                            DebugString = "Pin";
                        }

                        //
                        // Display debug message
                        //
                        if (ShowSnaps)
                        {
                            DbgPrint("LDR: %s %wZ (%lx)\n", 
                                     DebugString,
                                     DllString2,
                                     DllEntry->LoadCount);
                        }
                    }

                    //
                    // Update the load count of this DLL
                    //
                    LdrpUpdateLoadCount3(DllEntry, Flags, UpdateString);
                }

                //
                // Check if we have a string to free
                //
                if (DllString1.Buffer) RtlFreeUnicodeString(&DllString1);

SkipForwarderEntry:
                //
                // Move to the next forwarder
                //
                BoundForwarder++;
            }

            //
            // Continue with the Forwarder as Entry
            //
            BoundEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)BoundForwarder;
        }

        //
        // We're done, return
        //
        goto Quickie;
    }

    //
    // No Bound IAT, look for a normal one
    //
    IatEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &ImportSize);
    if (IatEntry)
    {
        //
        // Make sure we have a name and thunk
        //
        while ((IatEntry->Name) && (IatEntry->FirstThunk))
        {
            //
            // Get the thunk VA
            //
            FirstThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrEntry->DllBase +
                                             IatEntry->FirstThunk);

            //
            // Weird import, skip it
            //
            if (!FirstThunk->u1.Function) goto SkipEntry;

            //
            // Get the import's name
            //
            ImportName = (LPSTR)((ULONG_PTR)LdrEntry->DllBase + IatEntry->Name);

            //
            // Convert it to a Unicode String
            //
            RtlInitAnsiString(&TempString, ImportName);
            Status = RtlAnsiStringToUnicodeString(ImportNameString,
                                                  &TempString,
                                                  FALSE);
            if (!NT_SUCCESS(Status)) goto SkipEntry;

            //
            // Initialize the strings
            //
            DllString2 = ImportNameString;
            RtlInitEmptyUnicodeString(&DllString1, NULL, 0);

            //
            // Check if the SxS Assemblies specify another file
            //
            Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                              ImportNameString,
                                                              &DefaultExtension,
                                                              UpdateString,
                                                              &DllString1,
                                                              &DllString2,
                                                              NULL,
                                                              NULL,
                                                              NULL);
            //
            // Check success
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Remember this for later
                //
                RedirectedDll3 = TRUE;
            }
            else if (Status != STATUS_SXS_KEY_NOT_FOUND)
            {
                //
                // Unrecoverable SxS failure;
                //
                goto SkipEntry;
            }

            //
            // Set success
            //
            Status = STATUS_SUCCESS;

            //
            // Now check if this DLL is loaded
            //
            if (LdrpCheckForLoadedDll(NULL,
                                      DllString2,
                                      TRUE,
                                      RedirectedDll3,
                                      &DllEntry))
            {
                //
                // Check if it's locked
                //
                if (DllEntry->LoadCount != -1)
                {
                    //
                    // See what we're supposed to do with it
                    //
                    if (Flags == LDRP_REFCOUNT)
                    {
                        //
                        // We're referencing
                        //
                        DllEntry->LoadCount++;
                        DebugString = "Refcount";
                    }
                    else if (Flags == LDRP_DEREFCOUNT)
                    {
                        //
                        // We're dereferencing
                        //
                        DllEntry->LoadCount--;
                        DebugString = "Derefcount";
                    }
                    else
                    {
                        //
                        // We're pinning
                        //
                        DllEntry->LoadCount |= -1;
                        DebugString = "Pin";
                    }

                    //
                    // Display debug message
                    //
                    if (ShowSnaps)
                    {
                        DbgPrint("LDR: %s %wZ (%lx)\n", 
                                 DebugString,
                                 DllString2,
                                 DllEntry->LoadCount);
                    }
                }

                //
                // Update the load count of this DLL
                //
                LdrpUpdateLoadCount3(DllEntry, Flags, UpdateString);
            }

            //
            // Check if we have a string to free
            //
            if (DllString1.Buffer) RtlFreeUnicodeString(&DllString1);
        }

SkipEntry:
        //
        // Move to the next entry
        //
        IatEntry++;
    }

Quickie:
    //
    // Free the activation context
    //
    RtlDeactivateActivationContextUnsafeFast(&ActCtx);
}

/*++
* @name LdrpUpdateLoadCount2
*
* The LdrpUpdateLoadCount2 routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpUpdateLoadCount2(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                     IN ULONG Flags)
{
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING UpdateString;

    //
    // Setup the string
    //
    RtlInitEmptyUnicodeString(&UpdateString, Buffer, sizeof(Buffer));

    //
    // Call the extended API
    //
    LdrpUpdateLoadCount3(LdrEntry, Flags, &UpdateString);
}

/*++
* @name LdrpRunInitializeRoutines
*
* The LdrpRunInitializeRoutines routine FILLMEIN
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
LdrpRunInitializeRoutines(IN PCONTEXT Context OPTIONAL)
{
    PLDR_DATA_TABLE_ENTRY LocalArray[16];
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry, *LdrRootEntry, OldInitializer;
    PVOID EntryPoint;
    ULONG NumberOfRoutines, i;
    BOOLEAN Result;
    ULONG BreakOnInit;
    NTSTATUS Status = STATUS_SUCCESS;
    PPEB Peb = NtCurrentPeb();
    PTEB OldTeb;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;

    //
    // Check the Loader Lock
    //
    LdrpEnsureLoaderLockIsHeld();

    //
    // Get the number of entries to call
    //
    i = LdrpClearLoadInProgress();
    if (i)
    {
        //
        // Check if we can use our local buffer
        //
        if (i > 16)
        {
            //
            // Allocate space for all the entries
            //
            LdrRootEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           i * sizeof(LdrRootEntry));
            if (!LdrRootEntry)
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - failed to allocate dynamic array of %u "
                           "DLL initializers to run\n",
                           __FUNCTION__,
                           i);
                return STATUS_NO_MEMORY;
            }
        }
        else
        {
            //
            // Use our local array
            //
            LdrRootEntry = LocalArray;
        }
    }
    else
    {
        //
        // Don't need one
        //
        LdrRootEntry = NULL;
    }

    //
    // Show debug message
    //
    if ((ShowSnaps) || (LdrpShowInitRoutines))
    {
        DbgPrint("[%x,%x] LDR: Real INIT LIST for Process %wZ pid %u %0x%x\n",
                 NtCurrentTeb()->Cid.UniqueThread,
                 NtCurrentTeb()->Cid.UniqueProcess,
                 Peb->ProcessParameters->ImagePathName,
                 NtCurrentTeb()->Cid.UniqueThread,
                 NtCurrentTeb()->Cid.UniqueProcess);
    }

    //
    // Loop the modules
    //
    ListHead = &PebLdr.InInitializationOrderModuleList;
    NextEntry = ListHead->Flink;
    NumberOfRoutines = 0;
    while (NextEntry != NextEntry)
    {
        //
        // Get the Data Entry
        //
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderModuleList);

        //
        // Check if we have a Root Entry
        //
        if (LdrRootEntry)
        {
            //
            // Check if it hasn't been processed yet
            //
            if (!(LdrEntry->Flags & LDRP_ENTRY_PROCESSED))
            {
                //
                // Setup the Cookie for the DLL
                //
                LdrpInitSecurityCookie(LdrEntry);

                //
                // Check for valid entrypoint
                //
                if (LdrEntry->EntryPoint)
                {
                    //
                    // Write in array
                    //
                    ASSERT(i < NumberOfRoutines);
                    LdrRootEntry[NumberOfRoutines] = LdrEntry;

                    //
                    // Show debug message
                    //
                    if ((ShowSnaps) || (LdrpShowInitRoutines))
                    {
                        DbgPrint("[%x,%x] LDR: %wZ init routine %p\n",
                                 NtCurrentTeb()->Cid.UniqueThread,
                                 NtCurrentTeb()->Cid.UniqueProcess,
                                 &LdrEntry->FullDllName,
                                 LdrEntry->EntryPoint);
                    }

                    //
                    // Move to the next routine
                    //
                    NumberOfRoutines++;
                }
            }
        }

        //
        // Set the processed flag and move on to the next entry
        //
        LdrEntry->Flags |= LDRP_ENTRY_PROCESSED;
        NextEntry = NextEntry->Flink;
    }

    //
    // Sanity check
    //
    ASSERT(i == NumberOfRoutines);

    //
    // If we got a context, then we have to call kernel32 for Terminal Services
    //
    if (Context)
    {
        //
        // Check if we have a port-import function
        //
        if (Kernel32ProcessInitPostImportFunction)
        {
            //
            // Call it
            //
            Status = Kernel32ProcessInitPostImportFunction();
            if (!NT_SUCCESS(Status))
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - Failed running kernel32 post-import "
                           "function; status 0x%08lx\n",
                           __FUNCTION__,
                           Status);
                return Status;
            }
        }

        //
        // Clear it
        //
        Kernel32ProcessInitPostImportFunction = NULL;
    }

    //
    // No root entry? return
    //
    if (!LdrRootEntry) return STATUS_SUCCESS;

    //
    // Set the Top Level DLL TEB
    //
    OldTeb = LdrpTopLevelDllBeingLoadedTeb;
    LdrpTopLevelDllBeingLoadedTeb = NtCurrentTeb();

    //
    // Loop each DLL
    //
    NumberOfRoutines = 0;
    while (NumberOfRoutines < i)
    {
        //
        // Get an entry
        //
        LdrEntry = LdrRootEntry[NumberOfRoutines];

        //
        // Check if .NET doesn't own the unmapping
        //
        if (!(LdrEntry->Flags & LDR_COR_OWNS_UNMAP))
        {
            //
            // Verify NX Compatibility
            //
            LdrpCheckNXCompatibility(LdrEntry);
        }

        //
        // Move to next entry
        //
        NumberOfRoutines++;

        //
        // Get its entrypoint
        //
        EntryPoint = LdrEntry->EntryPoint;

        //
        // Are we being debugged?
        //
        BreakOnInit = 0;
        if ((Peb->BeingDebugged) || (Peb->ReadImageFileExecOptions))
        {
            //
            // Check if we should break on load
            //
            Status = LdrQueryImageFileExecutionOptions(&LdrEntry->BaseDllName,
                                                       L"BreakOnDllLoad",
                                                       REG_DWORD,
                                                       &BreakOnInit,
                                                       sizeof(ULONG),
                                                       NULL);
            if (!NT_SUCCESS(Status)) BreakOnInit = 0;
        }

        //
        // Break if asked
        //
        if (BreakOnInit)
        {
            //
            // Check if we should show a message
            //
            if (ShowSnaps)
            {
                //
                // Tell the debugger
                //
                DbgPrint("LDR: %wZ loaded.", &LdrEntry->BaseDllName);
                DbgPrint(" - About to call init routine at %lx\n", EntryPoint);
            }

            //
            // Break in debugger
            //
            DbgBreakPoint();
        }

        //
        // Check if we should show a message
        //
        if ((ShowSnaps) && (EntryPoint))
        {
            //
            // Tell the debugger
            //
            DbgPrint("[%x, %x] LDR: %wZ loaded.",
                     NtCurrentTeb()->Cid.UniqueThread,
                     NtCurrentTeb()->Cid.UniqueProcess,
                     &LdrEntry->BaseDllName);
            DbgPrint(" - Calling init routine at %p\n", EntryPoint);
        }

        //
        // Make sure we have an entrypoint
        //
        if (EntryPoint)
        {
            //
            // Save the old Dll Initializer and write the current one
            //
            OldInitializer = LdrpCurrentDllInitializer;
            LdrpCurrentDllInitializer = LdrEntry;

            //
            // Set up the Activation Context
            //
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format =
                RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
            RtlZeroMemory(&ActCtx, sizeof(ActCtx));

            //
            // Activate it
            //
            RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                   LdrEntry->
                                                   EntryPointActivationContext);

            //
            // Check if it has TLS and is Win32
            //
            if ((LdrEntry->TlsIndex) && (Context))
            {
                //
                // Call TLS
                //
                LdrpCallTlsInitializers(LdrEntry->DllBase, DLL_PROCESS_ATTACH);
            }

            //
            // Check if we should display a message
            //
            if (LdrpShowInitRoutines)
            {
                //
                // Print debug message
                //
                DbgPrint("[%x,%x] LDR: calling init routine %p for "
                         "DLL_PROCESS_ATTACH\n",
                         NtCurrentTeb()->Cid.UniqueThread,
                         NtCurrentTeb()->Cid.UniqueProcess,
                         EntryPoint);
            }

            //
            // Call the init routine
            //
            Result = LdrpCallInitRoutine(EntryPoint,
                                         LdrEntry->DllBase,
                                         DLL_PROCESS_ATTACH,
                                         (PVOID)1);

            //
            // Deactivate the activation context
            //
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);

            //
            // Save the Current DLL Initializer
            //
            LdrpCurrentDllInitializer = OldInitializer;

            //
            // Mark the entry as processed
            //
            LdrEntry->Flags |= LDRP_PROCESS_ATTACH_CALLED;

            //
            // Check if we failed
            //
            if (!Result)
            {
                //
                // Print debug message and fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "[%x,%x] LDR: DLL_PROCESS_ATTACH for dll \"%wZ\" "
                           "(InitRoutine: %p) failed\n",
                           NtCurrentTeb()->Cid.UniqueThread,
                           NtCurrentTeb()->Cid.UniqueProcess,
                           &LdrEntry->BaseDllName,
                           EntryPoint);
                return STATUS_DLL_INIT_FAILED;
            }
        }
    }

    //
    // Loop in order
    //
    ListHead = &Peb->Ldr->InInitializationOrderModuleList;
    NextEntry = NextEntry->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the Data Entrry
        //
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderModuleList);

        //
        // Check if .NET doesn't own the unmapping
        //
        if (!(LdrEntry->Flags & LDR_COR_OWNS_UNMAP))
        {
            //
            // Verify NX Compatibility
            //
            LdrpCheckNXCompatibility(LdrEntry);
        }

        //
        // Move to Next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check for TLS
    //
    if ((LdrpImageHasTls) && (Context))
    {
        //
        // Set up the Activation Context
        //
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format =
            RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
        RtlZeroMemory(&ActCtx, sizeof(ActCtx));

        //
        // Activate it
        //
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->
                                               EntryPointActivationContext);

        //
        // Do TLS callbacks
        //
        LdrpCallTlsInitializers(Peb->ImageBaseAddress, DLL_PROCESS_DETACH);

        //
        // Deactivate the activation context
        //
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    //
    // Restore old TEB
    //
    LdrpTopLevelDllBeingLoadedTeb = OldTeb;

    //
    // Check if the array is in the heap
    //
    if (LdrRootEntry != LocalArray)
    {
        //
        // Free the array
        //
        RtlFreeHeap(RtlGetProcessHeap(), 0, LdrRootEntry);
    }

    //
    // Return to caller
    //
    return Status;
}

/*++
* @name LdrpMungHeapImportsForTagging
*
* The LdrpMungHeapImportsForTagging routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpMungHeapImportsForTagging(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name LdrpMapViewOfDllSection
*
* The LdrpMapViewOfDllSection routine FILLMEIN
*
* @param SectionHandle
*        FILLMEIN
*
* @param ViewBase
*        FILLMEIN
*
* @param ViewSize
*        FILLMEIN
*
* @param ImageName
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpMapViewOfDllSection(IN HANDLE SectionHandle,
                        IN PVOID *ViewBase,
                        IN PSIZE_T ViewSize,
                        IN PWSTR ImageName,
                        IN ULONG Flags)
{
    PVOID Old = NULL;
    PVOID ArbitraryUserPointer;
    NTSTATUS Status;
    PTEB Teb = NtCurrentTeb();
    ULONG Privilege = SE_LOCK_MEMORY_PRIVILEGE;

    //
    // Check if we need to acquire memory lock privilege
    //
    if (Flags & MEM_LARGE_PAGES)
    {
        //
        // Acquire it
        //
        Status = RtlAcquirePrivilege(&Privilege,
                                     1,
                                     0,
                                     &Old);
        if (!NT_SUCCESS(Status)) Flags &= ~MEM_LARGE_PAGES;
    }

    //
    // Stuff the image name in the TIB, for the debugger
    //
    ArbitraryUserPointer = Teb->Tib.ArbitraryUserPointer;
    Teb->Tib.ArbitraryUserPointer = ImageName;

    //
    // Map the DLL
    //
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                ViewBase,
                                0,
                                0,
                                NULL,
                                ViewSize,
                                ViewShare,
                                Flags,
                                PAGE_READWRITE);

    //
    // Restore the data that was there before
    //
    Teb->Tib.ArbitraryUserPointer = ArbitraryUserPointer;

    //
    // Release the privilege if we had acquired it
    //
    if (Flags) RtlReleasePrivilege(Old);
    return Status;
}

/*++
* @name LdrpCodeAuthzCheckDllAllowed
*
* The LdrpCodeAuthzCheckDllAllowed routine FILLMEIN
*
* @param ImageName
*        FILLMEIN
*
* @param ImageHandle
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpCodeAuthzCheckDllAllowed(IN PUNICODE_STRING ImageName,
                             IN HANDLE ImageHandle)
{
    //
    // FIXME: TODO
    //
    return STATUS_SUCCESS;
}

/*++
* @name LdrpCheckCorImage
*
* The LdrpCheckCorImage routine FILLMEIN
*
* @param ComDirectory
*        FILLMEIN
*
* @param ImageName
*        FILLMEIN
*
* @param ViewBase
*        FILLMEIN
*
* @param ComFlag
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpCheckCorImage(IN PIMAGE_COR20_HEADER ComDirectory,
                  IN PUNICODE_STRING ImageName,
                  IN PVOID *ViewBase,
                  IN PBOOLEAN ComFlag)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID OldBase = *ViewBase;

    //
    // Make sure we have a directory at all
    //
    if (!ComDirectory) return Status;

    //
    // Validate the image
    //
    Status = LdrpCorValidateImage(ViewBase, ImageName->Buffer);
    if (!NT_SUCCESS(Status))
    {
        //
        // Failed, return the same image base
        //
        *ViewBase = OldBase;
        return Status;
    }

    //
    // Check if we have a native entrypoint
    //
    if (ComDirectory->Flags & COMIMAGE_FLAGS_ILONLY) *ComFlag = TRUE;

    //
    // Check if the base changed
    //
    if (ViewBase != OldBase)
    {
        //
        // Unmap the section
        //
        NtUnmapViewOfSection(NtCurrentProcess(), OldBase);

        //
        // Verify the new base
        //
        if (!RtlImageNtHeader(*ViewBase)) Status = STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Return the status
    //
    return Status;
}

/*++
* @name LdrpCreateDllSection
*
* The LdrpCreateDllSection routine FILLMEIN
*
* @param FullName
*        FILLMEIN
*
* @param DllHandle
*        FILLMEIN
*
* @param DllCharacteristics
*        FILLMEIN
*
* @param SectionHandle
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpCreateDllSection(IN PUNICODE_STRING FullName,
                     IN HANDLE DllHandle,
                     IN PULONG DllCharacteristics OPTIONAL,
                     OUT PHANDLE SectionHandle)
{
    HANDLE FileHandle;
    NTSTATUS Status, NewStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG_PTR HardErrorParameters[1];
    ULONG Response;
    SECTION_IMAGE_INFORMATION SectionImageInfo;

    //
    // Check if we don't already have a handle
    //
    if (!DllHandle)
    {
        //
        // Create the object attributes
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   FullName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        //
        // Open the DLL
        //
        Status = NtOpenFile(&FileHandle,
                            SYNCHRONIZE | FILE_EXECUTE | FILE_READ_DATA,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            FILE_NON_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(Status))
        {
            //
            // Check if we should snow snaps
            //
            if (ShowSnaps)
            {
                //
                // Display debug message
                //
                DbgPrint("LDR: %s - NtOpenFile failed; status = %x\n",
                         __FUNCTION__,
                         Status);
            }

            //
            // Check if we failed because the file doesn't exist
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                //
                // Convert it into what LDR expects
                //
                NewStatus = STATUS_DLL_NOT_FOUND;

                //
                // Check if we should snow snaps
                //
                if (ShowSnaps)
                {
                    //
                    // Display debug message
                    //
                    DbgPrint("LDR: %s - Turning NtOpenFile's %x into %x\n",
                             __FUNCTION__,
                             Status,
                             NewStatus);
                }

                //
                // Fail
                //
                *SectionHandle = NULL;
                return NewStatus;
            }
        }
    }
    else
    {
        //
        // Use the handle we already have
        //
        FileHandle = DllHandle;
    }

    //
    // Create a section for the DLL
    //
    Status = NtCreateSection(SectionHandle,
                             SECTION_MAP_READ | SECTION_MAP_EXECUTE |
                             SECTION_MAP_WRITE | SECTION_QUERY,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             FileHandle);
    if (!NT_SUCCESS(Status))
    {
        //
        // Forget the handle
        //
        *SectionHandle = NULL;

        //
        // Give the DLL name
        //
        HardErrorParameters[0] = (ULONG_PTR)FullName;

        //
        // Raise the error
        //
        ZwRaiseHardError(STATUS_INVALID_IMAGE_FORMAT,
                         1,
                         1,
                         HardErrorParameters,
                         OptionOk,
                         &Response);

        //
        // Increment the error count
        //
        if (LdrpInLdrInit) LdrpFatalHardErrorCount++;
    }

    //
    // Check for Safer restrictions
    //
    if (!(DllCharacteristics) ||
        (DllCharacteristics &&
        !(*DllCharacteristics & LDR_IGNORE_CODE_AUTHZ_LEVEL)))
    {
        //
        // Query the section
        //
        Status = NtQuerySection(*SectionHandle,
                                SectionImageInformation,
                                &SectionImageInfo,
                                sizeof(SECTION_IMAGE_INFORMATION),
                                NULL);
        if (NT_SUCCESS(Status))
        {
            //
            // Check if it's executable
            //
            if (SectionImageInfo.ImageContainsCode)
            {
                //
                // It is, check WinSafer
                //
                Status = LdrpCodeAuthzCheckDllAllowed(FullName, DllHandle);
                if (!NT_SUCCESS(Status) && (Status != STATUS_NOT_FOUND))
                {
                    //
                    // Show debug message
                    //
                    if (ShowSnaps)
                    {
                        DbgPrint("LDR: Loading of (%wZ) blocked by Winsafer\n",
                                 &FullName);
                    }
                }
                else
                {
                    //
                    // We're fine, return normally
                    //
                    Status = STATUS_SUCCESS;
                    goto Quickie;
                }
            }
        }

        //
        // Failure case, close section handle
        //
        NtClose(*SectionHandle);
        *SectionHandle = NULL;
    }
    else
    {
        //
        // Notify debugger
        //
        DbgPrint("LDR: WinSafer AuthzCheck on %wZ skipped by request\n");
    }

Quickie:
    //
    // Close the file handle, we don't need it
    //
    NtClose(FileHandle);

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpCheckForKnownDll
*
* The LdrpCheckForKnownDll routine FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param FullName
*        FILLMEIN
*
* @param BaseName
*        FILLMEIN
*
* @param SectionHandle
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpCheckForKnownDll(IN PWCHAR DllName,
                     OUT PUNICODE_STRING FullName,
                     OUT PUNICODE_STRING BaseName,
                     OUT PHANDLE SectionHandle)
{
    NTSTATUS Status;
    UNICODE_STRING DllNameString, FullDllName;
    UNICODE_STRING RealName, LocalName;
    HANDLE Section;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN PrivateName;
    ULONG FullLength;

    //
    // Initialize section handle
    //
    if (SectionHandle) *SectionHandle = NULL;

    //
    // Initialize Dll name strings
    //
    if (FullName) RtlInitEmptyUnicodeString(FullName, NULL, 0);
    if (BaseName) RtlInitEmptyUnicodeString(BaseName, NULL, 0);

    //
    // Make sure all our parameters are there
    //
    if (!(FullName) || !(BaseName) || !(SectionHandle))
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    //
    // Sanity check
    //
    LdrpEnsureLoaderLockIsHeld();

    //
    // Initialize the DLL Name 
    //
    RtlInitUnicodeString(&DllNameString, DllName);

    //
    // Check if we should compute the privatized name
    //
    if (((NtCurrentPeb()->ProcessParameters) &&
         (NtCurrentPeb()->ProcessParameters->Flags &
          RTL_USER_PROCESS_PARAMETERS_PRIVATE_DLL_PATH)) &&
          (DllNameString.Length))
    {
        //
        // Initialize the strings
        //
        RtlInitEmptyUnicodeString(&RealName, RealBuffer, sizeof(RealBuffer));
        RtlInitEmptyUnicodeString(&LocalName, LocalBuffer, sizeof(LocalBuffer));

        //
        // Query it
        //
        PrivateName = TRUE;
        Status = RtlComputePrivatizedDllName_U(&DllNameString,
                                               &LocalName,
                                               &RealName);
        if (!NT_SUCCESS(Status)) goto Quickie;

        //
        // Make sure both files exist
        //
        if (!(RtlDoesFileExists_U(RealName.Buffer)) ||
            !(RtlDoesFileExists_U(LocalName.Buffer)))
        {
            //
            // Turn the flag back off
            //
            PrivateName = FALSE;
        }

        //
        // Check if the real name wasn't buffer-allocated
        //
        if (RealName.Buffer != RealBuffer)
        {
            //
            // Free it
            //
            RtlFreeStringRoutine(RealName.Buffer);
        }

        //
        // Check if the local name wasn't buffer-allocated
        //
        if (LocalName.Buffer != LocalBuffer)
        {
            //
            // Free it
            //
            RtlFreeStringRoutine(LocalName.Buffer);
        }

        //
        // Check if we have a private name
        //
        if (PrivateName)
        {
            //
            // Return to caller
            //
            Status = STATUS_SUCCESS;
            goto Quickie;
        }
    }

    //
    // Try to find an activation context section for the DLL
    //
    Status = RtlFindActivationContextSectionString(NULL,
                                                   NULL,
                                                   2,
                                                   &DllNameString,
                                                   NULL);
    if ((Status == STATUS_SXS_KEY_NOT_FOUND) ||
        (Status == STATUS_SXS_SECTION_NOT_FOUND))
    {
        //
        // Check if the known DLL path plus our DLL name are not too large
        //
        FullLength = LdrpKnownDllPath.Length + DllNameString.Length;
        if (FullLength > UNICODE_STRING_MAX_BYTES)
        {
            //
            // They are, fail
            //
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }
    }
    else if (!NT_SUCCESS(Status))
    {
        //
        // There is a section but we've somehow failed to get it
        //
        goto Quickie;
    }
    else
    {
        //
        // We've found a context for this DLL, return now
        //
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    //
    // Allocate a string for the name
    //
    Status = LdrpAllocateUnicodeString(&FullDllName, FullLength);
    if (!NT_SUCCESS(Status)) goto Quickie;

    //
    // Add the known DLL path
    //
    RtlAppendUnicodeStringToString(&FullDllName, &LdrpKnownDllPath);

    //
    // Add a path separator
    //
    RtlAppendUnicodeStringToString(&FullDllName, &RtlNtPathSeparatorString);

    //
    // Add the DLL name
    //
    RtlAppendUnicodeStringToString(&FullDllName, &DllNameString);
    ASSERT(FullDllName.Length == FullLength);

    //
    // Initialize attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &DllNameString,
                               OBJ_CASE_INSENSITIVE,
                               LdrpKnownDllObjectDirectory,
                               NULL);

    //
    // Open the section
    //
    Status = NtOpenSection(&Section,
                           SECTION_MAP_READ |
                           SECTION_MAP_EXECUTE |
                           SECTION_MAP_WRITE,
                           &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        //
        // We failed. Should we normalize the error code?
        //
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) Status = STATUS_SUCCESS;
        goto Quickie;
    }

    //
    // Update profiling data
    //
    LdrpSectionOpens++;

    //
    // Setup the base name
    //
    BaseName->MaximumLength = DllNameString.Length + sizeof(WCHAR);
    BaseName->Length = DllNameString.Length;
    BaseName->Buffer = FullDllName.Buffer +
                       (FullDllName.Length - DllNameString.Length);

    //
    // Setup the full name
    //
    FullName->Length = FullDllName.Length;
    FullName->MaximumLength = FullDllName.MaximumLength;
    FullName->Buffer = FullDllName.Buffer;

    //
    // Return the section
    //
    *SectionHandle = Section;
    return STATUS_SUCCESS;

Quickie:
    //
    // Close the section handle if need be
    //
    if (Section) NtClose(Section);

    //
    // Free the name if it was allocated
    //
    if (FullDllName.Buffer) LdrpFreeUnicodeString(&FullDllName);

    //
    // Return failure code
    //
    return Status;
}

/*++
* @name LdrpMapDll
*
* The LdrpMapDll routine FILLMEIN
*
* @param SearchPath
*        FILLMEIN
*
* @param DllPath2
*        FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param DllCharacteristics
*        FILLMEIN
*
* @param Static
*        FILLMEIN
*
* @param Redirect
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpMapDll(IN PWCHAR SearchPath OPTIONAL,
           IN PWCHAR *DllPath2,
           IN PWCHAR DllName OPTIONAL,
           IN PULONG DllCharacteristics,
           IN BOOLEAN Static,
           IN BOOLEAN Redirect,
           OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    NTSTATUS Status;
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = NtCurrentPeb();
    PWCHAR p1 = DllName;
    WCHAR TempChar;
    BOOLEAN KnownDll = FALSE;
    UNICODE_STRING FullDllName = {0}, BaseDllName = {0};
    HANDLE SectionHandle = NULL, DllHandle;
    UNICODE_STRING NtDllName;
    ULONG_PTR HardErrorParameters[2];
    UNICODE_STRING HardErrorDllName, HardErrorDllPath;
    ULONG HardErrorResponse;
    SIZE_T ViewSize;
    PVOID ViewBase = NULL;
    PVOID BaseAddress = NULL;
    PUNICODE_STRING NullString = NULL;
    PUNICODE_STRING AppCompatData = NULL;
    ULONG MemFlags = 0;
    PIMAGE_NT_HEADERS NtHeader, NtHeader2;
    PLDR_DATA_TABLE_ENTRY Entry = NULL;
    NTSTATUS ErrorStatus;
    ULONG_PTR ImageBase, ImageEnd;
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY CurrentLdrEntry;
    ULONG_PTR CurrentBase, CurrentEnd;
    PUNICODE_STRING OverlapDllName;
    BOOLEAN RelocatableDll = TRUE;
    BOOLEAN IsOverlapping = FALSE;
    PVOID RelocData, ComDirectory;
    ULONG RelocDataSize = 0, ComSectionSize;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    PUNICODE_STRING CheckString;
    ULONG LargePage;
    BOOLEAN ComFlag;

    //
    // Make sure the loader lock is being held
    //
    LdrpEnsureLoaderLockIsHeld();

    //
    // Check if we have an application compatibility callback
    //
    if (LdrpAppCompatDllRedirectionCallbackFunction)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we have a known dll directory
    //
    if ((LdrpKnownDllObjectDirectory) && !(Redirect))
    {
        //
        // Check if the path is full
        //
        while (*p1)
        {
            TempChar = *p1++;
            if (TempChar == '\\' || TempChar == '/')
            {
                //
                // Complete path, don't do Known Dll lookup
                //
                goto SkipCheck;
            }
        }

        //
        // Try to find a Known DLL
        //
        Status = LdrpCheckForKnownDll(DllName,
                                      &FullDllName,
                                      &BaseDllName,
                                      &SectionHandle);
        if (!(NT_SUCCESS(Status)) && (Status != STATUS_DLL_NOT_FOUND))
        {
            //
            // Fail
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - call to LdrpCheckForKnownDll(\"%ws\", ...) "
                       "failed with status %x\n",
                       __FUNCTION__,
                       DllName,
                       Status);
            goto Quickie;
        }
    }

SkipCheck:
    //
    // Check if the Known DLL Check returned something
    //
    if (!SectionHandle)
    {
        //
        // It didn't, so try to resolve the name now
        //
        Status = LdrpResolveDllName(SearchPath,
                                    DllPath2,
                                    DllName,
                                    Redirect,
                                    &FullDllName,
                                    &BaseDllName,
                                    &DllHandle);
        if (NT_SUCCESS(Status))
        {
            //
            // Got a name, display a message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: Loading (%s, %s) %wZ\n",
                         Static ? "STATIC" : "DYNAMIC",
                         Static ? "REDIRECTED" : "NON_REDIRECTED",
                         &FullDllName);
            }

            //
            // Convert to NT Name
            //
            if (!RtlDosPathNameToNtPathName_U(FullDllName.Buffer,
                                              &NtDllName,
                                              NULL,
                                              NULL))
            {
                //
                // Path was invalid
                //
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - call to RtlDosPathNameToNtPathName_U on "
                           "path \"%wZ\" failed; returning status %x\n",
                           __FUNCTION__,
                           &FullDllName,
                           Status);
                goto Quickie;
            }

            //
            // Create a section for this DLL
            //
            Status = LdrpCreateDllSection(&NtDllName,
                                          DllHandle,
                                          DllCharacteristics,
                                          &SectionHandle);
            if (!NT_SUCCESS(Status))
            {
                //
                // Notify debugger
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "LDR: %s - LdrpCreateDllSection (%wZ) failed with "
                           "status %\n",
                           __FUNCTION__,
                           &NtDllName,
                           Status);

                //
                // Free the full name
                //
                LdrpFreeUnicodeString(&FullDllName);

                //
                // Free the NT Name
                //
                RtlFreeHeap(RtlGetProcessHeap(), 0, NtDllName.Buffer);
                goto Quickie;
            }

            //
            // Free the NT Name
            //
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtDllName.Buffer);
            LdrpSectionCreates++;
        }
        else if (Status != STATUS_DLL_NOT_FOUND)
        {
            //
            // We couldn't resolve the name, is this a static load?
            //
            if (Static)
            {
                //
                // This is BAD! Static loads are CRITICAL. Bugcheck!
                // Initialize the strings for the error
                //
                RtlInitUnicodeString(&HardErrorDllName, DllName);
                RtlInitUnicodeString(&HardErrorDllPath,
                                     SearchPath ? SearchPath :
                                                  LdrpDefaultPath.Buffer);

                //
                // Set them as error parameters
                //
                HardErrorParameters[0] = (ULONG_PTR)&HardErrorDllName;
                HardErrorParameters[1] = (ULONG_PTR)&HardErrorDllPath;

                //
                // Raise the hard error
                //
                ZwRaiseHardError(STATUS_DLL_NOT_FOUND,
                                 2,
                                 0x00000003,
                                 HardErrorParameters,
                                 OptionOk,
                                 &HardErrorResponse);

                //
                // We're back, where we initializing?
                //
                if (LdrpInLdrInit) LdrpFatalHardErrorCount++;
            }

            //
            // Fail
            //
            goto Quickie;
        }
        else
        {
            //
            // Notify debugger
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - call to LdrpResolveDllName on dll \"%ws\" "
                       "failed with status %x\n",
                       __FUNCTION__,
                       DllName,
                       Status);
        }
    }
    else
    {
        //
        // We have a section handle, so this is a known dll
        //
        KnownDll = TRUE;
    }

    //
    // Set section mapping data
    //
    ViewSize = 0;
    ViewBase = NULL;

    //
    // Setup profiling data
    //
    LdrpSectionMaps++;
    if (LdrpDisplayLoadTime)
    {
        //
        // Save the current time
        //
        NtQueryPerformanceCounter(&MapBeginTime, NULL);
    }

    //
    // Check if this is kernel32 or user32.
    //
    CheckString = &User32String;
    if (!RtlEqualUnicodeString(&BaseDllName, CheckString, TRUE))
    {
        //
        // It isn't user32... is it kernel 32?
        //
        CheckString = &Kernel32String;
        if (!RtlEqualUnicodeString(&BaseDllName, CheckString, TRUE))
        {
            //
            // Then this isn't a magic DLL
            //
            RelocatableDll = FALSE;
        }
    }

    //
    // Check if we have a Large Page DLLs key and if the two DLLs
    // are either User32 or Kernel32
    //
    if ((LdrpLargePageDLLKey) && (RelocatableDll))
    {
        //
        // Sanity check
        //
        ASSERT(Peb->ImageUsesLargePages);

        //
        // Query the option
        //
        Status = LdrQueryImageFileKeyOption(LdrpLargePageDLLKey,
                                            BaseDllName.Buffer,
                                            REG_DWORD,
                                            &LargePage,
                                            sizeof(ULONG),
                                            NULL);
        if ((NT_SUCCESS(Status)) && (LargePage))
        {
            //
            // Set the proper flag
            //
            MemFlags = MEM_LARGE_PAGES;
        }
    }

    //
    // Map the DLL
    //
    Status = LdrpMapViewOfDllSection(SectionHandle,
                                     &ViewBase,
                                     &ViewSize,
                                     FullDllName.Buffer,
                                     MemFlags);
    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - failed to map view of section; status = %x\n",
                   __FUNCTION__,
                   Status);
        goto Quickie;
    }

    //
    // Get the NT Header
    //
    NtHeader = RtlImageNtHeader(ViewBase);
    if (!NtHeader)
    {
        //
        // Invalid image, unmap
        //
        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "LDR: %s - unable to map ViewBase (%p) to image headers; "
                   "failing with status %x\n",
                   __FUNCTION__,
                   ViewBase,
                   Status);

        //
        // Fail
        //
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto Quickie;
    }

    //
    // Get the .NET section of the executable
    //
    ComDirectory =
        RtlImageDirectoryEntryToData(ViewBase,
                                     IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                     TRUE,
                                     &ComSectionSize);

    //
    // Save our base address
    //
    BaseAddress = ViewBase;

    //
    // Check if this is a .NET executable
    //
    if ((ComDirectory))
    {
        //
        // Validate it
        //
        Status = LdrpCheckCorImage(ComDirectory,
                                   &FullDllName,
                                   &ViewBase,
                                   &ComFlag);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    //
    // Check for profiling
    //
    if (LdrpDisplayLoadTime)
    {
        //
        // Profile the map time
        //
        NtQueryPerformanceCounter(&MapEndTime, NULL);
        MapElapsedTime.QuadPart = MapEndTime.QuadPart - MapBeginTime.QuadPart;

        //
        // Display it
        //
        DbgPrint("Map View of Section Time %ld %ws\n",
                 MapElapsedTime.LowPart,
                 DllName);
    }

    //
    // Allocate an entry
    //
    Entry = LdrpAllocateDataTableEntry(ViewBase);
    if (!Entry)
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_LDR_ID,
                   0,
                   "failed to allocate new data table entry for %p\n"
                   __FUNCTION__,
                   ViewBase);
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    //
    // Setup the entry
    //
    Entry->Flags = Static ? LDRP_STATIC_LINK : 0;
    Entry->LoadCount = 0;
    Entry->FullDllName = FullDllName;
    Entry->BaseDllName = BaseDllName;
    Entry->EntryPoint = LdrpFetchAddressOfEntryPoint(Entry->DllBase);

    //
    // Insert this entry
    //
    LdrpInsertMemoryTableEntry(Entry);

    //
    // Send DLL Notifications
    //
    LdrpSendDllNotifications(Entry,
                             TRUE,
                             (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH));

    //
    // Check for invalid CPU Image
    //
    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
    {
        //
        // Load our header
        //
        NtHeader2 = RtlImageNtHeader(Peb->ImageBaseAddress);

        //
        // Assume defaults if we don't have to run the Hard Error path
        //
        ErrorStatus = STATUS_SUCCESS;
        HardErrorResponse = ResponseCancel;

        //
        // Are we an NT 3.0 image?
        //
        if (NtHeader2->OptionalHeader.MajorSubsystemVersion <= 3)
        {
            //
            // Reset the entrypoint, save our Dll Name
            //
            Entry->EntryPoint = 0;
            HardErrorParameters[0] = (ULONG_PTR)&FullDllName;

            //
            // Raise the error
            //
            ErrorStatus = NtRaiseHardError(STATUS_IMAGE_MACHINE_TYPE_MISMATCH,
                                           1,
                                           1,
                                           HardErrorParameters,
                                           OptionOkCancel,
                                           &HardErrorResponse);
        }

        //
        // Check if the user pressed cancel
        //
        if ((NT_SUCCESS(ErrorStatus)) && (HardErrorResponse == ResponseCancel))
        {
            //
            // Remove the DLL from the lists
            //
            RemoveEntryList(&Entry->InLoadOrderLinks);
            RemoveEntryList(&Entry->InMemoryOrderModuleList);
            RemoveEntryList(&Entry->HashLinks);

            //
            // Remove the LDR Entry
            //
            RtlFreeHeap(LdrpHeap, 0, Entry);

            //
            // Did we do a hard error?
            //
            if (NtHeader2->OptionalHeader.MajorSubsystemVersion <= 3)
            {
                //
                // Yup, so increase fatal error count if we are initializing
                //
                if (LdrpInLdrInit) LdrpFatalHardErrorCount++;
            }

            //
            // Return failure
            //
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }
    }
    else
    {
        //
        // The image was valid. Is it a DLL?
        //
        if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL)
        {
            //
            // Set the DLL Flag
            //
            Entry->Flags |= LDRP_IMAGE_DLL;
        }

        //
        // If we're not a DLL, clear the entrypoint
        //
        if (!(Entry->Flags & LDRP_IMAGE_DLL)) Entry->EntryPoint = 0;
    }

    //
    // Return it for the caller
    //
    *LdrEntry = Entry;

    //
    // Check if we loaded somewhere else
    //
    if (Status == STATUS_IMAGE_NOT_AT_BASE)
    {
        //
        // Write the flag
        //
        Entry->Flags |= LDRP_IMAGE_NOT_AT_BASE;

        //
        // If we're profiling, display a message
        //
        if ((BeginTime.LowPart) || (BeginTime.HighPart))
        {
            DbgPrint("\nLDR: %s Relocating Image Name %ws\n",
                     __FUNCTION__,
                     DllName);
        }
        LdrpSectionRelocates++;

        //
        // Are we dealing with a DLL?
        //
        if (Entry->Flags & LDRP_IMAGE_DLL)
        {
            //
            // Check if relocs were stripped
            //
            if (!(NtHeader->FileHeader.Characteristics &
                  IMAGE_FILE_RELOCS_STRIPPED))
            {
                //
                // Get the relocation data
                //
                RelocData = 
                    RtlImageDirectoryEntryToData(ViewBase,
                                                 TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                                 &RelocDataSize);
                if (!(RelocData) && !(RelocDataSize)) goto NoRelocNeeded;
            }

            //
            // Check if this was a special and known DLL
            //
            if (!RelocatableDll && KnownDll)
            {
                //
                // FIXME: Query information about the memory segment where the
                // DLL has been mapped.
                //
                ImageBase = NtHeader->OptionalHeader.ImageBase;
                Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                              (PVOID)ImageBase,
                                              MemoryBasicInformation,
                                              &MemoryInformation,
                                              sizeof(MemoryInformation),
                                              NULL);
                if (NT_SUCCESS(Status))
                {
                    //
                    // Sanity check
                    //
                    ASSERT(MemoryInformation.State == MEM_FREE);

                    //
                    // Update addresses
                    //
                    ImageBase = (ULONG_PTR)MemoryInformation.AllocationBase;
                    ImageEnd = MemoryInformation.RegionSize +
                               (ULONG_PTR)MemoryInformation.BaseAddress;
                }
                else
                {
                    //
                    // Use an estimate
                    //
                    ImageEnd = ImageBase + ViewSize;
                }

                //
                // Scan all the modules
                //
                IsOverlapping = FALSE;
                ListHead = &PebLdr.InLoadOrderModuleList;
                NextEntry = ListHead->Flink;
                while (NextEntry != ListHead)
                {
                    //
                    // Get the entry and move to the next one
                    //
                    CurrentLdrEntry = CONTAINING_RECORD(NextEntry,
                                                        LDR_DATA_TABLE_ENTRY,
                                                        InLoadOrderLinks);
                    NextEntry = NextEntry->Flink;

                    //
                    // Get the entry's bounds
                    //
                    CurrentBase = (ULONG_PTR)CurrentLdrEntry->DllBase;
                    CurrentEnd = CurrentBase + CurrentLdrEntry->SizeOfImage;

                    //
                    // Make sure this entry isn't unloading
                    //
                    if (!CurrentLdrEntry->InMemoryOrderModuleList.Flink)
                        continue;

                    //
                    // Check if our regions are colliding
                    //
                    if (((CurrentBase >= CurrentBase) &&
                         (ImageBase <= CurrentEnd)) ||
                        ((ImageEnd >= CurrentBase) &&
                         (ImageEnd <= CurrentEnd)) ||
                        ((CurrentBase >= ImageBase) &&
                         (CurrentBase <= ImageEnd)))
                    {
                        //
                        // Found who is overlapping
                        //
                        IsOverlapping = TRUE;
                        OverlapDllName = &CurrentLdrEntry->FullDllName;
                        break;
                    }
                }

                //
                // Check if we didn't actually get a name for the overlapping
                // DLL, implying that it's actually some sort of dynamic memory
                //
                if (!IsOverlapping) OverlapDllName = &DynamicString;

                //
                // Setup for hard error
                //
                HardErrorParameters[0] = (ULONG_PTR)&CheckString;
                HardErrorParameters[1] = (ULONG_PTR)OverlapDllName;

                //
                // Raise the error
                //
                NtRaiseHardError(STATUS_ILLEGAL_DLL_RELOCATION,
                                 2,
                                 3,
                                 HardErrorParameters,
                                 OptionOk,
                                 &HardErrorResponse);

                //
                // If initializing, increase the error count
                //
                if (LdrpInLdrInit) LdrpFatalHardErrorCount++;

                //
                // Don't do relocation
                //
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto FailRelocate;
            }

            //
            // Change the protection to prepare for relocation
            //
            Status = LdrpSetProtection(ViewBase, FALSE);
            if (NT_SUCCESS(Status))
            {
                //
                // Do the relocation
                //
                Status = LdrRelocateImage(ViewBase,
                                          "LDR",
                                          STATUS_SUCCESS,
                                          STATUS_CONFLICTING_ADDRESSES,
                                          STATUS_INVALID_IMAGE_FORMAT);
                if (NT_SUCCESS(Status))
                {
                    //
                    // Map the section
                    //
                    Status = LdrpMapViewOfDllSection(SectionHandle,
                                                     &ViewBase,
                                                     &ViewSize,
                                                     Entry->FullDllName.Buffer,
                                                     0);
                    if ((!NT_SUCCESS(Status)) &&
                        (Status != STATUS_CONFLICTING_ADDRESSES))
                    {
                        //
                        // Fail
                        //
                        DbgPrintEx(DPFLTR_LDR_ID,
                                   0,
                                   "[%x,%x] LDR: Failed to map view of section"
                                   "; ntstatus = %x",
                                   Teb->Cid.UniqueProcess,
                                   Teb->Cid.UniqueThread,
                                   Status);
                        goto Quickie;
                    }

                    //
                    // Reset the protection
                    //
                    Status = LdrpSetProtection(ViewBase, TRUE);
                }
            }

            //
            // Handle any kind of failure
            //
FailRelocate:
            if (!NT_SUCCESS(Status))
            {
                //
                // Remove it from the lists
                //
                RemoveEntryList(&Entry->InLoadOrderLinks);
                RemoveEntryList(&Entry->InMemoryOrderModuleList);
                RemoveEntryList(&Entry->HashLinks);

                //
                // Show debug message
                //
                if (ShowSnaps)
                {
                    DbgPrint("LDR: Fixups unsuccessfully re-applied @ %lx\n",
                             ViewBase);
                }

                //
                // Fail
                //
                goto Quickie;
            }

            //
            // Check if we should show a message
            //
            if (ShowSnaps)
            {
                //
                // Display it
                //
                DbgPrint("LDR: Fixups successfully re-applied @ %lx\n",
                         ViewBase);
            }
        }
        else
        {
NoRelocNeeded:
            //
            // Map the section
            //
            Status = LdrpMapViewOfDllSection(SectionHandle,
                                             &ViewBase,
                                             &ViewSize,
                                             Entry->FullDllName.Buffer,
                                             0);
            if ((!NT_SUCCESS(Status)) &&
                (Status != STATUS_CONFLICTING_ADDRESSES))
            {
                //
                // Fail
                //
                DbgPrintEx(DPFLTR_LDR_ID,
                           0,
                           "[%x,%x] LDR: %s - LdrpMapViewOfDllSection on no "
                           " reloc needed dll failed with status %x\n",
                           Teb->Cid.UniqueProcess,
                           Teb->Cid.UniqueThread,
                           __FUNCTION__,
                           Status);
                goto Quickie;
            }

            //
            // Set success and check if we should display a message
            //
            Status = STATUS_SUCCESS;
            if (ShowSnaps)
            {
                //
                // Dislplay it
                //
                DbgPrint("LDR: Fixups won't be re-applied to non-Dll @ %lx\n",
                         ViewBase);
            }
        }
    }

    //
    // Check if we had a .NET section
    //
    if (ComDirectory)
    {
        //
        // Check the image for validity
        //
        Status = LdrpCheckCorImage(ComDirectory,
                                   &Entry->FullDllName,
                                   &ViewBase,
                                   &ComFlag);
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Check if this is a .NET image
    //
    if (ComFlag) Entry->Flags |= LDRP_COR_IMAGE;

    //
    // Check if .NET relocated the image
    //
    if (ViewBase != BaseAddress) Entry->Flags |= LDR_COR_OWNS_UNMAP;

    //
    // Check if this is an SMP Machine and a DLL
    //
    if ((LdrpNumberOfProcessors > 1) &&
        (Entry && (Entry->Flags & LDRP_IMAGE_DLL)))
    {
        //
        // Validate the image for MP
        //
        LdrpValidateImageForMp(Entry);
    }

    //
    // Clean up the view base for the cleanup code
    //
    ViewBase = NULL;

Quickie:

    //
    // Check if the image was loaded
    //
    if (ViewBase)
    {
        //
        // If it's .NET, unload it
        //
        if (ComFlag) LdrpCorUnloadImage(ViewBase);

        //
        // Check if it wasn't relocated by .NET
        //
        if (ViewBase == BaseAddress)
        {
            //
            // Unmap it
            //
            NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
        }
    }

    //
    // Check if we have a section handle and close it
    //
    if (SectionHandle) NtClose(SectionHandle);

    //
    // Check if we had application compatibility data
    //
    if (AppCompatData) RtlFreeStringRoutine(AppCompatData);

    //
    // Check if we had an allocated full name
    //
    if (FullDllName.Buffer) LdrpFreeUnicodeString(&FullDllName);

    //
    // Check if we got here due to success or failure
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Check if we should display the message
        //
        if ((ShowSnaps) || (Status = STATUS_DLL_NOT_FOUND))
        {
            //
            // Notify debugger
            //
            DbgPrint("LDR: %s(%ws) failing 0x%lx\n",
                     __FUNCTION__,
                     DllName,
                     Status);
        }
    }

    //
    // Return
    //
    return Status;
}

/*++
* @name LdrpLoadImportModule
*
* The LdrpLoadImportModule routine FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param ImportName
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param AlreadyLoaded
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpLoadImportModule(IN PWSTR DllPath OPTIONAL,
                     IN LPSTR ImportName,
                     OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                     OUT PBOOLEAN AlreadyLoaded)
{
    ANSI_STRING AnsiNameString;
    PUNICODE_STRING ImportNameString = &NtCurrentTeb()->StaticUnicodeString;
    NTSTATUS Status;
    WCHAR RedirBuffer[MAX_PATH];
    UNICODE_STRING RedirString, RedirString2;
    BOOLEAN RedirectedDll = FALSE;
    PWCHAR DllPath2;

    //
    // Initialize an ANSI string for the name
    //
    RedirString2.Buffer = NULL;
    RtlInitAnsiString(&AnsiNameString, ImportName);

    //
    // Convert it to a unicode string, stored in the TEB
    //
    Status = RtlAnsiStringToUnicodeString(ImportNameString, &AnsiNameString, FALSE);
    if (!NT_SUCCESS(Status)) goto Quickie;

    //
    // Check if the name is missing the DLL extension
    //
    if (!strchr(ImportName, '.'))
    {
        //
        // Add it ourselves
        //
        RtlAppendUnicodeToString(ImportNameString, L".dll");
    }

    //
    // Initialize the redirected name strings
    //
    RtlInitUnicodeString(&RedirString2, NULL);
    RtlInitEmptyUnicodeString(&RedirString, RedirBuffer, sizeof(RedirBuffer));

    //
    // Get the redirected name
    //
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      ImportNameString,
                                                      &DefaultExtension,
                                                      &RedirString,
                                                      &RedirString2,
                                                      &ImportNameString,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        //
        // Check if we failed for another reason then the fact that this
        // DLL might not have redirection information
        //
        if (Status != STATUS_SXS_KEY_NOT_FOUND)
        {
            //
            // Check if we should output
            //
            if (ShowSnaps)
            {
                //
                // Show a debug message
                //
                DbgPrint("LDR: %s - RtlDosApplyFileIsolationRedirection_Ustr "
                         "failed with status %x\n",
                        __FUNCTION__,
                        Status);
            }

            //
            // Fail
            //
            goto Quickie;
        }
    }
    else
    {
        //
        // Success, so remember that this is a directed DLL
        //
        RedirectedDll = TRUE;
    }

    //
    // Check if we have already been loaded
    //
    if (LdrpCheckForLoadedDll(DllPath,
                              ImportNameString,
                              TRUE,
                              RedirectedDll,
                              LdrEntry))
    {
        //
        // Remember that we already loaded
        //
        *AlreadyLoaded = TRUE;
    }
    else
    {
        //
        // Map us now
        //
        DllPath2 = DllPath;
        Status = LdrpMapDll(DllPath,
                            &DllPath2,
                            ImportNameString->Buffer,
                            NULL,
                            TRUE,
                            RedirectedDll,
                            LdrEntry);

        //
        // Remember that we weren't already loaded
        //
        *AlreadyLoaded = FALSE;

        //
        // Check if we were able to map it
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // Check if we should output
            //
            if (ShowSnaps)
            {
                //
                // Show a debug message
                //
                DbgPrint("LDR: %s - LdrpMapDll(%p, %ls, NULL, TRUE, %d, %p) "
                         "failed with status %x\n",
                        __FUNCTION__,
                        DllPath,
                        ImportNameString->Buffer,
                        RedirectedDll,
                        LdrEntry,
                        Status);
            }

            //
            // Fail
            //
            goto Quickie;
        }

        //
        // Mark the DLL
        //
        RtlpStkMarkDllRange(*LdrEntry);

        //
        // Walk the imports
        //
        Status = LdrpWalkImportDescriptor(DllPath, *LdrEntry);
        if (!NT_SUCCESS(Status))
        {
            //
            // Check if we should output
            //
            if (ShowSnaps)
            {
                //
                // Show a debug message
                //
                DbgPrint("LDR: %s - LdrpWalkImportDescriptor [dll %ls] failed "
                         "with status %x\n",
                        __FUNCTION__,
                        ImportNameString->Buffer,
                        Status);
            }

            //
            // Insert it into the list
            //
            InsertTailList(&PebLdr.InInitializationOrderModuleList,
                           &(*LdrEntry)->InInitializationOrderModuleList);
        }
    }

Quickie:
    //
    // Check if we had a redirected string, and free it, and return status
    //
    if (RedirString2.Buffer) RtlFreeUnicodeString(&RedirString2);
    return Status;
}

/*++
* @name LdrpNameToOrdinal
*
* The LdrpNameToOrdinal routine FILLMEIN
*
* @param ImportName
*        FILLMEIN
*
* @param NumberOfNames
*        FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param NameTable
*        FILLMEIN
*
* @param OrdinalTable
*        FILLMEIN
*
* @return USHORT
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
USHORT
LdrpNameToOrdinal(IN PCHAR ImportName,
                  IN ULONG NumberOfNames,
                  IN PVOID BaseAddress,
                  IN PULONG NameTable,
                  IN PUSHORT OrdinalTable)
{
    LONG High, Low = 0, Mid, Result;

    //
    // Do a lookup... get the highest possible number and loop
    //
    High = NumberOfNames - 1;
    while (High >= Low)
    {
        //
        // Now get the middle entry and compare
        //
        Mid = (Low + High) >> 1;
        Result = strcmp(ImportName, (PCHAR)((ULONG_PTR)BaseAddress +
                                            NameTable[Mid]));

        //
        // Check the comparison result
        //
        if (Result < 0)
        {
            //
            // The result is below the current middle point, so set it
            // as the new maximum.
            //
            High = Mid - 1;
        }
        else if (Result > 0)
        {
            //
            // The result is above the current middle point, so set it
            // as the new minimum.
            //
            Low = Mid + 1;
        }
        else
        {
            //
            // Otherwise, we found it!
            //
            break;
        }
    }

    //
    // If we got here and high is still bigger then low, then it
    // means that we haven't located a match.
    //
    if (High < Low)
    {
        //
        // So fail the lookup
        //
        return -1;
    }
    else
    {
        //
        // Otherwise, use the ordinal we found
        //
        return OrdinalTable[Mid];
    }
}

/*++
* @name LdrpSnapThunk
*
* The LdrpSnapThunk routine FILLMEIN
*
* @param ExportBase
*        FILLMEIN
*
* @param ImportBase
*        FILLMEIN
*
* @param OriginalThunk
*        FILLMEIN
*
* @param Thunk
*        FILLMEIN
*
* @param ExportEntry
*        FILLMEIN
*
* @param ExportSize
*        FILLMEIN
*
* @param Static
*        FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpSnapThunk(IN PVOID ExportBase,
              IN PVOID ImportBase,
              IN PIMAGE_THUNK_DATA OriginalThunk,
              IN OUT PIMAGE_THUNK_DATA Thunk,
              IN PIMAGE_EXPORT_DIRECTORY ExportEntry,
              IN ULONG ExportSize,
              IN BOOLEAN Static,
              IN LPSTR DllName)
{
    BOOLEAN IsOrdinal;
    USHORT Ordinal;
    ULONG OriginalOrdinal;
    PIMAGE_IMPORT_BY_NAME AddressOfData;
    PULONG NameTable;
    PUSHORT OrdinalTable;
    LPSTR ImportName;
    USHORT Hint;
    NTSTATUS Status;
    ULONG_PTR HardErrorParameters[3];
    UNICODE_STRING HardErrorDllName, HardErrorEntryPointName;
    ANSI_STRING TempString;
    ULONG Mask;
    ULONG Response;
    PULONG AddressOfFunctions;
    UNICODE_STRING TempUString;
    ANSI_STRING ForwarderName;
    PVOID ForwarderHandle;
    PUNICODE_STRING ForwardName;
    ULONG ForwardOrdinal;

    //
    // Check if the snap is by ordinal
    //
    IsOrdinal = IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal);
    if (IsOrdinal)
    {
        //
        // Get the ordinal number, and its normalized version
        //
        OriginalOrdinal = IMAGE_ORDINAL(OriginalThunk->u1.Ordinal);
        Ordinal = (USHORT)(OriginalOrdinal - ExportEntry->Base);
    }
    else
    {
        //
        // First get the data VA
        //
        AddressOfData = (PIMAGE_IMPORT_BY_NAME)
                        ((ULONG_PTR)ImportBase +
                        ((ULONG_PTR)OriginalThunk->u1.AddressOfData & -1));

        //
        // Get the name
        //
        ImportName = (LPSTR)AddressOfData->Name;

        //
        // Now get the VA of the Name and Ordinal Tables
        //
        NameTable = (PULONG)((ULONG_PTR)ImportBase +
                             (ULONG_PTR)ExportEntry->AddressOfNames);
        OrdinalTable = (PUSHORT)((ULONG_PTR)ImportBase +
                                 (ULONG_PTR)ExportEntry->
                                 AddressOfNameOrdinals);

        //
        // Get the hint
        //
        Hint = AddressOfData->Hint;

        //
        // Try to get a match by using the hint
        //
        if (((ULONG)Hint < ExportEntry->NumberOfNames) &&
             (!strcmp(ImportName,
                      (PCHAR)((ULONG_PTR)ImportBase + NameTable[Hint]))))
        {
            //
            // We got a match, get the Ordinal from the hint
            //
            Ordinal = OrdinalTable[Hint];
        }
        else
        {
            //
            // Well bummer, hint didn't work, do it the long way
            //
            Ordinal = LdrpNameToOrdinal(ImportName,
                                        ExportEntry->NumberOfNames,
                                        ExportBase,
                                        NameTable,
                                        OrdinalTable);
        }
    }

    //
    // Check if the ordinal is invalid
    //
    if ((ULONG)Ordinal >= ExportEntry->NumberOfFunctions)
    {
FailurePath:
        //
        // Is this a static snap?
        //
        if (Static)
        {
            //
            // These are critical errors. Setup a string for the DLL Name
            //
            RtlInitAnsiString(&TempString, DllName ? DllName : "Unknown");
            RtlAnsiStringToUnicodeString(&HardErrorDllName, &TempString, TRUE);

            //
            // Set it as the parameter
            //
            HardErrorParameters[1] = (ULONG_PTR)&HardErrorDllName;
            Mask = 2;

            //
            // Check if we have an ordinal
            //
            if (IsOrdinal)
            {
                //
                // Then set the ordinal as the 1st parameter
                //
                HardErrorParameters[0] = OriginalOrdinal;
            }
            else
            {
                //
                // We don't, use the entrypoint. Set up a string for it
                //
                RtlInitAnsiString(&TempString, ImportName);
                RtlAnsiStringToUnicodeString(&HardErrorEntryPointName,
                                             &TempString,
                                             TRUE);

                //
                // Set it as the parameter
                //
                HardErrorParameters[0] = (ULONG_PTR)&HardErrorEntryPointName;
                Mask = 3;
            }

            //
            // Raise the error
            //
            NtRaiseHardError(IsOrdinal ? STATUS_ORDINAL_NOT_FOUND :
                                         STATUS_ENTRYPOINT_NOT_FOUND,
                             2,
                             Mask,
                             HardErrorParameters,
                             OptionOk,
                             &Response);

            //
            // Increase the error count
            //
            if (LdrpInLdrInit) LdrpFatalHardErrorCount++;

            //
            // Free our string
            //
            RtlFreeUnicodeString(&HardErrorDllName);
            if (!IsOrdinal)
            {
                //
                // Free our second string. Return entrypoint error
                //
                RtlFreeUnicodeString(&HardErrorEntryPointName);
                RtlRaiseStatus(STATUS_ENTRYPOINT_NOT_FOUND);
            }

            //
            // Return ordinal error
            //
            RtlRaiseStatus(STATUS_ORDINAL_NOT_FOUND);
        }

        //
        // Set this as a bad DLL
        //
        Thunk->u1.Function = (ULONG_PTR)0xFFBADD11;

        //
        // Return the right error code
        //
        Status = IsOrdinal ? STATUS_ORDINAL_NOT_FOUND :
                             STATUS_ENTRYPOINT_NOT_FOUND;
    }
    else
    {
        //
        // The ordinal seems correct, get the AddressOfFunctions VA
        //
        AddressOfFunctions = (PULONG)
                             ((ULONG_PTR)ExportBase +
                              (ULONG_PTR)ExportEntry->AddressOfFunctions);

        //
        // Write the function pointer
        //
        Thunk->u1.Function = (ULONG)((ULONG_PTR)ExportBase +
                                     AddressOfFunctions[Ordinal]);

        //
        // Make sure it's within the exports
        //
        if ((Thunk->u1.Function > (ULONG_PTR)ExportEntry) &&
            (Thunk->u1.Function < ((ULONG_PTR)ExportEntry + ExportSize)))
        {
            //
            // Get the Import and Forwarder Names
            //
            ImportName = (LPSTR)UlongToPtr(Thunk->u1.Function);
            ForwarderName.Buffer = ImportName,
            ForwarderName.Length = (USHORT)(strchr(ImportName, '.') -
                                            ImportName);
            ForwarderName.MaximumLength = ForwarderName.Length;
            Status = RtlAnsiStringToUnicodeString(&TempUString,
                                                  &ForwarderName,
                                                  TRUE);

            //
            // Make sure the conversion was OK
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Load the forwarder, free the temporary string
                //
                Status = LdrpLoadDll(0,
                                     NULL,
                                     NULL,
                                     &TempUString,
                                     &ForwarderHandle,
                                     FALSE);
                RtlFreeUnicodeString(&TempUString);
            }

            //
            // If the load or conversion failed, use the failure path
            //
            if (!NT_SUCCESS(Status)) goto FailurePath;

            //
            // Now set up a name for the actual forwarder dll
            //
            RtlInitAnsiString(&ForwarderName,
                              ImportName +
                              ForwarderName.Length +
                              sizeof(CHAR));

            //
            // Check if it's an ordinal forward
            //
            if ((ForwarderName.Length > 1) && (*ForwarderName.Buffer == '#'))
            {
                //
                // We don't have an actual function name
                //
                ForwardName = NULL;

                //
                // Convert the string into an ordinal
                //
                Status = RtlCharToInteger(ForwarderName.Buffer + sizeof(CHAR),
                                          0,
                                          &ForwardOrdinal);

                //
                // If this fails, then error out
                //
                if (!NT_SUCCESS(Status)) goto FailurePath;
            }
            else
            {
                //
                // Import by name
                //
                ForwardName = (PUNICODE_STRING)&ForwarderName;
            }

            //
            // Get the pointer
            //
            Status = LdrpGetProcedureAddress(ForwarderHandle,
                                             (PANSI_STRING)ForwardName,
                                             ForwardOrdinal,
                                             &UlongToPtr(Thunk->u1.Function),
                                             FALSE);
            //
            // If this fails, then error out
            //
            if (!NT_SUCCESS(Status)) goto FailurePath;
        }
        else
        {
            //
            // It's not within the exports, let's hope it's valid
            //
            if (!AddressOfFunctions[Ordinal]) goto FailurePath;
        }

        //
        // If we got here, then it's success
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpSnapIAT
*
* The LdrpSnapIAT routine FILLMEIN
*
* @param ExportLdrEntry
*        FILLMEIN
*
* @param ImportLdrEntry
*        FILLMEIN
*
* @param IatEntry
*        FILLMEIN
*
* @param EntriesValid
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpSnapIAT(IN PLDR_DATA_TABLE_ENTRY ExportLdrEntry,
            IN PLDR_DATA_TABLE_ENTRY ImportLdrEntry,
            IN PIMAGE_IMPORT_DESCRIPTOR IatEntry,
            IN BOOLEAN EntriesValid)
{
    PIMAGE_EXPORT_DIRECTORY ExportEntry;
    ULONG ExportSize;
    PVOID Iat;
    SIZE_T ImportSize;
    ULONG IatSize;
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status;
    PIMAGE_THUNK_DATA Thunk, OriginalThunk;
    LPSTR ImportName;
    ULONG ForwarderChain;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG i, Rva;
    ULONG OldProtect;

    //
    // Get export directory
    //
    ExportEntry = RtlImageDirectoryEntryToData(ExportLdrEntry->DllBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_EXPORT,
                                               &ExportSize);
    if (!ExportEntry)
    {
        //
        // Fail
        //
        DbgPrint("LDR: %wZ doesn't contain an EXPORT table\n",
                 &ExportLdrEntry->BaseDllName);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Get the IAT
    //
    Iat = RtlImageDirectoryEntryToData(ImportLdrEntry->DllBase,
                                       TRUE,
                                       IMAGE_DIRECTORY_ENTRY_IAT,
                                       &IatSize);
    ImportSize = IatSize;

    //
    // Check if we don't have one
    //
    if (!Iat)
    {
        //
        // Get the NT Header and the first section
        //
        NtHeader = RtlImageNtHeader(ImportLdrEntry->DllBase);
        if (!NtHeader) return STATUS_INVALID_IMAGE_FORMAT;
        SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

        //
        // Get the RVA of the import directory
        //
        Rva = NtHeader->OptionalHeader.
              DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
        if (Rva)
        {
            //
            // Loop all the sections
            //
            for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
            {
                //
                // Check if we are inside this section
                //
                if ((Rva >= SectionHeader->VirtualAddress) &&
                    (Rva < (SectionHeader->VirtualAddress +
                     SectionHeader->SizeOfRawData)))
                {
                    //
                    // We are, so set the IAT here
                    //
                    Iat = (PVOID)((ULONG_PTR)(ImportLdrEntry->DllBase) +
                                  SectionHeader->VirtualAddress);

                    //
                    // Set the size
                    //
                    IatSize = SectionHeader->Misc.VirtualSize;

                    //
                    // Deal with Watcom and other retarded compilers
                    //
                    if (!IatSize) IatSize = SectionHeader->SizeOfRawData;

                    //
                    // Found it, get out
                    //
                    break;
                }

                //
                // No match, move to the next section
                //
                SectionHeader++;
            }
        }

        //
        // If we still don't have an IAT, that's bad
        //
        if (!Iat)
        {
            //
            // Fail
            //
            DbgPrint("LDR: Unable to unprotect IAT for %wZ (Image Base %p)\n",
                     &ImportLdrEntry->BaseDllName,
                     ImportLdrEntry->DllBase);
            return STATUS_INVALID_IMAGE_FORMAT;
        }

        //
        // Set the right size
        //
        ImportSize = IatSize;
    }

    //
    // Unprotect the IAT
    //
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &Iat,
                                    &ImportSize,
                                    PAGE_READWRITE,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrint("LDR: Unable to unprotect IAT for %wZ\n",
                 &ImportLdrEntry->BaseDllName);
        return Status;
    }

    //
    // Check if the Thunks are already valid
    //
    if (EntriesValid)
    {
        //
        // We'll only do forwarders. Get the import name
        //
        ImportName = (LPSTR)((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->Name);

        //
        // Get the list of forwaders
        //
        ForwarderChain = IatEntry->ForwarderChain;

        //
        // Loop them
        //
        while (ForwarderChain != -1)
        {
            //
            // Get the cached thunk VA
            //
            OriginalThunk = (PIMAGE_THUNK_DATA)
                            ((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->OriginalFirstThunk +
                             (ForwarderChain * sizeof(IMAGE_THUNK_DATA)));

            //
            // Get the first thunk
            //
            Thunk = (PIMAGE_THUNK_DATA)
                    ((ULONG_PTR)ImportLdrEntry->DllBase +
                     IatEntry->FirstThunk +
                    (ForwarderChain * sizeof(IMAGE_THUNK_DATA)));

            //
            // Get the Forwarder from the thunk
            //
            ForwarderChain = (ULONG)Thunk->u1.Ordinal;

            //
            // Snap the thunk
            //
            Status = LdrpSnapThunk(ExportLdrEntry->DllBase,
                                   ImportLdrEntry->DllBase,
                                   OriginalThunk,
                                   Thunk,
                                   ExportEntry,
                                   ExportSize,
                                   TRUE,
                                   ImportName);

            //
            // Move to the next thunk
            //
            Thunk++;

            //
            // If we messed up, exit
            //
            if (!NT_SUCCESS(Status)) break;
        }
    }
    else if (IatEntry->FirstThunk)
    {
        //
        // Full snapping. Get the First thunk
        //
        Thunk = (PIMAGE_THUNK_DATA)
                ((ULONG_PTR)ImportLdrEntry->DllBase +
                 IatEntry->FirstThunk);

        //
        // Get the NT Header
        //
        NtHeader = RtlImageNtHeader(ImportLdrEntry->DllBase);

        //
        // Get the Original thunk VA, watch out for weird images
        //
        if ((IatEntry->Characteristics <
             NtHeader->OptionalHeader.SizeOfHeaders) ||
            (IatEntry->Characteristics >=
             NtHeader->OptionalHeader.SizeOfImage))
        {
            //
            // Reuse it, this is a strange linked file
            //
            OriginalThunk = Thunk;
        }
        else
        {
            //
            // Get the address from the field and convert to VA
            //
            OriginalThunk = (PIMAGE_THUNK_DATA)
                            ((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->OriginalFirstThunk);
        }

        //
        // Get the Import name VA
        //
        ImportName = (LPSTR)((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->Name);

        //
        // Loop while it's valid
        //
        while (OriginalThunk->u1.AddressOfData)
        {
            //
            // Snap the Thunk
            //
            Status = LdrpSnapThunk(ExportLdrEntry->DllBase,
                                   ImportLdrEntry->DllBase,
                                   OriginalThunk,
                                   Thunk,
                                   ExportEntry,
                                   ExportSize,
                                   TRUE,
                                   ImportName);

            //
            // Next thunks
            //
            OriginalThunk++;
            Thunk++;

            //
            // If we failed the snap, break out
            //
            if (!NT_SUCCESS(Status)) break;
        }
    }

    //
    // Protect the IAT again
    //
    NtProtectVirtualMemory(NtCurrentProcess(),
                           &Iat,
                           &ImportSize,
                           OldProtect,
                           &OldProtect);

    //
    // Also flush out the cache
    //
    NtFlushInstructionCache(NtCurrentProcess(), Iat, IatSize);

    //
    // Return to caller
    //
    return Status;
}

/*++
* @name LdrpHandleOneOldFormatImportDescriptor
*
* The LdrpHandleOneOldFormatImportDescriptor routine FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param Entry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpHandleOneOldFormatImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                                       IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                       IN PIMAGE_IMPORT_DESCRIPTOR *Entry)
{
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry = *Entry;
    LPSTR ImportName;
    NTSTATUS Status;
    BOOLEAN AlreadyLoaded = FALSE, StaticEntriesValid = FALSE, SkipSnap = TRUE;
    PLDR_DATA_TABLE_ENTRY DllLdrEntry;
    PIMAGE_THUNK_DATA FirstThunk;

    //
    // Get the import name's VA
    //
    ImportName = (LPSTR)((ULONG_PTR)LdrEntry->DllBase + ImportEntry->Name);

    //
    // Get the first thunk
    //
    FirstThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrEntry->DllBase +
                                     ImportEntry->FirstThunk);

    //
    // Make sure it's valid
    //
    if (!FirstThunk->u1.Function ) goto SkipEntry;

    //
    // Show debug message
    //
    if (ShowSnaps)
    {
        DbgPrint("LDR: %s used by %wZ\n",
                 ImportName,
                 &LdrEntry->BaseDllName);
    }

    //
    // Load the module associated to it
    //
    Status = LdrpLoadImportModule(DllPath,
                                  ImportName,
                                  &DllLdrEntry,
                                  &AlreadyLoaded);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        if (ShowSnaps)
        {
            DbgPrint("LDR: LdrpWalkImportTable - LdrpLoadImportModule failed "
                     "on import %s with status %x\n",
                     ImportName,
                     Status);
        }

        //
        // Return
        //
        *Entry = ImportEntry;
        return Status;
    }

    //
    // Show debug message
    //
    if (ShowSnaps)
    {
        DbgPrint("LDR: Snapping imports for %wZ from %s\n",
                 &LdrEntry->BaseDllName,
                 ImportName);
    }

    //
    // Check if the image was bound when compiled
    //
    if (ImportEntry->OriginalFirstThunk)
    {
        //
        // It was, so check if the static IAT entries are still valid
        //
        if ((ImportEntry->TimeDateStamp) &&
            (ImportEntry->TimeDateStamp == DllLdrEntry->TimeDateStamp) &&
            (!DllLdrEntry->Flags & LDRP_IMAGE_NOT_AT_BASE))
        {
            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: Snap bypass %s from %wZ\n",
                         ImportName,
                         &LdrEntry->BaseDllName);
            }

            //
            // They are still valid, so we can skip snapping them.
            // Additionally, if we have no forwarders, we are totally
            // done.
            //
            if (ImportEntry->ForwarderChain == -1)
            {
                //
                // Totally skip LdrpSnapIAT
                //
                SkipSnap = TRUE;
            }
            else
            {
                //
                // Set this so LdrpSnapIAT will only do forwarders
                //
                StaticEntriesValid = TRUE;
            }
        }
    }

    //
    // Check if it wasn't already loaded
    //
    LdrpNormalSnap++;
    if (!AlreadyLoaded)
    {
        //
        // Add the DLL to our list
        //
        InsertTailList(&PebLdr.InInitializationOrderModuleList,
                       &DllLdrEntry->InInitializationOrderModuleList);
    }

    //
    // Check if we should snap at all
    //
    if (!SkipSnap)
    {
        //
        // Now snap the IAT Entry
        //
        Status = LdrpSnapIAT(DllLdrEntry,
                             LdrEntry,
                             ImportEntry,
                             StaticEntriesValid);
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: LdrpWalkImportTable - LdrpSnapIAT #2 failed with "
                         "status %x\n",
                         Status);
            }

            //
            // Return
            //
            *Entry = ImportEntry;
            return Status;
        }
    }

SkipEntry:
    //
    // Move to the next entry
    //
    ImportEntry++;
    *Entry = ImportEntry;
    return Status;
}

/*++
* @name LdrpHandleOneNewFormatImportDescriptor
*
* The LdrpHandleOneNewFormatImportDescriptor routine FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param Entry
*        FILLMEIN
*
* @param FirstEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpHandleOneNewFormatImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                                       IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                       IN PIMAGE_BOUND_IMPORT_DESCRIPTOR *Entry,
                                       IN PIMAGE_BOUND_IMPORT_DESCRIPTOR FirstEntry)
{
    LPSTR ImportName, BoundImportName, ForwarderName;
    NTSTATUS Status;
    BOOLEAN AlreadyLoaded = FALSE, Stale;
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry;
    PLDR_DATA_TABLE_ENTRY DllLdrEntry, ForwarderLdrEntry;
    PIMAGE_BOUND_FORWARDER_REF ForwarderEntry;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry = *Entry;
    ULONG IatSize, i;

    //
    // Get the name's VA
    //
    BoundImportName = (LPSTR)((ULONG_PTR)BoundEntry +
                              BoundEntry->OffsetModuleName);

    //
    // Show debug mesage
    //
    if (ShowSnaps)
    {
        DbgPrint("LDR: %wZ bound to %s\n",
                 &LdrEntry->BaseDllName,
                 BoundImportName);
    }

    //
    // Load the module for this entry
    //
    Status = LdrpLoadImportModule(DllPath,
                                  BoundImportName,
                                  &DllLdrEntry,
                                  &AlreadyLoaded);
    if (!NT_SUCCESS(Status))
    {
        //
        // Show debug message
        //
        if (ShowSnaps)
        {
            DbgPrint("LDR: %wZ failed to load import module %s; status = %x\n",
                     &LdrEntry->BaseDllName,
                     BoundImportName,
                     Status);
        }

        //
        // Fail
        //
        goto Quickie;
    }

    //
    // Check if it wasn't already loaded
    //
    if (!AlreadyLoaded)
    {
        //
        // Add it to our list
        //
        InsertTailList(&PebLdr.InInitializationOrderModuleList,
                       &DllLdrEntry->InInitializationOrderModuleList);
    }

    //
    // Check if the Bound Entry is now invalid
    //
    if ((BoundEntry->TimeDateStamp != DllLdrEntry->TimeDateStamp) ||
        (DllLdrEntry->Flags & LDRP_IMAGE_NOT_AT_BASE))
    {
        //
        // Show debug message
        //
        if (ShowSnaps)
        {
            DbgPrint("LDR: %wZ has stale binding to %s\n",
                     &DllLdrEntry->BaseDllName,
                     BoundImportName);
        }

        //
        // Remember it's become stale
        //
        Stale = TRUE;
    }
    else
    {
        //
        // Show debug message
        //
        if (ShowSnaps)
        {
            DbgPrint("LDR: %wZ has correct binding to %s\n",
                     &DllLdrEntry->BaseDllName,
                     BoundImportName);
        }

        //
        // Remember it's valid
        //
        Stale = FALSE;
    }

    //
    // Get the forwarders
    //
    ForwarderEntry = (PIMAGE_BOUND_FORWARDER_REF)(BoundEntry + 1);

    //
    // Loop them
    //
    for (i = 0; i < BoundEntry->NumberOfModuleForwarderRefs; i++)
    {
        //
        // Get the name
        //
        ForwarderName = (LPSTR)((ULONG_PTR)FirstEntry +
                                ForwarderEntry->OffsetModuleName);

        //
        // Show debug message
        //
        if (ShowSnaps)
        {
            DbgPrint("LDR: %wZ bound to %s via forwarder(s) from %wZ\n",
                     &LdrEntry->BaseDllName,
                     ForwarderName,
                     &DllLdrEntry->BaseDllName);
        }

        //
        // Load the module
        //
        Status = LdrpLoadImportModule(DllPath,
                                      ForwarderName,
                                      &ForwarderLdrEntry,
                                      &AlreadyLoaded);
        if (NT_SUCCESS(Status))
        {
            //
            // Loaded it, was it already loaded?
            //
            if (!AlreadyLoaded)
            {
                //
                // Add it to our list
                //
                InsertTailList(&PebLdr.InInitializationOrderModuleList,
                               &ForwarderLdrEntry->
                               InInitializationOrderModuleList);
            }
        }

        //
        // Check if the Bound Entry is now invalid
        //
        if (!(NT_SUCCESS(Status)) ||
            (ForwarderEntry->TimeDateStamp !=
             ForwarderLdrEntry->TimeDateStamp) ||
            (ForwarderLdrEntry->Flags & LDRP_IMAGE_NOT_AT_BASE))
        {
            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: %wZ has stale binding to %s\n",
                         &ForwarderLdrEntry->BaseDllName,
                         ForwarderName);
            }

            //
            // Remember it's become stale
            //
            Stale = TRUE;
        }
        else
        {
            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: %wZ has correct binding to %s\n",
                         &ForwarderLdrEntry->BaseDllName,
                         ForwarderName);
            }

            //
            // Remember it's valid
            //
            Stale = FALSE;
        }

        //
        // Move to the next one
        //
        ForwarderEntry++;
    }

    //
    // Set the next bound entry to the forwarder
    //
    FirstEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)ForwarderEntry;

    //
    // Check if the binding was stale
    //
    if (Stale)
    {
        //
        // It was, so find the IAT entry for it
        //
        ImportEntry = 
            RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_IMPORT,
                                         &IatSize);

        //
        // Make sure it has a name
        //
        while (ImportEntry->Name)
        {
            /* Get the name */
            ImportName = (LPSTR)((ULONG_PTR)LdrEntry->DllBase +
                                 ImportEntry->Name);

            //
            // Compare it
            //
            if (!_stricmp(ImportName, BoundImportName)) break;

            //
            // Move to next entry
            //
            ImportEntry += 1;
        }

        //
        // If we didn't find a name, fail
        //
        if (!ImportEntry->Name)
        {
            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: LdrpWalkImportTable - failing with"
                         "STATUS_OBJECT_NAME_INVALID due to no import "
                         "descriptor name\n");
            }

            //
            // Return error
            //
            Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        //
        // Show debug message
        //
        if (ShowSnaps)
        {
            DbgPrint("LDR: Stale Bind %s from %wZ\n",
                     ImportName,
                     &LdrEntry->BaseDllName);
        }

        //
        // Snap the IAT Entry
        //
        Status = LdrpSnapIAT(DllLdrEntry,
                             LdrEntry,
                             ImportEntry,
                             FALSE);

        //
        // Make sure we didn't fail
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // Show debug message
            //
            if (ShowSnaps)
            {
                DbgPrint("LDR: %wZ failed to load import module %s; "
                         "status = %x\n",
                         &LdrEntry->BaseDllName,
                         BoundImportName,
                         Status);
            }

            //
            // Return
            //
            goto Quickie;
        }
    }

    //
    // All done
    //
    Status = STATUS_SUCCESS;

Quickie:
    //
    // Write where we are now and return
    //
    *Entry = FirstEntry;
    return Status;
}

/*++
* @name LdrpHandleNewFormatImportDescriptors
*
* The LdrpHandleNewFormatImportDescriptors routine FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param BoundEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpHandleNewFormatImportDescriptors(IN LPWSTR DllPath OPTIONAL,
                                     IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                     IN PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry)
{
    PIMAGE_BOUND_IMPORT_DESCRIPTOR FirstEntry = BoundEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Make sure we have a name
    //
    while (BoundEntry->OffsetModuleName)
    {
        //
        // Parse this descriptor
        //
        Status = LdrpHandleOneNewFormatImportDescriptor(DllPath,
                                                        LdrEntry,
                                                        &BoundEntry,
                                                        FirstEntry);
        if (!NT_SUCCESS(Status)) break;
    }

    //
    // Done
    //
    return Status;
}

/*++
* @name LdrpHandleOldFormatImportDescriptors
*
* The LdrpHandleOldFormatImportDescriptors routine FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param ImportEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpHandleOldFormatImportDescriptors(IN LPWSTR DllPath OPTIONAL,
                                     IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                     IN PIMAGE_IMPORT_DESCRIPTOR ImportEntry)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Check for Name and Thunk
    //
    while ((ImportEntry->Name) && (ImportEntry->FirstThunk))
    {
        //
        // Parse this descriptor
        //
        Status = LdrpHandleOneOldFormatImportDescriptor(DllPath,
                                                        LdrEntry,
                                                        &ImportEntry);
        if (!NT_SUCCESS(Status)) break;
    }

    //
    // Done
    //
    return Status;
}

/*++
* @name LdrpWalkImportDescriptor
*
* The LdrpWalkImportDescriptor routine FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpWalkImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                         IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG IatSize, BoundSize;
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry;

    //
    // Set up the Activation Context
    //
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format =
        RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
    RtlZeroMemory(&ActCtx, sizeof(ActCtx));

    //
    // Check if we have a manifest prober routine
    //
    if (LdrpManifestProberRoutine)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we failed above
    //
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Get the Active Activation Context
    //
    Status = RtlGetActiveActivationContext(&LdrEntry->
                                           EntryPointActivationContext);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrintEx(DPFLTR_SXS_ID,
                   0,
                   "LDR: RtlGetActiveActivationContext() failed; ntstatus = "
                   "0x%08lx\n",
                   Status);
        return Status;
    }

    //
    // Activate it
    //
    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           LdrEntry->
                                           EntryPointActivationContext);

    //
    // Check if we were directed
    //
    if (!(LdrEntry->Flags & LDRP_REDIRECTED))
    {
        //
        // Get the Bound IAT
        //
        BoundEntry = 
            RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                         &BoundSize);
    }

    //
    // Get the regular IAT, for fallback
    //
    ImportEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IMPORT,
                                               &IatSize);

    //
    // Check if we got at least one
    //
    if ((BoundEntry) || (ImportEntry))
    {
        //
        // Do we have a Bound IAT
        //
        if (BoundEntry)
        {
            //
            // Handle the descriptor
            //
            Status = LdrpHandleNewFormatImportDescriptors(DllPath,
                                                          LdrEntry,
                                                          BoundEntry);
        }
        else
        {
            //
            // Handle the descriptor
            //
            Status = LdrpHandleOldFormatImportDescriptors(DllPath,
                                                          LdrEntry,
                                                          ImportEntry);
        }

        //
        // Check the status of the handlers
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Check for Per-DLL Heap Tagging
            //
            if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_TAG_BY_DLL)
            {
                //
                // Change the imports to use heap tagging instead
                //
                Status = LdrpMungHeapImportsForTagging(LdrEntry);
                if (!NT_SUCCESS(Status)) goto exit;
            }

            //
            // Check if Page Heap was enabled
            //
            if (Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)
            {
                //
                // Setup notifications
                //
                Status = AVrfPageHeapDllNotification(LdrEntry);
                if (!NT_SUCCESS(Status)) goto exit;
            }

            //
            // Check if Application Verifier was enabled
            //
            if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_CALL_TRACING)
            {
                //
                // Setup notifications
                //
                Status = AVrfDllLoadNotification(LdrEntry);
                if (!NT_SUCCESS(Status)) goto exit;
            }

            //
            // Just to be safe
            //
            Status = STATUS_SUCCESS;
        }
    }

exit:
    //
    // Release the activation context
    //
    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

    //
    // Return status//
    return Status;
}

