/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    overlap.cxx

Abstract:

    The Fault Tolerance Driver provides fault tolerance for disk by using
    disk mirroring and striping. Additionally, it creates disk device objects
    that represent volumes on Basic disks. For each volume, FtDisk creates a
    symbolic link of the form \Device\HarddiskVolumeX, identifying the volume.

Environment:

    Kernel mode

Revision History:

    - Started Implementation - 

--*/
#include "precomp.hxx"