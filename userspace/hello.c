#include <some-libc.h>

int main(void) {
    printf("Hello from ELF!\n");
    printf("Testing libc: %d + %d = %d\n", 2, 3, 2 + 3);
    printf("Hex 255 = 0x%x\n", 255);
    printf("String: %s\n", "libc works!");
    halt();
    return 0;
}
