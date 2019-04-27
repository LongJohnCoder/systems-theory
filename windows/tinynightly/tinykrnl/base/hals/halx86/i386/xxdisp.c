/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    display.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalDisplayString
 *
 * The HalDisplayString routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalDisplayString(IN PCHAR String)
{
    //
    // Call the bootvid function in the kernel
    //
    InbvDisplayString(String);
}

/*++
 * @name HalQueryDisplayParameters
 *
 * The HalQueryDisplayParameters routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalQueryDisplayParameters(PULONG WidthInCharacters,
                          PULONG HeightInLines,
                          PULONG CursorColumn,
                          PULONG CursorRow)
{
    //
    // Nothing to be implemented here
    //
}

/*++
 * @name HalSetDisplayParameters
 *
 * The HalSetDisplayParameters routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalSetDisplayParameters(ULONG CursorColumn,
                        ULONG CursorRow)
{
    //
    // Nothing to be implemented here
    //
}

/*++
 * @name HalAcquireDisplayOwnership
 *
 * The HalAcquireDisplayOwnership routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalAcquireDisplayOwnership(PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
    //
    // Nothing to be implemented here
    //
}
