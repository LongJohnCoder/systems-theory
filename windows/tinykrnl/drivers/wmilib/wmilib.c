/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    wmilib.c

Abstract:

    Very small driver library, containing 3 public functions:
    WmiCompleteRequest, WmiFireEvent and WmiSystemControl, and only 1
    internal function.

    NOTE: This module was cleanroom reverse-engineered using a document
    written by Alex Ionescu, to be found in the same dir as this source file is.

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Implemented - Feb-24-2006

--*/
#include "ntddk.h"
#include "wmilib.h"
#include "wmistr.h"

/*++
 * @name WmiCompleteRequest
 *
 * The WmiCompleteRequest routine specifies that a driver has finished
 * processing a WMI request (contained in a DpWmiXxx routine).
 *
 * @param DeviceObject
 *        Pointer to the driver's device object.
 *
 * @param Irp
 *        Pointer to the IRP
 *
 * @param Status
 *        Indicates which status to return for the IRP.
 *
 * @param BufferUsed
 *        Number of bytes needed by the buffer passed to the driver's DpWmiXxx
 *        routine. If the buffer passed is sufficient then BufferUsed is set to
 *        number of bytes needed. If the buffer is to small then Status is set
 *        to STATUS_BUFFER_TOO_SMALL and BufferUsed is set to the number of
 *        bytes needed.
 *
 * @param PriorityBoost
 *        Value to increment the runtime priority by (specified by a system
 *        defined constant)of the original thread that requested the operation.
 *        IoCompleteRequest is called with PriorityBoost when an IRP is
 *        completed.
 *
 * @return NTSTATUS
 *         The return value will always be the status WmiCompleteRequest
 *         received in Status parameter except if Status was set to
 *         STATUS_BUFFER_TOO_SMALL, in this case WmiCompleteRequest builds
 *         a WNODE_TOO_SMALL structure and returns STATUS_SUCCESS.
 *
 * @remarks WmiCompleteRequest is called by a driver's DpWmiXxx routine. It is
 *          called after either a pending IRP is processed or all other
 *          processing in the routine is finished.
 *          Notes:
 *            - IRQL <= DISPATCH_LEVEL is required to call WmiCompleteRequest.
 *            - WmiCompleteRequest must not be called in a driver's
 *              DpWmiQueryRegInfo routine.
 *            - A driver's DpWmiXxx routine should return the value received
 *              in WmiCompleteRequest's return value.
 *
 *--*/
NTSTATUS
WmiCompleteRequest(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN NTSTATUS Status,
                   IN ULONG BufferUsed,
                   IN CCHAR PriorityBoost)
{
    ULONG_PTR ResultInfo = 0;
    PIO_STACK_LOCATION IrpStack;
    PWNODE_METHOD_ITEM WnodeMethodItem;
    PWNODE_SINGLE_INSTANCE WnodeSingleInstance;
    ULONG BufferSize;
    PWNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_ALL_DATA WnodeAllData;
    ULONG Instances, i;
    ULONG DataBlockOffset;
    PULONG OffsetInstanceData;
    PULONG LengthInstanceData;
    ULONG CurrentLengthInstanceData;

    //
    // Get the IRP Stack
    //
    DbgPrint("WmiCompleteRequest\n");
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Only three IRP minor functions need to be treated in a special way
    //
    DbgPrint("MinorFunction: %lx\n", IrpStack->MinorFunction);
    if (IrpStack->MinorFunction == IRP_MN_EXECUTE_METHOD)
    {
        //
        // Get the WNODE structure and buffer size
        //
        WnodeMethodItem = IrpStack->Parameters.WMI.Buffer;
        BufferSize = WnodeMethodItem->SizeDataBlock + BufferUsed;

        //
        // Check if the driver failed because the buffer was too small
        //
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // Setup a Too Small WNODE
                //
SmallWnode:
                DbgPrint("Node too small\n");
                WnodeTooSmall = IrpStack->Parameters.WMI.Buffer;
                WnodeTooSmall->SizeNeeded = BufferSize;
                WnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                WnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;

                //
                // Return success and size of our structure
                //
                Status = STATUS_SUCCESS;
                ResultInfo = sizeof(WNODE_TOO_SMALL);
            }
        }
        else
        {
            //
            // Query the system time and write the buffer size
            //
            KeQuerySystemTime(&WnodeMethodItem->WnodeHeader.TimeStamp);
            WnodeMethodItem->WnodeHeader.BufferSize = BufferSize;
            WnodeMethodItem->SizeDataBlock = BufferUsed;
        }
    }
    else if (IrpStack->MinorFunction == IRP_MN_QUERY_SINGLE_INSTANCE)
    {
        //
        // Get the WNODE structure and buffer size
        //
        WnodeSingleInstance = IrpStack->Parameters.WMI.Buffer;
        BufferSize = WnodeSingleInstance->SizeDataBlock + BufferUsed;

        //
        // Check if the driver failed because the buffer was too small
        //
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // Setup a Too Small WNODE
                //
                goto SmallWnode;
            }
        }
        else
        {
            //
            // Set BufferSize, TimeStamp and assure it fits into a buffer
            //
            WnodeSingleInstance->WnodeHeader.BufferSize = BufferSize;
            KeQuerySystemTime(&WnodeSingleInstance->WnodeHeader.TimeStamp);
            ASSERT(WnodeSingleInstance->SizeDataBlock <= BufferUsed);
        }
    }
    else if (IrpStack->MinorFunction == IRP_MN_QUERY_ALL_DATA)
    {
        //
        // Get the WNODE structure and buffer size
        //
        WnodeAllData = IrpStack->Parameters.WMI.Buffer;

        //
        // Get the data block
        //
        DataBlockOffset = WnodeAllData->DataBlockOffset;
        BufferSize = DataBlockOffset + BufferUsed;

        //
        // Check if the driver had success
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Check if the buffer size is OK
            //
            if (BufferSize > IrpStack->Parameters.WMI.BufferSize)
            {
                //
                // It's too small. Change the status to reflect this
                //
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        //
        // Check if the driver failed because the buffer was too small
        //
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // Setup a Too Small WNODE
                //
                goto SmallWnode;
            }
        }
        else
        {
            //
            // The most complex case from all 3, which deals with returning
            // all the data from the instances
            //
            KeQuerySystemTime(&WnodeAllData->WnodeHeader.TimeStamp);
            WnodeAllData->WnodeHeader.BufferSize = BufferSize;

            //
            // Check if we have any instances
            //
            Instances = WnodeAllData->InstanceCount;
            DbgPrint("Instances: %lx\n", Instances);
            if (Instances)
            {
                //
                // Length of data block for the last instance
                //
                LengthInstanceData =
                    &WnodeAllData->OffsetInstanceDataAndLength[Instances - 1].
                    LengthInstanceData;

                //
                // Offset from beginning of WNODE_ALL_DATA to Instance Names
                //
                OffsetInstanceData =
                    &WnodeAllData->OffsetInstanceDataAndLength[Instances - 1].
                    OffsetInstanceData;

                //
                // Loop all instances
                //
                i = Instances;
                do
                {
                    //
                    // Set the length of the instance data as its size
                    //
                    *LengthInstanceData = *OffsetInstanceData;

                    //
                    // Increase pointers
                    //
                    LengthInstanceData--;
                    OffsetInstanceData -= 2;
                } while (--i);
            }

            //
            // Loop all the instances again
            //
            for (i = 0; i < Instances; i++)
            {
                //
                // Save the offset to the data
                //
                CurrentLengthInstanceData =
                    WnodeAllData->OffsetInstanceDataAndLength[i].
                    LengthInstanceData;

                //
                // Write the previous offset
                //
                WnodeAllData->OffsetInstanceDataAndLength[i].
                    OffsetInstanceData = DataBlockOffset;

                //
                // Get the next offset and align to 8 bytes
                //
                DataBlockOffset = (CurrentLengthInstanceData +
                                   DataBlockOffset + 7) &~ 7;
            }
        }
    }

    //
    // Complete this Irp, setting it's Status and Information fields
    //
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = ResultInfo;
    IoCompleteRequest(Irp, PriorityBoost);

    //
    // Return status to driver
    //
    DbgPrint("Status: %lx\n", Status);
    return Status;
}

/*++
 * @name WmiFireEvent
 *
 * The WmiFireEvent routine FILLMEIN
 *
 * @param DeviceObject
 *        Pointer to the driver's device object.
 *
 * @param Guid
 *        FILLMEIN
 *
 * @param InstanceIndex
 *        FILLMEIN
 *
 * @param EventDataSize
 *        FILLMEIN
 *
 * @param EventData
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
WmiFireEvent(IN PDEVICE_OBJECT DeviceObject,
             IN LPGUID Guid,
             IN ULONG InstanceIndex,
             IN ULONG EventDataSize,
             IN PVOID EventData)
{
    PWNODE_SINGLE_INSTANCE WnodeEventItem;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Check EventData, and if it's NULL force EventDataSize=0
    //
    if (!EventData) EventDataSize = 0;

    //
    // Allocate an event item
    //
    WnodeEventItem = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(WNODE_SINGLE_INSTANCE) +
                                           EventDataSize,
                                           'LimW');
    if (!WnodeEventItem)
    {
        //
        // WMI frees the buffer without further intervention by the driver
        //
        if (EventData) ExFreePool(EventData);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill the struct with values
    //
    RtlCopyMemory(&WnodeEventItem->WnodeHeader.Guid, Guid, sizeof(GUID));
    WnodeEventItem->WnodeHeader.ProviderId = (ULONG)DeviceObject;
    WnodeEventItem->WnodeHeader.BufferSize = sizeof(WNODE_SINGLE_INSTANCE) +
                                             EventDataSize;
    KeQuerySystemTime(&WnodeEventItem->WnodeHeader.TimeStamp);
    WnodeEventItem->WnodeHeader.Flags = WNODE_FLAG_STATIC_INSTANCE_NAMES |
                                        WNODE_FLAG_SINGLE_INSTANCE |
                                        WNODE_FLAG_EVENT_ITEM;
    WnodeEventItem->InstanceIndex = InstanceIndex;
    WnodeEventItem->SizeDataBlock = EventDataSize;
    WnodeEventItem->DataBlockOffset = sizeof(WNODE_SINGLE_INSTANCE);

    //
    // If EventData exists
    //
    if (EventData)
    {
        //
        // Copy it, and then free EventData as unneeded
        //
        RtlCopyMemory(WnodeEventItem->VariableData, EventData, EventDataSize);
        ExFreePool(EventData);
    }

    //
    // Write the event, and free WnodeEventItem if not successful
    //
    Status = IoWMIWriteEvent(WnodeEventItem);
    if (!NT_SUCCESS(Status)) ExFreePool(WnodeEventItem);

    //
    // Return status to caller
    //
    return Status;
}

/*++
 * @name WmiLibpFindGuid
 *
 * The WmiLibpFindGuid routine FILLMEIN
 *
 * @param GuidList
 *        FILLMEIN
 *
 * @param GuidCount
 *        FILLMEIN
 *
 * @param DataPath
 *        FILLMEIN
 *
 * @param WmiLibInfo
 *        FILLMEIN
 *
 * @param InstanceFound
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
WmiLibpFindGuid(PWMIGUIDREGINFO GuidList,
                         ULONG GuidCount,
                         PVOID DataPath,
                         PWMILIB_CONTEXT WmiLibInfo,
                         PULONG InstanceFound)
{
    ULONG GuidIndex;
    PAGED_CODE();

    //
    // If no GUIDs in the input - return that nothing was found
    //
    if (!GuidCount) return 0;

    //
    // Loop through all GUIDs in the list, and return index of found GUID
    //
    for (GuidIndex = 0; GuidIndex < GuidCount; GuidIndex++)
    {
        //
        // Compare the GUIDs
        //
        if (RtlCompareMemory(GuidList + GuidIndex,
                             DataPath,
                             sizeof(GUID)) == sizeof(GUID))
        {
            //
            // Break out
            //
            break;
        }
    }

    //
    // Return the index found
    //
    return GuidIndex;
}

/*++
 * @name WmiSystemControl
 *
 * The WmiSystemControl routine acts as a dispatch routine for drivers that use
 * the WMI library routines for handling WMI IRPs.
 *
 * @param WmiLibInfo
 *        Pointer to a WMILIB_CONTEXT structure which contains a driver's
 *        registration information. Information covered includes a driver's
 *        event and data blocks as well as defining entry points for the
 *        driver's WMI callback routines.
 *
 * @param DeviceObject
 *        Pointer to the driver's device object.
 *
 * @param Irp
 *        Pointer to the IRP
 *
 * @param IrpDisposition
 *        Upon the completion of WmiSystemControl IrpDisposition holds
 *        information about how the IRP was handled.
 *        Possible values include:
 *          - IrpForward
 *            The DeviceObject pointer does not match the one contained in
 *            the IRPs WMI.ProviderId. The IRP must be sent to the next lower
 *            driver.
 *          - IrpNotWmi
 *            The IRPs minor code is unknown to WMI. Unless the driver handles
 *            IRP_MJ_SYSTEM_CONTROL requests with this IRP_MN_XXX the IRP should
 *            be sent to the next lower driver.
 *          - IrpNotCompleted
 *            The IRP was processed but not completed. This is because either
 *            WMI processed a IRP_MN_REGINFO_EX or a IRP_MN_REGINFO request, or
 *            an error was detected and WMI set the appropriate error code for
 *            the IRP. IoCompleteRequest must be called by the driver to
 *            complete the IRP.
 *          - IrpProcessed
 *            The IRP has been processed and might have been completed. If the
 *            IRP is not completed by the driver's DpWmiXxx routine (which is
 *            called by WmiSystemControl) then when WmiSystemControl returns
 *            the must be completed with a call to WmiCompleteRequest by the
 *            driver.
 *
 * @return NTSTATUS
 *         Valid return codes for WmiSystemControl include:
 *           - STATUS_WMI_INSTANCE_NOT_FOUND
 *           - STATUS_WMI_GUID_NOT_FOUND
 *           - STATUS_INVALID_DEVICE_REQUEST
 *           - STATUS_SUCCESS
 *
 * @remarks A driver makes a call to WmiSystemControl when it receives a
 *          IRP_MJ_SYSTEM_CONTROL request with a WMI IRP minor code. Once
 *          confirms that the IRP is a WMI and that the specified block is
 *          valid for the driver, WmiSystemControl processes the IRP by calling
 *          the DpWmiXxx entry point found in the driver's WMILIB_CONTEXT
 *          structure.
 *          Notes:
 *            - IRQL PASSIVE_LEVEL is required to call WmiSystemControl.
 *            - IRQL PASSIVE_LEVEL is required for a driver to send a
 *              IRP_MJ_SYSTEM_CONTROL request to the next lower driver.
 *            - WMI is running at IRQL PASSIVE_LEVEL when the driver's DpWmiXxx
 *              routine is called.
 *
 *--*/
NTSTATUS
WmiSystemControl(IN PWMILIB_CONTEXT WmiLibInfo,
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp,
                 OUT PSYSCTL_IRP_DISPOSITION IrpDisposition)
{
    PIO_STACK_LOCATION IrpStack;
    UCHAR MinorFunc;
    ULONG BufferSize;
    NTSTATUS Status = STATUS_SUCCESS;
    PWNODE_ALL_DATA WnodeAllData;
    PWNODE_SINGLE_INSTANCE WnodeSingleInstance;
    PWNODE_SINGLE_ITEM WnodeSingleItem;
    PWNODE_METHOD_ITEM WnodeMethodItem;
    ULONG Guids;
    ULONG InstanceIndex, InstanceFound;
    ULONG RegFlags = 0;
    UNICODE_STRING InstanceName;
    PUNICODE_STRING RegistryPath = NULL;
    UNICODE_STRING MofResourceName;
    PDEVICE_OBJECT Pdo = NULL;
    ULONG GuidCount, MinBufferSize;
    PWMIGUIDREGINFO GuidList;
    ULONG BufferLength, BnoOrPdo;
    ULONG RegPathOffset, MofResNameOffset;
    ULONG TotalOutputBytes;
    PWMIREGINFO RegInfo;
    UNICODE_STRING EmptyString;
    BOOLEAN Flag = FALSE;
    PUNICODE_STRING BufferStr;
    ULONG i;
    PAGED_CODE();

    //
    // Get the IRP stack and minor function
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    MinorFunc = IrpStack->MinorFunction;
    DbgPrint("WmiSystemControl: %lx\n", MinorFunc);

    //
    // Confirm that it is a WMI request, keeping in mind MinorFunc code 0xa is
    // reserved
    //
    if ((MinorFunc > IRP_MN_EXECUTE_METHOD) &&
        (MinorFunc != IRP_MN_REGINFO_EX))
    {
        //
        // It's a reserved or unknown function, return.
        //
        *IrpDisposition = IrpNotWmi;
        return STATUS_SUCCESS;
    }

    //
    // We can't continue if either ProviderId mismatches, or if we don't have
    // a WMI Library context
    //
    if ((IrpStack->Parameters.WMI.ProviderId != (ULONG)DeviceObject) ||
        !(WmiLibInfo))
    {
        //
        // Notify the debugger
        //
        if (!WmiLibInfo)
        {
            KdPrint(("WMILIB: DeviceObject %X passed NULL WmiLibInfo\n",
                    DeviceObject));
        }

        //
        // Forward the IRP
        //
        (*IrpDisposition) = IrpForward;
        return Irp->IoStatus.Status;
    }

    //
    // It's WMI request, so mark it as processed and get the buffer size
    //
    (*IrpDisposition) = IrpProcessed;
    BufferSize = IrpStack->Parameters.WMI.BufferSize;

    //
    // It depends on MinorFunction how we're going to parse this information
    // but prepare all typecasts here, not to mess up inside switch statement
    //
    WnodeAllData = IrpStack->Parameters.WMI.Buffer;
    WnodeSingleInstance = IrpStack->Parameters.WMI.Buffer;
    WnodeSingleItem = IrpStack->Parameters.WMI.Buffer;
    WnodeMethodItem = IrpStack->Parameters.WMI.Buffer;

    //
    // Process REGINFO / REGINFO_EX - they don't need a call to WmiLibpFindGuid
    // because IrpStack->Parameters.WMI.Datapath may be NULL
    //
    if ((MinorFunc != IRP_MN_REGINFO) && (MinorFunc != IRP_MN_REGINFO_EX))
    {
        //
        // Assure GuidList is not NULL, and try to find GUID
        //
        DbgPrint("MinorFunction: %lx\n", MinorFunc);
        ASSERT(WmiLibInfo->GuidList != NULL);
        Guids = WmiLibpFindGuid(WmiLibInfo->GuidList,
                                WmiLibInfo->GuidCount,
                                IrpStack->Parameters.WMI.DataPath,
                                WmiLibInfo,
                                &InstanceFound);

        //
        // No GUIDs found
        //
        if (!Guids)
        {
            //
            // Fail, since this is critical
            //
            DbgPrint("GUID Not Found\n");
            Status = STATUS_WMI_GUID_NOT_FOUND;
            (*IrpDisposition) = IrpNotCompleted;
            Irp->IoStatus.Status = Status;
            return Status;
        }

        //
        // Check if we found a valid instance
        //
        if ((MinorFunc == IRP_MN_QUERY_SINGLE_INSTANCE) ||
            (MinorFunc == IRP_MN_CHANGE_SINGLE_INSTANCE) ||
            (MinorFunc == IRP_MN_CHANGE_SINGLE_ITEM) ||
            (MinorFunc == IRP_MN_EXECUTE_METHOD))
        {
            //
            // InstanceIndex is at the same offset in SINGLE_INSTANCE
            // and METHOD_ITEM structures
            //
            DbgPrint("Instance lookup\n");
            InstanceIndex = WnodeSingleInstance->InstanceIndex;

            //
            // Check if we didn't find as many instances as needed or if there
            // are no instance names provided in this WNODE struct
            //
            if (!(WnodeSingleInstance->WnodeHeader.Flags &
                  WNODE_FLAG_STATIC_INSTANCE_NAMES) ||
                (InstanceIndex >= InstanceFound))
            {
                //
                // Fail, this is critical
                //
                DbgPrint("Instance not found\n");
                Status = STATUS_WMI_INSTANCE_NOT_FOUND;
                (*IrpDisposition) = IrpNotCompleted;
                Irp->IoStatus.Status = Status;
                return Status;
            }
        }
    }

    //
    // And now finally do a big switch
    //
    DbgPrint("MinorFunction: %lx\n", MinorFunc);
    switch (MinorFunc)
    {
        case IRP_MN_REGINFO:
        case IRP_MN_REGINFO_EX:

            //
            // Get the registration info
            //
            ASSERT(WmiLibInfo->QueryWmiRegInfo != NULL);
            RegInfo = IrpStack->Parameters.WMI.Buffer;

            //
            // Clear the name strings
            //
            RtlInitUnicodeString(&InstanceName, NULL);
            RtlInitUnicodeString(&MofResourceName, NULL);

            //
            // Query the registration info
            //
            Status = WmiLibInfo->QueryWmiRegInfo(DeviceObject,
                                                 &RegFlags,
                                                 &InstanceName,
                                                 &RegistryPath,
                                                 &MofResourceName,
                                                 &Pdo);

            //
            // Check if we didn't get a PDO,
            //
            DbgPrint("RegFlags: %lx\n", RegFlags);
            if (NT_SUCCESS(Status) && !(RegFlags & WMIREG_FLAG_INSTANCE_PDO))
            {
                //
                // Make sure we got an instance name
                //
                if (!InstanceName.Buffer)
                {
                    //
                    // We didn't, this is a critical failure
                    //
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    DbgPrint("No buffer!\n");
                }
            }

            //
            // We got a PDO... do a sanity check
            //
            if (RegFlags & WMIREG_FLAG_INSTANCE_PDO) ASSERT(Pdo != NULL);

            //
            // The request failed or the case above happened
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // Free the name if we have one
                //
                DbgPrint("Failure 697!\n");
                if (InstanceName.Buffer) ExFreePool(InstanceName.Buffer);

                //
                // Fail the IRP
                //
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = Status;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Get the list of GUIDs and their count
            //
            ASSERT(WmiLibInfo->GuidList != NULL);
            GuidList = WmiLibInfo->GuidList;
            GuidCount = WmiLibInfo->GuidCount;

            //
            // Get the minimum buffer size to hold all the GUIDs
            //
            MinBufferSize = sizeof(WMIREGINFO) +
                            WmiLibInfo->GuidCount * sizeof(WMIREGGUID);

            //
            // Check if we got a PDO
            //
            if (RegFlags & WMIREG_FLAG_INSTANCE_PDO)
            {
                //
                // We don't have an actual buffer
                //
                BufferLength = 0;

                //
                // Set the pointer to the PDO
                //
                BnoOrPdo = (ULONG)Pdo;

                //
                // Remember if we got the Extended version call
                //
                if (MinorFunc == IRP_MN_REGINFO_EX) Flag = TRUE;
            }
            else
            {
                //
                // If we don't have a base name, add in the instnace list flag
                //
                if (!(RegFlags & WMIREG_FLAG_INSTANCE_BASENAME))
                    RegFlags |= WMIREG_FLAG_INSTANCE_LIST;

                //
                // Set the buffer length and the Base Name Offset,
                // which is right after the size of our structure
                //
                BufferLength = InstanceName.Length + sizeof(WCHAR);
                BnoOrPdo = MinBufferSize;
            }

            //
            // Initialize our dummy string
            //
            RtlInitUnicodeString(&EmptyString, NULL);

            //
            // Check RegistryPath now
            //
            if (!RegistryPath)
            {
                //
                // Provide empty unicode string in its place
                //
                KdPrint(("WMI: No registry path specified for device %x",
                         Pdo));
                RegistryPath = &EmptyString;
            }

            //
            // Various buffers sizes calculations, and saving resulting output
            // length into the output buffer
            //
            MofResNameOffset = MinBufferSize + BufferLength;
            RegPathOffset = MofResourceName.Length +
                            MofResNameOffset +
                            sizeof(WCHAR);
            TotalOutputBytes = RegPathOffset +
                               RegistryPath->Length +
                               sizeof(WCHAR);
            //RegInfo->BufferSize = TotalOutputBytes; // ???

            //
            // Place number of bytes our buffer takes to the WMI.Buffer
            //
            WnodeAllData->WnodeHeader.BufferSize = TotalOutputBytes;
            DbgPrint("Offsets and Bytes: %x %x %x!\n",
                     MofResNameOffset,
                     RegPathOffset,
                     TotalOutputBytes);

            //
            // Check to see if buffer is big enough
            //
            if (TotalOutputBytes > BufferSize)
            {
                //
                // It's not... free the instance name if we have one
                //
                DbgPrint("Failed! 806\n");
                if (InstanceName.Buffer) ExFreePool(InstanceName.Buffer);

                //
                // Return the number of bytes we read, and mark the IRP as
                // incomplete, not actually failed.
                //
                Irp->IoStatus.Information = sizeof(ULONG);
                Irp->IoStatus.Status = Status;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // From now on, assume success
            //
            Status = STATUS_SUCCESS;

            //
            // Set up the reg info structure with what we've calculated
            //
            RegInfo->NextWmiRegInfo = 0;
            RegInfo->MofResourceName = MofResNameOffset;
            RegInfo->RegistryPath = RegPathOffset;
            RegInfo->GuidCount = GuidCount;

            //
            // Check if we have any GUIDs
            //
            if (GuidCount > 0)
            {
                //
                // Loop the GUIDs
                //
                for (i = 0; i < GuidCount; i++)
                {
                    //
                    // Make a copy
                    //
                    RtlCopyMemory(&RegInfo->WmiRegGuid[i].Guid,
                                  &GuidList[i].Guid,
                                  sizeof(GUID));

                    //
                    // Save the current flags plus the ones in the GUID
                    //
                    RegInfo->WmiRegGuid[i].Flags = RegFlags |GuidList[i].Flags;

                    //
                    // Save the BaseNameOffset or PDO.
                    // Warning: This is a union so this works.
                    //
                    DbgPrint("WmiRegGuid: %p\n", RegInfo->WmiRegGuid[i]);
                    DbgPrint("PDO: %p\n", Pdo);
                    RegInfo->WmiRegGuid[i].Pdo = BnoOrPdo;

                    //
                    // And save the instance count
                    //
                    RegInfo->WmiRegGuid[i].InstanceCount =
                        GuidList[i].InstanceCount;

                    //
                    // If the EX version was called reference the PDO
                    //
                    if (Flag) ObReferenceObject(Pdo);
                }
            }

            //
            // Check if we need an instnace name
            //
            if (RegFlags & (WMIREG_FLAG_INSTANCE_LIST |
                            WMIREG_FLAG_INSTANCE_BASENAME))
            {
                //
                // Setup and copy Instance Name. Note THIS IS A HACK. The actual
                // string present is more of a BSTR, meaning there is a buffer at
                // the offset + 2.
                //
                DbgPrint("Here\n");
                BufferStr = (PUNICODE_STRING)((PUCHAR)RegInfo + MinBufferSize);
                BufferStr->Length = InstanceName.Length;
                RtlCopyMemory(&BufferStr->MaximumLength,
                              InstanceName.Buffer,
                              BufferStr->Length * sizeof(WCHAR));
                if (BufferStr->Length) DbgPrint("Name: %p %16S\n",
                                                BufferStr,
                                                &BufferStr->MaximumLength);
            }

            //
            // Setup and copy MofResourceName. Note THIS IS A HACK. The actual
            // string present is more of a BSTR, meaning there is a buffer at
            // the offset + 2.
            //
            BufferStr = (PUNICODE_STRING)((PUCHAR)RegInfo + MofResNameOffset);
            BufferStr->Length = MofResourceName.Length;
            RtlCopyMemory(&BufferStr->MaximumLength,
                          MofResourceName.Buffer,
                          BufferStr->Length * sizeof(WCHAR));
            if (BufferStr->Length) DbgPrint("Name: %p %16S\n",
                                            BufferStr,
                                            &BufferStr->MaximumLength);

            //
            // Setup and copy Registry Path. Note THIS IS A HACK. The actual
            // string present is more of a BSTR, meaning there is a buffer at
            // the offset + 2.
            //
            BufferStr = (PUNICODE_STRING)((PUCHAR)RegInfo + RegPathOffset);
            BufferStr->Length = RegistryPath->Length;
            RtlCopyMemory(&BufferStr->MaximumLength,
                          RegistryPath->Buffer,
                          BufferStr->Length * sizeof(WCHAR));
            DbgPrint("Name: %p %16S\n", BufferStr, &BufferStr->MaximumLength);

            //
            // Free the InstanceName buffer if it's allocated
            //
            if (InstanceName.Buffer) ExFreePool(InstanceName.Buffer);

            //
            // Return the IRP Status and mark it as non-completed
            // FIXME: WHY???
            //
            DbgPrint("returning non-completed\n");
            Irp->IoStatus.Information = TotalOutputBytes;
            Irp->IoStatus.Status = Status;
            (*IrpDisposition) = IrpNotCompleted;
            return Status;

        case IRP_MN_QUERY_ALL_DATA:
            //
            // FIXME: TODO
            //
            KdPrint(("Wmilib: IRP_MN_QUERY_ALL_DATA not handled!\n"));
            break;

        case IRP_MN_QUERY_SINGLE_INSTANCE:
            //
            // Make sure we have a handler registered
            //
            ASSERT(WmiLibInfo->QueryWmiDataBlock != NULL);

            //
            // Call the function in the driver handling it
            //
            Status = WmiLibInfo->QueryWmiDataBlock(
                DeviceObject,
                Irp,
                WmiLibInfo->GuidCount,
                InstanceIndex,
                1,
                &WnodeSingleInstance->SizeDataBlock,
                BufferSize - WnodeSingleInstance->DataBlockOffset,
                (PUCHAR)WnodeAllData + WnodeSingleInstance->DataBlockOffset);
            break;

        case IRP_MN_CHANGE_SINGLE_INSTANCE:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->SetWmiDataBlock)
            {
                //
                // Fail the request
                //
                Status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Otherwise, call the handler in the driver
            //
            Status = WmiLibInfo->SetWmiDataBlock(
                DeviceObject,
                Irp,
                WmiLibInfo->GuidCount,
                InstanceIndex,
                WnodeSingleInstance->SizeDataBlock,
                (PUCHAR)WnodeSingleInstance +
                WnodeSingleInstance->DataBlockOffset);
            break;

        case IRP_MN_CHANGE_SINGLE_ITEM:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->SetWmiDataItem)
            {
                //
                // Fail the request
                //
                Status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Call the handler in the driver
            //
            Status = WmiLibInfo->SetWmiDataItem(
                DeviceObject,
                Irp,
                WmiLibInfo->GuidCount,
                InstanceIndex,
                WnodeSingleInstance->DataBlockOffset,
                *(PULONG)WnodeSingleInstance->VariableData,
                (PUCHAR)WnodeSingleInstance +
                WnodeSingleInstance->DataBlockOffset);
            break;

        case IRP_MN_EXECUTE_METHOD:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->ExecuteWmiMethod)
            {
                //
                // Fail the request
                //
                Status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Call the handler in the driver
            //
            Status = WmiLibInfo->ExecuteWmiMethod(
                DeviceObject,
                Irp,
                WmiLibInfo->GuidCount,
                InstanceIndex,
                WnodeMethodItem->MethodId,
                *(PULONG)WnodeMethodItem->VariableData,
                BufferSize - WnodeMethodItem->SizeDataBlock,
                (PCHAR)WnodeSingleInstance + WnodeMethodItem->DataBlockOffset);
            break;

        case IRP_MN_ENABLE_EVENTS:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->WmiFunctionControl)
            {
                //
                // Fail the request
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Otherwise, call the handler
            //
            Status = WmiLibInfo->WmiFunctionControl(DeviceObject,
                                                    Irp,
                                                    WmiLibInfo->GuidCount,
                                                    TRUE,
                                                    WmiEventControl);
            break;

        case IRP_MN_DISABLE_EVENTS:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->WmiFunctionControl)
            {
                //
                // Fail the request
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Otherwise, call the handler
            //
            Status = WmiLibInfo->WmiFunctionControl(DeviceObject,
                                                    Irp,
                                                    WmiLibInfo->GuidCount,
                                                    FALSE,
                                                    WmiEventControl);
            break;
        case IRP_MN_ENABLE_COLLECTION:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->WmiFunctionControl)
            {
                //
                // Fail the request
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Otherwise, call the handler
            //
            Status = WmiLibInfo->WmiFunctionControl(DeviceObject,
                                                    Irp,
                                                    WmiLibInfo->GuidCount,
                                                    TRUE,
                                                    WmiDataBlockControl);
            break;

        case IRP_MN_DISABLE_COLLECTION:
            //
            // Make sure we have a handler registered
            //
            if (!WmiLibInfo->WmiFunctionControl)
            {
                //
                // Fail the request
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                (*IrpDisposition) = IrpNotCompleted;
                return Status;
            }

            //
            // Otherwise, call the handler
            //
            Status = WmiLibInfo->WmiFunctionControl(DeviceObject,
                                                    Irp,
                                                    WmiLibInfo->GuidCount,
                                                    FALSE,
                                                    WmiDataBlockControl);
            break;

        default:
            //
            // Unrecognized function...fail
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = Status;
    }

    //
    // If we got here, forward the IRP and return status
    //
    (*IrpDisposition) = IrpForward;
    return Irp->IoStatus.Status;
}

/*++
 * @name DriverEntry
 *
 * The DriverEntry routine FILLMEIN
 *
 * @param DeviceObject
 *        Pointer to the driver's device object.
 *
 * @param RegistryPath
 *        Pointer to a counted Unicode string that contains the path to the
 *        driver's registry key.
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    //
    // Nothing for us to do...we function as an export driver
    //
    return STATUS_SUCCESS;
}
