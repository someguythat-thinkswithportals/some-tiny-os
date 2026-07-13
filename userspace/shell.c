#include <some-libc.h>
#include <syscall.h>

#define LINE_MAX 256

static char line[LINE_MAX];
static int line_pos;
static volatile int got_sigint;

static void sigint_handler(int sig) {
    (void)sig;
    got_sigint = 1;
}

static void shell_readline(void) {
    line_pos = 0;
    got_sigint = 0;
    while (1) {
        char c = getchar();
        if (got_sigint) {
            putchar('\n');
            line_pos = 0;
            line[0] = 0;
            return;
        }
        if (c == '\n') {
            putchar('\n');
            line[line_pos] = 0;
            return;
        } else if (c == '\b') {
            if (line_pos > 0) {
                line_pos--;
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
        } else if (c >= ' ' && line_pos < LINE_MAX - 1) {
            line[line_pos++] = c;
            putchar(c);
        }
    }
}

static int startswith(const char* s, const char* prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static void cmd_help(void) {
    printf("Available commands:\n");
    printf("  help\n");
    printf("  echo\n");
    printf("  clear\n");
    printf("  reboot\n");
    printf("  shutdown\n");
    printf("  poweroff\n");
    printf("  cat\n");
    printf("  grep\n");
    printf("  uptime\n");
    printf("  date\n");
    printf("  mkdir\n");
    printf("  rmdir\n");
    printf("  rm\n");
    printf("  cp\n");
    printf("  mv\n");
    printf("  cd\n");
    printf("  exec\n");
    printf("  cmd1 | cmd2  (pipe)\n");
}

static void cmd_cat(char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == 0) {
        printf("cat: missing filename\n");
        return;
    }
    char buf[256];
    int n = _syscall3(SYS_CAT, (uint64_t)arg, (uint64_t)buf, 256);
    if (n < 0) {
        printf("cat: file not found: %s\n", arg);
    } else {
        buf[n] = 0;
        printf("%s", buf);
    }
}

static void cmd_echo(char* arg) {
    while (*arg == ' ') arg++;
    printf("%s\n", arg);
}

static void cmd_uptime(void) {
    uint64_t ticks = _syscall0(SYS_TICKS);
    uint64_t sec = ticks / 100;
    uint64_t min = sec / 60;
    uint64_t hr = min / 60;
    uint64_t days = hr / 24;
    sec = sec % 60;
    min = min % 60;
    hr = hr % 24;
    printf("Up: %dd %dh %dm %ds\n", (int)days, (int)hr, (int)min, (int)sec);
}

static void cmd_date(void) {
    char buf[32];
    int n = _syscall2(SYS_DATETIME, (uint64_t)buf, 32);
    if (n > 0) {
        buf[n] = 0;
        printf("%s", buf);
    }
}

static void cmd_ls(char* arg) {
    char buf[400];
    int n = _syscall3(SYS_LS, (uint64_t)arg, (uint64_t)buf, 400);
    if (n < 0) {
        printf("ls: no such directory\n");
    } else {
        buf[n] = 0;
        printf("%s", buf);
    }
}

static void cmd_mkdir(char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == 0) {
        printf("mkdir: missing operand\n");
        return;
    }
    if (_syscall1(SYS_MKDIR, (uint64_t)arg) < 0)
        printf("mkdir: failed\n");
}

static void cmd_rmdir(char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == 0) {
        printf("rmdir: missing operand\n");
        return;
    }
    if (_syscall1(SYS_RMDIR, (uint64_t)arg) < 0)
        printf("rmdir: failed\n");
}

static void cmd_rm(char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == 0) {
        printf("rm: missing operand\n");
        return;
    }
    if (_syscall1(SYS_RM, (uint64_t)arg) < 0)
        printf("rm: failed\n");
}

static void cmd_reboot(void) {
    printf("Rebooting...\n");
    _syscall1(SYS_REBOOT, 0);
}

static void cmd_shutdown(void) {
    printf("Shutting down...\n");
    _syscall1(SYS_SHUTDOWN, 0);
}

static void cmd_cd(char* arg) {
    while (*arg == ' ') arg++;
    const char* path = arg;
    if (*path == 0) path = "/";
    if (_syscall2(SYS_CD, 0, (uint64_t)path) < 0)
        printf("cd: no such directory\n");
}

static void cmd_exec(char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == 0) {
        printf("exec: missing filename\n");
        return;
    }
    int ret = _syscall1(SYS_EXEC, (uint64_t)arg);
    if (ret < 0)
        printf("exec: failed\n");
}

static void execute(char* cmd);

static void cmd_pipe(char* left, char* right) {
    while (*left == ' ') left++;
    while (*right == ' ') right++;
    if (*left == 0 || *right == 0) {
        printf("pipe: missing command\n");
        return;
    }

    int fds[2];
    if (_syscall1(SYS_PIPE, (uint64_t)fds) < 0) {
        printf("pipe: failed to create pipe\n");
        return;
    }

    int pid1 = _syscall0(SYS_FORK);
    if (pid1 == 0) {
        _syscall2(SYS_DUP2, (uint64_t)fds[1], 1);
        _syscall1(SYS_CLOSE, (uint64_t)fds[0]);
        _syscall1(SYS_CLOSE, (uint64_t)fds[1]);
        execute(left);
        _syscall1(SYS_EXIT, 0);
    }

    int pid2 = _syscall0(SYS_FORK);
    if (pid2 == 0) {
        _syscall2(SYS_DUP2, (uint64_t)fds[0], 0);
        _syscall1(SYS_CLOSE, (uint64_t)fds[0]);
        _syscall1(SYS_CLOSE, (uint64_t)fds[1]);
        execute(right);
        _syscall1(SYS_EXIT, 0);
    }

    _syscall1(SYS_CLOSE, (uint64_t)fds[0]);
    _syscall1(SYS_CLOSE, (uint64_t)fds[1]);
    _syscall1(SYS_WAITPID, (uint64_t)pid1);
    _syscall1(SYS_WAITPID, (uint64_t)pid2);
}

static void execute(char* cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == 0) return;

    for (int i = 0; cmd[i]; i++) {
        if (cmd[i] == '|') {
            cmd[i] = 0;
            cmd_pipe(cmd, cmd + i + 1);
            return;
        }
    }

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        _syscall0(SYS_CLEAR);
    } else if (strcmp(cmd, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(cmd, "date") == 0) {
        cmd_date();
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "poweroff") == 0 || strcmp(cmd, "exit") == 0) {
        cmd_shutdown();
    } else if (startswith(cmd, "echo ")) {
        cmd_echo(cmd + 5);
    } else if (strcmp(cmd, "echo") == 0) {
        printf("\n");
    } else if (startswith(cmd, "cat ")) {
        cmd_cat(cmd + 4);
    } else if (startswith(cmd, "ls")) {
        char* arg = 0;
        if (cmd[2] == ' ') arg = cmd + 3;
        else if (cmd[2] != 0) arg = cmd + 2;
        cmd_ls(arg);
    } else if (startswith(cmd, "mkdir ")) {
        cmd_mkdir(cmd + 6);
    } else if (startswith(cmd, "rmdir ")) {
        cmd_rmdir(cmd + 6);
    } else if (startswith(cmd, "rm ")) {
        cmd_rm(cmd + 3);
    } else if (startswith(cmd, "cd ")) {
        cmd_cd(cmd + 3);
    } else if (strcmp(cmd, "cd") == 0) {
        cmd_cd("");
    } else if (startswith(cmd, "exec ")) {
        cmd_exec(cmd + 5);
    } else if (startswith(cmd, "cp ")) {
        char* args = cmd + 3;
        while (*args == ' ') args++;
        if (*args == 0) {
            printf("cp: missing file arguments\n");
        } else {
            char* space = strchr(args, ' ');
            if (!space) {
                printf("cp: missing destination\n");
            } else {
                *space = 0;
                char* dst = space + 1;
                while (*dst == ' ') dst++;
                int ret = _syscall2(SYS_CP, (uint64_t)args, (uint64_t)dst);
                if (ret < 0) printf("cp: failed\n");
            }
        }
    } else if (startswith(cmd, "mv ")) {
        char* args = cmd + 3;
        while (*args == ' ') args++;
        if (*args == 0) {
            printf("mv: missing file arguments\n");
        } else {
            char* space = strchr(args, ' ');
            if (!space) {
                printf("mv: missing destination\n");
            } else {
                *space = 0;
                char* dst = space + 1;
                while (*dst == ' ') dst++;
                int ret = _syscall2(SYS_MV, (uint64_t)args, (uint64_t)dst);
                if (ret < 0) printf("mv: failed\n");
            }
        }
    } else {
        printf("unknown command: %s\n", cmd);
    }
}

int main(void) {
    signal(SIGINT, sigint_handler);
    printf("Welcome to some-tiny-os!\n");
    printf("Type 'help' for commands\n\n");

    while (1) {
        printf("some-tiny-os$ ");
        shell_readline();
        execute(line);
    }
    return 0;
}
