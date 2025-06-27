/*
 * Small-C Assembler (sas)
 * A minimal assembler for x64 and ARM64 that can assemble
 * the output from the Small-C compilers
 * 
 * Written in Small-C compatible subset
 * Can be compiled with: ./scc sas.c > sas.s
 * 
 * Usage: sas [-arm64|-x64] input.s -o output.o
 */

/* Configuration */
#define MAXSYMS 1000
#define MAXRELS 1000
#define MAXCODE 65536
#define MAXLINE 256
#define NAMESIZE 32

/* Target architecture */
#define TARGET_X64 0
#define TARGET_ARM64 1

/* Section types */
#define SEC_TEXT 0
#define SEC_DATA 1

/* Symbol types */
#define SYM_LOCAL 0
#define SYM_GLOBAL 1
#define SYM_EXTERN 2

/* Relocation types */
#define REL_ABS 0
#define REL_PC32 1
#define REL_CALL 2

/* Global variables */
int target = TARGET_X64;
int pass;
int section = SEC_TEXT;
int pc;  /* Program counter */

/* Code buffer */
char code[MAXCODE];
int codesize;

/* Symbol table */
char symname[MAXSYMS][NAMESIZE];
int symval[MAXSYMS];
int symtype[MAXSYMS];
int symsec[MAXSYMS];
int nsyms;

/* Relocation table */
int reloff[MAXRELS];
int relsym[MAXRELS];
int reltype[MAXRELS];
int nrels;

/* Input handling */
char line[MAXLINE];
char *lptr;
char token[NAMESIZE];
int tokval;

/* Output file */
int outfd;

/* Forward declarations */
int assemble_file(char *filename);
int process_line(void);
int parse_instruction(void);
int parse_directive(void);
int emit_byte(int b);
int emit_word(int w);
int emit_dword(int d);
int emit_qword(int q);
int get_token(void);
int get_number(void);
int lookup_sym(char *name);
int add_sym(char *name, int val, int type, int sec);
int add_reloc(int off, int sym, int type);
void write_elf_header(void);
void write_sections(void);
void error(char *msg);

/* Skip whitespace */
int skip_white(void) {
    while (*lptr == ' ' || *lptr == '\t') {
        lptr++;
    }
    return 0;
}

/* Get next token */
int get_token(void) {
    char *p;
    
    skip_white();
    
    if (*lptr == '\0' || *lptr == '\n') {
        token[0] = '\0';
        return 0;
    }
    
    /* Comments */
    if (*lptr == '#' || *lptr == ';') {
        token[0] = '\0';
        return 0;
    }
    
    /* Numbers */
    if (*lptr == '$' || (*lptr >= '0' && *lptr <= '9') || *lptr == '-') {
        return get_number();
    }
    
    /* Labels and identifiers */
    p = token;
    if ((*lptr >= 'A' && *lptr <= 'Z') || 
        (*lptr >= 'a' && *lptr <= 'z') || 
        *lptr == '_' || *lptr == '.') {
        
        while ((*lptr >= 'A' && *lptr <= 'Z') || 
               (*lptr >= 'a' && *lptr <= 'z') || 
               (*lptr >= '0' && *lptr <= '9') ||
               *lptr == '_' || *lptr == '.') {
            *p++ = *lptr++;
        }
        *p = '\0';
        
        /* Check for label */
        skip_white();
        if (*lptr == ':') {
            lptr++;
            return ':';
        }
        return 'I';  /* Identifier */
    }
    
    /* Single character tokens */
    if (*lptr == ',' || *lptr == '(' || *lptr == ')' || 
        *lptr == '[' || *lptr == ']' || *lptr == '!' ||
        *lptr == '+' || *lptr == '-' || *lptr == ':') {
        token[0] = *lptr;
        token[1] = '\0';
        return *lptr++;
    }
    
    /* Register prefix */
    if (*lptr == '%' || *lptr == 'x' || *lptr == 'w' || *lptr == 'r') {
        p = token;
        while (*lptr && *lptr != ' ' && *lptr != '\t' && 
               *lptr != ',' && *lptr != ')' && *lptr != '\n') {
            *p++ = *lptr++;
        }
        *p = '\0';
        return 'R';  /* Register */
    }
    
    error("Unknown character");
    return 0;
}

/* Get number */
int get_number(void) {
    int neg = 0;
    int base = 10;
    
    tokval = 0;
    
    if (*lptr == '-') {
        neg = 1;
        lptr++;
    }
    
    if (*lptr == '$') {
        lptr++;
        base = 16;
    } else if (*lptr == '0' && *(lptr+1) == 'x') {
        lptr += 2;
        base = 16;
    }
    
    while (1) {
        if (*lptr >= '0' && *lptr <= '9') {
            tokval = tokval * base + (*lptr - '0');
        } else if (base == 16 && *lptr >= 'a' && *lptr <= 'f') {
            tokval = tokval * base + (*lptr - 'a' + 10);
        } else if (base == 16 && *lptr >= 'A' && *lptr <= 'F') {
            tokval = tokval * base + (*lptr - 'A' + 10);
        } else {
            break;
        }
        lptr++;
    }
    
    if (neg) tokval = -tokval;
    return 'N';  /* Number */
}

/* Emit bytes to code buffer */
int emit_byte(int b) {
    if (codesize >= MAXCODE) {
        error("Code buffer overflow");
        return -1;
    }
    code[codesize++] = b & 0xFF;
    pc++;
    return 0;
}

int emit_word(int w) {
    emit_byte(w & 0xFF);
    emit_byte((w >> 8) & 0xFF);
    return 0;
}

int emit_dword(int d) {
    emit_byte(d & 0xFF);
    emit_byte((d >> 8) & 0xFF);
    emit_byte((d >> 16) & 0xFF);
    emit_byte((d >> 24) & 0xFF);
    return 0;
}

/* Emit 64-bit value (as two 32-bit values for Small-C compatibility) */
int emit_qword(int low, int high) {
    emit_dword(low);
    emit_dword(high);
    return 0;
}

/* Symbol table */
int lookup_sym(char *name) {
    int i;
    for (i = 0; i < nsyms; i++) {
        if (strcmp(symname[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

int add_sym(char *name, int val, int type, int sec) {
    int i;
    
    i = lookup_sym(name);
    if (i >= 0) {
        /* Update existing symbol */
        if (pass == 2 && symval[i] != val) {
            error("Symbol value changed between passes");
        }
        symval[i] = val;
        if (type == SYM_GLOBAL) {
            symtype[i] = SYM_GLOBAL;
        }
        return i;
    }
    
    if (nsyms >= MAXSYMS) {
        error("Too many symbols");
        return -1;
    }
    
    strcpy(symname[nsyms], name);
    symval[nsyms] = val;
    symtype[nsyms] = type;
    symsec[nsyms] = sec;
    return nsyms++;
}

/* Relocations */
int add_reloc(int off, int sym, int type) {
    if (pass != 2) return 0;
    
    if (nrels >= MAXRELS) {
        error("Too many relocations");
        return -1;
    }
    
    reloff[nrels] = off;
    relsym[nrels] = sym;
    reltype[nrels] = type;
    nrels++;
    return 0;
}

/* x64 instruction encoding */
int encode_x64_mov(char *dst, char *src) {
    /* Simple movq encoding for common cases */
    if (dst[0] == '%' && src[0] == '$') {
        /* movq $imm, %reg */
        if (strcmp(dst, "%rax") == 0) {
            emit_byte(0x48);  /* REX.W */
            emit_byte(0xB8);  /* MOV RAX, imm64 */
            emit_qword(tokval, 0);
            return 0;
        }
        /* Add other registers as needed */
    }
    
    /* Add more mov variants as needed */
    error("Unsupported mov instruction");
    return -1;
}

int encode_x64_push(char *reg) {
    if (strcmp(reg, "%rax") == 0) {
        emit_byte(0x50);  /* PUSH RAX */
        return 0;
    } else if (strcmp(reg, "%rbp") == 0) {
        emit_byte(0x55);  /* PUSH RBP */
        return 0;
    }
    /* Add other registers */
    error("Unsupported push register");
    return -1;
}

int encode_x64_pop(char *reg) {
    if (strcmp(reg, "%rax") == 0) {
        emit_byte(0x58);  /* POP RAX */
        return 0;
    } else if (strcmp(reg, "%rbp") == 0) {
        emit_byte(0x5D);  /* POP RBP */
        return 0;
    }
    /* Add other registers */
    error("Unsupported pop register");
    return -1;
}

int encode_x64_call(char *target) {
    int sym;
    
    emit_byte(0xE8);  /* CALL rel32 */
    
    sym = lookup_sym(target);
    if (sym < 0) {
        sym = add_sym(target, 0, SYM_EXTERN, SEC_TEXT);
    }
    
    add_reloc(pc, sym, REL_CALL);
    emit_dword(0);  /* Placeholder for relocation */
    return 0;
}

int encode_x64_ret(void) {
    emit_byte(0xC3);  /* RET */
    return 0;
}

/* ARM64 instruction encoding */
int encode_arm64_mov(char *dst, char *src) {
    /* Simplified ARM64 mov encoding */
    if (dst[0] == 'x' && src[0] == '#') {
        int reg = dst[1] - '0';
        int imm = tokval;
        
        /* MOV (immediate) */
        emit_dword(0xD2800000 | (imm << 5) | reg);
        return 0;
    }
    
    error("Unsupported ARM64 mov");
    return -1;
}

int encode_arm64_bl(char *target) {
    int sym;
    
    sym = lookup_sym(target);
    if (sym < 0) {
        sym = add_sym(target, 0, SYM_EXTERN, SEC_TEXT);
    }
    
    add_reloc(pc, sym, REL_CALL);
    emit_dword(0x94000000);  /* BL placeholder */
    return 0;
}

int encode_arm64_ret(void) {
    emit_dword(0xD65F03C0);  /* RET */
    return 0;
}

/* Parse instruction */
int parse_instruction(void) {
    char opcode[32];
    char op1[32], op2[32];
    int tok;
    
    strcpy(opcode, token);
    
    /* Get operands */
    op1[0] = op2[0] = '\0';
    
    tok = get_token();
    if (tok) {
        strcpy(op1, token);
        
        tok = get_token();
        if (tok == ',') {
            tok = get_token();
            if (tok) {
                strcpy(op2, token);
            }
        }
    }
    
    /* Encode based on architecture */
    if (target == TARGET_X64) {
        /* x64 instructions */
        if (strcmp(opcode, "movq") == 0) {
            return encode_x64_mov(op2, op1);
        } else if (strcmp(opcode, "pushq") == 0) {
            return encode_x64_push(op1);
        } else if (strcmp(opcode, "popq") == 0) {
            return encode_x64_pop(op1);
        } else if (strcmp(opcode, "call") == 0) {
            return encode_x64_call(op1);
        } else if (strcmp(opcode, "ret") == 0) {
            return encode_x64_ret();
        }
        /* Add more x64 instructions */
        
    } else {
        /* ARM64 instructions */
        if (strcmp(opcode, "mov") == 0) {
            return encode_arm64_mov(op1, op2);
        } else if (strcmp(opcode, "bl") == 0) {
            return encode_arm64_bl(op1);
        } else if (strcmp(opcode, "ret") == 0) {
            return encode_arm64_ret();
        }
        /* Add more ARM64 instructions */
    }
    
    error("Unknown instruction");
    return -1;
}

/* Parse directive */
int parse_directive(void) {
    if (strcmp(token, ".text") == 0) {
        section = SEC_TEXT;
        return 0;
    } else if (strcmp(token, ".data") == 0) {
        section = SEC_DATA;
        return 0;
    } else if (strcmp(token, ".globl") == 0 || strcmp(token, ".global") == 0) {
        get_token();
        add_sym(token, 0, SYM_GLOBAL, section);
        return 0;
    } else if (strcmp(token, ".extern") == 0) {
        get_token();
        add_sym(token, 0, SYM_EXTERN, section);
        return 0;
    } else if (strcmp(token, ".byte") == 0) {
        while (get_token() == 'N') {
            emit_byte(tokval);
            if (get_token() != ',') break;
        }
        return 0;
    } else if (strcmp(token, ".word") == 0) {
        while (get_token() == 'N') {
            emit_word(tokval);
            if (get_token() != ',') break;
        }
        return 0;
    } else if (strcmp(token, ".long") == 0 || strcmp(token, ".int") == 0) {
        while (get_token() == 'N') {
            emit_dword(tokval);
            if (get_token() != ',') break;
        }
        return 0;
    } else if (strcmp(token, ".quad") == 0) {
        while (get_token() == 'N') {
            emit_qword(tokval, 0);
            if (get_token() != ',') break;
        }
        return 0;
    } else if (strcmp(token, ".ascii") == 0 || strcmp(token, ".asciz") == 0) {
        int addnull = (strcmp(token, ".asciz") == 0);
        skip_white();
        if (*lptr == '"') {
            lptr++;
            while (*lptr && *lptr != '"') {
                if (*lptr == '\\') {
                    lptr++;
                    switch (*lptr) {
                        case 'n': emit_byte('\n'); break;
                        case 't': emit_byte('\t'); break;
                        case 'r': emit_byte('\r'); break;
                        case '\\': emit_byte('\\'); break;
                        case '"': emit_byte('"'); break;
                        default: emit_byte(*lptr);
                    }
                } else {
                    emit_byte(*lptr);
                }
                lptr++;
            }
            if (*lptr == '"') lptr++;
            if (addnull) emit_byte(0);
        }
        return 0;
    } else if (strcmp(token, ".space") == 0 || strcmp(token, ".zero") == 0) {
        int count;
        get_token();
        count = tokval;
        while (count-- > 0) {
            emit_byte(0);
        }
        return 0;
    }
    
    error("Unknown directive");
    return -1;
}

/* Process one line */
int process_line(void) {
    int tok;
    
    lptr = line;
    tok = get_token();
    
    if (tok == 0) {
        return 0;  /* Empty line */
    }
    
    if (tok == ':') {
        /* Label */
        add_sym(token, pc, SYM_LOCAL, section);
        tok = get_token();
    }
    
    if (tok == 0) {
        return 0;  /* Just a label */
    }
    
    if (token[0] == '.') {
        /* Directive */
        return parse_directive();
    } else {
        /* Instruction */
        return parse_instruction();
    }
}

/* Simple ELF header writer (simplified) */
void write_elf_header(void) {
    /* ELF magic */
    write(outfd, "\x7F" "ELF", 4);
    
    /* ELF class, data, version */
    if (target == TARGET_X64) {
        write(outfd, "\x02\x01\x01\x00", 4);  /* 64-bit, little-endian */
    } else {
        write(outfd, "\x02\x01\x01\x00", 4);  /* 64-bit ARM64 */
    }
    
    /* Padding */
    write(outfd, "\x00\x00\x00\x00\x00\x00\x00\x00", 8);
    
    /* e_type, e_machine */
    write(outfd, "\x01\x00", 2);  /* ET_REL */
    if (target == TARGET_X64) {
        write(outfd, "\x3E\x00", 2);  /* EM_X86_64 */
    } else {
        write(outfd, "\xB7\x00", 2);  /* EM_AARCH64 */
    }
    
    /* Simplified - would need full ELF implementation */
}

/* Assemble file */
int assemble_file(char *filename) {
    int fd;
    char c;
    int i;
    
    fd = open(filename, 0);
    if (fd < 0) {
        printf("Cannot open %s\n", filename);
        return -1;
    }
    
    /* Two-pass assembly */
    for (pass = 1; pass <= 2; pass++) {
        pc = 0;
        codesize = 0;
        
        /* Reset file position */
        close(fd);
        fd = open(filename, 0);
        
        /* Read line by line */
        i = 0;
        while (read(fd, &c, 1) == 1) {
            if (c == '\n') {
                line[i] = '\0';
                process_line();
                i = 0;
            } else if (i < MAXLINE - 1) {
                line[i++] = c;
            }
        }
        
        if (i > 0) {
            line[i] = '\0';
            process_line();
        }
    }
    
    close(fd);
    return 0;
}

/* Error handling */
void error(char *msg) {
    printf("Error: %s\n", msg);
    if (token[0]) {
        printf("  Near: %s\n", token);
    }
}

/* Main */
int main(int argc, char **argv) {
    char *infile = 0;
    char *outfile = 0;
    int i;
    
    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-x64") == 0) {
            target = TARGET_X64;
        } else if (strcmp(argv[i], "-arm64") == 0) {
            target = TARGET_ARM64;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outfile = argv[++i];
        } else if (argv[i][0] != '-') {
            infile = argv[i];
        }
    }
    
    if (!infile || !outfile) {
        printf("Usage: sas [-x64|-arm64] input.s -o output.o\n");
        return 1;
    }
    
    /* Initialize */
    nsyms = 0;
    nrels = 0;
    
    /* Assemble */
    if (assemble_file(infile) < 0) {
        return 1;
    }
    
    /* Write output */
    outfd = creat(outfile);
    if (outfd < 0) {
        printf("Cannot create %s\n", outfile);
        return 1;
    }
    
    write_elf_header();
    write(outfd, code, codesize);
    
    close(outfd);
    
    printf("Assembled %d bytes\n", codesize);
    return 0;
}
