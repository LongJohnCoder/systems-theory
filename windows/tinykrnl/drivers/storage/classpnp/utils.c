/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    utils.c

Abstract:

    SCSI class driver utility funtions

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassGetDeviceParameter)
#pragma alloc_text(PAGE, ClassSetDeviceParameter)
#pragma alloc_text(PAGE, ClassScanForSpecial)
#endif

VOID
ClassGetDeviceParameter(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                        __in_opt PWSTR SubkeyName OPTIONAL,
                        __in PWSTR ParameterName,
                        IN OUT PULONG ParameterValue)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {0};
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    UNICODE_STRING LocalSubkeyName;
    HANDLE DeviceSubkeyHandle, DeviceParameterHandle;
    ULONG DefaultParameterValue;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Open the registry key for the device.
    //
    Status = IoOpenDeviceRegistryKey(DeviceExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &DeviceParameterHandle);

    //
    // Now check if we were successful in opening the device's
    // registry key and that the subkey exists.
    //
    if ((NT_SUCCESS(Status)) && (SubkeyName))
    {
        //
        // Initialize the local subkey name and the object
        // attributes so we can read the device's
        // parameters.
        //
        RtlInitUnicodeString(&LocalSubkeyName, SubkeyName);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &LocalSubkeyName,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   DeviceParameterHandle,
                                   NULL);

        //
        // Open the device's subkey.
        //
        Status = ZwOpenKey(&DeviceSubkeyHandle,
                           KEY_READ,
                           &ObjectAttributes);

        //
        // Check if there was a problem opening the
        // device's subkey.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we close the device parameter handle.
            //
            ZwClose(DeviceParameterHandle);
        }
    }

    //
    // Now check if we have the device's subkey open.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // We do so we save the default parameter value.
        //
        DefaultParameterValue = *ParameterValue;

        //
        // Set up the query table structure so we can read the
        // device's parameters from the registry.
        //
        QueryTable->Flags = RTL_QUERY_REGISTRY_DIRECT |
                            RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable->Name = ParameterName;
        QueryTable->EntryContext = ParameterValue;
        QueryTable->DefaultType = REG_DWORD;
        QueryTable->DefaultData = NULL;
        QueryTable->DefaultLength = 0;

        //
        // Now we read the device's parameters from the registry.
        //
        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR)(SubkeyName ?
                                                DeviceSubkeyHandle :
                                                DeviceParameterHandle),
                                        QueryTable,
                                        NULL,
                                        NULL);

        //
        // Check if there was a problem reading the device's
        // parameters from the registry.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we use the default parameter value.
            //
            *ParameterValue = DefaultParameterValue;
        }

        //
        // Check if there is a subkey name.
        //
        if (SubkeyName)
        {
            //
            // There is so we close the device subkey handle.
            //
            ZwClose(DeviceSubkeyHandle);
        }

        //
        // Now we close the device parameter handle.
        //
        ZwClose(DeviceParameterHandle);
    }

    //
    // Now check if there was a problem opening device's
    // registry key/subkey.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we try to open the driver specific registry
        // key for the device (2k SP3 hack).
        //
        Status = IoOpenDeviceRegistryKey(DeviceExtension->LowerPdo,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         KEY_READ,
                                         &DeviceParameterHandle);

        //
        // Now check if we were successful in opening the driver
        // specific registry for the device and that the
        // subkey exists.
        //
        if ((NT_SUCCESS(Status)) && (SubkeyName))
        {
            //
            // Initialize the local subkey name and the object
            // attributes so we can read the device's
            // parameters.
            //
            RtlInitUnicodeString(&LocalSubkeyName, SubkeyName);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &LocalSubkeyName,
                                       (OBJ_CASE_INSENSITIVE |
                                       OBJ_KERNEL_HANDLE),
                                       DeviceParameterHandle,
                                       NULL);

            //
            // Open the device's subkey.
            //
            Status = ZwOpenKey(&DeviceSubkeyHandle,
                               KEY_READ,
                               &ObjectAttributes);

            //
            // Check if there was a problem opening the
            // device's subkey.
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // There was so we close the device parameter handle.
                //
                ZwClose(DeviceParameterHandle);
            }
        }

        //
        // Now check if we have the device's subkey open.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // We do so we save the default parameter value.
            //
            DefaultParameterValue = *ParameterValue;

            //
            // Set up the query table structure so we can read the
            // device's parameters from the registry.
            //
            QueryTable->Flags = RTL_QUERY_REGISTRY_DIRECT |
                                RTL_QUERY_REGISTRY_REQUIRED;
            QueryTable->Name = ParameterName;
            QueryTable->EntryContext = ParameterValue;
            QueryTable->DefaultType = REG_DWORD;
            QueryTable->DefaultData = NULL;
            QueryTable->DefaultLength = 0;

            //
            // Now we read the device's parameters from the registry.
            //
            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR)(SubkeyName ?
                                                    DeviceSubkeyHandle :
                                                    DeviceParameterHandle),
                                            QueryTable,
                                            NULL,
                                            NULL);

            //
            // Check if we were successful reading the device's
            // parameters from the registry.
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Now we convert from the driver specific key to
                // the device specific key.
                //
                ClassSetDeviceParameter(DeviceExtension,
                                        SubkeyName,
                                        ParameterName,
                                        *ParameterValue);
            }
            else
            {
                //
                // Otherwise, we use the default parameter value.
                //
                *ParameterValue = DefaultParameterValue;
            }

            //
            // Check if there is a subkey name.
            //
            if (SubkeyName)
            {
                //
                // There is so we close the device subkey handle.
                //
                ZwClose(DeviceSubkeyHandle);
            }

            //
            // Now we close the device parameter handle.
            //
            ZwClose(DeviceParameterHandle);
        }
    }
}

NTSTATUS
ClassSetDeviceParameter(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                        __in_opt PWSTR SubkeyName,
                        __in PWSTR ParameterName,
                        IN ULONG ParameterValue)
{
    UNICODE_STRING LocalSubkeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DeviceParameterHandle, DeviceSubkeyHandle;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Open the registry key for the device.
    //
    Status = IoOpenDeviceRegistryKey(DeviceExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     (KEY_READ | KEY_WRITE),
                                     &DeviceParameterHandle);

    //
    // Make sure we were successful in opening the device's
    // registry key and that we were given a subkey name.
    //
    if ((NT_SUCCESS(Status)) && (SubkeyName))
    {
        //
        // Now we initialize our local unicode string with the
        // subkey name so we can use it.
        //
        RtlInitUnicodeString(&LocalSubkeyName, SubkeyName);

        //
        // Initialize ObjectAttributes so we can open/create the subkey
        // for the device.
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &LocalSubkeyName,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   DeviceParameterHandle,
                                   NULL);

        //
        // Now we open/create the subkey for the device.
        //
        Status = ZwCreateKey(&DeviceSubkeyHandle,
                             (KEY_READ | KEY_WRITE),
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);

        //
        // Check if there was a problem opening/creating the subkey
        // for the device.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we close the device parameter
            // object handle.
            //
            ZwClose(DeviceParameterHandle);
        }
    }

    //
    // Check to see if we have an open device subkey.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // We do so we write the new device parameters to the registry.
        //
        Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                      (PWSTR)(SubkeyName ?
                                              DeviceSubkeyHandle :
                                              DeviceParameterHandle),
                                      ParameterName,
                                      REG_DWORD,
                                      &ParameterValue,
                                      sizeof(ULONG));

        //
        // Check to see if there is a subkey name.
        //
        if (SubkeyName)
        {
            //
            // There is so we close the device subkey object handle.
            //
            ZwClose(DeviceSubkeyHandle);
        }

        //
        // Close the device parameter object handle.
        //
        ZwClose(DeviceParameterHandle);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

VOID
ClassScanForSpecial(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                    IN CLASSPNP_SCAN_FOR_SPECIAL_INFO DeviceList[],
                    IN PCLASS_SCAN_FOR_SPECIAL_HANDLER CallBackFunction)
{
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUCHAR VendorId, ProductId, ProductRevision;
    UCHAR NullString[] = "";
    PAGED_CODE();

    //
    // Ensure we have a device list and a call back function.
    //
    ASSERT(DeviceList);
    ASSERT(CallBackFunction);

    //
    // Check to see if we are missing the device list or
    // the call back function.
    //
    if ((!DeviceList) || (!CallBackFunction))
    {
        //
        // We are so we simply return to the calling routine.
        //
        return;
    }

    //
    // Get the device descriptor.
    //
    DeviceDescriptor = DeviceExtension->DeviceDescriptor;

    //
    // Check if this device has something other than a ATAPI (0)
    // or SCSI (-1) vendor ID.
    //
    if (DeviceDescriptor->VendorIdOffset != 0 &&
        DeviceDescriptor->VendorIdOffset != -1)
    {
        //
        // It is so we get the vendor ID.
        //
        VendorId = ((PUCHAR)DeviceDescriptor);
        VendorId += DeviceDescriptor->VendorIdOffset;
    }
    else
    {
        //
        // Otherwise, we NULL the vendor ID.
        //
        VendorId = NullString;
    }

    //
    // Check if this device has something other than a ATAPI (0)
    // or SCSI (-1) product ID.
    //
    if (DeviceDescriptor->ProductIdOffset != 0 &&
        DeviceDescriptor->ProductIdOffset != -1)
    {
        //
        // It is so we get the product ID.
        //
        ProductId = ((PUCHAR)DeviceDescriptor);
        ProductId += DeviceDescriptor->ProductIdOffset;
    }
    else
    {
        //
        // Otherwise, we NULL the product ID.
        //
        ProductId = NullString;
    }

    //
    // Check if this device has something other than a ATAPI (0)
    // or SCSI (-1) product revision.
    //
    if (DeviceDescriptor->ProductRevisionOffset != 0 &&
        DeviceDescriptor->ProductRevisionOffset != -1)
    {
        //
        // It is so we get the product revision.
        //
        ProductRevision = ((PUCHAR)DeviceDescriptor);
        ProductRevision += DeviceDescriptor->ProductRevisionOffset;
    }
    else
    {
        //
        // Otherwise, we NULL the product revision.
        //
        ProductRevision = NullString;
    }

    //
    // Start looping through the device list until we have a
    // match or we hit the end of the list.
    //
    for ( ;(DeviceList->VendorId != NULL ||
           DeviceList->ProductId != NULL ||
           DeviceList->ProductRevision != NULL); DeviceList++)
    {
        //
        // Check to see if the vendor ID, product ID and product
        // revision match.
        //
        if (ClasspMyStringMatches(DeviceList->VendorId, VendorId) &&
            ClasspMyStringMatches(DeviceList->ProductId, ProductId) &&
            ClasspMyStringMatches(DeviceList->ProductRevision,
                                  ProductRevision))
        {
            //
            // They do so we state that we have found a match.
            //
            DbgPrint("ClasspScanForSpecial: Found matching controller "
                     "VendorID: %s ProductID: %s Revision: %s\n",
                     VendorId, ProductId, ProductRevision);

            //
            // Now we pass the matching information to the
            // callback function.
            //
            (CallBackFunction)(DeviceExtension, DeviceList->Data);

            //
            // State that we have completed the callback and return
            // to the calling routine.
            //
            DbgPrint("ClasspScanForSpecial: completed callback\n");
            return;
        }
    }

    //
    // State that no match was found and return to the
    // calling routine.
    //
    DbgPrint("ClasspScanForSpecial: no match found for %p\n",
             DeviceExtension->DeviceObject);
}

/*++
 * @name ClasspPerfIncrementErrorCount
 *
 * The ClasspPerfIncrementErrorCount routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClasspPerfIncrementErrorCount(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension)
{
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExtension->PrivateFdoData;
    KIRQL OldIrql;
    ULONG Errors;

    //
    // Lock the FDO
    //
    KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

    //
    // Reset the Successful IO count
    //
    FdoData->Perf.SuccessfulIO = 0;

    //
    // Increase the error count
    //
    Errors = InterlockedIncrement(&FdoExtension->ErrorCount);

    //
    // Check if we exceeded level 1 error limit
    //
    if (Errors >= CLASS_ERROR_LEVEL_1)
    {
        //
        // Disable tagged queuing
        //
        FdoExtension->SrbFlags &= ~SRB_FLAGS_NO_QUEUE_FREEZE;
        FdoExtension->SrbFlags &= ~SRB_FLAGS_QUEUE_ACTION_ENABLE;

        //
        // Disable synchronous transfer
        //
        FdoExtension->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        //
        // Notify debugger
        //
        DbgPrint("ClasspPerfIncrementErrorCount: "
                 "Too many errors; disabling tagged queuing and "
                 "synchronous data tranfers.\n");
    }

    //
    // Check if we exceeded level 2 error limit
    //
    if (Errors >= CLASS_ERROR_LEVEL_2)
    {
        //
        // At this point, also disable disconnects
        //
        FdoExtension->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;

        //
        // Notify debugger
        //
        DbgPrint("ClasspPerfIncrementErrorCount: "
                 "Too many errors; disabling disconnects.\n");
    }

    //
    // Unlock the FDO
    //
    KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
}

/*++
 * @name ClasspPerfIncrementSuccessfulIo
 *
 * The ClasspPerfIncrementSuccessfulIo routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClasspPerfIncrementSuccessfulIo(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension)
{
    PCLASS_PRIVATE_FDO_DATA FdoData;
    KIRQL OldIrql;
    ULONG Errors;
    ULONG Succeeded = 0;
    FdoData = FdoExtension->PrivateFdoData;

    //
    // If we don't have any errrors, return
    //
    if (FdoExtension->ErrorCount == 0) return;

    //
    // If we don't a threshhold to renable performance, return
    //
    if (!(FdoData->Perf.ReEnableThreshhold)) return;

    //
    // Increase the number of successful I/O operations
    //
    Succeeded = InterlockedIncrement(&FdoData->Perf.SuccessfulIO);

    //
    // If we haven't hit the threshhold, return
    //
    if (Succeeded < FdoData->Perf.ReEnableThreshhold) return;

    //
    // If we hit the threshold, grab the spinlock
    //
    KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

    //
    // Re-read the value in case it changed while we were holding the lock
    //
    Succeeded = FdoData->Perf.SuccessfulIO;

    //
    // Check if we now hit our threshold
    //
    if ((FdoExtension->ErrorCount) &&
        (FdoData->Perf.ReEnableThreshhold <= Succeeded))
    {
        //
        // Reset the number of successful I/O opeerations
        //
        FdoData->Perf.SuccessfulIO = 0;

        //
        // We must have an actual error count or else we wouldn't be here
        //
        ASSERT(FdoExtension->ErrorCount > 0);

        //
        // Increase the number of errors
        //
        Errors = InterlockedDecrement(&FdoExtension->ErrorCount);

        //
        // Check if we're now below level 2
        //
        if (Errors < CLASS_ERROR_LEVEL_2)
        {
            //
            // Check if we're now at level 1
            //
            if (Errors == CLASS_ERROR_LEVEL_2 - 1)
            {
                //
                // Notify debugger
                //
                DbgPrint("ClasspPerfIncrementSuccessfulIo: "
                         "Error level 2 no longer required.\n");
            }

            //
            // Check if disconnects were disabled
            //
            if (!(FdoData->Perf.OriginalSrbFlags &
                  SRB_FLAGS_DISABLE_DISCONNECT))
            {
                //
                // Re-enable them
                //
                FdoExtension->SrbFlags &= ~SRB_FLAGS_DISABLE_DISCONNECT;
            }
        }

        //
        // Check if we're below level 1
        //
        if (Errors < CLASS_ERROR_LEVEL_1)
        {
            //
            // Check if we've reached level 0
            //
            if (Errors == CLASS_ERROR_LEVEL_1 - 1)
            {
                //
                //Tell us about it
                //
                DbgPrint("ClasspPerfIncrementSuccessfulIo: "
                         "Error level 1 no longer required.\n");
            }

            //
            // Check if we had disabled synchronous transfer
            //
            if (!(FdoData->Perf.OriginalSrbFlags &
                  SRB_FLAGS_DISABLE_SYNCH_TRANSFER))
            {
                //
                // Re-enable it
                //
                FdoExtension->SrbFlags &= ~SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
            }

            //
            // Check if we had disabled tagged queues
            //
            if (FdoData->Perf.OriginalSrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
            {
                //
                // Re-enable them
                //
                FdoExtension->SrbFlags |= SRB_FLAGS_QUEUE_ACTION_ENABLE;
            }

            //
            // Check if we had disabled multiple queues
            //
            if (FdoData->Perf.OriginalSrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE)
            {
                //
                // Re-enable them
                //
                FdoExtension->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;
            }
        }
    }

    //
    // Unlock the FDO
    //
    KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
}

PMDL
BuildDeviceInputMdl(PVOID Buffer,
                    ULONG BufferLen)
{
    PMDL Mdl;
    NTSTATUS Status;

    //
    // Allocate an MDL
    //
    Mdl = IoAllocateMdl(Buffer, BufferLen, FALSE, FALSE, NULL);

    //
    // Check for successfull allocation
    //
    if (Mdl)
    {
        try
        {
            //
            // The device is WRITING to the locked memory, we
            // request IoWriteAccess.
            //
            MmProbeAndLockPages(Mdl, KernelMode, IoWriteAccess);

        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get last status
            //
            Status = GetExceptionCode();

            //
            // Announce failed mdl allocation
            //
            DbgPrint("BuildReadMdl: MmProbeAndLockPages failed with %xh.",Status);

            //
            // Free the allocated requests
            //
            IoFreeMdl(Mdl);
            Mdl = NULL;
        }
    }
    else
    {
        //
        // Anounced fail allocation
        //
        DbgPrint("BuildReadMdl: IoAllocateMdl failed");
    }

    return Mdl;
}

VOID
FreeDeviceInputMdl(PMDL Mdl)
{
    //
    // Unlock the pages
    //
    MmUnlockPages(Mdl);

    //
    // Free the Mdl
    //
    IoFreeMdl(Mdl);
}

BOOLEAN
ClasspMyStringMatches(__in_opt IN PCHAR StringToMatch OPTIONAL,
                      __in IN PCHAR TargetString)
{
    ULONG StringLength;

    //
    // Ensure we have a target string.
    //
    ASSERT(TargetString);

    //
    // Check if we have a string to match.
    //
    if (StringToMatch == NULL)
    {
        //
        // We weren't so we treat this as an automatic match.
        //
        return TRUE;
    }

    //
    // Get the size of the string to match.
    //
    StringLength = strlen(StringToMatch);

    //
    // Check if the string to match is zero length.
    //
    if (!StringLength)
    {
        //
        // It is so we return TRUE if the target string
        // is zero length as well and return FALSE
        // otherwise.
        //
        return !strlen(TargetString);
    }

    //
    // Now we return TRUE if the strings match and return
    // FALSE otherwise.
    //
    return !strncmp(StringToMatch, TargetString, StringLength);
}
NTSTATUS
ClasspDuidQueryProperty(PDEVICE_OBJECT DeviceObject,
                        PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}
NTSTATUS
ClasspWriteCacheProperty(IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp,
                                  IN PSCSI_REQUEST_BLOCK Srb)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;

}
