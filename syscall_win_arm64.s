// Small-C Runtime - Windows ARM64 System Calls
// Windows ARM64 calling convention:
// - Parameters in x0-x7
// - Return value in x0
// - x19-x28 must be preserved
// - Stack must be 16-byte aligned

.text
.globl _sys_read
.globl _sys_write
.globl _sys_open
.globl _sys_close
.globl _sys_exit
.globl _start

// Import Windows API functions
.extern GetStdHandle
.extern ReadFile
.extern WriteFile
.extern CreateFileA
.extern CloseHandle
.extern ExitProcess

// Standard handle constants
.equ STD_INPUT_HANDLE,  -10
.equ STD_OUTPUT_HANDLE, -11
.equ STD_ERROR_HANDLE,  -12

// File access modes
.equ GENERIC_READ,    0x80000000
.equ GENERIC_WRITE,   0x40000000
.equ CREATE_ALWAYS,   2
.equ OPEN_EXISTING,   3
.equ OPEN_ALWAYS,     4
.equ FILE_ATTRIBUTE_NORMAL, 0x80

// int _sys_read(int fd, char *buf, int count)
_sys_read:
    // Save registers
    stp     x29, x30, [sp, #-48]!
    mov     x29, sp
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    
    // Save arguments
    mov     x19, x0     // fd
    mov     x20, x1     // buffer
    mov     x21, x2     // count
    
    // If fd is 0, get stdin handle
    cbnz    x19, .read_file
    mov     x0, #STD_INPUT_HANDLE
    bl      GetStdHandle
    mov     x19, x0
    
.read_file:
    // Allocate space for bytesRead
    sub     sp, sp, #16
    
    // ReadFile(handle, buffer, count, &bytesRead, NULL)
    mov     x0, x19     // handle
    mov     x1, x20     // buffer
    mov     x2, x21     // count
    mov     x3, sp      // &bytesRead
    mov     x4, #0      // lpOverlapped = NULL
    bl      ReadFile
    
    // Get bytes read
    ldr     w0, [sp]
    
    // Restore stack
    add     sp, sp, #16
    
    // Restore registers
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x29, x30, [sp], #48
    ret

// int _sys_write(int fd, char *buf, int count)
_sys_write:
    // Save registers
    stp     x29, x30, [sp, #-48]!
    mov     x29, sp
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    
    // Save arguments
    mov     x19, x0     // fd
    mov     x20, x1     // buffer
    mov     x21, x2     // count
    
    // If fd is 1 or 2, get stdout/stderr handle
    cmp     x19, #1
    b.eq    .get_stdout
    cmp     x19, #2
    b.eq    .get_stderr
    b       .write_file
    
.get_stdout:
    mov     x0, #STD_OUTPUT_HANDLE
    bl      GetStdHandle
    mov     x19, x0
    b       .write_file
    
.get_stderr:
    mov     x0, #STD_ERROR_HANDLE
    bl      GetStdHandle
    mov     x19, x0
    
.write_file:
    // Allocate space for bytesWritten
    sub     sp, sp, #16
    
    // WriteFile(handle, buffer, count, &bytesWritten, NULL)
    mov     x0, x19     // handle
    mov     x1, x20     // buffer
    mov     x2, x21     // count
    mov     x3, sp      // &bytesWritten
    mov     x4, #0      // lpOverlapped = NULL
    bl      WriteFile
    
    // Get bytes written
    ldr     w0, [sp]
    
    // Restore stack
    add     sp, sp, #16
    
    // Restore registers
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x29, x30, [sp], #48
    ret

// int _sys_open(char *path, int flags, int mode)
_sys_open:
    // Save registers
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    stp     x19, x20, [sp, #16]
    
    // Save arguments
    mov     x19, x0     // path
    mov     x20, x1     // flags
    
    // Convert flags to Windows access mode
    mov     x1, #GENERIC_READ
    tst     x20, #1     // O_WRONLY
    b.eq    .check_rdwr
    mov     x1, #GENERIC_WRITE
    
.check_rdwr:
    tst     x20, #2     // O_RDWR
    b.eq    .set_creation
    movk    x1, #(GENERIC_READ >> 16), lsl #16
    
.set_creation:
    // Set creation disposition
    mov     x4, #OPEN_EXISTING
    tst     x20, #0x40  // O_CREAT
    b.eq    .call_create
    mov     x4, #OPEN_ALWAYS
    tst     x20, #0x200 // O_TRUNC
    b.eq    .call_create
    mov     x4, #CREATE_ALWAYS
    
.call_create:
    // CreateFileA(filename, access, share, security, creation, flags, template)
    mov     x0, x19                     // lpFileName
    // x1 already set               // dwDesiredAccess
    mov     x2, #0                      // dwShareMode
    mov     x3, #0                      // lpSecurityAttributes
    // x4 already set               // dwCreationDisposition
    mov     x5, #FILE_ATTRIBUTE_NORMAL  // dwFlagsAndAttributes
    mov     x6, #0                      // hTemplateFile
    bl      CreateFileA
    
    // Check for INVALID_HANDLE_VALUE (-1)
    cmn     x0, #1
    b.ne    .open_success
    mov     x0, #-1
    
.open_success:
    // Restore registers
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #32
    ret

// int _sys_close(int fd)
_sys_close:
    // Windows ARM64 preserves x0 across calls, so just jump
    b       CloseHandle

// void _sys_exit(int code)
_sys_exit:
    // ExitProcess never returns, so just jump
    b       ExitProcess

// Program entry point
_start:
    // Set up stack frame
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    
    // Ensure stack alignment
    and     sp, sp, #~15
    
    // Call main
    bl      main
    
    // Exit with return value
    // x0 already contains return value from main
    bl      ExitProcess
    
    // Should never reach here
.loop:
    b       .loop
