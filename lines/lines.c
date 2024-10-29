/**
 * Lines
 *
 * Draw lines using Bresenham's algorithm:
 *
 * https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
 */

#include <conio.h>                      // clrscr getch
#include <dos.h>                        // int86 outp inp
#include <math.h>                       // sin
#include <stdio.h>                      // printf sprintf
#include <stdlib.h>                     // EXIT_SUCCESS EXIT_FAILURE malloc

#define VIDEO_INT 0x10                  // BIOS video interrupt
#define SET_MODE 0x00                   // BIOS function to set video mode
#define VGA_256_COLOR_MODE 0x13         // use to set 256 color VGA mode
#define TEXT_MODE 0x03                  // use to set text mode
#define PIXEL_PLOT 0x0C                 // BIOS function to plot a pixel
#define VIDEO_MEMORY 0xA0000000L        // start of video memory
#define SCREEN_WIDTH 320                // width in pixels of VGA mode 0x13
#define SCREEN_HEIGHT 200               // height in pixels of VGA mode 0x13
#define NUM_COLORS 256                  // number of colors in VGA mode
#define INPUT_STATUS 0x3DA              // vga status register
#define VRTRACE_BIT 0x08                // 1 = vertical retrace, ram access ok for 1.25ms
#define PI 3.14159265359                // PI

// use all colors except black (0)
#define RANDOM_COLOR() (rand() % (NUM_COLORS - 1) + 1)

typedef unsigned char byte;
typedef unsigned short ushort;

byte far *VGA = (byte far *)VIDEO_MEMORY;

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

    offset = y * SCREEN_WIDTH + x;      // slower, but easy to understand
    //offset = (y<<8) + (y<<6) + x;       // faster, but harder to understand
    VGA[offset] = color;
}

void draw_line(ushort x1, ushort y1, ushort x2, ushort y2, byte color) {
    ushort x, y;
    int dx, dy, sx, sy, e1, e2;

    dx = x2 - x1;
    if (dx < 0) dx = -dx;
    sx = (x1 < x2) ? 1 : -1;
    dy = y2 - y1;
    if (dy > 0) dy = -dy;
    sy = (y1 < y2) ? 1 : -1;
    e1 = dx + dy;

    x = x1;
    y = y1;

    while (1) {
        if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
            draw_pixel(x, y, color);
        }
        if (x == x2 && y == y2) break;
        e2 = 2 * e1;
        if (e2 >= dy) {
            if (x == x2) break;
            e1 += dy;
            x += sx;
        }
        if (e2 <= dx) {
            if (y == y2) break;
            e1 += dx;
            y += sy;
        }
    }
}

double degrees_to_radians(ushort degree) {
    return degree * PI / 180.0;
}

void draw_lines() {
    ushort x1, y1, x2, y2, deg;
    byte color;

    x1 = 0;
    y1 = 0;
    x2 = SCREEN_WIDTH - 1;
    y2 = 0;
    color = 1;

    for (deg = 0; deg <= 90; deg += 1) {
        wait_for_retrace();
        draw_line(x1, y1, x2, y2, color);
        y2 = (ushort)((SCREEN_HEIGHT - 1) * sin(degrees_to_radians(deg)));
    }
    y2 = SCREEN_HEIGHT - 1;
    for (deg = 90; deg <= 180; deg += 1) {
        wait_for_retrace();
        draw_line(x1, y1, x2, y2, color);
        x2 = (ushort)((SCREEN_WIDTH - 1) * sin(degrees_to_radians(deg)));
    }
}

int main(void) {
    set_mode(VGA_256_COLOR_MODE);

    draw_lines();

    getch();

    set_mode(TEXT_MODE);

    return EXIT_SUCCESS;
}
