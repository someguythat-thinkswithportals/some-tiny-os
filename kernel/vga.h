#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15,
};

void vga_init(void);
void vga_clear(uint8_t color);
void vga_putchar(char c);
void vga_write(const char* str, size_t len);
void vga_writestring(const char* str);
void vga_setcolor(uint8_t color);
void vga_scroll(void);
void vga_setpos(size_t row, size_t col);
void vga_getpos(size_t* row, size_t* col);

#endif
