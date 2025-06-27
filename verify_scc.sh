#!/bin/bash
# Verify that scc.c generates correct assembly for runtime library calls

echo "Small-C Compiler Verification"
echo "============================="
echo ""

# Create a test program with various runtime calls
cat > verify_test.c << 'EOF'
int main() {
    int n;
    char buf[20];
    
    /* Direct function calls */
    puts("Hello");
    putchar(65);
    putchar(10);
    
    /* Printf with different formats */
    printf("Number: %d\n", 42);
    printf("Hex: %x\n", 255);
    printf("Char: %c\n", 88);
    printf("String: %s\n", "test");
    
    /* Function with return value */
    n = strlen("Hello");
    printf("Length: %d\n", n);
    
    /* Gets and atoi */
    gets(buf);
    n = atoi(buf);
    printf("You entered: %d\n", n);
    
    return 0;
}
EOF

# Build the compiler if needed
if [ ! -f scc ]; then
    echo "Building compiler..."
    gcc -o scc scc.c
fi

# Compile and show assembly
echo "Compiling verify_test.c..."
./scc verify_test.c > verify_test.s

echo ""
echo "Generated Assembly (first 100 lines):"
echo "====================================="
head -100 verify_test.s

echo ""
echo "Checking for external function declarations:"
echo "==========================================="
grep -E "^\.(extern|globl)" verify_test.s

echo ""
echo "Checking function calls:"
echo "======================="
grep -E "(call|bl)" verify_test.s

echo ""
echo "To complete the test, run:"
echo "  as verify_test.s -o verify_test.o"
echo "  ld -static syscall_linux_x64.o runtime.o verify_test.o -o verify_test"
echo "  ./verify_test"
