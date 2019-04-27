/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    pnp.c

Abstract:

    This SCSI class disk driver is responsible for interactions with with
    various disk devices. It contains routines for failure prediction
    (S.M.A.R.T.), WMI, Power Management, Plug and Play and is 64-bit clean.

    Note: Depends on classpnp.sys

Environment:

    Kernel mode

Revision History:

    Peter Ward - 24-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

//
// Global to keep track of the disk device sequence number.
//
ULONG DiskDeviceSequenceNumber = 0;

//
// External define so we can check if the system is in safe mode.
//
extern PULONG InitSafeBootMode;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DiskAddDevice)
#pragma alloc_text(PAGE, DiskInitFdo)
#pragma alloc_text(PAGE, DiskGenerateDeviceName)
#pragma alloc_text(PAGE, DiskCreateSymbolicLinks)
#pragma alloc_text(PAGE, DiskRemoveDevice)
#pragma alloc_text(PAGE, DiskStartFdo)
#pragma alloc_text(PAGE, DiskInitPdo)
#pragma alloc_text(PAGE, DiskStartPdo)
#endif

NTSTATUS
DiskAddDevice(IN PDRIVER_OBJECT DriverObject,
              IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    ULONG RootPartitionMountable = FALSE;
    PCONFIGURATION_INFORMATION ConfigurationInformation;
    ULONG DiskCount;
    HANDLE DeviceKey, DiskKey;
    UNICODE_STRING DiskKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {0};
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Find out if partition zero will allow files systems to mount.
    //
    TRY
    {
        //
        // Open the registry key for the device.
        //
        Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         KEY_READ,
                                         &DeviceKey);

        //
        // Make sure there was no problem opening the device key
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and leave the TRY.
            //
            DbgPrint("DiskAddDevice: Error %#08lx opening device key "
                     "for pdo %p\n",
                     Status, PhysicalDeviceObject);
            LEAVE;
        }

        //
        // Initialize a unicode string in preparation for opening
        // the disk key.
        //
        RtlInitUnicodeString(&DiskKeyName, L"Disk");

        //
        // Initialize ObjectAttributes so we can open the disk key.
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DiskKeyName,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   DeviceKey,
                                   NULL);

        //
        // Now we open the disk key.
        //
        Status = ZwOpenKey(&DiskKey, KEY_READ, &ObjectAttributes);

        //
        // Close the device key.
        //
        ZwClose(DeviceKey);

        //
        // Make sure there was no problem opening the disk key
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and leave the TRY.
            //
            DbgPrint("DiskAddDevice: Error %#08lx opening disk key "
                     "for pdo %p device key %p\n",
                     Status, PhysicalDeviceObject, DeviceKey);
            LEAVE;
        }

        //
        // Set up the QueryTable structure in preparation for reading
        // the disk key.
        //
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[0].Name = L"RootPartitionMountable";
        QueryTable[0].EntryContext = &(RootPartitionMountable);

        //
        // Now we copy the information we need from the disk key into
        // the QueryTable structure.
        //
        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        DiskKey,
                                        QueryTable,
                                        NULL,
                                        NULL);

        //
        // Make sure there was no problem reading the disk key.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and continue on.
            //
            DbgPrint("DiskAddDevice: Error %#08lx reading value from "
                     "disk key %p for pdo %p\n",
                     Status, DiskKey, PhysicalDeviceObject);
        }

        //
        // Close the disk key.
        //
        ZwClose(DiskKey);
    }
    FINALLY
    {
        //
        // Check if we were successful in determining if partition zero
        // will allow file systems to mount.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // We were so print information stating such.
            //
            DbgPrint("DiskAddDevice: Will %sallow file system to mount on "
                     "partition zero of disk %p\n",
                     (RootPartitionMountable ? "" : "not "),
                     PhysicalDeviceObject);
        }
    }

    //
    // Set the disk count to zero in preparation for creating device
    // objects for the disk.
    //
    DiskCount = 0;

    //
    // Now we create all needed device objects for the disk.
    //
    Status = DiskCreateFdo(DriverObject,
                           PhysicalDeviceObject,
                           &DiskCount,
                           (BOOLEAN)!RootPartitionMountable);

    //
    // Now we grab the current number of disks initialized in the system.
    //
    ConfigurationInformation = IoGetConfigurationInformation();

    //
    // Ensure everything was successful so far.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Everthing is fine so we increment the total number of
        // disk devices in the system by one.
        //
        ConfigurationInformation->DiskCount++;

    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskInitFdo(IN PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDISK_DATA DiskData;
    ULONG ScsiBlockFlags = 0, TimeOut = 0, BytesPerSector;
    PULONG DmSectorSkew;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DeviceInterfaceName;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PIRP Irp;
    KEVENT AutoEvent;
    IO_STATUS_BLOCK IoStatusBlock = {0};
#endif
    PAGED_CODE();

    //
    // Get the device extension.
    //
    DeviceExtension = Fdo->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)DeviceExtension->CommonExtension.DriverData;

    //
    // Build the SCSI request block lookaside list for the disk.
    // FIXME: This funtion is obsolete.
    //
    ClassInitializeSrbLookasideList((PCOMMON_DEVICE_EXTENSION)DeviceExtension,
                                    PARTITION0_LIST_SIZE);

    //
    // Check to see if the devices media is removable.
    //
    if (DeviceExtension->DeviceDescriptor->RemovableMedia)
    {
        //
        // It is so we set the flag indicating as much.
        //
        Fdo->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    //
    // Tell the port driver that each time it needs a sense buffer to
    // allocate a unique one by setting the appropriate SCSI request
    // block flag. By doing this we put the responsibility of freeing
    // the buffer on us.
    //
    DeviceExtension->SrbFlags |= SRB_FLAGS_PORT_DRIVER_ALLOCSENSE;

    //
    // Check to see if the device can handle command queueing.
    //
    if (DeviceExtension->DeviceDescriptor->CommandQueueing &&
        DeviceExtension->AdapterDescriptor->CommandQueueing)
    {
        //
        // It can so we set the appropriate SCSI request block flag.
        //
        DeviceExtension->SrbFlags |= SRB_FLAGS_QUEUE_ACTION_ENABLE;
    }

    //
    // Scan for controllers that require special hacks/flags.
    //
    ClassScanForSpecial(DeviceExtension,
                        DiskBadControllers,
                        DiskSetSpecialHacks);

    //
    // Clear the drive geometry buffer.
    //
    RtlZeroMemory(&(DeviceExtension->DiskGeometry),
                  sizeof(DISK_GEOMETRY));

    //
    // Now we allocate memory for a request sense buffer.
    //
    DeviceExtension->SenseData =
        ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                              SENSE_BUFFER_SIZE,
                              DISK_TAG_START);

    //
    // Make sure there wasn't a problem allocating the request
    // sense buffer.
    //
    if (!DeviceExtension->SenseData)
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskInitFdo: Can not allocate request sense buffer\n");

        //
        // Now we set the appropriate error status and return it.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    //
    // Set the number of bytes before the start of the partition
    // to byte offset zero.
    //
    DeviceExtension->CommonExtension.StartingOffset.QuadPart = (LONGLONG)(0);

    //
    // Now we get (if any) the user specified timeout in seconds
    // for the device.
    //
    TimeOut = ClassQueryTimeOutRegistryValue(Fdo);

    //
    // Check to see if there was a user specified timeout for
    // the device.
    //
    if (TimeOut)
    {
        //
        // There was so we set the devices timeout to the user
        // specified one.
        //
        DeviceExtension->TimeOutValue = TimeOut;
    }
    else
    {
        //
        // There wasn't one so we set the devices timeout to the current
        // default of 10 seconds.
        //
        DeviceExtension->TimeOutValue = SCSI_DISK_TIMEOUT;
    }

    //
    // Check to see if the device supports removable media.
    //
    if (Fdo->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // It does so we build a registry entry that contains it's
        // physical drive name and update partitions routine, we
        // also set the the drives flags appropriately.
        //
        ClassUpdateInformationInRegistry(Fdo,
                                         "PhysicalDrive",
                                         DeviceExtension->DeviceNumber,
                                         NULL,
                                         0);

        //
        // Now we turn on media change notification for the drive.
        //
        ClassInitializeMediaChangeDetection(DeviceExtension,
                                            "Disk");

        //
        // Set the partition update routine for removable media
        //
        DiskData->UpdatePartitionRoutine = DiskUpdateRemovablePartitions;
    }
    else
    {
        //
        // It isn't a removable drive so set the flag to indicate
        // that it is safe to send StartUnit commands to this
        // device.
        //
        DeviceExtension->DeviceFlags |= DEV_SAFE_START_UNIT;

        //
        // We also set the SCSI request block flag that indicates
        // that the command queue doesn't need to be frozen.
        //
        DeviceExtension->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;


        //
        // Set the partition update routine for fixed drives
        //
        DiskData->UpdatePartitionRoutine = DiskUpdatePartitions;
    }

    //
    // Save the SCSI request block flags locally to protect against
    // flag changes caused by an error (restored at the end of the
    // routine so the class driver can get them).
    //
    ScsiBlockFlags = DeviceExtension->SrbFlags;

    //
    // Now we read the drives capacity (we use the class driver version
    // because we don't have the drives signature yet).
    //
    ClassReadDriveCapacity(Fdo);

    //
    // Get the bytes per sector of the device.
    //
    BytesPerSector = DeviceExtension->DiskGeometry.BytesPerSector;

    //
    // Check to see if the sector size is zero.
    //
    if (!BytesPerSector)
    {
        //
        // It is so we set default values for sector size and
        // sector shift (we use SectorShift to calculate
        // sectors in I/O transfers).
        //
        BytesPerSector = DeviceExtension->DiskGeometry.BytesPerSector = 512;
        DeviceExtension->SectorShift = 9;
    }

    //
    // Examine the master boot record so we can make a check to see
    // if a DM driver (OnTrack) is loaded for this drive.
    //
    HalExamineMBR(DeviceExtension->CommonExtension.DeviceObject,
                  DeviceExtension->DiskGeometry.BytesPerSector,
                  (ULONG)0x54,
                  &DmSectorSkew);

    //
    // Now we make the check to see if a DM driver (OnTrack) is
    // loaded for this drive.
    //
    if (DmSectorSkew)
    {
        //
        // One is loaded so we update the drive's device extension
        // so that subsequent calls to IoReadPartitionTable
        // recieve correct information.
        //
        DeviceExtension->DMSkew = *DmSectorSkew;
        DeviceExtension->DMActive = TRUE;
        DeviceExtension->DMByteSkew = DeviceExtension->DMSkew * BytesPerSector;

        //
        // Check if there was a sector skew value.
        //
        if (DmSectorSkew)
        {
            //
            // There was so we free it and ensure that it is not
            // used again.
            //
            ExFreePool(DmSectorSkew);
            DmSectorSkew = NULL;
        }
    }

    //
    // FIXME BEGIN: Not needed on IA64.
    //

    //
    // Make sure this isn't removable media.
    //
    if (!(Fdo->Characteristics & FILE_REMOVABLE_MEDIA))
    {
        //
        // Read the disk's signature.
        //
        DiskReadSignature(Fdo);

        //
        // Read the drives capacity.
        //
        DiskReadDriveCapacity(Fdo);

        //
        // Check to see if the port driver or BIOS has given us reliable
        // drive geometry.
        //
        if (DiskData->GeometrySource == DiskGeometryUnknown)
        {
            //
            // Neither has so we check to see if it was partitioned
            // by NT4 (or earlier).
            //
            if (DiskIsNT4Geometry(DeviceExtension))
            {
                //
                // It was so we apply the old NT4 style geometry.
                //
                DiskData->RealGeometry = DeviceExtension->DiskGeometry;
                DiskData->RealGeometry.SectorsPerTrack = 0x20;
                DiskData->RealGeometry.TracksPerCylinder = 0x40;
                DeviceExtension->DiskGeometry = DiskData->RealGeometry;
                DiskData->GeometrySource = DiskGeometryFromNT4;
            }
        }
    }

    //
    // FIXME END: Not needed on IA64.
    //

    // Initialize a unicode string in preparation for registering
    // the interface for the device.
    //
    RtlInitUnicodeString(&DeviceInterfaceName, NULL);

    //
    // Now we register the interface for the device and create
    // a new instance of the interface class for it.
    //
    Status = IoRegisterDeviceInterface(DeviceExtension->LowerPdo,
                                       (LPGUID)&DiskClassGuid,
                                       NULL,
                                       &DeviceInterfaceName);

    //
    // Check if registering the interface for the device was
    // successful.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It was so we save the interface name and enable the
        // newly created interface class.
        //
        DiskData->DiskInterfaceString = DeviceInterfaceName;
        Status = IoSetDeviceInterfaceState(&DeviceInterfaceName, TRUE);
    }
    else
    {
        //
        // It wasn't so we clear out anything that may have
        // gotten into the unicode strings buffer.
        //
        DeviceInterfaceName.Buffer = NULL;
    }

    //
    // Check if there was a problem registering the interface
    // name for the device or enabling the the interface class
    // that was created for it.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskInitFdo: Unable to register or set disk DCA "
                 "for fdo %p [%lx]\n",
                 Fdo, Status);

        //
        // Now we free the unicode string allocated for the
        // device interface name.
        //
        RtlFreeUnicodeString(&DeviceInterfaceName);

        //
        // Make sure the device's interface name is clear.
        //
        RtlInitUnicodeString(&(DiskData->DiskInterfaceString), NULL);
    }

    //
    // Create a symbolic link for the device.
    //
    DiskCreateSymbolicLinks(Fdo);

#if (NTDDI_VERSION >= NTDDI_LONGHORN) // Vista sends the SMART command here
    //
    // Initialize an event object in preparation for getting
    // the SCSI address (if available).
    //
    KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

    //
    // Build an IRP so we can get the SCSI address (if available).
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                        DeviceExtension->
                                        CommonExtension.LowerDeviceObject,
                                        NULL,
                                        0L,
                                        &(DiskData->ScsiAddress),
                                        sizeof(SCSI_ADDRESS),
                                        FALSE,
                                        &AutoEvent,
                                        &IoStatusBlock);

    //
    // Now check if the device has a SCSI address.
    //
    if (Irp)
    {
        //
        // It does so we send the IRP to the driver for the device.
        //
        Status =
            IoCallDriver(DeviceExtension->CommonExtension.LowerDeviceObject,
            Irp);

        //
        // Check if the request was queued.
        //
        if (Status == STATUS_PENDING)
        {
            //
            // It was so we wait for the request to go through and
            // save the updated status.
            //
            KeWaitForSingleObject(&AutoEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
    }
#endif

    //
    // Make sure the system isn't running in safe mode before we check if
    // the device has failure prediction capabilities.
    //
    if (!(*InitSafeBootMode))
    {
        //
        // It isn't so now we detect if the device has failure prediction
        // capabilities.
        //
        DiskDetectFailurePrediction(DeviceExtension,
                                    &DiskData->FailurePredictionCapability);

        //
        // Make sure the device has failure prediction capabilities.
        //
        if (DiskData->FailurePredictionCapability != FailurePredictionNone)
        {
            //
            // It does so we allow performance to be degraded (default).
            //
            DiskData->AllowFPPerfHit = TRUE;

            //
            // Now we enable failure prediction at the hardware level
            // and the polling for it (failure prediction is enabled
            // within DiskEnableDisableFailurePredictPolling).
            // (default polling is once an hour)
            //
            Status = DiskEnableDisableFailurePredictPolling(DeviceExtension,
                                                            TRUE,
                                          DISK_DEFAULT_FAILURE_POLLING_PERIOD);

            //
            // State that failure prediction polling has been enabled.
            //
            DbgPrint("DiskInitFdo: Failure Prediction Poll enabled as "
                     "%d for device %p\n",
                     DiskData->FailurePredictionCapability,
                     Fdo);
        }
    }
    else
    {
        //
        // The system is in safe mode so we don't enable failure prediction.
        // (It could be the reason normal mode isn't working)
        //
        DiskData->FailurePredictionCapability = FailurePredictionNone;
    }

    //
    // Initialize the verify mutex for DiskData.
    //
    KeInitializeMutex(&DiskData->VerifyMutex, MAX_SECTORS_PER_VERIFY);

    //
    // Clear the flush group context.
    //
    RtlZeroMemory(&DiskData->FlushContext, sizeof(DISK_GROUP_CONTEXT));

    //
    // Initialize the flush group context's current and next lists
    //
    InitializeListHead(&DiskData->FlushContext.CurrList);
    InitializeListHead(&DiskData->FlushContext.NextList);

    //
    // Initialize the flush group context mutex.
    //
    KeInitializeMutex(&DiskData->FlushContext.Mutex, 0);

    //
    // Initialize the flush group context event.
    //
    KeInitializeEvent(&DiskData->FlushContext.Event,
                      SynchronizationEvent,
                      FALSE);

    //
    // Now we restore the saved SCSI request block flags so the
    // class driver can get them.
    //
    DeviceExtension->SrbFlags = ScsiBlockFlags;

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskStopDevice(IN PDEVICE_OBJECT DeviceObject,
               IN UCHAR Type)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskGenerateDeviceName(
#if (NTDDI_VERSION < NTDDI_LONGHORN)
                       IN BOOLEAN IsFdo,
#endif
                       IN ULONG DeviceNumber,
#if (NTDDI_VERSION < NTDDI_LONGHORN)
                       IN ULONG PartitionNumber OPTIONAL,
                       IN PLARGE_INTEGER StartingOffset OPTIONAL,
                       IN PLARGE_INTEGER PartitionLength OPTIONAL,
#endif
                       OUT PUCHAR *RawName)
{
    UCHAR TemporaryName[64] = {0};
    NTSTATUS Status;
    PAGED_CODE();

#if (NTDDI_VERSION < NTDDI_LONGHORN)
    //
    // Check if this is the name for a PDO
    //
    if (!IsFdo)
    {
        //
        // Make sure all our arguments are there
        //
        ASSERT(PartitionNumber);
        ASSERT(PartitionLength);
        ASSERT(StartingOffset);

        //
        // Format the device name properly.
        //
        Status = RtlStringCchPrintfA(TemporaryName,
                                     (sizeof(TemporaryName) - 1),
                                     PDO_NAME_FORMAT,
                                     DeviceNumber,
                                     PartitionNumber,
                                     StartingOffset->QuadPart,
                                     PartitionLength->QuadPart,
                                     DiskDeviceSequenceNumber++);
    }
    else
    {
        //
        // Make sure our PDO arguments are not there
        //
        ASSERT(!PartitionNumber);
        ASSERT(!PartitionLength);
        ASSERT(!StartingOffset);
#endif
        //
        // Format the device name properly.
        //
        Status = RtlStringCchPrintfA(TemporaryName,
                                     (sizeof(TemporaryName) - 1),
                                     FDO_NAME_FORMAT,
                                     DeviceNumber,
                                     DiskDeviceSequenceNumber++);
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    }
#endif

    //
    // Make sure there wasn't a problem formatting the device name.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error and return the error code.
        //
        DbgPrint("DiskGenerateDeviceName: Format FDO name failed "
                 "with error: 0x%X\n",
                 Status);
        return Status;
    }

    //
    // Allocate memory for the device name.
    //
    *RawName = ExAllocatePoolWithTag(PagedPool,
                                     (strlen(TemporaryName) + 1),
                                     DISK_TAG_NAME);

    //
    // Verify there was enough memory for the device name, if there wasn't
    // we return the error code.
    //
    if (!*RawName) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Now we copy the device name.
    //
    Status = RtlStringCchCopyA(*RawName,
                               (strlen(TemporaryName) + 1),
                               TemporaryName);

    //
    // Make sure there wasn't a problem copying the device name.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskGenerateDeviceName: Device name copy failed "
                 "with error: 0x%X\n",
                 Status);

        //
        // Check if there was any data copied.
        //
        if (*RawName)
        {
            //
            // There was so we free it and ensure that it is not used again.
            //
            ExFreePool(*RawName);
            *RawName = NULL;
        }

        //
        // Return the error code.
        //
        return Status;
    }

    //
    // State that the device name has been generated.
    //
    DbgPrint("DiskGenerateDeviceName: generated \"%s\"\n",
             TemporaryName);

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

VOID
DiskCreateSymbolicLinks(IN PDEVICE_OBJECT DeviceObject)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    WCHAR WideSourceName[64] = {0};
    UNICODE_STRING UnicodeSourceName;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = CommonExtension->DriverData;

    //
    // Ensure the device has a name.
    //
    ASSERT(CommonExtension->DeviceName.Buffer);

    //
    // Make sure the well known hasn't been created already.
    //
    if (!DiskData->LinkStatus.WellKnownNameCreated)
    {
        //
        // It hasn't so we build the user visible name (using the
        // partition number and device number in the common device
        // extension).
        //
#if (NTDDI_VERSION < NTDDI_LONGHORN)
        Status = RtlStringCchPrintfW(WideSourceName,
                                     sizeof(WideSourceName) /
                                     sizeof(WideSourceName[0]) - 1,
                                     L"\\Device\\Harddisk%d\\Partition%d",
                                     CommonExtension->PartitionZeroExtension->
                                     DeviceNumber,
                                     CommonExtension->IsFdo ?
                                     0 : CommonExtension->PartitionNumber);
#else
        Status = RtlStringCchPrintfW(WideSourceName,
                                     sizeof(WideSourceName) /
                                     sizeof(WideSourceName[0]) - 1,
                                     L"\\Device\\Harddisk%d\\Partition0",
                                     CommonExtension->
                                     PartitionZeroExtension->DeviceNumber);
#endif

        //
        // Check if there was a problem building the user visible
        // name.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // There wasn't so we initialize the unicode source name
            // in preparation for linking user visible name with
            // the device name.
            //
            RtlInitUnicodeString(&UnicodeSourceName, WideSourceName);

            //
            // State that we are about to create the link.
            //
            DbgPrint("DiskCreateSymbolicLink: Linking %wZ to %wZ\n",
                     &UnicodeSourceName, &CommonExtension->DeviceName);

            //
            // Now we create the symbolic link between the user visible
            // name and the device name.
            //
            Status = IoCreateSymbolicLink(&UnicodeSourceName,
                                          &CommonExtension->DeviceName);

            //
            // Make sure creating the symbolic link was successful.
            //
            if (NT_SUCCESS(Status))
            {
                //
                // It was so we set well know name created to TRUE.
                //
                DiskData->LinkStatus.WellKnownNameCreated = TRUE;
            }
        }
    }

    //
    // Make sure the physical drive link hasn't been created
    // already.
    //
    if ((!DiskData->LinkStatus.PhysicalDriveLinkCreated)
#if (NTDDI_VERSION < NTDDI_LONGHORN)
        && (CommonExtension->IsFdo)
#endif
        )
    {
        //
        // It hasn't so we build the physical drive name (using the
        // device number in the common device extension).
        //
        Status = RtlStringCchPrintfW(WideSourceName,
                                     sizeof(WideSourceName) /
                                     sizeof(WideSourceName[0]) - 1,
                                     L"\\DosDevices\\PhysicalDrive%d",
                                     CommonExtension->
                                     PartitionZeroExtension->
                                     DeviceNumber);

        //
        // Check if there was a problem building the physical
        // drive name.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // There wasn't so we initialize the unicode source name
            // in preparation for linking physical drive name with
            // the device name.
            //
            RtlInitUnicodeString(&UnicodeSourceName, WideSourceName);

            //
            // State that we are about to create the link.
            //
            DbgPrint("DiskCreateSymbolicLink: Linking %wZ to %wZ\n",
                     &UnicodeSourceName, &CommonExtension->DeviceName);

            //
            // Now we create the symbolic link between the physical
            // drive name and the device name.
            //
            Status = IoCreateSymbolicLink(&UnicodeSourceName,
                                          &CommonExtension->DeviceName);

            //
            // Make sure creating the symbolic link was successful.
            //
            if (NT_SUCCESS(Status))
            {
                //
                // It was so we set physical drive link created
                // to TRUE.
                //
                DiskData->LinkStatus.PhysicalDriveLinkCreated = TRUE;
            }
        }
    }
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    else if (!CommonExtension->IsFdo)
    {
        //
        // Remember the fact that we didn't create a PDO Symbolic Link
        //
        DiskData->LinkStatus.PhysicalDriveLinkCreated = FALSE;
    }
#endif
}

NTSTATUS
DiskRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                 IN UCHAR Type)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskStartFdo(IN PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    STORAGE_HOTPLUG_INFO HotPlugInfo = {0};
    DISK_CACHE_INFORMATION CacheInfo = {0};
    ULONG IsPowerProtected = 0;
    PIRP Irp;
    KEVENT AutoEvent;
    IO_STATUS_BLOCK IoStatusBlock = {0};
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the device extension.
    //
    DeviceExtension = Fdo->DeviceExtension;

    //
    // Get the common device extension.
    //
    CommonExtension = &(DeviceExtension->CommonExtension);

    //
    // Get the driver specific driver data.
    //
    DiskData = CommonExtension->DriverData;

    //
    // Initialize an event object in preparation for getting
    // the hotplug information (if available).
    //
    KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

    //
    // Build an IRP so we can get the hotplug information (if available).
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_HOTPLUG_INFO,
                                        Fdo,
                                        NULL,
                                        0L,
                                        &HotPlugInfo,
                                        sizeof(STORAGE_HOTPLUG_INFO),
                                        FALSE,
                                        &AutoEvent,
                                        &IoStatusBlock);

    //
    // Now check if the device has any hotplug information.
    //
    if (Irp)
    {
        //
        // It does so we send IRP to the driver for the device.
        //
        Status = IoCallDriver(Fdo, Irp);

        //
        // Check if the request was queued.
        //
        if (Status == STATUS_PENDING)
        {
            //
            // It was so we wait for the request to go through and
            // save the updated status.
            //
            KeWaitForSingleObject(&AutoEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
    }

    //
    // Clear the write caching flag for now until we have more
    // information.
    //
    DeviceExtension->DeviceFlags &= ~DEV_WRITE_CACHE;

    //
    // Clear the forced unit flag for now until we have more
    // information (indicates if we bypass the disk's onboard
    // cache).
    //
    DeviceExtension->CdbForceUnitAccess = FALSE;

    //
    // Set the disk's write cache to the default before we
    // check if the user has overridden it.
    //
    DiskData->WriteCacheOverride = DiskWriteCacheDefault;

    //
    // Now we get the user's write cache setting from the registry.
    //
    ClassGetDeviceParameter(DeviceExtension,
                            DiskDeviceParameterSubkey,
                            DiskDeviceUserWriteCacheSetting,
                            (PULONG)&DiskData->WriteCacheOverride);

    //
    // Check if the user has overridden the default write
    // cache setting
    //
    if (DiskData->WriteCacheOverride == DiskWriteCacheDefault)
    {
        //
        // The user hasn't overridden it so we check if there is
        // any known bugs with the write cache (such as faulty
        // firmware).
        //
        if (DeviceExtension->ScanForSpecialFlags &
            CLASS_SPECIAL_DISABLE_WRITE_CACHE)
        {
            //
            // There are so we state as much and disable the write cache.
            //
            DbgPrint("DiskStartFdo: Turning off write cache for %p due "
                     "to a firmware issue\n",
                     Fdo);
            DiskData->WriteCacheOverride = DiskWriteCacheDisable;
        }
        //
        // We also check if this is a hotplug device.
        //
        else if (HotPlugInfo.DeviceHotplug &&
                 !HotPlugInfo.WriteCacheEnableOverride)
        {
            //
            // It is so we state as much and disable the write cache
            // (write cache is unsafe on a hotplug device).
            //
            DbgPrint("DiskStartFdo: Turning off write cache for %p due "
                     "to hotpluggable device\n",
                     Fdo);
            DiskData->WriteCacheOverride = DiskWriteCacheDisable;
        }
        //
        // We also check if the device's media is lockable.
        //
        else if (HotPlugInfo.MediaHotplug)
        {
            //
            // It isn't so we state as much and disable the write cache
            // (write cache is unsafe for media that isn't locked).
            //
            DbgPrint("DiskStartFdo: Turning off write cache for %p due "
                     "to unlockable media\n",
                     Fdo);
            DiskData->WriteCacheOverride = DiskWriteCacheDisable;
        }
        else
        {
            //
            // Otherwise, we have checked for most common problems with
            // the write cache so now we leave it up to the user to
            // dedcide.
            //
        }
    }

    //
    // Get caching information from the hardware.
    //
    Status = DiskGetCacheInformation(DeviceExtension, &CacheInfo);

    //
    // Make sure there was no problem getting the caching information
    // from the hardware.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Now we check if the hardware has write caching enabled.
        //
        if (CacheInfo.WriteCacheEnabled == TRUE)
        {
            //
            // It does so we turn the write caching flag on.
            //
            DeviceExtension->DeviceFlags |= DEV_WRITE_CACHE;

            //
            // Now we check to see if the device has write caching, is not
            // power protected (ie. connected to a backup power supply)
            // and supports forced unit access.
            //
            if ((DeviceExtension->DeviceFlags & DEV_WRITE_CACHE) &&
                !(DeviceExtension->DeviceFlags & DEV_POWER_PROTECTED) &&
                !(DeviceExtension->ScanForSpecialFlags &
                CLASS_SPECIAL_FUA_NOT_SUPPORTED))
            {
                //
                // Checks out fine so we enable forced unit access.
                //
                DeviceExtension->CdbForceUnitAccess = TRUE;
            }
            else
            {
                //
                // Otherwise we disable forced unit access.
                //
                DeviceExtension->CdbForceUnitAccess = FALSE;
            }

            //
            // Check if there was a hardware problem or user override
            // that means we should disable write caching.
            //
            if (DiskData->WriteCacheOverride == DiskWriteCacheDisable)
            {
                //
                // There was so we turn write caching off.
                //
                CacheInfo.WriteCacheEnabled = FALSE;

                //
                // And disable write caching on the hardware.
                //
                Status = DiskSetCacheInformation(DeviceExtension,
                                                 &CacheInfo);
            }
        }
        else
        {
            //
            // The hardware does not have write caching enabled so
            // we check if there is a user override.
            //
            if (DiskData->WriteCacheOverride == DiskWriteCacheEnable)
            {
                //
                // There is so we turn write caching on.
                //
                CacheInfo.WriteCacheEnabled = TRUE;

                //
                // And enable write caching on the hardware.
                //
                Status = DiskSetCacheInformation(DeviceExtension,
                                                 &CacheInfo);
            }
        }
    }

    //
    // Clear the power protected flag.
    //
    DeviceExtension->DeviceFlags &= ~DEV_POWER_PROTECTED;

    //
    // Now we check the registry to see if the device is power
    // protected (ie. connected to a backup power supply).
    //
    ClassGetDeviceParameter(DeviceExtension,
                            DiskDeviceParameterSubkey,
                            DiskDeviceCacheIsPowerProtected,
                            &IsPowerProtected);

    //
    // Check if the device is power protected.
    //
    if (IsPowerProtected)
    {
        //
        // It is so we turn the power protected flag on.
        //
        DeviceExtension->DeviceFlags |= DEV_POWER_PROTECTED;
    }

    //
    // Now we check to see if the device has write caching, is not
    // power protected (ie. connected to a backup power supply)
    // and supports forced unit access.
    //
    if ((DeviceExtension->DeviceFlags & DEV_WRITE_CACHE) &&
        !(DeviceExtension->DeviceFlags & DEV_POWER_PROTECTED) &&
        !(DeviceExtension->ScanForSpecialFlags &
        CLASS_SPECIAL_FUA_NOT_SUPPORTED))
    {
        //
        // Checks out fine so we enable forced unit access.
        //
        DeviceExtension->CdbForceUnitAccess = TRUE;
    }
    else
    {
        //
        // Otherwise we disable forced unit access.
        //
        DeviceExtension->CdbForceUnitAccess = FALSE;
    }

#if (NTDDI_VERSION < NTDDI_LONGHORN) // NT 5.2 sends the SMART command here
    //
    // Flush any cached partition table
    //
    DiskAcquirePartitioningLock(DeviceExtension);
    DiskInvalidatePartitionTable(DeviceExtension, TRUE);
    DiskReleasePartitioningLock(DeviceExtension);

    //
    // Initialize an event object in preparation for getting
    // the SCSI address (if available).
    //
    KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

    //
    // Build an IRP so we can get the SCSI address (if available).
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                        CommonExtension->LowerDeviceObject,
                                        NULL,
                                        0L,
                                        &(DiskData->ScsiAddress),
                                        sizeof(SCSI_ADDRESS),
                                        FALSE,
                                        &AutoEvent,
                                        &IoStatusBlock);

    //
    // Now check if the device has a SCSI address.
    //
    if (Irp)
    {
        //
        // It does so we send the IRP to the driver for the device.
        //
        Status = IoCallDriver(DeviceExtension->CommonExtension.
                              LowerDeviceObject,
                              Irp);

        //
        // Check if the request was queued.
        //
        if (Status == STATUS_PENDING)
        {
            //
            // It was so we wait for the request to go through and
            // save the updated status.
            //
            KeWaitForSingleObject(&AutoEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
    }
#endif

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskInitPdo(IN PDEVICE_OBJECT Pdo)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    UNICODE_STRING InterfaceName;
    PDISK_DATA DiskData;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = Pdo->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = CommonExtension->DriverData;

    //
    // Create symbolic names (common names) for the physical
    // device object.
    //
    DiskCreateSymbolicLinks(Pdo);

    //
    // Initialize interface name before we use it.
    //
    RtlInitUnicodeString(&InterfaceName, NULL);

    //
    // Now we register the device interface class for the physical
    // device object and create a new instance of the class.
    //
    Status = IoRegisterDeviceInterface(Pdo,
                                       (LPGUID)&PartitionClassGuid,
                                       NULL,
                                       &InterfaceName);

    //
    // Check if we were successful in registering the device
    // interface class for the physical device object.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // We were so we set the device's interface name and enable
        // the device interface class we registered.
        //
        DiskData->PartitionInterfaceString = InterfaceName;
        Status = IoSetDeviceInterfaceState(&InterfaceName, TRUE);
    }
    else
    {
        //
        // Otherwise, we clear the interface name's buffer so
        // we can free it.
        //
        InterfaceName.Buffer = NULL;
    }

    //
    // Check if we were not successful in registering the device
    // interface class for the physical device object.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // We wern't so we state that we were unable to register it.
        //
        DbgPrint("DiskInitPdo: Unable to register partition DCA for "
                 "pdo %p [%lx]\n",
                 Pdo, Status);

        //
        // Now we free the interface name and ensure that the
        // device's interface string is clear.
        //
        RtlFreeUnicodeString(&InterfaceName);
        RtlInitUnicodeString(&(DiskData->PartitionInterfaceString), NULL);
    }

    //
    // Return to the calling routine with status information.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskStartPdo(IN PDEVICE_OBJECT Pdo)
{
    PAGED_CODE();

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskQueryId(IN PDEVICE_OBJECT Pdo,
            IN BUS_QUERY_ID_TYPE IdType,
            IN PUNICODE_STRING UnicodeIdString)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
