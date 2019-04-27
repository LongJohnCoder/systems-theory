!IF 0

Copyright (C) TinyKRNL, 2006

Module Name:

    makefile.inc

!ENDIF

ASM = masm
AFLAGS = -Mx
AINC = -I$(SDK_INC_PATH)

CC = cl16
CFLAGS = -W3 -G2s
CINC = -I$(SDK_INC_PATH) -I$(DDK_INC_PATH) -I$(CRT_INC_PATH) -I.\..\..\..\public\internal\base\inc -I.\..\inc

LINK = link16
LIBS = $(LIB)\lib16\clibc7.lib

.asm{obj$(BUILD_ALT_DIR)\i386\}.obj:
    $(ASM) $(AFLAGS) $(AINC) $< $@;

.c{obj$(BUILD_ALT_DIR)\i386\}.obj:
    $(CC) $(CFLAGS) $(CINC) -Fo$@ -c $<

OBJ=\
    obj$(BUILD_ALT_DIR)\i386\tbx86sys.obj   \
    obj$(BUILD_ALT_DIR)\i386\hyprgate.obj   \
    obj$(BUILD_ALT_DIR)\i386\init.obj

tbx86.com: $(OBJ)
    $(LINK) @<<
/tiny /nod /noi +
$(OBJ)
obj$(BUILD_ALT_DIR)\i386\tbx86.com

$(LIBS)

<<
