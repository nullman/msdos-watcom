/**
 * Baud
 *
 * Slows down text output to various baud rate speeds.
 */

#include <conio.h>                      // clrscr getch kbhit
#include <dos.h>                        // delay
#include <stdio.h>                      // printf, getchar, putchar
#include <stdlib.h>                     // atoi, EXIT_SUCCESS, EXIT_FAILURE

#define ESC    0x1b
#define CTRL_C 0x03

void usage(char app[]) {
    printf("Usage: %s BAUD [FILE]\n", app);
    printf("Where BAUD is any number, but often one of the standard bit rates:\n");
    printf("  50, 110, 300, 600, 1200, 2400, 4800, 9600\n");
    printf("If FILE is given, then it is used as the source. Otherwise, STDIN is used.\n");
}

int main(int argc, char *argv[]) {
    int baud, baud_delay, rc;
    char ch, kc;
    FILE *file = NULL;

    if (argc < 2 || argc > 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    baud = atoi(argv[1]);

    if (baud < 1 || baud > 9600) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 3) {
        file = fopen(argv[2], "r");
        if (file == NULL) {
            printf("Could not open file for reading: %s\n", argv[2]);
            return EXIT_FAILURE;
        }
    }

    baud_delay = 8 * 1000 / baud;
    printf("-- baud_delay: %d\n", baud_delay);
    kc = 0;

    // loop until ESC or CTRL-C is pressed
    while (kc != ESC && kc != CTRL_C) {
        while (!kbhit()) {
            if (file == NULL) {
                ch = getchar();
                if (ch == EOF) return EXIT_SUCCESS;
            } else {
                ch = fgetc(file);
                if (feof(file)) {
                    fclose(file);
                    return EXIT_SUCCESS;
                }
            }
            if (baud_delay > 0) delay(baud_delay);
            putchar(ch);
            fflush(stdout);
        }

        kc = getch();
        if (kc == (char)0) kc = getch();
    }

    if (file != NULL) fclose(file);

    return EXIT_SUCCESS;
}
