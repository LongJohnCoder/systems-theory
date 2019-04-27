/*++

Copyright (c) Relsoft Technologies.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmwmireg.c

Abstract:

    The Partition Manager is responsible for detecting the addition of new
    disk devices and mount each volume to a partition, depending on how the
    layout was specified in the partition table or GPT, for Dynamic Disks.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 06-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

WMIGUIDREGINFO DiskperfGuidList[] =
{
    {
        &DiskPerfGuid,
        1,
        0
    }
};

ULONG DiskperfGuidCount = 1;

/*++
 * @name PmDetermineDeviceNameAndNumber
 *
 * The PmDetermineDeviceNameAndNumber routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param WmiRegFlags
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmDetermineDeviceNameAndNumber(IN PDEVICE_OBJECT DeviceObject,
                               IN PULONG WmiRegFlags)
{
    KEVENT Event;
    PDEVICE_EXTENSION DeviceExtension;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    STORAGE_DEVICE_NUMBER StorageDeviceNumber;
    PIRP Irp;
    PAGED_CODE();

    //
    // Initialize our event and get the device extension
    //
    DbgPrint("PmDetermineDeviceNameAndNumber: %p\n", DeviceObject);
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        DeviceExtension->NextLowerDriver,
                                        NULL,
                                        0,
                                        &StorageDeviceNumber,
                                        sizeof(StorageDeviceNumber),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Save the device number
    //
    DeviceExtension->DeviceNumber = StorageDeviceNumber.DeviceNumber;

    //
    // Build the device name and save it
    //
    DeviceExtension->DeviceName.Buffer = DeviceExtension->DeviceString;
    DeviceExtension->DeviceName.MaximumLength = sizeof(DeviceExtension->DeviceString);
    DeviceExtension->DeviceName.Length = (USHORT)
        _snwprintf(DeviceExtension->DeviceString,
                   sizeof(DeviceExtension->DeviceString) / sizeof(WCHAR),
                   L"\\Device\\Harddisk%d\\Partition%d",
                   StorageDeviceNumber.DeviceNumber,
                   StorageDeviceNumber.PartitionNumber) * sizeof(WCHAR);

    //
    // Set the WMI Registration Flags
    //
    *WmiRegFlags = StorageDeviceNumber.PartitionNumber ?
                   0 : WMIREG_FLAG_TRACE_PROVIDER | WMIREG_NOTIFY_DISK_IO;

    //
    // Return to caller
    //
    return Status;
}

/*++
 * @name PmRegisterDevice
 *
 * The PmRegisterDevice routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param WmiRegFlags
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmRegisterDevice(IN PDEVICE_OBJECT DeviceObject,
                 IN ULONG WmiRegFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PAGED_CODE();

    //
    // Make sure we have a name for WMI
    //
    DbgPrint("PmRegisterDevice: %p %lx\n", DeviceObject, WmiRegFlags);
    if (DeviceExtension->DeviceName.Length)
    {
        //
        // Register us
        //
        DbgPrint("Device: %wZ\n", &DeviceExtension->DeviceName);
        Status = IoWMIRegistrationControl(DeviceObject,
                                          WmiRegFlags | WMIREG_ACTION_REGISTER);
        if (NT_SUCCESS(Status))
        {
            //
            // Enable the timer to set it up, then disable it for now
            //
            PmWmiCounterEnable(&DeviceExtension->WmiCounterContext);
            PmWmiCounterDisable(&DeviceExtension->WmiCounterContext,
                                FALSE,
                                FALSE);
        }
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmQueryWmiRegInfo
 *
 * The PmQueryWmiRegInfo routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param RegFlags
 *        FILLMEIN
 *
 * @param InstanceName
 *        FILLMEIN
 *
 * @param RegistryPath
 *        FILLMEIN
 *
 * @param MofResourceName
 *        FILLMEIN
 *
 * @param Pdo
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmQueryWmiRegInfo(IN PDEVICE_OBJECT DeviceObject,
                  OUT PULONG RegFlags,
                  OUT PUNICODE_STRING InstanceName,
                  OUT PUNICODE_STRING *RegistryPath,
                  OUT PUNICODE_STRING MofResourceName,
                  OUT PDEVICE_OBJECT *Pdo)
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PAGED_CODE();

    //
    // Clear the instance name
    //
    RtlInitUnicodeString(InstanceName, NULL);

    //
    // Save the registry path
    //
    *RegistryPath = &DeviceExtension->DriverExtension->RegistryPath;

    //
    // Save the PDO
    //
    *Pdo = DeviceExtension->Pdo;

    //
    // Set the flags
    //
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EXPENSIVE;
    return STATUS_SUCCESS;
}

/*++
 * @name PmQueryWmiDataBlock
 *
 * The PmQueryWmiDataBlock routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param GuidIndex
 *        FILLMEIN
 *
 * @param InstanceIndex
 *        FILLMEIN
 *
 * @param InstanceCount
 *        FILLMEIN
 *
 * @param InstanceLengthArray
 *        FILLMEIN
 *
 * @param BufferAvail
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmQueryWmiDataBlock(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp,
                    IN ULONG GuidIndex,
                    IN ULONG InstanceIndex,
                    IN ULONG InstanceCount,
                    IN OUT PULONG InstanceLengthArray,
                    IN ULONG BufferAvail,
                    OUT PUCHAR Buffer)
{
    return STATUS_SUCCESS;
}

