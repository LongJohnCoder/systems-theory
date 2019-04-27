/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ldrapi.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 16-Apr-06

--*/
#include "precomp.h"

#define LdrpTidCookie() ((HandleToUlong(NtCurrentTeb()->Cid.UniqueThread) & 0xFFF) >> 16)

LONG LdrpActiveUnloadCount, LdrpLoaderLockAcquisitonCount;
LIST_ENTRY LdrpUnloadHead;
LIST_ENTRY LdrpDllNotificationList;
WCHAR DllExtension[] = L".DLL";
PLDR_DATA_TABLE_ENTRY LdrpGetModuleHandleCache;
RTL_UNLOAD_EVENT_TRACE RtlpUnloadEventTrace[16];
ULONG LdrpUnloadIndex;

/*++
* @name LdrpLoadDll
*
* The LdrpLoadDll routine FILLMEIN
*
* @param IsolationFlags
*        FILLMEIN
*
* @param DllPath
*        FILLMEIN
*
* @param DllCharacteristics
*        FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param CallInit
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpLoadDll(IN BOOLEAN IsolationFlags,
            IN PWCHAR DllPath OPTIONAL,
            IN PULONG DllCharacteristics OPTIONAL,
            IN PUNICODE_STRING DllName,
            OUT PVOID *BaseAddress,
            IN BOOLEAN CallInit)
{
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status = STATUS_SUCCESS;
    PWCHAR p1, p2;
    WCHAR NameBuffer[267];
    PWCHAR RawDllName;
    UNICODE_STRING RawDllNameString;
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    BOOLEAN InInit = LdrpInLdrInit;
    BOOLEAN Redirected = IsolationFlags & 1;
    PWCHAR DllPathCopy;
    PSHIM_DLL_LOADED DllLoaded;

    //
    // Find the name without the extension
    //
    p1 = DllName->Buffer;
StartLoop:
    p2 = NULL;
    while (*p1)
    {
        //
        // Did we find a period
        //
        if (*p1++ == '.')
        {
            //
            // Then the name ends here
            //
            p2 = p1++;
        }
        else if (*p1++ == '\\')
        {
            //
            // We found a slash, so we have a new name to parse
            //
            goto StartLoop;
        }
    }

    //
    // Validate the Raw DLL Name
    //
    RawDllName = NameBuffer;
    if (DllName->Length >= sizeof(NameBuffer))
    {
        //
        // The DLL's name is too long
        //
        return STATUS_NAME_TOO_LONG;
    }

    //
    // Save it
    //
    RtlMoveMemory(RawDllName, DllName->Buffer, DllName->Length);

    //
    // Check if no extension was found or if we got a slash
    //
    if (!(p2) || (*p2 == '\\'))
    {
        //
        // Check that we have space to add one
        //
        if ((DllName->Length + sizeof(DllExtension) + sizeof(UNICODE_NULL)) >=
            sizeof(NameBuffer))
        {
            //
            // No space to add the extension
            //
            DbgPrintEx(DPFLTR_LDR_ID,
                       0,
                       "LDR: %s - Dll name missing extension; with extension "
                       "added the name is too long\n"
                       "   DllName: (@ %p) \"%wZ\"\n"
                       "   DllName->Length: %u\n",
                       __FUNCTION__,
                       DllName,
                       DllName,
                       DllName->Length);
            return STATUS_NAME_TOO_LONG;
        }

        //
        // Add it
        //
        RawDllNameString.Length = DllName->Length + sizeof(DllExtension);
        RtlMoveMemory((PVOID)((ULONG_PTR)RawDllName + DllName->Length),
                      DllExtension,
                      sizeof(DllExtension));
    }
    else
    {
        //
        // Null terminate it
        //
        RawDllName[DllName->Length / sizeof(WCHAR)] = 0;

        //
        // Set the length
        //
        RawDllNameString.Length = DllName->Length;
    }

    //
    // Now create a unicode string for the DLL's name
    //
    RawDllNameString.MaximumLength = sizeof(NameBuffer);
    RawDllNameString.Buffer = NameBuffer;

    //
    // Check for init flag and acquire lock if we need it
    //
    if (!InInit) RtlEnterCriticalSection(&LdrpLoaderLock);
    DllPathCopy = DllPath;

    //
    // Show debug message
    //
    if (ShowSnaps)
    {
        DbgPrint("LDR: LdrLoadDll, loading %ws from %ws\n",
                 RawDllName,
                 DllPath ? DllPath : L"");
    }

    //
    // Check if the DLL is already loaded
    //
    if (!LdrpCheckForLoadedDll(DllPath,
                               &RawDllNameString,
                               FALSE,
                               Redirected,
                               &LdrEntry))
    {
        //
        // Map it
        //
        Status = LdrpMapDll(DllPath,
                            &DllPathCopy,
                            NameBuffer,
                            DllCharacteristics,
                            FALSE,
                            Redirected,
                            &LdrEntry);
        if (!NT_SUCCESS(Status)) goto Quickie;

        // 
        // Mark the DLL Ranges for Stack Traces
        //
        RtlpStkMarkDllRange(LdrEntry);

        //
        // Check if this is actually an executable
        //
        if ((DllCharacteristics) &&
            (*DllCharacteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
        {
            //
            // Remove the DLL flag and don't call any initialization routine
            //
            LdrEntry->EntryPoint = 0;
            LdrEntry->Flags &= ~LDRP_IMAGE_DLL;
        }

        //
        // Make sure it's a DLL
        //
        if (LdrEntry->Flags & LDRP_IMAGE_DLL)
        {
            //
            // Check if this is a .NET Image
            //
            if (!(LdrEntry->Flags & LDRP_COR_IMAGE))
            {
                //
                // Walk the Import Descriptor
                //
                Status = LdrpWalkImportDescriptor(DllPath, LdrEntry);
            }

            //
            // Update load count, unless it's locked
            //
            if (LdrEntry->LoadCount != -1) LdrEntry->LoadCount++;
            LdrpUpdateLoadCount2(LdrEntry, TRUE);

            //
            // Check if we failed
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // Clear entrypoint, and insert into list
                //
                LdrEntry->EntryPoint = NULL;
                InsertTailList(&PebLdr.InInitializationOrderModuleList,
                               &LdrEntry->InInitializationOrderModuleList);

                //
                // Cancel the load
                //
                LdrpClearLoadInProgress();

                //
                // Show debug message
                //
                if (ShowSnaps)
                {
                    DbgPrint("LDR: Unloading %wZ due to error %x walking "
                             "import descriptors",
                             DllName,
                             Status);
                }

                //
                // Unload the DLL
                //
                LdrUnloadDll(LdrEntry->DllBase);

                //
                // Return the error
                //
                goto Quickie;
            }
        }
        else if (LdrEntry->LoadCount != -1)
        {
            //
            // Increase load count
            //
            LdrEntry->LoadCount++;
        }

        //
        // Insert it into the list
        //
        InsertTailList(&PebLdr.InInitializationOrderModuleList,
                       &LdrEntry->InInitializationOrderModuleList);

        //
        // If we have to run the entrypoint, make sure the database is ready
        //
        if ((CallInit) && (LdrpLdrDatabaseIsSetup))
        {
            //
            // Check if the shim engine is enabled
            //
            if (g_ShimsEnabled)
            {
                //
                // Call it
                //
                DllLoaded = RtlDecodeSystemPointer(g_pfnSE_DllLoaded);
                DllLoaded(LdrEntry);
            }

            //
            // Run the init routines
            //
            Status = LdrpRunInitializeRoutines(NULL);
            if (!NT_SUCCESS(Status))
            {
                //
                // Show debug message
                //
                if (ShowSnaps)
                {
                    DbgPrint("LDR: Unloading %wZ because either its init "
                             "routine or one of its static imports failed; "
                             "status = 0x%08lx\n",
                             DllName,
                             Status);
                }

                //
                // Failed, unload the DLL
                //
                LdrUnloadDll(LdrEntry->DllBase);
            }
        }
        else
        {
            //
            // The database isn't ready, which means we were loaded
            // because of a forwarder entry, so we'll feign success.
            //
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        //
        // We were already loaded. Are we a DLL, and are we unlocked?
        //
        if ((LdrEntry->Flags & LDRP_IMAGE_DLL) && (LdrEntry->LoadCount != -1))
        {
            //
            // Increase load count
            //
            LdrEntry->LoadCount++;
            LdrpUpdateLoadCount2(LdrEntry, TRUE);

            //
            // Clear the load in progress
            //
            LdrpClearLoadInProgress();
        }
        else
        {
            //
            // Not a DLL, just increase the load count, unless we're locked
            //
            if (LdrEntry->LoadCount != -1) LdrEntry->LoadCount++;
        }
    }

Quickie:
    /* Release the lock */
    if (!InInit) RtlLeaveCriticalSection(&LdrpLoaderLock);

    //
    // Check for success
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Return the base address
        //
        *BaseAddress = LdrEntry->DllBase;
    }
    else
    {
        //
        // Nothing found
        //
        *BaseAddress = NULL;
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpGetProcedureAddress
*
* The LdrpGetProcedureAddress routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param Name
*        FILLMEIN
*
* @param Ordinal
*        FILLMEIN
*
* @param ProcedureAddress
*        FILLMEIN
*
* @param ExecuteInit
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpGetProcedureAddress(IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress,
                        IN BOOLEAN ExecuteInit)
{
    NTSTATUS Status;
    UCHAR ImportBuffer[64];
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    IMAGE_THUNK_DATA Thunk;
    PVOID ImageBase;
    PIMAGE_IMPORT_BY_NAME ImportName = NULL;
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    ULONG ExportDirSize;
    PLIST_ENTRY Entry;

    //
    // Show debug message
    //
    if (ShowSnaps) DbgPrint("LDR: LdrGetProcedureAddress by ");

    //
    // Check if we got a name
    //
    if (Name)
    {
        //
        // Show debug message
        //
        if (ShowSnaps) DbgPrint("NAME - %s\n", Name->Buffer);

        //
        // Make sure it's not too long
        //
        if ((Name->Length +  sizeof(CHAR) + sizeof(USHORT)) > MAXLONG)
        {
            //
            // Won't have enough space to add the hint
            //
            return STATUS_NAME_TOO_LONG;
        }

        //
        // Check if our buffer is large enough
        //
        if (Name->Length >= (sizeof(ImportBuffer) - sizeof(CHAR)))
        {
            //
            // Allocate from heap, plus 2 bytes for the Hint
            //
            ImportName = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         Name->Length + sizeof(CHAR) +
                                         sizeof(USHORT));
        }
        else
        {
            //
            // Use our internal buffer
            //
            ImportName = (PIMAGE_IMPORT_BY_NAME)ImportBuffer;
        }

        //
        // Clear the hint
        //
        ImportName->Hint = 0;

        //
        // Copy the name and null-terminate it
        //
        RtlMoveMemory(&ImportName->Name, Name->Buffer, Name->Length);
        ImportName->Name[Name->Length + 1] = 0;

        //
        // Clear the high bit
        //
        ImageBase = ImportName;
        Thunk.u1.AddressOfData = 0;
    }
    else
    {
        //
        // Show debug message
        //
        if (ShowSnaps) DbgPrint("ORDINAL - %lx\n", Ordinal);

        //
        // Do it by ordinal
        //
        ImageBase = NULL;
        if (Ordinal)
        {
            //
            // Write the original ordinal
            //
            Thunk.u1.Ordinal = Ordinal | IMAGE_ORDINAL_FLAG;
        }
        else
        {
            //
            // No ordinal
            //
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Acquire lock unless we are initting
    //
    if (!LdrpInLdrInit) RtlEnterCriticalSection(&LdrpLoaderLock);

    //
    // Try to find the loaded DLL
    //
    if (!LdrpCheckForLoadedDllHandle(BaseAddress, &LdrEntry))
    {
        //
        // Invalid base
        //
        return STATUS_DLL_NOT_FOUND;
    }

    //
    // Get the pointer to the export directory
    //
    ExportDir = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                             TRUE,
                                             IMAGE_DIRECTORY_ENTRY_EXPORT,
                                             &ExportDirSize);
    if (!ExportDir) return STATUS_PROCEDURE_NOT_FOUND;

    //
    // Now get the thunk
    //
    Status = LdrpSnapThunk(LdrEntry->DllBase,
                           ImageBase,
                           &Thunk,
                           &Thunk,
                           ExportDir,
                           ExportDirSize,
                           FALSE,
                           NULL);

    //
    // Finally, see if we're supposed to run the init routines
    //
    if (ExecuteInit)
    {
        //
        // It's possible a forwarded entry had us load the DLL. In that case,
        // then we will call its DllMain. Use the last loaded DLL for this.
        //
        Entry = NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Blink;
        LdrEntry = CONTAINING_RECORD(Entry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderModuleList);

        //
        // Make sure we didn't process it yet
        //
        if (!(LdrEntry->Flags & LDRP_ENTRY_PROCESSED))
        {
            //
            // Call the init routine
            //
            Status = LdrpRunInitializeRoutines(NULL);
        }
    }

    //
    // Make sure we're OK till here
    //
    if (NT_SUCCESS(Status)) *ProcedureAddress = UlongToPtr(Thunk.u1.Function);

    //
    // Cleanup
    //
    if ((ImportName) && (ImportName != (PIMAGE_IMPORT_BY_NAME)ImportBuffer))
    {
        //
        // We allocated from heap, free it
        //
        RtlFreeHeap(RtlGetProcessHeap(), 0, ImportName);
    }

    //
    // Release the CS if we entered it
    //
    if (!LdrpInLdrInit) RtlLeaveCriticalSection(&LdrpLoaderLock);

    //
    // We're done
    //
    return Status;
}

/*++
* @name LdrpSendDllNotifications
*
* The LdrpSendDllNotifications routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @param Type
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
LdrpSendDllNotifications(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                         IN BOOLEAN Type,
                         IN ULONG Flags)
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Data;
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DLL_LOADED_NOTIFICATION_ENTRY CallbackEntry;

    //
    // Fill out the data
    //
    Data.Flags = Flags;
    Data.FullDllName = &LdrEntry->FullDllName;
    Data.BaseDllName = &LdrEntry->BaseDllName;
    Data.DllBase = LdrEntry->DllBase;
    Data.SizeOfImage = LdrEntry->SizeOfImage;

    //
    // Loop the notification list
    //
    ListHead = &LdrpDllNotificationList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get a callback entry
        //
        CallbackEntry =  CONTAINING_RECORD(NextEntry,
                                           LDR_DLL_LOADED_NOTIFICATION_ENTRY,
                                           NotificationListEntry);

        //
        // Call it
        //
        CallbackEntry->Callback(Type, &Data);

        //
        // Go to the next callback
        //
        NextEntry = NextEntry->Flink;
    }
}

/*++
* @name LdrpRecordUnloadEvent
*
* The LdrpRecordUnloadEvent routine FILLMEIN
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
LdrpRecordUnloadEvent(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG i;
    PRTL_UNLOAD_EVENT_TRACE EventTrace;
    ULONG NameSize;
    PIMAGE_NT_HEADERS NtHeader;

    //
    // Get the index
    //
    i = LdrpUnloadIndex & 0xF;
    LdrpUnloadIndex++;

    //
    // Get the static structure we'll use
    //
    EventTrace = &RtlpUnloadEventTrace[i];

    //
    // Fill it out
    //
    EventTrace->BaseAddress = LdrEntry->DllBase;
    EventTrace->SizeOfImage = LdrEntry->SizeOfImage;
    EventTrace->Sequence = i;

    //
    // Copy the name
    //
    NameSize = min(LdrEntry->BaseDllName.Length,
                   sizeof(EventTrace->ImageName));
    RtlCopyMemory(EventTrace->ImageName,
                  LdrEntry->BaseDllName.Buffer,
                  NameSize);

    //
    // Get the NT Header
    //
    NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
    if (NtHeader)
    {
        //
        // Write time and checksum
        //
        EventTrace->CheckSum = NtHeader->OptionalHeader.CheckSum;
        EventTrace->TimeDateStamp = NtHeader->FileHeader.TimeDateStamp;
    }
    else
    {
        //
        // Clear time and checksum
        //
        EventTrace->CheckSum = 0;
        EventTrace->TimeDateStamp = 0;
    }
}

/*++
* @name LdrUnloadDll
*
* The LdrUnloadDll routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrUnloadDll(IN PVOID BaseAddress)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPEB Peb = NtCurrentPeb();
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PLDR_DATA_TABLE_ENTRY LdrEntry, CurrentEntry;
    PVOID EntryPoint;
    PLIST_ENTRY NextEntry;
    LIST_ENTRY UnloadList;
    PSHIM_DLL_UNLOADED ShimUnload;
    PVOID ComData;
    ULONG ComSize;

    //
    // Get the LDR Lock
    //
    if (!LdrpInLdrInit) RtlEnterCriticalSection(&LdrpLoaderLock);

    //
    // Increase the unload count
    //
    LdrpActiveUnloadCount++;

    //
    // Skip unload if we're already doing it
    //
    if (LdrpShutdownInProgress) goto Quickie;

    //
    // Make sure the DLL is valid and get its entry
    //
    if (!LdrpCheckForLoadedDllHandle(BaseAddress, &LdrEntry))
    {
        //
        // Fail
        //
        Status = STATUS_DLL_NOT_FOUND;
        goto Quickie;
    }

    //
    // Check the current Load Count
    //
    if (LdrEntry->LoadCount != -1)
    {
        //
        // Decrease it
        //
        LdrEntry->LoadCount--;

        //
        // If it's a dll
        //
        if (LdrEntry->Flags & LDRP_IMAGE_DLL)
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
                                                   LdrEntry->
                                                   EntryPointActivationContext);

            //
            // Update the load count
            //
            LdrpUpdateLoadCount2(LdrEntry, LDRP_DEREFCOUNT);

            //
            // Release the context
            //
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);
        }
    }
    else
    {
        //
        // The DLL is locked
        //
        goto Quickie;
    }

    //
    // Show debug message
    //
    if (ShowSnaps) DbgPrint("LDR: UNINIT LIST\n");

    //
    // Check if this is our only unload and initialize the unload list if so
    //
    if (LdrpActiveUnloadCount == 1) InitializeListHead(&LdrpUnloadHead);

    //
    // Loop the modules to build the list
    //
    NextEntry = PebLdr.InInitializationOrderModuleList.Blink;
    while (NextEntry != &PebLdr.InInitializationOrderModuleList)
    {
        //
        // Get the entry
        //
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderModuleList);
        NextEntry = NextEntry->Blink;

        //
        // Remove unload flag
        //
        LdrEntry->Flags &= ~LDRP_UNLOAD_IN_PROGRESS;

        //
        // If the load count is now 0
        //
        if (!LdrEntry->LoadCount)
        {
            //
            // Check if we should show message
            //
            if (ShowSnaps)
            {
                //
                // Print debug message
                //
                DbgPrint("          (%d) [%ws] %ws (%lx) deinit %lx\n",
                         LdrpActiveUnloadCount,
                         LdrEntry->BaseDllName.Buffer,
                         LdrEntry->FullDllName.Buffer,
                         LdrEntry->LoadCount,
                         LdrEntry->EntryPoint);
            }

            //
            // Save the current entry
            //
            CurrentEntry = LdrEntry;

            //
            // Check if the Shim Engine is enabled
            //
            if (g_ShimsEnabled)
            {
                //
                // Get the unload pointer and call the Shim Engine for unload
                //
                ShimUnload = RtlDecodeSystemPointer(g_pfnSE_DllUnloaded);
                ShimUnload(LdrEntry);
            }

            //
            // Unlink this entry
            //
            RemoveEntryList(&CurrentEntry->InInitializationOrderModuleList);
            RemoveEntryList(&CurrentEntry->InMemoryOrderModuleList);
            RemoveEntryList(&CurrentEntry->HashLinks);

            //
            // If there's more then one active unload
            //
            if (LdrpActiveUnloadCount > 1)
            {
                //
                // There is; flush the cached DLL handle and clear the list
                //
                LdrpLoadedDllHandleCache = NULL;
                CurrentEntry->InMemoryOrderModuleList.Flink = NULL;
            }

            //
            // Add the entry on the unload list
            //
            InsertTailList(&LdrpUnloadHead, &CurrentEntry->HashLinks);
        }
    }

    //
    // Only call the entrypoints once
    //
    if (LdrpActiveUnloadCount > 1) goto Quickie;

    //
    // Now loop the unload list and create our own
    //
    InitializeListHead(&UnloadList);
    CurrentEntry = NULL;
    NextEntry = LdrpUnloadHead.Flink;
    while (NextEntry != &LdrpUnloadHead)
    {
        //
        // If we have an active entry
        //
        if (CurrentEntry)
        {
            //
            // Remove it
            //
            RemoveEntryList(&CurrentEntry->InLoadOrderLinks);
            CurrentEntry = NULL;

            //
            // Reset list pointers
            //
            NextEntry = LdrpUnloadHead.Flink;
            if (NextEntry == &LdrpUnloadHead) break;
        }

        //
        // Get the current entry
        //
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     HashLinks);

        //
        // Log the Unload Event
        //
        LdrpRecordUnloadEvent(LdrEntry);

        //
        // Set the entry and clear it from the list
        //
        CurrentEntry = LdrEntry;
        LdrpLoadedDllHandleCache = NULL;
        CurrentEntry->InMemoryOrderModuleList.Flink = NULL;

        //
        // Move it from the global to the local list
        //
        RemoveEntryList(&CurrentEntry->HashLinks);
        InsertTailList(&UnloadList, &CurrentEntry->HashLinks);

        //
        // Get the entrypoint
        //
        EntryPoint = LdrEntry->EntryPoint;

        //
        // Check if we should call it
        //
        if ((EntryPoint) && (LdrEntry->Flags & LDRP_PROCESS_ATTACH_CALLED))
        {
            //
            // Show message
            //
            if (ShowSnaps) DbgPrint("LDR: Calling deinit %lx\n", EntryPoint);

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
            // Call the entrypoint
            //
            LdrpCallInitRoutine(EntryPoint,
                                LdrEntry->DllBase,
                                DLL_PROCESS_DETACH,
                                NULL);

            //
            // Release the context
            //
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);

            //
            // Remove it from the list
            //
            RemoveEntryList(&CurrentEntry->InLoadOrderLinks);
            CurrentEntry = NULL;
            NextEntry = LdrpUnloadHead.Flink;
        }
        else
        {
            //
            // Remove it from the list
            //
            RemoveEntryList(&CurrentEntry->InLoadOrderLinks);
            CurrentEntry = NULL;
            NextEntry = LdrpUnloadHead.Flink;
        }
    }

    //
    // Now loop our local list
    //
    NextEntry = UnloadList.Flink;
    while (NextEntry != &UnloadList)
    {
        //
        // Get the entry
        //
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, HashLinks);
        NextEntry = NextEntry->Flink;
        CurrentEntry = LdrEntry;

        //
        // Check if Heap Tail Checking is enabled
        //
        if (Peb->NtGlobalFlag & FLG_POOL_ENABLE_TAIL_CHECK)
        {
            //
            // Notify Application Verifier
            //
            AVrfDllUnloadNotification(LdrEntry);
        }

        //
        // Check if we should display a message
        //
        if (ShowSnaps)
        {
            //
            // Notify debugger
            //
            DbgPrint("LDR: Unmapping [%ws]\n", LdrEntry->BaseDllName.Buffer);
        }

        //
        // Check if this is a .NET executable
        //
        ComData =
            RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                         IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                         TRUE,
                                         &ComSize);
        if (ComData)
        {
            //
            // Unload the CLR
            //
            LdrpCorUnloadImage(LdrEntry->DllBase);
        }

        //
        // Check if we should unmap
        //
        if (!(CurrentEntry->Flags & LDR_COR_OWNS_UNMAP))
        {
            //
            // Unmap the DLL
            //
            Status = NtUnmapViewOfSection(NtCurrentProcess(),
                                          LdrEntry->DllBase);
            ASSERT(NT_SUCCESS(Status));
        }

        //
        // Unload the alternate resource module, if any
        //
        LdrUnloadAlternateResourceModule(LdrEntry->DllBase);

        //
        // Send shutdown notification
        //
        LdrpSendDllNotifications(LdrEntry, 2, LdrpShutdownInProgress);

        //
        // Check if a Hotpatch is active
        //
        if (LdrEntry->PatchInformation)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Deallocate the Entry
        //
        LdrpFinalizeAndDeallocateDataTableEntry(CurrentEntry);

        //
        // Check if this is the cached entry
        //
        if (LdrpGetModuleHandleCache == CurrentEntry)
        {
            //
            // Invalidate it
            //
            LdrpGetModuleHandleCache = NULL;
        }
    }

Quickie:
    //
    // Decrease unload count
    //
    LdrpActiveUnloadCount--;

    //
    // Release the lock
    //
    if (!LdrpInLdrInit) RtlLeaveCriticalSection(Peb->LoaderLock);

    //
    // FIXME: Rundown the Hotpatch data, if present
    //
    //LdrpRundownHotpatchList

    //
    // Return to caller
    //
    return Status;
}

/*++
* @name LdrUnlockLoaderLock
*
* The LdrUnlockLoaderLock routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param Cookie
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
NTAPI
LdrUnlockLoaderLock(IN ULONG Flags,
                    IN ULONG Cookie OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Check for valid flags
    //
    if (Flags & ~LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
    {
        //
        // Flags are invalid, check how to fail
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
        {
            //
            // The caller wants us to raise status
            //
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
        }
        else
        {
            //
            // A normal failure
            //
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    //
    // If we don't have a cookie, just return
    //
    if (!Cookie) return STATUS_SUCCESS;

    //
    // Validate the cookie
    //
    if ((Cookie & 0xF0000000) || ((Cookie << 16) ^ LdrpTidCookie()))
    {
        //
        // Invalid cookie, check how to fail
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
        {
            //
            // The caller wants us to raise status
            //
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
        }
        else
        {
            //
            // A normal failure
            //
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    //
    // Ready to release the lock
    //
    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
    {
        //
        // Do a direct leave
        //
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }
    else
    {
        //
        // Wrap this in SEH, since we're not supposed to raise
        //
        __try
        {
            //
            // Leave the lock
            //
            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }
        __except (LdrpGenericExceptionFilter(GetExceptionInformation(),
                                             __FUNCTION__))
        {
            //
            // Get exceptions tatus
            //
            Status = GetExceptionCode();
        }
    }

    //
    // All done
    //
    return Status;
}

/*++
* @name LdrLockLoaderLock
*
* The LdrLockLoaderLock routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param Disposition
*        FILLMEIN
*
* @param Cookie
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrLockLoaderLock(IN ULONG Flags,
                  OUT PULONG Disposition OPTIONAL,
                  OUT PULONG Cookie OPTIONAL)
{
    BOOLEAN InInit = LdrpInLdrInit;
    LONG OldCount;

    //
    // Zero out the outputs
    //
    if (Disposition) *Disposition = 0;
    if (Cookie) *Cookie = 0;

    //
    // Validate the flags
    //
    if (Flags & ~(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS|
                  LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY))
    {
        //
        // Flags are invalid, check how to fail
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
        {
            //
            // The caller wants us to raise status
            //
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
        }
        else
        {
            //
            // A normal failure
            //
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    //
    // Make sure we got a cookie
    //
    if (!Cookie)
    {
        //
        // No cookie check how to fail
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
        {
            //
            // The caller wants us to raise status
            //
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_3);
        }
        else
        {
            //
            // A normal failure
            //
            return STATUS_INVALID_PARAMETER_3;
        }
    }

    //
    // Sanity check
    //
    ASSERT((Disposition != NULL) ||
           !(Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY));

    //
    // If the  flag is set, make sure we have a valid pointer to use
    //
    if ((Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY) && !(Disposition))
    {
        //
        // No pointer to return the data to
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
        {
            //
            // The caller wants us to raise status
            //
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
        }
        else
        {
            //
            // A normal failure
            //
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    //
    // If we were initializing, quit now
    //
    if (InInit) goto Quickie;

    //
    // Check what locking semantic to use
    //
    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
    {
        //
        // Check if we should enter or simply try
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
        {
            //
            // Do a try
            //
            if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
            {
                //
                // It's locked
                //
                *Disposition = 2;
                goto Quickie;
            }
            else
            {
                //
                // It worked
                //
                *Disposition = 1;
            }
        }
        else
        {
            //
            // Do an enter
            //
            RtlEnterCriticalSection(&LdrpLoaderLock);

            //
            // See if result was requested
            //
            if (Disposition) *Disposition = 1;
        }

        //
        // Increase the acquisition count
        //
        OldCount = _InterlockedIncrement(&LdrpLoaderLockAcquisitonCount);

        //
        // Generate a cookie
        //
        *Cookie = LdrpTidCookie() | OldCount;
    }
    else
    {
        //
        // Check if we should enter or simply try
        //
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
        {
            //
            // Do a try
            //
            if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
            {
                //
                // It's locked
                //
                *Disposition = 2;
                goto Quickie;
            }
            else
            {
                //
                // Increase the acquisition count
                //
                OldCount = _InterlockedIncrement(&LdrpLoaderLockAcquisitonCount);

                //
                // Generate a cookie
                //
                *Cookie = LdrpTidCookie() | OldCount;
            }
        }
        else
        {
            //
            // Do an enter
            //
            RtlEnterCriticalSection(&LdrpLoaderLock);

            //
            // See if result was requested
            //
            if (Disposition) *Disposition = 1;

            //
            // Increase the acquisition count
            //
            OldCount = _InterlockedIncrement(&LdrpLoaderLockAcquisitonCount);

            //
            // Generate a cookie
            //
            *Cookie = LdrpTidCookie() | OldCount;
        }
    }

Quickie:
    //
    // Return success
    //
    return STATUS_SUCCESS;
}

