/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The Fault Tolerance Driver provides fault tolerance for disk by using
    disk mirroring and striping. Additionally, it creates disk device objects
    that represent volumes on Basic disks. For each volume, FtDisk creates a
    symbolic link of the form \Device\HarddiskVolumeX, identifying the volume.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 22-Apr-06

--*/
#define INITGUID
#include "ntddk.h"
#include "ntddft.h"
#include "ntddft2.h"
#include "ntddvol.h"
#include "ntdddisk.h"
#include "volmgr.h"
#include "partmgrp.h"
#include "wmilib.h"
#include "stdio.h"
#include "stdarg.h"
#include "wmiguid.h"
#include "mountdev.h"
#include "wmistr.h"

#define NtUnhandled()                               \
{                                                   \
    FtDebugPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                                \
}

VOID
FORCEINLINE
FtDebugPrint(IN PCCH DebugString,
             ...)
{
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    va_start(ap, DebugString);
    vDbgPrintExWithPrefix("FTDISK: ", -1, DPFLTR_ERROR_LEVEL, DebugString, ap);
    va_end(ap);
}

//
// Defines
//
#define DEVICE_EXTENSION_ROOT                       0
#define DEVICE_EXTENSION_VOLUME                     1

typedef
VOID
(*PUNKNOWN_CALLBACK)(
    IN PVOID Unknown,
    IN NTSTATUS Status
);

typedef
VOID
(*PFTP_CALLBACK_ROUTINE)(
    IN struct _VOLUME_EXTENSION *VolumeExtension
);

//
// Types
//
typedef struct _FT_LOGICAL_DISK_DESCRIPTION
{
    USHORT Offset;
    UCHAR DriveLetter;
    FT_LOGICAL_DISK_TYPE LogicalDiskType;
    FT_LOGICAL_DISK_ID DiskId;
    LARGE_INTEGER PartitionStart;
    ULONG m18;
    // ??? //
} FT_LOGICAL_DISK_DESCRIPTION, *PFT_LOGICAL_DISK_DESCRIPTION;

typedef struct _FT_DESCRIPTOR
{
    union
    {
        struct
        {
            ULONG Signature;
            ULONG Active;
            ULONG Size;
        };
        GUID PartitionGuid;
    };
} FT_DESCRIPTOR, *PFT_DESCRIPTOR;

//
// Class Definitions
//
typedef class FT_BASE_CLASS
{
    public:
        static
        PVOID
        operator new(
            IN size_t Size
        );

        static
        VOID
        operator delete(
            IN PVOID P
        );
} *PFT_BASE_CLASS;

typedef class FT_LOGICAL_DISK_INFORMATION : public FT_BASE_CLASS
{
public:
    NTSTATUS
    Initialize(
        IN struct _ROOT_EXTENSION *RootExtension,
        IN PDEVICE_OBJECT WholeDiskPdo
    );

    PFT_LOGICAL_DISK_DESCRIPTION
    GetFirstLogicalDiskDescriptor(
        VOID
    );

    PFT_LOGICAL_DISK_DESCRIPTION
    GetNextLogicalDiskDescription(
        IN PFT_LOGICAL_DISK_DESCRIPTION Current
    );

    NTSTATUS
    InitializeGptAttributes(
        VOID
    );

    ULONG64
    GetGptAttributes(
        VOID
    );

    ~FT_LOGICAL_DISK_INFORMATION(
        VOID
    );

    struct _ROOT_EXTENSION *RootExtension;          // 0x00
    PDEVICE_OBJECT AttachedDevice;                  // 0x04
    PDEVICE_OBJECT DeviceObject;                    // 0x08
    ULONG DeviceNumber;                             // 0x0C
    ULONG BytesPerSector;                           // 0x10
    ULONG m_14;                                     // 0x14
    LARGE_INTEGER StartingOffset;                   // 0x18
    ULONG TableLength;                              // 0x20
    PFT_DESCRIPTOR TableBuffer;                     // 0x24
    BOOLEAN Active;                                 // 0x28
    BOOLEAN Invalid;                                // 0x29
} *PFT_LOGICAL_DISK_INFORMATION;

typedef class FT_LOGICAL_DISK_INFORMATION_SET : public FT_BASE_CLASS
{
public:
    BOOLEAN
    QueryFtPartitionInformation(
        IN LARGE_INTEGER Size,
        OUT PLARGE_INTEGER PartitionStart,
        OUT PDEVICE_OBJECT* DeviceObject,
        OUT PULONG DiskNumber,
        OUT PULONG Unknown,
        OUT PLARGE_INTEGER PartitionSize
    );

    NTSTATUS
    Initialize(
        VOID
    );

    BOOLEAN
    IsDiskInSet(
        IN PDEVICE_OBJECT WholeDiskPdo
    );

    NTSTATUS
    AddLogicalDiskInformation(
        IN PFT_LOGICAL_DISK_INFORMATION DiskInformation,
        IN PBOOLEAN NeedsSync
    );

    NTSTATUS
    MigrateRegistryInformation(
        IN PDEVICE_OBJECT DeviceObject,
        IN ULONG DeviceNumber,
        IN LARGE_INTEGER StartingOffset
    );

    FT_LOGICAL_DISK_ID
    QueryPartitionLogicalDiskId(
        IN ULONG DeviceNumber,
        IN LARGE_INTEGER StartingOffset
    );

    PFT_LOGICAL_DISK_DESCRIPTION
    GetParentLogicalDiskDescription(
        IN PFT_LOGICAL_DISK_DESCRIPTION DiskDescriptrion,
        IN ULONG DeviceNumber
    );

    VOID
    RecomputeArrayOfRootLogicalDiskIds(
        VOID
    );

    PFT_LOGICAL_DISK_INFORMATION
    FindLogicalDiskInformation(
        IN PDEVICE_OBJECT WholeDiskPdo
    );

    LONGLONG
    QueryRootLogicalDiskIdForContainedPartition(
        IN ULONG DeviceNumber,
        IN LARGE_INTEGER StartingOffset
    );

    BOOLEAN
    ReallocRootLogicalDiskIds(
        IN ULONG DiskNumber
    );

    ~FT_LOGICAL_DISK_INFORMATION_SET(
        VOID
    );

    //
    // FIXME: TODO Write other class functions
    //

    ULONG Count;                                    // 0x00
    PFT_LOGICAL_DISK_INFORMATION *Entry;            // 0x04
    ULONG NumberOfRootLogicalDiskIds;               // 0x08
    PVOID m_0c;                                     // 0x0C
} *PFT_LOGICAL_DISK_INFORMATION_SET;

typedef class TRANSFER_PACKET : public FT_BASE_CLASS
{

} *PTRANSFER_PACKET;

typedef class PARITY_TP : public TRANSFER_PACKET
{

} *PPARITY_TP;

typedef class FT_VOLUME : public FT_BASE_CLASS
{
public:
    NTSTATUS
    Initialize(
        IN struct _ROOT_EXTENSION* RootExtension,
        IN LARGE_INTEGER Size
    );

    LARGE_INTEGER
    QueryLogicalDiskId(
        VOID
    );

    KSPIN_LOCK SpinLock;                            // 0x0C
    PFT_LOGICAL_DISK_INFORMATION_SET LogicalSet;    // 0x10
    struct _ROOT_EXTENSION* RootExtension;          // 0x14
    LARGE_INTEGER LogicalId;                        // 0x18
} *PFT_VOLUME;

typedef class PARTITION : public FT_VOLUME
{
public:
    NTSTATUS
    Initialize(
        IN struct _ROOT_EXTENSION* RootExtension,
        IN LARGE_INTEGER Size,
        IN PDEVICE_OBJECT DeviceObject,
        IN PDEVICE_OBJECT AttachedDeviceObject
    );

    virtual
    VOID
    Notify(
        VOID
    );

    virtual
    FT_LOGICAL_DISK_TYPE
    QueryLogicalDiskType(
        VOID
    );

    virtual
    USHORT
    QueryNumberOfMembers(
        VOID
    );

    virtual
    PFT_VOLUME
    GetMember(
        IN USHORT Unknown
    );

    virtual
    NTSTATUS
    OrphanMember(
        IN USHORT Unknown,
        IN PUNKNOWN_CALLBACK Unknown2,
        IN PVOID Unknown3
    );

    virtual
    NTSTATUS
    RegenerateMember(
        IN USHORT Unknown,
        IN PUNKNOWN_CALLBACK Unknown2,
        IN PVOID Unknown3
    );

    virtual
    VOID
    Transfer(
        IN PTRANSFER_PACKET TransferPacket
    );

    virtual
    VOID
    ReplaceBadSector(
        IN PTRANSFER_PACKET TransferPacket
    );

    virtual
    VOID
    StartSyncOperations(
        IN BOOLEAN Reserved,
        IN PUNKNOWN_CALLBACK Unknown,
        IN PVOID Unknown2
    );

    virtual
    VOID
    StopSyncOperations(
        VOID
    );

    virtual
    VOID
    BroadcastIrp(
        IN PIRP Irp,
        IN PUNKNOWN_CALLBACK Unknown,
        IN PVOID Context
    );

    virtual
    ULONG
    QuerySectorSize(
        VOID
    );

    virtual
    LARGE_INTEGER
    QueryVolumeSize(
        VOID
    );

    virtual
    PFT_VOLUME
    GetContainedLogicalDisk(
        IN ULONG Signature,
        IN LARGE_INTEGER Start
    );

    virtual
    PFT_VOLUME
    GetContainedLogicalDisk(
        IN PDEVICE_OBJECT DeviceObject
    );

    virtual
    PFT_VOLUME
    GetContainedLogicalDisk(
        IN LARGE_INTEGER Identifier
    );

    virtual
    PFT_VOLUME
    GetParentLogicalDisk(
        IN PFT_VOLUME Identifier
    );

    virtual
    VOID
    SetDirtyBit(
        IN BOOLEAN Reserved,
        IN PUNKNOWN_CALLBACK Unknown,
        IN PVOID Data
    );

    virtual
    VOID
    SetMember(
        IN USHORT Member,
        IN PFT_VOLUME Volume
    );

    virtual
    BOOLEAN
    IsComplete(
        IN BOOLEAN Reserved
    );

    virtual
    VOID
    CompleteNotification(
        IN BOOLEAN Reserved
    );

    virtual
    NTSTATUS
    CheckIo(
        OUT PBOOLEAN State
    );

    virtual
    ULONG
    QueryNumberOfPartitions(
        VOID
    );

    virtual
    NTSTATUS
    SetPartitionType(
        OUT UCHAR Type
    );

    virtual
    UCHAR
    QueryPartitionType(
        VOID
    );

    virtual
    UCHAR
    QueryStackSize(
        VOID
    );

    virtual
    VOID
    CreateLegacyNameLinks(
        IN PUNICODE_STRING Name
    );

    virtual
    PDEVICE_OBJECT
    GetLeftmostPartitionObject(
        VOID
    );

    virtual
    NTSTATUS
    QueryDiskExtents(
        IN PDISK_EXTENT *Extents,
        IN PULONG Count
    );

    virtual
    BOOLEAN
    QueryVolumeState(
        IN PFT_VOLUME Volume,
        OUT PFT_MEMBER_STATE State
    );

    virtual
    NTSTATUS
    QueryPhysicalOffsets(
        IN LARGE_INTEGER Unknown,
        OUT PVOLUME_PHYSICAL_OFFSET *PhysicalOffset,
        IN PULONG Unknown2
    );

    virtual
    NTSTATUS
    QueryLogicalOffset(
        IN PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
        OUT PLARGE_INTEGER LogicalOffset
    );

    virtual
    ~PARTITION(
        VOID
    );

private:
    PDEVICE_OBJECT DeviceObject;                    // 0x20
    PDEVICE_OBJECT AttachedDeviceObject;            // 0x24
    ULONG BytesPerSector;                           // 0x28
    ULONG m_2c;
    LARGE_INTEGER PartitionOffset;                  // 0x30
    LARGE_INTEGER PartitionSize;                    // 0x38
    PIRP CompletionIrp;                             // 0x40
    BOOLEAN ActiveIrp;                              // 0x44
    LIST_ENTRY CompletionIrpList;                   // 0x48
} *PPARTITION;

//
// Structures
//
typedef struct _FTP_GPT_ATTRIBUTE_REVERT_ENTRY
{
    ULONG m_0;                                      // 0x00
} FTP_GPT_ATTRIBUTE_REVERT_ENTRY, *PFTP_GPT_ATTRIBUTE_REVERT_ENTRY;

typedef struct _ROOT_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;                    // 0x00
    struct _ROOT_EXTENSION *RootExtension;          // 0x04
    ULONG DeviceExtensionType;                      // 0x08
    KSPIN_LOCK SpinLock;                            // 0x0C
    PDRIVER_OBJECT DriverObject;                    // 0x10
    PDEVICE_OBJECT AttachedObject;                  // 0x14
    PDEVICE_OBJECT TargetObject;                    // 0x18
    LIST_ENTRY VolumeList;                          // 0x1C
    LIST_ENTRY IrpList;                             // 0x24
    ULONG VolumeCount;                              // 0x2C
    FT_LOGICAL_DISK_INFORMATION_SET *LocalDiskSet;  // 0x30
    UCHAR m_34_b;
    LIST_ENTRY WorkerQueue;                         // 0x38
    KSEMAPHORE WorkerSemaphore;                     // 0x40
    ULONG SomeCount;                                // 0x54
    LIST_ENTRY ChangeNotifyIrpList;                 // 0x58
    KSEMAPHORE ChangeSemaphore;                     // 0x60
    UNICODE_STRING SymbolicLink;                    // 0x74
    BOOLEAN BootReInitComplete;                     // 0x7C
    BOOLEAN HasParity;                              // 0x7D
    BOOLEAN ReInitComplete;                         // 0x7E
    BOOLEAN m_7f;
    UNICODE_STRING RegistryPath;                    // 0x80
    PM_WMI_COUNTER_CONTEXT PmWmiCounterContext;     // 0x88
    GUID DiskGuid;                                  // 0x9C
    ULONG RevertEntryCount;                         // 0xAC
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY RevertEntry;    // 0xB0
    ULONG VolumeExtensionCount;                     // 0xB4
} ROOT_EXTENSION, *PROOT_EXTENSION;

typedef struct _VOLUME_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;                    // 0x00
    struct _ROOT_EXTENSION *RootExtension;          // 0x04
    ULONG DeviceExtensionType;                      // 0x08
    KSPIN_LOCK SpinLock;                            // 0x0C
    PDEVICE_OBJECT TargetObject;                    // 0x10
    PFT_VOLUME Volume;                              // 0x14
    PEX_RUNDOWN_REF_CACHE_AWARE RundownProtect;     // 0x18
    PFTP_CALLBACK_ROUTINE ZeroRefCallback;          // 0x1C
    PKEVENT CallbackEvent;                          // 0x20
    LIST_ENTRY IrpList;                             // 0x24
    BOOLEAN Started;                                // 0x2C
    BOOLEAN Flag2D;
    BOOLEAN Flag2E;
    BOOLEAN Flag2F;
    BOOLEAN Online;                                 // 0x30
    BOOLEAN Flag31;
    BOOLEAN Flag32;
    BOOLEAN HasNewStyleName;                        // 0x33
    BOOLEAN Gpt;                                    // 0x34
    BOOLEAN Hidden;                                 // 0x35
    BOOLEAN Empty;                                  // 0x36
    BOOLEAN ReadOnly;                               // 0x37
    BOOLEAN SystemPartition;                        // 0x38
    BOOLEAN Installed;                              // 0x39
    BOOLEAN Flag3A;
    BOOLEAN Flag3B;
    volatile LONG CallbackState;                    // 0x3C
    ULONG OfflineOwner;                             // 0x40
    LIST_ENTRY VolumeListEntry;                     // 0x44
    ULONG VolumeCount;                              // 0x4C
    PPARITY_TP ParityPacket;                        // 0x50
    LIST_ENTRY TransferList;                        // 0x54
    ULONG u58;
    ULONG u5c;
    LIST_ENTRY ListEntry60;
    UNICODE_STRING VolumeName;                      // 0x68
    PDEVICE_OBJECT WholeDiskPdo;                    // 0x70
    PDEVICE_OBJECT AttachedObject;                  // 0x74
    LARGE_INTEGER StartingOffset;                   // 0x78
    LARGE_INTEGER PartitionLength;                  // 0x80
    ULONG SignatureCache;                           // 0x88
    UNICODE_STRING SymbolicName;                    // 0x8C
    KSEMAPHORE Semaphore1;                          // 0x94
    KSEMAPHORE Semaphore2;                          // 0xA8
    ULONG uBc;                                      // 0xBC
    GUID DiskGuid;                                  // 0xC0
    ULONG uD0;
    ULONG CounterEnabled;                           // 0xD4
    ULONG uD8;
    PVOID WmiCounterContext;                        // 0xDC
    PWMILIB_CONTEXT WmiLibInfo;                     // 0xE0
    ULONG64 RevertGptAttributes;                    // 0xE8
    PFILE_OBJECT FileObject;                        // 0xF0
    ULONG64 GptAttributes;                          // 0xF8
} VOLUME_EXTENSION, *PVOLUME_EXTENSION;

typedef struct _BROADCAST_CONTEXT
{
    ULONG Unknown[3];                               // 0x00
    PUNKNOWN_CALLBACK ErrorFunction;                // 0x0C
    PVOID Context;                                  // 0x10
    ULONG Unknown2;                                 // 0x14
} BROADCAST_CONTEXT, *PBROADCAST_CONTEXT;

//
// Externals
//

//
// Prototypes
//
extern "C"
{
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

VOID
FtpBootDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Context,
    IN ULONG Count
);

VOID
FtpDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Context,
    IN ULONG Count
);

NTSTATUS
FtpQueryRegistryRevertEntriesCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

NTSTATUS
FtpQueryRegistryRevertEntries(
    IN PROOT_EXTENSION RootExtension,
    OUT PFTP_GPT_ATTRIBUTE_REVERT_ENTRY *RevertEntry,
    OUT PULONG EntryCount
);

NTSTATUS
FtDiskDeviceControl(
    IN PDEVICE_OBJECT,
    IN OUT PIRP Irp
);

NTSTATUS
FtWmi(
    IN PDEVICE_OBJECT,
    IN OUT PIRP Irp
);

VOID
FtDiskUnload(
    IN PDRIVER_OBJECT Driver
);

ULONG
FtpQueryDiskSignature(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
FtpQueryPartitionInformation(
    IN PROOT_EXTENSION RootExtension,
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG DeviceNumber OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PULONG PartitionNumber OPTIONAL,
    IN PULONG PartitionType OPTIONAL,
    IN PLARGE_INTEGER PartitionLength OPTIONAL,
    IN LPGUID PartitionTypeGuid OPTIONAL,
    IN LPGUID PartitionIdGuid OPTIONAL,
    IN PBOOLEAN IsGpt OPTIONAL,
    IN PULONGLONG GptAttributes OPTIONAL
);

NTSTATUS
FtQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
);

NTSTATUS
FtQueryWmiDataBlock(
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
FtWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
);

NTSTATUS
FtpQuerySystemVolumeNameQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

NTSTATUS
FtpDiskRegistryQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

NTSTATUS
FtpPartitionArrivedHelper(
    IN PROOT_EXTENSION RootExtension,
    IN PDEVICE_OBJECT PartitionFdo,
    IN PDEVICE_OBJECT WholeDiskPdo
);

NTSTATUS
FtpPartitionArrived(
    IN PROOT_EXTENSION RootExtension,
    IN PIRP Irp
);

NTSTATUS
FtpReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDRIVE_LAYOUT_INFORMATION_EX *PartitionTable
);

ULONG
FtpQueryDiskSignatureCache(
    IN PVOLUME_EXTENSION VolumeExtension
);

NTSTATUS
FtpAllSystemsGo(
    IN PVOLUME_EXTENSION VolumeExtension,
    IN PIRP Irp,
    IN BOOLEAN Flag0,
    IN BOOLEAN Flag1,
    IN BOOLEAN Flag2
);

VOID
FtpCreateOldNameLinks(
    IN PVOLUME_EXTENSION VolumeExtension
);

BOOLEAN
FtpCreateNewDevice(
    IN PROOT_EXTENSION RootExtension,
    IN PDEVICE_OBJECT TargetObject,
    IN PFT_VOLUME FtVolume,
    IN PDEVICE_OBJECT WholeDiskPdo,
    IN ULONG AlignmentRequired,
    IN BOOLEAN UseNewName,
    IN BOOLEAN Hidden,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN System,
    IN ULONG64 GptAttributes
);

NTSTATUS
FtpQueryRootId(
    IN PROOT_EXTENSION RootExtension,
    IN PIRP Irp
);

NTSTATUS
FtDiskReadWrite(
    IN PDEVICE_OBJECT,
    IN OUT PIRP Irp
);

NTSTATUS
FtDiskShutdownFlush(
    IN PDEVICE_OBJECT,
    IN OUT PIRP Irp
);

NTSTATUS
FtCleanup(
    IN PDEVICE_OBJECT,
    IN OUT PIRP Irp
);

NTSTATUS
FtDiskInternalDeviceControl(
    IN PDEVICE_OBJECT,
    IN OUT PIRP Irp
);

NTSTATUS
FtRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
);

extern WMIGUIDREGINFO DiskperfGuidList[];
extern ULONG DiskperfGuidCount;

}
