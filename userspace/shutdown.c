#include <some-libc.h>
#include <syscall.h>

int main(void) {
    printf("Shutting down...\n");
    _syscall1(SYS_SHUTDOWN, 0);
    return 0;
}
