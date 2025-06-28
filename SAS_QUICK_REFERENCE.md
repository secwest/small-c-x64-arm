# SAS Enhanced Quick Reference

## Command Line Usage

```bash
# Assemble
sas_enhanced -arch x64 -o output.o input.s    # Specify architecture
sas_enhanced -o output.o input.s              # Auto-detect from .arch

# Link
sld_sas -o program file1.o file2.o           # Linux ELF output
sld_sas -target windows -o prog.exe file.o    # Windows PE output
```

## Essential Directives

### Architecture
```asm
.arch x64       ; or x86_64, amd64
.arch arm64     ; or aarch64
```

### Sections
```asm
.text           ; Code section
.data           ; Initialized data
.bss            ; Uninitialized data
.section name   ; Custom section
```

### Symbols
```asm
.global symbol  ; Export symbol
.extern symbol  ; Import symbol
label:          ; Define label
.label:         ; Local label
```

### Constants
```asm
.equ NAME, value    ; Define constant
.set NAME, value    ; Alias for .equ
NAME = value        ; Some assemblers support this
```

### Data Definition
```asm
.byte   1, 2, 3, 'A'           ; 8-bit values
.word   0x1234, -1             ; 16-bit values  
.dword  0x12345678, symbol     ; 32-bit values
.quad   0x123456789ABCDEF0     ; 64-bit values
.ascii  "raw string"           ; String without \0
.asciz  "C string"             ; String with \0
.string "C string"             ; Alias for .asciz
.space  count [, fill]         ; Reserve bytes
.align  boundary               ; Align to boundary
```

## x64 Instructions

### Data Movement
```asm
mov dst, src    ; Move data
push reg        ; Push to stack
pop reg         ; Pop from stack
lea reg, [addr] ; Load effective address
```

### Arithmetic
```asm
add dst, src    ; Addition
sub dst, src    ; Subtraction
imul dst, src   ; Signed multiply
xor dst, src    ; Exclusive OR
```

### Control Flow
```asm
call target     ; Call function
jmp target      ; Unconditional jump
ret             ; Return
jz/jnz target   ; Conditional jumps
```

### System
```asm
syscall         ; System call (Linux)
int 0x80        ; Interrupt (legacy)
```

## ARM64 Instructions

### Data Movement
```asm
mov dst, src           ; Move register/immediate
ldr dst, [base]        ; Load from memory
str src, [base]        ; Store to memory
adr reg, label         ; Address of label
```

### Arithmetic
```asm
add dst, src1, src2    ; dst = src1 + src2
sub dst, src1, src2    ; dst = src1 - src2
mul dst, src1, src2    ; dst = src1 * src2
```

### Control Flow
```asm
b target              ; Branch
bl target             ; Branch with link
ret                   ; Return
cbz reg, target       ; Branch if zero
cbnz reg, target      ; Branch if not zero
```

### System
```asm
svc #0                ; Supervisor call
```

## Expression Operators

```asm
; Arithmetic
value + 10            ; Addition
value - 10            ; Subtraction  
value * 10            ; Multiplication
value / 10            ; Division
value % 10            ; Modulo

; Bitwise
value & mask          ; AND
value | mask          ; OR
value ^ mask          ; XOR
~value                ; NOT
value << bits         ; Left shift
value >> bits         ; Right shift

; Special
$                     ; Current position
(expression)          ; Grouping
```

## Common Patterns

### Linux x64 System Call
```asm
mov rax, 1            ; sys_write
mov rdi, 1            ; stdout
mov rsi, buffer       ; data
mov rdx, length       ; size
syscall
```

### Linux ARM64 System Call
```asm
mov x0, #1            ; stdout
adr x1, buffer        ; data
mov x2, #length       ; size
mov x8, #64           ; sys_write
svc 0
```

### Function Prologue/Epilogue (x64)
```asm
function:
    push rbp
    mov rbp, rsp
    ; ... function body ...
    pop rbp
    ret
```

### Function Prologue/Epilogue (ARM64)
```asm
function:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    ; ... function body ...
    ldp x29, x30, [sp], #16
    ret
```

### Data Section Example
```asm
.data
.align 8
table:      .dword 1, 2, 3, 4, 5
table_end:
table_size: .equ (table_end - table) / 4

message:    .asciz "Hello, World!\n"
msg_len:    .equ $ - message
```

### BSS Section Example
```asm
.bss
.align 16
buffer:     .space 4096
counter:    .space 8
flags:      .space 4
```

## Register Names

### x64 Registers
- **64-bit**: rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8-r15
- **32-bit**: eax, ebx, ecx, edx, esi, edi, ebp, esp, r8d-r15d
- **16-bit**: ax, bx, cx, dx, si, di, bp, sp, r8w-r15w
- **8-bit**: al, bl, cl, dl, sil, dil, bpl, spl, r8b-r15b

### ARM64 Registers
- **64-bit**: x0-x30, sp, xzr
- **32-bit**: w0-w30, wsp, wzr
- **Special**: fp (x29), lr (x30), sp (x31/sp)

## Tips and Tricks

1. **Use meaningful constants**:
   ```asm
   .equ BUFFER_SIZE, 4096
   .equ MAX_ITEMS, 100
   ```

2. **Comment your code**:
   ```asm
   mov rax, 60     ; sys_exit
   ```

3. **Align data for performance**:
   ```asm
   .align 16
   large_buffer: .space 1024
   ```

4. **Use local labels in functions**:
   ```asm
   function:
   .loop:
       ; ... code ...
       jnz .loop
       ret
   ```

5. **Organize with sections**:
   ```asm
   .section .rodata    ; Read-only data
   constants: .dword 1, 2, 3
   ```
