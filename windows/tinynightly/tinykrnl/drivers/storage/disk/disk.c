/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    disk.c

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
// Global flag to show if the driver has been reinitialized or not.
//
BOOLEAN DiskIsPastReinitialization = FALSE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DiskShutdownFlush)
#pragma alloc_text(PAGE, DisableWriteCache)
#pragma alloc_text(PAGE, DiskSetSpecialHacks)
#pragma alloc_text(PAGE, DiskGetCacheInformation)
#pragma alloc_text(PAGE, DiskSetCacheInformation)
#pragma alloc_text(PAGE, DiskIoctlGetLengthInfo)
#pragma alloc_text(PAGE, DiskIoctlGetDriveGeometry)
#pragma alloc_text(PAGE, DiskIoctlGetDriveGeometryEx)
#pragma alloc_text(PAGE, DiskIoctlGetCacheInformation)
#pragma alloc_text(PAGE, DiskIoctlSetCacheInformation)
#pragma alloc_text(PAGE, DiskIoctlGetMediaTypesEx)
#pragma alloc_text(PAGE, DiskIoctlPredictFailure)
#pragma alloc_text(PAGE, DiskIoctlReassignBlocks)
#pragma alloc_text(PAGE, DiskIoctlIsWritable)
#pragma alloc_text(PAGE, DiskIoctlUpdateDriveSize)
#pragma alloc_text(PAGE, DiskIoctlGetVolumeDiskExtents)
#pragma alloc_text(PAGE, DiskIoctlSmartGetVersion)
#pragma alloc_text(PAGE, DiskIoctlSmartReceiveDriveData)
#pragma alloc_text(PAGE, DiskIoctlSmartSendDriveCommand)
#pragma alloc_text(PAGE, DiskIoctlCreateDisk)
#pragma alloc_text(PAGE, DiskIoctlGetDriveLayout)
#pragma alloc_text(PAGE, DiskIoctlGetDriveLayoutEx)
#pragma alloc_text(PAGE, DiskIoctlSetDriveLayout)
#pragma alloc_text(PAGE, DiskIoctlSetDriveLayoutEx)
#pragma alloc_text(PAGE, DiskIoctlGetPartitionInfo)
#pragma alloc_text(PAGE, DiskIoctlGetPartitionInfoEx)
#pragma alloc_text(PAGE, DiskIoctlGetLengthInfo)
#pragma alloc_text(PAGE, DiskIoctlSetPartitionInfo)
#pragma alloc_text(PAGE, DiskIoctlSetPartitionInfoEx)
#pragma alloc_text(PAGE, DiskQueryPnpCapabilities)
#pragma alloc_text(PAGE, DiskFlushDispatch)
#endif

/*++
 * @name DiskDriverReinit
 *
 * The DiskDriverReinit routine FILLMEIN
 *
 * @param DriverObject
 *        Pointer to a DRIVER_OBJECT structure. This is the driver's driver
 *        object.
 *
 * @param Context
 *        Pointer to Context information, specified in a previous call to
 *        DiskBootDriverReinit which calls IoRegisterDriverReinitialization.
 *
 * @param Count
 *        Value representing the number of times the DiskBootDriverReinit
 *        routine has been called, including the current call.
 *
 * @return None.
 *
 * @remarks Context and Count are not used.
 *
 *--*/
VOID
DiskDriverReinit(IN PDRIVER_OBJECT DriverObject,
                 IN PVOID Context,
                 IN ULONG Count)
{
    //
    // Set flag to show the driver has been reinitialized.
    //
    DiskIsPastReinitialization = TRUE;
}

/*++
 * @name DiskBootDriverReinit
 *
 * The DiskBootDriverReinit routine FILLMEIN
 *
 * @param DriverObject
 *        Pointer to a DRIVER_OBJECT structure. This is the driver's driver
 *        object.
 *
 * @param Context
 *        Pointer to Context information, to be passed to
 *        IoRegisterDriverReinitialization.
 *
 * @param Count
 *        Value representing the number of times the DiskBootDriverReinit
 *        routine has been called, including the current call.
 *
 * @return None.
 *
 * @remarks Context and Count are not used.
 *
 *--*/
VOID
DiskBootDriverReinit(IN PDRIVER_OBJECT DriverObject,
                     IN PVOID Context,
                     IN ULONG Count)
{
    PVOID Nothing = NULL;

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    //
    // Now register our reinitialize routine.
    //
    IoRegisterDriverReinitialization(DriverObject, DiskDriverReinit, NULL);
#endif

    //
    // Reinitialize the disk driver.
    // FIXME: Not needed on IA64.
    //
    DiskDriverReinitialization(DriverObject, Nothing, Count);
}

/*++
 * @name DriverEntry
 *
 * The DriverEntry routine is responsible for initializing the SCSI class HDD
 * driver (disk.sys).
 *
 * @param DriverObject
 *        Pointer to a DRIVER_OBJECT structure. This is the driver's driver
 *        object.
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
    CLASS_INIT_DATA InitData = {0};
    CLASS_QUERY_WMI_REGINFO_EX_LIST WmiRegInfoList = {0};
    GUID GuidQueryRegistrationInfoEx = GUID_CLASSPNP_QUERY_REGINFOEX;
    NTSTATUS Status;

    //
    // Get information about the disks in this system.
    // FIXME: Not needed on IA64.
    //
    DiskSaveDetectInfo(DriverObject);

    //
    // Now we set the size of the InitData structure.
    //
    InitData.InitializationDataSize = sizeof(CLASS_INIT_DATA);

    //
    // Set FDO information.
    //
    InitData.FdoData.DeviceExtensionSize = sizeof(DISK_DATA) +
                                           sizeof(FUNCTIONAL_DEVICE_EXTENSION);
    InitData.FdoData.DeviceType = FILE_DEVICE_DISK;
    InitData.FdoData.DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN;

    //
    // Set FDO class functions
    //
    InitData.FdoData.ClassInitDevice = DiskInitFdo;
    InitData.FdoData.ClassStartDevice = DiskStartFdo;
    InitData.FdoData.ClassStopDevice = DiskStopDevice;
    InitData.FdoData.ClassRemoveDevice = DiskRemoveDevice;
    InitData.FdoData.ClassPowerDevice = ClassSpinDownPowerHandler;
    InitData.FdoData.ClassError = DiskFdoProcessError;
    InitData.FdoData.ClassReadWriteVerification = DiskReadWriteVerification;
    InitData.FdoData.ClassDeviceControl = DiskDeviceControl;
    InitData.FdoData.ClassShutdownFlush = DiskShutdownFlush;
    InitData.FdoData.ClassCreateClose = NULL;

#if (NTDDI_VERSION < NTDDI_LONGHORN)
    //
    // Set PDO information.
    //
    InitData.PdoData.DeviceExtensionSize = sizeof(DISK_DATA) +
                                           sizeof(PHYSICAL_DEVICE_EXTENSION);
    InitData.PdoData.DeviceType = FILE_DEVICE_DISK;
    InitData.PdoData.DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN;

    //
    // Set PDO class functions
    //
    InitData.PdoData.ClassInitDevice = DiskInitPdo;
    InitData.PdoData.ClassStartDevice = DiskStartPdo;
    InitData.PdoData.ClassStopDevice = DiskStopDevice;
    InitData.PdoData.ClassRemoveDevice = DiskRemoveDevice;
    InitData.PdoData.ClassPowerDevice = NULL;
    InitData.PdoData.ClassError = NULL;
    InitData.PdoData.ClassReadWriteVerification = DiskReadWriteVerification;
    InitData.PdoData.ClassDeviceControl = DiskDeviceControl;
    InitData.PdoData.ClassShutdownFlush = DiskShutdownFlush;
    InitData.PdoData.ClassCreateClose = NULL;
    InitData.PdoData.ClassQueryPnpCapabilities = DiskQueryPnpCapabilities;

    //
    // Set Generic Data
    //
    InitData.ClassQueryId = DiskQueryId;
    InitData.ClassEnumerateDevice = DiskEnumerateDevice;
#endif
    InitData.ClassAddDevice = DiskAddDevice;
    InitData.ClassUnload = DiskUnload;

    //
    // Set WMI functions and information.
    //
    InitData.FdoData.ClassWmiInfo.GuidCount = 7;
    InitData.FdoData.ClassWmiInfo.GuidRegInfo =
        DiskWmiFdoGuidList;
    InitData.FdoData.ClassWmiInfo.ClassQueryWmiRegInfo =
        DiskFdoQueryWmiRegInfo;
    InitData.FdoData.ClassWmiInfo.ClassQueryWmiDataBlock =
        DiskFdoQueryWmiDataBlock;
    InitData.FdoData.ClassWmiInfo.ClassSetWmiDataBlock =
        DiskFdoSetWmiDataBlock;
    InitData.FdoData.ClassWmiInfo.ClassSetWmiDataItem =
        DiskFdoSetWmiDataItem;
    InitData.FdoData.ClassWmiInfo.ClassExecuteWmiMethod =
        DiskFdoExecuteWmiMethod;
    InitData.FdoData.ClassWmiInfo.ClassWmiFunctionControl =
        DiskWmiFunctionControl;

    //
    // Initialize reregistration structures.
    //
    DiskInitializeReregistration();

    //
    // Initialize the driver.
    //
    Status = ClassInitialize(DriverObject, RegistryPath, &InitData);

    //
    // If initialization was successful register DiskBootDriverInit as our
    // reinitialization routine.
    //
    if (NT_SUCCESS(Status))
    {
        IoRegisterBootDriverReinitialization(DriverObject,
                                             DiskBootDriverReinit,
                                             NULL);
    }

    //
    // Setup the ClassQueryWmiRegistrationInfoExList structure for a call to
    // ClassInitializeEx so we can register DiskFdoQueryWmiRegInfoEx as a
    // PCLASS_QUERY_WMI_REGINFO_EX routine.
    //
    WmiRegInfoList.Size = sizeof(CLASS_QUERY_WMI_REGINFO_EX_LIST);
    WmiRegInfoList.ClassFdoQueryWmiRegInfoEx = DiskFdoQueryWmiRegInfoEx;

    //
    // Now register the DiskFdoQueryWmiRegInfoEx routine.
    //
    ClassInitializeEx(DriverObject,
                      &GuidQueryRegistrationInfoEx,
                      &WmiRegInfoList);

    return Status;
}

VOID
DiskUnload(IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    //
    // Perform cleanup on the data structure built by the DiskSaveDetectInfo
    // routine.
    // FIXME: Not needed on IA64.
    //
    DiskCleanupDetectInfo(DriverObject);
}

NTSTATUS
DiskCreateFdo(IN PDRIVER_OBJECT DriverObject,
              IN PDEVICE_OBJECT PhysicalDeviceObject,
              IN PULONG DeviceCount,
              IN BOOLEAN DisallowMount)
{
    PUCHAR DeviceName = NULL;
    HANDLE Handle = NULL;
    PDEVICE_OBJECT LowerDevice = NULL, DeviceObject = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    WCHAR DirectoryBuffer[64] = {0};
    UNICODE_STRING DirectoryName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Set DeviceCount to 0 so we start with 0 devices at the beginning.
    //
    *DeviceCount = 0;

    //
    // Start looping.
    //
    do
    {
        //
        // Set the device name for this device.
        //
        Status = RtlStringCchPrintfW(DirectoryBuffer,
                                     sizeof(DirectoryBuffer) /
                                     (sizeof(DirectoryBuffer[0]) - 1),
                                     L"\\Device\\Harddisk%d",
                                     *DeviceCount);

        //
        // Check for a problem with getting the device name.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // Fail and print the error code.
            //
            DbgPrint("DiskCreateFdo: Format symbolic link failed: 0x%X\n",
                     Status);
            return Status;
        }

        //
        // Initialize the unicode string for the directory.
        //
        RtlInitUnicodeString(&DirectoryName, DirectoryBuffer);

        //
        // Initialize the object attributes for the new directory.
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DirectoryName,
                                   OBJ_CASE_INSENSITIVE |
                                   OBJ_PERMANENT |
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        //
        // Create the directory object.
        //
        Status = ZwCreateDirectoryObject(&Handle,
                                         DIRECTORY_ALL_ACCESS,
                                         &ObjectAttributes);

        //
        // Increment the device count so we move on to the next device.
        //
        (*DeviceCount)++;

      //
      // Continue looping until we have a unique name.
      //
    } while((Status == STATUS_OBJECT_NAME_COLLISION) ||
            (Status == STATUS_OBJECT_NAME_EXISTS));

    //
    // Check for successful creation of the directory.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail and print the error code.
        //
        DbgPrint("DiskCreateFdo: Could not create directory - %lx\n", Status);
        return Status;
    }

    //
    // Set the device count to reflect the actual number of devices which is
    // one less than the total loop count.
    //
    (*DeviceCount)--;

    //
    // Get the attached device and then call the class driver to claim it.
    //
    LowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);
    Status = ClassClaimDevice(LowerDevice, FALSE);

    //
    // Verify the device claim was successful.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // The claim failed so we make the handle temporary so we can close it.
        //
        ZwMakeTemporaryObject(Handle);
        ZwClose(Handle);

        //
        // Dereference the attached device and return the error code.
        //
        ObDereferenceObject(LowerDevice);
        return Status;
    }

    //
    // Generate a name for the device object.
    //
    Status = DiskGenerateDeviceName(
#if (NTDDI_VERSION < NTDDI_LONGHORN)
                                    TRUE,
#endif
                                    *DeviceCount,
#if (NTDDI_VERSION < NTDDI_LONGHORN)
                                    0,
                                    NULL,
                                    NULL,
#endif
                                    &DeviceName);

    //
    // Check if there was a problem generating a name.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Name generation failed so we print the error code and jump to Exit.
        //
        DbgPrint("DiskCreateFdo - couldn't create name %lx\n", Status);
        goto Exit;
    }

    //
    // Call the class driver to create the device object.
    //
    Status = ClassCreateDeviceObject(DriverObject,
                                     DeviceName,
                                     PhysicalDeviceObject,
                                     TRUE,
                                     &DeviceObject);

    //
    // Check if there was a problem creating the device object.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // The device object creation failed so we print the error code and
        // jump to Exit.
        //
        DbgPrint("DiskCreateFdo: Can not create device object %s\n", DeviceName);
        goto Exit;
    }

    //
    // Check if there was a device name.
    //
    if (DeviceName)
    {
        //
        // There was one so we free the device name and then ensure we
        // don’t use it again.
        //
        ExFreePool(DeviceName);
        DeviceName = NULL;
    }

    //
    // Set this device to use Direct I/O.
    //
    DeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Get the device extension for this device.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Check if mounts are allowed (except for RAW devices).
    //
    if (DisallowMount) DeviceObject->Vpb->Flags |= VPB_RAW_MOUNT;

    //
    // Initialize the lock count and save the current device count.
    //
    DeviceExtension->LockCount = 0;
    DeviceExtension->DeviceNumber = *DeviceCount;

    //
    // Set the alignment requirements for the device object according to the
    // host adaptor's requirements.
    //
    if (LowerDevice->AlignmentRequirement > DeviceObject->AlignmentRequirement)
    {
        DeviceObject->AlignmentRequirement = LowerDevice->AlignmentRequirement;
    }

    //
    // Save the Phyical Device Object and attach to the device stack.
    //
    DeviceExtension->LowerPdo = PhysicalDeviceObject;
    DeviceExtension->CommonExtension.LowerDeviceObject =
        IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

    //
    // Check if there was a problem attaching to the Physical Device Object.
    //
    if (!DeviceExtension->CommonExtension.LowerDeviceObject)
    {
        //
        // There was a problem attaching so we fail the request and
        // jump to Exit.
        //
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

#if (NTDDI_VERSION < NTDDI_LONGHORN)
    //
    // Initialize the partition event
    //
    KeInitializeEvent(&((PDISK_DATA)DeviceExtension->CommonExtension.
                      DriverData)->PartitioningEvent,
                      SynchronizationEvent,
                      TRUE);
#endif

    //
    // Clear the device initializing flag.
    //
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Save the handle to our device directory.
    //
    DeviceExtension->DeviceDirectory = Handle;

    //
    // Dereference the attached device and return to caller.
    //
    ObDereferenceObject(LowerDevice);
    return STATUS_SUCCESS;

Exit:
    //
    // Delete the device if one exists.
    //
    if (DeviceObject) IoDeleteDevice(DeviceObject);

    //
    // Free the device name if one exists.
    //
    if (DeviceName)
    {
        ExFreePool(DeviceName);
        DeviceName = NULL;
    }

    //
    // Dereference the attached device.
    //
    ObDereferenceObject(LowerDevice);

    //
    // Make the handle temporary so we can close it.
    //
    ZwMakeTemporaryObject(Handle);
    ZwClose(Handle);

    //
    // Return the error code.
    //
    return Status;
}

NTSTATUS
DiskReadWriteVerification(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    ULONG ResidualBytes;
    ULONGLONG BytesRemaining;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the number of bytes remaining in the request.
    //
    BytesRemaining = CommonExtension->PartitionLength.QuadPart -
                     StackLocation->Parameters.Read.ByteOffset.QuadPart;

    //
    // Set ResidualBytes to indicate if there are residual bytes or not.
    //
    ResidualBytes = StackLocation->Parameters.Read.Length &
                    (CommonExtension->PartitionZeroExtension->
                    DiskGeometry.BytesPerSector - 1);
    //
    // Check that the request is within the partition boundaries and
    // that the number of bytes is a multiple of the sector size.
    //
    if ((StackLocation->Parameters.Read.ByteOffset.QuadPart >
        CommonExtension->PartitionLength.QuadPart) ||
        (StackLocation->Parameters.Read.ByteOffset.QuadPart < 0) ||
        ResidualBytes)
    {
        //
        // It isn't so mark the request as invaild.
        //
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        //
        // Verify that there arn't to many bytes being read.
        //
        if ((ULONGLONG)StackLocation->Parameters.Read.Length > BytesRemaining)
        {
            //
            // There are so mark the request as invalid.
            //
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Check if we have hit any errors so far.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Get the drives ready status.
        //
        Status = ((PDISK_DATA)CommonExtension->DriverData)->ReadyStatus;

        //
        // Check if the drive was ready or not.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // It wasn't so we print the error code.
            //
            DbgPrint("DiskReadWriteVerification: ReadyStatus is %lx\n", Status);

            //
            // Check if the pointer to the thread exists.
            //
            if (Irp->Tail.Overlay.Thread)
            {
                //
                // It does so call IoSetHardErrorOrVerifyDevice so the file
                // system driver can present a popup to the user.
                //
                IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
            }

            //
            // Ensure the error isn't caused by insufficient resources.
            //
            ASSERT(Status != STATUS_INSUFFICIENT_RESOURCES);
        }
        else if (!ResidualBytes)
        {
            //
            // It is likely that the physical disk is too small, so we pass
            // it down to the hardware.
            //
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // Set status to invalid parameter so fastfat can determine when to
            // remount due to a sector size change.
            //
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Save the IRPs I/O status and return it.
    //
    Irp->IoStatus.Status = Status;
    return Status;
}

NTSTATUS
DiskDeviceControl(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION  StackLocation;
    ULONG IoControlCode;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Ensure we have a device object before moving on.
    //
    ASSERT(DeviceObject != NULL);

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Clear the IRPs I/O status information.
    //
    Irp->IoStatus.Information = 0;

    //
    // Get the disk device's I/O control code.
    //
    IoControlCode = StackLocation->Parameters.DeviceIoControl.IoControlCode;

    //
    // State that we have recieved the disk device's I/O control code.
    //
    DbgPrint("DiskDeviceControl: Received IOCTL 0x%X for device "
             "%p through IRP %p\n",
             IoControlCode, DeviceObject, Irp);

    //
    // Now we find out which I/O control code we have and take
    // the appropriate course of action.
    //
    switch (IoControlCode)
    {
        //
        // Do we want the disk device's cache configuration data?
        //
        case IOCTL_DISK_GET_CACHE_INFORMATION:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // For FDOs, get the disk device's cache configuration
            // data.
            //
            Status = DiskIoctlGetCacheInformation(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to set the disk device's cache configuration data?
        //
        case IOCTL_DISK_SET_CACHE_INFORMATION:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Set the disk device's cache configuration data
            // and break out of the switch.
            //
            Status = DiskIoctlSetCacheInformation(DeviceObject, Irp);
            break;
        }

        //
        // Do we want the disk device's cache settings?
        //
        case IOCTL_DISK_GET_CACHE_SETTING:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Get the disk device's cache settings.
            //
            Status = DiskIoctlGetCacheSetting(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to set the disk device's cache settings?
        //
        case IOCTL_DISK_SET_CACHE_SETTING:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Set the disk device's cache settings.
            //
            Status = DiskIoctlSetCacheSetting(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get the disk device's geometry?
        //
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the disk device's geometry and
            // break out of the switch.
            //
            Status = DiskIoctlGetDriveGeometry(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get the disk device's geometry (supports both MBR and
        // GPT partitioned media?
        //
        case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the disk device's geometry and
            // break out of the switch.
            //
            Status = DiskIoctlGetDriveGeometryEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to verify a logical section of the disk?
        //
        case IOCTL_DISK_VERIFY:
        {
            //
            // Yes so we perform the verification and break out
            // of the switch.
            //
            Status = DiskIoctlVerify(DeviceObject, Irp);
            break;
        }

        //
        // Do we want length (in bytes) of the volume, partition or disk?
        //
        case IOCTL_DISK_GET_LENGTH_INFO:
        {
            //
            // Yes so get the length and break out of the switch.
            //
            Status = DiskIoctlGetLengthInfo(DeviceObject, Irp);
            break;
        }

        //
        // Do we want know if the disk is writable?
        //
        case IOCTL_DISK_IS_WRITABLE:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we find out and break out of the switch.
            //
            Status = DiskIoctlIsWritable(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to update the device extension with drive
        // size information for current media?
        //
        case IOCTL_DISK_UPDATE_DRIVE_SIZE:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we update the size information and break
            // out of the switch.
            //
            Status = DiskIoctlUpdateDriveSize(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to map defective blocks to new location on
        // the disk (4-byte LBA)?
        //
        case IOCTL_DISK_REASSIGN_BLOCKS:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we reassign the blocks and break out of the
            // switch.
            //
            Status = DiskIoctlReassignBlocks(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to map defective blocks to new location on
        // the disk (8-byte LBA)?
        //
        case IOCTL_DISK_REASSIGN_BLOCKS_EX:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we reassign the blocks and break out of the
            // switch.
            //
            Status = DiskIoctlReassignBlocksEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to set the verify bit on the disk device
        // object?
        //
        case IOCTL_DISK_INTERNAL_SET_VERIFY:
        {
            //
            // Yes so we set the verify bit and break out of the
            // switch.
            //
            Status = DiskIoctlSetVerify(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to clear the verify bit on the disk device
        // object?
        //
        case IOCTL_DISK_INTERNAL_CLEAR_VERIFY:
        {
            //
            // Yes so we clear the verify bit and break out of the
            // switch.
            //
            Status = DiskIoctlClearVerify(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get the types of media supported by the
        // device?
        //
        case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the media types and break out of the
            // switch.
            //
            Status = DiskIoctlGetMediaTypesEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to poll for device failure (for drives that
        // support SMART or the SCSI equivalent)?
        //
        case IOCTL_STORAGE_PREDICT_FAILURE:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we poll for failure and break out of the
            // switch.
            //
            Status = DiskIoctlPredictFailure(DeviceObject, Irp);
            break;
        }

        //
        // Do we want version information about SMART on the drive (for
        // drives that support SMART)?
        //
        case SMART_GET_VERSION:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the version information and break out
            // of the switch.
            //
            Status = DiskIoctlSmartGetVersion(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get information about SMART on the drive (for
        // drives that support SMART)?
        //
        case SMART_RCV_DRIVE_DATA:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the information and break out of the
            // switch.
            //
            Status = DiskIoctlSmartReceiveDriveData(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to send a SMART command to the drive (for
        // drives that support SMART)?
        //
        case SMART_SEND_DRIVE_COMMAND:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we send the command and break out of the
            // switch.
            //
            Status = DiskIoctlSmartSendDriveCommand(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get the physical location(s) of a volume?
        //
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
            //
            // Fall through and let the next case handle this.
            //

        //
        // Do we want to get the physical location(s) of a volume?
        //
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN:
        {
            //
            // Yes so we get the location(s) and break out of the
            // switch.
            //
            Status = DiskIoctlGetVolumeDiskExtents(DeviceObject, Irp);
            break;
        }

#if (NTDDI_VERSION < NTDDI_LONGHORN)
        //
        // Do we want to create an empty partition?
        //
        case IOCTL_DISK_CREATE_DISK:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we create the empty partition and break out
            // of the switch.
            //
            Status = DiskIoctlCreateDisk(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get the layout of the drive (obsolete)?
        //
        case IOCTL_DISK_GET_DRIVE_LAYOUT:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the drive's layout and break out
            // of the switch.
            //
            Status = DiskIoctlGetDriveLayout(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to get the layout of the drive (extended,
        // supports MBR and GPT style partition tables)?
        //
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we get the drive's layout and break out
            // of the switch.
            //
            Status = DiskIoctlGetDriveLayoutEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to repartition the disk (obsolete)?
        //
        case IOCTL_DISK_SET_DRIVE_LAYOUT:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we repartition the disk and break out
            // of the switch.
            //
            Status = DiskIoctlSetDriveLayout(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to repartition the disk (extended, supports
        // MBR and GPT style partition tables)?
        //
        case IOCTL_DISK_SET_DRIVE_LAYOUT_EX:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we repartition the disk and break out
            // of the switch.
            //
            Status = DiskIoctlSetDriveLayoutEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to delete the partition table?
        //
        case IOCTL_DISK_DELETE_DRIVE_LAYOUT:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we delete the partition table and break out
            // of the switch.
            //
            Status = DiskIoctlDeleteDriveLayout(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to read the partition table (obsolete)?
        //
        case IOCTL_DISK_GET_PARTITION_INFO:
        {
            //
            // Yes so we read the partition table and break out
            // of the switch.
            //
            Status = DiskIoctlGetPartitionInfo(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to read the partition table (extended, supports
        // MBR and GPT style partition tables)?
        //
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
        {
            //
            // Yes so we read the partition table and break out
            // of the switch.
            //
            Status = DiskIoctlGetPartitionInfoEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to write the partition table (obsolete)?
        //
        case IOCTL_DISK_SET_PARTITION_INFO:
        {
            //
            // Yes so we write the partition table and break out
            // of the switch.
            //
            Status = DiskIoctlSetPartitionInfo(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to write the partition table (extended, supports
        // MBR and GPT style partition tables)?
        //
        case IOCTL_DISK_SET_PARTITION_INFO_EX:
        {
            //
            // Yes so we write the partition table and break out
            // of the switch.
            //
            Status = DiskIoctlSetPartitionInfoEx(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to increase the size of an existing partition?
        //
        case IOCTL_DISK_GROW_PARTITION:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we grow the partition and break out
            // of the switch.
            //
            Status = DiskIoctlGrowPartition(DeviceObject, Irp);
            break;
        }

        //
        // Do we want to reread the partition table (from the disk
        // invalidating the cached copy)?
        //
        case IOCTL_DISK_UPDATE_PROPERTIES:
        {
            //
            // Check if this is a PDO and handle it, if so.
            //
            if (!CommonExtension->IsFdo) goto HandlePdo;

            //
            // Yes so we reread the partition table and break out
            // of the switch.
            //
            Status = DiskIoctlUpdateProperties(DeviceObject, Irp);
            break;
        }
#endif

        default:
        {
            //
            // By default we simply call classpnp's common device
            // control routine and return.
            //
            return ClassDeviceControl(DeviceObject, Irp);
            break;
        }
    }

    //
    // Make sure there was no problem with the IOCTL operation.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskDeviceControl: IOCTL 0x%X to device %p failed "
                 "with error 0x%X\n",
                 IoControlCode, DeviceObject, Status);

        //
        // Check if this was a removable media error caused by the user.
        //
        if (IoIsErrorUserInduced(Status) && Irp->Tail.Overlay.Thread)
        {
            //
            // It is so we identify the removable media device so
            // we can promt the user to verify if the medium
            // is valid.
            //
            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        }
    }

    //
    // Check if we have a status pending status.
    //
    if (Status != STATUS_PENDING)
    {
        //
        // We don't so we save the I/O status.
        //
        Irp->IoStatus.Status = Status;

        //
        // Now we release the remove lock on and complete
        // the IRP.
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;

HandlePdo:
    //
    // Release the remove lock.
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);

    //
    // Now we copy the IRP stack parameters to, and send
    // the request to the next-lower driver.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);
    Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

    //
    // Return to the calling routine with the current status.
    //
    return Status;
}

NTSTATUS
DiskShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension;
    PDISK_DATA DiskData;
    PIO_STACK_LOCATION StackLocation;
    PDEVICE_OBJECT LowerDevice;
    PSCSI_REQUEST_BLOCK Srb;
    PLIST_ENTRY ListEntry;
    PCDB Cdb;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the functional device extension.
    //
    FdoExtension = CommonExtension->PartitionZeroExtension;

    //
    // Get the driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;
    
    //
    // Check if this is actually a PDO
    //
    if (!CommonExtension->IsFdo)
    {
        //
        // Get the lower device.
        //
        LowerDevice = CommonExtension->LowerDeviceObject;

        //
        // Release the remove lock.
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Mark the IRP as pending and pass the IRP to the lower device.
        //
        IoMarkIrpPending(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoCallDriver(LowerDevice, Irp);
        return STATUS_PENDING;
    }

    //
    // Flush requests are combined and need to be handled in a special manner
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (StackLocation->MajorFunction == IRP_MJ_FLUSH_BUFFERS)
    {
        //
        // Check for the are we plugged in flag
        //
        if (FdoExtension->DeviceFlags & DEV_POWER_PROTECTED)
        {
            //
            // Set the IRP status to success
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Release the remove lock
            //
            ClassReleaseRemoveLock(DeviceObject, Irp);

            //
            // Complete the request
            //
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        //
        // Wait for the flush mutex
        //
        DbgPrint("DiskData: %p\n", DiskData);
        DbgPrint("Mutex: %p\n", &DiskData->FlushContext.Mutex);
        DbgPrint("Mutex data: %lx\n", DiskData->FlushContext.Mutex.Header.Type);
        KeWaitForMutexObject(&DiskData->FlushContext.Mutex,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL);

        //
        // Mark as pending
        //
        IoMarkIrpPending(Irp);

        //
        // Look to see if a flush is in progress
        //
        if (DiskData->FlushContext.CurrIrp)
        {
            //
            // Check for available flush context on next IRP
            //
            if (DiskData->FlushContext.NextIrp)
            {
                //
                // Queue this request to the group that is next in line
                //
                InsertTailList(&DiskData->FlushContext.NextList,
                               &Irp->Tail.Overlay.ListEntry);

                //
                // Release the flush mutex
                //
                KeReleaseMutex(&DiskData->FlushContext.Mutex, FALSE);
            }
            else
            {
                //
                // Copy current IRP to next flush context
                //
                DiskData->FlushContext.NextIrp = Irp;

                //
                // Make sure list is empty
                //
                ASSERT(IsListEmpty(&DiskData->FlushContext.NextList));

                //
                // Release the flush mutex
                //
                KeReleaseMutex(&DiskData->FlushContext.Mutex, FALSE);

                //
                // Wait for the outstanding flush to complete
                //
                KeWaitForSingleObject(&DiskData->FlushContext.Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                //
                // Wait for the flush mutex to complete
                //
                KeWaitForMutexObject(&DiskData->FlushContext.Mutex,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     NULL);

                //
                // There is no way our current flush context list is NULL
                //
                ASSERT(IsListEmpty(&DiskData->FlushContext.CurrList));

                //
                // Make this group the outstanding one and free up the
                // next slot
                //
                while (!IsListEmpty(&DiskData->FlushContext.NextList))
                {
                    //
                    // Remove the next list from head entry
                    //
                    ListEntry =
                        RemoveHeadList(&DiskData->FlushContext.NextList);

                    //
                    // Insert the head we just removed of the list to the
                    // end of it
                    //
                    InsertTailList(&DiskData->FlushContext.CurrList, ListEntry);
                }

                //
                // Point the current IRP to the next one
                //
                DiskData->FlushContext.CurrIrp = DiskData->FlushContext.NextIrp;

                //
                // Point next IRP to NULL
                //
                DiskData->FlushContext.NextIrp = NULL;

                //
                // Release the mutex object
                //
                KeReleaseMutex(&DiskData->FlushContext.Mutex, FALSE);

                //
                // Send this request down to the device
                //
                DiskFlushDispatch(DeviceObject, &DiskData->FlushContext);
            }
        }
        else
        {
            //
            // Handle the IRP we recieved for the device
            //
            DiskData->FlushContext.CurrIrp = Irp;

            //
            // Make sure we have proper
            //
            ASSERT(IsListEmpty(&DiskData->FlushContext.CurrList));
            ASSERT(DiskData->FlushContext.NextIrp == NULL);
            ASSERT(IsListEmpty(&DiskData->FlushContext.NextList));

            KeReleaseMutex(&DiskData->FlushContext.Mutex, FALSE);

            //
            // Send this request down to the device
            //
            DiskFlushDispatch(DeviceObject, &DiskData->FlushContext);
        }
    }
    else
    {
        //
        // Allocate SCSI request block
        //
        Srb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK),
                                    DISK_TAG_SRB);

        //
        // Check for failed allocation
        //
        if (!Srb)
        {
            //
            // Set the status, release the remove lock, and complete the request
            //
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Zero out the SRB
        //
        RtlZeroMemory(Srb, SCSI_REQUEST_BLOCK_SIZE);

        //
        // Write the length to SRB
        //
        Srb->Length = SCSI_REQUEST_BLOCK_SIZE;

        //
        // Set the timeout value
        //
        Srb->TimeOutValue = FdoExtension->TimeOutValue * 4;

        //
        // Mark the request as not being a tagged request
        //
        Srb->QueueTag = SP_UNTAGGED;
        Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

        //
        // Set the SRB flags that our extention has
        //
        Srb->SrbFlags = FdoExtension->SrbFlags;

        //
        // If the write cache is enabled
        //
        if (FdoExtension->DeviceFlags & DEV_WRITE_CACHE)
        {
            //
            // Set the SRB function
            //
            Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

            //
            // Set the Cdb length and operation code
            //
            Srb->CdbLength = 10;
            Srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;

            //
            // Send the synchronize cache request
            //
            Status = ClassSendSrbSynchronous(DeviceObject,
                                             Srb,
                                             NULL,
                                             0,
                                             TRUE);

            //
            // Anounce synchronize event sent
            //
            DbgPrint("DiskShutdownFlush: Synchonize cache sent. Status = %lx\n",
                     Status);
        }

        //
        // If the device contains removable media
        //
        if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
        {
            //
            // Set the SRB length
            //
            Srb->CdbLength = 6;

            //
            // Get the CDB
            //
            Cdb = (PVOID)Srb->Cdb;

            //
            // Unlock the device
            //
            Cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            Cdb->MEDIA_REMOVAL.Prevent = FALSE;

            //
            // Set timeout value
            //
            Srb->TimeOutValue = FdoExtension->TimeOutValue;

            //
            // Send the unlock request
            //
            Status = ClassSendSrbSynchronous(DeviceObject,
                                             Srb,
                                             NULL,
                                             0,
                                             TRUE);

            //
            // Anounce unlock request sent
            //
            DbgPrint("DiskShutdownFlush: Unlock device request sent. "
                     "Status = %lx\n",
                     Status);
        }

        Srb->CdbLength = 0;

        //
        // Set shutdown function flag
        //
        Srb->Function = SRB_FUNCTION_SHUTDOWN;

        //
        // Set the retry count to zero
        //
        StackLocation->Parameters.Others.Argument4 = (PVOID)0;

        //
        // Register our completion routine.
        //
        IoSetCompletionRoutine(Irp,
                               ClassIoComplete,
                               Srb,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Get next stack location and set major function code
        //
        StackLocation = IoGetNextIrpStackLocation(Irp);
        StackLocation->MajorFunction = IRP_MJ_SCSI;

        //
        // Save SRB address in next stack for port driver.
        //
        StackLocation->Parameters.Scsi.Srb = Srb;

        //
        // Set the Irp
        //
        Srb->OriginalRequest = Irp;

        //
        // Mark the IRP pending
        //
        IoMarkIrpPending(Irp);

        //
        // Call the port driver to process the request
        //
        IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
    }

    return STATUS_PENDING;
}

NTSTATUS
DiskFlushComplete(IN PDEVICE_OBJECT Fdo,
                  IN PIRP Irp,
                  IN PVOID Context)
{
    PDISK_GROUP_CONTEXT FlushContext = Context;
    PLIST_ENTRY ListEntry;
    PIRP TempIrp;
    NTSTATUS Status;

    //
    // Anounce we where called
    //
    DbgPrint("DiskFlushComplete: %p %p %p\n", Fdo, Irp, FlushContext);

    //
    // Make sure everything is in order
    //
    ASSERT(Irp == FlushContext->CurrIrp);

    //
    // Complete the IO request
    //
    Status = ClassIoComplete(Fdo, Irp, &FlushContext->Srb);

    //
    // Make sure that ClassIoComplete did not decide to retry this request
    //
    ASSERT(Status != STATUS_MORE_PROCESSING_REQUIRED);

    //
    // Complete the flush requests tagged to this one
    //
    while (!IsListEmpty(&FlushContext->CurrList))
    {
        //
        // Get the list head
        //
        ListEntry = RemoveHeadList(&FlushContext->CurrList);

        //
        // Get the base address of the IRP
        //
        TempIrp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);

        //
        // Initialize the temporaty IRPs List of our flush context
        //
        InitializeListHead(&TempIrp->Tail.Overlay.ListEntry);

        //
        // Give our temporary IRP the IO status  block of the real IRP
        //
        TempIrp->IoStatus = Irp->IoStatus;

        //
        // Release the remove lock and complete the TempIrp
        //
        ClassReleaseRemoveLock(Fdo, TempIrp);
        ClassCompleteRequest(Fdo, TempIrp, IO_NO_INCREMENT);
    }

    //
    // Notify the next group's representative that it may go ahead now
    //
    KeSetEvent(&FlushContext->Event,
               IO_NO_INCREMENT,
               FALSE);

    return Status;
}

VOID
DiskFlushDispatch(IN PDEVICE_OBJECT Fdo,
                  IN PDISK_GROUP_CONTEXT FlushContext)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PIO_STACK_LOCATION StackLocation = NULL;
    PAGED_CODE();

    //
    // Get the functional device extension.
    //
    FdoExtension = Fdo->DeviceExtension;

    //
    // Get the SCSI Request Block.
    //
    Srb = &FlushContext->Srb;

    //
    // Zero out the memory
    //
    RtlZeroMemory(Srb, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Fill in the srb fields appropriately
    //
    Srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    Srb->TimeOutValue = FdoExtension->TimeOutValue * 4;
    Srb->QueueTag = SP_UNTAGGED;
    Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    Srb->SrbFlags = FdoExtension->SrbFlags;

    //
    // If write caching is enabled
    //
    if (FdoExtension->DeviceFlags & DEV_WRITE_CACHE)
    {
        //
        // Set the flags for a synchronize cache request
        //
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb->CdbLength = 10;
        Srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;

        //
        // Send down the synchronize cache request
        //
        ClassSendSrbSynchronous(Fdo,
                                Srb,
                                NULL,
                                0,
                                TRUE);
    }

    //
    // Set up the SRB for flushing, make sure no one free the SRB
    //
    Srb->Function = SRB_FUNCTION_FLUSH;
    Srb->CdbLength = 0;
    Srb->OriginalRequest = FlushContext->CurrIrp;
    Srb->SrbFlags |= SRB_CLASS_FLAGS_PERSISTANT;

    //
    // Make sure that this request does not get retried
    //
    StackLocation = IoGetCurrentIrpStackLocation(FlushContext->CurrIrp);
    StackLocation->Parameters.Others.Argument4 = (PVOID)0;

    //
    // Setup our stack location appropriately
    //
    StackLocation = IoGetNextIrpStackLocation(FlushContext->CurrIrp);
    StackLocation->MajorFunction = IRP_MJ_SCSI;
    StackLocation->Parameters.Scsi.Srb = Srb;

    //
    // Set the flush routine for our flush context
    //
    IoSetCompletionRoutine(FlushContext->CurrIrp,
                           DiskFlushComplete,
                           FlushContext,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Send down the flush request
    //
    IoCallDriver(((PCOMMON_DEVICE_EXTENSION)FdoExtension)->LowerDeviceObject,
                 FlushContext->CurrIrp);
}

VOID
DisableWriteCache(IN PDEVICE_OBJECT Fdo,
                  IN PIO_WORKITEM WorkItem)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}

VOID
DiskFdoProcessError(PDEVICE_OBJECT DeviceObject,
                    PSCSI_REQUEST_BLOCK ScsiRequestBlock,
                    NTSTATUS *Status,
                    BOOLEAN *Retry)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT PhysicalDeviceObject;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PIO_ERROR_LOG_PACKET LogEntry = NULL;
#endif
    PIO_WORKITEM IoWorkItem;
    PSENSE_DATA SenseData;
    PDISK_DATA DiskData;
    PCDB CommandBlock;
    ULONG SenseKey, AdditionalSenseCode, AdditionalSenseCodeQualifier;
    BOOLEAN InvalidatePartitionTable, ForceBusRelations = FALSE;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Ensure we are dealing with a functional device object.
    //
    ASSERT(DeviceExtension->CommonExtension.IsFdo);

    //
    // Get the command descriptor block from the SCSI request block.
    //
    CommandBlock = (PCDB)ScsiRequestBlock->Cdb;

    //
    // Check if we have a data overrun and this operation code is
    // a SCSI read/write command.
    //
    if ((*Status == STATUS_DATA_OVERRUN) &&
        (IS_SCSIOP_READWRITE(CommandBlock->CDB10.OperationCode)))
    {
        //
        // We do so we indicate that the request should be retried
        // and increment the device's error count.
        //
        *Retry = TRUE;
        DeviceExtension->ErrorCount++;
    }
    //
    // Now we check if the SCSI request block's SRB status is error
    // and the SCSI status is busy.
    //
    else if ((SRB_STATUS(ScsiRequestBlock->SrbStatus) == SRB_STATUS_ERROR) &&
             (ScsiRequestBlock->ScsiStatus == SCSISTAT_BUSY))
    {
        //
        // It is so we reset the SCSI bus to try and clear up the
        // condition and increment the device's error count.
        //
        ResetBus(DeviceObject);
        DeviceExtension->ErrorCount++;
    }
    else
    {
        //
        // Otherwise, we check to see if the sense info buffer data is
        // valid and it's size is correct.
        //
        if ((ScsiRequestBlock->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
            (ScsiRequestBlock->SenseInfoBufferLength >=
            FIELD_OFFSET(SENSE_DATA, CommandSpecificInformation)))
        {
            //
            // It is so we get the sense data, sense key, additional
            // sense code and the additional sense code qualifier.
            //
            SenseData = ScsiRequestBlock->SenseInfoBuffer;
            SenseKey = SenseData->SenseKey & 0xf;
            AdditionalSenseCode = SenseData->AdditionalSenseCode;
            AdditionalSenseCodeQualifier = SenseData->AdditionalSenseCodeQualifier;

            //
            // Now we find out which sense key we have and take the
            // appropriate course of action.
            //
            switch (SenseKey)
            {
                //
                // Is this an invalid logical block address or an
                // unsupported command?
                //
                case SCSI_SENSE_ILLEGAL_REQUEST:
                {
                    //
                    // It is so we check the additional sense code and
                    // take the appropiate course of action.
                    //
                    switch (AdditionalSenseCode)
                    {
#if (NTDDI_VERSION < NTDDI_LONGHORN)
                        //
                        // Is this an ivalid block address?
                        //
                        case SCSI_ADSENSE_ILLEGAL_BLOCK:
                        {
                            //
                            // It is so we check if this is a SCSI
                            // read/write command.
                            //
                            if (IS_SCSIOP_READWRITE(CommandBlock->
                                                    CDB10.OperationCode))
                            {
                                //
                                // It is so we indicate that the request
                                // should be retried.
                                //
                                *Retry = TRUE;
                            }

                            //
                            // Break out of the switch.
                            //
                            break;
                        }
#endif
                        //
                        // Is this an unsupported command?
                        //
                        case SCSI_ADSENSE_INVALID_CDB:
                        {
                            //
                            // It is so we check to see if this a SCSI write
                            // command and the forced unit access flag is
                            // set.
                            //
                            if (((CommandBlock->CDB10.OperationCode ==
                                 SCSIOP_WRITE) ||
                                 (CommandBlock->CDB10.OperationCode ==
                                 SCSIOP_WRITE16)) &&
                                (CommandBlock->CDB10.ForceUnitAccess))
                            {
                                //
                                // It is so we get the driver specific driver
                                // data.
                                //
                                DiskData =
                                    (PDISK_DATA)DeviceExtension->
                                    CommonExtension.DriverData;

                                //
                                // Check if the user has specified that write
                                // caching should be enabled.
                                //
                                if (DiskData->WriteCacheOverride ==
                                    DiskWriteCacheEnable)
                                {
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
                                    //
                                    // The user has so we allocate memory for a
                                    // log entry.
                                    //
                                    LogEntry =
                                        IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                                (sizeof(IO_ERROR_LOG_PACKET) +
                                                                (4 * sizeof(ULONG))));

                                    //
                                    // Make sure there wasn't a problem
                                    // allocating memory for the log entry.
                                    //
                                    if (LogEntry)
                                    {
                                        //
                                        // There wasn't so we fill out the log
                                        // entry indicating that writes with
                                        // forced unit access enabled are not
                                        // supported and the user should
                                        // disable write cache.
                                        //
                                        LogEntry->FinalStatus = *Status;
                                        LogEntry->ErrorCode =
                                            IO_WARNING_WRITE_FUA_PROBLEM;
                                        LogEntry->SequenceNumber = 0;
                                        LogEntry->MajorFunctionCode =
                                            IRP_MJ_SCSI;
                                        LogEntry->IoControlCode = 0;
                                        LogEntry->RetryCount = 0;
                                        LogEntry->UniqueErrorValue = 0;
                                        LogEntry->DumpDataSize =
                                            (4 * sizeof(ULONG));

                                        //
                                        // Fill in the disk data for the error
                                        // log.
                                        //
                                        LogEntry->DumpData[0] =
                                            DiskData->ScsiAddress.PortNumber;
                                        LogEntry->DumpData[1] =
                                            DiskData->ScsiAddress.PathId;
                                        LogEntry->DumpData[2] =
                                            DiskData->ScsiAddress.TargetId;
                                        LogEntry->DumpData[3] =
                                            DiskData->ScsiAddress.Lun;

                                        //
                                        // Now we write the log entry.
                                        //
                                        IoWriteErrorLogEntry(LogEntry);
                                    }
#endif
                                }
                                else
                                {
                                    //
                                    // Otherwise, we allocate memory for a
                                    // work item.
                                    //
                                    IoWorkItem =
                                        IoAllocateWorkItem(DeviceObject);

                                    //
                                    // Make sure there wasn't a problem
                                    // allocating the memory for the
                                    // work item.
                                    //
                                    if (IoWorkItem)
                                    {
                                        //
                                        // Now we queue the work item to
                                        // disable the write cache.
                                        //
                                        IoQueueWorkItem(IoWorkItem,
                                                        DisableWriteCache,
                                                        CriticalWorkQueue,
                                                        IoWorkItem);
                                    }
                                }

                                //
                                // Set the flag to indicate that forced unit
                                // access is not supported.
                                //
                                DeviceExtension->ScanForSpecialFlags |=
                                    CLASS_SPECIAL_FUA_NOT_SUPPORTED;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
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
#endif
                                //
                                // Indicate that the request should be retried
                                // and make sure forced unit access is off.
                                //
                                *Retry = TRUE;
                                CommandBlock->CDB10.ForceUnitAccess = FALSE;
                            }

                            //
                            // Break out of the switch.
                            //
                            break;
                        }
                    }

                    //
                    // Break out of the switch.
                    //
                    break;
                }

                //
                // Is the device not ready?
                //
                case SCSI_SENSE_NOT_READY:
                {
                    //
                    // It is so we check the additional sense code and
                    // take the appropiate course of action.
                    //
                    switch (AdditionalSenseCode)
                    {
                        //
                        // Is the logical unit number not ready?
                        //
                        case SCSI_ADSENSE_LUN_NOT_READY:
                        {
                            //
                            // It isn't so we check the additional sense
                            // code qualifier and take the appropriate
                            // course of action.
                            //
                            switch (AdditionalSenseCodeQualifier)
                            {
                                //
                                // Is it becoming ready?
                                //
                                case SCSI_SENSEQ_BECOMING_READY:
                                    //
                                    // Fall through and let the next case
                                    // handle this.
                                    //

                                //
                                // Is manual intervention required?
                                //
                                case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED:
                                    //
                                    // Fall through and let the next case
                                    // handle this.
                                    //

                                //
                                // Is the cause unknown/unreportable?
                                //
                                case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE:
                                {
                                    //
                                    // It is so indicate that the partition
                                    // table should be invalidated and
                                    // break out of the switch.
                                    //
                                    InvalidatePartitionTable = TRUE;
                                    break;
                                }
                            }

                            //
                            // Break out of the switch.
                            //
                            break;
                        }

                        //
                        // Is there no media in the device?
                        //
                        case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE:
                        {
                            //
                            // There isn't so we indicate that the partition
                            // table should be invalidated and break out of
                            // the switch.
                            //
                            InvalidatePartitionTable = TRUE;
                            break;
                        }
                    }

                    //
                    // Break out of the switch.
                    //
                    break;
                }

                //
                // Is this a medium error (unrecovered read error)?
                //
                case SCSI_SENSE_MEDIUM_ERROR:
                {
                    //
                    // It is so indicate that the partition table should
                    // be invalidated and break out of the switch.
                    //
                    InvalidatePartitionTable = TRUE;
                    break;
                }

                //
                // Is this a hardware error (unrecovered read error)?
                //
                case SCSI_SENSE_HARDWARE_ERROR:
                {
                    //
                    // It is so indicate that the partition table should
                    // be invalidated and break out of the switch.
                    //
                    InvalidatePartitionTable = TRUE;
                    break;
                }

                //
                // Is this a media change or unit reset?
                //
                case SCSI_SENSE_UNIT_ATTENTION:
                {
                    //
                    // It is so we indicate that the partition table should
                    // be invalidated.
                    //
                    InvalidatePartitionTable = TRUE;

                    //
                    // Now we check the additional sense code and take the
                    // appropiate course of action.
                    //
                    switch (SenseData->AdditionalSenseCode)
                    {
                        //
                        // Has the medium changed?
                        //
                        case SCSI_ADSENSE_MEDIUM_CHANGED:
                        {
                            //
                            // It has so we indicate that the PnP manager
                            // should be informed of the change and
                            // break out of the switch.
                            //
                            ForceBusRelations = TRUE;
                            break;
                        }
                    }

                    //
                    // Break out of the switch.
                    //
                    break;
                }

                //
                // Is this a recovered read error?
                //
                case SCSI_SENSE_RECOVERED_ERROR:
                {
                    //
                    // It is so indicate that the partition table should
                    // be invalidated and break out of the switch.
                    //
                    InvalidatePartitionTable = TRUE;
                    break;
                }
            }
        }
        else
        {
            //
            // Otherwise, we check what SCSI request block status we
            // have and take the appropriate course of action.
            //
            switch (SRB_STATUS(ScsiRequestBlock->SrbStatus))
            {
                //
                // Do we have an invalid logical unit number?
                //
                case SRB_STATUS_INVALID_LUN:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Do we have an invalid target ID?
                //
                case SRB_STATUS_INVALID_TARGET_ID:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Do we have an invalid path ID?
                //
                case SRB_STATUS_INVALID_PATH_ID:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Has the device not responded?
                //
                case SRB_STATUS_NO_DEVICE:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Has the host bus adapter not responded?
                //
                case SRB_STATUS_NO_HBA:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Has the command timed out?
                //
                case SRB_STATUS_COMMAND_TIMEOUT:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Did the request time out?
                //
                case SRB_STATUS_TIMEOUT:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Did the SCSI device selection time out?
                //
                case SRB_STATUS_SELECTION_TIMEOUT:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Was the status request stopped?
                //
                case SRB_STATUS_REQUEST_FLUSHED:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Did we have an unexpected disconnection?
                //
                case SRB_STATUS_UNEXPECTED_BUS_FREE:
                    //
                    // Fall through and let the next case handle this.
                    //

                //
                // Did we have a parity error on the SCSI bus or a
                // failed retry?
                //
                case SRB_STATUS_PARITY_ERROR:
                {
                    //
                    // We did so we indicate that the partition table should
                    // be invalidated and break out of the switch.
                    //
                    InvalidatePartitionTable = TRUE;
                    break;
                }

                //
                // Did the request complete with an error?
                //
                case SRB_STATUS_ERROR:
                {
                    //
                    // It did so we check if it was a reservation conflict.
                    //
                    if (ScsiRequestBlock->ScsiStatus ==
                        SCSISTAT_RESERVATION_CONFLICT)
                    {
                        //
                        // It was so we indicate that the partition table
                        // should be invalidated.
                        //
                        InvalidatePartitionTable = TRUE;
                    }

                    //
                    // Break out of the switch.
                    //
                    break;
                }
            }
        }

#if (NTDDI_VERSION < NTDDI_LONGHORN)
        //
        // Now we check to see if the partition table should be
        // invalidated.
        //
        if (InvalidatePartitionTable)
        {
            //
            // It should so we check if this device is removable.
            //
            if ((DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) &&
                (DeviceExtension->CommonExtension.ChildList))
            {
                //
                // It is so we get the physical device object for this
                // device.
                //
                PhysicalDeviceObject =
                    (DeviceExtension->CommonExtension.ChildList)->
                    CommonExtension.DeviceObject;

                //
                // Now we use class to make sure there is a volume parameter
                // block and that it's mounted.
                //
                if ((ClassGetVpb(PhysicalDeviceObject)) &&
                    (ClassGetVpb(PhysicalDeviceObject)->Flags & VPB_MOUNTED))
                {
                    //
                    // It is so we indicate that the volume needs
                    // verification.
                    //
                    PhysicalDeviceObject->Flags |= DO_VERIFY_VOLUME;
                }
            }

            //
            // Check to see if the partition table cache was valid or if
            // we should notify the PnP manager of a device change.
            //
            if (DiskInvalidatePartitionTable(DeviceExtension, FALSE) ||
                ForceBusRelations)
            {
                //
                // Yes, so we notify the PnP manager of a device
                // relations change.
                //
                IoInvalidateDeviceRelations(DeviceExtension->LowerPdo,
                                            BusRelations);
            }
        }
#else
        //
        // Now we check to see if the partition table should be
        // invalidated and this device is removable.
        //
        if ((InvalidatePartitionTable) &&
            (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA))
        {
            //
            // We are and it is so we indicate that the volume
            // needs verification.
            //
            DeviceObject->Flags |= DO_VERIFY_VOLUME;
        }
#endif
    }
}

VOID
DiskSetSpecialHacks(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                    IN ULONG_PTR Data)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}

VOID
ResetBus(IN PDEVICE_OBJECT Fdo)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}

NTSTATUS
DiskGetCacheInformation(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                        IN PDISK_CACHE_INFORMATION CacheInfo)
{
    PMODE_PARAMETER_HEADER ModeData;
    PMODE_CACHING_PAGE PageData;
    ULONG Length;
    PAGED_CODE();

    //
    // Allocate memory for the mode data.
    //
    ModeData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                     MODE_DATA_SIZE,
                                     DISK_TAG_DISABLE_CACHE);

    //
    // Check if there was a problem allocating memory for
    // the mode data.
    //
    if (!ModeData)
    {
        //
        // There was so we print an error and set status to
        // insufficient resources.
        //
        DbgPrint("DiskGetSetCacheInformation: Unable to allocate mode "
                 "data buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero out the memory we allocated.
    //
    RtlZeroMemory(ModeData, MODE_DATA_SIZE);

    //
    // Now we send the mode sense command and get the length
    // of the transferred data.
    //
    Length = ClassModeSense(DeviceExtension->DeviceObject,
                            (PUCHAR)ModeData,
                            MODE_DATA_SIZE,
                            MODE_SENSE_RETURN_ALL);

    //
    // Check if the transferred mode sense data was too small.
    //
    if (Length < sizeof(MODE_PARAMETER_HEADER))
    {
        //
        // It wasn't so we retry the mode sense request.
        //
        Length = ClassModeSense(DeviceExtension->DeviceObject,
                                (PUCHAR)ModeData,
                                MODE_DATA_SIZE,
                                MODE_SENSE_RETURN_ALL);

        //
        // Check again if the transferred mode sense data was
        // too small.
        //
        if (Length < sizeof(MODE_PARAMETER_HEADER))
        {
            //
            // It wasn't so we state that the mode sense failed.
            //
            DbgPrint("DiskGetCacheInformation: Mode Sense failed\n");

            //
            // Check if any mode data was read.
            //
            if (ModeData)
            {
                //
                // There was so we free the mode data and then
                // ensure we don’t use it again.
                //
                ExFreePool(ModeData);
                ModeData = NULL;
            }

            //
            // Return and mark this as a device error.
            //
            return STATUS_IO_DEVICE_ERROR;
        }
    }

    //
    // Check if the transferred mode sense data was too big.
    //
    if (Length > (ULONG)(ModeData->ModeDataLength + 1))
    {
        //
        // It is so we reset length to the correct size.
        //
        Length = ModeData->ModeDataLength + 1;
    }

    //
    // Now we check the mode sense data to see if write caching
    // is enabled
    //
    PageData = ClassFindModePage((PUCHAR)ModeData,
                                 Length,
                                 MODE_PAGE_CACHING,
                                 TRUE);

    //
    // Now we check to see if a caching page exists.
    //
    if (!PageData)
    {
        //
        // One doesn't so we state as much.
        //
        DbgPrint("DiskGetCacheInformation: Unable to find caching "
                 "mode page.\n");
        //
        // Check if any mode data was read.
        //
        if (ModeData)
        {
            //
            // There was so we free the mode data and then
            // ensure we don’t use it again.
            //
            ExFreePool(ModeData);
            ModeData = NULL;
        }

        //
        // Return with this marked as not supported.
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Clear CacheInfo so we can copy the cache information over.
    //
    RtlZeroMemory(CacheInfo, sizeof(DISK_CACHE_INFORMATION));

    //
    // Now we copy over the cache information.
    //
    CacheInfo->ParametersSavable = PageData->PageSavable;
    CacheInfo->ReadCacheEnabled = !(PageData->ReadDisableCache);
    CacheInfo->WriteCacheEnabled = PageData->WriteCacheEnable;

    //
    // These cache information values have to be translated to
    // the ones defined in ntdddisk.h before we copy them
    // over.
    //
    CacheInfo->ReadRetentionPriority =
        PageData->ReadRetensionPriority == 0xf ? 0x2 :
        (PageData->ReadRetensionPriority == 0x2 ? 0xf :
        PageData->ReadRetensionPriority);
    CacheInfo->WriteRetentionPriority =
        PageData->WriteRetensionPriority == 0xf ? 0x2 :
        (PageData->WriteRetensionPriority == 0x2 ? 0xf :
        PageData->WriteRetensionPriority);
    CacheInfo->DisablePrefetchTransferLength =
        ((PageData->DisablePrefetchTransfer[0] << 8) +
         PageData->DisablePrefetchTransfer[1]);
    CacheInfo->ScalarPrefetch.Minimum =
        ((PageData->MinimumPrefetch[0] << 8) + PageData->MinimumPrefetch[1]);
    CacheInfo->ScalarPrefetch.Maximum =
        ((PageData->MaximumPrefetch[0] << 8) + PageData->MaximumPrefetch[1]);

    //
    // Check if the cache page has a multiplication factor.
    //
    if (PageData->MultiplicationFactor)
    {
        //
        // It does so we fill in the related values.
        //
        CacheInfo->PrefetchScalar = TRUE;
        CacheInfo->ScalarPrefetch.MaximumBlocks =
            ((PageData->MaximumPrefetchCeiling[0] << 8) +
             PageData->MaximumPrefetchCeiling[1]);
    }

    //
    // Check if any mode data was read.
    //
    if (ModeData)
    {
        //
        // There was so we free the mode data and then
        // ensure we don’t use it again.
        //
        ExFreePool(ModeData);
        ModeData = NULL;
    }

    //
    // Return with a successful status.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskSetCacheInformation(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                        IN PDISK_CACHE_INFORMATION CacheInfo)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetCacheSetting(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION StackLocation;
    PDISK_CACHE_SETTING CacheSetting;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check to see if the output buffer is to small.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(DISK_CACHE_SETTING))
    {
        //
        // It is so we set status too buffer too small.
        //
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        //
        // Otherwise, read the cache settings.
        //
        CacheSetting = (PDISK_CACHE_SETTING)Irp->AssociatedIrp.SystemBuffer;

        //
        // Set the cache version, and set the state to normal.
        //
        CacheSetting->Version = sizeof(DISK_CACHE_SETTING);
        CacheSetting->State = DiskCacheNormal;

        //
        // Check to see if this device supports forced unit access.
        //
        if (DeviceExtension->ScanForSpecialFlags &
            CLASS_SPECIAL_FUA_NOT_SUPPORTED)
        {
            //
            // It doesn't so we indicate as much.
            //
            CacheSetting->State = DiskCacheWriteThroughNotSupported;
        }

        //
        // Check to see if this device allows the cache setting to
        // be modified.
        //
        if (DeviceExtension->ScanForSpecialFlags &
            CLASS_SPECIAL_MODIFY_CACHE_UNSUCCESSFUL)
        {
            //
            // It doesn't so we indicate as much.
            //
            CacheSetting->State = DiskCacheModifyUnsuccessful;
        }

        //
        // Indicate if the device is power protected or not (ie.
        // connected to a backup power supply).
        //
        CacheSetting->IsPowerProtected =
            (BOOLEAN)(DeviceExtension->DeviceFlags & DEV_POWER_PROTECTED);

        //
        // Set the IRP's I/O status information to the correct size.
        //
        Irp->IoStatus.Information = sizeof(DISK_CACHE_SETTING);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlSetCacheSetting(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(Irp);
    PDISK_CACHE_SETTING CacheSetting;
    ULONG IsPowerProtected;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Make sure were running at less than dispatch level.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        //
        // We arn't so fail and return the invalid level
        // error code.
        //
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return STATUS_INVALID_LEVEL;
    }

    //
    // Check to see if the input buffer is to small.
    //
    if (StackLocation->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(DISK_CACHE_SETTING))
    {
        //
        // It is so we set status to info length mismatch.
        //
        Status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else
    {
        //
        // Otherwise, read the cache settings.
        //
        CacheSetting = (PDISK_CACHE_SETTING)Irp->AssociatedIrp.SystemBuffer;

        //
        // Make sure the cache version (size) is correct.
        //
        if (CacheSetting->Version == sizeof(DISK_CACHE_SETTING))
        {
            //
            // Now we check to see if the device is power protected
            // (ie. connected to a backup power supply).
            //
            if (CacheSetting->IsPowerProtected)
            {
                //
                // It is so we set the power protected flag.
                //
                DeviceExtension->DeviceFlags |= DEV_POWER_PROTECTED;
                IsPowerProtected = 1;
            }
            else
            {
                //
                // It isn't so we clear the power protected flag.
                //
                DeviceExtension->DeviceFlags &= ~DEV_POWER_PROTECTED;
                IsPowerProtected = 0;
            }

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
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
#endif

            //
            // Now we call class to save the settings in the registry.
            //
            ClassSetDeviceParameter(DeviceExtension,
                                    DiskDeviceParameterSubkey,
                                    DiskDeviceCacheIsPowerProtected,
                                    IsPowerProtected);
        }
        else
        {
            //
            // Otherwise we set status to invalid parameter.
            //
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlGetLengthInfo(IN OUT PDEVICE_OBJECT DeviceObject,
                       IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetDriveGeometry(IN PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDISK_DATA DiskData;
    PIO_STACK_LOCATION StackLocation;
    NTSTATUS Status;

    //
    // Make sure were running at less than dispatch level.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        //
        // We arn't so fail and return the invalid level
        // error code.
        //
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return STATUS_INVALID_LEVEL;
    }

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Make sure the output buffer is big enough to contain the
    // disk's geometry data.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(DISK_GEOMETRY))
    {
        //
        // It isn't so we print an error and return the buffer too
        // small error code.
        //
        DbgPrint("DiskIoctlGetDriveGeometry: Output buffer too small.\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Check to see if the device supports removable media.
    //
    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // It does so we update the device extension with data for
        // the current media.
        //
        Status = DiskReadDriveCapacity(CommonExtension->
                                       PartitionZeroExtension->
                                       DeviceObject);

        //
        // Update the drive's information to show if it is in a
        // ready state or not.
        //
        DiskData->ReadyStatus = Status;

        //
        // Make sure there wasn't a problem updating the device extension
        // with information about the current media.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we return the error code.
            //
            return Status;
        }
    }

    //
    // Now we move the drive's geometry information from the device
    // extension to the IRP.
    //
    RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                  &(DeviceExtension->DiskGeometry),
                  sizeof(DISK_GEOMETRY));

    //
    // Check to see if the sector size is zero.
    //
    if (!(((PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer)->BytesPerSector))
    {
        //
        // It is so we set default value for sector size (512).
        //
        ((PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer)->BytesPerSector = 512;
    }

    //
    // Set the IRP's I/O status information to the correct size.
    //
    Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetDriveGeometryEx(IN PDEVICE_OBJECT DeviceObject,
                            IN OUT PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData;
    PDISK_GEOMETRY_EX_INTERNAL DiskGeometryEx;
    ULONG OutputBufferLength;
    NTSTATUS Status;

    //
    // Make sure were running at less than dispatch level.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        //
        // We arn't so fail and return the invalid level
        // error code.
        //
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return STATUS_INVALID_LEVEL;
    }

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Clear the disk geometry pointer.
    //
    DiskGeometryEx = NULL;

    //
    // Get the IRP's output buffer length.
    //
    OutputBufferLength =
        StackLocation->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Make sure the output buffer is big enough to contain the
    // disk's geometry data.
    //
    if (OutputBufferLength < FIELD_OFFSET(DISK_GEOMETRY_EX, Data))
    {
        //
        // It isn't so we print an error and return the buffer too
        // small error code.
        //
        DbgPrint("DiskIoctlGetDriveGeometryEx: Output buffer too small.\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Check to see if the device supports removable media.
    //
    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // It does so we update the device extension with data for
        // the current media.
        //
        Status = DiskReadDriveCapacity(CommonExtension->
                                       PartitionZeroExtension->
                                       DeviceObject);

        //
        // Update the drive's information to show if it is in a
        // ready state or not.
        //
        DiskData->ReadyStatus = Status;

        //
        // Make sure there wasn't a problem updating the device extension
        // with information about the current media.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we return the error code.
            //
            return Status;
        }
    }

    //
    // Copy the volume's physical location(s).
    //
    DiskGeometryEx =
        (PDISK_GEOMETRY_EX_INTERNAL)Irp->AssociatedIrp.SystemBuffer;

    //
    // Copy the disk's geometry information.
    //
    DiskGeometryEx->Geometry = DeviceExtension->DiskGeometry;

    //
    // Check to see if the sector size is zero.
    //
    if (!(DiskGeometryEx->Geometry.BytesPerSector))
    {
        //
        // It is so we set default value for sector size (512).
        //
        DiskGeometryEx->Geometry.BytesPerSector = 512;
    }

    //
    // Copy the partition size (in bytes).
    //
    DiskGeometryEx->DiskSize = CommonExtension->
                               PartitionZeroExtension->
                               CommonExtension.PartitionLength;

    //
    // Make sure the output buffer is big enough to contain the
    // partition information.
    //
    if (OutputBufferLength >=
        FIELD_OFFSET(DISK_GEOMETRY_EX_INTERNAL, Detection))
    {
        //
        // It is so we set the correct size to copy the partition
        // information.
        //
        DiskGeometryEx->Partition.SizeOfPartitionInfo =
            sizeof(DiskGeometryEx->Partition);

        //
        // Copy the partition style.
        //
        DiskGeometryEx->Partition.PartitionStyle =
            DiskData->PartitionStyle;

        //
        // Now we find out the partition style and take the appropriate
        // course of action.
        //
        switch (DiskData->PartitionStyle)
        {
            //
            // Is this a GUID partition table (GPT) style partition?
            // (for EFI systems)
            //
            case PARTITION_STYLE_GPT:
                //
                // It is so we copy the GPT signature and break out of
                // the switch.
                //
                DiskGeometryEx->Partition.Gpt.DiskId = DiskData->Efi.DiskId;
                break;

            //
            // Is this a master boot record (MBR) style partition?
            //
            case PARTITION_STYLE_MBR:
                //
                // It is so we copy the MBR's signature and checksum and
                // break out of the switch.
                //
                DiskGeometryEx->Partition.Mbr.Signature = DiskData->Mbr.Signature;
                DiskGeometryEx->Partition.Mbr.CheckSum = DiskData->Mbr.MbrCheckSum;
                break;

            default:
                //
                // By default we assume this is a raw disk and clear
                // the signature.
                //
                RtlZeroMemory(&DiskGeometryEx->Partition,
                              sizeof(DiskGeometryEx->Partition));
        }
    }

    //
    // Make sure the output buffer is big enough to contain the
    // detection information.
    //
    if (OutputBufferLength >= sizeof(DISK_GEOMETRY_EX_INTERNAL))
    {
        //
        // It is so we set the correct size to copy the detection
        // information.
        //
        DiskGeometryEx->Detection.SizeOfDetectInfo = sizeof(DiskGeometryEx->
                                                            Detection);

        //
        // Get the detection information.
        //
        Status = DiskGetDetectInfo(DeviceExtension,
                                   &DiskGeometryEx->Detection);

        //
        // Make sure there wasn't a problem getting the detection
        // information.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we set the detection type to none.
            //
            DiskGeometryEx->Detection.DetectionType = DetectNone;
        }
    }

    //
    // Mark our current status as successfull.
    //
    Status = STATUS_SUCCESS;

    //
    // Set the size of the buffer the IRP will recieve to whichever
    // is smaller, the output buffer or the size of the disk geometry
    // structure.
    //
    Irp->IoStatus.Information = min(OutputBufferLength,
                                    sizeof(DISK_GEOMETRY_EX_INTERNAL));

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlGetCacheInformation(IN PDEVICE_OBJECT DeviceObject,
                             IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlSetCacheInformation(IN PDEVICE_OBJECT DeviceObject,
                             IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetMediaTypesEx(IN PDEVICE_OBJECT DeviceObject,
                         IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlPredictFailure(IN PDEVICE_OBJECT DeviceObject,
                        IN OUT PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDISK_DATA DiskData;
    PIO_STACK_LOCATION StackLocation;
    PSTORAGE_PREDICT_FAILURE IoctlPredictFailure;
    STORAGE_FAILURE_PREDICT_STATUS DiskSmartStatus;
    IO_STATUS_BLOCK IoStatusBlock = {0};
    KEVENT AutoEvent;
    ULONG ScsiReadBufferSize;
    PUCHAR ScsiReadBuffer;
    PIRP ScsiReadIrp;
    PDEVICE_OBJECT DeviceTopOfStack;
    LARGE_INTEGER ReadOffset;
    NTSTATUS Status;

    //
    // Make sure were running at less than dispatch level.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        //
        // We arn't so fail and return the invalid level
        // error code.
        //
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return STATUS_INVALID_LEVEL;
    }

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // State that were in DiskIoctlPredictFailure and what
    // device and IRP we are working with.
    //
    DbgPrint("DiskIoctlPredictFailure: DeviceObject %p Irp %p\n",
             DeviceObject, Irp);

    //
    // Make sure the output buffer is big enough to contain
    // the failure prediction information.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(STORAGE_PREDICT_FAILURE))
    {
        //
        // It isn't so we print an error and return the buffer too
        // small error code.
        //
        DbgPrint("DiskIoctlPredictFailure: Output buffer too small.\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Get the disks failure prediction information.
    //
    IoctlPredictFailure =
        (PSTORAGE_PREDICT_FAILURE)Irp->AssociatedIrp.SystemBuffer;

    //
    // Check first is the disk supports failure prediction
    // via sense data.
    //
    if (DiskData->FailurePredictionCapability == FailurePredictionSense)
    {
        //
        // For now we indicate that this device doesn't support
        // failure prediction.
        //
        IoctlPredictFailure->PredictFailure = 0;

        //
        // Initialize an event object in preparation for sending a
        // read to the SCSI disk (to provoke the disk to report
        // any failures).
        //
        KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

        //
        // Get a reference to the device object at the top of the stack.
        //
        DeviceTopOfStack = IoGetAttachedDeviceReference(DeviceObject);

        //
        // Now we set the read buffer size and allocate memory for
        // the read buffer.
        //
        ScsiReadBufferSize = DeviceExtension->DiskGeometry.BytesPerSector;
        ScsiReadBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                               ScsiReadBufferSize,
                                               DISK_TAG_SMART);

        //
        // Make sure there wasn't a problem allocating memory
        // for the read buffer.
        //
        if (ScsiReadBuffer)
        {
            //
            // There wasn't so we set the starting offset and build
            // an IRP so we can send the read.
            //
            ReadOffset.QuadPart = 0;
            ScsiReadIrp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                                       DeviceTopOfStack,
                                                       ScsiReadBuffer,
                                                       ScsiReadBufferSize,
                                                       &ReadOffset,
                                                       &AutoEvent,
                                                       &IoStatusBlock);

            //
            // Make sure there wasn't a problem building the IRP.
            //
            if (ScsiReadIrp)
            {
                //
                // There wasn't so we send the read to the disk.
                //
                Status = IoCallDriver(DeviceTopOfStack, ScsiReadIrp);

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
            else
            {
                //
                // Otherwise we set status to insufficient resources.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Check if anything was read.
            //
            if (ScsiReadBuffer)
            {
                //
                // There was one so we free the read buffer and then
                // ensure we don’t use it again.
                //
                ExFreePool(ScsiReadBuffer);
                ScsiReadBuffer = NULL;
            }
        }
        else
        {
            //
            // Otherwise we set status to insufficient resources.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Now we dereference the device object.
        //
        ObDereferenceObject(DeviceTopOfStack);
    }

    //
    // Now we check if the disk supports failure prediction
    // via SMART or via sense data.
    //
    if ((DiskData->FailurePredictionCapability == FailurePredictionSmart) ||
        (DiskData->FailurePredictionCapability == FailurePredictionSense))
    {
        //
        // Get the current failure prediction status.
        //
        Status = DiskReadFailurePredictStatus(DeviceExtension,
                                              &DiskSmartStatus);

        //
        // Check if there was a problem getting the current
        // failure prediction status.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // There wasn't so we read any current failure
            // predicition data.
            //
            Status = DiskReadFailurePredictData(DeviceExtension,
                                                Irp->
                                                AssociatedIrp.SystemBuffer);

            //
            // Check if the device is predicting a failure.
            //
            if (DiskSmartStatus.PredictFailure)
            {
                //
                // It is so we indicate that it is.
                //
                IoctlPredictFailure->PredictFailure = 1;
            }
            else
            {
                //
                // It isn't so we indicate that it isn't
                //
                IoctlPredictFailure->PredictFailure = 0;
            }

            //
            // Update the IRP's I/O status information.
            //
            Irp->IoStatus.Information = sizeof(STORAGE_PREDICT_FAILURE);
        }
    }
    else
    {
        //
        // Otherwise we mark this as an invalid device request.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlVerify(IN PDEVICE_OBJECT DeviceObject,
                IN OUT PIRP Irp)
{
    //
    // FIXME: TODO (This funtion needs to handle PDOs as well
    // as FDOs)
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlReassignBlocks(IN PDEVICE_OBJECT DeviceObject,
                        IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlReassignBlocksEx(IN PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlIsWritable(IN PDEVICE_OBJECT DeviceObject,
                    IN OUT PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PMODE_PARAMETER_HEADER ModeData;
    PSCSI_REQUEST_BLOCK ScsiRequestBlock;
    PCDB ControlBlock;
    ULONG ModeLength, Retries = 4;
    NTSTATUS Status;

    //
    // Make sure were running at less than dispatch level.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        //
        // We arn't so fail and return the invalid level
        // error code.
        //
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return STATUS_INVALID_LEVEL;
    }

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // State that we are in DiskIoctlIsWritable and which
    // device object and IRP we are working with.
    //
    DbgPrint("DiskIoctlIsWritable: DeviceObject %p Irp %p\n",
             DeviceObject, Irp);

    //
    // Now we allocate memory for the SCSI request block.
    //
    ScsiRequestBlock = ExAllocatePoolWithTag(NonPagedPool,
                                             SCSI_REQUEST_BLOCK_SIZE,
                                             DISK_TAG_SRB);

    //
    // Check if there was a problem allocationg memory for the
    // SCSI request block.
    //
    if (!ScsiRequestBlock)
    {
        //
        // There was so we print an error and return with status
        // insufficient resources.
        //
        DbgPrint("DiskIoctlIsWritable: Unable to allocate memory.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Clear the memory we just allocated for the SCSI
    // request block.
    //
    RtlZeroMemory(ScsiRequestBlock, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Calculate the size we need for the mode data.
    // NOTE: Some port drivers need to convert to MODE10
    //       or return MODE_PARAMETER_BLOCK so we take
    //       this into account when calculating the
    //       size needed.
    //
    ModeLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_PARAMETER_BLOCK);

    //
    // Now we allocate the memory for the node data.
    //
    ModeData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                     ModeLength,
                                     DISK_TAG_MODE_DATA);

    //
    // Check if there was a problem allocating memory for
    // the mode data.
    //
    if (!ModeData)
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskIoctlIsWritable: Unable to allocate memory.\n");

        //
        // Check if there was a SCSI request block.
        //
        if (ScsiRequestBlock)
        {
            //
            // There was so we free the SCSI request block
            // and clear the pointer.
            //
            ExFreePool(ScsiRequestBlock);
            ScsiRequestBlock = NULL;
        }

        //
        // Return with status insufficient resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Clear the memory we just allocated for the mode data.
    //
    RtlZeroMemory(ModeData, ModeLength);

    //
    // Set the SCSI request block's control descriptor
    // block length (Start building MODE_SENSE
    // CDB here).
    //
    ScsiRequestBlock->CdbLength = 6;

    //
    // Get the control descriptor block from the SCSI
    // request block.
    //
    ControlBlock = (PCDB)ScsiRequestBlock->Cdb;

    //
    // Get the SCSI request block's timeout from the device
    // extension.
    //
    ScsiRequestBlock->TimeOutValue = DeviceExtension->TimeOutValue;

    //
    // Finish building the MODE_SENSE control descriptor block
    // by setting the operation code, page code and mode data
    // size.
    //
    ControlBlock->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    ControlBlock->MODE_SENSE.PageCode = MODE_SENSE_RETURN_ALL;
    ControlBlock->MODE_SENSE.AllocationLength = (UCHAR)ModeLength;

    //
    // Start looping to check if the disk is writable.
    //
    while (Retries)
    {
        //
        // Now we call class to send the SCSI request block to
        // the drive.
        //
        Status = ClassSendSrbSynchronous(DeviceObject,
                                         ScsiRequestBlock,
                                         ModeData,
                                         ModeLength,
                                         FALSE);

        //
        // Check if we have any status except verification required.
        //
        if (Status != STATUS_VERIFY_REQUIRED)
        {
            //
            // We do so we check if we have a data overrun status.
            //
            if (SRB_STATUS(ScsiRequestBlock->SrbStatus) ==
                SRB_STATUS_DATA_OVERRUN)
            {
                //
                // We do so we set status to successful.
                //
                Status = STATUS_SUCCESS;
            }

            //
            // Break out of the loop.
            //
            break;
        }

        //
        // Decrement the retry count.
        //
        Retries--;
    }

    //
    // Check to see if the drive is writable.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It is so we check to see if the write protected
        // flag is set.
        //
        if (ModeData->DeviceSpecificParameter & MODE_DSP_WRITE_PROTECT)
        {
            //
            // It is so we set status to media write protected.
            //
            Status = STATUS_MEDIA_WRITE_PROTECTED;
        }
    }

    //
    // Check if there was a SCSI request block.
    //
    if (ScsiRequestBlock)
    {
        //
        // There was so we free the SCSI request block
        // and clear the pointer.
        //
        ExFreePool(ScsiRequestBlock);
        ScsiRequestBlock = NULL;
    }

    //
    // Check if there was any mode data.
    //
    if (ModeData)
    {
        //
        // There was so we free the mode data and clear
        // the pointer.
        //
        ExFreePool(ModeData);
        ModeData = NULL;
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlSetVerify(IN PDEVICE_OBJECT DeviceObject,
                   IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlClearVerify(IN PDEVICE_OBJECT DeviceObject,
                     IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlUpdateDriveSize(IN PDEVICE_OBJECT DeviceObject,
                         IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetVolumeDiskExtents(IN PDEVICE_OBJECT DeviceObject,
                              IN OUT PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PVOLUME_DISK_EXTENTS VolumeExtents;
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Make sure were running at less than dispatch level.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        //
        // We arn't so fail and return the invalid level
        // error code.
        //
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return STATUS_INVALID_LEVEL;
    }

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // State that were in DiskIoctlGetVolumeDiskExtents and what
    // device and IRP we are working with.
    //
    DbgPrint("DiskIoctlGetVolumeDiskExtents: DeviceObject %p Irp %p\n",
             DeviceObject, Irp);

    //
    // Check to see if the device supports removable media.
    //
    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // It does so we make sure the output buffer is big enough
        // to contain the volume's physical location.
        //
        if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(VOLUME_DISK_EXTENTS))
        {
            //
            // It isn't so we print an error and return the buffer too
            // small error code.
            //
            DbgPrint("DiskIoctlGetVolumeDiskExtents: Output buffer "
                     "too small.\n");
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Now we update the device extension with data for
        // the current media.
        //
        Status = DiskReadDriveCapacity(CommonExtension->
                                       PartitionZeroExtension->
                                       DeviceObject);

        //
        // Update the drive's information to show if it is in a
        // ready state or not.
        //
        DiskData->ReadyStatus = Status;

        if (NT_SUCCESS(Status))
        {
            //
            // Initialize VolumeExtents with information from the
            // IRP.
            //
            VolumeExtents =
                (PVOLUME_DISK_EXTENTS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Set the number of physical locations to one.
            //
            VolumeExtents->NumberOfDiskExtents = 1;

            //
            // Get the device number.
            //
            VolumeExtents->Extents[0].DiskNumber =
                CommonExtension->PartitionZeroExtension->DeviceNumber;

            //
            // Get the starting offset for the volume.
            //
            VolumeExtents->Extents[0].StartingOffset =
                CommonExtension->StartingOffset;

            //
            // Get the partition size.
            //
            VolumeExtents->Extents[0].ExtentLength =
                CommonExtension->PartitionLength;

            //
            // Set the IRP's I/O status information to the correct
            // size.
            //
            Irp->IoStatus.Information = sizeof(VOLUME_DISK_EXTENTS);
        }
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlSmartGetVersion(IN PDEVICE_OBJECT DeviceObject,
                         IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlSmartReceiveDriveData(IN PDEVICE_OBJECT DeviceObject,
                               IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlSmartSendDriveCommand(IN PDEVICE_OBJECT DeviceObject,
                               IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlCreateDisk(IN OUT PDEVICE_OBJECT DeviceObject,
                    IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetDriveLayout(IN OUT PDEVICE_OBJECT DeviceObject,
                        IN OUT PIRP Irp)
{
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayoutEx = NULL;
    PDRIVE_LAYOUT_INFORMATION DriveLayout = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData;
    BOOLEAN UsePtCache = TRUE;
    ULONG DriveLayoutSize;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Ensure we actually have a device object and an IRP before
    // moving on.
    //
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if the partition table cache is invalid.
    //
    if (!DiskData->CachedPartitionTableValid)
    {
        //
        // It is so we re-read the drive capacity and indicate that
        // we will not be using the partition table cache.
        //
        DiskReadDriveCapacity(DeviceExtension->DeviceObject);
        UsePtCache = FALSE;
    }

    //
    // Now we acquire a partition lock.
    //
    DiskAcquirePartitioningLock(DeviceExtension);

    //
    // Get the current drive layout information.
    //
    Status = DiskReadPartitionTableEx(DeviceExtension,
                                      FALSE,
                                      &DriveLayoutEx);

    //
    // Check if there was a problem getting the current drive
    // layout.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we release the partition lock and
        // return the current status.
        //
        DiskReleasePartitioningLock(DeviceExtension);
        return Status;
    }

    //
    // Check if we are dealing with anything other than a master
    // boot record (MBR) style partition table.
    //
    if (DriveLayoutEx->PartitionStyle != PARTITION_STYLE_MBR)
    {
        //
        // We are so we release the partition lock and
        // fail by returning a status of invalid
        // device request.
        //
        DiskReleasePartitioningLock(DeviceExtension);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Calculate the size needed for the drive layout.
    //
    DriveLayoutSize =
        FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[0]) +
        DriveLayoutEx->PartitionCount * sizeof(PARTITION_INFORMATION);

    //
    // Make sure the output buffer is big enough to contain the drive
    // layout information.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        DriveLayoutSize)
    {
        //
        // It isn't so we set the IRPs status to buffer too small,
        // release the partition lock and return with status
        // buffer too small.
        //
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        DiskReleasePartitioningLock(DeviceExtension);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Ensure we have a update partition routine.
    //
    ASSERT(DiskData->UpdatePartitionRoutine);

    //
    // Now we synchronize the drive layout with the device object.
    //
    DiskData->UpdatePartitionRoutine(DeviceObject, DriveLayoutEx);

    //
    // Now we convert the new style drive layout to the old one.
    //
    DriveLayout = DiskConvertExtendedToLayout(DriveLayoutEx);

    //
    // Check if there was a problem converting the drive layout.
    //
    if (!DriveLayout)
    {
        //
        // There was so we set the IRPs status to insufficient
        // resources, release the partition lock and return
        // with status insufficient resources.
        //
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        DiskReleasePartitioningLock(DeviceExtension);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we are finished with the extended drive layout
    // so we clear the pointer.
    //
    DriveLayoutEx = NULL;

    //
    // Copy the drive layout to the output buffer.
    //
    RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                  DriveLayout,
                  DriveLayoutSize);

    //
    // Indicate the size of the output buffer and save the
    // current status.
    //
    Irp->IoStatus.Information = DriveLayoutSize;
    Irp->IoStatus.Status = Status;

    //
    // Make sure we have a drive layout.
    //
    if (DriveLayout)
    {
        //
        // We do so we free the memory allocated by DiskConvertExtendedToLayout
        // and clear the pointer.
        //
        ExFreePool(DriveLayout);
        DriveLayout = NULL;
    }

    //
    // Release the partition lock.
    //
    DiskReleasePartitioningLock(DeviceExtension);

    //
    // Check to see if we are not using the partition
    // table cache.
    //
    if (!UsePtCache)
    {
        //
        // We arn't so we call class to re-enumerate the devices
        // on the bus.
        //
        ClassInvalidateBusRelations(DeviceObject);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlGetDriveLayoutEx(IN OUT PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData;
    BOOLEAN UsePtCache = TRUE;
    ULONG DriveLayoutSize;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Ensure we actually have a device object and an IRP before
    // moving on.
    //
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if the partition table cache is invalid.
    //
    if (!DiskData->CachedPartitionTableValid)
    {
        //
        // It is so we re-read the drive capacity and indicate that
        // we will not be using the partition table cache.
        //
        DiskReadDriveCapacity(DeviceExtension->DeviceObject);
        UsePtCache = FALSE;
    }

    //
    // Now we acquire a partition lock.
    //
    DiskAcquirePartitioningLock(DeviceExtension);

    //
    // Get the current drive layout information.
    //
    Status = DiskReadPartitionTableEx(DeviceExtension,
                                      FALSE,
                                      &DriveLayout);

    //
    // Check if there was a problem getting the current drive
    // layout.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we release the partition lock and
        // return the current status.
        //
        DiskReleasePartitioningLock(DeviceExtension);
        return Status;
    }

    //
    // Calculate the size needed for the drive layout.
    //
    DriveLayoutSize =
        FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]) +
        DriveLayout->PartitionCount * sizeof(PARTITION_INFORMATION_EX);

    //
    // Make sure the output buffer is big enough to contain the drive
    // layout information.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        DriveLayoutSize)
    {
        //
        // It isn't so we set the IRPs status to buffer too small,
        // release the partition lock and return with status
        // buffer too small.
        //
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        DiskReleasePartitioningLock(DeviceExtension);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Ensure we have an update partition routine.
    //
    ASSERT(DiskData->UpdatePartitionRoutine);

    //
    // Now we synchronize the drive layout with the device object.
    //
    DiskData->UpdatePartitionRoutine(DeviceObject, DriveLayout);

    //
    // Copy the drive layout to the output buffer.
    //
    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                  DriveLayout,
                  DriveLayoutSize);

    //
    // Indicate the size of the output buffer and save the
    // current status.
    //
    Irp->IoStatus.Information = DriveLayoutSize;
    Irp->IoStatus.Status = Status;

    //
    // Release the partition lock.
    //
    DiskReleasePartitioningLock(DeviceExtension);

    //
    // Check to see if we are not using the partition
    // table cache.
    //
    if (!UsePtCache)
    {
        //
        // We arn't so we call class to re-enumerate the devices
        // on the bus.
        //
        ClassInvalidateBusRelations(DeviceObject);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlSetDriveLayout(IN OUT PDEVICE_OBJECT DeviceObject,
                        IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlSetDriveLayoutEx(IN OUT PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlDeleteDriveLayout(IN OUT PDEVICE_OBJECT DeviceObject,
                           IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGetPartitionInfo(IN OUT PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION PartitionZeroExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PPARTITION_INFORMATION PartitionInfo;
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData, PartitionZeroData;
    NTSTATUS Status, OldReadyStatus;
    PAGED_CODE();

    //
    // Ensure we actually have a device object and an IRP before
    // moving on.
    //
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the partition zero extension.
    //
    PartitionZeroExtension = CommonExtension->PartitionZeroExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Get the partition zero driver data.
    //
    PartitionZeroData = (PDISK_DATA)PartitionZeroExtension->
                                    CommonExtension.DriverData;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Make sure the output buffer is big enough to contain
    // the partition information.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(PARTITION_INFORMATION))
    {
        //
        // It isn't so we return a status of buffer too small.
        //
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Now we read the drive's capacity to make sure we have
    // current information.
    //
    Status = DiskReadDriveCapacity(PartitionZeroExtension->DeviceObject);

    //
    // Get the drive's current ready status in a safe way.
    //
    OldReadyStatus = InterlockedExchange(&(PartitionZeroData->ReadyStatus),
                                         Status);

    //
    // Make sure the ready status has not changed.
    //
    if (PartitionZeroData->ReadyStatus != OldReadyStatus)
    {
        //
        // It has so we notify the plug and play manager.
        //
        IoInvalidateDeviceRelations(PartitionZeroExtension->LowerPdo,
                                    BusRelations);
    }

    //
    // Check if there has been any problems so far.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There has so we return with the current status.
        //
        return Status;
    }

    //
    // Check if we are on partition zero.
    //
    if (!CommonExtension->PartitionNumber)
    {
        //
        // We are so we can safely do this on either a master boot
        // record (MBR) or a GUID partition table (GPT) style
        // disk. Proceed with filling in the partition
        // information.
        //
        PartitionInfo =
            (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
        PartitionInfo->PartitionType = PARTITION_ENTRY_UNUSED;
        PartitionInfo->StartingOffset = CommonExtension->StartingOffset;
        PartitionInfo->PartitionLength = CommonExtension->PartitionLength;
        PartitionInfo->HiddenSectors = 0;
        PartitionInfo->PartitionNumber = CommonExtension->PartitionNumber;
        PartitionInfo->BootIndicator = FALSE;
        PartitionInfo->RewritePartition = FALSE;
        PartitionInfo->RecognizedPartition = FALSE;
    }
    else
    {
        //
        // Otherwise, we check to see if this is a GUID partition
        // table (GPT) style disk.
        //
        if (DiskData->PartitionStyle != PARTITION_STYLE_MBR)
        {
            //
            // It is and we arn't on partition zero so we set the current
            // status to invalid device request, set the IRPs status
            // and return with the new status.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = Status;
            return Status;
        }

        //
        // Now we enumerate the partition zero extension to create
        // physical device objects for all partitions on the disk.
        //
        DiskEnumerateDevice(PartitionZeroExtension->DeviceObject);

        //
        // Get a partition lock on the partition zero extension.
        //
        DiskAcquirePartitioningLock(PartitionZeroExtension);

        //
        // Now we proceed with filling in the partition information.
        //
        PartitionInfo =
            (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
        PartitionInfo->PartitionType = DiskData->Mbr.PartitionType;
        PartitionInfo->StartingOffset = CommonExtension->StartingOffset;
        PartitionInfo->PartitionLength = CommonExtension->PartitionLength;
        PartitionInfo->HiddenSectors = DiskData->Mbr.HiddenSectors;
        PartitionInfo->PartitionNumber = CommonExtension->PartitionNumber;
        PartitionInfo->BootIndicator = DiskData->Mbr.BootIndicator;
        PartitionInfo->RewritePartition = FALSE;
        PartitionInfo->RecognizedPartition =
            IsRecognizedPartition(DiskData->Mbr.PartitionType);

        //
        // Release the partition lock on the partition zero extension.
        //
        DiskReleasePartitioningLock(PartitionZeroExtension);
    }

    //
    // Set status to successful.
    //
    Status = STATUS_SUCCESS;

    //
    // Indicate the size of the output buffer.
    //
    Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlGetPartitionInfoEx(IN OUT PDEVICE_OBJECT DeviceObject,
                            IN OUT PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION PartitionZeroExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PPARTITION_INFORMATION_EX PartitionInfo;
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData, PartitionZeroData;
    NTSTATUS Status, OldReadyStatus;
    PAGED_CODE();

    //
    // Ensure we actually have a device object and an IRP before
    // moving on.
    //
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the partition zero extension.
    //
    PartitionZeroExtension = CommonExtension->PartitionZeroExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Get the partition zero driver data.
    //
    PartitionZeroData = (PDISK_DATA)PartitionZeroExtension->
                                    CommonExtension.DriverData;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Make sure the output buffer is big enough to contain
    // the partition information.
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(PARTITION_INFORMATION_EX))
    {
        //
        // It isn't so we set status to buffer too small, save
        // the current status to the IRP and return with the
        // current status.
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Status = Status;
        return Status;
    }

    //
    // Now we read the drive's capacity to make sure we have
    // current information.
    //
    Status = DiskReadDriveCapacity(PartitionZeroExtension->DeviceObject);

    //
    // Get the drive's current ready status in a safe way.
    //
    OldReadyStatus = InterlockedExchange(&(PartitionZeroData->ReadyStatus),
                                         Status);

    //
    // Make sure the ready status has not changed.
    //
    if (PartitionZeroData->ReadyStatus != OldReadyStatus)
    {
        //
        // It has so we notify the plug and play manager.
        //
        IoInvalidateDeviceRelations(PartitionZeroExtension->LowerPdo,
                                    BusRelations);
    }

    //
    // Check if there has been any problems so far.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There has so we return with the current status.
        //
        return Status;
    }

    //
    // Check if we are on a partition other than partition zero.
    //
    if (CommonExtension->PartitionNumber)
    {
        //
        // We are so we re-enumerate to ensure that we have
        // current information.
        //
        DiskEnumerateDevice(PartitionZeroExtension->DeviceObject);

        //
        // Get a partition lock on the partition zero extension.
        //
        DiskAcquirePartitioningLock(PartitionZeroExtension);
    }

    //
    // Now we proceed with filling in the partition information.
    //
    PartitionInfo = (PPARTITION_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;
    PartitionInfo->StartingOffset = CommonExtension->StartingOffset;
    PartitionInfo->PartitionLength = CommonExtension->PartitionLength;
    PartitionInfo->RewritePartition = FALSE;
    PartitionInfo->PartitionNumber = CommonExtension->PartitionNumber;
    PartitionInfo->PartitionStyle = DiskData->PartitionStyle;

    //
    // Check if we are dealing with a master boot record (MBR)
    // style partition table.
    //
    if (DiskData->PartitionStyle == PARTITION_STYLE_MBR)
    {
        //
        // It is so we fill in the partition type, hidden sectors
        // and boot indicator and determine which drive letter
        // should be assigned to this partition.
        //
        PartitionInfo->Mbr.PartitionType = DiskData->Mbr.PartitionType;
        PartitionInfo->Mbr.HiddenSectors = DiskData->Mbr.HiddenSectors;
        PartitionInfo->Mbr.BootIndicator = DiskData->Mbr.BootIndicator;
        PartitionInfo->Mbr.RecognizedPartition =
            IsRecognizedPartition(DiskData->Mbr.PartitionType);
    }
    else
    {
        //
        // Otherwise, we have a GUID partition table (GPT) so
        // we set the partition type, partition ID, attributes
        // and copy the partition name for this partition.
        //
        PartitionInfo->Gpt.PartitionType = DiskData->Efi.PartitionType;
        PartitionInfo->Gpt.PartitionId = DiskData->Efi.PartitionId;
        PartitionInfo->Gpt.Attributes = DiskData->Efi.Attributes;
        RtlCopyMemory(PartitionInfo->Gpt.Name,
                      DiskData->Efi.PartitionName,
                      sizeof(PartitionInfo->Gpt.Name));
    }

    //
    // Set status to successful.
    //
    Status = STATUS_SUCCESS;

    //
    // Indicate the size of the output buffer.
    //
    Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);

    //
    // Check if we are on a partition other than partition zero.
    //
    if (CommonExtension->PartitionNumber)
    {
        //
        // We are so we release the partition lock on the partition
        // zero extension we acquired earlier.
        //
        DiskReleasePartitioningLock(PartitionZeroExtension);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskIoctlSetPartitionInfo(IN OUT PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlSetPartitionInfoEx(IN OUT PDEVICE_OBJECT DeviceObject,
                            IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlGrowPartition(IN OUT PDEVICE_OBJECT DeviceObject,
                       IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskIoctlUpdateProperties(IN OUT PDEVICE_OBJECT DeviceObject,
                          IN OUT PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskQueryPnpCapabilities(IN PDEVICE_OBJECT DeviceObject,
                         IN PDEVICE_CAPABILITIES Capabilities)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
