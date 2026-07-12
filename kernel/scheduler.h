#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "idt.h"
#include "memory.h"

#define TASK_NAME_MAX 16
#define MAX_TASKS 16
#define STACK_SIZE 4096
#define MAX_FDS 8

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

typedef struct task {
    uint32_t pid;
    char name[TASK_NAME_MAX];
    task_state_t state;
    uint64_t rsp;
    uint64_t stack_base;
    uint64_t cr3;
    struct task* next;
    uint32_t parent_pid;
    int32_t exit_code;
    uint8_t waited;
    vm_region_t vm_regions[MAX_VM_REGIONS];
    uint8_t fd_type[MAX_FDS];
    void* fd_data[MAX_FDS];
} task_t;

void scheduler_init(void);
int task_create(const char* name, void (*entry)(void));
void scheduler_yield(void);
void scheduler_tick(void);

task_t* get_current_task(void);
int sys_fork(uint64_t parent_rsp, uint64_t parent_cr3, registers_t* r);
void sys_exit(int code);
int sys_waitpid(uint32_t pid);
void scheduler_remove_zombie(uint32_t pid);

void task_block(task_t* task);
void task_wakeup(task_t* task);

extern volatile uint64_t* pending_switch;
extern volatile uint64_t pending_rsp;
extern volatile uint64_t pending_cr3;
extern task_t* current_task;

#endif
