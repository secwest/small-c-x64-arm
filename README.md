# Modern(2025)  Small-C Compiler and Runtime

![Dr. Dobb's Journal](https://raw.githubusercontent.com/secwest/small-c-x64-arm/refs/heads/main/Dr-Dobbs.jpg)

A minimalist, self-bootstrapping C compiler in the spirit of Ron Cain's original Small-C from Dr. Dobb's Journal (May-June 1980). This implementation targets modern 64-bit architectures while maintaining the simplicity and educational value of the original.

## Historical Background

Ron Cain's Small-C was published in **Dr. Dobb's Journal of Computer Calisthenics & Orthodontia** in 1980:

> 📚 **Original Small-C Publications**
> - **Issue #45 (May 1980)**: "A Small C Compiler for the 8080's" - Part 1 of the compiler
> - **Issue #46 (June/July 1980)**: Continuation and completion of the compiler listing
> - **Issue #48 (September 1980)**: "A Runtime Library for the Small C Compiler" - Runtime support
> 
> 📖 **Read the original articles:**
> - [Dr. Dobb's Journal Volume 5 (1980) on Internet Archive](https://archive.org/details/dr_dobbs_journal_vol_05_201803)
> - [High-quality PDF scans at 6502.org](http://6502.org/documents/publications/dr_dobbs_journal/dr_dobbs_journal_vol_05.pdf)
> 
> 💻 **Original Source Code:**
> - [Reconstructed Small-C v1.1 source on GitHub](https://github.com/trcwm/smallc_v1) - Faithful reconstruction from the magazine listings
> 
> These historic issues contain the complete source code listings that could be typed in by hand - a revolutionary concept that democratized compiler technology!

### Evolution of Small-C

After Ron Cain's original publication, the compiler evolved significantly:
- **Small-C v2.0** (December 1982) - James E. Hendrix published an enhanced version in Dr. Dobb's Journal with added features like structures, Boolean operators, and code optimization
- **The Small-C Handbook** (1984) - Hendrix wrote a comprehensive book documenting the enhanced compiler
- This modern implementation stays true to Cain's original minimalist design while targeting 64-bit systems

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
- **Windows**: x64 and ARM64

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
syscall_win_arm64.s     - Windows ARM64 system calls
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

The entire compiler is under 1200 lines of readable C code, making it perfect for study and modification.

## Historical Note

This implementation honors Ron Cain's original Small-C compiler first published in Dr. Dobb's Journal in 1980. The complete compiler listing spanned two issues due to its length, with the runtime library following later (see Historical Background section above for links to the original articles). 

The original was a groundbreaking achievement - a self-compiling C compiler small enough to type in from a magazine listing. Cain noted in the article that the compiler "will compile and run under UNIX" and was designed to be portable. This modern version maintains that spirit while targeting contemporary 64-bit systems.

Ron Cain developed Small-C at SRI International on a PDP 11/45 Unix system, with the explicit goal of creating a compiler simple enough for hobbyists to understand and modify. The fact that it was published as a type-in listing democratized compiler technology in an era when commercial compilers cost hundreds or thousands of dollars.

A long time ago Dragos Ruiu ported this to Pascal for the Apple ][+ UCSD p-System, by typing in the magazine article listing, and then porting the code to Pascal, with the intent to compile the original code with a new 6502 back end that used zero page pseudo registers to make up for the very limited register structure of the 6502. That effort, while it produced a compiler, ultimately failed when it hit a roadblock - because that Apple ][ Pascal system lacked enough memory, even with bank switched 64k, to hold a symbol table to compile to full source code. This project is a revisitation to that old classic project, inspired by a conversation and post from Ron Cain, because this compiler and related articles was what eventually started Dragos's career, and work with C code. So this new modern port follows in those footsteps, to leave code to study and use for any other student interested in learning about compilers by studying a simple recursive descent compiler, and to honor the simple elegance and brilliance of Ron's original bootstrap self-compiling concept.

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
