#include "vga.h"
#include "io.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)

static size_t row;
static size_t col;
static uint8_t color;

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

void vga_init(void) {
    row = 0;
    col = 0;
    color = VGA_LIGHT_GREY;
    vga_clear(VGA_BLACK);
}

void vga_clear(uint8_t bg) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', bg);
        }
    }
    row = 0;
    col = 0;
}

void vga_setcolor(uint8_t c) {
    color = c;
}

void vga_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', VGA_BLACK);
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        row++;
        col = 0;
    } else if (c == '\r') {
        col = 0;
    } else if (c == '\t') {
        col = (col + 8) & ~7;
    } else if (c == '\b') {
        if (col > 0) col--;
    } else if (c >= ' ') {
        VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry(c, color);
        col++;
    }
    if (col >= VGA_WIDTH) {
        col = 0;
        row++;
    }
    if (row >= VGA_HEIGHT) {
        vga_scroll();
        row = VGA_HEIGHT - 1;
    }
}

void vga_write(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        vga_putchar(str[i]);
    }
}

void vga_writestring(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_setpos(size_t r, size_t c) {
    if (r < VGA_HEIGHT && c < VGA_WIDTH) {
        row = r;
        col = c;
    }
}

void vga_getpos(size_t* r, size_t* c) {
    *r = row;
    *c = col;
}


