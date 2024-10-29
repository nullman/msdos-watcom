/**
 * Colors
 *
 * Display VGA colors.
 */

#include <conio.h>                      // clrscr getch
#include <dos.h>                        // int86 outp inp
#include <stdio.h>                      // printf sprintf
#include <stdlib.h>                     // EXIT_SUCCESS EXIT_FAILURE malloc

#define VIDEO_INT 0x10                  // BIOS video interrupt
#define SET_MODE 0x00                   // BIOS function to set video mode
#define VGA_16_COLOR_MODE 0x12          // use to set 16 color VGA mode
#define VGA_256_COLOR_MODE 0x13         // use to set 256 color VGA mode
#define TEXT_MODE 0x03                  // use to set text mode
#define PIXEL_PLOT 0x0C                 // BIOS function to plot a pixel
#define VIDEO_MEMORY 0xA0000000L        // start of video memory
#define VGA_16_COLOR_SCREEN_WIDTH 640   // width in pixels of VGA mode 0x12
#define VGA_16_COLOR_SCREEN_HEIGHT 480  // height in pixels of VGA mode 0x12
#define VGA_16_COLOR_NUM_COLORS 16      // number of colors in VGA mode 0x12
#define VGA_256_COLOR_SCREEN_WIDTH 320  // width in pixels of VGA mode 0x13
#define VGA_256_COLOR_SCREEN_HEIGHT 200 // height in pixels of VGA mode 0x13
#define VGA_256_COLOR_NUM_COLORS 256    // number of colors in VGA mode 0x13
#define INPUT_STATUS 0x3DA              // vga status register
#define VRTRACE_BIT 0x08                // 1 = vertical retrace, ram access ok for 1.25ms

typedef unsigned char byte;
typedef unsigned short ushort;

byte far *VGA = (byte far *)VIDEO_MEMORY;
ushort screen_width;

void wait_for_retrace() {
    while(inp(INPUT_STATUS) & VRTRACE_BIT);
    while(!(inp(INPUT_STATUS) & VRTRACE_BIT));
}

void wait(ushort time) {
    ushort i;

    for (i = 0; i < time; i++) {
        wait_for_retrace();
    }
}

void set_mode(byte mode) {
    union REGS regs;

    regs.h.ah = SET_MODE;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

void draw_pixel(ushort x, ushort y, byte color) {
    ushort offset;

    offset = y * screen_width + x;      // slower, but easy to understand
    //offset = (y<<8) + (y<<6) + x;       // faster, but harder to understand
    VGA[offset] = color;
}

void draw_box(ushort x1, ushort y1, ushort x2, ushort y2, byte color) {
    ushort x, y;

    if (y1 > y2) {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    if (x1 > x2) {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    for (y = y1; y < y2; y++) {
        for (x = x1; x < x2; x++) {
            draw_pixel(x, y, color);
        }
    }
}

void draw_colors(
    ushort width, ushort height, ushort colors,
    byte x_count, byte y_count)
{
    ushort x1, y1, x2, y2, c;
    ushort x_cell = width / x_count;
    ushort y_cell = height / y_count;

    for (c = 0; c < colors; c++) {
        x1 = (c % x_count) * x_cell;
        x2 = x1 + x_cell;
        y1 = (c / y_count) * y_cell;
        y2 = y1 + y_cell;
        draw_box(x1, y1, x2, y2, c);
    }
}

int main(void) {
    set_mode(VGA_256_COLOR_MODE);
    screen_width = VGA_256_COLOR_SCREEN_WIDTH;
    wait_for_retrace();
    draw_colors(
        VGA_256_COLOR_SCREEN_WIDTH,
        VGA_256_COLOR_SCREEN_HEIGHT,
        VGA_256_COLOR_NUM_COLORS,
        16, 16);

    getch();

    set_mode(TEXT_MODE);

    return EXIT_SUCCESS;
}
