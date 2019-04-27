#ifndef _ARC_
#define _ARC_

//
// Arc status/error types
//
typedef LONG ARC_STATUS;
typedef enum _ARC_ERRORS
{
    ESUCCESS,
    E2BIG,
    EACCES,
    EAGAIN,
    EBADF,
    EBUSY,
    EFAULT,
    EINVAL,
    EIO,
    EISDIR,
    EMFILE,
    EMLINK,
    ENAMETOOLONG,
    ENODEV,
    ENOENT,
    ENOEXEC,
    ENOMEM,
    ENOSPC,
    ENOTDIR,
    ENOTTY,
    ENXIO,
    EROFS,
    EMAXIMUM
} ARC_ERRORS;

typedef enum _IDENTIFIER_FLAG
{
    Failed = 0x01,
    ReadOnly = 0x02,
    Removable = 0x04,
    ConsoleIn = 0x08,
    ConsoleOut = 0x10,
    Input = 0x20,
    Output = 0x40
} IDENTIFIER_FLAG;

//
// ARC Functions
//
typedef enum _ARC_FUNCTION
{
    Load,
    Invoke,
    Execute,
    Halt,
    PowerDown,
    Restart,
    Reboot,
    InteractiveMode,
    Reserved,
    GetPeer,
    GetChild,
    GetParent,
    GetData,
    AddChild,
    DeleteComponent,
    GetComponent,
    SaveConfiguration,
    GetSystemId,
    GetMemoryDescriptor,
    Reserved2,
    GetTime,
    GetRelativeTime,
    GetDirectoryEntry,
    Open,
    Close,
    Read,
    ReadStatus,
    Write,
    Seek,
    Mount,
    GetEnvironment,
    SetEnvironment,
    GetFileInformation,
    SetFileInformation,
    FlushAllCaches,
    TestUnicodeCharacter,
    GetDisplayStatus,
} ARC_FUNCTION;

//
// Arc Function Flags
//
typedef enum _OPEN_MODE
{
    ArcOpenReadOnly,
    ArcOpenWriteOnly,
    ArcOpenReadWrite,
    ArcCreateWriteOnly,
    ArcCreateReadWrite,
    ArcSupersedeWriteOnly,
    ArcSupersedeReadWrite,
    ArcOpenDirectory,
    ArcCreateDirectory,
    ArcOpenMaximumMode
} OPEN_MODE;

typedef enum _SEEK_MODE
{
    SeekAbsolute,
    SeekRelative,
    SeekMaximum
} SEEK_MODE;

typedef enum _MOUNT_OPERATION
{
    MountLoadMedia,
    MountUnloadMedia,
    MountMaximum
} MOUNT_OPERATION;

typedef enum _CONFIGURATION_CLASS
{
    SystemClass,
    ProcessorClass,
    CacheClass,
    AdapterClass,
    ControllerClass,
    PeripheralClass,
    MemoryClass,
    MaximumClass
} CONFIGURATION_CLASS;

typedef enum _HwFileType
{
    HwFileDriver,
    HwFilePort,
    HwFileClass,
    HwFileInf,
    HwFileDll,
    HwFileDetect,
    HwFileHal,
    HwFileCatalog,
    HwFileMax,
    HwFileDynUpdt = 31,
} HwFileType;

//
// ARC Defined
//
typedef enum _MEMORY_TYPE
{
    MemoryExceptionBlock,
    MemorySystemBlock,
    MemoryFree,
    MemoryBad,
    MemoryLoadedProgram,
    MemoryFirmwareTemporary,
    MemoryFirmwarePermanent,
    MemoryFreeContiguous,
    MemorySpecialMemory,
    MemoryMaximum
} MEMORY_TYPE;

//
// NT Internal
//
typedef enum _TYPE_OF_MEMORY
{
    LoaderExceptionBlock,
    LoaderSystemBlock,
    LoaderFree,
    LoaderBad,
    LoaderLoadedProgram,
    LoaderFirmwareTemporary,
    LoaderFirmwarePermanent,
    LoaderOsloaderHeap,
    LoaderOsloaderStack,
    LoaderSystemCode,
    LoaderHalCode,
    LoaderBootDriver,
    LoaderConsoleInDriver,
    LoaderConsoleOutDriver,
    LoaderStartupDpcStack,
    LoaderStartupKernelStack,
    LoaderStartupPanicStack,
    LoaderStartupPcrPage,
    LoaderStartupPdrPage,
    LoaderRegistryData,
    LoaderMemoryData,
    LoaderNlsData,
    LoaderSpecialMemory,
    LoaderBBTMemory,
    LoaderReserve,
    LoaderXIPRom,
    LoaderHALCachedMemory,
    LoaderLargePageFiller,
    LoaderMaximum
} TYPE_OF_MEMORY;

typedef struct _MEMORY_ALLOCATION_DESCRIPTOR
{
    LIST_ENTRY ListEntry;
    TYPE_OF_MEMORY MemoryType;
    ULONG BasePage;
    ULONG PageCount;
} MEMORY_ALLOCATION_DESCRIPTOR, *PMEMORY_ALLOCATION_DESCRIPTOR;

typedef struct _MEMORY_DESCRIPTOR
{
    TYPE_OF_MEMORY MemoryType;
    ULONG BasePage;
    ULONG PageCount;
} MEMORY_DESCRIPTOR, *PMEMORY_DESCRIPTOR;

typedef struct _CONFIGURATION_COMPONENT
{
    CONFIGURATION_CLASS Class;
    CONFIGURATION_TYPE Type;
    IDENTIFIER_FLAG Flags;
    USHORT Version;
    USHORT Revision;
    ULONG Key;
    ULONG AffinityMask;
    ULONG ConfigurationDataLength;
    ULONG IdentifierLength;
    LPSTR Identifier;
} CONFIGURATION_COMPONENT, *PCONFIGURATION_COMPONENT;

typedef struct _CONFIGURATION_COMPONENT_DATA
{
    struct _CONFIGURATION_COMPONENT_DATA *Parent;
    struct _CONFIGURATION_COMPONENT_DATA *Child;
    struct _CONFIGURATION_COMPONENT_DATA *Sibling;
    CONFIGURATION_COMPONENT ComponentEntry;
    PVOID ConfigurationData;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

typedef struct _ARC_DISK_INFORMATION
{
    LIST_ENTRY DiskSignatureListHead;
} ARC_DISK_INFORMATION, *PARC_DISK_INFORMATION;

typedef struct _MONITOR_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    USHORT HorizontalResolution;
    USHORT HorizontalDisplayTime;
    USHORT HorizontalBackPorch;
    USHORT HorizontalFrontPorch;
    USHORT HorizontalSync;
    USHORT VerticalResolution;
    USHORT VerticalBackPorch;
    USHORT VerticalFrontPorch;
    USHORT VerticalSync;
    USHORT HorizontalScreenSize;
    USHORT VerticalScreenSize;
} MONITOR_CONFIGURATION_DATA, *PMONITOR_CONFIGURATION_DATA;

typedef struct _FLOPPY_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    CHAR Size[8];
    ULONG MaxDensity;
    ULONG MountDensity;
} FLOPPY_CONFIGURATION_DATA, *PFLOPPY_CONFIGURATION_DATA;

//
// ARC File Information
//
typedef struct _FILE_INFORMATION
{
    LARGE_INTEGER StartingAddress;
    LARGE_INTEGER EndingAddress;
    LARGE_INTEGER CurrentPosition;
    CONFIGURATION_TYPE Type;
    ULONG FileNameLength;
    UCHAR Attributes;
    CHAR FileName[32];
} FILE_INFORMATION, *PFILE_INFORMATION;

typedef struct SYSTEM_ID
{
    CHAR VendorId[8];
    CHAR ProductId[8];
} SYSTEM_ID, *PSYSTEM_ID;

typedef struct _DIRECTORY_ENTRY
{
    ULONG FileNameLength;
    UCHAR FileAttribute;
    CHAR FileName[32];
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef struct _ARC_DISPLAY_STATUS
{
    USHORT CursorXPosition;
    USHORT CursorYPosition;
    USHORT CursorMaxXPosition;
    USHORT CursorMaxYPosition;
    UCHAR ForegroundColor;
    UCHAR BackgroundColor;
    BOOLEAN HighIntensity;
    BOOLEAN Underscored;
    BOOLEAN ReverseVideo;
} ARC_DISPLAY_STATUS, *PARC_DISPLAY_STATUS;

//
// SMBIOS Table Header (FIXME: maybe move to smbios.h?)
//
typedef struct _SMBIOS_TABLE_HEADER
{
   CHAR Signature[4];
   UCHAR Checksum;
   UCHAR Length;
   UCHAR MajorVersion;
   UCHAR MinorVersion;
   USHORT MaximumStructureSize;
   UCHAR EntryPointRevision;
   UCHAR Reserved[5];
   CHAR Signature2[5];
   UCHAR IntermediateChecksum;
   USHORT StructureTableLength;
   ULONG StructureTableAddress;
   USHORT NumberStructures;
   UCHAR Revision;
} SMBIOS_TABLE_HEADER, *PSMBIOS_TABLE_HEADER;

//
// NLS Data Block
//
typedef struct _NLS_DATA_BLOCK
{
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCodePageData;
} NLS_DATA_BLOCK, *PNLS_DATA_BLOCK;

//
// Subsystem Specific Loader Blocks
//
typedef struct _PROFILE_PARAMETER_BLOCK
{
    USHORT Status;
    USHORT Reserved;
    USHORT DockingState;
    USHORT Capabilities;
    ULONG DockID;
    ULONG SerialNumber;
} PROFILE_PARAMETER_BLOCK, *PPROFILE_PARAMETER_BLOCK;

typedef struct _HEADLESS_LOADER_BLOCK
{
    UCHAR UsedBiosSettings;
    UCHAR DataBits;
    UCHAR StopBits;
    UCHAR Parity;
    ULONG BaudRate;
    ULONG PortNumber;
    PUCHAR PortAddress;
    USHORT PciDeviceId;
    USHORT PciVendorId;
    UCHAR PciBusNumber;
    UCHAR PciSlotNumber;
    UCHAR PciFunctionNumber;
    ULONG PciFlags;
    GUID SystemGUID;
    UCHAR IsMMIODevice;
    UCHAR TerminalType;
} HEADLESS_LOADER_BLOCK, *PHEADLESS_LOADER_BLOCK;

typedef struct _NETWORK_LOADER_BLOCK
{
    PCHAR DHCPServerACK;
    ULONG DHCPServerACKLength;
    PCHAR BootServerReplyPacket;
    ULONG BootServerReplyPacketLength;
} NETWORK_LOADER_BLOCK, *PNETWORK_LOADER_BLOCK;

typedef struct _LOADER_PERFORMANCE_DATA
{
    ULONGLONG StartTime;
    ULONGLONG EndTime;
} LOADER_PERFORMANCE_DATA, *PLOADER_PERFORMANCE_DATA;

//
// Extended Loader Parameter Block
//
typedef struct _LOADER_PARAMETER_EXTENSION
{
    ULONG Size;
    PROFILE_PARAMETER_BLOCK Profile;
    ULONG MajorVersion;
    ULONG MinorVersion;
    PVOID EmInfFileImage;
    ULONG EmInfFileSize;
    PVOID TriageDumpBlock;
    //
    // NT 5.1
    //
    ULONG LoaderPagesSpanned;
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PSMBIOS_TABLE_HEADER SMBiosEPSHeader;
    PVOID DrvDBImage;
    ULONG DrvDBSize;
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock;
    //
    // NT 5.2+
    //
    PCHAR HalpIRQLToTPR;
    PCHAR HalpVectorToIRQL;
    LIST_ENTRY FirmwareDescriptorListHead;
    PVOID AcpiTable;
    ULONG AcpiTableSize;
    //
    // NT 5.2 SP1+
    //
    ULONG BootViaWinload:1;
    ULONG BootViaEFI:1;
    ULONG Reserved:30;
    LOADER_PERFORMANCE_DATA LoaderPerformanceData;
    LIST_ENTRY BootApplicationPersistentData;
    PVOID WmdTestResult;
    GUID BootIdentifier;
} LOADER_PARAMETER_EXTENSION, *PLOADER_PARAMETER_EXTENSION;

//
// Architecture specific Loader Parameter Blocks
//
typedef struct _IA64_LOADER_BLOCK
{
    ULONG PlaceHolder;
} IA64_LOADER_BLOCK, *PIA64_LOADER_BLOCK;

typedef struct _ALPHA_LOADER_BLOCK
{
    ULONG PlaceHolder;
} ALPHA_LOADER_BLOCK, *PALPHA_LOADER_BLOCK;

typedef struct _I386_LOADER_BLOCK
{
    PVOID CommonDataArea;
    ULONG MachineType;
    ULONG Reserved;
} I386_LOADER_BLOCK, *PI386_LOADER_BLOCK;

//
// Loader Parameter Block
//
typedef struct _LOADER_PARAMETER_BLOCK
{
    LIST_ENTRY LoadOrderListHead;
    LIST_ENTRY MemoryDescriptorListHead;
    LIST_ENTRY BootDriverListHead;
    ULONG_PTR KernelStack;
    ULONG_PTR Prcb;
    ULONG_PTR Process;
    ULONG_PTR Thread;
    ULONG RegistryLength;
    PVOID RegistryBase;
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    LPSTR ArcBootDeviceName;
    LPSTR ArcHalDeviceName;
    LPSTR NtBootPathName;
    LPSTR NtHalPathName;
    LPSTR LoadOptions;
    PNLS_DATA_BLOCK NlsData;
    PARC_DISK_INFORMATION ArcDiskInformation;
    PVOID OemFontFile;
    struct _SETUP_LOADER_BLOCK *SetupLdrBlock;
    PLOADER_PARAMETER_EXTENSION Extension;
    union
    {
        I386_LOADER_BLOCK I386;
        ALPHA_LOADER_BLOCK Alpha;
        IA64_LOADER_BLOCK Ia64;
    } u;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

typedef struct _DEBUG_BLOCK
{
    ULONG Signature;
    ULONG Length;
} DEBUG_BLOCK, *PDEBUG_BLOCK;

typedef struct _BOOT_STATUS
{
    ULONG BootStarted:1;
    ULONG BootFinished:1;
    ULONG RestartStarted:1;
    ULONG RestartFinished:1;
    ULONG PowerFailStarted:1;
    ULONG PowerFailFinished:1;
    ULONG ProcessorReady:1;
} BOOT_STATUS, *PBOOT_STATUS;

typedef struct _RESTART_BLOCK
{
    ULONG Signature;
    ULONG Length;
    USHORT Version;
    USHORT Revision;
    struct _RESTART_BLOCK *NextRestartBlock;
    PVOID RestartAddress;
    ULONG BootMasterId;
    ULONG ProcessorId;
    BOOT_STATUS BootStatus;
    ULONG CheckSum;
    ULONG SaveAreaLength;
    ULONG SavedStateArea[];
} RESTART_BLOCK, *PRESTART_BLOCK;

typedef struct _SYSTEM_PARAMETER_BLOCK
{
    ULONG Signature;
    ULONG Length;
    USHORT Version;
    USHORT Revision;
    PRESTART_BLOCK RestartBlock;
    PDEBUG_BLOCK DebugBlock;
    PVOID GenerateExceptionVector;
    PVOID TlbMissExceptionVector;
    ULONG FirmwareVectorLength;
    PVOID *FirmwareVector;
    ULONG VendorVectorLength;
    PVOID *VendorVector;
    ULONG AdapterCount;
    struct
    {
        ULONG AdapterType;
        ULONG AdapterLength;
        PVOID *AdapterVector;
    } ADAPTERS[];
} SYSTEM_PARAMETER_BLOCK, *PSYSTEM_PARAMETER_BLOCK;

//
// ARC Standard Functions
//
typedef
ARC_STATUS
(*PARC_EXECUTE)(
    IN PCHAR ImagePath,
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]
);

typedef
ARC_STATUS
(*PARC_INVOKE)(
    IN ULONG EntryAddress,
    IN ULONG StackAddress,
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]
);

typedef
ARC_STATUS
(*PARC_LOAD)(
    IN PCHAR ImagePath,
    IN ULONG TopAddress,
    OUT PULONG EntryAddress,
    OUT PULONG LowAddress
);

typedef
VOID
(*PARC_HALT)(
    VOID
);

typedef
VOID
(*PARC_POWERDOWN)(
    VOID
);

typedef
VOID
(*PARC_RESTART)(
    VOID
);

typedef
VOID
(*PARC_REBOOT)(
    VOID
);

typedef
VOID
(*PARC_INTERACTIVE_MODE)(
    VOID
);

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_CHILD)(
    IN PCONFIGURATION_COMPONENT Component OPTIONAL
);

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_PARENT)(
    IN PCONFIGURATION_COMPONENT Component
);

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_PEER)(
    IN PCONFIGURATION_COMPONENT Component
);

typedef
PCONFIGURATION_COMPONENT
(*PARC_ADD_CHILD)(
    IN PCONFIGURATION_COMPONENT Component,
    IN PCONFIGURATION_COMPONENT NewComponent,
    IN PVOID ConfigurationData
);

typedef
ARC_STATUS
(*PARC_DELETE_COMPONENT)(
    IN PCONFIGURATION_COMPONENT Component
);

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_COMPONENT)(
    IN PCHAR Path
);

typedef
ARC_STATUS
(*PARC_GET_DATA)(
    OUT PVOID ConfigurationData,
    IN PCONFIGURATION_COMPONENT Component
);

typedef
ARC_STATUS
(*PARC_SAVE_CONFIGURATION)(
    VOID
);

typedef
PSYSTEM_ID
(*PARC_GET_SYSTEM_ID)(
    VOID
);

typedef
PMEMORY_DESCRIPTOR
(*PARC_MEMORY)(
    IN PMEMORY_DESCRIPTOR MemoryDescriptor OPTIONAL
);

typedef
PTIME_FIELDS
(*PARC_GET_TIME)(
    VOID
);

typedef
ULONG
(*PARC_GET_RELATIVE_TIME)(
    VOID
);

typedef
ARC_STATUS
(*PARC_CLOSE)(
    IN ULONG FileId
);

typedef
ARC_STATUS
(*PARC_MOUNT)(
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
);

typedef
ARC_STATUS
(*PARC_OPEN)(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
);

typedef
ARC_STATUS
(*PARC_READ)(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
);

typedef
ARC_STATUS
(*PARC_READ_STATUS)(
    IN ULONG FileId
);

typedef
ARC_STATUS
(*PARC_SEEK)(
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
);

typedef
ARC_STATUS
(*PARC_WRITE)(
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
);

typedef
ARC_STATUS
(*PARC_GET_FILE_INFO)(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInformation
);

typedef
ARC_STATUS
(*PARC_SET_FILE_INFO)(
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
);

typedef
ARC_STATUS
(*PARC_GET_DIRECTORY_ENTRY)(
     IN ULONG FileId,
     OUT PDIRECTORY_ENTRY Buffer,
     IN ULONG Length,
     OUT PULONG Count
);

typedef
PCHAR
(*PARC_GET_ENVIRONMENT)(
    IN PCHAR Variable
);

typedef
ARC_STATUS
(*PARC_SET_ENVIRONMENT)(
    IN PCHAR Variable,
    IN PCHAR Value
);

typedef
VOID
(*PARC_FLUSH_ALL_CACHES)(
    VOID
);

typedef
ARC_STATUS
(*PARC_TEST_UNICODE_CHARACTER)(
    IN ULONG FileId,
    IN WCHAR UnicodeCharacter
);

typedef
PARC_DISPLAY_STATUS
(*PARC_GET_DISPLAY_STATUS)(
    IN ULONG FileId
);

//
// Arc Firmware Vector Calls
//
extern SYSTEM_PARAMETER_BLOCK GlobalSystemBlock;
#define ArcExecute \
    ((PARC_EXECUTE)\
    (GlobalSystemBlock.FirmwareVector[Execute]))
#define ArcInvoke \
    ((PARC_INVOKE)\
    (GlobalSystemBlock.FirmwareVector[Invoke]))
#define ArcLoad \
    ((PARC_LOAD)\
    (GlobalSystemBlock.FirmwareVector[Load]))
#define ArcHalt \
    ((PARC_HALT)\
    (GlobalSystemBlock.FirmwareVector[Halt]))
#define ArcPowerDown \
    ((PARC_POWERDOWN)\
    (GlobalSystemBlock.FirmwareVector[PowerDown]))
#define ArcRestart \
    ((PARC_RESTART)\
    (GlobalSystemBlock.FirmwareVector[Restart]))
#define ArcReboot \
    ((PARC_REBOOT)\
    (GlobalSystemBlock.FirmwareVector[Reboot]))
#define ArcEnterInteractiveMode \
    ((PARC_INTERACTIVE_MODE)\
    (GlobalSystemBlock.FirmwareVector[InteractiveMode]))
#define ArcGetChild \
    ((PARC_GET_CHILD)\
    (GlobalSystemBlock.FirmwareVector[GetChild]))
#define ArcGetParent \
    ((PARC_GET_PARENT)\
    (GlobalSystemBlock.FirmwareVector[GetParent]))
#define ArcGetPeer \
    ((PARC_GET_PEER)\
    (GlobalSystemBlock.FirmwareVector[GetPeer]))
#define ArcAddChild \
    ((PARC_ADD_CHILD)\
    (GlobalSystemBlock.FirmwareVector[AddChild]))
#define ArcDeleteComponent \
    ((PARC_DELETE_COMPONENT)\
    (GlobalSystemBlock.FirmwareVector[DeleteComponent]))
#define ArcGetComponent \
    ((PARC_GET_COMPONENT)\
    (GlobalSystemBlock.FirmwareVector[GetComponent]))
#define ArcGetConfigurationData \
    ((PARC_GET_DATA)\
    (GlobalSystemBlock.FirmwareVector[GetData]))
#define ArcSaveConfiguration \
    ((PARC_SAVE_CONFIGURATION)\
    (GlobalSystemBlock.FirmwareVector[SaveConfiguration]))
#define ArcGetSystemId \
    ((PARC_GET_SYSTEM_ID)\
    (GlobalSystemBlock.FirmwareVector[GetSystemId]))
#define ArcGetMemoryDescriptor \
    ((PARC_MEMORY)\
    (GlobalSystemBlock.FirmwareVector[GetMemoryDescriptor]))
#define ArcGetTime \
    ((PARC_GET_TIME)\
    (GlobalSystemBlock.FirmwareVector[GetTime]))
#define ArcGetRelativeTime \
    ((PARC_GET_RELATIVE_TIME)\
    (GlobalSystemBlock.FirmwareVector[GetRelativeTime]))
#define ArcClose \
    ((PARC_CLOSE)\
    (GlobalSystemBlock.FirmwareVector[Close]))
#define ArcGetReadStatus \
    ((PARC_READ_STATUS)\
    (GlobalSystemBlock.FirmwareVector[ReadStatus]))
#define ArcMount \
    ((PARC_MOUNT)\
    (GlobalSystemBlock.FirmwareVector[Mount]))
#define ArcOpen \
    ((PARC_OPEN)\
    (GlobalSystemBlock.FirmwareVector[Open]))
#define ArcRead \
    ((PARC_READ)\
    (GlobalSystemBlock.FirmwareVector[Read]))
#define ArcSeek \
    ((PARC_SEEK)\
    (GlobalSystemBlock.FirmwareVector[Seek]))
#define ArcWrite \
    ((PARC_WRITE)\
    (GlobalSystemBlock.FirmwareVector[Write]))
#define ArcGetFileInformation \
    ((PARC_GET_FILE_INFO)(GlobalSystemBlock.FirmwareVector[GetFileInformation]))
#define ArcSetFileInformation \
    ((PARC_SET_FILE_INFO)\
    (GlobalSystemBlock.FirmwareVector[SetFileInformation]))
#define ArcGetDirectoryEntry \
    ((PARC_GET_DIRECTORY_ENTRY)\
    (GlobalSystemBlock.FirmwareVector[GetDirectoryEntry]))
#define ArcGetEnvironmentVariable \
    ((PARC_GET_ENVIRONMENT)\
    (GlobalSystemBlock.FirmwareVector[GetEnvironment]))
#define ArcSetEnvironmentVariable \
    ((PARC_SET_ENVIRONMENT)\
    (GlobalSystemBlock.FirmwareVector[SetEnvironment]))
#define ArcFlushAllCaches \
    ((PARC_FLUSH_ALL_CACHES)\
    (GlobalSystemBlock.FirmwareVector[FlushAllCaches]))
#define ArcTestUnicodeCharacter \
    ((PARC_TEST_UNICODE_CHARACTER) \
    (GlobalSystemBlock.FirmwareVector[TestUnicodeCharacter]))
#define ArcGetDisplayStatus \
    ((PARC_GET_DISPLAY_STATUS)\
    (GlobalSystemBlock.FirmwareVector[GetDisplayStatus]))

#endif
