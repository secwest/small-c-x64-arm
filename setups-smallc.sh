#!/bin/bash
# Small-C Compiler Setup Script
# This script helps you set up the Small-C compiler project

echo "Small-C Compiler Setup"
echo "====================="
echo ""

# Create project directory
PROJECT_DIR="smallc-compiler"
echo "Creating project directory: $PROJECT_DIR"
mkdir -p "$PROJECT_DIR"
cd "$PROJECT_DIR"

# Detect OS and architecture
OS=$(uname -s)
ARCH=$(uname -m)

echo "Detected system: $OS $ARCH"
echo ""

# Instructions for saving files
cat << 'EOF' > SETUP_INSTRUCTIONS.txt
Small-C Compiler Setup Instructions
===================================

Please save the following files from the artifacts:

CORE FILES (Required):
1. scc.c - Basic Small-C Compiler (artifact: modern-small-c)
2. runtime.c - Runtime Library (artifact: smallc-runtime)
3. Makefile - Build Configuration (artifact: smallc-makefile)
4. scc-build.sh - Build Script (artifact: smallc-build)

ENHANCED COMPILER (Optional):
5. scc_enhanced.c - Enhanced Compiler (artifact: smallc-enhanced)

SYSTEM CALLS (Choose based on your platform):
For Linux x64:
6. syscall_linux_x64.s (artifact: syscall-linux-x64)

For Linux ARM64:
6. syscall_linux_arm64.s (artifact: syscall-linux-arm64)

For Windows x64:
6. syscall_win_x64.s (artifact: syscall-win-x64)

For Windows ARM64:
6. syscall_win_arm64.s (artifact: syscall-win-arm64)

EXAMPLE PROGRAMS (Optional):
7. test.c - Test Program (artifact: smallc-test)
8. calculator.c - Calculator Demo (artifact: smallc-complete-app)
9. demo_enhanced.c - Enhanced Features Demo (artifact: smallc-demo-enhanced)

DOCUMENTATION (Optional):
10. README.md - Main Documentation (artifact: smallc-readme)
11. BUILD_GUIDE.md - Build Guide (artifact: smallc-buildguide)

After saving all files:
1. Make the build script executable: chmod +x scc-build.sh
2. Build everything: make
3. Test the compiler: ./scc-build.sh test.c && ./test

EOF

echo "Setup instructions written to SETUP_INSTRUCTIONS.txt"
echo ""

# Create a simple test program
cat << 'EOF' > hello.c
/* Hello World for Small-C */
int main() {
    puts("Hello from Small-C!");
    puts("If you see this, the compiler works!");
    return 0;
}
EOF

echo "Created hello.c test program"
echo ""

# Create a build helper
cat << 'EOF' > build_and_run.sh
#!/bin/bash
# Helper script to build and run Small-C programs

if [ $# -lt 1 ]; then
    echo "Usage: $0 program.c"
    exit 1
fi

echo "Building $1..."
if [ -f scc-build.sh ]; then
    ./scc-build.sh "$1"
else
    echo "Error: scc-build.sh not found. Please run 'make' first."
    exit 1
fi

# Run the program if build succeeded
PROGRAM="${1%.c}"
if [ -f "$PROGRAM" ]; then
    echo "Running $PROGRAM..."
    echo "===================="
    ./"$PROGRAM"
else
    echo "Build failed."
fi
EOF

chmod +x build_and_run.sh
echo "Created build_and_run.sh helper script"
echo ""

# Platform-specific instructions
case "$OS $ARCH" in
    "Linux x86_64")
        echo "Your system needs: syscall_linux_x64.s"
        ;;
    "Linux aarch64")
        echo "Your system needs: syscall_linux_arm64.s"
        ;;
    "Darwin x86_64")
        echo "macOS x64 detected - you'll need to adapt the Linux x64 syscalls"
        ;;
    "Darwin arm64")
        echo "macOS ARM64 detected - you'll need to adapt the Linux ARM64 syscalls"
        ;;
    *)
        echo "Platform $OS $ARCH may require adaptation"
        ;;
esac

echo ""
echo "Next steps:"
echo "1. Save all the required files listed in SETUP_INSTRUCTIONS.txt"
echo "2. Run 'make' to build the compiler"
echo "3. Test with: ./build_and_run.sh hello.c"
echo ""
echo "For the enhanced compiler, also save scc_enhanced.c"
echo "For full documentation, save README.md and BUILD_GUIDE.md"
