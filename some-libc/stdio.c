#include "some-libc.h"
#include "syscall.h"

int putchar(int c) {
    char ch = (char)c;
    _syscall3(SYS_WRITE, 1, (uint64_t)&ch, 1);
    return c;
}

int puts(const char* s) {
    int len = strlen(s);
    _syscall3(SYS_WRITE, 1, (uint64_t)s, len);
    _syscall3(SYS_WRITE, 1, (uint64_t)"\n", 1);
    return len + 1;
}

static void _printf_int(char** p, int val, int base, int uppercase) {
    char buf[32];
    int neg = 0;
    unsigned int uval;

    if (base == 10 && val < 0) {
        neg = 1;
        uval = (unsigned int)(-val);
    } else {
        uval = (unsigned int)val;
    }

    int i = 0;
    if (uval == 0) {
        buf[i++] = '0';
    } else {
        while (uval > 0) {
            int d = uval % base;
            buf[i++] = (d < 10) ? ('0' + d) : (uppercase ? 'A' + d - 10 : 'a' + d - 10);
            uval /= base;
        }
    }

    if (neg) buf[i++] = '-';

    while (i > 0) *(*p)++ = buf[--i];
}

int printf(const char* fmt, ...) {
    char buf[512];
    char* p = buf;
    char* end = buf + sizeof(buf) - 1;

    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    for (int i = 0; fmt[i] && p < end; i++) {
        if (fmt[i] != '%') {
            *p++ = fmt[i];
        } else {
            i++;
            if (i >= (int)strlen(fmt)) break;

            switch (fmt[i]) {
                case 's': {
                    const char* s = __builtin_va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s && p < end) *p++ = *s++;
                    break;
                }
                case 'd': {
                    int val = __builtin_va_arg(args, int);
                    _printf_int(&p, val, 10, 0);
                    break;
                }
                case 'u': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    _printf_int(&p, (int)val, 10, 0);
                    break;
                }
                case 'x': {
                    int val = __builtin_va_arg(args, int);
                    _printf_int(&p, val, 16, 0);
                    break;
                }
                case 'X': {
                    int val = __builtin_va_arg(args, int);
                    _printf_int(&p, val, 16, 1);
                    break;
                }
                case 'c': {
                    int c = __builtin_va_arg(args, int);
                    *p++ = (char)c;
                    break;
                }
                case '%':
                    *p++ = '%';
                    break;
            }
        }
    }

    __builtin_va_end(args);
    *p = 0;

    int len = p - buf;
    _syscall3(SYS_WRITE, 1, (uint64_t)buf, len);
    return len;
}

int getchar(void) {
    char c;
    int ret = _syscall3(SYS_READ, 0, (uint64_t)&c, 1);
    if (ret <= 0) return EOF;
    return (unsigned char)c;
}

int write(int fd, const void* buf, int len) {
    return (int)_syscall3(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}

int read(int fd, void* buf, int len) {
    return (int)_syscall3(SYS_READ, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}

int pipe(int fds[2]) {
    return (int)_syscall1(SYS_PIPE, (uint64_t)fds);
}

int dup2(int oldfd, int newfd) {
    return (int)_syscall2(SYS_DUP2, (uint64_t)oldfd, (uint64_t)newfd);
}

int close(int fd) {
    return (int)_syscall1(SYS_CLOSE, (uint64_t)fd);
}
