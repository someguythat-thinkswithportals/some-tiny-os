#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "timer.h"
#include "memory.h"
#include "syscall.h"
#include "shell.h"
#include "fs.h"
#include "serial.h"
#include "scheduler.h"
#include "elf.h"
#include "pipe.h"

static void idle_task(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

static void clock_task(void) {
    uint64_t last = 0;
    while (1) {
        uint64_t now = timer_ticks();
        if (now - last >= 100) {
            last = now;
            serial_write("CLOCK: ", 7);
            uint64_t sec = now / 100;
            char buf[16];
            int pos = 0;
            if (sec == 0) { buf[pos++] = '0'; }
            else {
                char rev[16];
                int rpos = 0;
                while (sec > 0) {
                    rev[rpos++] = '0' + (sec % 10);
                    sec /= 10;
                }
                for (int i = rpos - 1; i >= 0; i--) buf[pos++] = rev[i];
            }
            buf[pos++] = 's';
            buf[pos++] = '\n';
            serial_write(buf, pos);
        }
        __asm__ volatile("hlt");
    }
}

static uint64_t userspace_entry;

static void load_userspace(void) {
    uint64_t new_cr3 = paging_create();
    if (!new_cr3) {
        serial_write("FAILED to create shell address space\n", 37);
        return;
    }

    uint64_t entry;
    if (elf_load_into_address_space(new_cr3, "/bin/shell", &entry) < 0) {
        serial_write("FAILED to load shell.elf from filesystem\n", 40);
        paging_destroy(new_cr3);
        return;
    }

    current_task->cr3 = new_cr3;
    userspace_entry = entry;

    serial_write("Shell loaded from filesystem\n", 28);
}

static void dump_hex64_serial(uint64_t v) {
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nib = (v >> i) & 0xF;
        serial_putchar(nib < 10 ? '0' + nib : 'a' + nib - 10);
    }
}

static void go_userspace(void) {
    serial_write("go_userspace: entering\n", 22);
    tss_set_rsp0(0x9000);
    serial_write("go_userspace: tss_set_rsp0 done, cr3=", 37);
    dump_hex64_serial(current_task->cr3);
    serial_putchar('\n');
    paging_switch(current_task->cr3);
    serial_write("go_userspace: paging_switch done\n", 33);

    __asm__ volatile(
        "cli\n"
        "movq $0x9000, %%rsp\n"
        "pushq $0x23\n"
        "pushq $0x500000\n"
        "pushq $0x202\n"
        "pushq $0x1B\n"
        "pushq %0\n"
        "iretq\n"
        : : "r"(userspace_entry) : "memory"
    );
    serial_write("go_userspace: after iretq (should not reach)\n", 44);
}

void kmain(void) {
    serial_init();
    serial_write("some-tiny-os v0.2 booting...\n", 29);

    vga_init();
    vga_setcolor(VGA_LIGHT_GREY);
    vga_writestring("some-tiny-os booting...\n");

    gdt_init();
    serial_write("GDT OK\n", 7);
    vga_writestring("GDT OK\n");

    idt_init();
    serial_write("IDT OK\n", 7);
    vga_writestring("IDT OK\n");

    timer_init();
    serial_write("Timer OK\n", 9);
    vga_writestring("Timer OK\n");

    keyboard_init();
    serial_write("Keyboard OK\n", 12);
    vga_writestring("Keyboard OK\n");

    memory_init((void*)0x200000, 0x200000);
    serial_write("Memory OK\n", 10);
    vga_writestring("Memory OK\n");

    scheduler_init();
    serial_write("Scheduler OK\n", 13);
    vga_writestring("Scheduler OK\n");

    pipe_init();
    serial_write("Pipes OK\n", 9);
    vga_writestring("Pipes OK\n");

    task_create("clock", clock_task);
    serial_write("Clock task created\n", 19);
    vga_writestring("Clock task created\n");

    task_create("idle", idle_task);
    serial_write("Idle task created\n", 18);
    vga_writestring("Idle task created\n");

    syscall_init();
    serial_write("Syscall OK\n", 11);
    vga_writestring("Syscall OK\n");

    fs_init();
    serial_write("FS OK\n", 6);
    vga_writestring("FS OK\n");

    vga_clear(VGA_BLACK);
    vga_setcolor(VGA_LIGHT_GREY);

    vga_writestring("Welcome to some-tiny-os!\n");
    vga_writestring("Starting userspace...\n\n");

    serial_write("Starting userspace...\n", 21);
    serial_write("Calling load_userspace...\n", 25);
    load_userspace();
    serial_write("load_userspace returned\n", 23);
    go_userspace();

    shell_run();
}
