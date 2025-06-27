# Modern Small-C Compiler and Runtime

A minimalist, self-bootstrapping C compiler in the spirit of Ron Cain's original Small-C from Dr. Dobb's Journal (May 1980). This implementation targets modern 64-bit architectures while maintaining the simplicity and educational value of the original.

## Features

### Language Support
- Basic types: `int`, `char`, pointers, single-dimensional arrays
- Control flow: `if/else`, `while`, `for`, `break`, `continue`, `return`
- Operators: arithmetic, logical, bitwise, comparison, increment/decrement
- String literals
- Simple but complete expression parsing with proper precedence

### Runtime Library
- **I/O Functions**: `putchar`, `getchar`, `puts`, `gets`, `printf` (basic)
- **String Functions**: `strlen`, `strcmp`, `strcpy`
- **File Operations**: `open`, `creat`, `close`, `read`, `write`, `fgetc`, `fputc`, `fputs`
- **Memory Functions**: `memset`, `memcpy`
- **Utility Functions**: `abs`, `min`, `max`, `atoi`, `exit`

### Platform Support
- **Linux**: x64 and ARM64
- **Windows**: x64 (ARM64 planned)

## Building

### Prerequisites
- GCC or compatible C compiler (for initial bootstrap)
- GNU assembler (as)
- GNU linker (ld)
- Make

### Build Steps

1. Build everything:
```bash
make
```

2. Test the compiler:
```bash
make test
./test
```

## Usage

### Method 1: Using the build script
```bash
chmod +x scc-build.sh
./scc-build.sh myprogram.c
./myprogram
```

### Method 2: Manual compilation
```bash
# Compile to assembly
./scc myprogram.c > myprogram.s

# Assemble
as myprogram.s -o myprogram.o

# Link with runtime
ld -static syscall_linux_x64.o runtime.o myprogram.o -o myprogram

# Run
./myprogram
```

### Cross-compilation
```bash
# Target ARM64
./scc -arm64 myprogram.c > myprogram.s

# Target x64 (default)
./scc -x64 myprogram.c > myprogram.s
```

## Self-Bootstrapping

The compiler can compile itself:

```bash
# Stage 1: Use GCC to build initial compiler
gcc -o scc0 scc.c

# Stage 2: Use scc0 to compile itself
./scc0 scc.c > scc1.s
as scc1.s -o scc1.o
ld -static syscall_linux_x64.o runtime.o scc1.o -o scc1

# Stage 3: Verify self-hosting
./scc1 scc.c > scc2.s
diff scc1.s scc2.s  # Should be identical!
```

## Example Programs

### Hello World
```c
int main() {
    puts("Hello, World!");
    return 0;
}
```

### File I/O
```c
int main() {
    int fd;
    char buffer[100];
    
    /* Write to file */
    fd = creat("output.txt");
    fputs("Hello from Small-C!\n", fd);
    close(fd);
    
    /* Read from file */
    fd = open("output.txt", 0);
    read(fd, buffer, 99);
    close(fd);
    
    puts(buffer);
    return 0;
}
```

### Interactive Program
```c
int main() {
    char name[50];
    int age;
    
    printf("What's your name? ");
    gets(name);
    
    printf("Hello, %s!\n", name);
    printf("How old are you? ");
    gets(name);  /* Reuse buffer */
    age = atoi(name);
    
    if (age >= 18) {
        puts("You're an adult!");
    } else {
        puts("You're still young!");
    }
    
    return 0;
}
```

## Limitations

Following the minimalist philosophy of the original Small-C:

- No preprocessor (`#include`, `#define`, etc.)
- No structs, unions, or enums
- No floating-point types
- No multi-dimensional arrays
- No function prototypes
- Limited type checking
- No optimization

## Project Structure

```
scc.c                 - The Small-C compiler
runtime.c             - Runtime library (portable C)
syscall_linux_x64.s   - Linux x64 system calls
syscall_linux_arm64.s - Linux ARM64 system calls
syscall_win_x64.s     - Windows x64 system calls
Makefile              - Build configuration
scc-build.sh          - Convenience build script
test.c                - Test program
README.md             - This file
```

## Educational Value

This compiler is ideal for:
- Understanding how compilers work
- Learning about code generation
- Studying system calls and runtime libraries
- Exploring self-bootstrapping concepts
- Teaching compiler construction

The entire compiler is under 1000 lines of readable C code, making it perfect for study and modification.

## Historical Note

This implementation honors Ron Cain's original Small-C compiler published in Dr. Dobb's Journal #45 (May 1980). The original was a groundbreaking achievement - a self-compiling C compiler small enough to type in from a magazine listing. This modern version maintains that spirit while targeting contemporary 64-bit systems.

## License

Public domain, following the tradition of the original Small-C.

## Contributing

Feel free to:
- Port to new architectures
- Add missing C features (while keeping it "small")
- Improve the runtime library
- Fix bugs
- Create educational materials

Remember: the goal is simplicity and education, not competing with GCC or Clang!
