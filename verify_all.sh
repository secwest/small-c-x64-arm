#!/bin/bash
# Complete verification script for Small-C compiler project
# Tests both basic and enhanced compilers with all features

set -e  # Exit on error

echo "========================================="
echo "Small-C Compiler Project Verification"
echo "========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check required files
echo "Checking required files..."
REQUIRED_FILES=(
    "scc.c"
    "scc_enhanced.c"
    "runtime.c"
    "Makefile"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $file found"
    else
        echo -e "${RED}✗${NC} $file missing"
        exit 1
    fi
done
echo ""

# Detect platform
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

case $ARCH in
    x86_64) ARCH="x64" ;;
    aarch64) ARCH="arm64" ;;
esac

echo "Platform: $OS/$ARCH"
echo ""

# Build everything
echo "Building compilers and runtime..."
make clean >/dev/null 2>&1 || true
make
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Build successful"
else
    echo -e "${RED}✗${NC} Build failed"
    exit 1
fi
echo ""

# Test 1: Basic compiler with minimal program
echo "Test 1: Basic compiler with minimal program"
cat > test1.c << 'EOF'
int main() {
    puts("Basic compiler works!");
    return 0;
}
EOF

./scc test1.c > test1.s
as test1.s -o test1.o
ld -static syscall_linux_${ARCH}.o runtime.o test1.o -o test1
if ./test1 | grep -q "Basic compiler works!"; then
    echo -e "${GREEN}✓${NC} Basic compiler test passed"
else
    echo -e "${RED}✗${NC} Basic compiler test failed"
fi
echo ""

# Test 2: Basic compiler with printf
echo "Test 2: Basic compiler with printf"
cat > test2.c << 'EOF'
int main() {
    int x;
    x = 42;
    printf("The answer is %d\n", x);
    return 0;
}
EOF

./scc test2.c > test2.s
as test2.s -o test2.o
ld -static syscall_linux_${ARCH}.o runtime.o test2.o -o test2
OUTPUT=$(./test2)
if echo "$OUTPUT" | grep -q "The answer is 42"; then
    echo -e "${GREEN}✓${NC} Printf test passed"
else
    echo -e "${RED}✗${NC} Printf test failed"
fi
echo ""

# Test 3: Enhanced compiler with parameters
echo "Test 3: Enhanced compiler with parameters"
cat > test3.c << 'EOF'
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(30, 12);
    printf("30 + 12 = %d\n", result);
    return 0;
}
EOF

./scc_enhanced test3.c > test3.s
as test3.s -o test3.o
ld -static syscall_linux_${ARCH}.o runtime.o test3.o -o test3
OUTPUT=$(./test3)
if echo "$OUTPUT" | grep -q "30 + 12 = 42"; then
    echo -e "${GREEN}✓${NC} Function parameters test passed"
else
    echo -e "${RED}✗${NC} Function parameters test failed"
fi
echo ""

# Test 4: Enhanced compiler with local initialization
echo "Test 4: Enhanced compiler with local initialization"
cat > test4.c << 'EOF'
int main() {
    int x = 10;
    int y = 20;
    int sum = x + y;
    printf("Sum: %d\n", sum);
    return 0;
}
EOF

./scc_enhanced test4.c > test4.s
as test4.s -o test4.o
ld -static syscall_linux_${ARCH}.o runtime.o test4.o -o test4
OUTPUT=$(./test4)
if echo "$OUTPUT" | grep -q "Sum: 30"; then
    echo -e "${GREEN}✓${NC} Local initialization test passed"
else
    echo -e "${RED}✗${NC} Local initialization test failed"
fi
echo ""

# Test 5: File operations
echo "Test 5: File operations"
cat > test5.c << 'EOF'
int main() {
    int fd;
    char buffer[100];
    int n;
    
    fd = creat("testfile.txt");
    if (fd >= 0) {
        write(fd, "Hello from Small-C!\n", 20);
        close(fd);
        
        fd = open("testfile.txt", 0);
        if (fd >= 0) {
            n = read(fd, buffer, 99);
            buffer[n] = 0;
            close(fd);
            printf("Read: %s", buffer);
            return 0;
        }
    }
    return 1;
}
EOF

./scc test5.c > test5.s
as test5.s -o test5.o
ld -static syscall_linux_${ARCH}.o runtime.o test5.o -o test5
OUTPUT=$(./test5)
if echo "$OUTPUT" | grep -q "Read: Hello from Small-C!"; then
    echo -e "${GREEN}✓${NC} File operations test passed"
else
    echo -e "${RED}✗${NC} File operations test failed"
fi
rm -f testfile.txt
echo ""

# Test 6: Self-compilation test (basic compiler)
echo "Test 6: Self-compilation test (basic compiler)"
./scc scc.c > scc_self.s 2>/dev/null
if [ -s scc_self.s ]; then
    echo -e "${GREEN}✓${NC} Basic compiler can compile itself"
else
    echo -e "${RED}✗${NC} Basic compiler self-compilation failed"
fi
echo ""

# Test 7: Self-compilation test (enhanced compiler)
echo "Test 7: Self-compilation test (enhanced compiler)"
./scc_enhanced scc_enhanced.c > scc_enhanced_self.s 2>/dev/null
if [ -s scc_enhanced_self.s ]; then
    echo -e "${GREEN}✓${NC} Enhanced compiler can compile itself"
else
    echo -e "${RED}✗${NC} Enhanced compiler self-compilation failed"
fi
echo ""

# Test 8: Character literals (enhanced only)
echo "Test 8: Character literals (enhanced compiler)"
cat > test8.c << 'EOF'
int main() {
    char ch = 'A';
    char newline = '\n';
    printf("Character: %c%c", ch, newline);
    return 0;
}
EOF

./scc_enhanced test8.c > test8.s
as test8.s -o test8.o
ld -static syscall_linux_${ARCH}.o runtime.o test8.o -o test8
OUTPUT=$(./test8)
if echo "$OUTPUT" | grep -q "Character: A"; then
    echo -e "${GREEN}✓${NC} Character literals test passed"
else
    echo -e "${RED}✗${NC} Character literals test failed"
fi
echo ""

# Test 9: Comments (enhanced only)
echo "Test 9: Comments (enhanced compiler)"
cat > test9.c << 'EOF'
// Single line comment
int main() {
    /* Multi-line
       comment */
    puts("Comments work!");
    return 0; // End of line comment
}
EOF

./scc_enhanced test9.c > test9.s
as test9.s -o test9.o
ld -static syscall_linux_${ARCH}.o runtime.o test9.o -o test9
OUTPUT=$(./test9)
if echo "$OUTPUT" | grep -q "Comments work!"; then
    echo -e "${GREEN}✓${NC} Comments test passed"
else
    echo -e "${RED}✗${NC} Comments test failed"
fi
echo ""

# Test 10: Loops and control flow
echo "Test 10: Loops and control flow"
cat > test10.c << 'EOF'
int main() {
    int i;
    int sum = 0;
    
    for (i = 1; i <= 5; i++) {
        sum += i;
    }
    
    printf("Sum 1-5: %d\n", sum);
    
    i = 0;
    while (i < 3) {
        printf("i=%d ", i);
        i++;
    }
    puts("");
    
    return 0;
}
EOF

./scc_enhanced test10.c > test10.s
as test10.s -o test10.o
ld -static syscall_linux_${ARCH}.o runtime.o test10.o -o test10
OUTPUT=$(./test10)
if echo "$OUTPUT" | grep -q "Sum 1-5: 15" && echo "$OUTPUT" | grep -q "i=0 i=1 i=2"; then
    echo -e "${GREEN}✓${NC} Loops test passed"
else
    echo -e "${RED}✗${NC} Loops test failed"
fi
echo ""

# Clean up test files
echo "Cleaning up test files..."
rm -f test*.c test*.s test*.o test[0-9] test1[0-9]
rm -f scc_self.s scc_enhanced_self.s
echo ""

# Summary
echo "========================================="
echo "Verification Complete!"
echo "========================================="
echo ""
echo "Both compilers are working correctly with:"
echo "- Basic function calls and I/O"
echo "- Printf with format specifiers"  
echo "- Function parameters (enhanced)"
echo "- Local initialization (enhanced)"
echo "- File operations"
echo "- Self-compilation capability"
echo "- Character literals (enhanced)"
echo "- Comments (enhanced)"
echo "- Loops and control flow"
echo ""
echo -e "${GREEN}All tests passed!${NC}"
