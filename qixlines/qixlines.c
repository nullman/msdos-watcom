/**
 * QIX Lines
 *
 * Draw QIX lines with alternating colors.
 */

#include <conio.h>                      // clrscr getch kbhit
#include <dos.h>                        // int86 outp inp
#include <math.h>                       // sin
#include <stdio.h>                      // printf sprintf
#include <stdlib.h>                     // EXIT_SUCCESS EXIT_FAILURE malloc
#include <string.h>

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
#define PALETTE_INDEX 0x3C8             // use to reset palette index
#define PALETTE_DATA 0x3C9              // use to write colors to palette
#define INPUT_STATUS 0x3DA              // vga status register
#define VRTRACE_BIT 0x08                // 1 = vertical retrace, ram access ok for 1.25ms
#define PI 3.14159265359                // PI

#define COLOR_BG 0                      // default background color
#define COLOR_FG 1                      // default foreground color
#define MAX_SIN 180                     // maximum allowed value for sin math
#define HISTORY_SIZE 10                 // how many lines to display at once
#define STEP 8                          // line spacing
#define STEP_RANGE 6                    // spacing plus/minus range

typedef unsigned char byte;
typedef unsigned short ushort;

typedef struct {
    short x1;
    short y1;
    short x2;
    short y2;
    byte color;
} line_s;

typedef struct {
    byte help;
    byte vga_mode;
} args_s;

byte far *vga = (byte far *)VIDEO_MEMORY;
byte vga_mode, *palette;
ushort screen_width, screen_height, num_colors;

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

void set_black_palette() {
    ushort i;

    outp(PALETTE_INDEX, 0);
    for (i = 0; i < num_colors * 3; i++) {
        palette[i] = 0;
        outp(PALETTE_DATA, 0);
    }
}

void set_palette(byte index, byte r, byte g, byte b) {
    ushort i;

    palette[index * 3 + 0] = r;
    palette[index * 3 + 1] = g;
    palette[index * 3 + 2] = b;

    outp(PALETTE_INDEX, 0);
    for (i = 0; i < num_colors * 3; i++) {
        outp(PALETTE_DATA, palette[i]);
    }
}

byte random_color() {
    if (vga_mode == VGA_256_COLOR_MODE) {
        return rand() % num_colors;
    } else {
        // use all colors except black (0)
        return rand() % (num_colors - 1) + 1;
    }
}

byte random_neighbor_color() {
    static byte index = 0;
    byte prev_r, prev_g, prev_b, r, g, b;

    if (vga_mode != VGA_256_COLOR_MODE) {
        return random_color();
    }

    prev_r = palette[index * 3 + 0];
    prev_g = palette[index * 3 + 1];
    prev_b = palette[index * 3 + 2];

    // randomly change each color by -1, 0, or 1
    r = (prev_r + rand() % 3 + 63) % 64;
    g = (prev_g + rand() % 3 + 63) % 64;
    b = (prev_b + rand() % 3 + 63) % 64;

    // update next palette slot
    index = (index + 1) % num_colors;
    if (index == 0) index = 1;
    set_palette(index, r, g, b);

    return index;
}

void line_copy(line_s *target_line, line_s *source_line) {
    target_line->x1 = source_line->x1;
    target_line->y1 = source_line->y1;
    target_line->x2 = source_line->x2;
    target_line->y2 = source_line->y2;
    target_line->color = source_line->color;
}

void draw_pixel(ushort x, ushort y, byte color) {
    ushort offset;

    offset = y * screen_width + x;      // slower, but easy to understand
    //offset = (y<<8) + (y<<6) + x;       // faster, but harder to understand
    vga[offset] = color;
}

void draw_line(line_s *line) {
    ushort x1, y1, x2, y2, x, y;
    byte color;
    int dx, dy, sx, sy, e1, e2;

    x1 = line->x1;
    y1 = line->y1;
    x2 = line->x2;
    y2 = line->y2;
    color = line->color;

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
        if (x < screen_width && y < screen_height) {
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

ushort next_degree(ushort degree) {
    // add randomly to the degree
    ushort d = degree + STEP + rand() % (STEP_RANGE * 2 + 1) - STEP_RANGE;
    if (d >= MAX_SIN) d = d - MAX_SIN;
    return d;
}

double deg_to_rad(ushort degree) {
    return degree * PI / 180.0;
}

void next_line(line_s *line, line_s *line_delta, line_s *line_degree) {
    line->color = random_neighbor_color();

    // randomly add to the degrees
    line_degree->x1 = next_degree(line_degree->x1);
    line_degree->y1 = next_degree(line_degree->y1);
    line_degree->x2 = next_degree(line_degree->x2);
    line_degree->y2 = next_degree(line_degree->y2);

    // add using sin modified by a delta for each coordinate dimension
    line->x1 += (ushort)(line_delta->x1 * sin(deg_to_rad(line_degree->x1)));
    line->y1 += (ushort)(line_delta->y1 * sin(deg_to_rad(line_degree->y1)));
    line->x2 += (ushort)(line_delta->x2 * sin(deg_to_rad(line_degree->x2)));
    line->y2 += (ushort)(line_delta->y2 * sin(deg_to_rad(line_degree->y2)));

    // if any coordinates are out of range, reverse their direction and change color
    if (line->x1 < 0) {
        line->x1 = 0 - line->x1;
        line_delta->x1 = -line_delta->x1;
    }
    if (line->x1 >= screen_width) {
        line->x1 = screen_width - (line->x1 - screen_width);
        line_delta->x1 = -line_delta->x1;
    }
    if (line->y1 < 0) {
        line->y1 = 0 - line->y1;
        line_delta->y1 = -line_delta->y1;
    }
    if (line->y1 >= screen_height) {
        line->y1 = screen_height - (line->y1 - screen_height);
        line_delta->y1 = -line_delta->y1;
    }
    if (line->x2 < 0) {
        line->x2 = 0 - line->x2;
        line_delta->x2 = -line_delta->x2;
    }
    if (line->x2 >= screen_width) {
        line->x2 = screen_width - (line->x2 - screen_width);
        line_delta->x2 = -line_delta->x2;
    }
    if (line->y2 < 0) {
        line->y2 = 0 - line->y2;
        line_delta->y2 = -line_delta->y2;
    }
    if (line->y2 >= screen_height) {
        line->y2 = screen_height - (line->y2 - screen_height);
        line_delta->y2 = -line_delta->y2;
    }
}

// draw lines until a key is pressed
void draw_lines() {
    line_s line, line_delta, line_degree, line_history[HISTORY_SIZE];
    ushort i, history_index;

    // randomize starting values
    line.x1 = rand() % screen_width;
    line.y1 = rand() % screen_height;
    line.x2 = rand() % screen_width;
    line.y2 = rand() % screen_height;
    line.color = COLOR_BG;

    line_delta.x1 = STEP;
    line_delta.y1 = STEP;
    line_delta.x2 = STEP;
    line_delta.y2 = STEP;

    line_degree.x1 = rand() % MAX_SIN;
    line_degree.y1 = rand() % MAX_SIN;
    line_degree.x2 = rand() % MAX_SIN;
    line_degree.y2 = rand() % MAX_SIN;

    // initialize history
    for (i = 0; i < HISTORY_SIZE; i++) {
        line_copy(&line_history[i], &line);
    }
    history_index = 0;

    // loop until key-press
    while (!kbhit()) {
        //wait_for_retrace();
        wait(3);

        // draw next line
        next_line(&line, &line_delta, &line_degree);
        draw_line(&line);

        // undraw oldest line
        line_history[history_index].color = COLOR_BG;
        draw_line(&line_history[history_index]);

        // add to history
        line_copy(&line_history[history_index++], &line);
        if (history_index >= HISTORY_SIZE) history_index = 0;
    }

    getch();
}

void parse_args(int argc, char *argv[], args_s *args) {
    int i;

    args->help = 0;
    args->vga_mode = VGA_256_COLOR_MODE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "lo") == 0) {
            args->vga_mode = VGA_256_COLOR_MODE;
        } else if (strcmp(argv[i], "hi") == 0) {
            args->vga_mode = VGA_16_COLOR_MODE;
        } else {
            args->help = 1;
        }
    }
}

int main(int argc, char *argv[]) {
    args_s args;

    parse_args(argc, argv, &args);

    if (args.help) {
        printf("Usage: %s [lo|hi]\n", argv[0]);
        printf("Where:\n");
        printf("  lo - VGA 256 color mode (320x200)\n");
        printf("  hi - VGA 16 color mode (640x480)\n");
        return EXIT_FAILURE;
    }

    vga_mode = args.vga_mode;
    if (vga_mode == VGA_256_COLOR_MODE) {
        screen_width = VGA_256_COLOR_SCREEN_WIDTH;
        screen_height = VGA_256_COLOR_SCREEN_HEIGHT;
        num_colors = VGA_256_COLOR_NUM_COLORS;
    } else {
        screen_width = VGA_16_COLOR_SCREEN_WIDTH;
        screen_height = VGA_16_COLOR_SCREEN_HEIGHT;
        num_colors = VGA_16_COLOR_NUM_COLORS;
    }

    set_mode(vga_mode);

    palette = malloc(VGA_256_COLOR_NUM_COLORS * 3 * sizeof(byte));

    set_black_palette();

    draw_lines();

    set_mode(TEXT_MODE);

    return EXIT_SUCCESS;
}
