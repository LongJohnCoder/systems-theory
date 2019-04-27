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

    Matthieu Suiche - 20-Nov-2006 -

--*/
#define INITGUID
#include "stdio.h"
#include "ntifs.h"
#include "wmilib.h"
#include "wdmguid.h"
#include "crashdmp.h"

//
// Debugging Macros
//
#define PROLOG()                                        \
{                                                       \
    DbgPrint("ATAPI: Entering %s...\n", __FUNCTION__);  \
    DbgBreakPoint();                                    \
}

#define EPILOG()                                        \
{                                                       \
    DbgPrint("ATAPI: Entering %s...\n", __FUNCTION__);  \
    DbgBreakPoint();                                    \
}

//
// Others macros
//
#define IDE_PORT_CHANNEL_MASK(a) (UCHAR)((((a & 1) | 0xFA) >> 4))

//
// Constants
//
#define IDE_TAG   'IdeP'

//
// IDE command definitions
//
#define IDE_COMMAND_NULL            0x00
#define IDE_COMMAND_ATAPI_PACKET    0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY  0xA1
#define IDE_COMMAND_UNKNOW1         0xB0
#define IDE_COMMAND_IDENTIFY        0xEC

//
// IDE status definitions
//
#define IDE_STATUS_ERROR            0x01
#define IDE_STATUS_INDEX            0x02
#define IDE_STATUS_DRQ              0x08
#define IDE_STATUS_IDLE             0x50
#define IDE_STATUS_BUSY             0x80

//
// IDE drive control definitions
//
#define IDE_DC_DISABLE_INTERRUPTS   0x02
#define IDE_DC_RESET_CONTROLLER     0x04
#define IDE_DC_REENABLE_CONTROLLER  0x00

DEFINE_GUID(WmiScsiAddressGuid,
            0x53F5630FL,
            0xB6BF,
            0x11D0,
            0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B);

//
// Structures
//
typedef struct _ATAPI_CRASH_DUMP_DATA
{
    PVOID PortConfiguration;
    ULONG u04;
    ULONG u08;
    ULONG u0C;
    PVOID StallRoutine;
} ATAPI_CRASH_DUMP_DATA, *PATAPI_CRASH_DUMP_DATA;

typedef struct _IDE_DRIVER_EXTENSION
{
    UNICODE_STRING RegistryPath;
} IDE_DRIVER_EXTENSION, *PIDE_DRIVER_EXTENSION;

typedef struct _IDE_FDO_LIST
{
    ULONG NumberOfEntry;
    LIST_ENTRY ListEntry;
    KSPIN_LOCK SpinLock;
} IDE_FDO_LIST, *PIDE_FDO_LIST;

typedef struct _DETECTION_LIST
{
    ULONG Port1;     // +0x00
    ULONG Port2;     // +0x04
    ULONG IRQ;       // +0x08
} DETECTION_LIST, *PDETECTION_LIST;

typedef struct _IO_ADDRESS
{
    union {
        PUCHAR u00;     // +0x00
        struct {
            UCHAR c00;        // +0x00
            UCHAR c01;        // +0x01
            UCHAR c02;        // +0x02
            UCHAR IdeStatus;  // +0x03
        };
    };
    PUSHORT Port04;     // +0x04
    PUCHAR Error;       // +0x08
    PUCHAR SectorCount; // +0x0C
    PUCHAR LBALow;      // +0x10
    PUCHAR LBAMid;      // +0x14
    PUCHAR LBAHigh;     // +0x18
    PUCHAR Command;     // +0x1C
    PUCHAR Status;      // +0x20
} IO_ADDRESS, *PIO_ADDRESS;

typedef struct _IO_ADDRESS_2
{
    UCHAR u00;          // +0x00
    UCHAR Error;        // +0x01
    UCHAR SectorCount;  // +0x02
    UCHAR LBALow;       // +0x03
    UCHAR LBAMid;       // +0x04
    UCHAR LBAHigh;      // +0x05
    UCHAR Command;      // +0x06
    UCHAR Status;       // +0x07
    UCHAR u08;          // +0x08
} IO_ADDRESS_2, *PIO_ADDRESS_2;

typedef struct _IO_ADDRESS_3
{
    union {
        PULONG u00;     // +0x00
        struct {
            UCHAR c00;        // +0x00
            UCHAR c01;        // +0x01
            UCHAR c02;        // +0x02
            UCHAR IdeStatus;  // +0x03
        };
    };
    PUCHAR Control;     // +0x04
    PULONG u08;         // +0x08
} IO_ADDRESS_3, *PIO_ADDRESS_3;

typedef struct _IOADDRESS
{
    PIO_ADDRESS_2 IoAddress2;       // +0x00
    PULONG u04;                     // +0x04
    PVOID Address;                  // +0x08
} IOADDRESS, *PIOADDRESS;

typedef struct _CHANNEL_EXTENSION
{
    PDEVICE_OBJECT DeviceObject1;                   // +0x000
    PDEVICE_OBJECT DeviceObject2;                   // +0x004
    PDRIVER_OBJECT DriverObject;                    // +0x008
    PDEVICE_OBJECT DeviceObject3;                   // +0x00C
    UCHAR u010[0x03C];                              // +0x010
    PDRIVER_DISPATCH IdePortPassDownToNextDriver;   // +0x04C
    PDRIVER_DISPATCH *FdoPnpDispatchTable;          // +0x050
    PDRIVER_DISPATCH *FdoPowerDispatchTable;        // +0x054
    PVOID *FdoWmiDispatchTable;                     // +0x058
    UCHAR u05C[0x050];                              // +0x05C
    PULONG Reserved1;                               // +0x0AC
    UCHAR u0B0[0x004];                              // +0x0B0
    ULONG TotalFdoEntries;                          // +0x0B4
    LIST_ENTRY ListEntry;                           // +0x0B8
    UCHAR u0C0[0x700];                              // +0x0C0
    ULONG Reserved2;                                // +0x7C0
    UCHAR u7C4[0xAA0];                              // +0x7C4
} CHANNEL_EXTENSION, *PCHANNEL_EXTENSION;



//
// Globals
//
extern PDRIVER_DISPATCH FdoPnpDispatchTable[25];
extern PDRIVER_DISPATCH PdoPnpDispatchTable[25];
extern PDRIVER_DISPATCH FdoPowerDispatchTable[4];
extern PDRIVER_DISPATCH PdoPowerDispatchTable[4];
extern PVOID FdoWmiDispatchTable[12];
extern PVOID PdoWmiDispatchTable[12];
extern WMIGUIDREGINFO IdePortWmiGuidList;
extern IDE_FDO_LIST IdeGlobalFdoList;
extern ULONG TotalFdoEntries;

//
// Prototypes for init.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

//
// Prototypes for crashdmp.c
//
NTSTATUS
AtapiCrashDumpDriverEntry(
    PDUMP_INITIALIZATION_CONTEXT DriverObject
);

BOOLEAN
AtapiCrashDumpOpen(
    IN LARGE_INTEGER Offset
);

NTSTATUS
AtapiCrashDumpIdeWrite(
    IN PLARGE_INTEGER Offset,
    IN PMDL Mdl
);

NTSTATUS
AtapiCrashDumpFinish(
    VOID
);

NTSTATUS
AtapiCrashDumpIdeWriteDMA(
    IN ULONG Count,
    IN PLARGE_INTEGER Offset,
    IN PMDL Mdl,
    IN PVOID Data
);

//
// Prototypes for ide.c
//
VOID
IdePortStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
IdePortUnload(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
IdePortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortAlwaysStatusSuccessIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortPassDownToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortStatusSuccessAndPassDownToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortNoSupportIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdePortWmiSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
IdeCreateIdeDirectory(
    VOID
);

VOID
IdeInitializeFdoList(
    OUT PIDE_FDO_LIST FdoList
);

NTSTATUS
IdePortDetectLegacyController(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
IdePortOkToDetectLegacy(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
IdePortGetParameterFromServiceSubKey(
    IN PDRIVER_OBJECT DriverObject,
    IN LPWSTR Name,
    IN ULONG ContextOrType,
    IN BOOLEAN Flags,
    IN PULONG EntryContext,
    IN ULONG DataSize
);

HANDLE
IdePortOpenServiceSubKey(
    IN HANDLE Handle,
    IN PUNICODE_STRING ObjectName
);

NTSTATUS
IdePortRegQueryRoutine(
    IN PCWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN SIZE_T ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

NTSTATUS
IdePortCloseServiceSubKey(
    IN HANDLE Handle
);

NTSTATUS
IdePortCreateDetectionList(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID *Buffer,
    OUT PULONG Counter
);

NTSTATUS
IdePortTranslateAddress(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    OUT PULONG AddressSpace,
    OUT PNTSTATUS Status,
    OUT PPHYSICAL_ADDRESS PhysicalAddress2
);

BOOLEAN
IdePortChannelEmpty(
    OUT PIO_ADDRESS IoAddress,
    OUT PIO_ADDRESS_3 IoAddress3,
    IN ULONG maxIdeDevice
);

NTSTATUS
IdePortpWaitOnBusyEx(
    OUT PIO_ADDRESS IoAddress,
    OUT PUCHAR StatusPort,
    IN UCHAR StatusValue
);

VOID
IdePortFreeTranslatedAddress(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG AddressSpace
);

BOOLEAN
IdePortDetectAlias(
    IN PIO_ADDRESS IoAddress
);

BOOLEAN
IdePortIdentifyDevice(
    OUT PIO_ADDRESS IoAddress,
    OUT PIO_ADDRESS_3 IoAddress3,
    IN ULONG maxIdeDevice
);

NTSTATUS
IdeHardReset(
    OUT PIO_ADDRESS IoAddress,
    OUT PIO_ADDRESS_3 IoAddress3,
    IN ULONG PortValue,
    IN ULONG Counter
);

VOID
IdeAddToFdoList(
    PIDE_FDO_LIST GlobalFdoList,
    PCHANNEL_EXTENSION ChannelExtension
);

BOOLEAN
IssueIdentify(
    OUT PIO_ADDRESS IoAddress,
    OUT PIO_ADDRESS_3 IoAddress3,
    IN UCHAR PortValue1,
    IN UCHAR PortValue2,
    IN UCHAR PortValue3,
    OUT PUSHORT Buffer
);

//
// Prototypes for chanfdo.c
//
NTSTATUS
ChannelAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

NTSTATUS
ChannelStartDevice(
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
ChannelQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelUsageNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryPnPDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelSurpriseRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelAddChannel(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PCHANNEL_EXTENSION *DeviceExtension
);
//
// Prototypes for devpdo.c
//
NTSTATUS
DeviceRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryStopRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceUsageNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryPnPDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for fdopower.c
//
NTSTATUS
IdePortSetFdoPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ChannelQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for atapi.c
//
VOID
AtapiBuildIoAddress(
    IN PIO_ADDRESS_2 IoAddress2,
    OUT PULONG arg2,
    OUT PIO_ADDRESS IoAddress,
    OUT PIO_ADDRESS_3 IoAddress3,
    OUT PULONG baseIoAddress1Length,
    OUT PULONG baseIoAddress2Length,
    OUT PULONG maxIdeDevice,
    OUT PULONG maxIdeChannel
);

//
// Prototypes for pdopower.c
//
NTSTATUS
IdePortSetPdoPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DeviceQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for wmi.c
//
VOID
IdePortWmiInit(
    VOID
);
