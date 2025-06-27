#!/bin/bash
# Test script to verify Small-C compiler functionality

echo "Small-C Compiler Test Script"
echo "============================"
echo ""

# Check if compiler exists
if [ ! -f scc ]; then
    echo "Building compiler first..."
    gcc -o scc scc.c
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build compiler"
        exit 1
    fi
fi

# Check if runtime exists
if [ ! -f runtime.o ]; then
    echo "Building runtime library..."
    # First compile runtime.c with the Small-C compiler
    ./scc runtime.c > runtime.s
    if [ $? -ne 0 ]; then
        echo "Error: Failed to compile runtime.c"
        exit 1
    fi
    
    # Assemble it
    as runtime.s -o runtime.o
    if [ $? -ne 0 ]; then
        echo "Error: Failed to assemble runtime.s"
        exit 1
    fi
fi

# Detect architecture
ARCH=$(uname -m)
case $ARCH in
    x86_64)
        SYSCALL_OBJ="syscall_linux_x64.o"
        ;;
    aarch64)
        SYSCALL_OBJ="syscall_linux_arm64.o"
        ;;
    *)
        echo "Error: Unsupported architecture $ARCH"
        exit 1
        ;;
esac

# Check if syscall object exists
if [ ! -f $SYSCALL_OBJ ]; then
    echo "Error: $SYSCALL_OBJ not found"
    echo "Please assemble the appropriate syscall file first"
    exit 1
fi

# Create a simple test program
cat > test_simple.c << 'EOF'
int main() {
    puts("Hello from Small-C!");
    printf("Number test: %d\n", 42);
    return 0;
}
EOF

echo "Compiling test_simple.c..."
./scc test_simple.c > test_simple.s
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed"
    exit 1
fi

echo "Generated assembly:"
echo "==================="
head -20 test_simple.s
echo "..."
echo ""

echo "Assembling..."
as test_simple.s -o test_simple.o
if [ $? -ne 0 ]; then
    echo "Error: Assembly failed"
    exit 1
fi

echo "Linking..."
ld -static $SYSCALL_OBJ runtime.o test_simple.o -o test_simple
if [ $? -ne 0 ]; then
    echo "Error: Linking failed"
    echo ""
    echo "Checking for undefined symbols:"
    nm -u test_simple.o
    exit 1
fi

echo "Running test_simple..."
echo "======================"
./test_simple
if [ $? -eq 0 ]; then
    echo ""
    echo "SUCCESS: Basic test passed!"
else
    echo ""
    echo "ERROR: Test program failed"
    exit 1
fi

# Now test the more complex runtime test
if [ -f runtime_test.c ]; then
    echo ""
    echo "Compiling runtime_test.c..."
    ./scc runtime_test.c > runtime_test.s
    if [ $? -eq 0 ]; then
        as runtime_test.s -o runtime_test.o
        ld -static $SYSCALL_OBJ runtime.o runtime_test.o -o runtime_test
        echo "You can now run: ./runtime_test"
    fi
fi

echo ""
echo "Compiler test completed!"
