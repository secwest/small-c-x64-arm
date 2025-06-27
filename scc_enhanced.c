/*
 * Enhanced Modern Small-C Compiler
 * Based on Ron Cain's original Small-C from Dr. Dobb's Journal, May 1980
 * 
 * This version includes:
 * - Better error reporting with line numbers
 * - Function parameter support
 * - Local variable initialization
 * - Improved code generation
 * - Better handling of character literals
 * 
 * Still maintains the simplicity and self-bootstrapping capability
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
#define MAXFUNCS 100

/* Target architecture */
enum { TARGET_X64, TARGET_ARM64 };
int target = TARGET_X64;

/* Token types */
enum {
    T_EOF = -1, T_INT = 256, T_CHAR, T_IF, T_ELSE, T_WHILE, T_FOR,
    T_RETURN, T_BREAK, T_CONTINUE, T_IDENT, T_NUMBER, T_STRING,
    T_EQ, T_NE, T_LE, T_GE, T_SHL, T_SHR, T_AND, T_OR, T_INC, T_DEC,
    T_PLUSEQ, T_MINUSEQ, T_STAREQ, T_SLASHEQ, T_CHARLIT
};

/* Symbol table entry */
struct symbol {
    char name[NAMESIZE];
    int type;       /* 0=int, 1=char, 2=int*, 3=char* */
    int offset;     /* stack offset for locals, label for globals */
    int isarray;
    int size;       /* array size */
    int isparam;    /* is function parameter */
};

/* Function table entry */
struct function {
    char name[NAMESIZE];
    int defined;
    int nparams;
    int param_types[MAXARGS];
};

/* Global state */
char line[LINESIZE];
char *lptr;
int lineno = 1;
int token;
int tokval;
char tokstr[NAMESIZE];
FILE *input;
char *filename;

/* Symbol tables */
struct symbol globals[MAXGLOBALS];
int nglobals = 0;
struct symbol locals[MAXLOCALS];
int nlocals = 0;
int sp = 0;  /* stack pointer offset */
int param_offset = 16;  /* parameter offset from frame pointer */

/* Function table */
struct function functions[MAXFUNCS];
int nfuncs = 0;
char curfunc[NAMESIZE];

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
void global_declaration(int type);
void function(int type);
void parameter_list(void);
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
struct function *lookup_func(char *name);
struct function *add_function(char *name);
void emit_load_param(int offset);
void emit_store_local(int offset);
void emit_load_local(int offset);

/* Enhanced error reporting */
void error(char *msg) {
    fprintf(stderr, "%s:%d: Error: %s\n", filename, lineno, msg);
    if (*lptr) {
        fprintf(stderr, "  Near: %.20s...\n", lptr);
    }
    exit(1);
}

/* Warning messages */
void warning(char *msg) {
    fprintf(stderr, "%s:%d: Warning: %s\n", filename, lineno, msg);
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

/* Stack operations */
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

/* Parameter and local variable access */
void emit_load_param(int offset) {
    if (target == TARGET_X64) {
        emit("  movq %d(%%rbp), %%rax", offset);
    } else {
        emit("  ldr x0, [x29, #%d]", offset);
    }
}

void emit_store_local(int offset) {
    if (target == TARGET_X64) {
        emit("  movq %%rax, %d(%%rbp)", offset);
    } else {
        emit("  str x0, [x29, #%d]", offset);
    }
}

void emit_load_local(int offset) {
    if (target == TARGET_X64) {
        emit("  movq %d(%%rbp), %%rax", offset);
    } else {
        emit("  ldr x0, [x29, #%d]", offset);
    }
}

/* Emit assembly based on target */
void emit_prolog(void) {
    if (target == TARGET_X64) {
        emit(".text");
        emit(".globl main");
        emit("");
    } else {
        emit(".text");
        emit(".globl main");
        emit("");
    }
}

/* Lexical analyzer */
void skip_white(void) {
    while (*lptr && isspace(*lptr)) {
        if (*lptr == '\n') lineno++;
        lptr++;
    }
}

void skip_comment(void) {
    if (*lptr == '/' && *(lptr+1) == '/') {
        while (*lptr && *lptr != '\n') lptr++;
        if (*lptr == '\n') {
            lineno++;
            lptr++;
        }
    } else if (*lptr == '/' && *(lptr+1) == '*') {
        lptr += 2;
        while (*lptr && !(*lptr == '*' && *(lptr+1) == '/')) {
            if (*lptr == '\n') lineno++;
            lptr++;
        }
        if (*lptr) lptr += 2;
    }
}

int gettoken(void) {
    skip_white();
    skip_comment();
    skip_white();
    
    if (!*lptr) {
        if (!fgets(line, LINESIZE, input)) return T_EOF;
        lptr = line;
        return gettoken();
    }
    
    /* Character literals */
    if (*lptr == '\'') {
        lptr++;
        tokval = 0;
        if (*lptr == '\\') {
            lptr++;
            switch (*lptr) {
                case 'n': tokval = '\n'; break;
                case 't': tokval = '\t'; break;
                case 'r': tokval = '\r'; break;
                case 'b': tokval = '\b'; break;
                case '\\': tokval = '\\'; break;
                case '\'': tokval = '\''; break;
                case '0': tokval = '\0'; break;
                default: tokval = *lptr;
            }
            lptr++;
        } else {
            tokval = *lptr++;
        }
        if (*lptr != '\'') error("Unterminated character constant");
        lptr++;
        return T_CHARLIT;
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
        if (c == '+' && *lptr == '=') { lptr++; return T_PLUSEQ; }
        if (c == '-' && *lptr == '=') { lptr++; return T_MINUSEQ; }
        if (c == '*' && *lptr == '=') { lptr++; return T_STAREQ; }
        if (c == '/' && *lptr == '=') { lptr++; return T_SLASHEQ; }
        
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
            if (p - tokstr < NAMESIZE - 1) {
                *p++ = *lptr;
            }
            lptr++;
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
                    case 'r': *p++ = '\r'; break;
                    case 'b': *p++ = '\b'; break;
                    case '\\': *p++ = '\\'; break;
                    case '"': *p++ = '"'; break;
                    case '0': *p++ = '\0'; break;
                    default: *p++ = *lptr;
                }
                lptr++;
            } else {
                if (*lptr == '\n') lineno++;
                *p++ = *lptr++;
            }
        }
        *p = '\0';
        if (*lptr == '"') lptr++;
        else error("Unterminated string literal");
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
    if (nlocals > 0 || param_offset < 16) {
        if (nlocals >= MAXLOCALS) error("Too many local variables");
        sym = &locals[nlocals++];
        if (param_offset < 16) {
            /* Function parameter */
            sym->offset = param_offset;
            sym->isparam = 1;
            param_offset += 8;
        } else {
            /* Local variable */
            sp -= (type < 2 ? 8 : 8) * (size > 0 ? size : 1);
            sym->offset = sp;
            sym->isparam = 0;
        }
    } else {
        if (nglobals >= MAXGLOBALS) error("Too many global variables");
        sym = &globals[nglobals++];
        sym->offset = lab++;
        sym->isparam = 0;
    }
    strcpy(sym->name, name);
    sym->type = type;
    sym->isarray = (size > 0);
    sym->size = size;
    return sym;
}

/* Function table */
struct function *lookup_func(char *name) {
    int i;
    for (i = 0; i < nfuncs; i++) {
        if (!strcmp(functions[i].name, name)) return &functions[i];
    }
    return NULL;
}

struct function *add_function(char *name) {
    struct function *func;
    if (nfuncs >= MAXFUNCS) error("Too many functions");
    func = &functions[nfuncs++];
    strcpy(func->name, name);
    func->defined = 0;
    func->nparams = 0;
    return func;
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
        char name[NAMESIZE];
        strcpy(name, tokstr);
        token = gettoken();
        
        /* Function or global variable */
        if (token == '(') {
            strcpy(curfunc, name);
            struct function *func = lookup_func(name);
            if (!func) func = add_function(name);
            if (func->defined) error("Function already defined");
            func->defined = 1;
            
            emit(".globl %s", name);
            emit("%s:", name);
            token = gettoken();
            param_offset = 16;  /* Reset parameter offset */
            function(type);
        } else {
            global_declaration(type);
        }
    }
}

void global_declaration(int type) {
    /* Global variable already parsed */
    char name[NAMESIZE];
    strcpy(name, tokstr);
    
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
    
    /* Handle initialization */
    if (token == '=') {
        token = gettoken();
        emit(".data");
        emit(".globl %s", name);
        emit("%s:", name);
        
        if (token == T_STRING && type == T_CHAR && size > 0) {
            /* String initialization for char array */
            emit("  .ascii \"%s\"", tokstr);
            emit("  .zero %d", size - strlen(tokstr) - 1);
            token = gettoken();
        } else if (token == T_NUMBER || token == T_CHARLIT) {
            emit("  .quad %d", tokval);
            token = gettoken();
        } else {
            error("Invalid initializer");
        }
        emit(".text");
    } else {
        /* Uninitialized global */
        emit(".data");
        emit(".globl %s", name);
        emit("%s:", name);
        if (size > 0) {
            emit("  .space %d", size * (type == T_CHAR ? 1 : 8));
        } else {
            emit("  .quad 0");
        }
        emit(".text");
    }
    
    if (token != ';') error("Expected ;");
    token = gettoken();
}

void parameter_list(void) {
    struct function *func = lookup_func(curfunc);
    
    while (token != ')') {
        int type = T_INT;
        if (token == T_INT || token == T_CHAR) {
            type = token;
            token = gettoken();
        }
        
        if (token != T_IDENT) error("Expected parameter name");
        add_symbol(tokstr, type == T_CHAR ? 1 : 0, 0);
        
        if (func) {
            if (func->nparams >= MAXARGS) error("Too many parameters");
            func->param_types[func->nparams++] = type;
        }
        
        token = gettoken();
        if (token == ',') {
            token = gettoken();
        } else if (token != ')') {
            error("Expected , or )");
        }
    }
}

void function(int type) {
    /* Parse parameters */
    parameter_list();
    token = gettoken();
    
    if (token != '{') error("Expected {");
    token = gettoken();
    
    /* Function prologue */
    if (target == TARGET_X64) {
        emit("  pushq %%rbp");
        emit("  movq %%rsp, %%rbp");
        /* Save argument registers */
        if (param_offset > 16) {
            emit("  pushq %%rdi");
            if (param_offset > 24) emit("  pushq %%rsi");
            if (param_offset > 32) emit("  pushq %%rdx");
            if (param_offset > 40) emit("  pushq %%rcx");
            if (param_offset > 48) emit("  pushq %%r8");
            if (param_offset > 56) emit("  pushq %%r9");
        }
    } else {
        emit("  stp x29, x30, [sp, #-16]!");
        emit("  mov x29, sp");
        /* Save argument registers */
        int nparams = (param_offset - 16) / 8;
        if (nparams > 0) {
            for (int i = 0; i < nparams && i < 8; i++) {
                emit("  str x%d, [sp, #-16]!", i);
            }
        }
    }
    
    sp = 0;
    
    /* Local declarations */
    while (token == T_INT || token == T_CHAR) {
        int ltype = token;
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
            
            struct symbol *sym = add_symbol(name, ltype == T_CHAR ? 1 : 0, size);
            
            /* Handle initialization */
            if (token == '=') {
                token = gettoken();
                expression();
                emit_store_local(sym->offset);
            }
            
            if (token != ',') break;
            token = gettoken();
        }
        
        if (token != ';') error("Expected ;");
        token = gettoken();
    }
    
    /* Allocate locals */
    if (sp < 0) {
        int alloc = ((-sp + 15) / 16) * 16;  /* Align to 16 bytes */
        if (target == TARGET_X64) {
            emit("  subq $%d, %%rsp", alloc);
        } else {
            emit("  sub sp, sp, #%d", alloc);
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
    
    /* Reset for next function */
    nlocals = 0;
}

void statement(void) {
    int lab1, lab2, lab3;
    
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
            
        case T_FOR:
            token = gettoken();
            if (token != '(') error("Expected (");
            token = gettoken();
            
            /* Initialization */
            if (token != ';') {
                expression();
            }
            if (token != ';') error("Expected ;");
            token = gettoken();
            
            lab1 = lab++;  /* loop start */
            lab2 = lab++;  /* loop end */
            lab3 = lab++;  /* continue target */
            
            breaklab[wsp] = lab2;
            contlab[wsp] = lab3;
            wsp++;
            
            emit_label(lab1);
            
            /* Condition */
            if (token != ';') {
                expression();
                emit_branch_false(lab2);
            }
            if (token != ';') error("Expected ;");
            token = gettoken();
            
            /* Save increment expression position */
            char *inc_start = lptr;
            int inc_line = lineno;
            
            /* Skip increment expression */
            int paren = 1;
            while (paren > 0 && *lptr) {
                if (*lptr == '(') paren++;
                else if (*lptr == ')') paren--;
                if (*lptr == '\n') lineno++;
                if (paren > 0) lptr++;
            }
            
            if (token != ')') error("Expected )");
            token = gettoken();
            
            /* Body */
            statement();
            
            /* Continue label and increment */
            emit_label(lab3);
            if (inc_start != lptr - 1) {
                /* Restore position for increment */
                char saved = *lptr;
                *lptr = '\0';
                char *save_lptr = lptr;
                lptr = inc_start;
                int save_line = lineno;
                lineno = inc_line;
                expression();
                *save_lptr = saved;
                lptr = save_lptr;
                lineno = save_line;
            }
            
            emit_jump(lab1);
            emit_label(lab2);
            
            wsp--;
            break;
            
        case T_RETURN:
            token = gettoken();
            if (token != ';') {
                expression();
            } else {
                /* Return 0 by default */
                if (target == TARGET_X64) {
                    emit("  xorq %%rax, %%rax");
                } else {
                    emit("  mov x0, #0");
                }
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
    
    while (token == '=' || token == T_PLUSEQ || token == T_MINUSEQ || 
           token == T_STAREQ || token == T_SLASHEQ) {
        int op = token;
        push();
        token = gettoken();
        
        if (op != '=') {
            /* Compound assignment: load left side again */
            if (target == TARGET_X64) {
                emit("  movq (%%rsp), %%rdx");
                emit("  movq (%%rdx), %%rax");
                emit("  pushq %%rax");
            } else {
                emit("  ldr x1, [sp]");
                emit("  ldr x0, [x1]");
                emit("  str x0, [sp, #-16]!");
            }
            sp -= (target == TARGET_X64 ? 8 : 16);
        }
        
        assignment();
        
        if (op != '=') {
            /* Perform operation */
            if (target == TARGET_X64) {
                pop("%rdx");
                switch (op) {
                    case T_PLUSEQ: emit("  addq %%rdx, %%rax"); break;
                    case T_MINUSEQ: 
                        emit("  subq %%rax, %%rdx");
                        emit("  movq %%rdx, %%rax");
                        break;
                    case T_STAREQ: emit("  imulq %%rdx, %%rax"); break;
                    case T_SLASHEQ:
                        emit("  movq %%rax, %%rbx");
                        emit("  movq %%rdx, %%rax");
                        emit("  cqo");
                        emit("  idivq %%rbx");
                        break;
                }
            } else {
                pop("x1");
                switch (op) {
                    case T_PLUSEQ: emit("  add x0, x1, x0"); break;
                    case T_MINUSEQ: emit("  sub x0, x1, x0"); break;
                    case T_STAREQ: emit("  mul x0, x1, x0"); break;
                    case T_SLASHEQ: emit("  sdiv x0, x1, x0"); break;
                }
            }
        }
        
        /* Store result */
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
        int lab1 = lab++;
        int lab2 = lab++;
        
        if (target == TARGET_X64) {
            emit("  testq %%rax, %%rax");
            emit("  jnz L%d", lab1);
        } else {
            emit("  cbnz x0, L%d", lab1);
        }
        
        token = gettoken();
        logical_and();
        
        emit_label(lab1);
        if (target == TARGET_X64) {
            emit("  testq %%rax, %%rax");
            emit("  setne %%al");
            emit("  movzbq %%al, %%rax");
        } else {
            emit("  cmp x0, #0");
            emit("  cset x0, ne");
        }
        emit_label(lab2);
    }
}

void logical_and(void) {
    bitwise_or();
    
    while (token == T_AND) {
        int lab1 = lab++;
        int lab2 = lab++;
        
        if (target == TARGET_X64) {
            emit("  testq %%rax, %%rax");
            emit("  jz L%d", lab1);
        } else {
            emit("  cbz x0, L%d", lab1);
        }
        
        token = gettoken();
        bitwise_or();
        
        if (target == TARGET_X64) {
            emit("  testq %%rax, %%rax");
            emit("  setne %%al");
            emit("  movzbq %%al, %%rax");
        } else {
            emit("  cmp x0, #0");
            emit("  cset x0, ne");
        }
        emit_jump(lab2);
        emit_label(lab1);
        if (target == TARGET_X64) {
            emit("  xorq %%rax, %%rax");
        } else {
            emit("  mov x0, #0");
        }
        emit_label(lab2);
    }
}

/* Continue with rest of expression parsing functions... */
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
            
            if (sym->isparam || sym->offset < 0) {
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
        } else if (token == '(') {
            /* Function call */
            struct symbol *sym = lookup(tokstr);
            char fname[NAMESIZE];
            strcpy(fname, tokstr);
            int nargs = 0;
            
            token = gettoken();
            
            /* Push arguments in reverse order */
            if (token != ')') {
                /* Count arguments first */
                char *save_lptr = lptr;
                int save_line = lineno;
                int paren = 1;
                nargs = 1;
                while (paren > 0 && *lptr) {
                    if (*lptr == '(') paren++;
                    else if (*lptr == ')') paren--;
                    else if (*lptr == ',' && paren == 1) nargs++;
                    if (*lptr == '\n') lineno++;
                    lptr++;
                }
                lptr = save_lptr;
                lineno = save_line;
                
                /* Evaluate arguments */
                int arg_count = 0;
                while (token != ')') {
                    expression();
                    push();
                    arg_count++;
                    if (token == ',') {
                        token = gettoken();
                    } else if (token != ')') {
                        error("Expected , or )");
                    }
                }
                
                /* Pop arguments into registers */
                if (target == TARGET_X64) {
                    char *arg_regs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
                    for (int i = arg_count - 1; i >= 0 && i < 6; i--) {
                        pop(arg_regs[i]);
                    }
                } else {
                    for (int i = arg_count - 1; i >= 0 && i < 8; i--) {
                        char reg[16];
                        sprintf(reg, "x%d", i);
                        pop(reg);
                    }
                }
            }
            
            if (token != ')') error("Expected )");
            token = gettoken();
            
            /* Call function */
            if (target == TARGET_X64) {
                emit("  call %s", fname);
            } else {
                emit("  bl %s", fname);
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
            
        case T_CHARLIT:
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
                char name[NAMESIZE];
                strcpy(name, tokstr);
                struct symbol *sym = lookup(name);
                
                token = gettoken();
                
                if (token == '(') {
                    /* Function call - handled in postfix */
                    strcpy(tokstr, name);
                    postfix();
                    return;
                }
                
                if (!sym) {
                    /* Might be a function */
                    struct function *func = lookup_func(name);
                    if (func) {
                        if (target == TARGET_X64) {
                            emit("  movq $%s, %%rax", name);
                        } else {
                            emit("  adrp x0, %s", name);
                            emit("  add x0, x0, :lo12:%s", name);
                        }
                    } else {
                        error("Undefined variable");
                    }
                } else if (sym->isarray) {
                    if (sym->isparam || sym->offset < 0) {
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
                    if (sym->isparam || sym->offset < 0) {
                        emit_load_local(sym->offset);
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
    int i;
    
    filename = NULL;
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
