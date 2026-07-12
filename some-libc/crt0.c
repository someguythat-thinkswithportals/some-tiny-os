extern int main(void);
extern void exit(int);

void _start(void) {
    exit(main());
    while (1);
}
