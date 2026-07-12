#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_putchar(char c);
void serial_write(const char* buf, uint64_t len);
char serial_read(void);
int serial_data_available(void);

#endif
