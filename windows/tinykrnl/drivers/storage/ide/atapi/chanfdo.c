/*++

Copyright (c) Matthieu Suiche.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" THE TOP
    LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    All ATA Programming Interface Driver <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Matthieu Suiche

--*/
#include "precomp.h"

extern ULONG TotalFdoEntries;

/*++
 * @name FILLMEIN
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelAddDevice(IN PDRIVER_OBJECT DriverObject,
                 IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelStartDevice
 *
 * The ChannelStartDevice routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelStartDevice(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelRemoveDevice
 *
 * The ChannelRemoveDevice routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelStopDevice
 *
 * The ChannelStopDevice routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelStopDevice(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)

{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryDeviceRelations
 *
 * The ChannelQueryDeviceRelations routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryDeviceRelations(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryId
 *
 * The ChannelQueryId routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryId(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelUsageNotification
 *
 * The ChannelUsageNotification routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelUsageNotification(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelFilterResourceRequirements
 *
 * The ChannelFilterResourceRequirements routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelFilterResourceRequirements(IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryPnPDeviceState
 *
 * The ChannelQueryPnPDeviceState routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryPnPDeviceState(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelSurpriseRemoveDevice
 *
 * The ChannelSurpriseRemoveDevice routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelSurpriseRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}
/*++
 * @name FILLMEIN
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return FILLMEIN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelAddChannel(IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT DeviceObject,
                  OUT PCHANNEL_EXTENSION *DeviceExtension)
{
    PCHANNEL_EXTENSION ChannelExtension;
    PDEVICE_OBJECT DeviceObject1;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    WCHAR Buffer[64];

    PROLOG();

    PAGED_CODE();

    swprintf((PWCHAR)&Buffer,
              L"\\Device\\Ide\\IdePort%d",
              TotalFdoEntries);

    RtlInitUnicodeString(&DeviceName, (PCWSTR)&Buffer);

    Status = IoCreateDevice(DriverObject,
                            sizeof(CHANNEL_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject1);

    if (!NT_SUCCESS(Status)) return Status;

    RtlZeroMemory(DeviceObject1->DeviceExtension,
        sizeof(CHANNEL_EXTENSION));

    ChannelExtension = DeviceObject1->DeviceExtension;

    ChannelExtension->DeviceObject2 = DeviceObject;
    ChannelExtension->DriverObject = DriverObject;
    ChannelExtension->DeviceObject3 = DeviceObject1;
    ChannelExtension->Reserved1 = &ChannelExtension->Reserved2;

    ChannelExtension->IdePortPassDownToNextDriver = 
        IdePortPassDownToNextDriver;
    ChannelExtension->FdoPnpDispatchTable = FdoPnpDispatchTable;
    ChannelExtension->FdoPowerDispatchTable = FdoPowerDispatchTable;
    ChannelExtension->FdoWmiDispatchTable = FdoWmiDispatchTable;

    ChannelExtension->DeviceObject1 = IoAttachDeviceToDeviceStack(
                                        DeviceObject1,
                                        DeviceObject);

    if (!ChannelExtension->DeviceObject1)
    {
        IoDeleteDevice(DeviceObject1);
        DbgPrint("DeviceObject %x returnd status %x from AddDevice",
                DeviceObject1,
                STATUS_UNSUCCESSFUL);
        return STATUS_UNSUCCESSFUL;
    }

    DeviceObject1->AlignmentRequirement = ChannelExtension->
                                            DeviceObject1->
                                            AlignmentRequirement;

    if (DeviceObject1->AlignmentRequirement == FILE_BYTE_ALIGNMENT)
    {
        DeviceObject1->AlignmentRequirement = FILE_WORD_ALIGNMENT;
    }

    ChannelExtension->TotalFdoEntries = ++TotalFdoEntries;

    *DeviceExtension = ChannelExtension;

    IdeAddToFdoList(&IdeGlobalFdoList, ChannelExtension);

    DeviceObject1->Flags &= ~DO_DEVICE_INITIALIZING;

    EPILOG();
    return Status;
}