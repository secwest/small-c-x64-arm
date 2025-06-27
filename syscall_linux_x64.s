# Small-C Runtime - Linux x64 System Calls
# System call numbers for Linux x64

.text
.globl _sys_read
.globl _sys_write
.globl _sys_open
.globl _sys_close
.globl _sys_exit
.globl _start

# System call numbers
.equ SYS_READ,    0
.equ SYS_WRITE,   1
.equ SYS_OPEN,    2
.equ SYS_CLOSE,   3
.equ SYS_EXIT,    60

# int _sys_read(int fd, char *buf, int count)
_sys_read:
    movl    $SYS_READ, %eax
    syscall
    ret

# int _sys_write(int fd, char *buf, int count)
_sys_write:
    movl    $SYS_WRITE, %eax
    syscall
    ret

# int _sys_open(char *path, int flags, int mode)
_sys_open:
    movl    $SYS_OPEN, %eax
    syscall
    ret

# int _sys_close(int fd)
_sys_close:
    movl    $SYS_CLOSE, %eax
    syscall
    ret

# void _sys_exit(int code)
_sys_exit:
    movl    $SYS_EXIT, %eax
    syscall

# Program entry point
_start:
    # Call main
    call    main
    
    # Exit with return value
    movl    %eax, %edi
    movl    $SYS_EXIT, %eax
    syscall
