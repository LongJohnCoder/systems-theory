/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    ftwmireg.cxx

Abstract:

    The Fault Tolerance Driver provides fault tolerance for disk by using
    disk mirroring and striping. Additionally, it creates disk device objects
    that represent volumes on Basic disks. For each volume, FtDisk creates a
    symbolic link of the form \Device\HarddiskVolumeX, identifying the volume.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 21-09-06

--*/
#include "precomp.hxx"

WMIGUIDREGINFO DiskperfGuidList[] =
{
    {
        &DiskPerfGuid,
        1,
        0
    }
};

ULONG DiskperfGuidCount = 1;

NTSTATUS
FtQueryWmiRegInfo(IN PDEVICE_OBJECT DeviceObject,
                  OUT PULONG RegFlags,
                  OUT PUNICODE_STRING InstanceName,
                  OUT PUNICODE_STRING *RegistryPath,
                  OUT PUNICODE_STRING MofResourceName,
                  OUT PDEVICE_OBJECT *Pdo)
{
    PVOLUME_EXTENSION VolumeExtension;
    PAGED_CODE();

    //
    // Clear the instance name
    //
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    RtlInitEmptyUnicodeString(InstanceName, NULL, 0);

    //
    // Save the registry path
    //
    *RegistryPath = &VolumeExtension->RootExtension->RegistryPath;

    //
    // Save the PDO
    //
    *Pdo = DeviceObject;

    //
    // Set the flags
    //
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EXPENSIVE;
    return STATUS_SUCCESS;
}

NTSTATUS
FtQueryWmiDataBlock(IN PDEVICE_OBJECT DeviceObject,
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

NTSTATUS
FtRegisterDevice(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PVOLUME_EXTENSION VolumeExtension;
    PROOT_EXTENSION RootExtension;
    PAGED_CODE();

    //
    // Register us
    //
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    Status = IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);
    if (NT_SUCCESS(Status))
    {
        //
        // Check if we have an enable routine
        //
        RootExtension = VolumeExtension->RootExtension;
        if (RootExtension->PmWmiCounterContext.PmWmiCounterEnable)
        {
            //
            // Enable the timer to set it up, then disable it for now
            //
            RootExtension->PmWmiCounterContext.PmWmiCounterEnable(
                &VolumeExtension->WmiCounterContext);
            RootExtension->PmWmiCounterContext.PmWmiCounterDisable(
                &VolumeExtension->WmiCounterContext,
                FALSE,
                FALSE);
        }
    }

    //
    // Return status
    //
    return Status;
}
