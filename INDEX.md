# Small-C Compiler - Complete File Index

## Overview
This is the complete file listing for the Modern Small-C Compiler project, supporting both Linux and Windows on x64 and ARM64 architectures.

## Core Compiler Files

### 1. **scc.c** - Basic Small-C Compiler
- **Code ID**: `modern-small-c`
- **Description**: The basic Small-C compiler (~900 lines) in the spirit of Ron Cain's original
- **Features**: Minimal C subset, self-bootstrapping, direct assembly generation
- **Language support**: int, char, pointers, arrays, basic control flow

### 2. **scc_enhanced.c** - Enhanced Small-C Compiler
- **Code ID**: `smallc-enhanced`
- **Description**: Enhanced version (~1800 lines) with modern features
- **Features**: Function parameters, local initialization, character literals, compound assignments, comments
- **Additional**: Better error reporting with line numbers

## Runtime Library

### 3. **runtime.c** - Universal Runtime Library
- **Code ID**: `smallc-runtime`
- **Description**: Portable runtime library for all platforms
- **Functions**: 
  - I/O: putchar, getchar, puts, gets, printf
  - Files: open, creat, close, read, write
  - Strings: strlen, strcmp, strcpy
  - Memory: memset, memcpy
  - Utilities: atoi, abs, min, max, exit

## Platform-Specific System Calls

### 4. **syscall_linux_x64.s** - Linux x64 System Calls
- **Code ID**: `syscall-linux-x64`
- **Description**: Linux x64 system call wrappers and program entry point
- **Implements**: _sys_read, _sys_write, _sys_open, _sys_close, _sys_exit, _start

### 5. **syscall_linux_arm64.s** - Linux ARM64 System Calls
- **Code ID**: `syscall-linux-arm64`
- **Description**: Linux ARM64 system call wrappers and program entry point
- **Implements**: Same as x64 but for ARM64 architecture

### 6. **syscall_win_x64.s** - Windows x64 System Calls
- **Code ID**: `syscall-win-x64`
- **Description**: Windows x64 API wrappers using Win32 functions
- **Implements**: Windows API calls (ReadFile, WriteFile, CreateFileA, etc.)

### 7. **syscall_win_arm64.s** - Windows ARM64 System Calls
- **Code ID**: `syscall-win-arm64`
- **Description**: Windows ARM64 API wrappers (fully implemented)
- **Implements**: Same Windows APIs for ARM64 architecture

## Build System

### 8. **Makefile** - Cross-Platform Build Configuration
- **Code ID**: `smallc-makefile`
- **Description**: Automatic platform detection and building
- **Supports**: Linux/Windows, x64/ARM64 auto-detection

### 9. **scc-build.sh** - Build Script
- **Code ID**: `smallc-build`
- **Description**: Convenient shell script for building Small-C programs
- **Usage**: `./scc-build.sh program.c [output_name]`

## Documentation

### 10. **README.md** - Main Documentation
- **Code ID**: `smallc-readme`
- **Description**: Complete project overview, features, and usage instructions
- **Contents**: Introduction, features, building, usage, examples

### 11. **BUILD_GUIDE.md** - Detailed Build Guide
- **Code ID**: `smallc-buildguide`
- **Description**: Comprehensive build and testing instructions
- **Contents**: Build steps, self-bootstrapping, cross-platform building, debugging

### 12. **HISTORY.md** - Small-C History and Evolution
- **Code ID**: `smallc-history`
- **Description**: Complete history of Small-C from 1980 to present
- **Contents**: Original publication details, version evolution, derivatives, legacy

### 13. **Compiler Comparison** - Feature Analysis
- **Code ID**: `smallc-comparison`
- **Description**: Detailed comparison between basic and enhanced compilers
- **Contents**: Feature tables, performance metrics, use cases

## Example Programs

### 14. **test.c** - Runtime Test Program
- **Code ID**: `smallc-test`
- **Description**: Comprehensive test of all runtime library functions
- **Tests**: I/O, strings, files, memory, utilities

### 15. **demo_enhanced.c** - Enhanced Features Demo
- **Code ID**: `smallc-demo-enhanced`
- **Description**: Demonstrates enhanced compiler features
- **Shows**: Parameters, initialization, sorting, recursion

### 16. **calculator.c** - Complete Application
- **Code ID**: `smallc-complete-app`
- **Description**: Interactive calculator with expression parser
- **Features**: Arithmetic operations, memory storage, error handling

### 17. **example.c** - Simple Example
- **Code ID**: `small-c-example`
- **Description**: Basic example showing Small-C capabilities
- **Features**: String operations, loops, Fibonacci

### 18. **test_enhanced.c** - Enhanced Compiler Test Suite
- **Code ID**: `smallc-enhanced-test`
- **Description**: Comprehensive test suite for enhanced compiler
- **Tests**: All enhanced features, 10 test categories

### 19. **error_test.c** - Error Handling Test Cases
- **Code ID**: `smallc-error-test`
- **Description**: Test cases for error conditions
- **Tests**: 12 different error scenarios

### 20. **Enhanced Improvements Summary**
- **Code ID**: `smallc-enhanced-improvements`
- **Description**: Summary of robustness improvements
- **Contents**: Complete list of enhancements and testing guide

## Platform Support Summary

| Component | Linux x64 | Linux ARM64 | Windows x64 | Windows ARM64 |
|-----------|-----------|-------------|-------------|---------------|
| Compiler | ✓ scc.c, scc_enhanced.c | ✓ scc.c, scc_enhanced.c | ✓ scc.c, scc_enhanced.c | ✓ scc.c, scc_enhanced.c |
| Runtime | ✓ runtime.c | ✓ runtime.c | ✓ runtime.c | ✓ runtime.c |
| Syscalls | ✓ syscall_linux_x64.s | ✓ syscall_linux_arm64.s | ✓ syscall_win_x64.s | ✓ syscall_win_arm64.s |

## Quick Start Commands

### Linux Build
```bash
# Build everything
make

# Compile a program (Linux x64)
./scc program.c > program.s
as program.s -o program.o
ld -static syscall_linux_x64.o runtime.o program.o -o program

# Or use the build script
./scc-build.sh program.c
```

### Windows Build
```bash
# Compile for Windows x64
./scc program.c > program.s
x86_64-w64-mingw32-as program.s -o program.o
x86_64-w64-mingw32-ld syscall_win_x64.o runtime.o program.o -o program.exe

# For Windows ARM64
./scc -arm64 program.c > program.s
aarch64-w64-mingw32-as program.s -o program.o
aarch64-w64-mingw32-ld syscall_win_arm64.o runtime.o program.o -o program.exe
```

## File Sizes (Approximate)
- scc (compiled): ~50KB
- scc_enhanced (compiled): ~80KB
- runtime.o: ~20KB
- syscall_*.o: ~2KB each
- Total project: <1MB

## Notes
- All files are self-contained and have no external dependencies
- The compilers can compile themselves (self-bootstrapping)
- The runtime works identically on all platforms
- Assembly files are platform-specific but provide the same interface
