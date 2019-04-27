/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    sxsctxact.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - 

--*/
#include "precomp.h"

/*++
* @name RtlAllocateActivationContextStack
*
* The RtlAllocateActivationContextStack routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlAllocateActivationContextStack(IN PVOID *Context)
{
    //
    // FIXME: STUB
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlDeactivateActivationContextUnsafeFast
*
* The RtlDeactivateActivationContextUnsafeFast routine FILLMEIN
*
* @param Frame
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlDeactivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame)
{
    //
    // FIXME: STUB
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlActivateActivationContextUnsafeFast
*
* The RtlActivateActivationContextUnsafeFast routine FILLMEIN
*
* @param Frame
*        FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlActivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame,
                                       IN PVOID Context)
{
    //
    // FIXME: STUB
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlGetActiveActivationContext
*
* The RtlGetActiveActivationContext routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlGetActiveActivationContext(IN PVOID *Context)
{
    //
    // FIXME: STUB
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlReleaseActivationContext
*
* The RtlReleaseActivationContext routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlReleaseActivationContext(IN PVOID *Context)
{
    //
    // FIXME: STUB
    //
}



