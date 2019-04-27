/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ixmca.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 26-Dec-06

--*/
#include "halp.h"

ULONG HalpCMCEnabled, HalpNoMCABugCheck;
BOOLEAN HalpMcaInterfaceLocked;
MCA_INFO HalpMcaInfo;

VOID
HalpMcaGetConfiguration(OUT PULONG MceActive,
                        OUT PULONG McaActive,
                        OUT PULONG CMCEnabled,
                        OUT PULONG McaBugCheck)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[5];
    ULONG DefaultMce, DefaultMca, DefaultCmc, DefaultBugCheck;

    //
    // Clear query table
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));

    //
    // Setup defaults
    //
    DefaultMce = *MceActive = FALSE;
    DefaultMca = *McaActive = TRUE;
    DefaultCmc = *CMCEnabled = TRUE;
    DefaultBugCheck = *McaBugCheck = FALSE;

    //
    // Setup query tables
    //
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"EnableMCE";
    QueryTable[0].EntryContext = MceActive;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DefaultMce;
    QueryTable[0].DefaultLength = sizeof(ULONG);
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].Name = L"EnableMCA";
    QueryTable[1].EntryContext =  McaActive;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = &DefaultMca;
    QueryTable[1].DefaultLength = sizeof(ULONG);
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[2].Name = L"EnableCMC";
    QueryTable[2].EntryContext = MceActive;
    QueryTable[2].DefaultType = REG_DWORD;
    QueryTable[2].DefaultData = &DefaultCmc;
    QueryTable[2].DefaultLength = sizeof(ULONG);
    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[3].Name = L"NoMCABugCheck";
    QueryTable[3].EntryContext =  McaBugCheck;
    QueryTable[3].DefaultType = REG_DWORD;
    QueryTable[3].DefaultData = &DefaultBugCheck;
    QueryTable[3].DefaultLength = sizeof(ULONG);

    //
    // Query registry settings
    //
    RtlQueryRegistryValues(RTL_REGISTRY_CONTROL | RTL_REGISTRY_OPTIONAL,
                           L"Session Manager",
                           QueryTable,
                           NULL,
                           NULL);
}

VOID
HalpMcaInit(VOID)
{
    ULONGLONG MsrBanks;
    ULONG MceActive, McaActive;
    KIRQL OldIrql;
    KAFFINITY ActiveProcessors, CurrentAffinity;
    PKTSS Tss;

    //
    // Initialize the MCA Mutex
    //
    ExInitializeFastMutex(&HalpMcaInfo.Mutex);

    //
    // Get MCA Configuration
    //
    HalpMcaGetConfiguration(&MceActive,
                            &McaActive,
                            &HalpCMCEnabled,
                            &HalpNoMCABugCheck);

    //
    // Check if MCE is forced because of the registry
    //
    if ((HalpFeatureBits & HF_MCE) && !(HalpFeatureBits & HF_MCA))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we have MCA support
    //
    if (HalpFeatureBits & HF_MCA)
    {
        //
        // Check if registry disabled it
        //
        if (McaActive == FALSE)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Count the number of banks
        //
        MsrBanks = RDMSR(0x179);
        HalpMcaInfo.NumBanks = (UCHAR)(MsrBanks & 0xFF);

        //
        // Make sure we have banks at all
        //
        if (!HalpMcaInfo.NumBanks)
        {
            //
            // We don't, disable MCA/MCE support
            //
            HalpFeatureBits &= ~(HF_MCE | HF_MCA);
        }
        else
        {
            //
            // Get the bank configuration
            //
            HalpMcaInfo.Bank0Config = RDMSR(0x400);
        }
    }

    //
    // Check if either MCA or MCE are still enabled
    //
    if ((HalpFeatureBits & HF_MCA) || (HalpFeatureBits & HF_MCE))
    {
        //
        // FIXME: Untested code path
        //
        NtUnhandled();

        //
        // Can't have MCA without MCE
        //
        ASSERT(HalpFeatureBits & HF_MCE);

        //
        // Loop active CPUs
        //
        ActiveProcessors = HalpActiveProcessors;
        for (CurrentAffinity = 1; ActiveProcessors; CurrentAffinity <<= 1)
        {
            //
            // Check if the CPU is part of the affinity
            //
            if (ActiveProcessors & CurrentAffinity)
            {
                //
                // Mask out the active processors and set the system affinity
                //
                ActiveProcessors &= ~CurrentAffinity;
                KeSetSystemAffinityThread(CurrentAffinity);

                //
                // Allocate a new TSS
                //
                Tss = ExAllocatePoolWithTag(NonPagedPool, 0x68, ' laH');

                //
                // Raise IRQL to HIGH
                //
                OldIrql = KfRaiseIrql(HIGH_LEVEL);

                //
                // Setup MCA for the CPU
                //
                //HalpMcaCurrentProcessorSetTSS(Tss);
                //HalpMcaCurrentProcessorSetConfig();

                //
                // Lower IRQL back
                //
                KfLowerIrql(OldIrql);
            }
        }

        //
        // Restore affinity
        //
        KeRevertToUserAffinityThread();
    }
}

VOID
HalpMcaLockInterface(VOID)
{
    //
    // Acquire lock
    //
    ExAcquireFastMutex(&HalpMcaInfo.Mutex);
    ASSERT(HalpMcaInterfaceLocked);
    HalpMcaInterfaceLocked = TRUE;
}

VOID
HalpMcaUnlockInterface(VOID)
{
    //
    // Acquire lock
    //
    ExReleaseFastMutex(&HalpMcaInfo.Mutex);
    ASSERT(HalpMcaInterfaceLocked == TRUE);
    HalpMcaInterfaceLocked = FALSE;
}

NTSTATUS
HalpMcaRegisterDriver(IN PMCA_DRIVER_INFO DriverInfo)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PAGED_CODE();

    //
    // Make sure MCE is supported
    //
    if (HalpFeatureBits & HF_MCE)
    {
        //
        // Lock interface
        //
        HalpMcaLockInterface();

        //
        // Make sure nobody registered us yet
        //
        if (!HalpMcaInfo.DriverInfo.DpcCallback)
        {
            //
            // Register data and set success
            //
            HalpMcaInfo.DriverInfo.ExceptionCallback = DriverInfo->
                                                       ExceptionCallback;
            HalpMcaInfo.DriverInfo.DeviceContext = DriverInfo->DeviceContext;
            Status = STATUS_SUCCESS;
        }

        //
        // Unlock interface
        //
        HalpMcaUnlockInterface();
    }

    //
    // Return status
    //
    return Status;
}

NTSTATUS
HalpMceRegisterKernelDriver(IN PKERNEL_ERROR_HANDLER_INFO ErrorInfo,
                            IN ULONG InfoSize)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    //
    // Validate parameters
    //
    if (!ErrorInfo) return STATUS_INVALID_PARAMETER;
    if (ErrorInfo->Version > HAL_ERROR_HANDLER_VERSION)
    {
        //
        // Revision failure
        //
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Lock the interface
    //
    HalpMcaLockInterface();
    if (!HalpMcaInfo.WmiMcaCallback)
    {
        //
        // No callback registered, register it now
        //
        HalpMcaInfo.WmiMcaCallback = ErrorInfo->KernelMcaDelivery;
        Status = STATUS_SUCCESS;
    }

    //
    // Unlock interface and return
    //
    HalpMcaUnlockInterface();
    return Status;
}

NTSTATUS
HalpGetMceInformation(IN OUT PHAL_ERROR_INFO ErrorInfo,
                      OUT PULONG Length)
{
    ULONG Version;
    PAGED_CODE();

    //
    // Sanity checks
    //
    ASSERT(ErrorInfo != NULL);
    ASSERT(*Length == sizeof(HAL_ERROR_INFO));

    //
    // Validate version
    //
    if ((ErrorInfo->Version > HAL_ERROR_INFO_VERSION) || !(ErrorInfo->Version))
    {
        //
        // Invalid revision, fail
        //
        return STATUS_REVISION_MISMATCH;
    }
    else
    {
        //
        // Sanity check
        //
        ASSERT(ErrorInfo->Version == HAL_ERROR_INFO_VERSION);
    }

    //
    // Clear it
    //
    Version = ErrorInfo->Version;
    RtlZeroMemory(ErrorInfo, sizeof(HAL_ERROR_INFO));

    //
    // Set it up
    //
    ErrorInfo->McaMaxSize = 256;
    ErrorInfo->CmcMaxSize = 256;
    ErrorInfo->Version = Version;
    ErrorInfo->McaPreviousEventsCount = 1;
    ErrorInfo->CmcPollingInterval = ((HalpFeatureBits & HF_MCE) &&
                                     (HalpCMCEnabled)) ? 1: -1;
    ErrorInfo->CpePollingInterval = 0;
    ErrorInfo->KernelReserved[0] = 0x59364117;
    ErrorInfo->KernelReserved[2] = 0x59364117;

    //
    // Return length and success
    //
    *Length = sizeof(HAL_ERROR_INFO);
    return STATUS_SUCCESS;
}
