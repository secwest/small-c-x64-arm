#!/bin/bash
# test_sas_enhanced.sh - Test script for SAS Enhanced assembler and linker

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name=$1
    local expected_result=$2
    
    echo -n "Testing $test_name... "
    
    if [ "$expected_result" = "0" ]; then
        echo -e "${GREEN}PASSED${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAILED${NC}"
        ((TESTS_FAILED++))
    fi
}

# Check if assembler and linker exist
if [ ! -f "./sas_enhanced" ]; then
    echo "Error: sas_enhanced not found. Please build it first."
    exit 1
fi

if [ ! -f "./sld_sas" ]; then
    echo "Error: sld_sas not found. Please build it first."
    exit 1
fi

# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    TEST_ARCH="x64"
elif [ "$ARCH" = "aarch64" ]; then
    TEST_ARCH="arm64"
else
    echo "Unsupported architecture: $ARCH"
    exit 1
fi

echo "Running SAS Enhanced tests on $TEST_ARCH..."
echo

# Test 1: Basic assembly
echo "=== Test 1: Basic Assembly ==="
cat > test1.s << EOF
.arch $TEST_ARCH
.text
.global _start
_start:
    nop
    nop
    ret
EOF

./sas_enhanced -o test1.o test1.s
run_test "Basic assembly" $?

# Test 2: Data directives
echo -e "\n=== Test 2: Data Directives ==="
cat > test2.s << EOF
.arch $TEST_ARCH
.data
byte_val:   .byte 0x41, 0x42, 0x43
word_val:   .word 0x1234
dword_val:  .dword 0x12345678
string_val: .asciz "Hello"
buffer:     .space 64

.text
.global test_func
test_func:
    ret
EOF

./sas_enhanced -o test2.o test2.s
run_test "Data directives" $?

# Test 3: Expression evaluation
echo -e "\n=== Test 3: Expression Evaluation ==="
cat > test3.s << EOF
.arch $TEST_ARCH
.equ CONST1, 100
.equ CONST2, 200
.equ RESULT, CONST1 + CONST2

.data
value1: .dword RESULT
value2: .dword CONST1 * 2
value3: .dword (1 << 8) | 0xFF
value4: .dword ~0x12345678

.text
.global _start
_start:
    ret
EOF

./sas_enhanced -o test3.o test3.s
run_test "Expression evaluation" $?

# Test 4: Symbol references
echo -e "\n=== Test 4: Symbol References ==="
if [ "$TEST_ARCH" = "x64" ]; then
    cat > test4.s << EOF
.arch x64
.text
.global func1
.global func2

func1:
    call func2
    ret

func2:
    mov rax, 42
    ret
EOF
else
    cat > test4.s << EOF
.arch arm64
.text
.global func1
.global func2

func1:
    bl func2
    ret

func2:
    mov x0, #42
    ret
EOF
fi

./sas_enhanced -o test4.o test4.s
run_test "Symbol references" $?

# Test 5: Multiple sections
echo -e "\n=== Test 5: Multiple Sections ==="
cat > test5.s << EOF
.arch $TEST_ARCH
.text
.global code_func
code_func:
    nop
    ret

.data
initialized_data: .dword 0x12345678

.bss
uninitialized_buffer: .space 1024

.text
another_func:
    ret
EOF

./sas_enhanced -o test5.o test5.s
run_test "Multiple sections" $?

# Test 6: Linking test
echo -e "\n=== Test 6: Linking Test ==="
if [ "$TEST_ARCH" = "x64" ]; then
    cat > main.s << EOF
.arch x64
.text
.global _start
.extern helper_func

_start:
    call helper_func
    mov rax, 60
    xor rdi, rdi
    syscall
EOF

    cat > helper.s << EOF
.arch x64
.text
.global helper_func

helper_func:
    mov rax, 1
    mov rdi, 1
    mov rsi, msg
    mov rdx, 7
    syscall
    ret

.data
msg: .ascii "Test!\\n"
EOF
else
    cat > main.s << EOF
.arch arm64
.text
.global _start
.extern helper_func

_start:
    bl helper_func
    mov x0, #0
    mov x8, #93
    svc 0
EOF

    cat > helper.s << EOF
.arch arm64
.text
.global helper_func

helper_func:
    mov x0, #1
    adr x1, msg
    mov x2, #6
    mov x8, #64
    svc 0
    ret

.data
msg: .ascii "Test!\\n"
EOF
fi

./sas_enhanced -o main.o main.s
./sas_enhanced -o helper.o helper.s
./sld_sas -o test_linked main.o helper.o
run_test "Linking multiple objects" $?

# Test 7: Complex expressions
echo -e "\n=== Test 7: Complex Expressions ==="
cat > test7.s << EOF
.arch $TEST_ARCH
.equ BASE, 0x1000
.equ SIZE, 256
.equ MASK, 0xFF

.data
expr1: .dword BASE + SIZE * 4
expr2: .dword (BASE << 4) | MASK
expr3: .dword ~(MASK << 8)
expr4: .dword (SIZE / 8) - 1

.text
.global _start
_start:
    ret
EOF

./sas_enhanced -o test7.o test7.s
run_test "Complex expressions" $?

# Test 8: Local labels
echo -e "\n=== Test 8: Local Labels ==="
if [ "$TEST_ARCH" = "x64" ]; then
    cat > test8.s << EOF
.arch x64
.text
.global loop_func

loop_func:
    mov rcx, 10
.loop_start:
    dec rcx
    jnz .loop_start
    ret

another_func:
    mov rcx, 5
.loop_start:
    dec rcx
    jnz .loop_start
    ret
EOF
else
    cat > test8.s << EOF
.arch arm64
.text
.global loop_func

loop_func:
    mov x0, #10
.loop_start:
    sub x0, x0, #1
    cbnz x0, .loop_start
    ret

another_func:
    mov x0, #5
.loop_start:
    sub x0, x0, #1
    cbnz x0, .loop_start
    ret
EOF
fi

./sas_enhanced -o test8.o test8.s
run_test "Local labels" $?

# Test 9: Alignment directive
echo -e "\n=== Test 9: Alignment Directive ==="
cat > test9.s << EOF
.arch $TEST_ARCH
.data
byte1: .byte 1
.align 4
word1: .word 0x1234
.align 8
qword1: .quad 0x123456789ABCDEF0
.align 16
buffer: .space 32

.text
.global _start
_start:
    ret
EOF

./sas_enhanced -o test9.o test9.s
run_test "Alignment directive" $?

# Test 10: String handling
echo -e "\n=== Test 10: String Handling ==="
cat > test10.s << EOF
.arch $TEST_ARCH
.data
str1: .ascii "Simple ASCII"
str2: .asciz "Null terminated"
str3: .string "Also null terminated"
str4: .ascii "With\\nescapes\\tand\\\\backslash"
str5: .byte 'A', 'B', 'C', 0

.text
.global _start
_start:
    ret
EOF

./sas_enhanced -o test10.o test10.s
run_test "String handling" $?

# Summary
echo
echo "======================================="
echo "Test Summary:"
echo "  Passed: $TESTS_PASSED"
echo "  Failed: $TESTS_FAILED"
echo "======================================="

# Cleanup
rm -f test*.s test*.o main.s helper.s main.o helper.o test_linked

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
