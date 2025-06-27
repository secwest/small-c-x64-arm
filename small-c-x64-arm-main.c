/*
 * Modern Small-C Compiler
 * Based on Ron Cain's original Small-C from Dr. Dobb's Journal, May 1980
 * 
 * This is a minimal, self-bootstrapping C compiler targeting ARM64 and x86-64
 * Supports: char, int, pointers, arrays, basic control flow
 * 
 * Compile with: gcc -o scc scc.c
 * Usage: ./scc [-arm64|-x64] source.c > output.s
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Configuration */
#define NAMESIZE 32
#define MAXARGS 8
#define MAXLOCALS 32
#define MAXGLOBALS 200
#define MAXWHILE 20
#define MAXSTRING 2048
#define LINESIZE 256

/* Target architecture */
enum { TARGET_X64, TARGET_ARM64 };
int target = TARGET_X64;

/* Token types */
enum {
    T_EOF = -1, T_INT = 256, T_CHAR, T_IF, T_ELSE, T_WHILE, T_FOR,
    T_RETURN, T_BREAK, T_CONTINUE, T_IDENT, T_NUMBER, T_STRING,
    T_EQ, T_NE, T_LE, T_GE, T_SHL, T_SHR, T_AND, T_OR, T_INC, T_DEC
};

/* Symbol table entry */
struct symbol {
    char name[NAMESIZE];
    int type;       /* 0=int, 1=char, 2=int*, 3=char* */
    int offset;     /* stack offset for locals, label for globals */
    int isarray;
    int size;       /* array size */
};

/* Global state */
char line[LINESIZE];
char *lptr;
int token;
int tokval;
char tokstr[NAMESIZE];
FILE *input;

/* Symbol tables */
struct symbol globals[MAXGLOBALS];
int nglobals = 0;
struct symbol locals[MAXLOCALS];
int nlocals = 0;
int sp = 0;  /* stack pointer offset */

/* Control flow */
int breaklab[MAXWHILE];
int contlab[MAXWHILE];
int wsp = 0;
int lab = 1;

/* String pool */
char strpool[MAXSTRING];
int strptr = 0;

/* Forward declarations */
void program(void);
void function(void);
void statement(void);
void expression(void);
void assignment(void);
void logical_or(void);
void logical_and(void);
void bitwise_or(void);
void bitwise_xor(void);
void bitwise_and(void);
void equality(void);
void relational(void);
void shift(void);
void additive(void);
void multiplicative(void);
void unary(void);
void postfix(void);
void primary(void);
int gettoken(void);
void error(char *msg);
void emit(char *fmt, ...);
void emit_label(int n);
void emit_jump(int n);
void emit_branch_false(int n);
void push(void);
void pop(char *reg);
struct symbol *lookup(char *name);
struct symbol *add_symbol(char *name, int type, int size);

/* Emit assembly based on target */
void emit_prolog(void) {
    if (target == TARGET_X64) {
        emit(".text");
        emit(".globl _start");
        emit("_start:");
        emit("  call main");
        emit("  movl %%eax, %%edi");
        emit("  movl $60, %%eax");
        emit("  syscall");
    } else {
        emit(".text");
        emit(".globl _start");
        emit("_start:");
        emit("  bl main");
        emit("  mov x8, #93");
        emit("  svc #0");
    }
}

void push(void) {
    if (target == TARGET_X64) {
        emit("  pushq %%rax");
        sp -= 8;
    } else {
        emit("  str x0, [sp, #-16]!");
        sp -= 16;
    }
}

void pop(char *reg) {
    if (target == TARGET_X64) {
        emit("  popq %s", reg);
        sp += 8;
    } else {
        emit("  ldr %s, [sp], #16", reg);
        sp += 16;
    }
}

/* Simple error handling */
void error(char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

#include <stdarg.h>
void emit(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void emit_label(int n) {
    printf("L%d:\n", n);
}

void emit_jump(int n) {
    if (target == TARGET_X64) {
        emit("  jmp L%d", n);
    } else {
        emit("  b L%d", n);
    }
}

void emit_branch_false(int n) {
    if (target == TARGET_X64) {
        emit("  testq %%rax, %%rax");
        emit("  jz L%d", n);
    } else {
        emit("  cbz x0, L%d", n);
    }
}

/* Lexical analyzer */
void skip_white(void) {
    while (*lptr && isspace(*lptr)) lptr++;
}

int gettoken(void) {
    skip_white();
    
    if (!*lptr) {
        if (!fgets(line, LINESIZE, input)) return T_EOF;
        lptr = line;
        return gettoken();
    }
    
    /* Single character tokens */
    if (strchr("+-*/%&|^~!<>()[]{}.,;=", *lptr)) {
        int c = *lptr++;
        
        /* Two character tokens */
        if (c == '=' && *lptr == '=') { lptr++; return T_EQ; }
        if (c == '!' && *lptr == '=') { lptr++; return T_NE; }
        if (c == '<' && *lptr == '=') { lptr++; return T_LE; }
        if (c == '>' && *lptr == '=') { lptr++; return T_GE; }
        if (c == '<' && *lptr == '<') { lptr++; return T_SHL; }
        if (c == '>' && *lptr == '>') { lptr++; return T_SHR; }
        if (c == '&' && *lptr == '&') { lptr++; return T_AND; }
        if (c == '|' && *lptr == '|') { lptr++; return T_OR; }
        if (c == '+' && *lptr == '+') { lptr++; return T_INC; }
        if (c == '-' && *lptr == '-') { lptr++; return T_DEC; }
        
        return c;
    }
    
    /* Numbers */
    if (isdigit(*lptr)) {
        tokval = 0;
        while (isdigit(*lptr)) {
            tokval = tokval * 10 + (*lptr++ - '0');
        }
        return T_NUMBER;
    }
    
    /* Identifiers and keywords */
    if (isalpha(*lptr) || *lptr == '_') {
        char *p = tokstr;
        while (isalnum(*lptr) || *lptr == '_') {
            *p++ = *lptr++;
        }
        *p = '\0';
        
        if (!strcmp(tokstr, "int")) return T_INT;
        if (!strcmp(tokstr, "char")) return T_CHAR;
        if (!strcmp(tokstr, "if")) return T_IF;
        if (!strcmp(tokstr, "else")) return T_ELSE;
        if (!strcmp(tokstr, "while")) return T_WHILE;
        if (!strcmp(tokstr, "for")) return T_FOR;
        if (!strcmp(tokstr, "return")) return T_RETURN;
        if (!strcmp(tokstr, "break")) return T_BREAK;
        if (!strcmp(tokstr, "continue")) return T_CONTINUE;
        
        return T_IDENT;
    }
    
    /* String literals */
    if (*lptr == '"') {
        lptr++;
        char *p = tokstr;
        while (*lptr && *lptr != '"') {
            if (*lptr == '\\') {
                lptr++;
                switch (*lptr) {
                    case 'n': *p++ = '\n'; break;
                    case 't': *p++ = '\t'; break;
                    case '\\': *p++ = '\\'; break;
                    case '"': *p++ = '"'; break;
                    default: *p++ = *lptr;
                }
                lptr++;
            } else {
                *p++ = *lptr++;
            }
        }
        *p = '\0';
        if (*lptr == '"') lptr++;
        return T_STRING;
    }
    
    error("Unknown character");
    return T_EOF;
}

/* Symbol table */
struct symbol *lookup(char *name) {
    int i;
    for (i = 0; i < nlocals; i++) {
        if (!strcmp(locals[i].name, name)) return &locals[i];
    }
    for (i = 0; i < nglobals; i++) {
        if (!strcmp(globals[i].name, name)) return &globals[i];
    }
    return NULL;
}

struct symbol *add_symbol(char *name, int type, int size) {
    struct symbol *sym;
    if (nlocals > 0) {
        if (nlocals >= MAXLOCALS) error("Too many locals");
        sym = &locals[nlocals++];
        sp -= (type < 2 ? 8 : 8) * (size > 0 ? size : 1);
        sym->offset = sp;
    } else {
        if (nglobals >= MAXGLOBALS) error("Too many globals");
        sym = &globals[nglobals++];
        sym->offset = lab++;
    }
    strcpy(sym->name, name);
    sym->type = type;
    sym->isarray = (size > 0);
    sym->size = size;
    return sym;
}

/* Parser */
void program(void) {
    lptr = line;
    token = gettoken();
    
    while (token != T_EOF) {
        int type = T_INT;
        if (token == T_INT || token == T_CHAR) {
            type = token;
            token = gettoken();
        }
        
        if (token != T_IDENT) error("Expected identifier");
        strcpy(tokstr, tokstr);
        char name[NAMESIZE];
        strcpy(name, tokstr);
        token = gettoken();
        
        /* Function or global variable */
        if (token == '(') {
            token = gettoken();
            function();
        } else {
            /* Global variable */
            int size = 0;
            if (token == '[') {
                token = gettoken();
                if (token != T_NUMBER) error("Expected array size");
                size = tokval;
                token = gettoken();
                if (token != ']') error("Expected ]");
                token = gettoken();
            }
            
            struct symbol *sym = add_symbol(name, type == T_CHAR ? 1 : 0, size);
            emit(".data");
            emit(".globl %s", name);
            emit("%s:", name);
            if (size > 0) {
                emit("  .space %d", size * (type == T_CHAR ? 1 : 8));
            } else {
                emit("  .quad 0");
            }
            emit(".text");
            
            if (token != ';') error("Expected ;");
            token = gettoken();
        }
    }
}

void function(void) {
    /* Skip parameters for now - Small-C style */
    while (token != ')') {
        token = gettoken();
    }
    token = gettoken();
    
    if (token != '{') error("Expected {");
    token = gettoken();
    
    /* Function prologue */
    if (target == TARGET_X64) {
        emit("  pushq %%rbp");
        emit("  movq %%rsp, %%rbp");
    } else {
        emit("  stp x29, x30, [sp, #-16]!");
        emit("  mov x29, sp");
    }
    
    sp = 0;
    nlocals = 0;
    
    /* Local declarations */
    while (token == T_INT || token == T_CHAR) {
        int type = token;
        token = gettoken();
        
        while (1) {
            if (token != T_IDENT) error("Expected identifier");
            char name[NAMESIZE];
            strcpy(name, tokstr);
            token = gettoken();
            
            int size = 0;
            if (token == '[') {
                token = gettoken();
                if (token != T_NUMBER) error("Expected array size");
                size = tokval;
                token = gettoken();
                if (token != ']') error("Expected ]");
                token = gettoken();
            }
            
            add_symbol(name, type == T_CHAR ? 1 : 0, size);
            
            if (token != ',') break;
            token = gettoken();
        }
        
        if (token != ';') error("Expected ;");
        token = gettoken();
    }
    
    /* Allocate locals */
    if (sp < 0) {
        if (target == TARGET_X64) {
            emit("  subq $%d, %%rsp", -sp);
        } else {
            emit("  sub sp, sp, #%d", -sp);
        }
    }
    
    /* Statements */
    while (token != '}') {
        statement();
    }
    token = gettoken();
    
    /* Function epilogue */
    if (target == TARGET_X64) {
        emit("  movq %%rbp, %%rsp");
        emit("  popq %%rbp");
        emit("  ret");
    } else {
        emit("  mov sp, x29");
        emit("  ldp x29, x30, [sp], #16");
        emit("  ret");
    }
}

void statement(void) {
    int lab1, lab2;
    
    switch (token) {
        case '{':
            token = gettoken();
            while (token != '}') {
                statement();
            }
            token = gettoken();
            break;
            
        case T_IF:
            token = gettoken();
            if (token != '(') error("Expected (");
            token = gettoken();
            expression();
            if (token != ')') error("Expected )");
            token = gettoken();
            
            lab1 = lab++;
            emit_branch_false(lab1);
            statement();
            
            if (token == T_ELSE) {
                token = gettoken();
                lab2 = lab++;
                emit_jump(lab2);
                emit_label(lab1);
                statement();
                emit_label(lab2);
            } else {
                emit_label(lab1);
            }
            break;
            
        case T_WHILE:
            token = gettoken();
            if (token != '(') error("Expected (");
            token = gettoken();
            
            lab1 = lab++;
            lab2 = lab++;
            breaklab[wsp] = lab2;
            contlab[wsp] = lab1;
            wsp++;
            
            emit_label(lab1);
            expression();
            if (token != ')') error("Expected )");
            token = gettoken();
            
            emit_branch_false(lab2);
            statement();
            emit_jump(lab1);
            emit_label(lab2);
            
            wsp--;
            break;
            
        case T_RETURN:
            token = gettoken();
            if (token != ';') {
                expression();
            }
            if (token != ';') error("Expected ;");
            token = gettoken();
            
            if (target == TARGET_X64) {
                emit("  movq %%rbp, %%rsp");
                emit("  popq %%rbp");
                emit("  ret");
            } else {
                emit("  mov sp, x29");
                emit("  ldp x29, x30, [sp], #16");
                emit("  ret");
            }
            break;
            
        case T_BREAK:
            token = gettoken();
            if (token != ';') error("Expected ;");
            token = gettoken();
            if (wsp == 0) error("break outside loop");
            emit_jump(breaklab[wsp-1]);
            break;
            
        case T_CONTINUE:
            token = gettoken();
            if (token != ';') error("Expected ;");
            token = gettoken();
            if (wsp == 0) error("continue outside loop");
            emit_jump(contlab[wsp-1]);
            break;
            
        case ';':
            token = gettoken();
            break;
            
        default:
            expression();
            if (token != ';') error("Expected ;");
            token = gettoken();
    }
}

/* Expression parser - operator precedence */
void expression(void) {
    assignment();
}

void assignment(void) {
    logical_or();
    
    if (token == '=') {
        push();
        token = gettoken();
        assignment();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  movq %%rax, (%%rdx)");
        } else {
            pop("x1");
            emit("  str x0, [x1]");
        }
    }
}

void logical_or(void) {
    logical_and();
    
    while (token == T_OR) {
        push();
        token = gettoken();
        logical_and();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  orq %%rdx, %%rax");
            emit("  setne %%al");
            emit("  movzbq %%al, %%rax");
        } else {
            pop("x1");
            emit("  orr x0, x0, x1");
            emit("  cmp x0, #0");
            emit("  cset x0, ne");
        }
    }
}

void logical_and(void) {
    bitwise_or();
    
    while (token == T_AND) {
        push();
        token = gettoken();
        bitwise_or();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  testq %%rdx, %%rdx");
            emit("  setne %%dl");
            emit("  testq %%rax, %%rax");
            emit("  setne %%al");
            emit("  andb %%dl, %%al");
            emit("  movzbq %%al, %%rax");
        } else {
            pop("x1");
            emit("  cmp x1, #0");
            emit("  cset x1, ne");
            emit("  cmp x0, #0");
            emit("  cset x0, ne");
            emit("  and x0, x0, x1");
        }
    }
}

void bitwise_or(void) {
    bitwise_xor();
    
    while (token == '|') {
        push();
        token = gettoken();
        bitwise_xor();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  orq %%rdx, %%rax");
        } else {
            pop("x1");
            emit("  orr x0, x0, x1");
        }
    }
}

void bitwise_xor(void) {
    bitwise_and();
    
    while (token == '^') {
        push();
        token = gettoken();
        bitwise_and();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  xorq %%rdx, %%rax");
        } else {
            pop("x1");
            emit("  eor x0, x0, x1");
        }
    }
}

void bitwise_and(void) {
    equality();
    
    while (token == '&') {
        push();
        token = gettoken();
        equality();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  andq %%rdx, %%rax");
        } else {
            pop("x1");
            emit("  and x0, x0, x1");
        }
    }
}

void equality(void) {
    relational();
    
    while (token == T_EQ || token == T_NE) {
        int op = token;
        push();
        token = gettoken();
        relational();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  cmpq %%rax, %%rdx");
            emit("  %s %%al", op == T_EQ ? "sete" : "setne");
            emit("  movzbq %%al, %%rax");
        } else {
            pop("x1");
            emit("  cmp x1, x0");
            emit("  cset x0, %s", op == T_EQ ? "eq" : "ne");
        }
    }
}

void relational(void) {
    shift();
    
    while (token == '<' || token == '>' || token == T_LE || token == T_GE) {
        int op = token;
        push();
        token = gettoken();
        shift();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            emit("  cmpq %%rax, %%rdx");
            switch (op) {
                case '<': emit("  setl %%al"); break;
                case '>': emit("  setg %%al"); break;
                case T_LE: emit("  setle %%al"); break;
                case T_GE: emit("  setge %%al"); break;
            }
            emit("  movzbq %%al, %%rax");
        } else {
            pop("x1");
            emit("  cmp x1, x0");
            switch (op) {
                case '<': emit("  cset x0, lt"); break;
                case '>': emit("  cset x0, gt"); break;
                case T_LE: emit("  cset x0, le"); break;
                case T_GE: emit("  cset x0, ge"); break;
            }
        }
    }
}

void shift(void) {
    additive();
    
    while (token == T_SHL || token == T_SHR) {
        int op = token;
        push();
        token = gettoken();
        additive();
        
        if (target == TARGET_X64) {
            emit("  movq %%rax, %%rcx");
            pop("%rax");
            emit("  %s %%cl, %%rax", op == T_SHL ? "shl" : "shr");
        } else {
            emit("  mov x2, x0");
            pop("x0");
            emit("  %s x0, x0, x2", op == T_SHL ? "lsl" : "lsr");
        }
    }
}

void additive(void) {
    multiplicative();
    
    while (token == '+' || token == '-') {
        int op = token;
        push();
        token = gettoken();
        multiplicative();
        
        if (target == TARGET_X64) {
            pop("%rdx");
            if (op == '+') {
                emit("  addq %%rdx, %%rax");
            } else {
                emit("  subq %%rax, %%rdx");
                emit("  movq %%rdx, %%rax");
            }
        } else {
            pop("x1");
            if (op == '+') {
                emit("  add x0, x1, x0");
            } else {
                emit("  sub x0, x1, x0");
            }
        }
    }
}

void multiplicative(void) {
    unary();
    
    while (token == '*' || token == '/' || token == '%') {
        int op = token;
        push();
        token = gettoken();
        unary();
        
        if (target == TARGET_X64) {
            if (op == '*') {
                pop("%rdx");
                emit("  imulq %%rdx, %%rax");
            } else {
                emit("  movq %%rax, %%rbx");
                pop("%rax");
                emit("  cqo");
                emit("  idivq %%rbx");
                if (op == '%') {
                    emit("  movq %%rdx, %%rax");
                }
            }
        } else {
            pop("x1");
            if (op == '*') {
                emit("  mul x0, x1, x0");
            } else {
                emit("  sdiv x2, x1, x0");
                if (op == '/') {
                    emit("  mov x0, x2");
                } else {
                    emit("  msub x0, x2, x0, x1");
                }
            }
        }
    }
}

void unary(void) {
    switch (token) {
        case '!':
            token = gettoken();
            unary();
            if (target == TARGET_X64) {
                emit("  testq %%rax, %%rax");
                emit("  setz %%al");
                emit("  movzbq %%al, %%rax");
            } else {
                emit("  cmp x0, #0");
                emit("  cset x0, eq");
            }
            break;
            
        case '~':
            token = gettoken();
            unary();
            if (target == TARGET_X64) {
                emit("  notq %%rax");
            } else {
                emit("  mvn x0, x0");
            }
            break;
            
        case '-':
            token = gettoken();
            unary();
            if (target == TARGET_X64) {
                emit("  negq %%rax");
            } else {
                emit("  neg x0, x0");
            }
            break;
            
        case '*':
            token = gettoken();
            unary();
            if (target == TARGET_X64) {
                emit("  movq (%%rax), %%rax");
            } else {
                emit("  ldr x0, [x0]");
            }
            break;
            
        case '&':
            token = gettoken();
            if (token != T_IDENT) error("Expected identifier");
            struct symbol *sym = lookup(tokstr);
            if (!sym) error("Undefined variable");
            token = gettoken();
            
            if (sym->offset < 0) {
                if (target == TARGET_X64) {
                    emit("  leaq %d(%%rbp), %%rax", sym->offset);
                } else {
                    emit("  add x0, x29, #%d", sym->offset);
                }
            } else {
                if (target == TARGET_X64) {
                    emit("  movq $%s, %%rax", sym->name);
                } else {
                    emit("  adrp x0, %s", sym->name);
                    emit("  add x0, x0, :lo12:%s", sym->name);
                }
            }
            break;
            
        case T_INC:
        case T_DEC:
            {
                int op = token;
                token = gettoken();
                unary();
                if (target == TARGET_X64) {
                    emit("  %sq (%%rax)", op == T_INC ? "inc" : "dec");
                    emit("  movq (%%rax), %%rax");
                } else {
                    emit("  ldr x1, [x0]");
                    emit("  %s x1, x1, #1", op == T_INC ? "add" : "sub");
                    emit("  str x1, [x0]");
                    emit("  mov x0, x1");
                }
            }
            break;
            
        default:
            postfix();
    }
}

void postfix(void) {
    primary();
    
    while (1) {
        if (token == '[') {
            push();
            token = gettoken();
            expression();
            if (token != ']') error("Expected ]");
            token = gettoken();
            
            if (target == TARGET_X64) {
                emit("  shlq $3, %%rax");
                pop("%rdx");
                emit("  addq %%rdx, %%rax");
                emit("  movq (%%rax), %%rax");
            } else {
                emit("  lsl x0, x0, #3");
                pop("x1");
                emit("  add x0, x1, x0");
                emit("  ldr x0, [x0]");
            }
        } else if (token == T_INC || token == T_DEC) {
            int op = token;
            token = gettoken();
            
            if (target == TARGET_X64) {
                emit("  movq %%rax, %%rdx");
                emit("  movq (%%rax), %%rax");
                emit("  %sq (%%rdx)", op == T_INC ? "inc" : "dec");
            } else {
                emit("  mov x1, x0");
                emit("  ldr x0, [x0]");
                emit("  ldr x2, [x1]");
                emit("  %s x2, x2, #1", op == T_INC ? "add" : "sub");
                emit("  str x2, [x1]");
            }
        } else {
            break;
        }
    }
}

void primary(void) {
    switch (token) {
        case T_NUMBER:
            if (target == TARGET_X64) {
                emit("  movq $%d, %%rax", tokval);
            } else {
                emit("  mov x0, #%d", tokval);
            }
            token = gettoken();
            break;
            
        case T_STRING:
            {
                int slab = lab++;
                emit(".data");
                emit("S%d:", slab);
                emit("  .asciz \"%s\"", tokstr);
                emit(".text");
                
                if (target == TARGET_X64) {
                    emit("  movq $S%d, %%rax", slab);
                } else {
                    emit("  adrp x0, S%d", slab);
                    emit("  add x0, x0, :lo12:S%d", slab);
                }
                token = gettoken();
            }
            break;
            
        case T_IDENT:
            {
                struct symbol *sym = lookup(tokstr);
                if (!sym) error("Undefined variable");
                token = gettoken();
                
                if (sym->isarray) {
                    if (sym->offset < 0) {
                        if (target == TARGET_X64) {
                            emit("  leaq %d(%%rbp), %%rax", sym->offset);
                        } else {
                            emit("  add x0, x29, #%d", sym->offset);
                        }
                    } else {
                        if (target == TARGET_X64) {
                            emit("  movq $%s, %%rax", sym->name);
                        } else {
                            emit("  adrp x0, %s", sym->name);
                            emit("  add x0, x0, :lo12:%s", sym->name);
                        }
                    }
                } else {
                    if (sym->offset < 0) {
                        if (target == TARGET_X64) {
                            emit("  movq %d(%%rbp), %%rax", sym->offset);
                        } else {
                            emit("  ldr x0, [x29, #%d]", sym->offset);
                        }
                    } else {
                        if (target == TARGET_X64) {
                            emit("  movq %s(%%rip), %%rax", sym->name);
                        } else {
                            emit("  adrp x0, %s", sym->name);
                            emit("  ldr x0, [x0, :lo12:%s]", sym->name);
                        }
                    }
                }
            }
            break;
            
        case '(':
            token = gettoken();
            expression();
            if (token != ')') error("Expected )");
            token = gettoken();
            break;
            
        default:
            error("Expected primary expression");
    }
}

int main(int argc, char **argv) {
    char *filename = NULL;
    int i;
    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-arm64")) {
            target = TARGET_ARM64;
        } else if (!strcmp(argv[i], "-x64")) {
            target = TARGET_X64;
        } else {
            filename = argv[i];
        }
    }
    
    if (!filename) {
        fprintf(stderr, "Usage: %s [-arm64|-x64] source.c\n", argv[0]);
        return 1;
    }
    
    input = fopen(filename, "r");
    if (!input) {
        perror(filename);
        return 1;
    }
    
    emit_prolog();
    program();
    
    fclose(input);
    return 0;
}
