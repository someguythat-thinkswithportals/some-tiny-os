#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEY_UP     0x80
#define KEY_DOWN   0x81
#define KEY_LEFT   0x82
#define KEY_RIGHT  0x83
#define KEY_HOME   0x84
#define KEY_END    0x85
#define KEY_DELETE 0x86

void keyboard_init(void);
int keyboard_read(void);
int keyboard_data(void);
void keyboard_inject(const char* str);

#endif
