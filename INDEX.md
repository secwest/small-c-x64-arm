# Small-C Compiler - Code Quick Reference

Use these Code IDs to access the complete source code for the Small-C compiler project.

## Core Compilers
- **Basic Compiler**: `modern-small-c` - scc.c
- **Enhanced Compiler**: `smallc-enhanced` - scc_enhanced.c

## Runtime Library
- **Runtime Library**: `smallc-runtime` - runtime.c

## Linux System Calls
- **Linux x64**: `syscall-linux-x64` - syscall_linux_x64.s
- **Linux ARM64**: `syscall-linux-arm64` - syscall_linux_arm64.s

## Windows System Calls
- **Windows x64**: `syscall-win-x64` - syscall_win_x64.s
- **Windows ARM64**: `syscall-win-arm64` - syscall_win_arm64.s

## Build System
- **Makefile**: `smallc-makefile` - Makefile
- **Build Script**: `smallc-build` - scc-build.sh

## Documentation
- **README**: `smallc-readme` - README.md
- **Build Guide**: `smallc-buildguide` - BUILD_GUIDE.md
- **History**: `smallc-history` - HISTORY.md
- **Comparison**: `smallc-comparison` - Feature comparison
- **File Index**: `smallc-file-index` - Complete file listing
- **This Reference**: `smallc-Code-links` - Quick Code reference

## Example Programs
- **Test Program**: `smallc-test` - test.c
- **Enhanced Demo**: `smallc-demo-enhanced` - demo_enhanced.c
- **Calculator App**: `smallc-complete-app` - calculator.c
- **Simple Example**: `small-c-example` - example.c

## Download All Files

To get started with the Small-C compiler:

1. Save all the core files:
   - scc.c (basic compiler)
   - scc_enhanced.c (enhanced compiler)
   - runtime.c

2. Save the appropriate system call files for your platform:
   - Linux: syscall_linux_x64.s or syscall_linux_arm64.s
   - Windows: syscall_win_x64.s or syscall_win_arm64.s

3. Save the build files:
   - Makefile
   - scc-build.sh (make it executable with `chmod +x`)

4. Save example programs to test:
   - test.c
   - calculator.c

5. Build everything:
   ```bash
   make
   ./scc-build.sh test.c
   ./test
   ```

## Platform Notes

### Linux Support
- Full support for x64 and ARM64
- Direct system calls for maximum efficiency
- Static linking by default
- Tested on Ubuntu, Debian, Fedora, Arch

### Windows Support  
- Full support for x64 and ARM64
- Uses Windows API (kernel32.dll)
- Requires MinGW-w64 toolchain for cross-compilation
- Tested on Windows 10/11

Both platforms use the exact same compiler and runtime library code - only the system call layer differs.
