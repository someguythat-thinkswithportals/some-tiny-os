#include <some-libc.h>
#include <syscall.h>

static int read_line(char* buf, int max) {
    int pos = 0;
    while (pos < max - 1) {
        char c;
        int ret = read(0, &c, 1);
        if (ret <= 0) break;
        if (c == '\n') break;
        buf[pos++] = c;
    }
    buf[pos] = 0;
    return pos;
}

int main(void) {
    char pattern[128];
    printf("pattern: ");
    if (read_line(pattern, sizeof(pattern)) == 0) {
        return 1;
    }

    char line[256];
    while (1) {
        int len = read_line(line, sizeof(line));
        if (len == 0) break;
        if (strstr(line, pattern)) {
            puts(line);
        }
    }

    return 0;
}
