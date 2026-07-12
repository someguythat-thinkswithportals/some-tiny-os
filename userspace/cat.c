#include <some-libc.h>
#include <syscall.h>

int main(void) {
    char c;
    while (1) {
        int ret = _syscall3(SYS_READ, 0, (uint64_t)&c, 1);
        if (ret <= 0) break;
        _syscall3(SYS_WRITE, 1, (uint64_t)&c, 1);
        if (c == '\n') break;
    }
    return 0;
}
