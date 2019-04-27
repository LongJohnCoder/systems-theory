/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    sertl.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Apr-06

--*/
#include "precomp.h"

/*++
* @name RtlpOpenThreadToken
*
* The RtlpOpenThreadToken routine FILLMEIN
*
* @param DesiredAccess
*        FILLMEIN
*
* @param ThreadToken
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlpOpenThreadToken(IN ACCESS_MASK DesiredAccess,
                    IN PHANDLE ThreadToken)
{
    NTSTATUS Status;

    //
    // Try open it as ourselves first
    //
    Status = NtOpenThreadToken(NtCurrentThread(),
                               DesiredAccess,
                               TRUE,
                               ThreadToken);
    if (!NT_SUCCESS(Status))
    {
        //
        // Well that didn't work... try again
        //
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   DesiredAccess,
                                   FALSE,
                                   ThreadToken);
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name RtlAcquirePrivilege
*
* The RtlAcquirePrivilege routine FILLMEIN
*
* @param Privilege
*        FILLMEIN
*
* @param NumPriv
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param ReturnedState
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlAcquirePrivilege(IN PULONG Privilege,
                    IN ULONG NumPriv,
                    IN ULONG Flags,
                    OUT PVOID *ReturnedState)
{
    ULONG BytesNeeded;
    PRTL_ACQUIRE_STATE AcquireState;
    HANDLE NewToken;
    PTOKEN_PRIVILEGES Privileges;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status, Status1;
    HANDLE Token, NullToken;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    ULONG PrivilegeSetSize;
    ULONG i;

    //
    // Validate the flags
    //
    if (Flags &~ (RTL_ACQUIRE_PRIVILEGE_PROCESS |
                  RTL_ACQUIRE_PRIVILEGE_IMPERSONATE))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if acquiring privileges for the process as well
    //
    if (Flags & RTL_ACQUIRE_PRIVILEGE_PROCESS)
    {
        //
        // This implies using the impersonation token
        //
        Flags |= RTL_ACQUIRE_PRIVILEGE_IMPERSONATE;
    }

    //
    // Allocate memory for the state
    //
    BytesNeeded = sizeof(RTL_ACQUIRE_STATE) + sizeof(TOKEN_PRIVILEGES) +
                  ((NumPriv  -1)* sizeof(LUID_AND_ATTRIBUTES));
    AcquireState = RtlAllocateHeap(RtlGetProcessHeap(), 0, BytesNeeded);
    if (!AcquireState) return STATUS_NO_MEMORY;

    //
    // Clear it
    //
    AcquireState->Token = NULL;
    AcquireState->OldImpersonationToken = NULL;
    AcquireState->Flags = 0;

    //
    // Check if the thread is impersonating
    //
    if (NtCurrentTeb()->IsImpersonating)
    {
        //
        // Check if the flags specify to use the impersonation token
        //
        if (Flags & RTL_ACQUIRE_PRIVILEGE_IMPERSONATE)
        {
            //
            // Open it and remember that we used the impersonation token
            //
            Status = RtlpOpenThreadToken(TOKEN_IMPERSONATE,
                                         &AcquireState->OldImpersonationToken);
            if (!NT_SUCCESS(Status)) goto Quickie;
            AcquireState->Flags |= RTL_ACQUIRE_PRIVILEGE_IMPERSONATE;

            //
            // Clear the current thread's token
            //
            NewToken =  NULL;
            Status1 = ZwSetInformationThread(NtCurrentThread(),
                                             ThreadImpersonationToken,
                                             &NewToken,
                                             sizeof(NewToken));
            ASSERT(NT_SUCCESS(Status1));
        }
        else
        {
            //
            // Open the token without impersonation privileges
            //
            Status = RtlpOpenThreadToken(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                         &AcquireState->Token);
            if (!NT_SUCCESS(Status)) goto Quickie;
        }
    }

    //
    // Check if we have a token
    //
    if (!AcquireState->Token)
    {
        //
        // We don't; Check if we should use the thread's
        //
        if (!(Flags & RTL_ACQUIRE_PRIVILEGE_PROCESS))
        {
            //
            // Open the process's token
            //
            Status = ZwOpenProcessToken(NtCurrentProcess(),
                                        TOKEN_DUPLICATE,
                                        &Token);
            if (NT_SUCCESS(Status))
            {
                //
                // Setup the object attributes and QoS for duplication
                //
                InitializeObjectAttributes(&ObjectAttributes,
                                           NULL,
                                           0,
                                           NULL,
                                           NULL);
                ObjectAttributes.SecurityQualityOfService = &SecurityQos;
                SecurityQos.Length = sizeof(SecurityQos);
                SecurityQos.ImpersonationLevel = SecurityDelegation;
                SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
                SecurityQos.EffectiveOnly = FALSE;

                //
                // Duplicate the token
                //
                Status = NtDuplicateToken(Token,
                                          TOKEN_ADJUST_PRIVILEGES |
                                          TOKEN_QUERY |
                                          TOKEN_DUPLICATE,
                                          &ObjectAttributes,
                                          FALSE,
                                          TokenImpersonation,
                                          &NullToken);
                if (NT_SUCCESS(Status))
                {
                    //
                    // Set it as the new thread token
                    //
                    Status = ZwSetInformationToken(NtCurrentThread(),
                                                   ThreadImpersonationToken,
                                                   &NullToken,
                                                   sizeof(NullToken));
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Save the current token
                        //
                        AcquireState->Token = NullToken;
                    }
                    else
                    {
                        //
                        // Close the handle
                        //
                        Status1 = NtClose(NullToken);
                        ASSERT(NT_SUCCESS(Status1));
                    }
                }

                //
                // Close the handle
                //
                Status1 = NtClose(Token);
                ASSERT(NT_SUCCESS(Status1));
            }

            //
            // Check if success got us here
            //
            if (!NT_SUCCESS(Status)) goto CleanupNoToken;

            //
            // Write the impersonation token flag
            //
            AcquireState->Flags |= RTL_ACQUIRE_PRIVILEGE_IMPERSONATE;
        }
        else
        {
            //
            // Open the process token
            //
            Status = ZwOpenProcessToken(NtCurrentProcess(),
                                        TOKEN_ADJUST_PRIVILEGES |
                                        TOKEN_QUERY |
                                        TOKEN_DUPLICATE,
                                        &AcquireState->Token);
            if (!NT_SUCCESS(Status)) goto CleanupNoToken;
        }

        //
        // Make sure we have a token now
        //
        ASSERT(AcquireState->Token);
    }

    //
    // Fill out the old privilege pointers
    //
    AcquireState->OldPrivileges = (PTOKEN_PRIVILEGES)AcquireState->OldPrivBuffer;
    AcquireState->NewPrivileges = (PTOKEN_PRIVILEGES)(AcquireState + 1);
    Privileges = AcquireState->NewPrivileges;
    Privileges->PrivilegeCount = NumPriv;

    //
    // Loop the privileges to change
    //
    for (i = 0; i < NumPriv; i++)
    {
        //
        // Setup this privilege structure
        //
        Privileges->Privileges[i].Luid = RtlConvertUlongToLuid(Privilege[i]);
        Privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    //
    // Set the privileges
    //
    PrivilegeSetSize = sizeof(AcquireState->OldPrivBuffer);
    Status = ZwAdjustPrivilegesToken(AcquireState->Token,
                                     FALSE,
                                     AcquireState->NewPrivileges,
                                     PrivilegeSetSize,
                                     AcquireState->OldPrivileges,
                                     &PrivilegeSetSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        //
        // Allocate heap
        //
RetryPrivAllocation:
        AcquireState->OldPrivileges = RtlAllocateHeap(RtlGetProcessHeap(),
                                                      0,
                                                      PrivilegeSetSize);

        //
        // Try to adjust now
        //
        Status = ZwAdjustPrivilegesToken(AcquireState->Token,
                                         FALSE,
                                         AcquireState->NewPrivileges,
                                         sizeof(AcquireState->OldPrivBuffer),
                                         AcquireState->OldPrivileges,
                                         &PrivilegeSetSize);
        if (NT_SUCCESS(Status))
        {
            //
            // This should've only happened for a process
            //
            ASSERT(Flags & RTL_ACQUIRE_PRIVILEGE_PROCESS);

            //
            // Free the smaller allocation and try a bigger one
            //
            RtlFreeHeap(RtlGetProcessHeap(), 0, AcquireState->OldPrivileges);
            goto RetryPrivAllocation;
        }
    }

    //
    // Check if we failed, unless we only partially succeeded
    //
    if (!(NT_SUCCESS(Status)) &&
        ((Status == STATUS_NOT_ALL_ASSIGNED) && (NumPriv == 1)))
    {
        //
        // If we only partially succeeded, was there only one privilege anyway?
        //
        if ((Status == STATUS_NOT_ALL_ASSIGNED) && (NumPriv == 1))
        {
            //
            // This means all failed
            //
            Status = STATUS_PRIVILEGE_NOT_HELD;
        }

        //
        // Check if we had old privileges that weren't static in our buffer
        //
        if ((AcquireState->OldPrivileges) &&
            (AcquireState->OldPrivileges !=
             (PTOKEN_PRIVILEGES)AcquireState->OldPrivBuffer))
        {
            //
            // Free those privileges
            //
            RtlFreeHeap(RtlGetProcessHeap(), 0, AcquireState->OldPrivileges);
        }

        //
        // Make sure that we had a token handle and close it
        //
        ASSERT(AcquireState->Token);
        Status1 = NtClose(AcquireState->Token);
        ASSERT(NT_SUCCESS(Status1));

        //
        // Check if we used an impersonation token
        //
CleanupNoToken:
        if (AcquireState->Flags & RTL_ACQUIRE_PRIVILEGE_IMPERSONATE)
        {
            //
            // Reset to our old one
            //
            Status1 = ZwSetInformationThread(NtCurrentThread(),
                                             ThreadImpersonationToken,
                                             &AcquireState->
                                             OldImpersonationToken,
                                             sizeof(HANDLE));
            if (!NT_SUCCESS(Status1)) RtlRaiseStatus(Status1);
        }

        //
        // Check if we had an old impersonation token handle
        //
        if (AcquireState->OldImpersonationToken)
        {
            //
            // Close it
            //
            Status1 = NtClose(AcquireState->OldImpersonationToken);
            ASSERT(NT_SUCCESS(Status1));
        }

Quickie:
        //
        // Free the context
        //
        RtlFreeHeap(RtlGetProcessHeap(), 0, AcquireState);
        return Status;
    }

    //
    // Return the state and return success
    //
    *ReturnedState = AcquireState;
    return STATUS_SUCCESS;
}

/*++
* @name RtlReleasePrivilege
*
* The RtlReleasePrivilege routine FILLMEIN
*
* @param ReturnedState
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlReleasePrivilege(IN PVOID ReturnedState)
{
    PRTL_ACQUIRE_STATE AcquireState = ReturnedState;
    NTSTATUS Status;

    //
    // Check if we had an impersonation token
    //
    if (!(AcquireState->Flags & RTL_ACQUIRE_PRIVILEGE_IMPERSONATE))
    {
        //
        // We didn't; simply restore the privileges
        //
        ZwAdjustPrivilegesToken(AcquireState->Token,
                                FALSE,
                                AcquireState->OldPrivileges,
                                0,
                                NULL,
                                0);
    }
    else
    {
        //
        // We did; restore it
        //
        Status = ZwSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        &AcquireState->OldImpersonationToken,
                                        sizeof(HANDLE));
        if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
    }

    //
    // Check if we still have an impersonation token handle
    //
    if (AcquireState->OldImpersonationToken)
    {
        //
        // Close it
        //
        Status = NtClose(AcquireState->OldImpersonationToken);
        ASSERT(NT_SUCCESS(Status));
    }

    //
    // Check if we had old privileges that weren't static in our buffer
    //
    if ((AcquireState->OldPrivileges) &&
        (AcquireState->OldPrivileges !=
         (PTOKEN_PRIVILEGES)AcquireState->OldPrivBuffer))
    {
        //
        // Free those privileges
        //
        RtlFreeHeap(RtlGetProcessHeap(), 0, AcquireState->OldPrivileges);
    }

    //
    // Close the token handle
    //
    Status = NtClose(AcquireState->Token);
    ASSERT(NT_SUCCESS(Status));

    //
    // Free the entire state structure
    //
    RtlFreeHeap(RtlGetProcessHeap(), 0, AcquireState);
}

