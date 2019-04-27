@echo off
echo [boot loader] > %_NT386TREE%\boot.ini
echo timeout=10 >> %_NT386TREE%\boot.ini
echo default=multi(0)disk(0)rdisk(0)partition(1)\MININT >> %_NT386TREE%\boot.ini
echo [operating systems] >> %_NT386TREE%\boot.ini
echo multi(0)disk(0)rdisk(0)partition(1)\MININT="TinyKRNL" /noexecute=optout /fastdetect >> %_NT386TREE%\boot.ini
echo multi(0)disk(0)rdisk(0)partition(1)\MININT="TinyKRNL" /noexecute=optout /fastdetect /break /debug /SOS /debugport=com1 /baudrate=115200 >> %_NT386TREE%\boot.ini
echo multi(0)disk(0)rdisk(0)partition(1)\MININT="TinyKRNL [ACPI]" /noexecute=optout /fastdetect /break /hal=halacpi.dll /debug /SOS /debugport=com1 /baudrate=115200 >> %_NT386TREE%\boot.ini
