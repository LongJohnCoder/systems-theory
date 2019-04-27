/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    precomp.h

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
#include "ntddk.h"
#include "classpnp.h"
#include "initguid.h"
#include "ntddstor.h"
#include "ntddvol.h"
#include "ioevent.h"
#include "scsi.h"
#include <wmidata.h>
#include <wmistr.h>
#include "ntstrsafe.h"
#include "ntintsafe.h"
#include <storswtr.h>
#include "hal.h"

#define NtUnhandled()                                   \
{                                                       \
    DbgPrint("Unexpected call: %s!\n", __FUNCTION__);   \
    DbgBreakPoint();                                    \
}

//
// Callback for Partition Updates (changes on Removable vs Fixed)
//
typedef
VOID
(*PDISK_UPDATE_PARTITIONS)(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PDRIVE_LAYOUT_INFORMATION_EX PartitionList
);

//
// Structures/Typedefs
//
typedef struct _DISK_MEDIA_TYPES_LIST
{
    PCHAR VendorId;
    PCHAR ProductId;
    PCHAR Revision;
    const ULONG NumberOfTypes;
    const ULONG NumberOfSides;
    const STORAGE_MEDIA_TYPE MediaTypes[4];
} DISK_MEDIA_TYPES_LIST, *PDISK_MEDIA_TYPES_LIST;

//
// FIXME: Not needed on IA64.
//
typedef enum _DISK_GEOMETRY_SOURCE
{
    DiskGeometryUnknown,
    DiskGeometryFromBios,
    DiskGeometryFromPort,
    DiskGeometryFromNec98,
    DiskGeometryGuessedFromBios,
    DiskGeometryFromDefault,
    DiskGeometryFromNT4
} DISK_GEOMETRY_SOURCE, *PDISK_GEOMETRY_SOURCE;

typedef struct _DISK_GROUP_CONTEXT
{
    LIST_ENTRY CurrList;
    PIRP CurrIrp;
    LIST_ENTRY NextList;
    PIRP NextIrp;
    SCSI_REQUEST_BLOCK Srb;
    KMUTEX Mutex;
    KEVENT Event;
#if DBG
    ULONG DbgTagCount;
    ULONG DbgSavCount;
    ULONG DbgRefCount[64];
#endif
} DISK_GROUP_CONTEXT, *PDISK_GROUP_CONTEXT;

typedef enum _DISK_USER_WRITE_CACHE_SETTING
{
    DiskWriteCacheDisable = 0,
    DiskWriteCacheEnable = 1,
    DiskWriteCacheDefault = -1
} DISK_USER_WRITE_CACHE_SETTING, *PDISK_USER_WRITE_CACHE_SETTING;

typedef struct _DISK_DATA
{
    ULONG PartitionOrdinal;
    PARTITION_STYLE PartitionStyle;
    union
    {
        struct
        {
            ULONG Signature;
            ULONG MbrCheckSum;
            ULONG HiddenSectors;
            UCHAR PartitionType;
            BOOLEAN BootIndicator;
        } Mbr;

        struct
        {
            GUID DiskId;
            GUID PartitionType;
            GUID PartitionId;
            ULONG64 Attributes;
            WCHAR PartitionName[36];
        } Efi;
    };

    struct
    {
        BOOLEAN WellKnownNameCreated : 1;
        BOOLEAN PhysicalDriveLinkCreated : 1;
    } LinkStatus;
    NTSTATUS ReadyStatus;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    PDISK_UPDATE_PARTITIONS UpdatePartitionRoutine;
#endif
    SCSI_ADDRESS ScsiAddress;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    KEVENT PartitioningEvent;
#endif
    UNICODE_STRING DiskInterfaceString;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    UNICODE_STRING PartitionInterfaceString;
#endif
    FAILURE_PREDICTION_METHOD FailurePredictionCapability;
    BOOLEAN AllowFPPerfHit;
    //
    // FIXME: Not needed on IA64.
    //
    DISK_GEOMETRY_SOURCE GeometrySource;
    DISK_GEOMETRY RealGeometry;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    ULONG CachedPartitionTableValid;
    PDRIVE_LAYOUT_INFORMATION_EX CachedPartitionTable;
#endif
    KMUTEX VerifyMutex;
    DISK_GROUP_CONTEXT FlushContext;
    DISK_USER_WRITE_CACHE_SETTING WriteCacheOverride;
} DISK_DATA, *PDISK_DATA;

typedef struct _DISK_DETECT_INFO
{
    BOOLEAN Initialized;
    ULONG Style;
    ULONG Signature;
    ULONG MbrCheckSum;
    PDEVICE_OBJECT Device;
    CM_INT13_DRIVE_PARAMETER DriveParameters;
} DISK_DETECT_INFO, *PDISK_DETECT_INFO;

typedef struct _DISK_GEOMETRY_EX_INTERNAL
{
    DISK_GEOMETRY Geometry;
    LARGE_INTEGER DiskSize;
    DISK_PARTITION_INFO Partition;
    DISK_DETECTION_INFO Detection;
} DISK_GEOMETRY_EX_INTERNAL, *PDISK_GEOMETRY_EX_INTERNAL;

typedef struct _DISKREREGREQUEST
{
    SINGLE_LIST_ENTRY Next;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
} DISKREREGREQUEST, *PDISKREREGREQUEST;

//
// Externals
//
extern CLASSPNP_SCAN_FOR_SPECIAL_INFO DiskBadControllers[];
extern const DISK_MEDIA_TYPES_LIST DiskMediaTypes[];
extern GUIDREGINFO DiskWmiFdoGuidList[];

//
// Defines
//
#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define CdbForceUnitAccess ReservedByte
#endif

#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN  \
    CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_READ_ACCESS)

#define DiskGeometryGuid                            0
#define SmartStatusGuid                             1
#define SmartDataGuid                               2
#define SmartPerformFunction                        3

#define AllowDisallowPerformanceHit                 1
#define EnableDisableHardwareFailurePrediction      2
#define EnableDisableFailurePredictionPolling       3
#define GetFailurePredictionCapability              4
#define EnableOfflineDiags                          5

#define SmartEventGuid                              4
#define SmartThresholdsGuid                         5
#define ScsiInfoExceptionsGuid                      6

#define VALUE_BUFFER_SIZE                           2048

#define DISK_TAG_DISABLE_CACHE                      'CDcS'
#define DISK_TAG_INFO_EXCEPTION                     'ADcS'
#define DISK_TAG_MODE_DATA                          'MDcS'
#define DISK_TAG_NAME                               'NDcS'
#define DISK_TAG_PART_LIST                          'pDcS'
#define DISK_TAG_SMART                              'aDcS'
#define DISK_TAG_SRB                                'SDcS'
#define DISK_TAG_START                              'sDcS'
#define DISK_TAG_UPDATE_GEOM                        'gDcS'

#define FDO_NAME_FORMAT                             \
    "\\Device\\Harddisk%d\\DR%d"
#define PDO_NAME_FORMAT                             \
    "\\Device\\Harddisk%d\\DP(%d)%#I64x-%#I64x+%lx"

#define SCSI_DISK_TIMEOUT                           10
#define PARTITION0_LIST_SIZE                        4
#define MODE_DATA_SIZE                              192

#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define MAX_SECTORS_PER_VERIFY                      0x200
#else
#define MAX_SECTORS_PER_VERIFY                      0x100
#endif

#define DISK_DEFAULT_FAILURE_POLLING_PERIOD         1 * 60 * 60

#define DiskDeviceParameterSubkey                   L"Disk"
#define DiskDeviceUserWriteCacheSetting             L"UserWriteCacheSetting"
#define DiskDeviceCacheIsPowerProtected             L"CacheIsPowerProtected"

//
// Prototypes for disk.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
);

VOID
DiskUnload(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
DiskCreateFdo(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PULONG DeviceCount,
    IN BOOLEAN DisallowMount
);

NTSTATUS
DiskReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DiskDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DiskShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
DisableWriteCache(
    IN PDEVICE_OBJECT Fdo,
    IN PIO_WORKITEM WorkItem
);

VOID
DiskFdoProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK ScsiRequestBlock,
    NTSTATUS *Status,
    BOOLEAN *Retry
);

VOID
DiskSetSpecialHacks(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN ULONG_PTR Data
);

VOID
ResetBus(
    IN PDEVICE_OBJECT Fdo
);

NTSTATUS
DiskGetCacheInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN PDISK_CACHE_INFORMATION CacheInfo
);

NTSTATUS
DiskSetCacheInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PDISK_CACHE_INFORMATION CacheInfo
);

NTSTATUS
DiskIoctlGetCacheSetting(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DiskIoctlSetCacheSetting(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DiskIoctlGetLengthInfo(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetDriveGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetDriveGeometryEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetCacheInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSetCacheInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetMediaTypesEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlPredictFailure(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlVerify(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlReassignBlocks(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlReassignBlocksEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlIsWritable(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSetVerify(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlClearVerify(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlUpdateDriveSize(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetVolumeDiskExtents(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSmartGetVersion(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSmartReceiveDriveData(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSmartSendDriveCommand(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlCreateDisk(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetDriveLayout(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetDriveLayoutEx(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSetDriveLayout(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSetDriveLayoutEx(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlDeleteDriveLayout(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetPartitionInfo(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGetPartitionInfoEx(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSetPartitionInfo(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlSetPartitionInfoEx(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlGrowPartition(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskIoctlUpdateProperties(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
);

NTSTATUS
DiskQueryPnpCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_CAPABILITIES Capabilities
);

VOID
DiskFlushDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PDISK_GROUP_CONTEXT FlushContext
);

NTSTATUS
DiskFlushComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
);

//
// Prototypes for diskwmi.c
//
NTSTATUS
DiskEnableSmart(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DiskDisableSmart(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DiskPerformSmartCommand(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ScsiControlCode,
    IN UCHAR Command,
    IN UCHAR Feature,
    IN UCHAR SectorCount,
    IN UCHAR SectorNumber,
    IN OUT PSRB_IO_CONTROL ScsiIoControl,
    OUT PULONG BufferSize
);

NTSTATUS
DiskGetIdentifyInfo(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    PBOOLEAN SupportSmart
);

NTSTATUS
DiskSendFailurePredictIoctl(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    PSTORAGE_PREDICT_FAILURE IoctlPredictFailure
);

NTSTATUS
DiskEnableDisableFailurePrediction(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    BOOLEAN Enable
);

NTSTATUS
DiskEnableDisableFailurePredictPolling(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    BOOLEAN Enable,
    ULONG PollTimeInSeconds
);

NTSTATUS
DiskReadFailurePredictStatus(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    PSTORAGE_FAILURE_PREDICT_STATUS DiskSmartStatus
);

NTSTATUS
DiskReadFailurePredictData(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    PSTORAGE_FAILURE_PREDICT_DATA DiskSmartData
);

VOID
DiskReregWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
);

NTSTATUS
DiskInitializeReregistration(
    VOID
);

NTSTATUS
DiskPostReregisterRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
DiskInfoExceptionCheck(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DiskInfoExceptionComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
);

NTSTATUS
DiskDetectFailurePrediction(
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    PFAILURE_PREDICTION_METHOD PredictCapability
);

NTSTATUS
DiskWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN CLASSENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
);

NTSTATUS
DiskFdoQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegistrationFlags,
    OUT PUNICODE_STRING InstanceName
);

NTSTATUS
DiskFdoQueryWmiRegInfoEx(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegistrationFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING MofName
);

NTSTATUS
DiskFdoQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvailable,
    OUT PUCHAR Buffer
);

NTSTATUS
DiskFdoSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
);

NTSTATUS
DiskFdoSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
);

NTSTATUS
DiskFdoExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InputBufferSize,
    IN ULONG OutputBufferSize,
    IN PUCHAR Buffer
);

//
// Prototypes for enum.c
//
PDRIVE_LAYOUT_INFORMATION
DiskConvertExtendedToLayout(
    IN CONST PDRIVE_LAYOUT_INFORMATION_EX DriveLayoutEx
);

PDRIVE_LAYOUT_INFORMATION_EX
DiskConvertLayoutToExtended(
    IN CONST PDRIVE_LAYOUT_INFORMATION DriveLayout
);

NTSTATUS
DiskCreatePdo(
    IN PDEVICE_OBJECT Fdo,
    IN ULONG PartitionOrdinal,
    IN PPARTITION_INFORMATION_EX PartitionEntry,
    IN PARTITION_STYLE PartitionStyle,
    OUT PDEVICE_OBJECT *Pdo
);

NTSTATUS
DiskEnumerateDevice(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
DiskUpdatePartitions(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PDRIVE_LAYOUT_INFORMATION_EX PartitionList
);

VOID
DiskUpdateRemovablePartitions(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PDRIVE_LAYOUT_INFORMATION_EX PartitionList
);

VOID
DiskAcquirePartitioningLock(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

VOID
DiskReleasePartitioningLock(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

//
// Prototypes for geometry.c
// FIXME: Not needed on IA64. (All of geometry.c)
// (on IA64 we replace calls to DiskReadDriveCapacity(DeviceObject)
// with calls to ClassReadDriveCapacity(DeviceObject))
//
NTSTATUS
DiskSaveDetectInfo(
    PDRIVER_OBJECT DriverObject
);

VOID
DiskCleanupDetectInfo(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
DiskSaveGeometryDetectInfo(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE HardwareKey
);

VOID
DiskScanBusDetectInfo(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE BusKey
);

NTSTATUS
DiskSaveBusDetectInfo(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE TargetKey,
    IN ULONG DiskNumber
);

DISK_GEOMETRY_SOURCE
DiskUpdateGeometry(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DiskUpdateRemovableGeometry(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DiskGetPortGeometry(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    OUT PDISK_GEOMETRY DiskGeometry
);

BOOLEAN
DiskIsNT4Geometry(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DiskReadDriveCapacity(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
DiskDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Nothing,
    IN ULONG Count
);

NTSTATUS
DiskGetDetectInfo(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    OUT PDISK_DETECTION_INFO DetectInfo
);

NTSTATUS
DiskReadSignature(
    IN PDEVICE_OBJECT DeviceObject
);

//
// Prototypes for part.c
//
NTSTATUS
DiskReadPartitionTableEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN BOOLEAN BypassCache,
    OUT PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout
);

NTSTATUS
DiskWritePartitionTableEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
);

NTSTATUS
DiskSetPartitionInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
);

NTSTATUS
DiskSetPartitionInformationEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN ULONG PartitionNumber,
    IN struct _SET_PARTITION_INFORMATION_EX* PartitionInfo
);

NTSTATUS
DiskVerifyPartitionTable(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN FixErrors
);

BOOLEAN
DiskInvalidatePartitionTable(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN BOOLEAN PartitionLockHeld
);

//
// Prototypes for pnp.c
//
NTSTATUS
DiskAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
);

NTSTATUS
DiskInitFdo(
    IN PDEVICE_OBJECT Fdo
);

NTSTATUS
DiskStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
);

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
    OUT PUCHAR *RawName
);

VOID
DiskCreateSymbolicLinks(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
DiskRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
);

NTSTATUS
DiskStartFdo(
    IN PDEVICE_OBJECT Fdo
);

NTSTATUS
DiskInitPdo(
    IN PDEVICE_OBJECT Pdo
);

NTSTATUS
DiskStartPdo(
    IN PDEVICE_OBJECT Pdo
);

NTSTATUS
DiskQueryId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING UnicodeIdString
);
