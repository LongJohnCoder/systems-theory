/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fatstruc.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#ifndef _FATSTRUC_H_
#define _FATSTRUC_H_

//
// Types.
//
typedef PVOID PBCB;

typedef struct _VCB VCB;
typedef VCB *PVCB;
typedef struct _FCB FCB;
typedef FCB *PFCB;

//
// Defines.
//
#define FAT_FILL_FREE                   0
#define REPINNED_BCBS_ARRAY_SIZE        (4)

#define IRP_CONTEXT_FLAG_DISABLE_POPUPS (0x00000020)
#define IRP_CONTEXT_FLAG_WAIT           (0x00000002)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH  (0x00000004)
#define IRP_CONTEXT_FLAG_RECURSIVE_CALL (0x00000010)
#define IRP_CONTEXT_STACK_IO_CONTEXT    (0x00000100)

#define VCB_STATE_FLAG_SHUTDOWN         (0x00000040)
#define VCB_STATE_FLAG_MOUNTED_DIRTY    (0x00000010)

//
// Structures/Enumerations.
//
typedef struct _FAT_DATA
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    PVOID LazyWriteThread;
    LIST_ENTRY VcbQueue;
    PDRIVER_OBJECT DriverObject;
    PVOID DiskFileSystemDeviceObject;
    PVOID CdromFileSystemDeviceObject;
    ERESOURCE Resource;
    PEPROCESS OurProcess;
    BOOLEAN ChicagoMode:1;
    BOOLEAN FujitsuFMR:1;
    BOOLEAN AsyncCloseActive:1;
    BOOLEAN ShutdownStarted:1;
    BOOLEAN CodePageInvariant:1;
    BOOLEAN HighAsync:1;
    BOOLEAN HighDelayed:1;
    ULONG AsyncCloseCount;
    LIST_ENTRY AsyncCloseList;
    ULONG DelayedCloseCount;
    LIST_ENTRY DelayedCloseList;
    PIO_WORKITEM FatCloseItem;
    KSPIN_LOCK GeneralSpinLock;
    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerNoOpCallbacks;
} FAT_DATA, *PFAT_DATA;

typedef struct _FAT_WINDOW
{
    ULONG FirstCluster;
    ULONG LastCluster;
    ULONG ClustersFree;
} FAT_WINDOW, *PFAT_WINDOW;

typedef struct _FILE_NAME_NODE
{
    struct _FCB *Fcb;
    union
    {
        OEM_STRING Oem;
        UNICODE_STRING Unicode;
    } Name;
    BOOLEAN FileNameDos;
    RTL_SPLAY_LINKS Links;
} FILE_NAME_NODE, *PFILE_NAME_NODE;

typedef struct _FILE_SYSTEM_STATISTICS
{
    FILESYSTEM_STATISTICS Common;
    FAT_STATISTICS Fat;
    UCHAR Pad[64-(sizeof(FILESYSTEM_STATISTICS)+sizeof(FAT_STATISTICS))%64];
} FILE_SYSTEM_STATISTICS, *PFILE_SYSTEM_STATISTICS;

typedef struct _CLOSE_CONTEXT
{
    LIST_ENTRY GlobalLinks;
    LIST_ENTRY VcbLinks;
    PVCB Vcb;
    PFCB Fcb;
    enum _TYPE_OF_OPEN TypeOfOpen;
    BOOLEAN Free;
} CLOSE_CONTEXT, *PCLOSE_CONTEXT;

typedef enum _VCB_CONDITION
{
    VcbGood = 1,
    VcbNotMounted,
    VcbBad
} VCB_CONDITION, *PVCB_CONDITION;

typedef struct _VCB
{
    FSRTL_ADVANCED_FCB_HEADER VolumeFileHeader;
    LIST_ENTRY VcbLinks;
    PDEVICE_OBJECT TargetDeviceObject;
    PVPB Vpb;
    ULONG VcbState;
    VCB_CONDITION VcbCondition;
    struct _FCB *RootDcb;
    ULONG NumberOfWindows;
    PFAT_WINDOW Windows;
    PFAT_WINDOW CurrentWindow;
    CLONG DirectAccessOpenCount;
    SHARE_ACCESS ShareAccess;
    CLONG OpenFileCount;
    CLONG ReadOnlyCount;
    ULONG InternalOpenCount;
    ULONG ResidualOpenCount;
    BIOS_PARAMETER_BLOCK Bpb;
    PUCHAR First0x24BytesOfBootSector;
    struct
    {
        LBO RootDirectoryLbo;
        LBO FileAreaLbo;
        ULONG RootDirectorySize;
        ULONG NumberOfClusters;
        ULONG NumberOfFreeClusters;
        UCHAR FatIndexBitSize;
        UCHAR LogOfBytesPerSector;
        UCHAR LogOfBytesPerCluster;
    } AllocationSupport;
    LARGE_MCB DirtyFatMcb;
    RTL_BITMAP FreeClusterBitMap;
    FAST_MUTEX FreeClusterBitMapMutex;
    ERESOURCE Resource;
    ERESOURCE ChangeBitMapResource;
    PFILE_OBJECT VirtualVolumeFile;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    ULONG ClusterHint;
    PDEVICE_OBJECT CurrentDevice;
    PFILE_OBJECT VirtualEaFile;
    struct _FCB *EaFcb;
    PFILE_OBJECT FileObjectWithVcbLocked;
    LIST_ENTRY DirNotifyList;
    PNOTIFY_SYNC NotifySync;
    FAST_MUTEX DirectoryFileCreationMutex;
    PKTHREAD VerifyThread;
    KDPC CleanVolumeDpc;
    KTIMER CleanVolumeTimer;
    LARGE_INTEGER LastFatMarkVolumeDirtyCall;
    struct _FILE_SYSTEM_STATISTICS *Statistics;
    TUNNEL Tunnel;
    ULONG ChangeCount;
    PVPB SwapVpb;
    LIST_ENTRY AsyncCloseList;
    LIST_ENTRY DelayedCloseList;
    FAST_MUTEX AdvancedFcbHeaderMutex;
    PCLOSE_CONTEXT CloseContext;
#if DBG
    ULONG CloseContextCount;
#endif
} VCB, *PVCB;

typedef enum _FCB_CONDITION
{
    FcbGood = 1,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION, *PFCB_CONDITION;

typedef struct _NON_PAGED_FCB
{
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    ULONG OutstandingAsyncWrites;
    PKEVENT OutstandingAsyncEvent;
    FAST_MUTEX AdvancedFcbHeaderMutex;
} NON_PAGED_FCB, *PNON_PAGED_FCB;

typedef struct _FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    PNON_PAGED_FCB NonPaged;
    ULONG FirstClusterOfFile;
    LIST_ENTRY ParentDcbLinks;
    struct _FCB *ParentDcb;
    PVCB Vcb;
    ULONG FcbState;
    FCB_CONDITION FcbCondition;
    SHARE_ACCESS ShareAccess;
#ifdef SYSCACHE_COMPILE
    PULONG WriteMask;
    ULONG WriteMaskData;
#endif
    CLONG UncleanCount;
    CLONG OpenCount;
    CLONG NonCachedUncleanCount;
    VBO DirentOffsetWithinDirectory;
    VBO LfnOffsetWithinDirectory;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    ULONG ValidDataToDisk;
    LARGE_MCB Mcb;
    union
    {
        struct
        {
            LIST_ENTRY ParentDcbQueue;
            ULONG DirectoryFileOpenCount;
            PFILE_OBJECT DirectoryFile;
            VBO UnusedDirentVbo;
            VBO DeletedDirentHint;
            PRTL_SPLAY_LINKS RootOemNode;
            PRTL_SPLAY_LINKS RootUnicodeNode;
            RTL_BITMAP FreeDirentBitmap;
            ULONG FreeDirentBitmapBuffer[1];
        } Dcb;
        struct
        {
            FILE_LOCK FileLock;
            OPLOCK Oplock;
            PVOID LazyWriteThread;
        } Fcb;
    } Specific;
    ULONG EaModificationCount;
    FILE_NAME_NODE ShortName;
    UNICODE_STRING FullFileName;
    USHORT FinalNameLength;
    UCHAR DirentFatFlags;
    UNICODE_STRING ExactCaseLongName;
    union
    {
        FILE_NAME_NODE Oem;
        FILE_NAME_NODE Unicode;
    } LongName;
    PKEVENT MoveFileEvent;
} FCB, *PFCB;

typedef struct _CCB
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    ULONG Flags:24;
    BOOLEAN ContainsWildCards;
    union
    {
        struct
        {
            VBO OffsetToStartSearchFrom;
            union
            {
                OEM_STRING Wild;
                FAT8DOT3 Constant;
            } OemQueryTemplate;
            UNICODE_STRING UnicodeQueryTemplate;
            ULONG EaModificationCount;
            ULONG OffsetOfNextEaToReturn;
        };
        CLOSE_CONTEXT CloseContext;
    };
} CCB, *PCCB;

typedef struct _VOLUME_DEVICE_OBJECT
{
    DEVICE_OBJECT DeviceObject;
    ULONG PostedRequestCount;
    ULONG OverflowQueueCount;
    LIST_ENTRY OverflowQueue;
    KSPIN_LOCK OverflowQueueSpinLock;
    FSRTL_COMMON_FCB_HEADER VolumeFileHeader;
    VCB Vcb;
} VOLUME_DEVICE_OBJECT, *PVOLUME_DEVICE_OBJECT;

typedef struct _REPINNED_BCBS
{
    struct _REPINNED_BCBS *Next;
    PBCB Bcb[REPINNED_BCBS_ARRAY_SIZE];
} REPINNED_BCBS, *PREPINNED_BCBS;

typedef struct _IRP_CONTEXT
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    WORK_QUEUE_ITEM WorkQueueItem;
    PIRP OriginatingIrp;
    PDEVICE_OBJECT RealDevice;
    PVCB Vcb;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR PinCount;
    ULONG Flags;
    NTSTATUS ExceptionStatus;
    struct _FAT_IO_CONTEXT *FatIoContext;
    REPINNED_BCBS Repinned;
} IRP_CONTEXT, *PIRP_CONTEXT;

typedef struct _FAT_IO_CONTEXT
{
    LONG IrpCount;
    PIRP MasterIrp;
    PMDL ZeroMdl;
    union
    {
        struct
        {
            PERESOURCE Resource;
            PERESOURCE Resource2;
            ERESOURCE_THREAD ResourceThreadId;
            ULONG RequestedByteCount;
            PFILE_OBJECT FileObject;
            PNON_PAGED_FCB NonPagedFcb;
        } Async;
        KEVENT SyncEvent;
    } Wait;
} FAT_IO_CONTEXT, *PFAT_IO_CONTEXT;

typedef FCB DCB;
typedef DCB *PDCB;

#endif
