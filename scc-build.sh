#!/bin/bash
# Build script for Small-C programs
# Usage: ./scc-build.sh program.c [output_name]

if [ $# -lt 1 ]; then
    echo "Usage: $0 source.c [output_name]"
    exit 1
fi

SOURCE=$1
OUTPUT=${2:-${SOURCE%.c}}

# Detect OS and architecture
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

# Map architecture names
case $ARCH in
    x86_64)
        ARCH="x64"
        SCC_ARCH="-x64"
        ;;
    aarch64)
        ARCH="arm64"
        SCC_ARCH="-arm64"
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        exit 1
        ;;
esac

# Determine system call object
case $OS in
    linux)
        SYSCALL_OBJ="syscall_linux_${ARCH}.o"
        LD_FLAGS="-static"
        ;;
    mingw*|cygwin*|msys*)
        OS="windows"
        SYSCALL_OBJ="syscall_win_${ARCH}.o"
        LD_FLAGS="-lkernel32"
        ;;
    *)
        echo "Unsupported OS: $OS"
        exit 1
        ;;
esac

echo "Building $SOURCE for $OS/$ARCH..."

# Check if runtime files exist
if [ ! -f runtime.o ]; then
    echo "Error: runtime.o not found. Run 'make' first."
    exit 1
fi

if [ ! -f $SYSCALL_OBJ ]; then
    echo "Error: $SYSCALL_OBJ not found. Run 'make' first."
    exit 1
fi

# Compile the source file
echo "Compiling $SOURCE..."
./scc $SCC_ARCH $SOURCE > ${OUTPUT}.s
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

# Assemble
echo "Assembling ${OUTPUT}.s..."
as ${OUTPUT}.s -o ${OUTPUT}.o
if [ $? -ne 0 ]; then
    echo "Assembly failed"
    exit 1
fi

# Link
echo "Linking..."
ld $LD_FLAGS $SYSCALL_OBJ runtime.o ${OUTPUT}.o -o $OUTPUT
if [ $? -ne 0 ]; then
    echo "Linking failed"
    exit 1
fi

# Clean up intermediate files
rm -f ${OUTPUT}.s ${OUTPUT}.o

echo "Build successful: $OUTPUT"

# Make executable
chmod +x $OUTPUT
