#!/bin/bash
# build_linkers.sh - Build Small-C linkers

# Function to build a linker
build_linker() {
    local source=$1
    local target=$2
    local syscall=$3
    
    echo "Building $target from $source..."
    
    # Compile to assembly
    ./scc $source > ${target}.s || { echo "Compilation failed"; exit 1; }
    
    # Assemble
    as ${target}.s -o ${target}.o || { echo "Assembly failed"; exit 1; }
    
    # Link
    ld -static $syscall runtime.o ${target}.o -o $target || { echo "Linking failed"; exit 1; }
    
    # Make executable
    chmod +x $target
    
    echo "$target built successfully"
}

# Check if scc exists
if [ ! -f "./scc" ]; then
    echo "Error: scc compiler not found. Please build it first."
    exit 1
fi

# Detect architecture
ARCH=$(uname -m)
OS=$(uname -s)

echo "Detected: $OS on $ARCH"

# Build appropriate linkers
if [ "$OS" = "Linux" ]; then
    if [ "$ARCH" = "x86_64" ]; then
        # Build x64 Linux linker
        build_linker "sld_linux_x64.c" "sld" "syscall_linux_x64.o"
        
        # Build enhanced linker
        build_linker "sld_enhanced.c" "sld_enhanced" "syscall_linux_x64.o"
        
    elif [ "$ARCH" = "aarch64" ]; then
        # Build ARM64 Linux linker
        build_linker "sld_linux_arm64.c" "sld" "syscall_linux_arm64.o"
        
        # Build enhanced linker
        build_linker "sld_enhanced.c" "sld_enhanced" "syscall_linux_arm64.o"
    else
        echo "Unsupported architecture: $ARCH"
        exit 1
    fi
    
    echo ""
    echo "Linux linkers built successfully!"
    echo "Usage: ./sld -o myprogram syscall_linux_*.o runtime.o myprogram.o"
    
elif [ "$OS" = "MINGW"* ] || [ "$OS" = "MSYS"* ] || [ "$OS" = "CYGWIN"* ]; then
    # Windows build (using MinGW/MSYS/Cygwin)
    echo "Building Windows linkers..."
    
    # Note: Windows builds would need different handling
    echo "Windows build requires different toolchain setup."
    echo "Please compile manually using:"
    echo "  cl /c sld_win_x64.c      (for x64)"
    echo "  cl /c sld_win_arm64.c    (for ARM64)"
    echo "  cl /c sld_win_enhanced.c (for enhanced)"
    
else
    echo "Unsupported OS: $OS"
    exit 1
fi

echo ""
echo "To test a linker:"
echo "1. Compile a test program: ./scc test.c > test.s"
echo "2. Assemble: as test.s -o test.o"
echo "3. Link: ./sld -o test syscall_*.o runtime.o test.o"
echo "4. Run: ./test"
