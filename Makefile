# Makefile for Small-C Compiler and Runtime
# Supports Linux and Windows on x64 and ARM64

# Detect OS and architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Set OS
ifeq ($(UNAME_S),Linux)
    OS := linux
else ifeq ($(OS),Windows_NT)
    OS := windows
else
    $(error Unsupported OS: $(UNAME_S))
endif

# Set architecture
ifeq ($(UNAME_M),x86_64)
    ARCH := x64
else ifeq ($(UNAME_M),aarch64)
    ARCH := arm64
else
    $(error Unsupported architecture: $(UNAME_M))
endif

# Compiler settings
CC := gcc
AS := as
LD := ld
SCC := ./scc

# Platform-specific settings
ifeq ($(OS),linux)
    ifeq ($(ARCH),x64)
        SYSCALL_OBJ := syscall_linux_x64.o
        AS_FLAGS := 
        LD_FLAGS := -static
    else
        SYSCALL_OBJ := syscall_linux_arm64.o
        AS_FLAGS := 
        LD_FLAGS := -static
    endif
else
    ifeq ($(ARCH),x64)
        SYSCALL_OBJ := syscall_win_x64.o
        AS_FLAGS := 
        LD_FLAGS := -lkernel32 -luser32
    else
        SYSCALL_OBJ := syscall_win_arm64.o
        AS_FLAGS := 
        LD_FLAGS := -lkernel32 -luser32
    endif
endif

# Targets
all: scc runtime.o $(SYSCALL_OBJ)

# Build the Small-C compiler
scc: scc.c
	$(CC) -o scc scc.c

# Compile runtime library with Small-C
runtime.o: runtime.c scc
	$(SCC) runtime.c > runtime.s
	$(AS) $(AS_FLAGS) runtime.s -o runtime.o

# Assemble system call wrappers
syscall_linux_x64.o: syscall_linux_x64.s
	$(AS) $(AS_FLAGS) syscall_linux_x64.s -o syscall_linux_x64.o

syscall_linux_arm64.o: syscall_linux_arm64.s
	$(AS) $(AS_FLAGS) syscall_linux_arm64.s -o syscall_linux_arm64.o

syscall_win_x64.o: syscall_win_x64.s
	$(AS) $(AS_FLAGS) syscall_win_x64.s -o syscall_win_x64.o

syscall_win_arm64.o: syscall_win_arm64.s
	$(AS) $(AS_FLAGS) syscall_win_arm64.s -o syscall_win_arm64.o

# Build a test program
test: test.c runtime.o $(SYSCALL_OBJ)
	$(SCC) test.c > test.s
	$(AS) $(AS_FLAGS) test.s -o test.o
	$(LD) $(LD_FLAGS) $(SYSCALL_OBJ) runtime.o test.o -o test

# Clean build files
clean:
	rm -f scc *.o *.s test

# Install (optional)
install: scc runtime.o $(SYSCALL_OBJ)
	mkdir -p /usr/local/smallc/lib
	cp runtime.o $(SYSCALL_OBJ) /usr/local/smallc/lib/
	cp scc /usr/local/bin/

.PHONY: all clean test install
