CC := gcc
CFLAGS := -ffreestanding -m64 -mno-red-zone -fno-builtin -fno-stack-protector -Wall -Wextra -I.
AR := ar

OBJS := crt0.o string.o stdio.o stdlib.o file.o

.PHONY: all clean

all: some-libc.a

some-libc.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c some-libc.h syscall.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.a
