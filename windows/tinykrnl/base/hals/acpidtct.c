/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    acpidtct.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 
    Alex Ionescu - Wrote RSDT and ACPI Table Cache Initialization - 23-Nov-06

--*/
#include "halp.h"

FAST_MUTEX HalpAcpiTableCacheLock;
LIST_ENTRY HalpAcpiTableCacheList;
PACPI_BIOS_MULTI_NODE HalpAcpiMultiNode;

/*++
 * @name HalpAcpiFindRsdtPhase0
 *
 * The HalpAcpiFindRsdtPhase0 routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpAcpiFindRsdtPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PACPI_BIOS_MULTI_NODE *AcpiMultiNode)
{
    PCONFIGURATION_COMPONENT_DATA Data = NULL, Entry;
    PACPI_BIOS_MULTI_NODE NodeAddress, NewNodeAddress;
    ULONG ConfigurationLength;
    PHYSICAL_ADDRESS Address;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;

    //
    // Don't do anything if we already have a node
    //
    if (HalpAcpiMultiNode) return STATUS_SUCCESS;

    //
    // Assume failure
    //
    *AcpiMultiNode = NULL;

    //
    // Try to find it
    //
    while (Entry = KeFindConfigurationNextEntry(LoaderBlock->ConfigurationRoot,
                                                AdapterClass,
                                                MultiFunctionAdapter,
                                                NULL,
                                                &Data))
    {
        //
        // Check the identifier
        //
        if (RtlEqualMemory(Entry->ComponentEntry.Identifier,
                           "ACPI BIOS",
                           10)) break;

        //
        // We didn't find it yet, keep trying
        //
        Data = Entry;
    }

    //
    // Check if we didn't find it
    //
    if (!Entry)
    {
        //
        // Panic
        //
        DbgPrint("**** HalpAcpiFindRsdtPHase0: did NOT find RSDT\n");
        return STATUS_NOT_FOUND;
    }

    //
    // Get the resource list and descriptor
    //
    ResourceList = Entry->ConfigurationData;
    ResourceDescriptor = &ResourceList->PartialDescriptors[0];
    NodeAddress = (PVOID)((ULONG_PTR)ResourceDescriptor +
                          sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    //
    // Calculate the size that will be required
    //
    ConfigurationLength = sizeof(ACPI_BIOS_MULTI_NODE) +
                          (((ULONG)NodeAddress->Count - 1) *
                           sizeof(ACPI_E820_ENTRY));

    //
    // Allocate memory for it
    //
    NewNodeAddress = (PVOID)HalpAllocPhysicalMemory(
        LoaderBlock,
        0x1000000,
        BYTES_TO_PAGES(ConfigurationLength),
        FALSE);
    if (NodeAddress)
    {
        //
        // Map it
        //
        Address.QuadPart = (ULONG)NewNodeAddress;
        NewNodeAddress = HalpMapPhysicalMemory64(
            Address,
            BYTES_TO_PAGES(ConfigurationLength));
    }

    //
    // Set the node address
    //
    HalpAcpiMultiNode = NewNodeAddress;
    if (!HalpAcpiMultiNode) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Copy it
    //
    RtlMoveMemory(HalpAcpiMultiNode,
                  NodeAddress,
                  ConfigurationLength);

    //
    // Return pointer and success
    //
    *AcpiMultiNode = HalpAcpiMultiNode;
    return STATUS_SUCCESS;
}

/*++
 * @name HalpAcpiCopyBiosTable
 *
 * The HalpAcpiCopyBiosTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalpAcpiCopyBiosTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN PDESCRIPTION_HEADER Header)
{
    ULONG TableLength;
    PACPI_CACHED_TABLE Table = NULL;
    PHYSICAL_ADDRESS Address;

    //
    // Calculate length needed
    //
    TableLength = Header->Length + sizeof(ACPI_CACHED_TABLE);
    if (LoaderBlock)
    {
        //
        // Allocate memory for it
        //
        Table = (PVOID)HalpAllocPhysicalMemory(LoaderBlock,
                                               0x1000000,
                                               BYTES_TO_PAGES(TableLength),
                                               FALSE);
        if (Table)
        {
            //
            // Map it
            //
            Address.QuadPart = (ULONG)Table;
            Table = HalpMapPhysicalMemory64(Address,
                                            BYTES_TO_PAGES(TableLength));
        }
    }
    else
    {
        //
        // Allocate from pool
        //
        Table = ExAllocatePoolWithTag(NonPagedPool, TableLength, 'Hal ');
    }

    //
    // Copy the table
    //
    if (Table) RtlCopyMemory(Table + 1, Header, Header->Length);
    return Table + 1;
}

/*++
 * @name HalpAcpiCacheTable
 *
 * The HalpAcpiCacheTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpAcpiCacheTable(IN PVOID Table)
{
    PACPI_CACHED_TABLE CachedTable;

    //
    // Get the cached table structure
    //
    CachedTable = (PVOID)((ULONG_PTR)Table - sizeof(ACPI_CACHED_TABLE));

    //
    // Insert it
    //
    InsertTailList(&HalpAcpiTableCacheList, &CachedTable->Links);
}

/*++
 * @name HalpAcpiTableCacheInit
 *
 * The HalpAcpiTableCacheInit routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpAcpiTableCacheInit(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    PACPI_BIOS_MULTI_NODE AcpiMultiNode;
    PHYSICAL_ADDRESS RsdtAddress;
    PRSDT_32 Rsdt;
    ULONG AcpiPages;
    PLOADER_PARAMETER_EXTENSION Extension;
    PDESCRIPTION_HEADER OldTable;

    //
    // Don't do anything if we already have a cache list
    //
    if (HalpAcpiTableCacheList.Flink) return STATUS_SUCCESS;

    //
    // Setup ACPI Table Cache Lock and list
    //
    ExInitializeFastMutex(&HalpAcpiTableCacheLock);
    InitializeListHead(&HalpAcpiTableCacheList);

    //
    // Find the RSDT Multi Node
    //
    Status = HalpAcpiFindRsdtPhase0(LoaderBlock, &AcpiMultiNode);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Get the RSDT
    //
    RsdtAddress = AcpiMultiNode->RsdtAddress;

    //
    // Check if we have a loader block
    //
    if (LoaderBlock)
    {
        //
        // Map the RSDT through physical memory
        //
        Rsdt = HalpMapPhysicalMemory64(RsdtAddress, 2);
    }
    else
    {
        //
        // Map the RSDT through I/O Space
        //
        Rsdt = MmMapIoSpace(RsdtAddress, 2 * PAGE_SIZE, MmNonCached);
    }

    //
    // Make sure that by now we have one
    //
    if (!Rsdt)
    {
        //
        // Fail
        //
        DbgPrint("HAL: Failed to map RSDT\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Validate it
    //
    if ((Rsdt->Header.Signature != 'TDSR') &&
        (Rsdt->Header.Signature != 'TDSX'))
    {
        //
        // Fail
        //
        HalDisplayString("Bad RSDT pointer\n");
        KeBugCheckEx(MISMATCHED_HAL, 4, 0xAC31, 0, 0);
    }

    //
    // Check how many pages it takes
    //
    AcpiPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(RsdtAddress.LowPart,
                                               Rsdt->Header.Length);
    if (AcpiPages != 2)
    {
        //
        // Check if we have a loader block
        //
        if (LoaderBlock)
        {
            //
            // Unmap our copy
            //
            HalpUnmapVirtualAddress(Rsdt, 2);
        }
        else
        {
            //
            // Unmap it from I/O space
            //
            MmUnmapIoSpace(Rsdt, 2 * PAGE_SIZE);
        }

        //
        // Check if we have a loader block
        //
        if (LoaderBlock)
        {
            //
            // Map the RSDT through physical memory
            //
            Rsdt = HalpMapPhysicalMemory64(RsdtAddress, AcpiPages);
        }
        else
        {
            //
            // Map the RSDT through I/O Space
            //
            Rsdt = MmMapIoSpace(RsdtAddress,
                                AcpiPages * PAGE_SIZE,
                                MmNonCached);
        }

        //
        // Make sure that by now we have one
        //
        if (!Rsdt)
        {
            //
            // Fail
            //
            DbgPrint("HAL: Couldn't remap RSDT\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Copy the ACPI Table
    //
    OldTable = &Rsdt->Header;
    Rsdt = HalpAcpiCopyBiosTable(LoaderBlock, OldTable);
    if (!Rsdt)
    {
        //
        // Fail
        //
        DbgPrint("HAL: Couldn't copy RSDT\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Check if we have a loader block
    //
    if (LoaderBlock)
    {
        //
        // Unmap our copy
        //
        HalpUnmapVirtualAddress(OldTable, AcpiPages);
    }
    else
    {
        //
        // Unmap it from I/O space
        //
        MmUnmapIoSpace(OldTable, AcpiPages * PAGE_SIZE);
    }

    //
    // Cache the table
    //
    HalpAcpiCacheTable(Rsdt);

    //
    // Get the loader extension and validate it
    //
    Extension = LoaderBlock->Extension;
    if (Extension->Size >= sizeof(LOADER_PARAMETER_EXTENSION))
    {
        //
        // Check if we got custom ACPI data
        //
        if ((Extension->AcpiTable) && (Extension->AcpiTableSize))
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
    }

    //
    // We're all done!
    //
    return STATUS_SUCCESS;
}

/*++
 * @name HalpAcpiGetTableFromBios
 *
 * The HalpAcpiGetTableFromBios routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalpAcpiGetTableFromBios(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN ULONG Signature)
{
    PRSDT_32 Rsdt;
    PXSDT Xsdt = NULL;
    PDESCRIPTION_HEADER Table = NULL;
    ULONG TableCount, i, PageCount;
    PHYSICAL_ADDRESS TableAddress, PhysicalAddress;

    //
    // Fail if the caller is trying to get the RSDT or XSDT
    //
    if ((Signature == 'TDSR') || (Signature == 'TDSX')) return NULL;

    //
    // Check if caller wants the DSDT
    //
    if (Signature == 'TDSD')
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Get the RSDT
        //
        Rsdt = HalpAcpiGetTable(LoaderBlock, 'TDSR');
        if (!Rsdt)
        {
            //
            // Couldn't find it, get the XSDT instead
            //
            NtUnhandled();
        }

        //
        // Check if we are using the XSDT
        //
        if (Xsdt)
        {
            NtUnhandled();
        }
        else
        {
            //
            // Check how many tables there are
            //
            if (Rsdt->Header.Length < FIELD_OFFSET(RSDT_32, Tables))
            {
                //
                // No tables are following
                //
                TableCount = 0;
            }
            else
            {
                //
                // Otherwise check how many pointers there are in the array
                //
                TableCount = (Rsdt->Header.Length -
                              FIELD_OFFSET(RSDT_32, Tables)) /
                             sizeof(ULONG_PTR);
            }
        }

        //
        // Start the table loop
        //
        for (i = 0; i < TableCount; i++)
        {
            //
            // Check if we're using the XSDT or the RSDT
            //
            if (Xsdt)
            {
                NtUnhandled();
            }
            else
            {
                //
                // Get the pointer
                //
                PhysicalAddress.QuadPart = Rsdt->Tables[i];
            }

            //
            // Do we have a table from before?
            //
            if (Table)
            {
                //
                // Check if we had a loader block
                //
                if (LoaderBlock)
                {
                    //
                    // Unmap from virtual memory
                    //
                    HalpUnmapVirtualAddress(Table, 2);
                }
                else
                {
                    //
                    // Unmap from I/O Space
                    //
                    MmUnmapIoSpace(Table, 2 << PAGE_SHIFT);
                }
            }

            //
            // Check if we have a loader block
            //
            if (LoaderBlock)
            {
                //
                // Map the table through physical memory
                //
                Table = HalpMapPhysicalMemory64(PhysicalAddress, 2);
            }
            else
            {
                //
                // Map the table through I/O Space
                //
                Table = MmMapIoSpace(PhysicalAddress,
                                     2 * PAGE_SIZE,
                                     MmNonCached);
            }

            //
            // Make sure mapping didn't fail
            //
            if (!Table)
            {
                //
                // Leave
                //
                DbgPrint("HAL: Failed to map ACPI table.\n");
                return NULL;
            }

            //
            // Break out if the signature matches
            //
            if (Table->Signature == Signature) break;
        }

        //
        // The loop is done, did we break out or not find anything?
        //
        if (TableCount == i)
        {
            //
            // Couldn't find any table, check if we had a loader block
            //
            if (LoaderBlock)
            {
                //
                // Unmap the last table from virtual memory
                //
                HalpUnmapVirtualAddress(Table, 2);
            }
            else
            {
                //
                // Unmap the last table from I/O Space
                //
                MmUnmapIoSpace(Table, 2 << PAGE_SHIFT);
            }

            //
            // Return failure
            //
            return NULL;
        }
    }

    //
    // We either found the DSDT or a custom table... so we should one by now.
    //
    ASSERT(Table);

    //
    // Check how many pages it takes
    //
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Table, Table->Length);
    if (PageCount != 2)
    {
        //
        // Check if we have a loader block
        //
        if (LoaderBlock)
        {
            //
            // Unmap our copy
            //
            HalpUnmapVirtualAddress(Table, 2);
        }
        else
        {
            //
            // Unmap it from I/O space
            //
            MmUnmapIoSpace(Table, 2 * PAGE_SIZE);
        }

        //
        // Check if we have a loader block
        //
        if (LoaderBlock)
        {
            //
            // Map the RSDT through physical memory
            //
            Table = HalpMapPhysicalMemory64(PhysicalAddress, PageCount);
        }
        else
        {
            //
            // Map the RSDT through I/O Space
            //
            Table = MmMapIoSpace(PhysicalAddress,
                                 PageCount * PAGE_SIZE,
                                 MmNonCached);
        }
    }

    //
    // Make sure we still have a table
    //
    if (Table)
    {
        //
        // Check if this isn't a FACP 2.0 (or below) Table
        //
        if ((Table->Signature != 'PCAF') || (Table->Revision > 2))
        {
            //
            // FIXME: make sure the table has a valid size
            //
        }
    }

    //
    // Return the table
    //
    return Table;
}

/*++
 * @name HalpAcpiGetCachedTable
 *
 * The HalpAcpiGetCachedTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalpAcpiGetCachedTable(IN ULONG Signature)
{
    PLIST_ENTRY ListHead, NextEntry;
    PACPI_CACHED_TABLE Table;
    PDESCRIPTION_HEADER Header;

    //
    // Loop the cached list
    //
    ListHead = &HalpAcpiTableCacheList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the cached table
        //
        Table = CONTAINING_RECORD(NextEntry, ACPI_CACHED_TABLE, Links);
        Header = (PDESCRIPTION_HEADER)(Table + 1);

        //
        // Compare the signature
        //
        if (Header->Signature == Signature) return Header;

        //
        // Next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Nothing found
    //
    return NULL;
}

/*++
 * @name HalpAcpiGetTable
 *
 * The HalpAcpiGetTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalpAcpiGetTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                 IN ULONG Signature)
{
    PVOID Table, TableCopy;
    ULONG PageCount;

    //
    // First try getting it from the cache
    //
    Table = HalpAcpiGetCachedTable(Signature);
    if (Table) return Table;

    //
    // Get it from the BIOS Instead
    //
    Table = HalpAcpiGetTableFromBios(LoaderBlock, Signature);
    if (!Table) return NULL;

    //
    // We have it, now copy it
    //
    TableCopy = HalpAcpiCopyBiosTable(LoaderBlock, Table);

    //
    // Calculate the number of pages
    //
    PageCount = 0;

    //
    // Check if we had a loader block
    //
    if (LoaderBlock)
    {
        //
        // Unmap from virtual memory
        //
        HalpUnmapVirtualAddress(Table, PageCount);
    }
    else
    {
        //
        // Unmap from I/O Space
        //
        MmUnmapIoSpace(Table, PageCount << PAGE_SHIFT);
    }

    //
    // If we have a table, cache it now, then return it
    //
    if (TableCopy) HalpAcpiCacheTable(TableCopy);
    return TableCopy;
}

/*++
 * @name HalAcpiGetTable
 *
 * The HalAcpiGetTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalAcpiGetTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                IN ULONG Signature)
{
    NTSTATUS Status;
    PVOID Table;

    //
    // Check if we have a loader block
    //
    if (LoaderBlock)
    {
        //
        // Initialize the ACPI Table Cache
        //
        Status = HalpAcpiTableCacheInit(LoaderBlock);
        if (!NT_SUCCESS(Status)) return NULL;
    }
    else
    {
        //
        // Acquire cache lock
        //
        ExAcquireFastMutex(&HalpAcpiTableCacheLock);
    }

    //
    // Get the table
    //
    Table = HalpAcpiGetTable(LoaderBlock, Signature);

    //
    // Release lock if we had acquired it and return the table
    //
    if (!LoaderBlock) ExReleaseFastMutex(&HalpAcpiTableCacheLock);
    return Table;
}

