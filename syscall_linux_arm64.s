// Small-C Runtime - Linux ARM64 System Calls
// System call numbers for Linux ARM64

.text
.globl _sys_read
.globl _sys_write
.globl _sys_open
.globl _sys_close
.globl _sys_exit
.globl _start

// System call numbers
.equ SYS_READ,    63
.equ SYS_WRITE,   64
.equ SYS_OPENAT,  56
.equ SYS_CLOSE,   57
.equ SYS_EXIT,    93
.equ AT_FDCWD,    -100

// int _sys_read(int fd, char *buf, int count)
_sys_read:
    mov     x8, #SYS_READ
    svc     #0
    ret

// int _sys_write(int fd, char *buf, int count)
_sys_write:
    mov     x8, #SYS_WRITE
    svc     #0
    ret

// int _sys_open(char *path, int flags, int mode)
_sys_open:
    // ARM64 uses openat, so we need to set AT_FDCWD
    mov     x3, x2              // mode
    mov     x2, x1              // flags
    mov     x1, x0              // pathname
    mov     x0, #AT_FDCWD       // dirfd
    mov     x8, #SYS_OPENAT
    svc     #0
    ret

// int _sys_close(int fd)
_sys_close:
    mov     x8, #SYS_CLOSE
    svc     #0
    ret

// void _sys_exit(int code)
_sys_exit:
    mov     x8, #SYS_EXIT
    svc     #0

// Program entry point
_start:
    // Call main
    bl      main
    
    // Exit with return value
    mov     x8, #SYS_EXIT
    svc     #0
