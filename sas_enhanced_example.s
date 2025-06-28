; sas_enhanced_examples.s - Example assembly files for sas_enhanced

; ============================================================================
; Example 1: Hello World for x64 Linux
; ============================================================================
.arch x64
.text
.global _start

_start:
    ; write(1, msg, len)
    mov rax, 1          ; sys_write
    mov rdi, 1          ; stdout
    mov rsi, msg        ; buffer
    mov rdx, msglen     ; length
    syscall
    
    ; exit(0)
    mov rax, 60         ; sys_exit
    xor rdi, rdi        ; status = 0
    syscall

.data
msg:    .asciz "Hello, World from sas_enhanced!\n"
msglen  .equ $ - msg

; ============================================================================
; Example 2: Hello World for ARM64 Linux
; ============================================================================
.arch arm64
.text
.global _start

_start:
    ; write(1, msg, len)
    mov x0, #1          ; stdout
    adr x1, msg         ; buffer address
    mov x2, msglen      ; length
    mov x8, #64         ; sys_write
    svc 0
    
    ; exit(0)
    mov x0, #0          ; status
    mov x8, #93         ; sys_exit
    svc 0

.data
msg:    .asciz "Hello, World from ARM64!\n"
msglen  .equ 26

; ============================================================================
; Example 3: Simple function example (x64)
; ============================================================================
.arch x64
.text
.global add_numbers
.global multiply

add_numbers:
    ; int add_numbers(int a, int b)
    ; Arguments in rdi, rsi
    mov rax, rdi
    add rax, rsi
    ret

multiply:
    ; int multiply(int a, int b)
    mov rax, rdi
    imul rax, rsi
    ret

; ============================================================================
; Example 4: Data types and directives
; ============================================================================
.data
.align 8

byte_data:      .byte   0x41, 0x42, 0x43, 0x44    ; 'ABCD'
word_data:      .word   0x1234, 0x5678
dword_data:     .dword  0x12345678, 0xABCDEF00
qword_data:     .quad   0x123456789ABCDEF0

string1:        .ascii  "This is ASCII text"
string2:        .asciz  "This is null-terminated"
string3:        .string "Also null-terminated"

buffer:         .space  256     ; 256 bytes of zeros
padding:        .skip   16, 0xFF ; 16 bytes of 0xFF

; ============================================================================
; Example 5: Expressions and symbols
; ============================================================================
.equ BUFFER_SIZE, 4096
.equ FLAG_READ,   0x01
.equ FLAG_WRITE,  0x02
.equ FLAG_EXEC,   0x04

.set MAX_COUNT, 100
.set MIN_COUNT, 10

.data
array_size:     .dword  MAX_COUNT * 4      ; 400
flags:          .byte   FLAG_READ | FLAG_WRITE  ; 0x03
offset:         .word   end_label - start_label

.text
start_label:
    nop
    nop
    nop
end_label:

; ============================================================================
; Example 6: Conditional assembly with expressions
; ============================================================================
.data
; Using expressions in .space directive
dynamic_buffer: .space  BUFFER_SIZE / 8

; Complex expressions
value1:         .dword  (1 << 16) | 0xFF
value2:         .dword  ~0x12345678
value3:         .dword  (MAX_COUNT - MIN_COUNT) * 4

; ============================================================================
; Example 7: Windows x64 example
; ============================================================================
.arch x64
.text
.global mainCRTStartup

mainCRTStartup:
    ; Align stack
    sub rsp, 40        ; 32 bytes shadow space + 8 for alignment
    
    ; GetStdHandle(STD_OUTPUT_HANDLE)
    mov rcx, -11       ; STD_OUTPUT_HANDLE
    call GetStdHandle
    mov rbx, rax       ; Save handle
    
    ; WriteFile(handle, buffer, length, &written, NULL)
    mov rcx, rbx       ; Handle
    lea rdx, [win_msg] ; Buffer
    mov r8, win_msglen ; Length
    lea r9, [written]  ; Bytes written
    push 0             ; lpOverlapped = NULL
    sub rsp, 32        ; Shadow space
    call WriteFile
    add rsp, 40
    
    ; ExitProcess(0)
    xor rcx, rcx
    call ExitProcess
    
.data
win_msg:        .asciz "Hello from Windows x64!\r\n"
win_msglen      .equ $ - win_msg
written:        .dword 0

; ============================================================================
; Example 8: ARM64 function with local variables
; ============================================================================
.arch arm64
.text
.global factorial

factorial:
    ; int factorial(int n) - in x0
    ; Prologue
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    
    ; Base case: if (n <= 1) return 1
    cmp x0, #1
    bgt recursive_case
    mov x0, #1
    b factorial_return
    
recursive_case:
    ; Save n
    stp x19, x20, [sp, #-16]!
    mov x19, x0
    
    ; Call factorial(n-1)
    sub x0, x0, #1
    bl factorial
    
    ; result = n * factorial(n-1)
    mul x0, x0, x19
    
    ; Restore saved registers
    ldp x19, x20, [sp], #16
    
factorial_return:
    ; Epilogue
    ldp x29, x30, [sp], #16
    ret

; ============================================================================
; Example 9: Macros and advanced features (if implemented)
; ============================================================================
.text
; Simple delay loop
delay_loop:
    mov rcx, 1000000
.loop:
    dec rcx
    jnz .loop
    ret

; Local labels example
process_data:
    mov rcx, 10
.next_item:
    ; Process item
    nop
    dec rcx
    jnz .next_item
    ret

; ============================================================================
; Example 10: BSS section
; ============================================================================
.bss
.align 16

input_buffer:   .space  1024
output_buffer:  .space  1024
temp_storage:   .space  256
counter:        .space  8

; ============================================================================
; Build instructions:
; ============================================================================
; For x64 Linux:
;   sas_enhanced -arch x64 -o hello_x64.o hello_x64.s
;   sld_sas -o hello_x64 hello_x64.o
;
; For ARM64 Linux:
;   sas_enhanced -arch arm64 -o hello_a
