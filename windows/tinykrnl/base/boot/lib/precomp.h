/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 09-May-06

--*/

//
// This hack is needed because the WDK defines NTKERNELAPI
// as DECLSPEC_IMPORT, and this generates warnings.
//
#include "excpt.h"
#include "ntdef.h"
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT

//
// Now include our files
//
#include "bldr.h"
#include "bldrx86.h"
#include "stdio.h"
#include "stdlib.h"
#include "ntdddisk.h"
#include "blfat.h"
#include "bldrpx86.h"
#include "asmlayer.h"

//
// Temporary debugging macro
//
#define NtUnhandled()                           \
{                                               \
    DbgPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                            \
}

//
// Macro to cache-align buffers
//
#define ALIGN_BUFFER(Buffer) (PVOID) \
    ((((ULONG)(Buffer) + BlDcacheFillSize - 1)) & (~(BlDcacheFillSize - 1)))

//
// Definitions
//
#define BLDR_DISK_CACHE_ENTRIES         2
#define BLDR_CLOSE_NOTIFICATIONS        5
#define BLDR_CACHED_ENTRIES             48
#define BLDR_FILE_TABLE_MAX_NAME_CHARS  32
#define BLDR_STATIC_BUFFER_SIZE         9216

//
// ARC Doesn't provide renaming, this is an extension
//
typedef
ARC_STATUS
(*PARC_RENAME)(
    IN ULONG FileId,
    IN PCHAR NewName
);

//
// Notification for closed device
//
typedef
VOID
(*PBL_DEVICE_CLOSE_NOTIFY_ROUTINE)(
    IN ULONG Handle
);

//
// Heap Allocation Policy
//
typedef enum _BL_HEAP_ALLOCATION_POLICY
{
    BlLowestFitPolicy,
    BlBestFitPolicy,
    BlHighestFitPolicy
} BL_HEAP_ALLOCATION_POLICY, *PBL_HEAP_ALLOCATION_POLICY;

//
// Device routines
//
typedef enum _BL_DEVICE_METHODS
{
    CloseMethod,
    MountMethod,
    OpenMethod,
    ReadMethod,
    GetReadStatusMethod,
    SeekMethod,
    WriteMethod,
    GetFileInformationMethod,
    SetFileInformationMethod,
    RenameMethod,
    GetDirectoryEntryMethod,
    MaximumMethod,
} BL_DEVICE_METHODS;

//
// Redirection Data
//
typedef struct _BL_REDIRECTION_INFORMATION
{
    USHORT Active;
    UCHAR StopBits;
    UCHAR Parity;
    ULONG BaudSpeed;
    ULONG ComPort;
    ULONG Address;
    USHORT PciDeviceId;
    USHORT PciVendorId;
    UCHAR BusNumber;
    UCHAR DeviceNumber;
    UCHAR FunctionNumber;
    ULONG Reserved;
    ULONG Flags;
    GUID Guid;
    UCHAR AddressType;
    UCHAR TerminalType;
} BL_REDIRECTION_INFORMATION, *PBL_REDIRECTION_INFORMATION;

//
// Context for the device owning the file in the file table
//
typedef struct _BL_DEVICE_CONTEXT
{
    PVOID DeviceVector[MaximumMethod];
    PWCHAR *DriverName;
} BL_DEVICE_CONTEXT, *PBL_DEVICE_CONTEXT;

//
// All FS Structures supported are in a union before for the FS Recognizer
//
typedef union
{
    //BL_NET_FILE_SYSTEM_CONTEXT Net;
    BL_FAT_FILE_SYSTEM_CONTEXT Fat;
    //BL_CDFS_FILE_SYSTEM_CONTEXT Cdfs;
    //BL_UDFS_FILE_SYSTEM_CONTEXT Udfs;
    //BL_NTFS_FILE_SYSTEM_CONTEXT Ntfs;
    //BL_ETFS_FILE_SYSTEM_CONTEXT Etfs;
} BL_FILE_SYSTEM_CONTEXT, *PBL_FILE_SYSTEM_CONTEXT;

//
// File Table Contexts
//
typedef struct _BL_ARC_PARTITION_DATA
{
    LARGE_INTEGER Length;
    ULONG Start;
    ULONG End;
    UCHAR DiskNumber;
    UCHAR DeviceId;
    UCHAR TargetId;
    UCHAR PathId;
    ULONG SectorShift;
    ULONG SectorSize;
    struct _DEVICE_OBJECT *PortDeviceObject;
} BL_ARC_PARTITION_DATA, *PBL_ARC_PARTITION_DATA;

typedef struct _BL_ARC_SERIAL_DATA
{
    ULONG PortBase;
    ULONG PortNumber;
} BL_ARC_SERIAL_DATA, *PBL_ARC_SERIAL_DATA;

typedef struct _BL_ARC_DRIVE_DATA
{
    BOOLEAN IsCd;
    UCHAR DriveId;
    UCHAR Sectors;
    USHORT Cylinders;
    USHORT Heads;
    UCHAR SupportsAh48;
} BL_ARC_DRIVE_DATA, *PBL_ARC_DRIVE_DATA;

typedef struct _BL_ARC_FLOPPY_DATA
{
    ULONG DriveType;
    ULONG SectorsPerTrack;
    UCHAR DiskNumber;
} BL_ARC_FLOPPY_DATA, *PBL_ARC_FLOPPY_DATA;

typedef struct _BL_ARC_KEYBOARD_DATA
{
    BOOLEAN HasScanCode;
} BL_ARC_KEYBOARD_DATA, *PBL_ARC_KEYBOARD_DATA;

typedef struct _BL_ARC_CONSOLE_DATA
{
    ULONG ConsoleId;
} BL_ARC_CONSOLE_DATA, *PBL_ARC_CONSOLE_DATA;

//
// File Table Flags
//
typedef struct _BL_FILE_TABLE_FLAGS
{
    ULONG Open:1;
    ULONG Read:1;
    ULONG Write:1;
} BL_FILE_TABLE_FLAGS, *PBL_FILE_TABLE_FLAGS;

//
// File Table Structure
//
typedef struct _BL_FILE_TABLE
{
    BL_FILE_TABLE_FLAGS Flags;
    ULONG DeviceId;
    LARGE_INTEGER CurrentOffset;
    PBL_FILE_SYSTEM_CONTEXT FileSystemContext;
    PBL_DEVICE_CONTEXT DeviceContext;
    UCHAR FileNameLength;
    CHAR FileName[BLDR_FILE_TABLE_MAX_NAME_CHARS];
    union
    {
        BL_FAT_FILE_DATA FatFileData;
        //BL_CDFS_FILE_DATA CdfsFileData;
        //BL_ETFS_FILE_DATA EtfsFileData;
        //BL_UDFS_FILE_DATA UdfsFileData;
        //BL_NTFS_FILE_DATA NtfsFileData;
        BL_ARC_PARTITION_DATA PartitionData;
        BL_ARC_SERIAL_DATA SerialData;
        BL_ARC_DRIVE_DATA DriveData;
        BL_ARC_FLOPPY_DATA FloppyData;
        BL_ARC_KEYBOARD_DATA KeyboardData;
        BL_ARC_CONSOLE_DATA ConsoleData;
    };
} BL_FILE_TABLE, *PBL_FILE_TABLE;

//
// File System Lookup Cache
//
typedef struct _BL_FS_DEVICE_CACHE
{
    ULONG DeviceId;
    PBL_DEVICE_CONTEXT DeviceContext;
    PBL_FILE_SYSTEM_CONTEXT FileSystemContext;
} BL_FS_DEVICE_CACHE, *PBL_FS_DEVICE_CACHE;

//
// Firmware LastSector Cache
//
typedef struct _FW_LAST_SECTOR_CACHE
{
    BOOLEAN PoolAllocated;
    BOOLEAN InUse;
    ULONG FileHandle;
    LARGE_INTEGER CurrentSector;
    PVOID Buffer;
} FW_LAST_SECTOR_CACHE, *PFW_LAST_SECTOR_CACHE;

//
// Cache Manager structures
//
typedef struct _BL_DISK_CACHE_ENTRY
{
    BOOLEAN Valid;
    ULONG DeviceId;
    ULONG u8;
    ULONG uC;
    ULONG u10;
    ULONG u14;
    ULONG u18;
} BL_DISK_CACHE_ENTRY, *PBL_DISK_CACHE_ENTRY;

typedef struct _BL_DISK_CACHE
{
    BL_DISK_CACHE_ENTRY Entry[BLDR_DISK_CACHE_ENTRIES];
    LIST_ENTRY List1;
    PVOID Cache1;
    PVOID Cache2;
    LIST_ENTRY List2;
    BOOLEAN Initialized;
} BL_DISK_CACHE, *PBL_DISK_CACHE;

//
// File Flag Checks
//
#define BlIsFileOpen(Handle) (BlFileTable[Handle].Flags.Open)
#define BlIsFileReadable(Handle) (BlFileTable[Handle].Flags.Read)
#define BlIsFileWriteable(Handle) (BlFileTable[Handle].Flags.Write)

//
// Device Routines
//
#define BlDeviceClose \
    ((PARC_CLOSE)(BlFileTable[Handle].DeviceContext->DeviceVector[CloseMethod]))
#define BlDeviceRead \
    ((PARC_READ)(BlFileTable[Handle].DeviceContext->DeviceVector[ReadMethod]))
#define BlDeviceSeek \
    ((PARC_SEEK)(BlFileTable[Handle].DeviceContext->DeviceVector[SeekMethod]))
#define BlDeviceWrite \
    ((PARC_WRITE)(BlFileTable[Handle].DeviceContext->DeviceVector[WriteMethod]))
#define BlDeviceGetFileInformation \
    ((PARC_GET_FILE_INFO)(BlFileTable[Handle].DeviceContext->DeviceVector[GetFileInformationMethod]))
#define BlDeviceOpen \
    ((PARC_OPEN)(BlFileTable[Handle].DeviceContext->DeviceVector[OpenMethod]))

//
// Memory Manager Routines
//
PVOID
BlAllocateHeap(
    IN ULONG Size
);

VOID
BlInsertDescriptor(
    IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor
);

ARC_STATUS
BlMemoryInitialize(
    VOID
);

ARC_STATUS
BlAllocateAlignedDescriptor(
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount,
    IN ULONG Alignment,
    OUT PULONG AlignedBase
);

ULONG
BlDetermineOSVisibleMemory(
    VOID
);

VOID
BlpTrackUsage(
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount
);

//
// I/O Manager Routines
//
ULONG
DecompPrepareToReadCompressedFile(
    IN PCHAR FileName,
    IN ULONG Handle
);

BOOLEAN
BlDiskCacheMergeRangeRoutine(
    IN PCHAR FileName,
    OUT PCHAR CompressedName
);

PBL_DEVICE_CONTEXT
IsEtfsFileStructure(
    IN ULONG DeviceId,
    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext
);

PBL_DEVICE_CONTEXT
IsCdfsFileStructure(
    IN ULONG DeviceId,
    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext
);

PBL_DEVICE_CONTEXT
IsUDFSFileStructure(
    IN ULONG DeviceId,
    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext
);

PBL_DEVICE_CONTEXT
IsNtfsFileStructure(
    IN ULONG DeviceId,
    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext
);

PBL_DEVICE_CONTEXT
IsFatFileStructure(
    IN ULONG DeviceId,
    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext
);

PBL_DEVICE_CONTEXT
IsNetFileStructure(
    IN ULONG DeviceId,
    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext
);

ARC_STATUS
CdfsInitialize(
    VOID
);

ARC_STATUS
NtfsInitialize(
    VOID
);

ARC_STATUS
UDFSInitialize(
    VOID
);

ARC_STATUS
NetInitialize(
    VOID
);

ARC_STATUS
FatInitialize(
    VOID
);

ARC_STATUS
BlDiskCacheRead(
    IN ULONG DeviceId,
    IN PLARGE_INTEGER Lbo,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PULONG ReturnedLength,
    IN BOOLEAN Flag
);

//
// RAMDisk Support
//
ARC_STATUS
RamdiskOpen(
    IN PCHAR Path,
    IN OPEN_MODE OpenMode,
    OUT PULONG Handle
);

//
// SCSI Support
//
ARC_STATUS
ScsiDiskOpen(
    IN PCHAR Path,
    IN OPEN_MODE OpenMode,
    OUT PULONG Handle
);

ARC_STATUS
ScsiDiskSeek(
    IN ULONG Handle,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
);

ARC_STATUS
ScsiDiskClose(
    IN ULONG Handle
);

ARC_STATUS
ScsiDiskGetReadStatus(
    IN ULONG Handle
);

ARC_STATUS
ScsiDiskRead(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
ScsiDiskWrite(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
ScsiDiskGetFileInformation(
    IN ULONG Handle,
    OUT PFILE_INFORMATION FileInformation
);

ARC_STATUS
ScsiDiskMount(
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
);

//
// x86 BIOS ARC Wrapper Routines
//
ARC_STATUS
BiosConsoleOpen(
    IN PCHAR Path,
    IN OPEN_MODE OpenMode,
    OUT PULONG Handle
);

ARC_STATUS
HardDiskPartitionOpen(
    IN ULONG Handle,
    IN ULONG DriveNumber,
    IN ULONG PartitionNumber
);

ARC_STATUS
BiosDiskOpen(
    IN ULONG DriveNumber,
    IN OPEN_MODE OpenMode,
    OUT PULONG Handle
);

ARC_STATUS
BiosPartitionOpen(
    IN PCHAR Path,
    IN OPEN_MODE OpenMode,
    OUT PULONG Handle
);

ARC_STATUS
BiosPartitionSeek(
    IN ULONG Handle,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
);

ARC_STATUS
BiosPartitionClose(
    IN ULONG Handle
);

ARC_STATUS
BiosPartitionRead(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
BiosPartitionWrite(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
BiosPartitionGetFileInfo(
    IN ULONG Handle,
    OUT PFILE_INFORMATION FileInformation
);

ARC_STATUS
BiosDiskSeek(
    IN ULONG Handle,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
);

ARC_STATUS
BiosDiskClose(
    IN ULONG Handle
);

ARC_STATUS
BiosDiskRead(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
BiosElToritoDiskRead(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
BiosDiskWrite(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
BiosDiskGetFileInfo(
    IN ULONG Handle,
    OUT PFILE_INFORMATION FileInformation
);

//
// Path Routines
//
BOOLEAN
FwGetPathMnemonicKey(
    IN PCHAR Path,
    IN PCHAR Mnemonic,
    IN PULONG PathId
);

BOOLEAN
BlGetPathMnemonicKey(
    IN PCHAR Path,
    IN PCHAR Mnemonic,
    IN PULONG PathId
);

//
// Generic Display routines
//
VOID
TextSetCursorPosition(
    IN USHORT X,
    IN USHORT Y
);

//
// Text-Mode Display routines (avoid calling directly)
//
VOID
TextTmPositionCursor(
    IN USHORT Row,
    IN USHORT Column
);

PUCHAR
TextTmCharOut(
    IN PUCHAR Char
);

//
// ACPI/BIOS Routines
//
PVOID
BlFindACPITable(
    IN PCHAR Signature,
    IN ULONG TableSize
);

//
// Externals
//
extern BL_FILE_TABLE BlFileTable[BLDR_CACHED_ENTRIES];
extern BL_DEVICE_CONTEXT ScsiDiskEntryTable;
extern ULONG BlDcacheFillSize;
extern PCHAR OsLoaderName;
extern USHORT TextColumn;
extern USHORT TextRow;
extern UCHAR TextCurrentAttribute;
extern PVOID BlpResourceDirectory;
extern ULONG BlpResourceFileOffset;
