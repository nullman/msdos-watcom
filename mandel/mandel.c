/**
 * Mandelbrot
 *
 * Draw colored Mandelbrot.
 *
 * Inspiration: https://github.com/ms0g/dosbrot/blob/main/SRC/DOSBROT.C
 */

#include <conio.h>                      // clrscr getch kbhit
#include <dos.h>                        // int86 outp inp
#include <stdlib.h>                     // EXIT_SUCCESS EXIT_FAILURE malloc

#define VIDEO_INT 0x10                  // BIOS video interrupt
#define SET_MODE 0x00                   // BIOS function to set video mode
#define VGA_16_COLOR_MODE 0x12          // use to set 16 color VGA mode
#define VGA_256_COLOR_MODE 0x13         // use to set 256 color VGA mode
#define TEXT_MODE 0x03                  // use to set text mode
#define PIXEL_PLOT 0x0C                 // BIOS function to plot a pixel
#define VIDEO_MEMORY 0xA0000000L        // start of video memory
#define VGA_256_COLOR_SCREEN_WIDTH 320  // width in pixels of VGA mode 0x13
#define VGA_256_COLOR_SCREEN_HEIGHT 200 // height in pixels of VGA mode 0x13
#define VGA_256_COLOR_NUM_COLORS 256    // number of colors in VGA mode 0x13
#define PALETTE_INDEX 0x3C8             // use to reset palette index
#define PALETTE_DATA 0x3C9              // use to write colors to palette
#define INPUT_STATUS 0x3DA              // vga status register
#define VRTRACE_BIT 0x08                // 1 = vertical retrace, ram access ok for 1.25ms

typedef unsigned char byte;
typedef unsigned short ushort;


enum COLORS {
    // dark colors
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHT_GRAY,
    DARK_GRAY,

    // light colors
    LIGHT_BLUE,
    LIGHT_GREEN,
    LIGHT_CYAN,
    LIGHT_RED,
    LIGHT_MAGENTA,
    YELLOW,
    WHITE
};

static char palette[12] = {
    LIGHT_MAGENTA,
    MAGENTA,
    BLUE,
    LIGHT_BLUE,
    0x50,
    CYAN,
    LIGHT_CYAN,
    0x60,
    YELLOW,
    RED,
    LIGHT_RED,
    GREEN
};

byte far *vga = (byte far *)VIDEO_MEMORY;

void wait_for_retrace() {
    while(inp(INPUT_STATUS) & VRTRACE_BIT);
    while(!(inp(INPUT_STATUS) & VRTRACE_BIT));
}

void set_mode(byte mode) {
    union REGS regs;

    regs.h.ah = SET_MODE;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

void draw_pixel(ushort x, ushort y, byte color) {
    ushort offset;

    offset = y * VGA_256_COLOR_SCREEN_WIDTH + x; // slower, but easy to understand
    // offset = (y<<8) + (y<<6) + x;       // faster, but harder to understand
    vga[offset] = color;
}

int compute_mandelbrot(double re, double im, int iteration) {
    int i;
    double r2, i2;
    double zR = re;
    double zI = im;

    for (i = 0; i < iteration; ++i) {
        r2 = zR * zR;
        i2 = zI * zI;

        if (r2 + i2 > 4.0) {
            return i;
        }

        zI = 2.0 * zR * zI + im;
        zR = r2 - i2 + re;
    }

    return iteration;
}

void draw_mandelbrot() {
    int x, y, value;
    double im;

    const double remin = -2.0;
    const double remax = 1.0;
    const double immin = -1.0;
    const double immax = 1.0;

    const double dx = (remax - remin) / (VGA_256_COLOR_SCREEN_WIDTH - 1);
    const double dy = (immax - immin) / (VGA_256_COLOR_SCREEN_HEIGHT - 1);

    wait_for_retrace();

    for (y = 0; y < VGA_256_COLOR_SCREEN_HEIGHT; y++) {
        im = immax - y * dy;

        for (x = 0; x < VGA_256_COLOR_SCREEN_WIDTH; x++) {
            value = compute_mandelbrot(remin + x * dx, im, 100);

            if (value == 100)
                draw_pixel(x, y, BLACK);
            else {
                value = (value < 0) ? 0 : (value > 11) ? 11 : value;
                draw_pixel(x, y, palette[value]);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    set_mode(VGA_256_COLOR_MODE);

    draw_mandelbrot();

    getch();

    set_mode(TEXT_MODE);

    return EXIT_SUCCESS;
}
