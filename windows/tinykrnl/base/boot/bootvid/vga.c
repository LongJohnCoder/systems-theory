/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    vga.c

Abstract:

    The VGA Boot Driver is responsible for text-mode VGA output, such as the
    boot logo, the blue screen, and a variety of other operations that it
    supports, such as filling the screen, BitBLTing, displaying text and
    drawing simple shapes. It is accessible through the Inbv interface present
    in ntoskrnl.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 14-Feb-2006

--*/
#include "precomp.h"

//
// Scroll Region
//
ULONG ScrollRegion[4] =
{
    0,      // Minimum X
    0,      // Minimum Y
    639,    // Maximum X
    479     // Maximum Y
};

//
// Default Text Color
//
ULONG TextColor = 0xF;

//
// Left/Right Mask Tables
//
UCHAR lMaskTable[8] =
{
    (1 << 8) - (1 << 0),
    (1 << 7) - (1 << 0),
    (1 << 6) - (1 << 0),
    (1 << 5) - (1 << 0),
    (1 << 4) - (1 << 0),
    (1 << 3) - (1 << 0),
    (1 << 2) - (1 << 0),
    (1 << 1) - (1 << 0)
};
UCHAR rMaskTable[8] =
{
    (1 << 7),
    (1 << 7)+ (1 << 6),
    (1 << 7)+ (1 << 6) + (1 << 5),
    (1 << 7)+ (1 << 6) + (1 << 5) + (1 << 4),
    (1 << 7)+ (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3),
    (1 << 7)+ (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2),
    (1 << 7)+ (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1),
    (1 << 7)+ (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1) +
    (1 << 0),
};

//
// Pixel Mask Table
//
UCHAR PixelMask[8] =
{
    (1 << 7),
    (1 << 6),
    (1 << 5),
    (1 << 4),
    (1 << 3),
    (1 << 2),
    (1 << 1),
    (1 << 0),
};

//
// Bit lookup Table
//
ULONG lookup[16] =
{
    0x0000,
    0x0100,
    0x1000,
    0x1100,
    0x0001,
    0x0101,
    0x1001,
    0x1101,
    0x0010,
    0x0110,
    0x1010,
    0x1110,
    0x0011,
    0x0111,
    0x1011,
    0x1111,
};

//
// Screen position
//
ULONG curr_x = 0;
ULONG curr_y = 0;
BOOLEAN NextLine = FALSE;

//
// VGA Registers
//
ULONG_PTR VgaRegisterBase = 0;
ULONG_PTR VgaBase = 0;

/*++
 * @name VidSetTextColor
 *
 * The VidSetTextColor routine sets the foreground text color that will be used
 * for VidDisplayString calls.
 *
 * @param Color
 *        4-bit VGA Color
 *
 * @return The previous text color.
 *
 * @remarks VidDisplayStringXY ignores this value and always uses color 12.
 *
 *--*/
ULONG
NTAPI
VidSetTextColor(ULONG Color)
{
    ULONG OldColor;

    //
    // Save the old color and set the new one
    //
    OldColor = TextColor;
    TextColor = Color;
    return OldColor;
}

/*++
 * @name ReadWriteMode
 *
 * The ReadWriteMode routine selects the VGA Graphics Mode to be used.
 *
 * @param Mode
 *        VGA Graphic Mode.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
ReadWriteMode(UCHAR Mode)
{
    UCHAR Value;

    //
    // Switch to graphics mode register
    //
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 5);

    //
    // Get the current register value, minus the current mode
    //
    Value = READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF) & 0xF4;

    //
    // Set the new mode
    //
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, Mode | Value);
}

/*++
 * @name __outpb
 *
 * The __outpb routine writes a byte to a VGA Register.
 *
 * @param Port
 *        VGA Register address.
 *
 * @param Value
 *        Byte value to write.
 *
 * @return None.
 *
 * @remarks The VGA Register address is incremented with the pointer where the
 *          I/O space is actually mapped, if any. Do not send the added value
 *          manually.
 *
 *--*/
VOID
NTAPI
__outpb(IN ULONG Port,
        IN ULONG Value)
{
    //
    // Write to the VGA Register
    //
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + Port, (UCHAR)Value);
}

/*++
 * @name __outpw
 *
 * The __outpw routine writes a word to a VGA Register.
 *
 * @param Port
 *        VGA Register address.
 *
 * @param Value
 *        Word value to write.
 *
 * @return None.
 *
 * @remarks The VGA Register address is incremented with the pointer where the
 *          I/O space is actually mapped, if any. Do not send the added value
 *          manually.
 *
 *--*/
VOID
NTAPI
__outpw(IN ULONG Port,
        IN ULONG Value)
{
    //
    // Write to the VGA Register
    //
    WRITE_PORT_USHORT((PUSHORT)(VgaRegisterBase + Port), (USHORT)Value);
}

/*++
 * @name SetPixel
 *
 * The SetPixel routine sets the color of a pixel at the given coordinates.
 *
 * @param Left
 *        X Ordinate of the pixel.
 *
 * @param Top
 *        Y Ordinate of the pixe.
 *
 * @param Color
 *        Color to set the pixel to.
 *
 * @return None.
 *
 * @remarks The operation is done in Mode 10 unchained.
 *
 *--*/
VOID
NTAPI
SetPixel(IN ULONG Left,
         IN ULONG Top,
         IN UCHAR Color)
{
    PUCHAR PixelPosition;

    //
    // Calculate the pixel position.
    //
    PixelPosition = (PUCHAR)VgaBase + (Left >> 3) + (Top * 80);

    //
    // Switch to mode 10
    //
    ReadWriteMode(10);

    //
    // Clear the 4 planes (we're already in unchained mode here)
    //
    __outpw(0x3C4, 0xF02);

    //
    // Select the color don't care register
    //
    __outpw(0x3CE, 7);

    //
    // Select the bitmask register and write the mask
    //
    __outpw(0x3CE, (PixelMask[Left & 7] << 8) | 8);

    //
    // Read the current pixel value and add our color
    //
    WRITE_REGISTER_UCHAR(PixelPosition,
                         READ_REGISTER_UCHAR(PixelPosition) & Color);
}

/*++
 * @name DisplayCharacter
 *
 * The DisplayCharacter routine draws a character on the screen.
 *
 * @param Character
 *        ASCII value of the character to be drawn.
 *
 * @param Left
 *        X ordinate where the character will be drawn.
 *
 * @param Top
 *        Y ordinate where the character will be drawn.
 *
 * @param TextColor
 *        Foreground color of the character.
 *
 * @param BackTextColor
 *        Background color of the character.
 *
 * @return None.
 *
 * @remarks A value of 16 for BackTextColor implies a transparent color.
 *
 *--*/
VOID
NTAPI
DisplayCharacter(CHAR Character,
                 ULONG Left,
                 ULONG Top,
                 ULONG TextColor,
                 ULONG BackTextColor)
{
    PUCHAR FontChar;
    ULONG i, j, XOffset;

    //
    // Get the font line for this character
    //
    FontChar = &FontData[Character * 13 - Top];

    //
    // Loop each pixel height
    //
    i = 13;
    do
    {
        //
        // Loop each pixel width
        //
        j = 128;
        XOffset = Left;
        do
        {
            //
            // Check if we should draw this pixel
            //
            if (FontChar[Top] & (UCHAR)j)
            {
                //
                // We do, use the given Text Color
                //
                SetPixel(XOffset, Top, (UCHAR)TextColor);
            }
            else if (BackTextColor < 16)
            {
                //
                // This is a background pixel. We're drawing it unless it's
                // transparent.
                //
                SetPixel(XOffset, Top, (UCHAR)BackTextColor);
            }

            //
            // Increase X Offset
            //
            XOffset++;
        } while (j >>= 1);

        //
        // Move to the next Y ordinate
        //
        Top++;
    } while (--i);
}

/*++
 * @name DisplayStringXY
 *
 * The DisplayStringXY routine displays a string at the specified coordinates.
 *
 * @param String
 *        Pointer to the string to display.
 *
 * @param Left
 *        X ordinate where the string will be displayed.
 *
 * @param Top
 *        Y ordinate where the string will be displayed.
 *
 * @param TextColor
 *        Foreground color of the string.
 *
 * @param BackColor
 *        Background color of the string.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
DisplayStringXY(PUCHAR String,
                ULONG Left,
                ULONG Top,
                ULONG TextColor,
                ULONG BackColor)
{
    //
    // Loop every character
    //
    while (*String)
    {
        //
        // Display a character
        //
        DisplayCharacter(*String, Left, Top, TextColor, BackColor);

        //
        // Move to next character and next position
        //
        String++;
        Left += 8;
    }
}

/*++
 * @name VidDisplayStringXY
 *
 * The VidDisplayStringXY routine displays a string at the specified coordinates.
 *
 * @param String
 *        Pointer to the string to display.
 *
 * @param Left
 *        X ordinate where the string will be displayed.
 *
 * @param Top
 *        Y ordinate where the string will be displayed.
 *
 * @param Transparent
 *        Specifies whether the text should be printed with a transparent
 *        background color.
 *
 * @return None.
 *
 * @remarks Unlike VidDisplayString, this variant allows the caller to choose
 *          the location where the string will be displayed. However, the
 *          foreground color of the string cannot be chosen and will always be
 *          red. As well, the background color will be yellow, unless the
 *          transparent flag has been set.
 *
 *--*/
VOID
NTAPI
VidDisplayStringXY(PUCHAR String,
                   ULONG Left,
                   ULONG Top,
                   BOOLEAN Transparent)
{
    ULONG BackColor;

    //
    // If the caller wanted transparent, then send the special value (16), else
    // use our default and call the helper routine.
    //
    BackColor = (Transparent) ? 16 : 14;
    DisplayStringXY(String, Left, Top, 12, BackColor);
}

/*++
 * @name SetPaletteEntryRGB
 *
 * The SetPaletteEntryRGB routine sets the RGB palette according to the
 * specified parameter.
 *
 * @param Id
 *        Index of the palette entry to modify.
 *
 * @param Rgb
 *        Combined value of the red, green, and blue values of tha palette
 *        to set.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
SetPaletteEntryRGB(IN ULONG Id,
                   IN ULONG Rgb)
{
    PCHAR Colors = (PCHAR)&Rgb;

    //
    // Set the palette index
    //
    __outpb(0x3C8, (UCHAR)Id);

    //
    // Set RGB colors
    //
    __outpb(0x3C9, Colors[2] >> 2);
    __outpb(0x3C9, Colors[1] >> 2);
    __outpb(0x3C9, Colors[0] >> 2);
}

/*++
 * @name InitPaletteWithTable
 *
 * The InitPaletteWithTable routine initializes all the palette entries based
 * on the given table.
 *
 * @param Table
 *        Table containing the palette entries to initialize.
 *
 * @param Count
 *        Number of entries in the table.
 *
 * @return None.
 *
 * @remarks The VGA specifications only support 16 palettes. Count should not
 *          be above this.
 *
 *--*/
VOID
NTAPI
InitPaletteWithTable(IN PULONG Table,
                     IN ULONG Count)
{
    ULONG i;
    PULONG Entry = Table;

    //
    // Loop every entry
    //
    for (i = 0; i < Count; i++, Entry++)
    {
        //
        // Set the entry
        //
        SetPaletteEntryRGB(i, *Entry);
    }
}

/*++
 * @name SetPaletteEntry
 *
 * The SetPaletteEntry routine sets the RGB palette according to the
 * specified parameter.
 *
 * @param Id
 *        Index of the palette entry to modify.
 *
 * @param PaletteEntry
 *        Combined value of the red, green, and blue values of tha palette
 *        to set.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
SetPaletteEntry(IN ULONG Id,
                IN ULONG PaletteEntry)
{
    //
    // Set the palette index
    //
    __outpb(0x3C8, (UCHAR)Id);

    //
    // Set RGB colors
    //
    __outpb(0x3C9, PaletteEntry & 0xFF);            // AABBCCDD -> DD
    __outpb(0x3C9, (PaletteEntry >>= 8) & 0xFF);    // AABBCCDD -> AABBCC -> CC
    __outpb(0x3C9, (PaletteEntry >> 8) & 0xFF);     // AABBCC   -> AABB   -> BB
}

/*++
 * @name InitializePalette
 *
 * The InitializePalette routine initializes the default palette by setting
 * the 16 entries to their default values.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
InitializePalette(VOID)
{
    ULONG PaletteEntry[16] = {0,
                              0x20,
                              0x2000,
                              0x2020,
                              0x200000,
                              0x200020,
                              0x202000,
                              0x202020,
                              0x303030,
                              0x3F,
                              0x3F00,
                              0x3F3F,
                              0x3F0000,
                              0x3F003F,
                              0x3F3F00,
                              0x3F3F3F};
    ULONG i;

    //
    // Loop all the entries and set their palettes
    //
    for (i = 0; i < 16; i++) SetPaletteEntry(i, PaletteEntry[i]);
}

/*++
 * @name VidSetScrollRegion
 *
 * The VidSetScrollRegion routine sets the a new scroll region in which text
 * and scrolling operations will be limited to.
 *
 * @param x1
 *        Leftmost ordinate where the scroll region starts.
 *
 * @param y1
 *        Topmost ordinate where the scroll region starts.
 *
 * @param x2
 *        Rightmost ordinate where the scroll region starts.
 *
 * @param y2
 *        Bottomest ordinate where the scroll region starts.
 *
 * @return None.
 *
 * @remarks The X ordinate values must be 8-pixel aligned.
 *
 *--*/
VOID
NTAPI
VidSetScrollRegion(ULONG x1,
                   ULONG y1,
                   ULONG x2,
                   ULONG y2)
{
    //
    // Assert alignment
    //
    ASSERT((x1 & 0x7) == 0);
    ASSERT((x2 & 0x7) == 0);

    //
    // Set Scroll Region
    //
    ScrollRegion[0] = x1;
    ScrollRegion[1] = y1;
    ScrollRegion[2] = x2;
    ScrollRegion[3] = y2;

    //
    // Set current X and Y
    //
    curr_x = x1;
    curr_y = y1;
}

/*++
 * @name VidCleanUp
 *
 * The VidCleanUp routine resets the bit mask for pixel operations.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
VidCleanUp(VOID)
{
    //
    // Select bit mask register
    //
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 8);

    //
    // Clear it
    //
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 255);
}

/*++
 * @name VgaScroll
 *
 * The VgaScroll routine scrolls the specified value.
 *
 * @param Scroll
 *        Number of pixels to scroll down.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
VgaScroll(ULONG Scroll)
{
    ULONG Top;
    ULONG SourceOffset, DestOffset;
    ULONG Offset;
    ULONG i, j;

    //
    // Set memory positions of the scroll
    //
    SourceOffset = VgaBase + (ScrollRegion[1] * 80) + (ScrollRegion[0] >> 3);
    DestOffset = SourceOffset + (Scroll * 80);

    //
    // Clear the 4 planes
    //
    __outpw(0x3C4, 0xF02);

    //
    // Set the bitmask to 0xFF for all 4 planes
    //
    __outpw(0x3CE, 0xFF08);

    //
    // Set Mode 1
    //
    ReadWriteMode(1);

    //
    // Save top and check if it's above the bottom
    //
    Top = ScrollRegion[1];
    if (Top > ScrollRegion[3]) return;

    //
    // Start loop
    //
    do
    {
        //
        // Set number of bytes to loop and start offset
        //
        Offset = ScrollRegion[0] >> 3;
        j = SourceOffset;

        //
        // Check if this is part of the scroll region
        //
        if (Offset <= (ScrollRegion[2] >> 3))
        {
            //
            // Update position
            //
            i = DestOffset - SourceOffset;

            //
            // Loop the X axis
            //
            do
            {
                //
                // Write value in the new position so that we can do the scroll
                //
                WRITE_REGISTER_UCHAR((PUCHAR)j,
                                     READ_REGISTER_UCHAR((PUCHAR)j + i));

                //
                // Move to the next memory location to write to
                //
                j++;

                //
                // Move to the next byte in the region
                //
                Offset++;

                //
                // Make sure we don't go past the scroll region
                //
            } while (Offset <= (ScrollRegion[2] >> 3));
        }

        //
        // Move to the next line
        //
        SourceOffset += 80;
        DestOffset += 80;

        //
        // Increase top
        //
        Top++;

        //
        // Make sure we don't go past the scroll region
        //
    } while (Top <= ScrollRegion[3]);
}

/*++
 * @name PreserveRow
 *
 * The PreserveRow routine saves a row and moves it to a new position.
 *
 * @param CurrentTop
 *        Current position of the row.
 *
 * @param TopDelta
 *        New position of the row.
 *
 * @param Unknown
 *        Possibly direction of the copy.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
PreserveRow(IN ULONG CurrentTop,
            IN ULONG TopDelta,
            IN BOOLEAN Unknown)
{
    PUCHAR Position1, Position2;
    ULONG Count;

    //
    // Clear the 4 planes
    //
    __outpw(0x3C4, 0xF02);

    //
    // Set the bitmask to 0xFF for all 4 planes
    //
    __outpw(0x3CE, 0xFF08);

    //
    // Set Mode 1
    //
    ReadWriteMode(1);

    //
    // Check if ???
    //
    if (Unknown)
    {
        //
        // Calculate the position in memory for the row
        //
        Position1 = (PUCHAR)VgaBase + CurrentTop * 80;
        Position2 = (PUCHAR)VgaBase + 0x9600;
    }
    else
    {
        //
        // Calculate the position in memory for the row
        //
        Position1 = (PUCHAR)VgaBase + 0x9600;
        Position2 = (PUCHAR)VgaBase + CurrentTop * 80;
    }

    //
    // Set the count and make sure it's above 0
    Count = TopDelta * 80;
    if (Count)
    {
        //
        // Loop every pixel
        //
        do
        {
            //
            // Write the data back on the other position
            //
            WRITE_REGISTER_UCHAR(Position1, READ_REGISTER_UCHAR(Position2));

            //
            // Increase both positions
            //
            Position2++;
            Position1++;
        } while (--Count);
    }
}

/*++
 * @name BitBlt
 *
 * The BitBlt routine peforms a blitting operation.
 *
 * @param Left
 *        X Ordinate of the blit.
 *
 * @param Top
 *        Y Ordinate of the blit.
 *
 * @param Width
 *        Width of the blit rectangle.
 *
 * @param Height
 *        Heigh of the blig rectangle.
 *
 * @param Buffer
 *        Pointer to the graphic buffer where to perform the blit.
 *
 * @param BitsPerPixel
 *        Number of bits per pixel in the buffer.
 *
 * @param Delta
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks This code has an optimized path for 4 bit per pixel operations.
 *
 *--*/
VOID
NTAPI
BitBlt(IN ULONG Left,
       IN ULONG Top,
       IN ULONG Width,
       IN ULONG Height,
       IN PUCHAR Buffer,
       IN ULONG BitsPerPixel,
       IN ULONG Delta)
{
    ULONG LeftAnd, LeftShifted, LeftPlusOne, LeftPos;
    ULONG lMask, rMask;
    UCHAR NotlMask;
    ULONG Distance;
    ULONG DistanceMinusLeftBpp;
    ULONG SomeYesNoFlag, SomeYesNoFlag2;
    PUCHAR PixelPosition, m;
    PUCHAR i, k;
    ULONG j;
    ULONG x;
    ULONG Plane;
    UCHAR LeftArray[84];
    PUCHAR CurrentLeft;
    PUCHAR l;
    ULONG LoopCount;
    UCHAR pMask, PlaneShift;
    BOOLEAN Odd;
    UCHAR Value;

    //
    // Check if the buffer isn't 4bpp
    //
    if (BitsPerPixel != 4)
    {
        //
        // FIXME: TODO
        //
        DbgPrint("Unhandled BitBlt\n"
                 "%lxx%lx @ (%lx,%lx)\n"
                 "Bits Per Pixel %lx\n"
                 "Buffer: %p. Delta: %lx\n",
                 Width,
                 Height,
                 Left,
                 Top,
                 BitsPerPixel,
                 Buffer,
                 Delta);
        return;
    }

    //
    // Get the masks and other values
    //
    LeftAnd = Left & 0x7;
    lMask = lMaskTable[LeftAnd];
    Distance = Width + Left;
    rMask = rMaskTable[(Distance - 1) & 0x7];
    Left >>= 3;

    //
    // Set some values
    //
    SomeYesNoFlag = FALSE;
    SomeYesNoFlag2 = FALSE;
    Distance = (Distance - 1) >> 3;
    DistanceMinusLeftBpp = Distance - Left;

    //
    // Check if the distance is equal to the left position and add the masks
    //
    if (Left == Distance) lMask += rMask;

    //
    // Check if there's no distance offset
    //
    if (DistanceMinusLeftBpp)
    {
        //
        // Set the first flag on
        //
        SomeYesNoFlag = TRUE;

        //
        // Decrease offset and check if we still have one
        //
        if (--DistanceMinusLeftBpp)
        {
            //
            // Still have a distance offset
            //
            SomeYesNoFlag2 = TRUE;
        }
    }

    //
    // Calculate initial pixel position
    //
    PixelPosition = (PUCHAR)VgaBase + (Top * 80) + Left;

    //
    // Set loop buffer variable
    //
    i = Buffer;

    //
    // Switch to mode 0
    //
    ReadWriteMode(0);

    //
    // Leave now if the height is 0
    //
    if (Height <= 0) return;

    //
    // Set more weird values
    //
    CurrentLeft = &LeftArray[Left];
    NotlMask = ~(UCHAR)lMask;
    LeftPlusOne = Left + 1;
    LeftShifted = (lMask << 8) | 8;
    j = Height;

    //
    // Start the height loop
    //
    do
    {
        //
        // Start the plane loop
        //
        Plane = 0;
        do
        {
            //
            // Clear the current value
            //
            *CurrentLeft = 0;
            LoopCount = 0;

            //
            // Set the buffer loop variable for this loop
            //
            k = i;

            //
            // Calculate plane shift and pixel mask
            //
            PlaneShift = 1 << Plane;
            pMask = PixelMask[LeftAnd];

            //
            // Check if we have a width
            //
            if (Width > 0)
            {
                //
                // Loop it
                //
                l = CurrentLeft;
                x = Width;
                do
                {
                    //
                    // Check if we're odd and increase the loop count
                    //
                    Odd = LoopCount & 1 ? TRUE : FALSE;
                    LoopCount++;
                    if (Odd)
                    {
                        //
                        // Check for the plane shift
                        //
                        if (*k & PlaneShift)
                        {
                            //
                            // Write the pixel mask
                            //
                            *l |= pMask;
                        }

                        //
                        // Increase buffer position
                        //
                        k++;
                    }
                    else
                    {
                        //
                        // Check for plane shift
                        //
                        if ((*k >> 4) & PlaneShift)
                        {
                            //
                            // Write the pixel mask
                            //
                            *l |= pMask;
                        }
                    }

                    //
                    // Shift the pixel mask
                    //
                    pMask >>= 1;
                    if (!pMask)
                    {
                        //
                        // Move to the next current left position and clear it
                        //
                        l++;
                        *l = 0;

                        //
                        // Set the pixel mask to 0x80
                        //
                        pMask = 0x80;
                    }
                } while (--x);
            }

            //
            // Set the plane value
            //
            __outpw(0x3C4, (1 << (Plane + 8) | 2));

            //
            // Select the bitmask register and write the mask
            //
            __outpw(0x3CE, (USHORT)LeftShifted);

            //
            // Read the current Pixel value
            //
            Value = READ_REGISTER_UCHAR(PixelPosition);

            //
            // Add our mask
            //
            Value = (Value & NotlMask) | *CurrentLeft;

            //
            // Set current left for the loop, and write new pixel value
            //
            LeftPos = LeftPlusOne;
            WRITE_REGISTER_UCHAR(PixelPosition, Value);

            //
            // Set loop pixel position and check if we should loop
            //
            m = PixelPosition + 1;
            if (SomeYesNoFlag2)
            {
                //
                // Set the bitmask to 0xFF for all 4 planes
                //
                __outpw(0x3CE, 0xFF08);

                //
                // Check if we have any distance left
                //
                if (DistanceMinusLeftBpp > 0)
                {
                    //
                    // Start looping it
                    //
                    x = DistanceMinusLeftBpp;
                    do
                    {
                        //
                        // Write the value
                        //
                        WRITE_REGISTER_UCHAR(m, LeftArray[LeftPos]);

                        //
                        // Go to the next position
                        //
                        m++;
                        LeftPos++;
                    } while (--x);
                }
            }

            //
            // Check if the first flag is on
            //
            if (SomeYesNoFlag)
            {
                //
                // Set the mask value
                //
                __outpw(0x3CE, (rMask << 8) | 8);

                //
                // Read the current Pixel value
                //
                Value = READ_REGISTER_UCHAR(m);

                //
                // Add our mask
                //
                Value = (Value & ~(UCHAR)rMask) | LeftArray[LeftPos];

                //
                // Set current left for the loop, and write new pixel value
                //
                WRITE_REGISTER_UCHAR(m, Value);
            }
        } while (++Plane < 4);

        //
        // Update pixel position, buffer and height
        //
        PixelPosition += 80;
        i += Delta;
    } while (--j);
}

/*++
 * @name RleBitBlt
 *
 * The RleBitBlt routine performs a blitting operation on an RLE bitmap.
 *
 * @param Left
 *        X Ordinate of the blit.
 *
 * @param Top
 *        Y Ordinate of the blit.
 *
 * @param Width
 *        Width of the blit rectangle.
 *
 * @param Height
 *        Height of the blit rectangle.
 *
 * @param Buffer
 *        Pointer to the RLE graphics buffer where to perform the blit.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
RleBitBlt(IN ULONG Left,
          IN ULONG Top,
          IN ULONG Width,
          IN ULONG Height,
          IN PUCHAR Buffer)
{
    ULONG YDelta;
    ULONG x;
    ULONG RleValue, NewRleValue;
    ULONG Color, Color2;
    ULONG i, j;
    ULONG Code;

    //
    // Set Y height and current X value and start loop
    //
    YDelta = Top + Height - 1;
    x = Left;
    for (;;)
    {
        //
        // Get the current value and advance in the buffer
        //
        RleValue = *Buffer;
        Buffer++;
        if (RleValue)
        {
            //
            // Check if we've gone past the edge
            //
            if ((x + RleValue) > (Width + Left))
            {
                //
                // Fixeup the pixel value
                //
                RleValue = Left - x + Width;
            }

            //
            // Get the new value
            //
            NewRleValue = *Buffer;

            //
            // Get the two colors
            //
            Color = NewRleValue >> 4;
            Color2 = NewRleValue & 0xF;

            //
            // Increase buffer positition
            //
            Buffer++;

            //
            // Check if we need to do a fill
            //
            if (Color == Color2)
            {
                //
                // Do a fill and continue the loop
                //
                RleValue += x;
                VidSolidColorFill(x, YDelta, RleValue - 1, YDelta, (UCHAR)Color);
                x = RleValue;
                continue;
            }

            //
            // Check if the pixel value is 1 or below
            //
            if (RleValue > 1)
            {
                //
                // Set loop variables
                //
                i = (RleValue - 2) / 2 + 1;
                do
                {
                    //
                    // Set the pixels
                    //
                    SetPixel(x, YDelta, (UCHAR)Color);
                    x++;
                    SetPixel(x, YDelta, (UCHAR)Color2);
                    x++;

                    //
                    // Decrease pixel value
                    //
                    RleValue -= 2;
                } while (--i);
            }

            //
            // Check if there is any value at all
            //
            if (RleValue)
            {
                //
                // Set the pixel and increase posititon
                //
                SetPixel(x, YDelta, (UCHAR)Color);
                x++;
            }

            //
            // Start over
            //
            continue;
        }

        //
        // Get the current pixel value
        //
        RleValue = *Buffer;
        Code = RleValue;
        switch (Code)
        {
            //
            // Case 0
            //
            case 0:
                //
                // Set new x value, decrease distance and restart
                //
                x = Left;
                YDelta--;
                Buffer++;
                continue;

            //
            // Case 1
            //
            case 1:
                //
                // Done
                //
                return;

            //
            // Case 2
            //
            case 2:
                //
                // Set new x value, decrease distance and restart
                //
                Buffer++;
                x += *Buffer;
                Buffer++;
                YDelta -= *Buffer;
                Buffer++;
                continue;

            //
            // Other values
            //
            default:
                Buffer++;
                break;
        }

        //
        // Check if we've gone past the edge
        //
        if ((x + RleValue) > (Width + Left))
        {
            //
            // Set fixed up loop count
            //
            i = RleValue - Left - Width + x;

            //
            // Fixup pixel value
            //
            RleValue -= i;
        }
        else
        {
            //
            // Clear loop count
            //
            i = 0;
        }

        //
        // Check the value now
        //
        if (RleValue > 1)
        {
            //
            // Set loop variables
            //
            j = (RleValue - 2) / 2 + 1;
            do
            {
                //
                // Get the new value
                //
                NewRleValue = *Buffer;

                //
                // Get the two colors
                //
                Color = NewRleValue >> 4;
                Color2 = NewRleValue & 0xF;

                //
                // Increase buffer positition
                //
                Buffer++;

                //
                // Set the pixels
                //
                SetPixel(x, YDelta, (UCHAR)Color);
                x++;
                SetPixel(x, YDelta, (UCHAR)Color2);
                x++;

                //
                // Decrease pixel value
                //
                RleValue -= 2;
            } while (--j);
        }

        //
        // Check if there is any value at all
        //
        if (RleValue)
        {
            //
            // Set the pixel and increase posititon
            //
            Color = *Buffer >> 4;
            Buffer++;
            SetPixel(x, YDelta, (UCHAR)Color);
            x++;
            i--;
        }

        //
        // Check loop count now
        //
        if ((LONG)i > 0)
        {
            //
            // Decrease it
            //
            i--;

            //
            // Set new position
            //
            Buffer = Buffer + (i / 2) + 1;
        }

        //
        // Check if we need to increase the buffer
        //
        if ((ULONG_PTR)Buffer & 1) Buffer++;
    }
}

/*++
 * @name VidBufferToScreenBlt
 *
 * The VidBufferToScreenBlt routine performs a 4bpp blitting operation in order
 * to copy the contents of a graphics buffer to screen.
 *
 * @param Buffer
 *        Pointer to a graphics buffer to copy to screen.
 *
 * @param Left
 *        X ordinate where to start the copy.
 *
 * @param Top
 *        Y ordinate where to start the copy.
 *
 * @param Width
 *        Width of the copy (blit) rectangle.
 *
 * @param Height
 *        Height of the copy (blit) rectangle.
 *
 * @param Delta
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks This function internally does a BitBlt.
 *
 *--*/
VOID
NTAPI
VidBufferToScreenBlt(IN PUCHAR Buffer,
                     IN ULONG Left,
                     IN ULONG Top,
                     IN ULONG Width,
                     IN ULONG Height,
                     IN ULONG Delta)
{
    //
    // Make sure we have a width and height
    //
    if (!(Width) || !(Height)) return;

    //
    // Call the helper function
    //
    BitBlt(Left, Top, Width, Height, Buffer, 4, Delta);
}

/*++
 * @name VidDisplayString
 *
 * The VidDisplayString routine displays a string on the screen.
 *
 * @param String
 *        Pointer to the string to display.
 *
 * @return None.
 *
 * @remarks The string will be written at the last coordinates inside the
 *          current scrolling region, using the previously set foreground
 *          text color. The string's background color will be transparent.
 *          For greater control over the position of the string, see the
 *          VidDisplayStringXY routine.
 *
 *--*/
VOID
NTAPI
VidDisplayString(PUCHAR String)
{
    ULONG TopDelta = 14;

    //
    // Start looping the string
    //
    while (*String)
    {
        //
        // Treat new-line separately
        //
        if (*String == '\n')
        {
            //
            // Modify Y position
            //
            curr_y += TopDelta;
            if (curr_y >= ScrollRegion[3])
            {
                //
                // Scroll the view
                //
                VgaScroll(TopDelta);
                curr_y -= TopDelta;

                //
                // Preserve row
                //
                PreserveRow(curr_y, TopDelta, TRUE);
            }

            //
            // Update current X
            //
            curr_x = ScrollRegion[0];

            //
            // Preseve the current row
            //
            PreserveRow(curr_y, TopDelta, FALSE);
        }
        else if (*String == '\r')
        {
            //
            // Update current X
            //
            curr_x = ScrollRegion[0];

            //
            // Check if we're being followed by a new line
            //
            if (String[1] != '\n') NextLine = TRUE;
        }
        else
        {
            //
            // Check if we had a \n\r last time
            //
            if (NextLine)
            {
                //
                // We did, preserve the current row
                //
                PreserveRow(curr_y, TopDelta, TRUE);
                NextLine = FALSE;
            }

            //
            // Display this character
            //
            DisplayCharacter(*String, curr_x, curr_y, TextColor, 16);
            curr_x += 8;

            //
            // Check if we should scroll
            //
            if (curr_x > ScrollRegion[2])
            {
                //
                // Update Y position and check if we should scroll it
                //
                curr_y += TopDelta;
                if (curr_y > ScrollRegion[3])
                {
                    //
                    // Do the scroll
                    //
                    VgaScroll(TopDelta);
                    curr_y -= TopDelta;

                    //
                    // Save the row
                    //
                    PreserveRow(curr_y, TopDelta, TRUE);
                }

                //
                // Update X
                //
                curr_x = ScrollRegion[0];
            }
        }

        //
        // Get the next character
        //
        String++;
    }
}

/*++
 * @name VidBitBlt
 *
 * The VidBitBlt routine performs a blitting operation with a botmap.
 *
 * @param Buffer
 *        Pointer to a bitmap graphics buffer.
 *
 * @param Left
 *        X ordinate where to start the blit.
 *
 * @param Top
 *        Y ordinate where to start the blit.
 *
 * @return None.
 *
 * @remarks This function has an optimized path for RLE bitmaps.
 *
 *--*/
VOID
NTAPI
VidBitBlt(PUCHAR Buffer,
          ULONG Left,
          ULONG Top)
{
    PBITMAPINFOHEADER BitmapInfoHeader;
    LONG Delta;
    PUCHAR BitmapOffset;

    //
    // Get the Bitmap Header
    //
    BitmapInfoHeader = (PBITMAPINFOHEADER)Buffer;

    //
    // Initialize the palette
    //
    InitPaletteWithTable((PULONG)(Buffer + BitmapInfoHeader->biSize),
                         (BitmapInfoHeader->biClrUsed) ?
                         BitmapInfoHeader->biClrUsed : 16);

    //
    // Make sure we can support this bitmap
    //
    ASSERT((BitmapInfoHeader->biBitCount * BitmapInfoHeader->biPlanes) <= 4);

    //
    // Calculate the delta and align it on 32-bytes, then calculate the actual
    // start of the bitmap data.
    //
    Delta = (BitmapInfoHeader->biBitCount * BitmapInfoHeader->biWidth) + 31;
    Delta >>= 3;
    Delta &= ~3;
    BitmapOffset = Buffer + sizeof(BITMAPINFOHEADER) + 16 * sizeof(ULONG);

    //
    // Check the compression of the bitmap
    //
    if (BitmapInfoHeader->biCompression == 2)
    {
        //
        // Make sure we have a width and a height
        //
        if ((BitmapInfoHeader->biWidth) && (BitmapInfoHeader->biHeight))
        {
            //
            // We can use RLE Bit Blt
            //
            RleBitBlt(Left,
                      Top,
                      BitmapInfoHeader->biWidth,
                      BitmapInfoHeader->biHeight,
                      BitmapOffset);
        }
    }
    else
    {
        //
        // Check if the height is negative
        //
        if (BitmapInfoHeader->biHeight < 0)
        {
            //
            // Make it positive in the header
            //
            BitmapInfoHeader->biHeight *= -1;
        }
        else
        {
            //
            // Update buffer offset
            //
            BitmapOffset += ((BitmapInfoHeader->biHeight -1) * Delta);
            Delta *= -1;
        }

        //
        // Make sure we have a width and a height
        //
        if ((BitmapInfoHeader->biWidth) && (BitmapInfoHeader->biHeight))
        {
            //
            // Do the BitBlt
            //
            BitBlt(Left,
                   Top,
                   BitmapInfoHeader->biWidth,
                   BitmapInfoHeader->biHeight,
                   BitmapOffset,
                   BitmapInfoHeader->biBitCount,
                   Delta);
        }
    }
}

/*++
 * @name VidScreenToBufferBlt
 *
 * The VidScreenToBufferBlt routine performs a blitting operation in order to
 * copy the contents of the screen to a graphics buffer.
 *
 * @param Buffer
 *        Pointer to the graphics buffer in which to copy the screen contents.
 *
 * @param Left
 *        X ordinate where to start the copy (blit).
 *
 * @param Top
 *        Y ordinate where to start the copy (blit).
 *
 * @param Width
 *        Width of the copy (blit) rectangle.
 *
 * @param Height
 *        Heigh of the copy (blit) rectangle.
 *
 * @param Delta
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
VidScreenToBufferBlt(PUCHAR Buffer,
                     ULONG Left,
                     ULONG Top,
                     ULONG Width,
                     ULONG Height,
                     ULONG Delta)
{
    ULONG Plane;
    ULONG XDistance;
    ULONG LeftDelta, RightDelta;
    ULONG PixelOffset;
    PUCHAR PixelPosition;
    PUCHAR k, i;
    PULONG m;
    UCHAR Value, Value2;
    UCHAR a;
    ULONG b;
    ULONG x, y;

    //
    // Calculate total distance to copy on X
    //
    XDistance = Left + Width - 1;

    //
    // Start at plane 0
    //
    Plane = 0;

    //
    // Calculate the 8-byte left and right deltas
    //
    LeftDelta = Left & 7;
    RightDelta = 8 - LeftDelta;

    //
    // Clear the destination buffer
    //
    RtlZeroMemory(Buffer, Delta * Height);

    //
    // Calculate the pixel offset and convert the X distance into byte form
    //
    PixelOffset = Top * 80 + (Left >> 3);
    XDistance >>= 3;

    //
    // Loop the 4 planes
    //
    do
    {
        //
        // Set the current pixel position and reset buffer loop variable
        //
        PixelPosition = (PUCHAR)VgaBase + PixelOffset;
        i = Buffer;

        //
        // Set Mode 0
        //
        ReadWriteMode(0);

        //
        // Set the current plane
        //
        __outpw(0x3CE, (Plane << 8) | 4);

        //
        // Make sure we have a height
        //
        if (Height > 0)
        {
            //
            // Start the outer Y loop
            //
            y = Height;
            do
            {
                //
                // Read the current value
                //
                m = (PULONG)i;
                Value = READ_REGISTER_UCHAR(PixelPosition);

                //
                // Set Pixel Position loop variable
                //
                k = PixelPosition + 1;

                //
                // Check if we're still within bounds
                //
                if (Left <= XDistance)
                {
                    //
                    // Start X Inner loop
                    //
                    x = (XDistance - Left) + 1;
                    do
                    {
                        //
                        // Read the current value
                        //
                        Value2 = READ_REGISTER_UCHAR(k);

                        //
                        // Increase pixel position
                        //
                        k++;

                        //
                        // Do the blt
                        //
                        a = Value2 >> (UCHAR)RightDelta;
                        a |= Value << (UCHAR)LeftDelta;
                        b = lookup[a & 0xF];
                        a >>= 4;
                        b <<= 16;
                        b |= lookup[a];

                        //
                        // Save new value to buffer
                        //
                        *m |= (b << Plane);

                        //
                        // Move to next desination location
                        //
                        m++;

                        //
                        // Write new value
                        //
                        Value = Value2;
                    } while (--x);
                }

                //
                // Update pixel position
                //
                PixelPosition += 80;
                i += Delta;
            } while (--y);
        }
   } while (++Plane < 4);
}

/*++
 * @name VidSolidColorFill
 *
 * The VidSolidColorFill routine fills the specified rectangular area with the
 * given color.
 *
 * @param Left
 *        Leftmost ordinate where the fill region starts.
 *
 * @param Top
 *        Topmost ordinate where the fill region starts.
 *
 * @param Right
 *        Rightmost ordinate where the fill region starts.
 *
 * @param Bottom
 *        Bottomest ordinate where the fill region starts.
 *
 * @param Color
 *        Color with which to fill the given region.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
VidSolidColorFill(IN ULONG Left,
                  IN ULONG Top,
                  IN ULONG Right,
                  IN ULONG Bottom,
                  IN UCHAR Color)
{
    ULONG rMask, lMask;
    ULONG LeftOffset, RightOffset, Distance;
    PUCHAR Offset;
    ULONG i, j;

    //
    // Get the left and right masks, shifts, and delta
    //
    LeftOffset = Left >> 3;
    lMask = (lMaskTable[Left & 0x7] << 8) | 8;
    RightOffset = Right >> 3;
    rMask = (rMaskTable[Right & 0x7] << 8) | 8;
    Distance = RightOffset - LeftOffset;

    //
    // If there is no distance, then combine the right and left masks
    //
    if (!Distance) lMask &= rMask;

    //
    // Switch to mode 10
    //
    ReadWriteMode(10);

    //
    // Clear the 4 planes (we're already in unchained mode here)
    //
    __outpw(0x3C4, 0xF02);

    //
    // Select the color don't care register
    //
    __outpw(0x3CE, 7);

    //
    // Calculate pixel position for the read
    //
    Offset = VgaBase + (Top * 80) + (PUCHAR)LeftOffset;

    //
    // Select the bitmask register and write the mask
    //
    __outpw(0x3CE, (USHORT)lMask);

    //
    // Check if the top coord is below the bottom one
    //
    if (Top <= Bottom)
    {
        //
        // Start looping each line
        //
        i = (Bottom - Top) + 1;
        do
        {
            //
            // Read the previous value and add our color
            //
            WRITE_REGISTER_UCHAR(Offset, READ_REGISTER_UCHAR(Offset) & Color);

            //
            // Move to the next line
            //
            Offset += 80;
        } while (--i);
    }

    //
    // Check if we have a delta
    //
    if (Distance)
    {
        //
        // Calculate new pixel position
        //
        Offset = VgaBase + (Top * 80) + (PUCHAR)RightOffset;
        Distance--;

        //
        // Select the bitmask register and write the mask
        //
        __outpw(0x3CE, (USHORT)rMask);

        //
        // Check if the top coord is below the bottom one
        //
        if (Top <= Bottom)
        {
            //
            // Start looping each line
            //
            i = (Bottom - Top) + 1;
            do
            {
                //
                // Read the previous value and add our color
                //
                WRITE_REGISTER_UCHAR(Offset,
                                     READ_REGISTER_UCHAR(Offset) & Color);

                //
                // Move to the next line
                //
                Offset += 80;
            } while (--i);
        }

        //
        // Check if we still have a delta
        //
        if (Distance)
        {
            //
            // Calculate new pixel position
            //
            Offset = VgaBase + (Top * 80) + (PUCHAR)(LeftOffset + 1);

            //
            // Set the bitmask to 0xFF for all 4 planes
            //
            __outpw(0x3CE, 0xFF08);

            //
            // Check if the top coord is below the bottom one
            //
            if (Top <= Bottom)
            {
                //
                // Start looping each line
                //
                i = (Bottom - Top) + 1;
                do
                {
                    //
                    // Loop the shift delta
                    //
                    if (Distance > 0)
                    {
                        for (j = Distance; j; Offset++, j--)
                        {
                            //
                            // Write the color
                            //
                            WRITE_REGISTER_UCHAR(Offset, Color);
                        }
                    }

                    //
                    // Update position in memory
                    //
                    Offset += (80 - Distance);
                } while (--i);
            }
        }
    }
}

