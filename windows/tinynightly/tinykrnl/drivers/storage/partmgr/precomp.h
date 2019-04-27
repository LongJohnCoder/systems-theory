#define INITGUID
#include "stdio.h"
#include "ntifs.h"
#include "ntdddisk.h"
#include "iotypes.h"
#include "volmgr.h"
#include "partmgrp.h"
#include "hal.h"
#include "wmilib.h"
#include "wmiguid.h"
#include "wmistr.h"
#include "wdmguid.h"
#include "ioevent.h"

extern ULONG DiskperfGuidCount;
extern WMIGUIDREGINFO DiskperfGuidList[];

#define IRP_MN_SET_TRACE_NOTIFY 0xA

#define UNEXPECTED_CALL                                 \
{                                                       \
    DbgPrint("Unexpected call: %s!\n", __FUNCTION__);   \
    DbgBreakPoint();                                    \
}

typedef
VOID
(*PM_WMI_TRACE_NOTIFY)(
    IN ULONG DeviceNumber,
    IN PIRP Irp,
    IN BOOLEAN Unknown
);

typedef struct _PM_SIGNATURE_TABLE_BLOCK
{
    LIST_ENTRY SignatureListEntry;              // 0x00
    struct _DEVICE_EXTENSION *DeviceExtension;  // 0x08
    union
    {
        ULONG Signature;                        // 0x0C
        GUID Guid;                              // 0x0C
    };
} PM_SIGNATURE_TABLE_BLOCK, *PPM_SIGNATURE_TABLE_BLOCK;

typedef struct _PM_NOTIFICATION_BLOCK
{
    ULONG EpochUpdates;                     // 0x00
    ULONG NumberOfDisks;                    // 0x04
    ULONG UpdateCount;                      // 0x08
    ULONG DiskArray[ANYSIZE_ARRAY];         // 0x0C
} PM_NOTIFICATION_BLOCK, *PPM_NOTIFICATION_BLOCK;

typedef struct _PM_VOLUME_ENTRY
{
    LIST_ENTRY VolumeListEntry;             // 0x00
    UNICODE_STRING DeviceName;              // 0x08
    ULONG PartitionCount;                   // 0x10
    PDEVICE_OBJECT DeviceObject;            // 0x14
    PFILE_OBJECT FileObject;                // 0x18
} PM_VOLUME_ENTRY, *PPM_VOLUME_ENTRY;

typedef struct _PM_PARTITION_ENTRY
{
    LIST_ENTRY PartitionListEntry;          // 0x00
    PDEVICE_OBJECT DeviceObject;            // 0x08
    PDEVICE_OBJECT WholeDiskPdo;            // 0x0C
    PPM_VOLUME_ENTRY VolumeEntry;           // 0x10
} PM_PARTITION_ENTRY, *PPM_PARTITION_ENTRY;

typedef struct _PM_WMI_COUNTER_DATA
{
    LARGE_INTEGER TimeStamp;
} PM_WMI_COUNTER_DATA, *PPM_WMI_COUNTER_DATA;

typedef struct _PM_POWER_WORK_ITEM_DATA
{
    LIST_ENTRY PowerListEntry;
    SYSTEM_POWER_STATE SystemPowerState;
} PM_POWER_WORK_ITEM_DATA, *PPM_POWER_WORK_ITEM_DATA;

typedef struct _PM_DRIVER_OBJECT_EXTENSION
{
    PDRIVER_OBJECT DriverObject;    // 0x00
    LIST_ENTRY VolumeListHead;      // 0x04
    LIST_ENTRY DeviceListHead;      // 0x0C
    PVOID NotificationEntry;        // 0x14
    KMUTEX Mutex;                   // 0x18
    ULONG ReinitFlag;               // 0x38
    RTL_AVL_TABLE SignatureTable;   // 0x3C
    RTL_AVL_TABLE GuidTable;        // 0x74
    UNICODE_STRING RegistryPath;    // 0xAC
    ULONG Signature;                // 0xB4
    ULONG GuidFlag;                 // 0xB8
    GUID Guid;                      // 0xBC
    volatile LONG EpochUpdates;     // 0xCC
    LIST_ENTRY SigUpdateListHead;   // 0xD0
} PM_DRIVER_OBJECT_EXTENSION, *PPM_DRIVER_OBJECT_EXTENSION;

typedef struct _DEVICE_EXTENSION
{
    UCHAR DeviceCleaned;
    UCHAR WmiCounterEnabled;
    UCHAR NewSignature;
    UCHAR u4;
    ULONG UpdateFailed;                             // 0x04
    PDEVICE_OBJECT DeviceObject;                    // 0x08
    PPM_DRIVER_OBJECT_EXTENSION DriverExtension;    // 0x0C
    PDEVICE_OBJECT NextLowerDriver;                 // 0x10
    PDEVICE_OBJECT Pdo;                             // 0x14
    LIST_ENTRY PartitionListHead;                   // 0x18
    LIST_ENTRY DeviceListEntry;                     // 0x20
    ULONG u28;                                      // 0x28
    KEVENT Event;                                   // 0x2C
    ULONG Signature;                                // 0x3C
    LIST_ENTRY SignatureList;                       // 0x40
    LIST_ENTRY GuidList;                            // 0x48
    ULONG IoCtlCounterEnabled;                      // 0x50
    ULONG DeviceNumber;                             // 0x54
    ULONG SigUpdateEpochCount;                      // 0x58
    PVOID WmiCounterContext;                        // 0x5C
    UNICODE_STRING DeviceName;                      // 0x60
    WCHAR DeviceString[64];                         // 0x68
    PM_WMI_TRACE_NOTIFY WmiTraceNotify;             // 0xE8
    PWMILIB_CONTEXT WmiLibInfo;                     // 0xEC
    LIST_ENTRY PowerListHead;                       // 0xF0
    KSPIN_LOCK PowerLock;                           // 0xF8
    IO_REMOVE_LOCK RemoveLock;                      // 0x100
    BOOLEAN u158;                                   // 0x158
    PPARTITION_INFORMATION_GPT GptAttributes;       // 0x15C
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
PmSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context OPTIONAL
);

NTSTATUS
PmAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

NTSTATUS
PmPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmQueryDeviceRelations(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
);

NTSTATUS
PmQueryRemovalRelations(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
);

ULONG
PmQueryRegistrySignature(
    VOID
);

VOID
PmQueryRegistryGuid(
    IN PPM_DRIVER_OBJECT_EXTENSION PrivateExtension
);

VOID
PmTableFreeRoutine(
    PRTL_AVL_TABLE Table,
    PVOID Buffer
);

PVOID
PmTableAllocateRoutine(
    PRTL_AVL_TABLE Table,
    CLONG ByteSize
);

RTL_GENERIC_COMPARE_RESULTS
PmTableGuidCompareRoutine(
    PRTL_AVL_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);

RTL_GENERIC_COMPARE_RESULTS
PmTableSignatureCompareRoutine(
    PRTL_AVL_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);

NTSTATUS
PmVolumeManagerNotification(
    IN PVOID NotificationStructure,
    IN OUT PVOID Context
);

NTSTATUS
PmQueryRegistryGuidQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

VOID
PmDriverReinit(
    PDRIVER_OBJECT DriverObject,
    PVOID Context,
    ULONG Count
);

NTSTATUS
PmQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
);

NTSTATUS
PmQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
);

NTSTATUS
PmWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
);

BOOLEAN
PmWmiCounterDisable(
    IN PVOID *CounterContext,
    IN BOOLEAN ForceDisable,
    IN BOOLEAN DeallocateOnZero
);

VOID
PmWmiCounterIoStart(
    IN PVOID CounterContext,
    OUT PLARGE_INTEGER TimeStamp
);

VOID
PmWmiCounterIoComplete(
    IN PVOID CounterContext,
    IN PIRP Irp,
    IN PLARGE_INTEGER TimeStamp
);

NTSTATUS
PmWmiCounterEnable(
    IN PVOID *CounterContext
);

VOID
PmWmiCounterQuery(
    IN PVOID CounterContext,
    IN OUT PDISK_PERFORMANCE CounterBuffer,
    IN PWCHAR StorageManagerName,
    IN ULONG StorageDeviceNumber
);

BOOLEAN
LockDriverWithTimeout(
    IN PPM_DRIVER_OBJECT_EXTENSION DriverExtension
);

VOID
PmTakePartition(
    IN PPM_VOLUME_ENTRY Volume,
    IN PDEVICE_OBJECT PartitionDeviceObject,
    IN PDEVICE_OBJECT WholeDiskPdo
);

NTSTATUS
PmGivePartition(
    IN PPM_VOLUME_ENTRY Volume,
    IN PDEVICE_OBJECT PartitionDeviceObject,
    IN PDEVICE_OBJECT WholeDiskPdo
);

NTSTATUS
PmChangePartitionIoctl(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PPM_PARTITION_ENTRY Partition,
    IN ULONG IoControlCode
);

NTSTATUS
PmStartPartition(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
PmNotifyPartitions(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
);

BOOLEAN
PmIsRedundantPath(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_EXTENSION FoundDeviceExtension,
    IN ULONG Signature,
    IN PULONG Unknown
);

NTSTATUS
PmReadGptAttributesOnMbr(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PPARTITION_INFORMATION_GPT *Attributes
);

NTSTATUS
PmWriteGptAttributesOnMbr(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PPARTITION_INFORMATION_GPT Attributes
);

NTSTATUS
PmQueryDeviceId(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PSTORAGE_DEVICE_ID_DESCRIPTOR *DeviceId
);

BOOLEAN
PmLookupId(
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceId,
    IN PVOID CheckIdentifier,
    IN USHORT IdentifierSize
);

NTSTATUS
PmCheckForUnclaimedPartitions(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
PmDiskGrowPartition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmEjectVolumeManagers(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
PmQueryDependantVolumeList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT PartitionDeviceObject,
    IN PDEVICE_OBJECT WholeDiskPdo,
    IN PVOLMGR_DEPENDANT_VOLUMES_INFORMATION *DependantVolumes
);

NTSTATUS
PmReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDRIVE_LAYOUT_INFORMATION_EX *PartitionTable
);

NTSTATUS
PmWritePartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION_EX PartitionTable
);

VOID
PmSigCheckCompleteNotificationIrps(
    IN PLIST_ENTRY ListEntry
);

NTSTATUS
PmQueryDiskSignature(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
PmCheckAndUpdateSignature(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN CheckUpdateEpoch,
    IN BOOLEAN LastValue
);

NTSTATUS
PmRemoveDevice(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
);

NTSTATUS
PmDetermineDeviceNameAndNumber(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG WmiRegFlags
);

NTSTATUS
PmRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG WmiRegFlags
);

NTSTATUS
PmSigCheckNotificationInsert(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
);
