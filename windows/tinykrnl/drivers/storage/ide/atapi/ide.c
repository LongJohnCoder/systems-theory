/*++

Copyright (c) Matthieu Suiche.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" THE TOP
    LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    All ATA Programming Interface Driver    

Environment:

    Kernel mode

Revision History:

    Matthieu Suiche

--*/
#include "precomp.h"

IDE_FDO_LIST IdeGlobalFdoList = {-1, NULL, NULL};
ULONG TotalFdoEntries;

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
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
IdePortStartIo(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    PROLOG();

    EPILOG();
}

/*++
 * @name FILLMEIN
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
IdePortUnload(IN PDRIVER_OBJECT DriverObject)
{
    PROLOG();

    EPILOG();
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
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortDispatch(IN PDEVICE_OBJECT DeviceObject,
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
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,
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
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortDispatchPower(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortDispatchPnp
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortDispatchPnp(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortAlwaysStatusSuccessIrp
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortAlwaysStatusSuccessIrp(IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortPassDownToNextDriver
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortPassDownToNextDriver(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortDispatchSystemControl
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortDispatchSystemControl(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortStatusSuccessAndPassDownToNextDriver
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortStatusSuccessAndPassDownToNextDriver(IN PDEVICE_OBJECT DeviceObject,
                                            IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortNoSupportIrp
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortNoSupportIrp(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortWmiSystemControl
 *
 * The IdePortWmiSystemControl routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortWmiSystemControl(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdeInitializeFdoList
 *
 * The IdeInitializeFdoList routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *		  FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
IdeInitializeFdoList(OUT PIDE_FDO_LIST FdoList)
{
    /* Sanity check */
    ASSERT(FdoList != NULL);

    /* Make sure the list is valid */
    if (FdoList->NumberOfEntry != -1)
    {
        /* Initialize the list and lock */
        FdoList->NumberOfEntry = 0;
        InitializeListHead(&FdoList->ListEntry);
        KeInitializeSpinLock(&FdoList->SpinLock);
    }
}

/*++
 * @name IdePortDetectLegacyController
 *
 * The IdePortDetectLegacyController routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks FILLMEIN.
 *
 *--*/
NTSTATUS
IdePortDetectLegacyController(IN PDRIVER_OBJECT DriverObject,
                              IN PUNICODE_STRING RegistryPath)
{
    PIOADDRESS Buffer;
    PCM_RESOURCE_LIST CmRessourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR List;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PCHANNEL_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status, Status2, Status3;
    IO_ADDRESS IoAddress;
    IO_ADDRESS_3 IoAddress3;
    UNICODE_STRING ScsiAdapter;
    BOOLEAN ConflitDetected, RetValue;
    ULONG baseIoAddress2Length, maxIdeDevice, nCounter,
        baseIoAddress1Length, AddressSpace;
    ULONG AddressSpace2;
    UCHAR DeviceNumber;
    PHYSICAL_ADDRESS PhysicalAddress, PhysicalAddress2;
    USHORT szBuffer[0x100];

    PROLOG();

    Status = IdePortOkToDetectLegacy(DriverObject);
    if (!NT_SUCCESS(Status)) return STATUS_UNSUCCESSFUL;

    Status = IdePortCreateDetectionList(DriverObject,
                                        &Buffer,
                                        &nCounter);
    if (!NT_SUCCESS(Status))
    {
        if (!Buffer) return Status;
        ExFreePool(Buffer);
    }

    CmRessourceList = ExAllocatePoolWithTag(PagedPool,
                                            sizeof(CM_RESOURCE_LIST),
                                            IDE_TAG);

    if (!CmRessourceList) return STATUS_NO_MEMORY;

    RtlZeroMemory(CmRessourceList, sizeof(CM_RESOURCE_LIST));

    RtlInitUnicodeString(&ScsiAdapter, L"ScsiAdapter");

    if (!nCounter)
    {
        if (!Buffer) return Status;

        ExFreePool(Buffer);
    }

    //
    // Big big loop
    //
    while (nCounter)
    {
        List = &CmRessourceList->List[0];

        AtapiBuildIoAddress(Buffer->IoAddress2,
                            Buffer->u04,
                            &IoAddress,
                            &IoAddress3,
                            &baseIoAddress1Length,
                            &baseIoAddress2Length,
                            &maxIdeDevice,
                            NULL);

        CmRessourceList->Count = 1;

        List->InterfaceType = Isa;
        List->BusNumber = 0;
        List->PartialResourceList.Version = 1;
        List->PartialResourceList.Revision = 1;
        List->PartialResourceList.Count = 3;

        Descriptor = &List->PartialResourceList.PartialDescriptors[0];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_10_BIT_DECODE |
                            CM_RESOURCE_PORT_IO;
        Descriptor->u.Generic.Length = baseIoAddress1Length;
        Descriptor->u.Generic.Start.LowPart = (ULONG)(Buffer->IoAddress2);
        Descriptor->u.Generic.Start.HighPart = 0;

        Descriptor = &List->PartialResourceList.PartialDescriptors[1];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_10_BIT_DECODE |
                            CM_RESOURCE_PORT_IO;
        Descriptor->u.Generic.Length = sizeof(UCHAR);
        Descriptor->u.Generic.Start.LowPart = (ULONG)(Buffer->u04);
        Descriptor->u.Generic.Start.HighPart = 0;

        Descriptor = &List->PartialResourceList.PartialDescriptors[2];
        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_LATCHED_BITS;
        Descriptor->u.Generic.Length |= 0xFFFFFFFF;
        Descriptor->u.Generic.Start.LowPart = (ULONG)(Buffer->Address);
        Descriptor->u.Generic.Start.HighPart = (ULONG)(Buffer->Address);

        Status2 = STATUS_SUCCESS;

        while (Status2 < STATUS_SEVERITY_WARNING)
        {
            Status = IoReportResourceForDetection(DriverObject,
                                                  CmRessourceList,
                                                  sizeof(CM_RESOURCE_LIST),
                                                  NULL,
                                                  0,
                                                  0,
                                                  &ConflitDetected);

            if (Status == STATUS_SUCCESS)
            {
                break;
            }
            else if (Status < STATUS_SUCCESS)
            {
                List->PartialResourceList.PartialDescriptors[0].Flags =
                    CM_RESOURCE_PORT_16_BIT_DECODE | CM_RESOURCE_PORT_IO;
                List->PartialResourceList.PartialDescriptors[1].Flags =
                    CM_RESOURCE_PORT_16_BIT_DECODE | CM_RESOURCE_PORT_IO;
            }
            else
            {
                IoReportResourceForDetection(DriverObject,
                                             NULL,
                                             0,
                                             NULL,
                                             NULL,
                                             0,
                                             &ConflitDetected);
                Status = STATUS_UNSUCCESSFUL;
            }
            Status2++;
        }

        if (NT_SUCCESS(Status))
        {
            Status3 = STATUS_SUCCESS;
            Status2 = STATUS_SUCCESS;

            Status = IdePortTranslateAddress(List->InterfaceType,
                                             List->BusNumber,
                                             List->PartialResourceList.
                                             PartialDescriptors[0].u.
                                             Generic.Start,
                                             List->PartialResourceList.
                                             PartialDescriptors[0].u.
                                             Generic.Length,
                                             &AddressSpace,
                                             &Status2,
                                             &PhysicalAddress);

            if (NT_SUCCESS(Status))
            {
                Status = IdePortTranslateAddress(List->InterfaceType,
                                                 List->BusNumber,
                                                 List->PartialResourceList.
                                                 PartialDescriptors[1].u.
                                                 Generic.Start,
                                                 List->PartialResourceList.
                                                 PartialDescriptors[1].u.
                                                 Generic.Length,
                                                 &AddressSpace2,
                                                 &Status3,
                                                 &PhysicalAddress2);

                if (NT_SUCCESS(Status))
                {
                    AtapiBuildIoAddress((PIO_ADDRESS_2)Status2,
                                        (PULONG)Status3,
                                        &IoAddress,
                                        &IoAddress3,
                                        &baseIoAddress1Length,
                                        &baseIoAddress2Length,
                                        &maxIdeDevice,
                                        NULL);

                    WRITE_PORT_UCHAR(IoAddress.Command, IDE_COMMAND_ATAPI_PACKET);

                    WRITE_PORT_UCHAR(IoAddress.Command, IDE_COMMAND_UNKNOW1);

                    if (READ_PORT_UCHAR(IoAddress3.Control) == 0xFF)
                        {
                            Status = STATUS_UNSUCCESSFUL;
                        }
                }
                else
                {
                    if(IdePortChannelEmpty(&IoAddress,
                                           &IoAddress3,
                                           maxIdeDevice) == FALSE)
                    {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    else
                    {
                        RetValue = FALSE;
                        if (maxIdeDevice > 0)
                        {
                            for (DeviceNumber = 0;
                                 DeviceNumber < maxIdeDevice;
                                 DeviceNumber++)
                            {
                                RetValue = IssueIdentify(&IoAddress,
                                                &IoAddress3,
                                                DeviceNumber,
                                                IDE_COMMAND_IDENTIFY,
                                                1,
                                                (PUSHORT)&szBuffer);

                                if (RetValue == TRUE) break;

                                RetValue = IssueIdentify(&IoAddress,
                                                &IoAddress3,
                                                DeviceNumber,
                                                IDE_COMMAND_ATAPI_IDENTIFY,
                                                1,
                                                (PUSHORT)&szBuffer);

                                if (RetValue == TRUE) break;
                            }
                        }

                        if (RetValue == FALSE)
                        {
                            Status = STATUS_UNSUCCESSFUL;
                        }
                    }
                }
            }

            if (!NT_SUCCESS(Status))
            {
                if (Status2 != STATUS_SUCCESS)
                {
                    IdePortFreeTranslatedAddress((PVOID)Status2,
                                        List->PartialResourceList.
                                        PartialDescriptors[0].u.
                                        Generic.Length,
                                        AddressSpace);
                }
                if (Status3 != STATUS_SUCCESS)
                {
                    IdePortFreeTranslatedAddress((PVOID)Status3,
                                        List->PartialResourceList.
                                        PartialDescriptors[1].u.
                                        Generic.Length,
                                        AddressSpace2);
                }
            }
            else
            {
                if ((List->PartialResourceList.
                    PartialDescriptors[0].
                    Flags | CM_RESOURCE_PORT_10_BIT_DECODE) ==
                    (List->PartialResourceList.
                    PartialDescriptors[0].
                    Flags))
                {
                    if(IdePortDetectAlias(&IoAddress) == FALSE)
                    {
                         List->PartialResourceList.
                             PartialDescriptors[0].Flags =
                            CM_RESOURCE_PORT_16_BIT_DECODE |
                            CM_RESOURCE_PORT_IO;

                        List->PartialResourceList.
                            PartialDescriptors[1].Flags =
                            CM_RESOURCE_PORT_16_BIT_DECODE |
                            CM_RESOURCE_PORT_IO;
                    }
                }
            }

            IoReportResourceForDetection(DriverObject,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL,
                                         0,
                                         &ConflitDetected);

            if (!NT_SUCCESS(Status)) continue;

            DeviceObject = NULL;
            Status = IoReportDetectedDevice(DriverObject,
                                        InterfaceTypeUndefined,
                                       -1,
                                       -1,
                                       CmRessourceList,
                                       0,
                                       0,
                                       &DeviceObject);

            if (!NT_SUCCESS(Status)) continue;

            Status = ChannelAddChannel(DriverObject,
                                  DeviceObject,
                                  &DeviceExtension);

        }
        nCounter--;
        Buffer += sizeof(IOADDRESS);
    }

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortOkToDetectLegacy
 *
 * The IdePortOkToDetectLegacy routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortOkToDetectLegacy(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING RegistryPath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    RTL_QUERY_REGISTRY_TABLE QueryTable;
    HANDLE Key;
    NTSTATUS Status;
    PVOID Reserved = NULL;
    ULONG EntryContext;

    PROLOG();

    RtlInitUnicodeString(&RegistryPath,
                         L"\\Registry\\Machine\\System\\CurrentControlSet"
                         L"\\Control\\Pnp");

    InitializeObjectAttributes(&ObjectAttributes,
                               &RegistryPath,
                               FILE_ATTRIBUTE_DEVICE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&QueryTable, sizeof(RTL_QUERY_REGISTRY_TABLE));

        QueryTable.EntryContext = &Reserved;
        QueryTable.DefaultData = &Reserved;
        QueryTable.QueryRoutine = NULL;
        QueryTable.Flags = RTL_QUERY_REGISTRY_DIRECT |
                           RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable.Name = L"DisableFirmwareMapper";
        QueryTable.DefaultType = REG_DWORD;
        QueryTable.DefaultLength = sizeof(ULONG);

        RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                               Key,
                               &QueryTable,
                               NULL,
                               NULL);

        ZwClose(Key);

        if (Reserved != NULL) return STATUS_SUCCESS;

        Status = IdePortGetParameterFromServiceSubKey(DriverObject,
                                                      L"LegacyDetection",
                                                      sizeof(ULONG),
                                                      TRUE,
                                                      &EntryContext,
                                                      0);

        if (NT_SUCCESS(Status)) return STATUS_SUCCESS;

        if (!EntryContext) return STATUS_UNSUCCESSFUL;

        *(PULONG)EntryContext = 0;
        Status = IdePortGetParameterFromServiceSubKey(DriverObject,
                                                      L"LegacyDetection",
                                                      sizeof(ULONG),
                                                      FALSE,
                                                      &EntryContext,
                                                      sizeof(ULONG));
    }

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortGetParameterFromServiceSubKey
 *
 * The IdePortGetParameterFromServiceSubKey routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortGetParameterFromServiceSubKey(IN PDRIVER_OBJECT DriverObject,
                                     IN LPWSTR Name,
                                     IN ULONG ContextOrType,
                                     IN BOOLEAN Flags,
                                     IN PULONG EntryContext,
                                     IN ULONG DataSize)
{
    LPWSTR QueryTableName;
    RTL_QUERY_REGISTRY_TABLE QueryTable;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    HANDLE DriverHandle;
    HANDLE Key;
    UCHAR Parameters[64];
    NTSTATUS Status;
    PROLOG();

    QueryTableName = Name;
    DriverHandle = (HANDLE)(DriverObject);

    PAGED_CODE();

    sprintf(Parameters, "Parameters");

    *EntryContext = 0;

    RtlInitAnsiString(&AnsiString, Parameters);

    Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);

    if (!NT_SUCCESS(Status)) return STATUS_UNSUCCESSFUL;

    Key = IdePortOpenServiceSubKey(DriverHandle, &UnicodeString);

    RtlFreeUnicodeString(&UnicodeString);

    if ( !Key)
        return STATUS_UNSUCCESSFUL;

    if (!Flags)
    {
        RtlInitUnicodeString(&UnicodeString, QueryTableName);

        Status = ZwSetValueKey(Key,
                               &UnicodeString,
                               (ULONG)NULL,
                               ContextOrType,
                               EntryContext,
                               DataSize);
    }
    else
    {
        RtlZeroMemory(&QueryTable,
                      sizeof(RTL_QUERY_REGISTRY_TABLE));

        QueryTable.Name = Name;
        QueryTable.QueryRoutine = &IdePortRegQueryRoutine;
        QueryTable.Flags = RTL_QUERY_REGISTRY_NOEXPAND |
                           RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable.EntryContext = EntryContext;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        Key,
                                        &QueryTable,
                                        (PVOID)ContextOrType,
                                        NULL);
    }

    EPILOG();
    return Status;
}

/*++
 * @name IdePortOpenServiceSubKey
 *
 * The IdePortOpenServiceSubKey routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
HANDLE
IdePortOpenServiceSubKey(IN HANDLE Handle,
                         IN PUNICODE_STRING ObjectName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    PVOID ContextArea;
    HANDLE HandleBis;
    PROLOG();

    ContextArea = IoGetDriverObjectExtension(Handle, &DriverEntry);

    if (!ContextArea) return FALSE;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)&ContextArea,
                               FILE_ATTRIBUTE_DEVICE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Handle,KEY_READ,&ObjectAttributes);

    if (!NT_SUCCESS(Status)) return FALSE;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)&ObjectName, 
                               FILE_ATTRIBUTE_DEVICE,
                               &Handle,
                               NULL);

    Status = ZwOpenKey(&HandleBis,KEY_READ,&ObjectAttributes);

    ZwClose(Handle);

    if (!NT_SUCCESS(Status)) return FALSE;

    return HandleBis;

    EPILOG();
}

/*++
 * @name IdePortRegQueryRoutine
 *
 * The IdePortRegQueryRoutine routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortRegQueryRoutine(IN PCWSTR ValueName,
                       IN ULONG ValueType,
                       IN PVOID ValueData,
                       IN SIZE_T ValueLength,
                       IN PVOID Context,
                       IN PVOID EntryContext)
{
    PROLOG();

    PAGED_CODE();

    if (ValueType != (ULONG)Context) return STATUS_UNSUCCESSFUL;

    switch (ValueType)
    {
        case REG_MULTI_SZ:
            *(PULONG)EntryContext = (ULONG)ExAllocatePoolWithTag(PagedPool,
                                                                 ValueLength,
                                                                 IDE_TAG);

            if (!EntryContext) return STATUS_UNSUCCESSFUL;

            RtlMoveMemory(EntryContext, ValueData, ValueLength);
            break;

        case REG_DWORD:
            *(PULONG)EntryContext = *(PULONG)ValueData;
            break;
    }

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortCloseServiceSubKey
 *
 * The IdePortCloseServiceSubKey routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortCloseServiceSubKey(OUT HANDLE Handle)
{
    return ZwClose(Handle);
}

/*++
 * @name IdePortCreateDetectionList
 *
 * The IdePortCreateDetectionList routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortCreateDetectionList(IN PDRIVER_OBJECT DriverObject,
                           IN PVOID *Buffer,
                           OUT PULONG Counter)
{
    PCONFIGURATION_INFORMATION ConfigInfo;
    PDETECTION_LIST DetectionList;
    PVOID Pool;
    ULONG nCounter;
    PROLOG();

    nCounter = 0;

    ConfigInfo = IoGetConfigurationInformation();

    Pool = ExAllocatePoolWithTag(PagedPool,
                                 sizeof(DETECTION_LIST) * 4,
                                 IDE_TAG);

    DetectionList = (PDETECTION_LIST)Pool;

    if (DetectionList)
    {
        *(PULONG)Buffer = 0;
        *Counter = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ConfigInfo->AtDiskPrimaryAddressClaimed)
    {
        DetectionList->Port1 = 0x1F0;
        DetectionList->Port2 = 0x3F6;
        DetectionList->IRQ = 0x0F;
        Counter++;
    }

    if (ConfigInfo->AtDiskSecondaryAddressClaimed)
    {
        DetectionList += sizeof(DETECTION_LIST);
        DetectionList->Port1 = 0x170;
        DetectionList->Port2 = 0x376;
        DetectionList->IRQ = 0x0F;
        Counter++;
    }

    DetectionList += sizeof(DETECTION_LIST);
    DetectionList->Port1 = 0x1E8;
    DetectionList->Port2 = 0x3EE;
    DetectionList->IRQ = 0x0B;
    Counter++;

    DetectionList += sizeof(DETECTION_LIST);
    DetectionList->Port1 = 0x168;
    DetectionList->Port2 = 0x36E;
    DetectionList->IRQ = 0x0A;
    Counter++;

    *Counter = nCounter;
    *Buffer = Pool;

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortCreateDetectionList
 *
 * The IdePortTranslateAddress routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortTranslateAddress(IN INTERFACE_TYPE InterfaceType,
                        IN ULONG BusNumber,
                        IN PHYSICAL_ADDRESS PhysicalAddress,
                        IN ULONG Length,
                        OUT PULONG AddressSpace,
                        OUT PNTSTATUS Status,
                        OUT PPHYSICAL_ADDRESS PhysicalAddress2)
{
    PROLOG();

    PhysicalAddress.QuadPart = (LONGLONG)NULL;
    *Status = 0;

    if (!HalTranslateBusAddress(InterfaceType,
                                BusNumber,
                                PhysicalAddress,
                                AddressSpace,
                                &PhysicalAddress))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (*AddressSpace == 1)
    {
        *Status = PhysicalAddress.LowPart;
        return STATUS_UNSUCCESSFUL;
    }

    MmMapIoSpace(PhysicalAddress,
                 Length,
                 MmNonCached);

    PhysicalAddress2->LowPart = PhysicalAddress.LowPart;
    PhysicalAddress2->HighPart = PhysicalAddress.HighPart;

    EPILOG();

    return *Status;
}

/*++
 * @name IdePortChannelEmpty
 *
 * The IdePortChannelEmpty routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
IdePortChannelEmpty(OUT PIO_ADDRESS IoAddress,
                    OUT PIO_ADDRESS_3 IoAddress3,
                    IN ULONG maxIdeDevice)
{
    ULONG Counter;
    BOOLEAN Status;
    UCHAR IdeStatus;

    PROLOG();

    Counter = 0;

    if (maxIdeDevice <= 0)
        return TRUE;


    while (Counter < maxIdeDevice)
    {
        WRITE_PORT_UCHAR(IoAddress->Command, IDE_PORT_CHANNEL_MASK(Counter));
        IdeStatus = READ_PORT_UCHAR(IoAddress->Status);

        IoAddress->IdeStatus = IdeStatus;
        switch (IdeStatus)
        {
            case 0xFF:
            case 0xFE:
                Counter++;
            break;
            default:
                IdePortpWaitOnBusyEx(IoAddress,
                                     &IoAddress3->IdeStatus,
                                     0xFF);
                switch (IoAddress3->IdeStatus)
                {
                    case 0x00:
                    case 0xFE:
                        Status = FALSE;
                    case 0xFF:
                        Counter++;
                    break;
                    default:
                        DbgPrint("IdePortChannelEmpty: channel looks busy.\n");
                        WRITE_PORT_UCHAR((PUCHAR)IoAddress3->Control, IDE_DC_RESET_CONTROLLER);

                        KeStallExecutionProcessor(10);

                        WRITE_PORT_UCHAR((PUCHAR)IoAddress3->Control, IDE_DC_REENABLE_CONTROLLER);
                        WRITE_PORT_UCHAR((PUCHAR)IoAddress->Command, IDE_COMMAND_NULL);

                        IdePortpWaitOnBusyEx(IoAddress,
                                             &IoAddress3->IdeStatus,
                                             0xFF);

                        if (IoAddress3->IdeStatus == 0xFF)
                        {
                            Counter++;
                        }
                        else 
                        {
                            Status = FALSE;
                        }

                    break;
                }
            break;
        }
    }

    if (Status == FALSE)
    {
        Status = IdePortIdentifyDevice(IoAddress,
                                       IoAddress3,
                                       maxIdeDevice);
    }

    EPILOG();
    return Status;
}

/*++
 * @name IdePortpWaitOnBusyEx
 *
 * The IdePortpWaitOnBusyEx routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortpWaitOnBusyEx(OUT PIO_ADDRESS IoAddress,
                     OUT PUCHAR StatusPort,
                     IN UCHAR StatusValue)
{
    ULONG Counter, LoopCounter;
    UCHAR IdeStatus;

    PROLOG();

    Counter = 0;
    LoopCounter = 0;

    while (Counter < 2)
    {

        while (LoopCounter < 200000)
        {
            IdeStatus = READ_PORT_UCHAR(IoAddress->Status);

            if ((IdeStatus == StatusValue) || 
                (IdeStatus >= 0))
            {

                *StatusPort = IdeStatus;
                DbgPrint("WaitOnBusy failed.\n");
                return STATUS_UNSUCCESSFUL;

            }
            else
            {

                KeStallExecutionProcessor(5);
                LoopCounter++;
                DbgPrint("ATAPI: after 1 sec wait, device is still busy.\n");

            }

        }

    }

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name IdePortIdentifyDevice
 *
 * The IdePortIdentifyDevice routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
IdePortIdentifyDevice(OUT PIO_ADDRESS IoAddress,
                      OUT PIO_ADDRESS_3 IoAddress3,
                      IN ULONG maxIdeDevice)
{
    BOOLEAN Status;
    ULONG DeviceNumber;
    ULONG Counter2;
    ULONG Counter3;

    PROLOG();

    DeviceNumber = 0;
    Counter2 = 4;
    Status = TRUE;

    for (DeviceNumber = 0, Status = TRUE;
         DeviceNumber < maxIdeDevice, Status != FALSE;
         DeviceNumber++)
    {

        WRITE_PORT_UCHAR(IoAddress->Command, IDE_PORT_CHANNEL_MASK(DeviceNumber));
        WRITE_PORT_UCHAR(IoAddress->LBAHigh, 0xAA);
        WRITE_PORT_UCHAR(IoAddress->LBAMid, 0x55);

        if ((READ_PORT_UCHAR(IoAddress->LBAHigh) == 0xAA) &&
            (READ_PORT_UCHAR(IoAddress->LBAMid) == 0x55))
        {

            Status = FALSE;
            continue;

        }

        IoAddress->IdeStatus = READ_PORT_UCHAR(IoAddress->Status);
        DbgPrint("IdePortChannelEmpty: status read back from Master (%x)",
                 IoAddress->IdeStatus);

        if (!(IoAddress->IdeStatus & IDE_STATUS_BUSY))
        {

            for (Counter3 = 0; Counter3 < 10; Counter3++)
            {

                KeStallExecutionProcessor(1000);
                IoAddress->IdeStatus = READ_PORT_UCHAR(IoAddress->Status);
                DbgPrint("IdePortChannelEmpty: First access to status %x",
                         IoAddress->IdeStatus);

                if ((IoAddress->IdeStatus & IDE_STATUS_BUSY)) break;

            }

            Counter2--;

            if (Status != FALSE) continue;

            if ((IoAddress->IdeStatus & IDE_STATUS_BUSY)) continue;

        }

        DeviceNumber++;
        WRITE_PORT_UCHAR(IoAddress->Command, IDE_PORT_CHANNEL_MASK(DeviceNumber));
        WRITE_PORT_UCHAR(IoAddress->LBAHigh, 0xAA);
        WRITE_PORT_UCHAR(IoAddress->LBAMid, 0x55);

        if ( (READ_PORT_UCHAR(IoAddress->LBAHigh) == 0xAA) &&
            (READ_PORT_UCHAR(IoAddress->LBAMid) == 0x55)
           )
        {

            Status = FALSE;
            continue;

        }

        DbgPrint("IdePortChannelEmpty: status read back from Slave"
            "(%x)",
           READ_PORT_UCHAR(IoAddress->Status));

    }

    EPILOG();
    return Status;
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
BOOLEAN
IssueIdentify(OUT PIO_ADDRESS IoAddress,
              OUT PIO_ADDRESS_3 IoAddress3,
              IN UCHAR DeviceNumber,
              IN UCHAR Command,
              IN UCHAR Control,
              OUT PUSHORT Buffer)
{
    UCHAR IdeStatus1, IdeStatus2, IdeError;
    ULONG Counter1, Counter2, Counter3, Counter4;
    USHORT szBuffer[0x100];

    PROLOG();

    RtlZeroMemory(Buffer, sizeof(PUSHORT));

    WRITE_PORT_UCHAR(IoAddress->Command, 
        IDE_PORT_CHANNEL_MASK(DeviceNumber));

    IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);

    if (Command == IDE_COMMAND_IDENTIFY)
    {
        IdeStatus1 &= ~(IDE_STATUS_ERROR | IDE_STATUS_INDEX);
        DbgPrint("IssueIdentify: Checking for IDE. Status (%x)\n",
            IdeStatus1);

        if (IdeStatus1 != IDE_STATUS_IDLE)
        {
           IdeHardReset(IoAddress,
                        IoAddress3,
                        Control,
                        1);

           WRITE_PORT_UCHAR(IoAddress->Command, 
               IDE_PORT_CHANNEL_MASK(DeviceNumber));

           //
           // Check Signature
           //
           if ((READ_PORT_UCHAR(IoAddress->LBAMid) == 0x14) &&
               (READ_PORT_UCHAR(IoAddress->LBAHigh) == 0xEB))
           {
               return FALSE;
           }

           IdeStatus2 = READ_PORT_UCHAR(IoAddress->Status);
           IdeStatus2 &= ~IDE_STATUS_INDEX;

           if (IdeStatus2 != IDE_STATUS_IDLE) return FALSE;
        }
    }
    else 
    {
        DbgPrint("IssueIdentify: Checking for ATAPI. Status (%x)",
            IdeStatus1);
    }

    WRITE_PORT_UCHAR(IoAddress->LBAHigh, (UCHAR)2);

    WRITE_PORT_UCHAR(IoAddress->LBAMid, (UCHAR)0);

    for (Counter3 = 0; Counter3 < 2; Counter3++)
    {
        for (Counter2 = 0; Counter2 < 10; Counter2++)
        {
            for (Counter1 = 0; Counter1 < 25000; Counter1++)
            {
               IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);
               
               if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

               KeStallExecutionProcessor(40);
            }

            if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

            DbgPrint("ATAPI: after 1 sec wait, device is still b"
                "usy with 0x%x status = 0x%x",
                IoAddress->u00,
                IdeStatus1);
        }

        if (IdeStatus1 & IDE_STATUS_BUSY)
        {
            DbgPrint("WaitOnBusy failed. 0x%x status = 0x%x",
                IoAddress->u00,
                IdeStatus1);
        }

        WRITE_PORT_UCHAR(IoAddress->Status, (UCHAR)Command);

        for (Counter2 = 0; Counter2 < 10; Counter2++)
        {
            for (Counter1 = 0; Counter1 < 25000; Counter1++)
            {
               IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);
               
               if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

               KeStallExecutionProcessor(40);
            }

            if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

            DbgPrint("ATAPI: after 1 sec wait, device is still "
                "busy with 0x%x status = 0x%x",
                IoAddress->u00,
                IdeStatus1);
        }

        if (IdeStatus1 & IDE_STATUS_BUSY)
        {
            DbgPrint("WaitOnBusy failed. 0x%x status = 0x%x",
                IoAddress->u00,
                IdeStatus1);
        }

        if (IdeStatus1 & IDE_STATUS_ERROR) continue;

        for (Counter1 = 0; Counter1 < 4; Counter1++)
        {
            for (Counter2 = 0; Counter2 < 1000; Counter2++)
            {
               IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);
               
               if (IdeStatus1 & IDE_STATUS_BUSY)
               {
                    KeStallExecutionProcessor(100);
               } 
               else
               {
                   if (IdeStatus1 & IDE_STATUS_BUSY)
                   {
                       break;
                   }
                   KeStallExecutionProcessor(200);
               }
               if (IdeStatus1 & IDE_STATUS_BUSY) break;
            }

            if (Command == IDE_COMMAND_IDENTIFY)
            {
                //
                // Check the signature
                //
                if ((READ_PORT_UCHAR(IoAddress->LBAMid) == 0x14) &&
                    (READ_PORT_UCHAR(IoAddress->LBAHigh) == 0xEB))
                {
                       return FALSE;
                }
            }

            for (Counter4 = 0; Counter4 < 10; Counter4++)
            {
                for (Counter2 = 0; Counter2 < 25000; Counter2++)
                {
                   IdeStatus1 = READ_PORT_UCHAR(
                       IoAddress->Status);
                   
                   if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

                   KeStallExecutionProcessor(40);
                }

                if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

                DbgPrint("ATAPI: after 1 sec wait, device is still "
                    "busy with 0x%x status = 0x%x",
                    IoAddress->u00,
                    IdeStatus1);
            }

            if (IdeStatus1 & IDE_STATUS_BUSY)
            {
                DbgPrint("WaitOnBusy failed. 0x%x status = 0x%x",
                    IoAddress->u00,
                    IdeStatus1);
            }
        }

        if (IdeStatus1 & IDE_STATUS_BUSY)
        {
            IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);

           //
           // Check Signature
           //
            if ((READ_PORT_UCHAR(IoAddress->LBAMid) == 0x14) &&
                (READ_PORT_UCHAR(IoAddress->LBAHigh) == 0xEB))
            {
                   return FALSE;
            }
        }

        if ((Counter1 != 4) &&
            (Counter3 != 0))
        {
            IdeError = READ_PORT_UCHAR(IoAddress->Error);

            DbgPrint("IssueIdentify:"
                "DRQ never asserted (%x). Error reg (%x)",
                IdeStatus1,
                IdeError);

            WRITE_PORT_UCHAR(IoAddress->Command,
                IDE_PORT_CHANNEL_MASK(DeviceNumber));

            KeStallExecutionProcessor(500);

            WRITE_PORT_UCHAR(IoAddress->Status, IDE_STATUS_DRQ);

            KeStallExecutionProcessor(500);

            WRITE_PORT_UCHAR(IoAddress->Command,
                IDE_PORT_CHANNEL_MASK(DeviceNumber));

            for (Counter2 = 0; Counter2 < 10; Counter2)
            {
                for (Counter4; Counter4 < 25000; Counter4++)
                {
                   IdeStatus1 = READ_PORT_UCHAR(
                       IoAddress->Status);
                   
                   if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

                   KeStallExecutionProcessor(40);
                }

                if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

                DbgPrint("ATAPI: after 1 sec wait, device is still "
                    "busy with 0x%x status = 0x%x",
                    IoAddress->u00,
                    IdeStatus1);
            }

            if (IdeStatus1 & IDE_STATUS_BUSY)
            {
                DbgPrint("WaitOnBusy failed. 0x%x status = 0x%x",
                    IoAddress->u00,
                    IdeStatus1);
            }

            KeStallExecutionProcessor(500);

            if (Control != 0)
                WRITE_PORT_UCHAR(IoAddress3->Control, IDE_DC_DISABLE_INTERRUPTS);

            IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);

            DbgPrint("IssueIdentify: Status after soft reset (%x)",
                IdeStatus1);
        }
        else break;
    }

    if (IdeStatus1 & IDE_STATUS_ERROR)
    {
        return FALSE;
    }

    DbgPrint("IssueIdentify: Status before read words %x\n",
            IdeStatus1);

    for (Counter2 = 0; Counter2 < 10; Counter2++)
    {
        for (Counter1 = 0; Counter1 < 25000; Counter1++)
        {
           IdeStatus1 = READ_PORT_UCHAR(
               IoAddress->Status);
           
           if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

           KeStallExecutionProcessor(40);
        }
        
        if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

        DbgPrint("ATAPI: after 1 sec wait, device is still "
            "busy with 0x%x status = 0x%x",
            IoAddress->u00,
            IdeStatus1);
    }

    if (IdeStatus1 & IDE_STATUS_BUSY)
    {
        DbgPrint("WaitOnBusy failed. 0x%x status = 0x%x",
            IoAddress->u00,
            IdeStatus1);
    }

    if (!(IdeStatus1 & IDE_STATUS_BUSY))
    {
        return FALSE;
    }

    READ_PORT_BUFFER_USHORT(IoAddress->Port04,
        (PUSHORT)&szBuffer,
        sizeof(szBuffer));

    memmove(Buffer, &szBuffer, sizeof(szBuffer) * sizeof(USHORT));

    for (Counter2 = 0; Counter2 < 10; Counter2++)
    {
        for (Counter1 = 0; Counter1 < 25000; Counter1++)
        {
           IdeStatus1 = READ_PORT_UCHAR(
               IoAddress->Status);
           
           if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

           KeStallExecutionProcessor(40);
        }
        
        if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

        DbgPrint("ATAPI: after 1 sec wait, device is still "
            "busy with 0x%x status = 0x%x",
            IoAddress->u00,
            IdeStatus1);
    }

    if (IdeStatus1 & IDE_STATUS_BUSY)
    {
        DbgPrint("WaitOnBusy failed. 0x%x status = 0x%x",
            IoAddress->u00,
            IdeStatus1);
    }

    for (Counter1 = 0; Counter1 < 0x10000; Counter1++)
    {
        IdeStatus1 = READ_PORT_UCHAR(IoAddress->Status);

        if (!(IdeStatus1 & IDE_STATUS_BUSY)) break;

        READ_PORT_USHORT(IoAddress->Port04);
    }

    DbgPrint("IssueIdentify: Status after read words (%x)",
        IdeStatus1);

    EPILOG();

    return TRUE;
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
IdeHardReset(OUT PIO_ADDRESS IoAddress,
             OUT PIO_ADDRESS_3 IoAddress3,
             IN ULONG Control,
             IN ULONG Counter)
{
    ULONG Counter1, Counter2;
    UCHAR IdeStatus, IdeCommand;

    PROLOG();

    DbgPrint("IdeHardReset: Resetting controller.\n");

    WRITE_PORT_UCHAR(IoAddress->Command, IDE_COMMAND_ATAPI_PACKET);

    WRITE_PORT_UCHAR(IoAddress3->Control, IDE_DC_DISABLE_INTERRUPTS | IDE_DC_RESET_CONTROLLER);

    KeStallExecutionProcessor(10);

    if (!Control) Control = 1;
    Control = ((Control - 1) & IDE_DC_DISABLE_INTERRUPTS);

    WRITE_PORT_UCHAR(IoAddress3->Control, (UCHAR)Control);

    KeStallExecutionProcessor(1);

    if (Counter == 0) return STATUS_SUCCESS;

    Counter1 = 0;

    for (Counter1 = 0; Counter1 < 5000; Counter1++)
    {
        IdeStatus = READ_PORT_UCHAR(IoAddress->Status);

        if (!(IdeStatus & IDE_STATUS_BUSY)) break;

        KeStallExecutionProcessor(100);
    }

    if (Counter1 == 5000)
    {
        DbgPrint("WaitOnBusyUntil failed status = 0x%x",
                 IdeStatus);
    }

    WRITE_PORT_UCHAR(IoAddress->Command, IDE_COMMAND_ATAPI_PACKET);

    IdeCommand = READ_PORT_UCHAR(IoAddress->Command);

    if (IdeCommand != IDE_COMMAND_ATAPI_PACKET)
    {
        KeStallExecutionProcessor(1000);
        WRITE_PORT_UCHAR(IoAddress->Command, IDE_COMMAND_UNKNOW1);
    }

    for (Counter1 = 0; Counter1 < 31; Counter1++)
    {
        for (Counter2 = 0; Counter2 < 2500; Counter2++)
        {
            IdeStatus = READ_PORT_UCHAR(IoAddress->Status);

            if (!(IdeStatus & IDE_STATUS_BUSY)) break;

            KeStallExecutionProcessor(400);
        }

        if ((UCHAR)IdeStatus == 0xFF) break;

        if (IdeStatus == IDE_STATUS_BUSY) return STATUS_SUCCESS;

        DbgPrint(
            "ATAPI: IdeHardReset WaitOnBusy failed. status = 0x%x\n",
            IdeStatus);

    }

    if (IdeStatus & IDE_STATUS_BUSY)
    {
        return STATUS_SUCCESS;
    }

    DbgPrint(
        "ATAPI: IdeHardReset WaitOnBusy failed. status = 0x%x\n",
        IdeStatus);

    EPILOG();

    return STATUS_UNSUCCESSFUL;
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
VOID
IdePortFreeTranslatedAddress(IN PVOID BaseAddress,
                             IN SIZE_T NumberOfBytes,
                             IN ULONG AddressSpace)
{
    PROLOG();

    if ((BaseAddress != NULL) && (AddressSpace == 0))
    {
    MmUnmapIoSpace(BaseAddress, NumberOfBytes);
    }

    EPILOG();
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
BOOLEAN
IdePortDetectAlias(IN PIO_ADDRESS IoAddress)
{
    UCHAR ReadedPortValue;

    PROLOG();

    WRITE_PORT_UCHAR(IoAddress->LBAHigh, 0xAA);

    WRITE_PORT_UCHAR(IoAddress->LBAMid, 0x55);

    if((READ_PORT_UCHAR(IoAddress->LBAHigh + 0x8000) != 0xAA) ||
       (READ_PORT_UCHAR(IoAddress->LBAMid + 0x8000) != 0x55))
    {
        return FALSE;
    }

    EPILOG();

    return TRUE;
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
VOID
IdeAddToFdoList(PIDE_FDO_LIST GlobalFdoList,
                PCHANNEL_EXTENSION ChannelExtension)
{
    KIRQL OldIrql;

    PROLOG();

    KeAcquireSpinLock(&GlobalFdoList->SpinLock, &OldIrql);

    InsertHeadList(&ChannelExtension->ListEntry,
        &GlobalFdoList->ListEntry);

    KeReleaseSpinLock(&GlobalFdoList->SpinLock, OldIrql);

    EPILOG();
}