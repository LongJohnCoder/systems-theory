/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    dictlib.c

Abstract:

    SCSI class driver dictionary

Environment:

    Kernel mode

Revision History:

    samwise52 - 10-Apr-2006 - Started Implementation

--*/
#include <precomp.h>

VOID
InitializeDictionary(IN PDICTIONARY Dictionary)
{
    //
    // Zero out the dictionary
    //
    RtlZeroMemory(Dictionary, sizeof(DICTIONARY));

    //
    // Give our dictionary the predefined signature
    //
    Dictionary->Signature = DICTIONARY_SIGNATURE;

    //
    // Initialize a lock
    //
    KeInitializeSpinLock(&Dictionary->SpinLock);
}

NTSTATUS
AllocateDictionaryEntry(IN PDICTIONARY Dictionary,
                        IN ULONGLONG Key,
                        __in_range(0,sizeof(FILE_OBJECT_EXTENSION))IN ULONG Size,
                        IN ULONG Tag,
                        OUT PVOID *Entry)
{
    PDICTIONARY_HEADER Header;
    KIRQL OldIrql;
    PDICTIONARY_HEADER *EntryHeader;
    NTSTATUS Status = STATUS_SUCCESS;
    Header = ExAllocatePoolWithTag(NonPagedPool,
                                   Size + sizeof(DICTIONARY_HEADER),
                                   Tag);
    *Entry = NULL;
    Header->Key = Key;

    //
    // Checkfo failed header allocation
    //
    if(!Header) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Zero out the memory for header
    //
    RtlZeroMemory(Header, sizeof(DICTIONARY_HEADER) + Size);

    //
    // Get a lock
    //
    KeAcquireSpinLock(&(Dictionary->SpinLock), &OldIrql);

    //
    // Find location of this entry in the dictionary
    //
    TRY
    {
        EntryHeader = &(Dictionary->List);

        while(*EntryHeader)
        {
            if((*EntryHeader)->Key == Key)
            {
                //
                // Dictionary must have unique key
                //
                Status = STATUS_OBJECT_NAME_COLLISION;
                LEAVE;
            }
            else if ((*EntryHeader)->Key < Key)
            {
                //
                // We will go ahead and insert the key in here
                //
                break;
            }
            else
                //
                // Try next EntryHeader
                //
                EntryHeader = &((*EntryHeader)->Next);
        }

        //
        // If we make it here then we will go ahead and do the insertion
        //
        Header->Next = *EntryHeader;
        *EntryHeader = Header;
    }

    //
    // Before exiting we must release the lock and free the memory
    //
    FINALLY
    {
        //
        // Release lock
        //
        KeReleaseSpinLock(&(Dictionary->SpinLock), OldIrql);

        //
        // If we succeded allocating entry into dictionary
        //
        if(!NT_SUCCESS(Status))
            //
            // We can freeup some used memory
            //
            ExFreePool(Header);
        else
            //
            // Give our header data to OUT PVOID Entry
            //
            *Entry = (PVOID)Header->Data;
    }
    return Status;
}

PVOID
GetDictionaryEntry(IN PDICTIONARY Dictionary,
                   IN ULONGLONG Key)
{
    PDICTIONARY_HEADER Entry;
    PVOID Data = NULL;
    KIRQL OldIrql;

    //
    // Acquire a Lock
    //
    KeAcquireSpinLock(&(Dictionary->SpinLock), &OldIrql);

    //
    // Load dictionary list into entry
    //
    Entry = Dictionary->List;

    //
    // Compare all Entries
    //
    while (Entry)
    {
        //
        // If the entry key is equal to the search key
        //
        if (Entry->Key == Key)
        {
            //
            // We now have the data we wanted
            //
            Data = Entry->Data;
            break;
        } 
        else
        {
            //
            // Move on to next entry
            //
            Entry = Entry->Next;
        }
    }

    //
    // Release the lock
    //
    KeReleaseSpinLock(&(Dictionary->SpinLock), OldIrql);

    //
    // Give the data found to caller
    //
    return Data;
}

VOID
FreeDictionaryEntry(IN PDICTIONARY Dictionary,
                    IN PVOID Entry)
{
    PDICTIONARY_HEADER Header;
    PDICTIONARY_HEADER *EntryHeader;
    KIRQL OldIrql;
    BOOLEAN Found = FALSE;
    Header = CONTAINING_RECORD(Entry, DICTIONARY_HEADER, Data);

    //
    // Aquire a spin lock
    //
    KeAcquireSpinLock(&(Dictionary->SpinLock), &OldIrql);

    //
    // Dump our dictionary list into entry header
    //
    EntryHeader = &(Dictionary->List);

    //
    // Search every entry header
    //
    while(*EntryHeader)
    {
        //
        // Compare every one
        //
        if(*EntryHeader == Header)
        {
            //
            // If we find it substitude this header with the next one
            //
            *EntryHeader = Header->Next;
            
            //
            // We've found it!
            //
            Found = TRUE;
            break;
        }
        else
        {
            //
            // Move on to next header we are still looking
            //
            EntryHeader = &(*EntryHeader)->Next;
        }
    }

    //
    // We can release the lock now
    //
    KeReleaseSpinLock(&(Dictionary->SpinLock), OldIrql);

    //
    // Using and invalid pointer will break the dictionary
    //
    ASSERT(Found);

    //
    // If we found the entry
    //
    if (Found)
    {
        //
        // Free the EntryHeader
        //
        ExFreePool(EntryHeader);
        EntryHeader = NULL;
    }
    return;
}
