/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    envvar.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalGetEnvironmentVariable
 *
 * The HalGetEnvironmentVariable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
HalGetEnvironmentVariable(IN PCH Variable,
                          IN USHORT Length,
                          OUT PCH Buffer)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
 * @name HalSetEnvironmentVariable
 *
 * The HalSetEnvironmentVariable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
HalSetEnvironmentVariable(IN PCH Name,
                          IN PCH Value)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

