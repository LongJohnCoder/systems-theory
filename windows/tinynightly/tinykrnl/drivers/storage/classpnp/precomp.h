/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    precomp.h

Abstract:

    Class Driver Precompiled Header

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - Implemented - 16-Feb-06

--*/
#include "ntddk.h"
#include "ntintsafe.h"
#include <classpnp.h>
#include <mountdev.h>

#define NtUnhandled()                                   \
{                                                       \
    DbgPrint("Unexpected call: %s!\n", __FUNCTION__);   \
    DbgBreakPoint();                                    \
}

//
// Externals
//
extern GUID ClassGuidQueryRegInfoEx;
extern CLASSPNP_SCAN_FOR_SPECIAL_INFO ClassBadItems[];
extern LIST_ENTRY AllFdosList;
extern ULONG ClassMaxInterleavePerCriticalIo;

//
// Defines
//
#define NUM_DRIVECAPACITY_RETRIES 1
#define NUM_ERROR_LOG_ENTRIES 16
#define NUM_IO_RETRIES 8
#define NUM_MODESENSE_RETRIES 1
#define MINIMUM_RETRY_UNITS ((LONGLONG)32)
#define CLASS_ERROR_LEVEL_1 (0x4)
#define CLASS_ERROR_LEVEL_2 (0x8)
#define DICTIONARY_SIGNATURE 'tciD'
#define CLASS_PERF_RESTORE_MINIMUM (0x10)
#define CLASS_MAX_INTERLEAVE_PER_CRITICAL_IO 0x4
#define CLASS_TAG_PRIVATE_DATA 'CPcS'
#define CLASSP_REG_SUBKEY_NAME (L"Classpnp")
#define CLASSP_REG_REMOVAL_POLICY_VALUE_NAME (L"UserRemovalPolicy")
#define CLASSP_REG_PERF_RESTORE_VALUE_NAME (L"RestorePerfAtCount")
#define CLASSP_REG_HACK_VALUE_NAME (L"HackMask")
#define CLASSP_REG_WRITE_CACHE_VALUE_NAME (L"WriteCacheEnableOverride")
#define CLASSP_VOLUME_VERIFY_CHECKED 0x34
#define FDO_HACK_VALID_FLAGS (0x0000000F)
#define FDO_HACK_INVALID_FLAGS (~FDO_HACK_VALID_FLAGS)
#define FDO_HACK_CANNOT_LOCK_MEDIA (0x00000001)
#define FDO_HACK_NO_RESERVE6 (0x00000008)
#define FDO_HACK_NO_SYNC_CACHE (0x00000004)
#define MIN_INITIAL_TRANSFER_PACKETS 1
#define MIN_WORKINGSET_TRANSFER_PACKETS_Consumer 4
#define MAX_WORKINGSET_TRANSFER_PACKETS_Consumer 64
#define MIN_WORKINGSET_TRANSFER_PACKETS_Server 64
#define MAX_WORKINGSET_TRANSFER_PACKETS_Server 1024
#define MIN_WORKINGSET_TRANSFER_PACKETS_Enterprise 256
#define MAX_WORKINGSET_TRANSFER_PACKETS_Enterprise 2048

#ifdef _WIN64
    #define PTRALIGN DECLSPEC_ALIGN(16)
#else
    #define PTRALIGN
#endif

//
// Macros for single-linked list queuing
//
__inline VOID
SimpleInitSlistHdr(SINGLE_LIST_ENTRY *SListHdr)
{
    SListHdr->Next = NULL;
}
__inline VOID
SimplePushSlist(SINGLE_LIST_ENTRY *SListHdr,
                SINGLE_LIST_ENTRY *SListEntry)
{
    SListEntry->Next = SListHdr->Next;
    SListHdr->Next = SListEntry;
}
__inline SINGLE_LIST_ENTRY
*SimplePopSlist(SINGLE_LIST_ENTRY *SListHdr)
{
    SINGLE_LIST_ENTRY *SListEntry = SListHdr->Next;
    if (SListEntry)
    {
        SListHdr->Next = SListEntry->Next;
        SListEntry->Next = NULL;
    }
    return SListEntry;
}
__inline BOOLEAN
SimpleIsSlistEmpty(SINGLE_LIST_ENTRY *SListHdr)
{
    return (SListHdr->Next == NULL);
}

//
// Structures
//
typedef struct _CLASS_RETRY_INFO
{
    struct _CLASS_RETRY_INFO *Next;
} CLASS_RETRY_INFO, *PCLASS_RETRY_INFO;

typedef struct _CLASS_ERROR_LOG_DATA
{
    LARGE_INTEGER TickCount;
    ULONG PortNumber;
    UCHAR ErrorPaging:1;
    UCHAR ErrorRetried:1;
    UCHAR ErrorUnhandled:1;
    UCHAR ErrorReserved:5;
    UCHAR Reserved[3];
    SCSI_REQUEST_BLOCK Srb;
    SENSE_DATA SenseData;
} CLASS_ERROR_LOG_DATA, *PCLASS_ERROR_LOG_DATA;

struct _CLASS_PRIVATE_FDO_DATA
{
    LIST_ENTRY AllFdosListEntry;
    struct
    {
        ULONG OriginalSrbFlags;
        ULONG SuccessfulIO;
        ULONG ReEnableThreshhold;
    } Perf;
    ULONG_PTR HackFlags;
    STORAGE_HOTPLUG_INFO HotplugInfo;
    struct
    {
        LARGE_INTEGER Delta;
        LARGE_INTEGER Tick;
        PCLASS_RETRY_INFO ListHead;
        ULONG Granularity;
        KSPIN_LOCK Lock;
        KDPC Dpc;
        KTIMER Timer;
    } Retry;
    BOOLEAN TimerStarted;
    BOOLEAN LoggedTURFailureSinceLastIO;
    BOOLEAN LoggedSYNCFailure;
    BOOLEAN ReleaseQueueIrpAllocated;
    PIRP ReleaseQueueIrp;
    LIST_ENTRY AllTransferPacketsList;
    SLIST_HEADER FreeTransferPacketsList;
    ULONG NumFreeTransferPackets;
    ULONG NumTotalTransferPackets;
    ULONG DbgPeakNumTransferPackets;
    LIST_ENTRY DeferredClientIrpList;
    ULONG HwMaxXferLen;
    SCSI_REQUEST_BLOCK SrbTemplate;
    KSPIN_LOCK SpinLock;
    READ_CAPACITY_DATA_EX LastKnownDriveCapacityData;
    BOOLEAN IsCachedDriveCapDataValid;
    ULONG ErrorLogNextIndex;
    CLASS_ERROR_LOG_DATA ErrorLogs[NUM_ERROR_LOG_ENTRIES];
    ULONG NumHighPriorityPagingIo;
    ULONG MaxInterleavedNormalIo;
    LARGE_INTEGER ThrottleStartTime;
    LARGE_INTEGER ThrottleStopTime;
    LARGE_INTEGER LongestThrottlePeriod;
#if DBG
    ULONG DbgMaxPktId;
    BOOLEAN DbgInitFlushLogging;
    ULONG DbgNumIORequests;
    ULONG DbgNumFUAs;
    ULONG DbgNumFlushes;
    ULONG DbgIOsSinceFUA;
    ULONG DbgIOsSinceFlush;
    ULONG DbgAveIOsToFUA;
    ULONG DbgAveIOsToFlush;
    ULONG DbgMaxIOsToFUA;
    ULONG DbgMaxIOsToFlush;
    ULONG DbgMinIOsToFUA;
    ULONG DbgMinIOsToFlush;
    ULONG DbgPacketLogNextIndex;
    TRANSFER_PACKET DbgPacketLogs[DBG_NUM_PACKET_LOG_ENTRIES];
#endif
    KSPIN_LOCK IdleListLock;
    LIST_ENTRY IdleIrpList;
    KTIMER IdleTimer;
    KDPC IdleDpc;
    USHORT IdleTimerInterval;
    USHORT StarvationCount;
    ULONG IdleTimerTicks;
    ULONG IdleTicks;
    ULONG IdleIoCount;
    LONG IdleTimerStarted;
    LARGE_INTEGER LastIoTime;
};

typedef struct _TRANSFER_PACKET
{
    LIST_ENTRY AllPktsListEntry;
    SLIST_ENTRY SlistEntry;
    PIRP Irp;
    PDEVICE_OBJECT Fdo;
    PIRP OriginalIrp;
    BOOLEAN CompleteOriginalIrpWhenLastPacketCompletes;
    ULONG NumRetries;
    KTIMER RetryTimer;
    KDPC RetryTimerDPC;
    ULONG RetryIntervalSec;
    PKEVENT SyncEventPtr;
    BOOLEAN InLowMemRetry;
    PUCHAR LowMemRetry_remainingBufPtr;
    ULONG LowMemRetry_remainingBufLen;
    LARGE_INTEGER LowMemRetry_nextChunkTargetLocation;
    PUCHAR BufPtrCopy;
    ULONG BufLenCopy;
    LARGE_INTEGER TargetLocationCopy;
    SENSE_DATA SrbErrorSenseData;
    SCSI_REQUEST_BLOCK Srb;
#if DBG
    LARGE_INTEGER DbgTimeSent;
    LARGE_INTEGER DbgTimeReturned;
    ULONG DbgPktId;
    IRP DbgOriginalIrpCopy;
    MDL DbgMdlCopy;
#endif
    BOOLEAN UsePartialMdl;
    PMDL PartialMdl;
} TRANSFER_PACKET, *PTRANSFER_PACKET;

typedef enum {
    SimpleMediaLock,
    SecureMediaLock,
    InternalMediaLock
} MEDIA_LOCK_TYPE, *PMEDIA_LOCK_TYPE;

typedef struct _FAILURE_PREDICTION_INFO {
    FAILURE_PREDICTION_METHOD Method;
    ULONG CountDown;
    ULONG Period;
    PIO_WORKITEM WorkQueueItem;
    KEVENT Event;
} FAILURE_PREDICTION_INFO, *PFAILURE_PREDICTION_INFO;


struct _DICTIONARY_HEADER
{
    PDICTIONARY_HEADER Next;
    ULONGLONG Key;
    UCHAR Data[0];
};

struct _DICTIONARY_HEADER;
typedef struct _DICTIONARY_HEADER DICTIONARY_HEADER, *PDICTIONARY_HEADER;

//
// Prototypes for autorun.c
//
NTSTATUS
ClasspMcnControl(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb
);

NTSTATUS
ClasspDisableTimer(
    PDEVICE_OBJECT DeviceObject
);

NTSTATUS
ClasspEnableTimer(
    PDEVICE_OBJECT DeviceObject
);

VOID
ClasspTimerTick(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
);

VOID
ClasspFailurePredict(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFAILURE_PREDICTION_INFO Info
);
//
// Prototypes for class.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

ULONG
ClassInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PCLASS_INIT_DATA InitializationData
);

ULONG
ClassInitializeEx(
    IN PDRIVER_OBJECT DriverObject,
    IN LPGUID Guid,
    IN PVOID Data
);

NTSTATUS
ClassAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT PhysicalDeviceObject
);

NTSTATUS
ClassRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR RemoveType
);

VOID
ClassUnload(
    IN PDRIVER_OBJECT DriverObject
);

VOID
ClasspStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClassIoComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
);

BOOLEAN
ClassInterpretSenseInfo(
    IN PDEVICE_OBJECT Fdo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR MajorFunctionCode,
    IN ULONG IoDeviceCode,
    IN ULONG RetryCount,
    OUT NTSTATUS *Status,
    OUT OPTIONAL ULONG *RetryInterval
);

VOID
ClassRetryRequest(
    IN PDEVICE_OBJECT SelfDeviceObject,
    IN PIRP Irp,
    IN LARGE_INTEGER  TimeDelta100ns
);

VOID
ClasspRetryDpcTimer(
    IN PCLASS_PRIVATE_FDO_DATA FdoData
);

VOID
ClassReleaseQueue(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
ClasspReleaseQueue(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP ReleaseQueueIrp OPTIONAL
);

NTSTATUS
ClassReleaseQueueCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
);

NTSTATUS
ClasspAllocateReleaseQueueIrp(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

NTSTATUS
ClassDispatchPnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
ClassReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClassInternalIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClassDeviceControlDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
ClassShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

PVPB
ClassGetVpb(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
ClassAsynchronousCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
ClassClaimDevice(
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN BOOLEAN Release
);

NTSTATUS
ClassCreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PCCHAR ObjectNameBuffer,
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN BOOLEAN IsFdo,
    IN OUT PDEVICE_OBJECT *DeviceObject
);

NTSTATUS
ClassDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

PVOID
ClassFindModePage(
    __in_bcount(Length) PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode,
    IN BOOLEAN Use6Byte
);

VOID
ClassInvalidateBusRelations(
    IN PDEVICE_OBJECT DeviceObject
);

ULONG
ClassModeSense(
    IN PDEVICE_OBJECT DeviceObject,
    __in PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
);

ULONG
ClassQueryTimeOutRegistryValue(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
ClassReadDriveCapacity(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
ClassAcquireChildLock(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

VOID
ClassReleaseChildLock(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension
);

VOID
ClassAddChild(
    IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
    IN PPHYSICAL_DEVICE_EXTENSION Child,
    IN BOOLEAN AcquireLock
);

PPHYSICAL_DEVICE_EXTENSION
ClassRemoveChild(
    IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
    IN PPHYSICAL_DEVICE_EXTENSION Child,
    IN BOOLEAN AcquireLock
);

BOOLEAN
ClassMarkChildMissing(
    IN PPHYSICAL_DEVICE_EXTENSION PhysicalDeviceExtension,
    IN BOOLEAN AcquireChildLock
);

VOID
ClassSendDeviceIoControlSynchronous(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID Buffer,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    OUT PIO_STATUS_BLOCK IoStatus
);

NTSTATUS
ClassSendSrbSynchronous(
    IN PDEVICE_OBJECT Fdo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID BufferAddress,
    IN ULONG BufferLength,
    IN BOOLEAN WriteToDevice
);

NTSTATUS
ClasspSendSynchronousCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
ClassSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
);

VOID
ClassUpdateInformationInRegistry(
    IN PDEVICE_OBJECT Fdo,
    IN PCHAR DeviceName,
    IN ULONG DeviceNumber,
    IN PINQUIRYDATA InquiryData,
    IN ULONG InquiryDataLength
);

NTSTATUS
ClasspAllocateReleaseRequest(
    IN PDEVICE_OBJECT Fdo
);

NTSTATUS
ClassPnpStartDevice(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
ClassPnpQueryFdoRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
);

NTSTATUS
ClassGetPdoId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING IdString
);

NTSTATUS
ClassQueryPnpCapabilities(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_CAPABILITIES Capabilities
);

NTSTATUS ClasspWriteCacheProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb
);

VOID
ClasspRetryRequestDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Arg1,
    IN PVOID Arg2
);

VOID
ClasspScanForSpecialInRegistry(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

VOID
ClasspScanForClassHacks(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN ULONG_PTR Data
);

NTSTATUS
ClasspInitializeHotplugInfo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

VOID
ClasspRegisterMountedDeviceInterface(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
ClasspScanForSpecialInRegistry(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

NTSTATUS
ClasspUpdateDiskProperties(
    IN PDEVICE_OBJECT Fdo
);

VOID
InterpretCapacityData(
    PDEVICE_OBJECT Fdo,
    PREAD_CAPACITY_DATA_EX ReadCapacityData
);

NTSTATUS
ServiceTransferRequest(
    PDEVICE_OBJECT Fdo,
    PIRP Irp
);

VOID
SetupReadWriteTransferPacket(
    PTRANSFER_PACKET Pkt,
    PVOID Buf, ULONG Len,
    LARGE_INTEGER DiskLocation,
    PIRP OriginalIrp
);

NTSTATUS
ClassRetrieveDeviceRelations(
    IN PDEVICE_OBJECT Fdo,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
);

//
// Prototypes for classwmi.c
//
NTSTATUS
ClassSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClassWmiCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG BufferUsed,
    IN CCHAR PriorityBoost
);

VOID
ClasspCleanupProtectedLocks(
    IN PFILE_OBJECT_EXTENSION FsContext
);

//
// Prototypes for clntirp.c
//
VOID
EnqueueDeferredClientIrp(
    PCLASS_PRIVATE_FDO_DATA FdoData,
    PIRP Irp
);

PIRP
DequeueDeferredClientIrp(
    PCLASS_PRIVATE_FDO_DATA FdoData
);

//
// Prototypes for create.c
//
NTSTATUS
ClassCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClasspCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClasspEjectionControl(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN MEDIA_LOCK_TYPE LockType,
    IN BOOLEAN Lock
);

NTSTATUS
ClasspDuidQueryProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

VOID
ClasspCleanupDisableMcn(
    IN PFILE_OBJECT_EXTENSION FsContext
);

//
// Prototypes for dictlib.c
//

VOID
InitializeDictionary(
    IN PDICTIONARY Dictionary
);

BOOLEAN
TestDictionarySignature(
    IN PDICTIONARY Dictionary
);

NTSTATUS
AllocateDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN ULONGLONG Key,
    IN ULONG Size,
    IN ULONG Tag,
    OUT PVOID *Entry
);

PVOID
GetDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN ULONGLONG Key
);

VOID
FreeDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN PVOID Entry
);

//
// Prototypes for dispatch.c
//
VOID
ClassInitializeDispatchTables(
    PCLASS_DRIVER_EXTENSION DriverExtension
);

NTSTATUS
ClassGlobalDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for obsolete.c
//
VOID
RetryRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN Associated,
    ULONG RetryInterval
);

VOID
ClassFreeOrReuseSrb(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

VOID
ClassDeleteSrbLookasideList(
    IN PCOMMON_DEVICE_EXTENSION CommonExtension
);

VOID
ClassInitializeSrbLookasideList(
    IN PCOMMON_DEVICE_EXTENSION CommonExtension,
    IN ULONG NumberElements
);

//
// Prototypes for power.c
//
NTSTATUS
ClassMinimalPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClassDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
ClassSpinDownPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Prototypes for utils.c
//
VOID
ClassGetDeviceParameter(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    __in_opt PWSTR SubkeyName OPTIONAL,
    __in PWSTR ParameterName,
    IN OUT PULONG ParameterValue
);

NTSTATUS
ClassSetDeviceParameter(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    __in_opt PWSTR SubkeyName,
    __in PWSTR ParameterName,
    IN ULONG ParameterValue
);

VOID
ClassScanForSpecial(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN CLASSPNP_SCAN_FOR_SPECIAL_INFO DeviceList[],
    IN PCLASS_SCAN_FOR_SPECIAL_HANDLER CallBackFunction
);

VOID
ClasspPerfIncrementErrorCount(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

VOID
ClasspPerfIncrementSuccessfulIo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

PMDL
BuildDeviceInputMdl(
    PVOID Buffer,
    ULONG BufferLen
);

VOID
FreeDeviceInputMdl(
    PMDL Mdl
);

BOOLEAN
ClasspMyStringMatches(
    __in_opt IN PCHAR StringToMatch OPTIONAL,
    __in IN PCHAR TargetString
);

//
// Prototypes for xferpkt.c
//
PTRANSFER_PACKET
DequeueFreeTransferPacket(
    PDEVICE_OBJECT Fdo,
    BOOLEAN AllocIfNeeded
);

NTSTATUS
SubmitTransferPacket(
    PTRANSFER_PACKET Pkt
);

VOID
SetupModeSenseTransferPacket(
    PTRANSFER_PACKET Pkt,
    PKEVENT SyncEventPtr,
    PVOID ModeSenseBuffer,
    UCHAR ModeSenseBufferLen,
    UCHAR PageMode,
    PIRP OriginalIrp,
    UCHAR PageControl
);

NTSTATUS
InitializeTransferPackets(
    PDEVICE_OBJECT Fdo
);

PTRANSFER_PACKET
NewTransferPacket(
    PDEVICE_OBJECT Fdo
);

VOID
EnqueueFreeTransferPacket(
    PDEVICE_OBJECT Fdo,
    PTRANSFER_PACKET Pkt
);

VOID
DestroyTransferPacket(
    PTRANSFER_PACKET Pkt
);

VOID
SetupDriveCapacityTransferPacket(
    TRANSFER_PACKET *Pkt,
    PVOID ReadCapacityBuffer,
    ULONG ReadCapacityBufferLen,
    PKEVENT SyncEventPtr,
    PIRP OriginalIrp,
    BOOLEAN Use16ByteCdb
);

NTSTATUS
TransferPktComplete(
    IN PDEVICE_OBJECT NullFdo,
    IN PIRP Irp,
    IN PVOID Context
);

//
// Prototypes for retry.c
//

BOOLEAN
InterpretTransferPacketError(
    PTRANSFER_PACKET Pkt
);

BOOLEAN
RetryTransferPacket(
    PTRANSFER_PACKET Pkt
);

BOOLEAN
StepLowMemRetry(
    PTRANSFER_PACKET Pkt
);

VOID
InitLowMemRetry(
    PTRANSFER_PACKET Pkt,
    PVOID BufPtr,
    ULONG Len,
    LARGE_INTEGER TargetLocation
);


//
// Prototypes for debug.c
//
char
*DbgGetIoctlStr(
    ULONG Ioctl
);

VOID
DbgCheckReturnedPkt(
    TRANSFER_PACKET *Pkt
);

VOID
DbgLogSendPacket(
    TRANSFER_PACKET *Pkt
);

VOID
DbgLogReturnPacket(
TRANSFER_PACKET *Pkt
);

VOID
DbgLogFlushInfo(
    PCLASS_PRIVATE_FDO_DATA FdoData,
    BOOLEAN IsIO,
    BOOLEAN IsFUA,
    BOOLEAN IsFlush
);
