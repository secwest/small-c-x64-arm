# Small-C Runtime - Windows x64 System Calls
# Uses Windows API functions via import libraries

.text
.globl _sys_read
.globl _sys_write
.globl _sys_open
.globl _sys_close
.globl _sys_exit
.globl _start

# Import Windows API functions
.extern GetStdHandle
.extern ReadFile
.extern WriteFile
.extern CreateFileA
.extern CloseHandle
.extern ExitProcess

# Standard handle constants
.equ STD_INPUT_HANDLE,  -10
.equ STD_OUTPUT_HANDLE, -11
.equ STD_ERROR_HANDLE,  -12

# File access modes
.equ GENERIC_READ,    0x80000000
.equ GENERIC_WRITE,   0x40000000
.equ CREATE_ALWAYS,   2
.equ OPEN_EXISTING,   3
.equ OPEN_ALWAYS,     4

# int _sys_read(int fd, char *buf, int count)
_sys_read:
    push    %rbp
    mov     %rsp, %rbp
    sub     $48, %rsp
    
    # If fd is 0, get stdin handle
    cmp     $0, %rcx
    jne     .read_file
    mov     $STD_INPUT_HANDLE, %rcx
    call    GetStdHandle
    mov     %rax, %rcx
    
.read_file:
    # ReadFile(handle, buffer, count, &bytesRead, NULL)
    mov     %rcx, %rcx          # handle
    mov     %rdx, %rdx          # buffer
    mov     %r8d, %r8d          # count
    lea     -8(%rbp), %r9       # &bytesRead
    push    $0                  # lpOverlapped = NULL
    call    ReadFile
    
    # Return bytes read
    mov     -8(%rbp), %eax
    
    leave
    ret

# int _sys_write(int fd, char *buf, int count)
_sys_write:
    push    %rbp
    mov     %rsp, %rbp
    sub     $48, %rsp
    
    # If fd is 1, get stdout handle
    cmp     $1, %rcx
    jne     .write_file
    mov     $STD_OUTPUT_HANDLE, %rcx
    call    GetStdHandle
    mov     %rax, %rcx
    
.write_file:
    # WriteFile(handle, buffer, count, &bytesWritten, NULL)
    mov     %rcx, %rcx          # handle
    mov     %rdx, %rdx          # buffer
    mov     %r8d, %r8d          # count
    lea     -8(%rbp), %r9       # &bytesWritten
    push    $0                  # lpOverlapped = NULL
    call    WriteFile
    
    # Return bytes written
    mov     -8(%rbp), %eax
    
    leave
    ret

# int _sys_open(char *path, int flags, int mode)
_sys_open:
    push    %rbp
    mov     %rsp, %rbp
    sub     $64, %rsp
    
    # Convert flags to Windows access mode
    mov     $GENERIC_READ, %rdx
    test    $1, %rsi            # O_WRONLY
    jz      .open_readonly
    mov     $GENERIC_WRITE, %rdx
    test    $2, %rsi            # O_RDWR
    jz      .open_writeonly
    or      $GENERIC_READ, %rdx
    
.open_writeonly:
.open_readonly:
    # CreateFileA(filename, access, share, security, creation, flags, template)
    mov     %rcx, %rcx          # lpFileName
    # %rdx already set       # dwDesiredAccess
    mov     $0, %r8             # dwShareMode
    mov     $0, %r9             # lpSecurityAttributes
    
    # Set creation disposition
    mov     $OPEN_EXISTING, %rax
    test    $0100, %rsi         # O_CREAT
    jz      .set_disp
    mov     $OPEN_ALWAYS, %rax
    test    $01000, %rsi        # O_TRUNC
    jz      .set_disp
    mov     $CREATE_ALWAYS, %rax
    
.set_disp:
    push    $0                  # hTemplateFile
    push    $0x80               # dwFlagsAndAttributes (FILE_ATTRIBUTE_NORMAL)
    push    %rax                # dwCreationDisposition
    call    CreateFileA
    
    leave
    ret

# int _sys_close(int fd)
_sys_close:
    # CloseHandle(handle)
    jmp     CloseHandle

# void _sys_exit(int code)
_sys_exit:
    # ExitProcess(code)
    jmp     ExitProcess

# Program entry point
_start:
    sub     $32, %rsp           # Shadow space for Win64 ABI
    
    # Call main
    call    main
    
    # Exit with return value
    mov     %eax, %ecx
    call    ExitProcess
