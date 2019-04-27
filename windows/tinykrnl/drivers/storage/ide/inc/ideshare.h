/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    ideshare.h

Abstract:

    This file contains prototypes for shared code among the various IDE
    device drivers, such as PCIIDEX.

Environment:

    Kernel mode

Revision History:

    Evgeny Pinchuk - Feb-22-06 - Creater

--*/

VOID
IdeCreateIdeDirectory(
    VOID
);
