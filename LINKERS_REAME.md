# Small-C Linkers Documentation

This repository contains minimal linkers for the Small-C compiler that can produce executables for Linux and Windows on both x64 and ARM64 architectures.

## Overview

These linkers are designed to:
1. Be compilable by the Small-C compiler itself (self-hosting)
2. Create static executables from Small-C's assembly output
3. Support both x64 and ARM64 architectures
4. Work on Linux (ELF) and Windows (PE) platforms

## Linker Variants

### Linux Linkers

1. **sld_linux_x64.c** - Basic ELF linker for x64 Linux
   - Minimal implementation (~400 lines)
   - Handles basic x64 relocations
   - Creates static ELF executables

2. **sld_linux_arm64.c** - Basic ELF linker for ARM64 Linux
   - Similar to x64 version but for ARM64
   - Handles ARM64-specific relocations
   - Creates static ARM64 ELF executables

3. **sld_enhanced.c** - Enhanced ELF linker
   - Auto-detects architecture from object files
   - Supports both x64 and ARM64 in one binary
   - Better section handling (including BSS)
   - More relocation types supported

### Windows Linkers

4. **sld_win_x64.c** - Basic PE linker for x64 Windows
   - Creates Windows PE executables
   - Handles COFF object files
   - Supports console and GUI applications

5. **sld_win_arm64.c** - Basic PE linker for ARM64 Windows
   - Windows on ARM support
   - Handles ARM64 COFF relocations

6. **sld_win_enhanced.c** - Enhanced PE linker
   - Auto-detects x64 or ARM64
   - Better section merging
   - Automatic subsystem detection

## Building the Linkers

### On Linux

```bash
# Make the build script executable
chmod +x build_linkers.sh

# Run the build script
./build_linkers.sh

# Or manually:
./scc sld_linux_x64.c > sld.s
as sld.s -o sld.o
ld -static syscall_linux_x64.o runtime.o sld.o -o sld
```

### On Windows

```batch
REM Use the batch file
build_linkers.bat

REM Or manually with MinGW:
scc.exe sld_win_x64.c > sld_win_x64.s
as sld_win_x64.s -o sld_win_x64.o
ld sld_win_x64.o syscall_win_x64.o runtime.o -o sld.exe
```

## Usage

### Linux
```bash
# Basic usage
./sld -o myprogram syscall_linux_x64.o runtime.o myprogram.o

# Multiple object files
./sld -o myapp syscall_linux_x64.o runtime.o main.o utils.o data.o
```

### Windows
```batch
REM Basic usage
sld.exe -o myprogram.exe syscall_win_x64.obj runtime.obj myprogram.obj

REM Specify subsystem (enhanced version)
sld_win_enhanced.exe -o myapp.exe -subsystem:windows main.obj gui.obj
```

## Linking GNU ld/GCC Output

### Can sld_enhanced link GNU ld/gcc output?

**Short answer:** Limited support only.

**Long answer:** The enhanced linkers can handle simple GNU-generated object files with these limitations:

1. **Static linking only** - No dynamic library support
2. **Simple C code only** - No C++, exceptions, or complex features
3. **Basic relocations only** - Limited relocation types supported
4. **No debug info** - DWARF and other debug formats ignored
5. **No TLS** - Thread-local storage not supported

### What works:
- Simple C programs compiled with: `gcc -c -static -fno-pic -O0 simple.c`
- Basic data types and functions
- Simple global variables
- Basic arithmetic and control flow

### What doesn't work:
- Dynamic libraries (.so/.dll)
- Position-independent code (-fPIC)
- C++ features (vtables, RTTI, exceptions)
- Thread-local storage
- Complex optimizations
- Most standard library functions

### Example of compatible GCC usage:
```bash
# Compile simple C code with GCC
gcc -c -static -fno-pic -fno-stack-protector -ffreestanding -nostdlib simple.c

# Link with sld_enhanced
./sld_enhanced -o simple simple.o
```

## Architecture Support Summary

| Linker | Linux x64 | Linux ARM64 | Windows x64 | Windows ARM64 |
|--------|-----------|-------------|-------------|---------------|
| sld_linux_x64 | ✓ | - | - | - |
| sld_linux_arm64 | - | ✓ | - | - |
| sld_enhanced | ✓ | ✓ | - | - |
| sld_win_x64 | - | - | ✓ | - |
| sld_win_arm64 | - | - | - | ✓ |
| sld_win_enhanced | - | - | ✓ | ✓ |

## Technical Details

### ELF Structure (Linux)
- Minimal ELF header
- Two PT_LOAD segments (code and data)
- Simple virtual memory layout starting at 0x400000
- Basic symbol resolution
- Limited relocation support

### PE Structure (Windows)
- Standard DOS stub and PE headers
- Automatic section merging
- Standard Windows base address (0x140000000)
- Console and GUI subsystem support
- COFF symbol table parsing

### Limitations
1. No dynamic linking
2. No shared libraries
3. No debug information
4. Limited relocation types
5. No optimization
6. No incremental linking
7. Fixed memory layout

## Why These Limitations?

These linkers are designed to work within Small-C's constraints:
- No structs (makes complex data structures difficult)
- No preprocessor (no conditional compilation)
- Limited array support (single dimension only)
- No function pointers in structures
- Basic type system

The goal is a self-hosting toolchain that can bootstrap itself, not compete with professional linkers like GNU ld or MSVC's link.exe.

## Future Improvements

Possible enhancements while maintaining Small-C compatibility:
1. Archive (.a/.lib) support for static libraries
2. Better error messages
3. Symbol map generation
4. Size optimizations
5. Basic dead code elimination

## Contributing

When modifying these linkers, ensure they remain compilable by Small-C:
- Use only features supported by Small-C
- Keep functions simple and focused
- Avoid complex expressions
- Test self-hosting capability
