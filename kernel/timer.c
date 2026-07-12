#include "timer.h"
#include "idt.h"
#include "io.h"
#include "scheduler.h"

static volatile uint64_t tick_count;

static void timer_handler(registers_t* r) {
    (void)r;
    tick_count++;
    scheduler_tick();
}

void timer_init(void) {
    tick_count = 0;

    uint32_t divisor = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    idt_register_handler(32, timer_handler);
}

uint64_t timer_ticks(void) {
    return tick_count;
}

void timer_sleep(uint64_t ms) {
    uint64_t target = tick_count + (ms / 10);
    while (tick_count < target) {
        __asm__ volatile("hlt");
    }
}
