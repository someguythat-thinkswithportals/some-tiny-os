#include <some-libc.h>
#include <syscall.h>

int main(void) {
    char buf[400];
    int n = _syscall3(SYS_LS, 0, (uint64_t)buf, 400);
    if (n < 0) {
        printf("ls: failed\n");
    } else {
        buf[n] = 0;
        printf("%s", buf);
    }
    return 0;
}
