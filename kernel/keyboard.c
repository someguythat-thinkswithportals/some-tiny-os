#include "keyboard.h"
#include "idt.h"
#include "io.h"
#include "serial.h"

#define BUF_SIZE 256

static char buf[BUF_SIZE];
static int head;
static int tail;

static const char keymap[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char keymap_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static int shift_pressed;
static int ctrl_pressed;

static void keyboard_handler(registers_t* r) {
    (void)r;
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        return;
    }
    if (scancode == 0x1D) {
        ctrl_pressed = 1;
        return;
    }
    if (scancode == 0x9D) {
        ctrl_pressed = 0;
        return;
    }

    if (scancode & 0x80) return;

    char c = 0;
    if (scancode < sizeof(keymap)) {
        if (shift_pressed)
            c = keymap_shift[scancode];
        else
            c = keymap[scancode];
    }

    if (c) {
        int next = (head + 1) % BUF_SIZE;
        if (next != tail) {
            buf[head] = c;
            head = next;
        }
    }
}

void keyboard_init(void) {
    head = 0;
    tail = 0;
    shift_pressed = 0;
    ctrl_pressed = 0;
    idt_register_handler(33, keyboard_handler);
}

char keyboard_read(void) {
    while (head == tail) {
        if (serial_data_available()) {
            return serial_read();
        }
        __asm__ volatile("hlt");
    }
    char c = buf[tail];
    tail = (tail + 1) % BUF_SIZE;
    return c;
}

int keyboard_data(void) {
    return head != tail;
}

void keyboard_inject(const char* str) {
    for (int i = 0; str[i]; i++) {
        int next = (head + 1) % BUF_SIZE;
        if (next == tail) break;
        buf[head] = str[i];
        head = next;
    }
}
