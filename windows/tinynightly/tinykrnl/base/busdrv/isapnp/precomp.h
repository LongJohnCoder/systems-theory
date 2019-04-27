/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    PnP ISA Bus Extender

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Mar-2006
    Aleksey Bragin - 

--*/
#define INITGUID
#include "ntddk.h"
#include "wdmguid.h"
#include "stdarg.h"

//
// Only compile debug prints for Checked builds
//
#if DBG
#define PipDebugPrint           _PipDebugPrint
#define PipDebugPrintContinue   _PipDebugPrintContinue
#else
#define PipDebugPrint
#define PipDebugPrintContinue
#endif

//
// Device Flags
//
#define DF_DELETED          0x1
#define DF_REMOVED          0x2
#define DF_NOT_FUNCTIONING  0x4
#define DF_ENUMERATED       0x8
#define DF_ACTIVATED        0x10
#define DF_QUERY_STOPPED    0x20
#define DF_SURPRISE_REMOVED 0x40
#define DF_PROCESSING_RDP   0x80
#define DF_STOPPED          0x100
#define DF_RESTARTED_MOVED  0x200
#define DF_RESTARTED_NOMOVE 0x400
#define DF_REQ_TRIMMED      0x800
#define DF_READ_DATA_PORT   0x40000000
#define DF_BUS              0x80000000

//
// Structure describing each card
//
typedef struct _CARD_INFORMATION
{
    SINGLE_LIST_ENTRY CardList;                     // 0x00
    USHORT CardSelectNumber;                        // 0x04
    ULONG NumberLogicalDevices;                     // 0x08
    SINGLE_LIST_ENTRY LogicalDeviceList;            // 0x0C
    PVOID CardData;                                 // 0x10
    ULONG CardDataLength;                           // 0x14
    ULONG CardFlags;                                // 0x18
} CARD_INFORMATION, *PCARD_INFORMATION;

//
// Structure describing each device
//
typedef struct _DEVICE_INFORMATION
{
    ULONG Flags;                                    // 0x00
    ULONG DevicePowerState;                         // 0x04
    PDEVICE_OBJECT PhysicalDeviceObject;            // 0x08
    struct _PI_BUS_EXTENSION *ParentDeviceExtension;// 0x0C
    SINGLE_LIST_ENTRY DeviceList;                   // 0x10
    PVOID ResourceRequirements;                     // 0x14
    PCARD_INFORMATION CardInformation;              // 0x18
    SINGLE_LIST_ENTRY LogicalDeviceList;            // 0x1C
    USHORT LogicalDeviceNumber;                     // 0x20
    PVOID DeviceData;                               // 0x24
    ULONG DeviceDataLength;                         // 0x28
    PVOID BootResources;                            // 0x2C
    ULONG BootResourcesLength;                      // 0x30
    PVOID AllocatedResources;                       // 0x34
    HANDLE LogConfHandle;                           // 0x38
    PCHAR CrashDump;                                // 0x3C
    PCHAR Paging;                                   // 0x40
} DEVICE_INFORMATION, *PDEVICE_INFORMATION;

//
// Device Extension Structure
//
typedef struct _PI_BUS_EXTENSION
{
    ULONG Flags;
    ULONG NumberCSNs;
    ULONG ReadDataPort;
    ULONG DataPortMapped;
    ULONG AddressPort;
    ULONG AddrPortMapped;
    ULONG CommandPort;
    ULONG CmdPortMapped;
    ULONG u20; 
    SINGLE_LIST_ENTRY DeviceList;
    SINGLE_LIST_ENTRY CardList;
    PDEVICE_OBJECT PhysicalBusDevice;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT NextLowerDriver;
    ULONG BusNumber;
    ULONG SystemPowerState;
    ULONG DevicePowerState;
} PI_BUS_EXTENSION, *PPI_BUS_EXTENSION;

//
// Single Linked List of all Device Extensions
//
typedef struct _BUS_EXTENSION_LIST
{
    struct _BUS_EXTENSION_LIST *Next;
    PPI_BUS_EXTENSION DeviceExtension;
} BUS_EXTENSION_LIST, *PBUS_EXTENSION;

//
// Externals
//
extern KEVENT IsaBusNumberLock;
extern PRTL_BITMAP BusNumBM;
extern PDRIVER_OBJECT PipDriverObject;
extern UNICODE_STRING PipRegistryPath;
extern ULONG ActiveIsaCount;
extern PDEVICE_INFORMATION PipRDPNode;
extern PULONG PipReadDataPort;
extern KEVENT PipDeviceTreeLock;
extern BOOLEAN PipIsolationDisabled;
extern PHYSICAL_ADDRESS ADDRESS_PORT;
extern PHYSICAL_ADDRESS BusAddress;

//
// Prototypes for dispatch.c
//
VOID
PiUnload(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
PiAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

NTSTATUS
PiDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiDispatchDevCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for init.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

BOOLEAN
PipIsIsolationDisabled(
    VOID
);

//
// Prototypes for misc.c
//
BOOLEAN
PiNeedDeferISABridge(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

VOID
PipResetGlobals(
    VOID
);

NTSTATUS
PipOpenRegistryKey(
    OUT PHANDLE KeyHandle,
    IN HANDLE RootHandle,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
);

NTSTATUS
PipGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *FullInformation
);

VOID
_PipDebugPrint(
    IN ULONG Level,
    IN PCHAR Format,
    ...
);

VOID
_PipDebugPrintContinue(
    IN ULONG Level,
    IN PCHAR Format,
    ...
);

VOID
PipLockDeviceDatabase(
    VOID
);

VOID
PipUnlockDeviceDatabase(
    VOID
);

//
// Prototypes for fdopnp.c
//
NTSTATUS
PiDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
PipCompleteRequest(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Information
);

NTSTATUS
PiDispatchPnpPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryInterfaceFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryDeviceRelationsFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiCancelRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiRemoveFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiCancelRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryRemoveStopFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiStartFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryPnpDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiSurpriseRemoveFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryLegacyBusInformationFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipPassIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for pdopnp.c
//
NTSTATUS
PiStartPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiRemovePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiCancelRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiCancelRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryDeviceRelationsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryCapabilitiesPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryResourcesPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryResourceRequirementsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryDeviceTextPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiFilterResourceRequirementsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryIdPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiQueryBusInformationPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiDeviceUsageNotificationPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiSurpriseRemovePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiIrpNotSupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for resource.c
//
NTSTATUS
PipRebuildInterfaces(
    IN PPI_BUS_EXTENSION DeviceExtension
);

//
// Prototypes for translate.c
//
NTSTATUS
PiQueryInterface(
    IN PPI_BUS_EXTENSION DeviceExtension,
    IN PIRP Irp
);

NTSTATUS
FindInterruptTranslator(
    IN PPI_BUS_EXTENSION DeviceExtension,
    IN PIRP Irp
);

//
// Prototypes for bus.c
//
VOID
PipInitializeDeviceInfo(
    IN PDEVICE_INFORMATION DeviceInformation,
    IN PCARD_INFORMATION CardInformation,
    IN USHORT LogicalDeviceNumber
);

NTSTATUS
PipCreateReadDataPort(
    IN PPI_BUS_EXTENSION DeviceExtension
);

//
// Prototypes for power.c
//
NTSTATUS
PiDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipPowerIrpNotSupportedPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipPassPowerIrpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PiDispatchPowerPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipPowerIrpNotSupportedPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipSetQueryPowerStateFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipSetPowerStatePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PipQueryPowerStatePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);
