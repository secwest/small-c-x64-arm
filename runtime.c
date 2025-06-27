/*
 * Small-C Runtime Library
 * Minimal runtime support for Small-C compiled programs
 * 
 * This provides basic I/O and file operations in the spirit
 * of Ron Cain's original Small-C runtime from Dr. Dobb's Journal
 * 
 * Original publications:
 * - "A Small C Compiler for the 8080's" Part 1 - Dr. Dobb's Journal #45 (May 1980)
 * - "A Small C Compiler for the 8080's" Part 2 - Dr. Dobb's Journal #46 (June/July 1980)
 * - "A Runtime Library for the Small C Compiler" - Dr. Dobb's Journal #48 (September 1980)
 * - Scans available at: https://archive.org/details/dr_dobbs_journal_vol_05_201803
 * - Original source: https://github.com/trcwm/smallc_v1
 * 
 * Note: The original runtime was written in 8080 assembly. This C version
 * provides equivalent functionality for modern systems.
 */

/* File descriptors */
#define STDIN  0
#define STDOUT 1
#define STDERR 2

/* File modes */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0100
#define O_TRUNC  01000

/* External system calls (implemented in assembly) */
int _sys_read(int fd, char *buf, int count);
int _sys_write(int fd, char *buf, int count);
int _sys_open(char *path, int flags, int mode);
int _sys_close(int fd);
int _sys_exit(int code);

/* Character I/O */
int putchar(int c) {
    char ch;
    ch = c;
    _sys_write(STDOUT, &ch, 1);
    return c;
}

int getchar() {
    char ch;
    if (_sys_read(STDIN, &ch, 1) != 1) {
        return -1;
    }
    return ch;
}

/* String I/O */
int puts(char *s) {
    while (*s) {
        putchar(*s);
        s++;
    }
    putchar('\n');
    return 0;
}

int gets(char *s) {
    char *p;
    int c;
    
    p = s;
    while ((c = getchar()) != '\n' && c != -1) {
        *p++ = c;
    }
    *p = '\0';
    return s;
}

/* String functions */
int strlen(char *s) {
    int n;
    n = 0;
    while (*s++) {
        n++;
    }
    return n;
}

int strcmp(char *s1, char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strcpy(char *dest, char *src) {
    char *d;
    d = dest;
    while (*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

/* Simple printf - supports %d, %s, %c, %x */
int printf(char *fmt, ...) {
    char **args;
    char *p;
    
    args = (char **)&fmt + 1;  /* Point to first argument after format */
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                    printn(*((int *)args));
                    args++;
                    break;
                case 'x':
                    printh(*((int *)args));
                    args++;
                    break;
                case 'c':
                    putchar(*((int *)args));
                    args++;
                    break;
                case 's':
                    p = *args;
                    while (*p) {
                        putchar(*p++);
                    }
                    args++;
                    break;
                case '%':
                    putchar('%');
                    break;
                default:
                    putchar('%');
                    putchar(*fmt);
                    break;
            }
        } else {
            putchar(*fmt);
        }
        fmt++;
    }
    return 0;
}

/* Print decimal number */
int printn(int n) {
    int a;
    
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    
    a = n / 10;
    if (a) {
        printn(a);
    }
    putchar(n % 10 + '0');
    return 0;
}

/* Print hexadecimal number */
int printh(int n) {
    int a;
    int d;
    
    a = n / 16;
    if (a) {
        printh(a);
    }
    
    d = n % 16;
    if (d < 10) {
        putchar(d + '0');
    } else {
        putchar(d - 10 + 'a');
    }
    return 0;
}

/* File operations */
int open(char *path, int flags) {
    return _sys_open(path, flags, 0644);
}

int creat(char *path) {
    return _sys_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

int close(int fd) {
    return _sys_close(fd);
}

int read(int fd, char *buf, int count) {
    return _sys_read(fd, buf, count);
}

int write(int fd, char *buf, int count) {
    return _sys_write(fd, buf, count);
}

/* File I/O helpers */
int fgetc(int fd) {
    char ch;
    if (read(fd, &ch, 1) != 1) {
        return -1;
    }
    return ch;
}

int fputc(int c, int fd) {
    char ch;
    ch = c;
    write(fd, &ch, 1);
    return c;
}

int fputs(char *s, int fd) {
    write(fd, s, strlen(s));
    return 0;
}

/* Memory operations */
int memset(char *s, int c, int n) {
    char *p;
    p = s;
    while (n--) {
        *p++ = c;
    }
    return s;
}

int memcpy(char *dest, char *src, int n) {
    char *d;
    char *s;
    d = dest;
    s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/* Utility functions */
int abs(int n) {
    return n < 0 ? -n : n;
}

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

/* Simple atoi */
int atoi(char *s) {
    int n;
    int neg;
    
    n = 0;
    neg = 0;
    
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    
    return neg ? -n : n;
}

/* Exit function */
int exit(int code) {
    _sys_exit(code);
    return 0;  /* Never reached */
}
