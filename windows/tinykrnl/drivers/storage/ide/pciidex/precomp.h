/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    All integrated drive electronics (IDE) controller drivers must implement a
    series of standard routines that implement hardware-specific functionality.
    The PciIdeX library facilitates the development of these routines in a
    platform-independent manner.

Environment:

    Kernel mode

Revision History:

    Evgeny Pinchuk - Started Implementation - 22-Feb-06

--*/
#define INITGUID
#include "stdio.h"
#include "stdarg.h"
#include "ntifs.h"
#include "wdmguid.h"
#include "ide.h"

//
// Temporary debugging macro
//
#define NtUnhandled()                           \
{                                               \
    DbgPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                            \
}

/* Constants */
#define PCI_IDE_COMPONENT_ID    58
#define LSZ                     sizeof(ULONG)
#define HEADERSIZE              (FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific)) / LSZ
#define PCI_IDE_POOL_TAG        0x58656449

DEFINE_GUID(GUID_PCI_NATIVE_IDE_INTERFACE,
            0x98F37D63L, 0x42AE, 0x4AD9,
            0x8C, 0x36, 0x93, 0x2D, 0x28, 0x38, 0x3D, 0xF8);

typedef
VOID
(*PINTERRUPT_CONTROL_FUNC)(
    IN ULONG Unknown,
    IN ULONG Unknown2
);

//
// Structures
//
typedef struct _POWER_COMPLETION_CONTEXT
{
    KEVENT Event;
    NTSTATUS Status;
} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;

typedef struct _PCI_IDE_EXTENSIONS
{
    PCONTROLLER_PROPERTIES HwGetControllerProperties;
    ULONG ExtensionSize;
} PCI_IDE_EXTENSIONS, *PPCI_IDE_EXTENSIONS;

typedef struct _PCI_NATIVE_IDE_INTERFACE
{
    ULONG Unknown1;
    ULONG Unknown2;
    ULONG Unknown3;
    ULONG Unknown4;
    PINTERRUPT_CONTROL_FUNC InterruptControl;
} PCI_NATIVE_IDE_INTERFACE, *PPCI_NATIVE_IDE_INTERFACE;

typedef struct _PCI_FDO_INTERRUPT_CONTEXT
{
    struct FDO_EXTENSION *FdoExtension;
    ULONG Channel;
} PCI_FDO_INTERRUPT_CONTEXT, *PPCI_FDO_INTERRUPT_CONTEXT;

typedef struct _FDO_EXTENSION
{
    PDEVICE_OBJECT AttacheeDeviceObject;            // 0x0
    PDEVICE_OBJECT PhysicalDeviceObject;            // 0x4
    PDRIVER_OBJECT DriverObject;                    // 0x8
    PDEVICE_OBJECT DeviceObject;                    // 0xC
    ULONG Unknown1;                                 // 0x10
    ULONG Unknown2;                                 // 0x14
    ULONG Unknown3;                                 // 0x18
    ULONG SystemPowerRo;                            // 0x1C
    ULONG DevicePowerRo;                            // 0x20
    ULONG Unknown6[40];                             // 0x24
    PDRIVER_DISPATCH DefaultFunction;               // 0x4C
    PDRIVER_DISPATCH *PnpDispatchTable;             // 0x50
    PDRIVER_DISPATCH *PowerDispatchTable;           // 0x54
    PVOID WmiDispatchTable;                         // 0x58
    ULONG PciIdeNumber;                             // 0x5C
    ULONG DeviceId;                                 // 0x60
    PVOID PdoExtension_Chan1;                       // 0x64
    PVOID PdoExtension_Chan2;                       // 0x68
    ULONG Counter;                                  // 0x6C
    ULONG NumberOfChildrenPowerUp;                  // 0x70
    BOOLEAN NativeMode[2];                          // 0x74
    BOOLEAN Foo1[2];                                // 0x76
    BOOLEAN Foo2[2];                                // 0x78
    BOOLEAN Foo3[2];                                // 0x7A
    ULONG DeviceResourcesSize[2];                   // 0x7C
    PVOID DeviceResources[2];                       // 0x84
    ULONG HardwareResourcesSize;                    // 0x8C
    PVOID HardwareResources;                        // 0x90
    ULONG TranslatedBusMasterBaseAddressIsIo;       // 0x94
    PVOID TranslatedBusMasterBaseAddress;           // 0x98
    UCHAR Unknown8[44];                             // 0x9C
    ULONG AlignmentRequirement;                     // 0xC8
    ULONG Unknown9;                                 // 0xCC
    ULONG Unknown10;                                // 0xD0
    PPCI_IDE_EXTENSIONS IdeExtension;               // 0xD4
    PCONTROLLER_OBJECT ControllerObject;            // 0xD8
    KSPIN_LOCK SpinLock;                            // 0xDC
    ULONG DeviceControlFlags;                       // 0xE0
    BUS_INTERFACE_STANDARD BusInterface;            // 0xE4
    UCHAR Unknown11[40];                            // 0x104
    PVOID PowerContext1[3];                         // 0x114
    PVOID PowerContext2[3];                         // 0x120
    ULONG PowerContextLock[2];                      // 0x12C
    PKINTERRUPT pKInterrupt1[2];                    // 0x134
    PCI_FDO_INTERRUPT_CONTEXT InterruptContext[2];  // 0x13C
    UCHAR Unknown12[124];                           // 0x14C
    PKINTERRUPT pKInterrupt2[2];                    // 0x1C8
    UCHAR Unknown13[32];                            // 0x1D0
    BOOLEAN ControllerIsrInstalled;                 // 0x1F0
    BOOLEAN NativeInterruptEnabled;                 // 0x1F1
    USHORT Unknown14;                               // 0x1F2
    PCI_NATIVE_IDE_INTERFACE NativeIdeInterface;    // 0x1F4
} FDO_EXTENSION, *PFDO_EXTENSION;

//
// Globals
//
extern PDRIVER_DISPATCH FdoPnpDispatchTable[25];
extern PDRIVER_DISPATCH PdoPnpDispatchTable[25];
extern PDRIVER_DISPATCH FdoPowerDispatchTable[4];
extern PDRIVER_DISPATCH PdoPowerDispatchTable[4];
extern PVOID FdoWmiDispatchTable[12];
extern PVOID PdoWmiDispatchTable[12];

//
// PciIdeX Functions
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

VOID
PciIdeUnload(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeInternalDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DispatchWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PassDownToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NoSupportIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
StatusSuccessAndPassDownToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeXAlwaysStatusSuccessIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeXQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeXGetDeviceParameter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWSTR KeyName,
    OUT PVOID KeyValue
);

VOID
PciIdeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
);

//
// Controller FDO Functions
//
NTSTATUS
ControllerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

VOID 
ControllerOpMode(
    IN PFDO_EXTENSION FdoExtension
);

NTSTATUS
PciIdeGetBusStandardInterface(
    IN PFDO_EXTENSION FdoExtension
);

NTSTATUS
PciIdeBusData(
    IN PFDO_EXTENSION FdoExtension,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    IN BOOLEAN GetData
);

NTSTATUS
PciIdeGetNativeModeInterface(
    IN PFDO_EXTENSION FdoExtension
);

NTSTATUS
PciIdeGetNativeModeInterface(
    IN PFDO_EXTENSION FdoExtension
);

NTSTATUS
ControllerStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerQueryPnPDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerUsageNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ControllerSurpriseRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
EnablePCIBusMastering(
    IN PFDO_EXTENSION FdoExtension
);

NTSTATUS
ControllerStartDeviceCompletionRoutine(
    IN PDEVICE_OBJECT Device,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
AnalyzeResourceList(
    IN PFDO_EXTENSION FdoExtension,
    IN PCM_RESOURCE_LIST AllocatedResources
);

//
// Channel PDO Functions
//
NTSTATUS
ChannelStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryStopRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryCapabitilies(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryPnPDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelUsageNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeChannelQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Power Functions
//
NTSTATUS
DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeSetFdoPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeSetPdoPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PciIdeIssueSetPowerState(
    IN PFDO_EXTENSION FdoExtension,
    IN POWER_STATE_TYPE PowerType,
    IN SYSTEM_POWER_STATE PowerState,
    IN ULONG InitializeEvent
);

//
// Utility Functions
//
VOID
IdeCreateIdeDirectory(
    VOID
);

