#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
char keyboard_read(void);
int keyboard_data(void);
void keyboard_inject(const char* str);

#endif
