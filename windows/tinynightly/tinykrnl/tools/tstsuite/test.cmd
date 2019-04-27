::
:: Test Suite Batch File.
:: If you make any changes, please increase the minor version number.
:: If your changes are major, or if there have already been multiple
:: minor updates, then increase the major version number.
::
@echo off
set TEST_SUITE_VERSION=1.0

::
:: Enable Delayed Variable Expansion so that we can set local variables
::
setlocal ENABLEDELAYEDEXPANSION

::
:: Cleanup local variables
::
set FILENAME=
set INSTALL_DONE=

::
:: Set our \bin folder in the path for the session
::
set PATH=%PATH%;%_NT_DRIVE%%_NTROOT%\tools\tstsuite\bin

::
:: Delete the previous log
::
echo. > tstsuite.log

::
:: Make sure 7za exists
::
IF NOT EXIST 7za.exe (
    ::
    : This is a fatal error. Notify the user and fail
    ::
    echo We're sorry, but 7zip was not found in the location where the Test Suite is
    echo located. Please make sure that the file "7za.exe" is located in this directory,
    echo and try the Test Suite again.
    goto FINISH
)

::
:: Check if the test image exists
::
IF NOT EXIST image\test.img (
    ::
    : It doesn't. Tell the user and extract it from our 7zip file.
    ::
    echo.
    echo Please wait while the Test Image is being setup for the first time...
    7za x image.7z >> tstsuite.log
    set INSTALL_DONE=yes
)

::
:: Check if the test tools exist
::
IF NOT EXIST bin\qemu.exe (
    ::
    : They don't, tell the user and extract them.
    ::
    echo.
    echo Please wait while the Test Tools are being setup for the first time...
    7za x tools.7z >> tstsuite.log
    set INSTALL_DONE=yes
)

::
:: Now check if KQEMU is installed
::
IF NOT EXIST %WINDIR%\system32\drivers\kqemu.sys (
    ::
    : It's not. Make sure we have the INF file
    ::
    IF NOT EXIST bin\kqemu.inf (
        ::
        : KQEMU doesn't seem to be present. Notify the user and fail.
        ::
        echo We're sorry, but KQEMU was not found in the location where the Test Suite is
        echo located. Please make sure that the file "kqemu.inf" is located in the "bin"
        echo directory and try the Test Suite again.
        goto FINISH
    )

    ::
    : Use our INF file and call SysSetup to install it
    ::
    echo.
    echo Please wait while KQEMU is being setup for the first time...
    rundll32 syssetup,SetupInfObjectInstallAction DefaultInstall 128 .\bin\kqemu.inf
    set INSTALL_DONE=yes
)

::
:: If installation was done, and display an on-screen pause to make
:: sure that the user can check everything went fine.
::
if "!INSTALL_DONE!" == "yes" (
    echo Installation Complete
    pause
    cls
)

::
:: Make sure the user typed a command
::
IF %1'==' (
    ::
    : He hasn't, so display usage information
    ::
    echo.
    echo Welcome to the TinyKRNL Test Suite. Version %TEST_SUITE_VERSION%.
    echo Usage: TEST [install / mount / current] [module name / path]
    echo.
    echo    * install - Installs the Test Suite; must be followed by the path
    echo                to your Windows 2003 Service Pack 1 Installation Files.
    echo    * mount   - Mounts the test image into drive R:
    echo    * unmount - UnMounts the test image.
    echo    * "name"  - Tests the module specified in "name" in the Test Suite. eg: pci
    echo    * current - Runs the Test Suite without testing any specific modules.
    echo                The Test Suite will run with whatever modules are installed
    echo                in the image. Useful for a complete system test or regression
    echo                testing.
    echo.
    echo Additionally, specifying "debug" as a second argument in any of the two testing
    echo commands above will result in the Microsoft Windows Debugger to be launched
    echo and connected to the Test Suite.
    goto FINISH
)

::
:: Start a new log entry
::
echo **** NEW BUILD %DATE% %TIME% **** >> tstsuite.log

::
:: Quick case: check if the user only wants to mount the image
::
if "%1" == "mount" (
    ::
    : Mount the image
    ::
    vdk open 0 image\test.img /L:R /RW < bin\yes.txt >> tstsuite.log
    echo.
    echo Test Image mounted successfully in drive R:
    goto FINISH
)

::
:: Quick case: check if the user only wants to unmount the image
::
if "%1" == "unmount" (
    ::
    : UnMount the image
    ::
    vdk close 0 >> tstsuite.log
    echo.
    echo Test Image unmounted successfully.
    goto FINISH
)

::
:: Start up KQEMU, and clear the screen
::
net start kqemu
cls

::
:: Mount the test image
::
echo Mounting Test Image...
vdk open 0 image\test.img /L:R /RW < bin\yes.txt >> tstsuite.log

::
:: Check if the image has been setup
::
IF NOT EXIST r:\minint\system32\ntoskrnl.exe (
    ::
    : It hasn't. Check if the user specified installation mode
    ::
    IF NOT "%1" == "install" (
        ::
        : He didn't. Clear the screen and tell the user that Windows 2003 SP1
        : is required.
        ::
        cls
        echo We're sorry, but we've detected that your TinyKRNL Image has not been setup.
        echo Please download Windows 2003 Service Pack 1 and run the Test Suite again with
        echo the "install" argument and the path to your SP1 files as a parameter eg:
        echo "%0 install c:\sp1files". Do not include the i386 directory name.
        echo.

       ::
       : Ask the user if he would like to download SP1
       ::
:DownloadNow
        set /P DOWNLOAD=Would you like to download SP1 now?
        IF "!DOWNLOAD!"=="yes" (
            ::
            : Display legal warning about downloading SP1 without a license.
            ::
            echo.
            echo LEGAL INFORMATION: Please check your country's laws; some countries require you
            echo to have a Windows 2003 license before proceeding with the download.
            set /P RULEGIT=Are you legally entitled to download Microsoft Windows 2003 Service Pack?

            ::
            : If the user said yes, then start a browser on Microsoft's site.
            ::
            IF "!RULEGIT!"=="yes" start http://www.microsoft.com/downloads/details.aspx?FamilyID=22CFC239-337C-4D81-8354-72593B1C1F43
       )

        ::
        : In either case, close the image now and quit
        ::
        vdk close 0 >> tstsuite.log
        goto FINISH
    )
)

::
:: Check if this is installation Mode
::
IF "%1" == "install" (
    ::
    : Make sure the user gave a path
    ::
    if %2'==' (
        ::
        : Complain and quit
        ::
        echo.
        echo We're sorry, but you seem not to have included the path to your Service Pack 1
        echo files. Please run the "install" command again, followed by the path to where
        echo your Service Pack 1 files are located. Do not include the i386 directory.

        ::
        : Let the user download it
        ::
    goto DownloadNow;
    )

    ::
    : Otherwise, validate the path
    ::
    IF NOT EXIST %2\i386\ntdll.dll (
        ::
        : Complain and quit
        ::
        echo.
        echo We're sorry, but we cannot find the necessary files in the Service Pack 1
        echo directory which you've specified: %2.
        echo.
        echo Are you sure this path is correct and that all the files where properly
        echo extracted? Also, make sure that this path does not include the i386 directory.
        echo Please run the "install" command again, with a proper path to the SP1 files.
        vdk close 0 >> tstsuite.log
        goto FINISH
    )

    ::
    : Create the drivers directory
    ::
    if NOT EXIST r:\minint\system32\drivers mkdir r:\minint\system32\drivers

    ::
    : Build a release
    ::
    pushd %CD%
    cd %_NT_DRIVE%%_NTROOT%
    %_NTBUILD% -cZ
    popd
    call %_NT_DRIVE%%_NTROOT%\tools\release.bat

    ::
    : Copy the files which we build ourselves
    ::
    copy /y %_NT386TREE%\system32\c_1252.nls r:\minint\system32 >> tstsuite.log
    copy /y %_NT386TREE%\system32\c_437.nls r:\minint\system32 >> tstsuite.log
    copy /y %_NT386TREE%\system32\l_intl.nls r:\minint\system32 >> tstsuite.log
    copy /y %_NT386TREE%\system32\native.exe r:\minint\system32\smss.exe >> tstsuite.log
    copy /y %_NT386TREE%\system32\kdcom.dll r:\minint\system32 >> tstsuite.log
    copy /y %_NT386TREE%\system32\bootvid.dll r:\minint\system32 >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\isapnp.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\wmilib.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\classpnp.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\intelide.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\partmgr.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\mountmgr.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\disk.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\system32\drivers\ftdisk.sys r:\minint\system32\drivers >> tstsuite.log
    copy /y %_NT386TREE%\boot.ini r:\ >> tstsuite.log

    ::
    : Copy the files which we need to take from the local XP installation
    ::
    copy /Y %WINDIR%\system32\drivers\kbdclass.sys r:\minint\system32\drivers >> tstsuite.log

    ::
    : Copy and expand the files which we take from the SP1 download
    ::
    copy /y %2\i386\ntldr r:\ntldr >> tstsuite.log
    copy /y %2\i386\ntdetect.com r:\ntdetect.com >> tstsuite.log
    copy /y %2\i386\ntdll.dll r:\minint\system32\ntdll.dll >> tstsuite.log
    expand %2\i386\hal.dl_ r:\minint\system32\hal.dll >> tstsuite.log
    expand %2\i386\halacpi.dl_ r:\minint\system32\halacpi.dll >> tstsuite.log
    expand %2\i386\ntoskrnl.ex_ r:\minint\system32\ntoskrnl.exe >> tstsuite.log
    expand %2\i386\acpi.sy_ r:\minint\system32\drivers\acpi.sys >> tstsuite.log
    expand %2\i386\atapi.sy_ r:\minint\system32\drivers\atapi.sys >> tstsuite.log
    expand %2\i386\fastfat.sy_ r:\minint\system32\drivers\fastfat.sys >> tstsuite.log
    expand %2\i386\i8042prt.sy_ r:\minint\system32\drivers\i8042prt.sys >> tstsuite.log
    expand %2\i386\pci.sy_ r:\minint\system32\drivers\pci.sys >> tstsuite.log
    expand %2\i386\pciidex.sy_ r:\minint\system32\drivers\pciidex.sys >> tstsuite.log

    ::
    : Notify the user that all went well, and quit
    ::
    echo.
    echo Congratulations! You have now properly setup your Test Suite. You may now enjoy
    echo its features and access it at any time by writing %0 in your Dazzle window.
    vdk close 0 >> tstsuite.log
    goto FINISH
)

::
:: We're either testing a module or the entire system
::
IF NOT "%1" == "current" (

    ::
    : We're testing a specific module
    ::
    IF EXIST %_NT386TREE%\system32\drivers\%1.sys (
        ::
        : Set the name
        ::
        set FILENAME=system32\drivers\%1.sys
    ) else (
        ::
        : It's not a driver; it could be a DLL or an EXE.
        ::
        IF EXIST %_NT386TREE%\system32\%1.dll (
            ::
            : It's a DLL, set the name
            ::
            set FILENAME=system32\%1.dll
            pause
        ) else IF EXIST %_NT386TREE%\system32\%1.exe (
            ::
            : It's an EXE, set the name
            ::
            set FILENAME=system32\%1.exe
        ) else (
            ::
            : Invalid file... don't let the user go on
            ::
            echo.
            echo We're sorry, but we were unable to locate the module you specified. Please make
            echo sure that it is a valid DLL, EXE or SYS file in one of these two directories:
            echo.
            echo * %_NT386TREE%\system32
            echo * %_NT386TREE%\system32\drivers
            vdk close 0 >> tstsuite.log
            goto FINISH
        )
    )

    ::
    : Notify user and copy
    ::
    echo Copying specified test module [!FILENAME!]
    copy r:\minint\!FILENAME! %_NT386TREE%\!FILENAME!.ms >> tstsuite.log
    copy /Y %_NT386TREE%\!FILENAME! r:\minint\!FILENAME! >> tstsuite.log
)

::
:: Tell the user we're now ready to run the emulator, and close the image
::
echo Running Emulation...
vdk close 0 >> tstsuite.log

::
:: Check if the debug parameter was passed
::
IF "%2" == "debug" (
    ::
    : Check if WinDbg is installed
    ::
    IF NOT EXIST C:\progra~1\Debugg~1\windbg.exe (
        ::
        : WinDbg doesn't seem to be present. Notify the user and fail.
        ::
        echo We're sorry, but the Microsoft Debugging Tools for Windows were not found on your
        echo system. Please make sure that they are installed, or obtain them from Microsoft's
        echo Developer Site or try the Test Suite again, without the "debug" argument.
        goto :Finish
    )

    ::
    : Start it
    ::
    start C:\progra~1\Debugg~1\windbg.exe -b -k com:pipe,port=\\.\pipe\com_1,resets=0,reconnect

    ::
    : Start KQEMU with Serial Port support
    ::
    qemu -kernel-kqemu -L .\bin .\image\test.img -serial pipe:com_1 -m 32
) else (
    ::
    : Start KQEMU
    ::
    qemu -kernel-kqemu -L .\bin .\image\test.img -m 32
)

::
:: We've returned. Stop KQEMU and clear the screen
::
net stop kqemu
cls

::
:: Check if the user gave a specific module
::
IF NOT "%1" == "current" (
    ::
    : He did. Remount the test image
    ::
    echo Remounting Test Image...
    vdk open 0 image\test.img /L:R /RW < bin\yes.txt >> tstsuite.log

    ::
    : Restore the module that was being tested and close the image
    ::
    echo Restoring specified test module [!FILENAME!.ms]
    move /Y %_NT386TREE%\!FILENAME!.ms r:\minint\!FILENAME! >> tstsuite.log
    vdk close 0 >> tstsuite.log
)

::
:: Notify the user that it is finished
::
echo Test Suite complete

:FINISH

::
:: We're done. Terminate the log.
::

echo. >> tstsuite.log
echo. >> tstsuite.log
endlocal
