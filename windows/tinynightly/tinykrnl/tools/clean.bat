@echo off
echo Cleaning Tree...

::
:: Force a clean (for when the check below fails).
::
IF "%1" == "force" (
    del /S %_NT_DRIVE%%_NTROOT%\base\*.obj 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.mac 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.res 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.sys 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.lib 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.pch 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.pdb 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.obj 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.mac 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.res 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.sys 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.pch 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.pdb 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.lib 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\public\ddk\lib\*.lib 1> NUL 2> NUL
    del /S %_NT_DRIVE%%_NTROOT%\public\ddk\lib\*.exp 1> NUL 2> NUL
    rd /S /q %_NT386TREE%
    echo Clean complete!
    goto :EOF
)

::
:: Check if there is anything to clean, if so, clean it.
::
IF EXIST %_NT_DRIVE%%_NTROOT%\public\ddk\lib\i386\*.lib (
    del /S %_NT_DRIVE%%_NTROOT%\base\*.obj > NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.mac > NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.res > NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.sys > NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.lib > NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.pch > NUL
    del /S %_NT_DRIVE%%_NTROOT%\base\*.pdb > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.obj > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.mac > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.res > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.sys > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.pch > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.pdb > NUL
    del /S %_NT_DRIVE%%_NTROOT%\drivers\*.lib > NUL
    del /S %_NT_DRIVE%%_NTROOT%\public\ddk\lib\*.lib > NUL
    del /S %_NT_DRIVE%%_NTROOT%\public\ddk\lib\*.exp > NUL
    rd /S /q %_NT386TREE%
    echo Clean complete!
) else (
    echo Nothing to clean!
)
