# The History and Evolution of Small-C

## Original Publication (1980)

Ron Cain's Small-C compiler was published in **Dr. Dobb's Journal of Computer Calisthenics & Orthodontia** across multiple issues:

### The Compiler (May-June 1980)
- **Issue #45 (May 1980)**: "A Small C Compiler for the 8080's" - Part 1
  - Introduction and first portion of the compiler source
  - Explained the philosophy of a minimal C subset
  - Began the complete source listing
  
- **Issue #46 (June/July 1980)**: Continuation and completion
  - Remainder of the compiler source code
  - Code generation routines
  - Complete listing of Small-C v1.1

### The Runtime Library (September 1980)
- **Issue #48 (September 1980)**: "A Runtime Library for the Small C Compiler"
  - 8080 assembly language runtime support
  - I/O routines, startup code
  - Essential for creating working programs

### Original Features (v1.1)
Ron Cain's Small-C v1.1 supported:
- **Data types**: char (8-bit), int (16-bit)
- **Pointers**: Single level of indirection
- **Arrays**: Single-dimensional only
- **Functions**: K&R style, no prototypes
- **Control flow**: if/else, while, return
- **Operators**: Basic arithmetic, logical, bitwise
- **Notable omissions**: No structs, unions, floats, switch, for loops, preprocessing

## Evolution and Enhancements

### Small-C v2.0 (1982)
James E. Hendrix significantly enhanced Small-C:
- **Published**: Dr. Dobb's Journal (December 1982)
- **New features**:
  - Code optimization
  - Data initialization
  - Conditional compilation (#ifdef)
  - extern storage class
  - for, do/while, switch, goto statements
  - Boolean operators (&&, ||)
  - Block local variables
  - Improved code generation

### The Small-C Handbook (1984)
- **Author**: James E. Hendrix
- **Publisher**: Reston Publishing Company
- Comprehensive documentation of Small-C v2.x
- Included complete CP/M runtime library
- Became the definitive Small-C reference

### Further Developments (1985-1990)
- **Small-C v2.1** (1984): Added CP/M-86 support
- **Small-C v2.2** (1987): MS-DOS support
- **Small-C v3.0** (1988): Never fully completed
- **Various ports**: Z80, 6502, 6809, 68000 processors

## Notable Derivatives

### BDS C (1979-1987)
- By Leor Zolman
- Influenced by Small-C concepts
- More complete C implementation for CP/M

### Small-C++ (1988)
- Object-oriented extensions
- Never gained wide adoption

### Various Educational Versions
- Used in compiler courses worldwide
- Simplified versions for teaching
- Modern rewrites for new architectures

## Preservation and Archives

### Original Source Archives
- **[GitHub - smallc_v1](https://github.com/trcwm/smallc_v1)**: Faithful reconstruction of Cain's v1.1
- **[Dr. Dobb's Journal scans](https://archive.org/details/dr_dobbs_journal_vol_05_201803)**: Original magazine articles
- **C Users Group**: Distributed various versions in the 1980s

### Modern Implementations
- This project: Targets modern 64-bit systems
- Various GitHub repos: Updated for contemporary use
- Educational rewrites: Simplified for teaching

## Design Philosophy

Ron Cain's original design principles:
1. **Simplicity over completeness** - A useful subset of C
2. **Self-bootstrapping** - Compiler can compile itself
3. **Readable source** - Educational value
4. **Minimal size** - Fits in limited memory
5. **Portability** - Written in its own subset of C

These principles influenced many subsequent projects and remain relevant for educational compilers today.

## Impact and Legacy

Small-C's influence extends far beyond its original 8080 target:
- **Democratized compiler technology** - Anyone could type it in and modify it
- **Educational standard** - Used in countless compiler courses
- **Bootstrapping demonstration** - Showed self-compilation was practical
- **Inspired derivatives** - Led to many small language implementations
- **Open source pioneer** - Published source in a magazine predated modern open source

## Timeline Summary

- **1980**: Ron Cain publishes Small-C v1.1 in Dr. Dobb's Journal
- **1982**: James Hendrix publishes Small-C v2.0 with major enhancements
- **1984**: The Small-C Handbook documents the enhanced version
- **1985-1990**: Various ports and enhancements by the community
- **2000s**: Revival for embedded systems and education
- **Today**: Continues as an educational tool and minimalist compiler example

## This Implementation

Our modern Small-C implementation:
- **Basic compiler (scc.c)**: Follows Cain's v1.1 minimalist design
- **Enhanced compiler (scc_enhanced.c)**: Includes v2.0-style improvements
- **Target**: Modern 64-bit architectures (x64, ARM64)
- **Platforms**: Linux and Windows
- **Philosophy**: Maintains simplicity while being practically useful

The goal is to preserve the educational value and self-bootstrapping capability that made Small-C revolutionary, while making it relevant for modern systems.

## References

1. Cain, Ron. "A Small C Compiler for the 8080's, Part 1." Dr. Dobb's Journal, no. 45, May 1980.
2. Cain, Ron. "A Small C Compiler for the 8080's, Part 2." Dr. Dobb's Journal, no. 46, June/July 1980.
3. Cain, Ron. "A Runtime Library for the Small C Compiler." Dr. Dobb's Journal, no. 48, September 1980.
4. Hendrix, James E. "Small-C Compiler, Version 2.0." Dr. Dobb's Journal, December 1982.
5. Hendrix, James E. The Small-C Handbook. Reston Publishing Company, 1984.
6. [Dr. Dobb's Journal Archive](https://archive.org/details/dr_dobbs_journal)
7. [Original Small-C v1.1 Source](https://github.com/trcwm/smallc_v1)
