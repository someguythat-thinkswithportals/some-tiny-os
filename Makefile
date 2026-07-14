NASM := nasm
CC := gcc
LD := ld
OBJCOPY := objcopy
QEMU := qemu-system-x86_64

NASM_ELF := -f elf64
CCFLAGS := -ffreestanding -m64 -mno-red-zone -fno-builtin -fno-stack-protector -Wall -Wextra -I.
USER_CFLAGS := -ffreestanding -m64 -mno-red-zone -fno-builtin -fno-stack-protector -Wall -Wextra -I some-libc
LDFLAGS := -m elf_x86_64 -T kernel/kernel.ld -nostdlib

USER_PROGRAMS := shell cat ls echo grep shutdown touch uname

KERNEL_OBJS := kernel/entry.o kernel/kernel.o kernel/vga.o kernel/gdt.o kernel/idt.o \
            kernel/isr.o kernel/keyboard.o kernel/timer.o kernel/memory.o kernel/syscall.o \
            kernel/shell.o kernel/serial.o kernel/scheduler.o kernel/elf.o kernel/cmos.o \
            kernel/pipe.o kernel/fs/ata.o kernel/fs/tinyfs.o kernel/fs/vfs.o

.PHONY: all kernel clean run

all: some-tiny-os.img

kernel: kernel.elf

boot/boot.bin: boot/boot.asm
	$(NASM) -f bin -o $@ $<

boot/stage2.bin: boot/stage2.asm
	$(NASM) -f bin -o $@ $<

kernel/entry.o: kernel/entry.asm
	$(NASM) $(NASM_ELF) -o $@ $<

kernel/isr.o: kernel/isr.asm
	$(NASM) $(NASM_ELF) -o $@ $<

kernel/%.o: kernel/%.c
	$(CC) $(CCFLAGS) -c -o $@ $<

kernel/fs/%.o: kernel/fs/%.c
	$(CC) $(CCFLAGS) -c -o $@ $<

tools/mkfs.tinyfs: tools/mkfs.tinyfs.c
	$(CC) -o $@ $<

tools/tinyfs-add: tools/tinyfs-add.c
	$(CC) -o $@ $<

some-libc/some-libc.a: some-libc/crt0.c some-libc/string.c some-libc/stdio.c some-libc/stdlib.c some-libc/file.c
	$(MAKE) -C some-libc

define build-user-program
userspace/$(1).o: userspace/$(1).c some-libc/some-libc.h
	$$(CC) $$(USER_CFLAGS) -c -o $$@ $$<

userspace/$(1).elf: userspace/$(1).o some-libc/some-libc.a userspace/hello.ld
	$$(LD) -m elf_x86_64 -T userspace/hello.ld -o $$@ userspace/$(1).o some-libc/some-libc.a -static -e _start
endef

$(foreach prog,$(USER_PROGRAMS),$(eval $(call build-user-program,$(prog))))

USER_ELFS := $(addprefix userspace/,$(addsuffix .elf,$(USER_PROGRAMS)))

kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

some-tiny-os.img: boot/boot.bin boot/stage2.bin kernel.bin tools/mkfs.tinyfs tools/tinyfs-add $(USER_ELFS)
	dd if=/dev/zero of=$@ bs=512 count=131072 2>/dev/null
	dd if=boot/boot.bin of=$@ bs=512 conv=notrunc 2>/dev/null
	dd if=boot/stage2.bin of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	dd if=kernel.bin of=$@ bs=512 seek=3 conv=notrunc 2>/dev/null
	./tools/mkfs.tinyfs $@
	./tools/tinyfs-add $@ userspace/shell.elf bin/shell
	./tools/tinyfs-add $@ userspace/cat.elf bin/cat
	./tools/tinyfs-add $@ userspace/ls.elf bin/ls
	./tools/tinyfs-add $@ userspace/echo.elf bin/echo
	./tools/tinyfs-add $@ userspace/grep.elf bin/grep
	./tools/tinyfs-add $@ userspace/shutdown.elf bin/shutdown
	./tools/tinyfs-add $@ userspace/touch.elf bin/touch
	./tools/tinyfs-add $@ userspace/uname.elf bin/uname


run: some-tiny-os.img
	$(QEMU) -drive format=raw,file=$< -m 64M

run-serial: some-tiny-os.img
	$(QEMU) -drive format=raw,file=$< -m 64M -serial stdio

clean:
	rm -f *.bin *.elf *.img
	rm -f boot/*.bin boot/*.o
	rm -f kernel/*.o kernel/*.bin kernel/*.elf
	rm -f kernel/fs/*.o
	rm -f userspace/*.bin userspace/*.o userspace/*.elf
	rm -f tools/mkfs.tinyfs tools/tinyfs-add
	$(MAKE) -C some-libc clean
