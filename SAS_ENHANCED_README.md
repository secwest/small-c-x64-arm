# SAS Enhanced - General Purpose Assembler for Small-C

A feature-rich assembler and linker system that can be compiled by Small-C, supporting both x64 and ARM64 architectures for Linux and Windows.

## Overview

SAS Enhanced is a significant upgrade from basic assemblers, providing:
- Support for x64 and ARM64 instruction sets
- Multiple output formats (ELF and PE)
- Expression evaluation in operands
- Symbol and relocation management
- Section control directives
- Macro-like features (through .equ and .set)
- Compatible with Small-C's language constraints

## Components

### 1. **sas_enhanced.c** - The Assembler
- Processes assembly source files
- Generates object files in SAS format
- Supports both architectures in one binary

### 2. **sld_sas.c** - The Linker
- Links SAS object files
- Produces ELF (Linux) or PE (Windows) executables
- Handles relocations and symbol resolution

## Building

```bash
# Build the assembler
./scc sas_enhanced.c > sas_enhanced.s
as sas_enhanced.s -o sas_enhanced.o
ld -static syscall_linux_x64.o runtime.o sas_enhanced.o -o sas_enhanced

# Build the linker
./scc sld_sas.c > sld_sas.s
as sld_sas.s -o sld_sas.o
ld -static syscall_linux_x64.o runtime.o sld_sas.o -o sld_sas
```

## Usage

### Assembling
```bash
# For x64
sas_enhanced -arch x64 -o output.o input.s

# For ARM64
sas_enhanced -arch arm64 -o output.o input.s

# Architecture auto-detection from .arch directive
sas_enhanced -o output.o input.s
```

### Linking
```bash
# Linux executable
sld_sas -o program file1.o file2.o

# Windows executable
sld_sas -target windows -o program.exe file1.o file2.o
```

## Supported Directives

### Architecture Selection
- `.arch x64` or `.arch x86_64` - Target x64
- `.arch arm64` or `.arch aarch64` - Target ARM64

### Section Control
- `.text` - Code section
- `.data` - Initialized data section
- `.bss` - Uninitialized data section
- `.section name` - Custom section

### Symbol Management
- `.global symbol` or `.globl symbol` - Export symbol
- `.extern symbol` - Import external symbol
- `.equ symbol, value` - Define constant
- `.set symbol, value` - Define constant (alias for .equ)

### Data Definition
- `.byte values...` or `.db` - 8-bit values
- `.word values...` or `.dw` - 16-bit values
- `.dword values...` or `.dd` or `.long` - 32-bit values
- `.quad values...` or `.dq` - 64-bit values
- `.ascii "string"` - String without null terminator
- `.asciz "string"` or `.string` - Null-terminated string
- `.space count [, fill]` or `.skip` - Reserve bytes
- `.align boundary` - Align to boundary

### Labels
- `label:` - Define a label
- `.label` - Local label (within function)

## Expression Evaluation

The assembler supports complex expressions in operands:

### Operators (in precedence order)
1. Primary: numbers, symbols, `$` (current position), parentheses
2. Unary: `-` (negation), `~` (bitwise NOT)
3. Multiplicative: `*`, `/`, `%`
4. Shift: `<<`, `>>`
5. Bitwise: `&`, `|`, `^`
6. Additive: `+`, `-`

### Examples
```asm
.equ BUFFER_SIZE, 4096
.equ FLAGS, 0x01 | 0x02 | 0x04

mov rax, BUFFER_SIZE / 8        ; 512
mov rbx, (1 << 16) - 1         ; 65535
lea rcx, [data + BUFFER_SIZE]  ; address calculation
.space BUFFER_SIZE * 2          ; 8192 bytes
```

## Instruction Support

### x64 Instructions
Basic instruction set including:
- **Data Movement**: `mov`, `push`, `pop`
- **Arithmetic**: `add`, `sub`, `imul`, `xor`
- **Control Flow**: `call`, `jmp`, `ret`
- **System**: `syscall`, `int`
- **Misc**: `nop`

### ARM64 Instructions
- **Data Movement**: `mov`, `ldr`, `str`
- **Arithmetic**: `add`, `sub`, `mul`
- **Control Flow**: `b`, `bl`, `ret`
- **System**: `svc`
- **Misc**: `nop`

### Adding New Instructions

To add new instructions, modify the `encode_x64_*` or `encode_arm64_*` functions:

```c
// Example: Adding x64 CMP instruction
void encode_x64_cmp(char *op1, char *op2) {
    // REX prefix if needed
    emit_byte(0x48);
    // CMP opcode
    emit_byte(0x39);
    // ModR/M byte
    emit_byte(0xC0 | (src_reg << 3) | dst_reg);
}
```

## Object File Format

SAS Enhanced uses a simple object format:

```
Header:
  Magic:      "SAS\0" (4 bytes)
  Arch:       0=x64, 1=ARM64 (4 bytes)
  Sections:   Count (4 bytes)
  Symbols:    Count (4 bytes)
  Relocs:     Count (4 bytes)

Sections:
  Name:       128 bytes
  Size:       4 bytes
  Flags:      4 bytes
  Align:      4 bytes
  Data:       variable

Symbols:
  Name:       128 bytes
  Value:      4 bytes
  Section:    4 bytes
  Type:       4 bytes
  Defined:    4 bytes

Relocations:
  Offset:     4 bytes
  Symbol:     4 bytes
  Type:       4 bytes
  Section:    4 bytes
  Addend:     4 bytes
```

## Example Programs

### 1. Hello World (x64 Linux)
```asm
.arch x64
.text
.global _start

_start:
    mov rax, 1          ; sys_write
    mov rdi, 1          ; stdout
    mov rsi, msg        ; buffer
    mov rdx, 13         ; length
    syscall
    
    mov rax, 60         ; sys_exit
    xor rdi, rdi
    syscall

.data
msg: .ascii "Hello World!\n"
```

### 2. Function Example (ARM64)
```asm
.arch arm64
.text
.global add_numbers

add_numbers:
    ; Arguments in x0, x1
    add x0, x0, x1      ; result = a + b
    ret
```

### 3. Windows Console App (x64)
```asm
.arch x64
.text
.global mainCRTStartup
.extern GetStdHandle
.extern WriteFile
.extern ExitProcess

mainCRTStartup:
    sub rsp, 40
    
    mov rcx, -11
    call GetStdHandle
    mov rbx, rax
    
    mov rcx, rbx
    lea rdx, [message]
    mov r8, msg_len
    lea r9, [written]
    push 0
    sub rsp, 32
    call WriteFile
    add rsp, 40
    
    xor rcx, rcx
    call ExitProcess

.data
message: .asciz "Hello from Windows!\r\n"
msg_len .equ $ - message
written: .dword 0
```

## Limitations

Due to Small-C constraints:
1. **No preprocessor** - No #include, #define, or conditional assembly
2. **Limited string handling** - Basic string operations only
3. **No structs** - All data stored in parallel arrays
4. **Fixed limits** - Maximum symbols, sections, etc. are compile-time constants
5. **Basic error messages** - Limited diagnostic information
6. **No optimization** - Instructions encoded as-is

## Extending the Assembler

### Adding New Directives
1. Add handler in `process_directive()`
2. Implement directive logic
3. Update documentation

### Adding New Instructions
1. Add encoding function (e.g., `encode_x64_newinsn()`)
2. Add case in `process_instruction()`
3. Handle operand parsing
4. Test with example code

### Supporting New Architectures
1. Add architecture detection in `.arch` directive
2. Create encoding functions for the architecture
3. Add relocation types
4. Update linker to handle new relocations

## Comparison with GNU as

| Feature | SAS Enhanced | GNU as |
|---------|-------------|---------|
| Architectures | x64, ARM64 | 30+ architectures |
| Directives | ~20 basic | 100+ directives |
| Macros | Basic (.equ) | Full macro processor |
| Expressions | Basic math | Complex expressions |
| Debug info | None | Full DWARF support |
| Optimization | None | Instruction selection |
| Size | ~2000 lines | ~1M+ lines |
| Dependencies | None | Multiple libraries |

## Why Use SAS Enhanced?

1. **Educational** - Understand how assemblers work
2. **Self-hosting** - Can be built with Small-C
3. **Minimal dependencies** - No external libraries
4. **Portable** - Simple C code
5. **Hackable** - Easy to modify and extend

## Future Improvements

Possible enhancements while maintaining Small-C compatibility:
1. More instructions (currently ~10-20% coverage)
2. Better error messages with line numbers
3. Listing file generation
4. Symbol map output
5. Basic macro support (if feasible)
6. Limited preprocessor directives
7. More addressing modes

## Contributing

When modifying sas_enhanced:
- Ensure all code compiles with Small-C
- Test self-hosting capability
- Keep functions simple (no complex expressions)
- Document new features
- Add example code
