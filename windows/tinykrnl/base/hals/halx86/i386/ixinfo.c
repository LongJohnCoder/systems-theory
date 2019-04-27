/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    sysinfo.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu- Started Implementation - 26-Dec-06

--*/
#include "halp.h"

/*++
 * @name HalInitSystemPhase2
 *
 * The HalInitSystemPhase2 routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalInitSystemPhase2(VOID)
{
    //
    // Function does nothing
    //
}

/*++
 * @name HaliQuerySystemInformation
 *
 * The HaliQuerySystemInformation routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HaliQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN ULONG BufferSize,
                           OUT PVOID Buffer,
                           OUT PULONG ReturnedLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN UseCache;
    HAL_ERROR_INFO HalErrorInfo;
    PVOID DataBuffer;
    ULONG Length = 0;
    PAGED_CODE();

    //
    // Initialize return variables
    //
    *ReturnedLength = 0;

    //
    // Check which class this is
    //
    switch (InformationClass)
    {
        //
        // MCA, MCE, etc error information
        //
        case HalErrorInformation:

            //
            // Get MCE Error Information
            //
            DataBuffer = &HalErrorInfo;
            HalErrorInfo.Version = ((PHAL_ERROR_INFO)Buffer)->Version;
            Length = sizeof(HAL_ERROR_INFO);
            Status = HalpGetMceInformation(DataBuffer, &Length);
            break;

        //
        // Frame buffer caching information
        //
        case HalFrameBufferCachingInformation:

            //
            // Always enable frame buffer caching
            //
            UseCache = TRUE;
            DataBuffer = &UseCache;
            Length = sizeof(UseCache);
            break;

        //
        // Hot-pluggable memory support
        //
        case HalQueryMaxHotPlugMemoryAddress:

            //
            // Validate length
            //
            if (Length < sizeof(PHYSICAL_ADDRESS))
            {
                //
                // Invalid, fail
                //
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            //
            // Return hot-pluggable memory
            //
            *(PPHYSICAL_ADDRESS)Buffer = HalpMaxHotPlugMemoryAddress;
            *ReturnedLength = sizeof(PPHYSICAL_ADDRESS);
            break;

        //
        // Anything else
        //
        default:

            //
            // Invalid class
            //
            Status = STATUS_INVALID_LEVEL;
            NtUnhandled();
    }

    //
    // Check for length
    //
    if (Length)
    {
        //
        // Normalize
        //
        if (BufferSize < Length) Length = BufferSize;

        //
        // Return length and data
        //
        *ReturnedLength = Length;
        RtlCopyMemory(Buffer, DataBuffer, Length);
    }

    //
    // Return actual status
    //
    return Status;
}

/*++
 * @name HaliSetSystemInformation
 *
 * The HaliSetSystemInformation routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HaliSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN PVOID Buffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    //
    // Check the kind of class this is
    //
    switch (InformationClass)
    {
        //
        // MCE Handler
        //
        case HalKernelErrorHandler:

            //
            // Register it
            //
            Status = HalpMceRegisterKernelDriver(Buffer, BufferSize);
            break;

        //
        // MCA Handler
        //
        case HalMcaRegisterDriver:

            //
            // Register it
            //
            Status =  HalpMcaRegisterDriver(Buffer);
            break;

        //
        // Other classes
        //
        default:

            //
            // Invalid class
            //
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    //
    // Return status
    //
    return Status;
}

