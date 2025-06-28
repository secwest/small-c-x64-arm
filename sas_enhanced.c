/* sas_enhanced.c - Enhanced General-Purpose Assembler for x64 and ARM64 */
/* A more complete assembler that can be compiled by Small-C */

/* Maximum limits */
#define MAX_SYMBOLS 4096
#define MAX_SECTIONS 32
#define MAX_RELOCS 8192
#define MAX_INSTRUCTIONS 16384
#define MAX_LINE 512
#define MAX_NAME 128
#define MAX_OPERANDS 4
#define MAX_EXPR_DEPTH 16
#define MAX_MACROS 256
#define MAX_MACRO_PARAMS 8
#define MAX_MACRO_LINES 64
#define MAX_INCLUDES 16

/* Output buffer */
#define OUTPUT_SIZE 1048576
char output[OUTPUT_SIZE];
int output_size;

/* Architecture */
int is_arm64 = 0;

/* Current section */
int current_section = 0;
int current_offset = 0;
int line_number = 0;
char current_file[MAX_NAME];

/* Sections */
char section_names[MAX_SECTIONS][MAX_NAME];
int section_offsets[MAX_SECTIONS];
int section_sizes[MAX_SECTIONS];
int section_flags[MAX_SECTIONS];
int section_aligns[MAX_SECTIONS];
char *section_data[MAX_SECTIONS];
int section_count;

/* Section flags */
#define SEC_CODE 1
#define SEC_DATA 2
#define SEC_BSS 4
#define SEC_READONLY 8

/* Symbols */
char symbol_names[MAX_SYMBOLS][MAX_NAME];
int symbol_values[MAX_SYMBOLS];
int symbol_sections[MAX_SYMBOLS];
int symbol_types[MAX_SYMBOLS];
int symbol_defined[MAX_SYMBOLS];
int symbol_count;

/* Symbol types */
#define SYM_LOCAL 0
#define SYM_GLOBAL 1
#define SYM_WEAK 2
#define SYM_EXTERN 3

/* Relocations */
int reloc_offsets[MAX_RELOCS];
int reloc_symbols[MAX_RELOCS];
int reloc_types[MAX_RELOCS];
int reloc_sections[MAX_RELOCS];
int reloc_addends[MAX_RELOCS];
int reloc_count;

/* Macros */
char macro_names[MAX_MACROS][MAX_NAME];
char macro_params[MAX_MACROS][MAX_MACRO_PARAMS][MAX_NAME];
char macro_lines[MAX_MACROS][MAX_MACRO_LINES][MAX_LINE];
int macro_param_counts[MAX_MACROS];
int macro_line_counts[MAX_MACROS];
int macro_count;

/* Expression evaluation stack */
int expr_stack[MAX_EXPR_DEPTH];
int expr_stack_ptr;

/* x64 Registers */
char *x64_regs_64[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", NULL
};

char *x64_regs_32[] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", NULL
};

char *x64_regs_16[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w", NULL
};

char *x64_regs_8[] = {
    "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b", NULL
};

/* ARM64 Registers */
char *arm64_regs[] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp",
    "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7",
    "w8", "w9", "w10", "w11", "w12", "w13", "w14", "w15",
    "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
    "w24", "w25", "w26", "w27", "w28", "w29", "w30", "wzr",
    "xzr", "fp", "lr", NULL
};

/* Helper functions */
int streq(char *a, char *b) {
    return strcmp(a, b) == 0;
}

int starts_with(char *str, char *prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++; prefix++;
    }
    return 1;
}

void error(char *msg) {
    printf("Error at %s:%d: %s\n", current_file, line_number, msg);
    exit(1);
}

/* String to number conversion */
int parse_number(char *str) {
    int base = 10;
    int value = 0;
    int neg = 0;
    
    if (*str == '-') {
        neg = 1;
        str++;
    }
    
    if (*str == '0') {
        str++;
        if (*str == 'x' || *str == 'X') {
            base = 16;
            str++;
        } else if (*str == 'b' || *str == 'B') {
            base = 2;
            str++;
        } else if (*str >= '0' && *str <= '7') {
            base = 8;
        }
    }
    
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') {
            digit = *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            digit = *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            digit = *str - 'A' + 10;
        } else {
            break;
        }
        
        if (digit >= base) break;
        value = value * base + digit;
        str++;
    }
    
    return neg ? -value : value;
}

/* Skip whitespace */
char *skip_space(char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

/* Get next token */
char *get_token(char *line, char *buf) {
    line = skip_space(line);
    
    if (*line == '"') {
        /* String literal */
        line++;
        while (*line && *line != '"') {
            if (*line == '\\' && line[1]) {
                line++;
                switch (*line) {
                case 'n': *buf++ = '\n'; break;
                case 't': *buf++ = '\t'; break;
                case 'r': *buf++ = '\r'; break;
                case '\\': *buf++ = '\\'; break;
                case '"': *buf++ = '"'; break;
                default: *buf++ = *line; break;
                }
                line++;
            } else {
                *buf++ = *line++;
            }
        }
        if (*line == '"') line++;
    } else if (*line == '\'' && line[1] && line[2] == '\'') {
        /* Character literal */
        *buf++ = line[1];
        line += 3;
    } else {
        /* Regular token */
        while (*line && *line != ' ' && *line != '\t' && 
               *line != ',' && *line != ':' && *line != ';' &&
               *line != '[' && *line != ']' && *line != '(' && 
               *line != ')' && *line != '+' && *line != '-' &&
               *line != '*' && *line != '/' && *line != '#') {
            *buf++ = *line++;
        }
    }
    
    *buf = 0;
    return line;
}

/* Find or add section */
int find_section(char *name) {
    int i;
    for (i = 0; i < section_count; i++) {
        if (streq(section_names[i], name)) {
            return i;
        }
    }
    return -1;
}

int add_section(char *name, int flags, int align) {
    int idx = find_section(name);
    if (idx >= 0) return idx;
    
    strcpy(section_names[section_count], name);
    section_flags[section_count] = flags;
    section_aligns[section_count] = align;
    section_offsets[section_count] = 0;
    section_sizes[section_count] = 0;
    section_data[section_count] = output + output_size;
    return section_count++;
}

/* Find or add symbol */
int find_symbol(char *name) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (streq(symbol_names[i], name)) {
            return i;
        }
    }
    return -1;
}

int add_symbol(char *name, int value, int section, int type) {
    int idx = find_symbol(name);
    if (idx < 0) {
        idx = symbol_count++;
        strcpy(symbol_names[idx], name);
        symbol_defined[idx] = 0;
    }
    
    if (section >= 0) {
        symbol_values[idx] = value;
        symbol_sections[idx] = section;
        symbol_types[idx] = type;
        symbol_defined[idx] = 1;
    }
    
    return idx;
}

/* Emit bytes */
void emit_byte(int byte) {
    section_data[current_section][section_sizes[current_section]++] = byte;
    output_size++;
}

void emit_word(int word) {
    emit_byte(word & 0xFF);
    emit_byte((word >> 8) & 0xFF);
}

void emit_dword(int dword) {
    emit_byte(dword & 0xFF);
    emit_byte((dword >> 8) & 0xFF);
    emit_byte((dword >> 16) & 0xFF);
    emit_byte((dword >> 24) & 0xFF);
}

void emit_qword(int qword) {
    emit_dword(qword);
    emit_dword(0); /* High 32 bits */
}

/* Add relocation */
void add_relocation(int offset, int symbol, int type, int addend) {
    reloc_offsets[reloc_count] = offset;
    reloc_symbols[reloc_count] = symbol;
    reloc_types[reloc_count] = type;
    reloc_sections[reloc_count] = current_section;
    reloc_addends[reloc_count] = addend;
    reloc_count++;
}

/* Expression evaluation */
int eval_expr(char **line);

int eval_primary(char **line) {
    char token[MAX_NAME];
    int value = 0;
    
    *line = skip_space(*line);
    
    if (**line == '(') {
        (*line)++;
        value = eval_expr(line);
        *line = skip_space(*line);
        if (**line == ')') (*line)++;
        return value;
    }
    
    if (**line == '-') {
        (*line)++;
        return -eval_primary(line);
    }
    
    if (**line == '~') {
        (*line)++;
        return ~eval_primary(line);
    }
    
    if (**line == '$') {
        (*line)++;
        return section_sizes[current_section];
    }
    
    *line = get_token(*line, token);
    
    if ((token[0] >= '0' && token[0] <= '9') || 
        (token[0] == '-' && token[1] >= '0' && token[1] <= '9')) {
        return parse_number(token);
    }
    
    /* Symbol reference */
    int sym = find_symbol(token);
    if (sym >= 0 && symbol_defined[sym]) {
        return symbol_values[sym];
    }
    
    return 0;
}

int eval_term(char **line) {
    int left = eval_primary(line);
    
    while (1) {
        *line = skip_space(*line);
        if (**line == '*') {
            (*line)++;
            left = left * eval_primary(line);
        } else if (**line == '/') {
            (*line)++;
            int right = eval_primary(line);
            if (right != 0) left = left / right;
        } else if (**line == '%') {
            (*line)++;
            int right = eval_primary(line);
            if (right != 0) left = left % right;
        } else {
            break;
        }
    }
    
    return left;
}

int eval_expr(char **line) {
    int left = eval_term(line);
    
    while (1) {
        *line = skip_space(*line);
        if (**line == '+') {
            (*line)++;
            left = left + eval_term(line);
        } else if (**line == '-' && (*line)[1] != '>') {
            (*line)++;
            left = left - eval_term(line);
        } else if (**line == '<' && (*line)[1] == '<') {
            (*line) += 2;
            left = left << eval_term(line);
        } else if (**line == '>' && (*line)[1] == '>') {
            (*line) += 2;
            left = left >> eval_term(line);
        } else if (**line == '&' && (*line)[1] != '&') {
            (*line)++;
            left = left & eval_term(line);
        } else if (**line == '|' && (*line)[1] != '|') {
            (*line)++;
            left = left | eval_term(line);
        } else if (**line == '^') {
            (*line)++;
            left = left ^ eval_term(line);
        } else {
            break;
        }
    }
    
    return left;
}

/* x64 instruction encoding */
void encode_x64_mov(char *dst, char *src) {
    int dst_reg = -1, src_reg = -1;
    int dst_size = 0, src_size = 0;
    int i;
    
    /* Identify destination register */
    for (i = 0; x64_regs_64[i]; i++) {
        if (streq(dst, x64_regs_64[i])) {
            dst_reg = i; dst_size = 64; break;
        }
    }
    if (dst_reg < 0) {
        for (i = 0; x64_regs_32[i]; i++) {
            if (streq(dst, x64_regs_32[i])) {
                dst_reg = i; dst_size = 32; break;
            }
        }
    }
    
    /* Check if source is immediate */
    if ((src[0] >= '0' && src[0] <= '9') || src[0] == '-' || src[0] == '$') {
        char *p = src;
        int value = eval_expr(&p);
        
        if (dst_size == 64) {
            /* movabs for 64-bit immediate */
            if (dst_reg >= 8) emit_byte(0x49);
            else emit_byte(0x48);
            emit_byte(0xB8 + (dst_reg & 7));
            emit_qword(value);
        } else {
            /* mov r32, imm32 */
            if (dst_reg >= 8) emit_byte(0x41);
            emit_byte(0xB8 + (dst_reg & 7));
            emit_dword(value);
        }
        return;
    }
    
    /* Register to register */
    for (i = 0; x64_regs_64[i]; i++) {
        if (streq(src, x64_regs_64[i])) {
            src_reg = i; src_size = 64; break;
        }
    }
    if (src_reg < 0) {
        for (i = 0; x64_regs_32[i]; i++) {
            if (streq(src, x64_regs_32[i])) {
                src_reg = i; src_size = 32; break;
            }
        }
    }
    
    if (dst_reg >= 0 && src_reg >= 0) {
        /* REX prefix */
        int rex = 0x40;
        if (dst_size == 64) rex |= 0x08;
        if (src_reg >= 8) rex |= 0x04;
        if (dst_reg >= 8) rex |= 0x01;
        if (rex != 0x40) emit_byte(rex);
        
        /* mov instruction */
        emit_byte(0x89);
        emit_byte(0xC0 | ((src_reg & 7) << 3) | (dst_reg & 7));
    }
}

void encode_x64_push(char *reg) {
    int reg_num = -1;
    int i;
    
    for (i = 0; x64_regs_64[i]; i++) {
        if (streq(reg, x64_regs_64[i])) {
            reg_num = i;
            break;
        }
    }
    
    if (reg_num >= 0) {
        if (reg_num >= 8) {
            emit_byte(0x41);
            emit_byte(0x50 + (reg_num & 7));
        } else {
            emit_byte(0x50 + reg_num);
        }
    }
}

void encode_x64_pop(char *reg) {
    int reg_num = -1;
    int i;
    
    for (i = 0; x64_regs_64[i]; i++) {
        if (streq(reg, x64_regs_64[i])) {
            reg_num = i;
            break;
        }
    }
    
    if (reg_num >= 0) {
        if (reg_num >= 8) {
            emit_byte(0x41);
            emit_byte(0x58 + (reg_num & 7));
        } else {
            emit_byte(0x58 + reg_num);
        }
    }
}

void encode_x64_call(char *target) {
    if ((target[0] >= '0' && target[0] <= '9') || target[0] == '-') {
        /* Direct call with offset */
        emit_byte(0xE8);
        emit_dword(parse_number(target));
    } else {
        /* Call to symbol */
        int sym = add_symbol(target, 0, -1, SYM_EXTERN);
        emit_byte(0xE8);
        add_relocation(section_sizes[current_section], sym, 2, -4); /* PC-relative */
        emit_dword(0);
    }
}

void encode_x64_jmp(char *target) {
    if ((target[0] >= '0' && target[0] <= '9') || target[0] == '-') {
        /* Direct jump with offset */
        emit_byte(0xE9);
        emit_dword(parse_number(target));
    } else {
        /* Jump to symbol */
        int sym = add_symbol(target, 0, -1, SYM_EXTERN);
        emit_byte(0xE9);
        add_relocation(section_sizes[current_section], sym, 2, -4);
        emit_dword(0);
    }
}

void encode_x64_ret() {
    emit_byte(0xC3);
}

void encode_x64_nop() {
    emit_byte(0x90);
}

void encode_x64_int(char *num) {
    int value = parse_number(num);
    if (value == 3) {
        emit_byte(0xCC); /* int3 */
    } else {
        emit_byte(0xCD);
        emit_byte(value);
    }
}

/* ARM64 instruction encoding */
int get_arm64_reg(char *name) {
    if (name[0] == 'x' && name[1] >= '0' && name[1] <= '9') {
        int reg = name[1] - '0';
        if (name[2] >= '0' && name[2] <= '9') {
            reg = reg * 10 + name[2] - '0';
        }
        return reg;
    }
    if (name[0] == 'w' && name[1] >= '0' && name[1] <= '9') {
        int reg = name[1] - '0';
        if (name[2] >= '0' && name[2] <= '9') {
            reg = reg * 10 + name[2] - '0';
        }
        return reg;
    }
    if (streq(name, "sp")) return 31;
    if (streq(name, "xzr")) return 31;
    if (streq(name, "wzr")) return 31;
    if (streq(name, "fp")) return 29;
    if (streq(name, "lr")) return 30;
    return -1;
}

void encode_arm64_mov(char *dst, char *src) {
    int dst_reg = get_arm64_reg(dst);
    int src_reg = get_arm64_reg(src);
    int is_64bit = dst[0] == 'x';
    
    if (dst_reg >= 0 && src_reg >= 0) {
        /* MOV (register) - encoded as ORR dst, xzr, src */
        int instr = 0xAA0003E0; /* ORR x0, xzr, x0 */
        if (!is_64bit) instr = 0x2A0003E0; /* ORR w0, wzr, w0 */
        instr |= dst_reg;
        instr |= (src_reg << 16);
        emit_dword(instr);
    } else if (dst_reg >= 0 && (src[0] == '#' || (src[0] >= '0' && src[0] <= '9'))) {
        /* MOV (immediate) */
        char *p = src;
        if (*p == '#') p++;
        int value = eval_expr(&p);
        
        if (value >= 0 && value <= 65535) {
            /* MOVZ */
            int instr = 0xD2800000; /* MOVZ x0, #0 */
            if (!is_64bit) instr = 0x52800000;
            instr |= dst_reg;
            instr |= ((value & 0xFFFF) << 5);
            emit_dword(instr);
        } else if (value == -1) {
            /* MOVN */
            int instr = 0x92800000; /* MOVN x0, #0 */
            if (!is_64bit) instr = 0x12800000;
            instr |= dst_reg;
            emit_dword(instr);
        }
    }
}

void encode_arm64_add(char *dst, char *src1, char *src2) {
    int dst_reg = get_arm64_reg(dst);
    int src1_reg = get_arm64_reg(src1);
    int is_64bit = dst[0] == 'x';
    
    if (src2[0] == '#' || (src2[0] >= '0' && src2[0] <= '9')) {
        /* ADD (immediate) */
        char *p = src2;
        if (*p == '#') p++;
        int imm = eval_expr(&p);
        
        int instr = 0x91000000; /* ADD x0, x0, #0 */
        if (!is_64bit) instr = 0x11000000;
        instr |= dst_reg;
        instr |= (src1_reg << 5);
        instr |= ((imm & 0xFFF) << 10);
        emit_dword(instr);
    } else {
        /* ADD (register) */
        int src2_reg = get_arm64_reg(src2);
        int instr = 0x8B000000; /* ADD x0, x0, x0 */
        if (!is_64bit) instr = 0x0B000000;
        instr |= dst_reg;
        instr |= (src1_reg << 5);
        instr |= (src2_reg << 16);
        emit_dword(instr);
    }
}

void encode_arm64_sub(char *dst, char *src1, char *src2) {
    int dst_reg = get_arm64_reg(dst);
    int src1_reg = get_arm64_reg(src1);
    int is_64bit = dst[0] == 'x';
    
    if (src2[0] == '#' || (src2[0] >= '0' && src2[0] <= '9')) {
        /* SUB (immediate) */
        char *p = src2;
        if (*p == '#') p++;
        int imm = eval_expr(&p);
        
        int instr = 0xD1000000; /* SUB x0, x0, #0 */
        if (!is_64bit) instr = 0x51000000;
        instr |= dst_reg;
        instr |= (src1_reg << 5);
        instr |= ((imm & 0xFFF) << 10);
        emit_dword(instr);
    } else {
        /* SUB (register) */
        int src2_reg = get_arm64_reg(src2);
        int instr = 0xCB000000; /* SUB x0, x0, x0 */
        if (!is_64bit) instr = 0x4B000000;
        instr |= dst_reg;
        instr |= (src1_reg << 5);
        instr |= (src2_reg << 16);
        emit_dword(instr);
    }
}

void encode_arm64_ldr(char *dst, char *src) {
    int dst_reg = get_arm64_reg(dst);
    int is_64bit = dst[0] == 'x';
    
    if (src[0] == '[') {
        /* Parse [base, offset] */
        char base[MAX_NAME], offset[MAX_NAME];
        char *p = src + 1;
        p = get_token(p, base);
        
        int base_reg = get_arm64_reg(base);
        
        if (*p == ']') {
            /* LDR with no offset */
            int instr = 0xF9400000; /* LDR x0, [x0] */
            if (!is_64bit) instr = 0xB9400000;
            instr |= dst_reg;
            instr |= (base_reg << 5);
            emit_dword(instr);
        } else if (*p == ',') {
            p++;
            p = skip_space(p);
            if (*p == '#') p++;
            int imm = parse_number(p);
            
            /* LDR with immediate offset */
            int instr = 0xF9400000;
            if (!is_64bit) instr = 0xB9400000;
            instr |= dst_reg;
            instr |= (base_reg << 5);
            if (is_64bit) {
                instr |= ((imm / 8) & 0xFFF) << 10;
            } else {
                instr |= ((imm / 4) & 0xFFF) << 10;
            }
            emit_dword(instr);
        }
    }
}

void encode_arm64_str(char *src, char *dst) {
    int src_reg = get_arm64_reg(src);
    int is_64bit = src[0] == 'x';
    
    if (dst[0] == '[') {
        /* Parse [base, offset] */
        char base[MAX_NAME];
        char *p = dst + 1;
        p = get_token(p, base);
        
        int base_reg = get_arm64_reg(base);
        
        if (*p == ']') {
            /* STR with no offset */
            int instr = 0xF9000000; /* STR x0, [x0] */
            if (!is_64bit) instr = 0xB9000000;
            instr |= src_reg;
            instr |= (base_reg << 5);
            emit_dword(instr);
        } else if (*p == ',') {
            p++;
            p = skip_space(p);
            if (*p == '#') p++;
            int imm = parse_number(p);
            
            /* STR with immediate offset */
            int instr = 0xF9000000;
            if (!is_64bit) instr = 0xB9000000;
            instr |= src_reg;
            instr |= (base_reg << 5);
            if (is_64bit) {
                instr |= ((imm / 8) & 0xFFF) << 10;
            } else {
                instr |= ((imm / 4) & 0xFFF) << 10;
            }
            emit_dword(instr);
        }
    }
}

void encode_arm64_bl(char *target) {
    int sym = add_symbol(target, 0, -1, SYM_EXTERN);
    emit_byte(0x00);
    emit_byte(0x00);
    emit_byte(0x00);
    emit_byte(0x94); /* BL placeholder */
    add_relocation(section_sizes[current_section] - 4, sym, 283, 0); /* R_AARCH64_CALL26 */
}

void encode_arm64_b(char *target) {
    int sym = add_symbol(target, 0, -1, SYM_EXTERN);
    emit_byte(0x00);
    emit_byte(0x00);
    emit_byte(0x00);
    emit_byte(0x14); /* B placeholder */
    add_relocation(section_sizes[current_section] - 4, sym, 282, 0); /* R_AARCH64_JUMP26 */
}

void encode_arm64_ret() {
    emit_dword(0xD65F03C0); /* RET */
}

void encode_arm64_nop() {
    emit_dword(0xD503201F); /* NOP */
}

/* Process instruction */
void process_instruction(char *mnemonic, char *operands) {
    char op1[MAX_NAME], op2[MAX_NAME], op3[MAX_NAME];
    char *p = operands;
    
    /* Parse operands */
    *op1 = *op2 = *op3 = 0;
    if (*p) {
        p = get_token(p, op1);
        p = skip_space(p);
        if (*p == ',') {
            p++;
            p = get_token(p, op2);
            p = skip_space(p);
            if (*p == ',') {
                p++;
                p = get_token(p, op3);
            }
        }
    }
    
    if (is_arm64) {
        /* ARM64 instructions */
        if (streq(mnemonic, "mov")) {
            encode_arm64_mov(op1, op2);
        } else if (streq(mnemonic, "add")) {
            encode_arm64_add(op1, op2, op3);
        } else if (streq(mnemonic, "sub")) {
            encode_arm64_sub(op1, op2, op3);
        } else if (streq(mnemonic, "ldr")) {
            encode_arm64_ldr(op1, op2);
        } else if (streq(mnemonic, "str")) {
            encode_arm64_str(op1, op2);
        } else if (streq(mnemonic, "bl")) {
            encode_arm64_bl(op1);
        } else if (streq(mnemonic, "b")) {
            encode_arm64_b(op1);
        } else if (streq(mnemonic, "ret")) {
            encode_arm64_ret();
        } else if (streq(mnemonic, "nop")) {
            encode_arm64_nop();
        } else if (streq(mnemonic, "svc")) {
            int imm = parse_number(op1);
            emit_dword(0xD4000001 | ((imm & 0xFFFF) << 5));
        }
    } else {
        /* x64 instructions */
        if (streq(mnemonic, "mov")) {
            encode_x64_mov(op1, op2);
        } else if (streq(mnemonic, "push")) {
            encode_x64_push(op1);
        } else if (streq(mnemonic, "pop")) {
            encode_x64_pop(op1);
        } else if (streq(mnemonic, "call")) {
            encode_x64_call(op1);
        } else if (streq(mnemonic, "jmp")) {
            encode_x64_jmp(op1);
        } else if (streq(mnemonic, "ret")) {
            encode_x64_ret();
        } else if (streq(mnemonic, "nop")) {
            encode_x64_nop();
        } else if (streq(mnemonic, "int")) {
            encode_x64_int(op1);
        } else if (streq(mnemonic, "syscall")) {
            emit_byte(0x0F);
            emit_byte(0x05);
        } else if (streq(mnemonic, "add")) {
            /* Basic ADD support */
            emit_byte(0x48); /* REX.W */
            emit_byte(0x01);
            emit_byte(0xC0); /* ADD RAX, RAX as placeholder */
        } else if (streq(mnemonic, "sub")) {
            /* Basic SUB support */
            emit_byte(0x48); /* REX.W */
            emit_byte(0x29);
            emit_byte(0xC0); /* SUB RAX, RAX as placeholder */
        } else if (streq(mnemonic, "xor")) {
            /* Basic XOR support */
            emit_byte(0x48); /* REX.W */
            emit_byte(0x31);
            emit_byte(0xC0); /* XOR RAX, RAX as placeholder */
        }
    }
}

/* Process directive */
void process_directive(char *directive, char *args) {
    if (streq(directive, ".text")) {
        current_section = add_section(".text", SEC_CODE, 16);
    } else if (streq(directive, ".data")) {
        current_section = add_section(".data", SEC_DATA, 8);
    } else if (streq(directive, ".bss")) {
        current_section = add_section(".bss", SEC_BSS, 8);
    } else if (streq(directive, ".section")) {
        char name[MAX_NAME];
        char *p = args;
        p = get_token(p, name);
        current_section = add_section(name, SEC_DATA, 1);
    } else if (streq(directive, ".global") || streq(directive, ".globl")) {
        char name[MAX_NAME];
        char *p = args;
        p = get_token(p, name);
        int sym = add_symbol(name, 0, -1, SYM_GLOBAL);
        symbol_types[sym] = SYM_GLOBAL;
    } else if (streq(directive, ".extern")) {
        char name[MAX_NAME];
        char *p = args;
        p = get_token(p, name);
        add_symbol(name, 0, -1, SYM_EXTERN);
    } else if (streq(directive, ".align")) {
        char *p = args;
        int align = eval_expr(&p);
        while (section_sizes[current_section] & (align - 1)) {
            emit_byte(0);
        }
    } else if (streq(directive, ".byte") || streq(directive, ".db")) {
        char *p = args;
        while (*p) {
            p = skip_space(p);
            if (*p == '"') {
                /* String */
                p++;
                while (*p && *p != '"') {
                    if (*p == '\\' && p[1]) {
                        p++;
                        switch (*p) {
                        case 'n': emit_byte('\n'); break;
                        case 't': emit_byte('\t'); break;
                        case 'r': emit_byte('\r'); break;
                        case '0': emit_byte('\0'); break;
                        default: emit_byte(*p); break;
                        }
                        p++;
                    } else {
                        emit_byte(*p++);
                    }
                }
                if (*p == '"') p++;
            } else {
                int value = eval_expr(&p);
                emit_byte(value);
            }
            p = skip_space(p);
            if (*p == ',') p++;
        }
    } else if (streq(directive, ".word") || streq(directive, ".dw")) {
        char *p = args;
        while (*p) {
            int value = eval_expr(&p);
            emit_word(value);
            p = skip_space(p);
            if (*p == ',') p++;
        }
    } else if (streq(directive, ".dword") || streq(directive, ".dd") || 
               streq(directive, ".long")) {
        char *p = args;
        while (*p) {
            int value = eval_expr(&p);
            emit_dword(value);
            p = skip_space(p);
            if (*p == ',') p++;
        }
    } else if (streq(directive, ".quad") || streq(directive, ".dq")) {
        char *p = args;
        while (*p) {
            int value = eval_expr(&p);
            emit_qword(value);
            p = skip_space(p);
            if (*p == ',') p++;
        }
    } else if (streq(directive, ".space") || streq(directive, ".skip")) {
        char *p = args;
        int count = eval_expr(&p);
        int fill = 0;
        p = skip_space(p);
        if (*p == ',') {
            p++;
            fill = eval_expr(&p);
        }
        while (count-- > 0) {
            emit_byte(fill);
        }
    } else if (streq(directive, ".ascii")) {
        char *p = args;
        if (*p == '"') {
            p++;
            while (*p && *p != '"') {
                emit_byte(*p++);
            }
        }
    } else if (streq(directive, ".asciz") || streq(directive, ".string")) {
        char *p = args;
        if (*p == '"') {
            p++;
            while (*p && *p != '"') {
                emit_byte(*p++);
            }
            emit_byte(0);
        }
    } else if (streq(directive, ".arch")) {
        char arch[MAX_NAME];
        char *p = args;
        p = get_token(p, arch);
        if (streq(arch, "x64") || streq(arch, "x86_64") || 
            streq(arch, "amd64")) {
            is_arm64 = 0;
        } else if (streq(arch, "arm64") || streq(arch, "aarch64")) {
            is_arm64 = 1;
        }
    } else if (streq(directive, ".equ") || streq(directive, ".set")) {
        char name[MAX_NAME];
        char *p = args;
        p = get_token(p, name);
        p = skip_space(p);
        if (*p == ',') p++;
        int value = eval_expr(&p);
        add_symbol(name, value, current_section, SYM_LOCAL);
    }
}

/* Process line */
void process_line(char *line) {
    char label[MAX_NAME];
    char mnemonic[MAX_NAME];
    char *p;
    
    line_number++;
    
    /* Skip empty lines and comments */
    p = skip_space(line);
    if (!*p || *p == ';' || *p == '#') return;
    
    /* Check for label */
    if (strchr(line, ':')) {
        p = get_token(line, label);
        if (*p == ':') {
            p++;
            add_symbol(label, section_sizes[current_section], 
                      current_section, SYM_LOCAL);
            line = p;
        }
    }
    
    /* Get mnemonic */
    p = skip_space(line);
    if (!*p) return;
    
    p = get_token(p, mnemonic);
    
    /* Check if directive */
    if (mnemonic[0] == '.') {
        process_directive(mnemonic, p);
    } else {
        process_instruction(mnemonic, p);
    }
}

/* Write object file header */
void write_object_header(int fd) {
    /* Simple object file format:
     * Magic: "SAS\0"
     * Architecture: 4 bytes (0=x64, 1=arm64)
     * Section count: 4 bytes
     * Symbol count: 4 bytes
     * Relocation count: 4 bytes
     * Followed by sections, symbols, and relocations
     */
    write(fd, "SAS\0", 4);
    int arch = is_arm64 ? 1 : 0;
    write(fd, &arch, 4);
    write(fd, &section_count, 4);
    write(fd, &symbol_count, 4);
    write(fd, &reloc_count, 4);
}

/* Write sections */
void write_sections(int fd) {
    int i;
    for (i = 0; i < section_count; i++) {
        write(fd, section_names[i], MAX_NAME);
        write(fd, &section_sizes[i], 4);
        write(fd, &section_flags[i], 4);
        write(fd, &section_aligns[i], 4);
        if (section_sizes[i] > 0 && !(section_flags[i] & SEC_BSS)) {
            write(fd, section_data[i], section_sizes[i]);
        }
    }
}

/* Write symbols */
void write_symbols(int fd) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        write(fd, symbol_names[i], MAX_NAME);
        write(fd, &symbol_values[i], 4);
        write(fd, &symbol_sections[i], 4);
        write(fd, &symbol_types[i], 4);
        write(fd, &symbol_defined[i], 4);
    }
}

/* Write relocations */
void write_relocations(int fd) {
    int i;
    for (i = 0; i < reloc_count; i++) {
        write(fd, &reloc_offsets[i], 4);
        write(fd, &reloc_symbols[i], 4);
        write(fd, &reloc_types[i], 4);
        write(fd, &reloc_sections[i], 4);
        write(fd, &reloc_addends[i], 4);
    }
}

/* Main assembler */
int main(int argc, char *argv[]) {
    char line[MAX_LINE];
    int input_fd, output_fd;
    char *input_file = NULL;
    char *output_file = "a.out";
    int i;
    
    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (streq(argv[i], "-o") && i + 1 < argc) {
            output_file = argv[++i];
        } else if (streq(argv[i], "-arch") && i + 1 < argc) {
            i++;
            if (streq(argv[i], "x64") || streq(argv[i], "x86_64")) {
                is_arm64 = 0;
            } else if (streq(argv[i], "arm64") || streq(argv[i], "aarch64")) {
                is_arm64 = 1;
            }
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }
    
    if (!input_file) {
        puts("Usage: sas_enhanced [-o output] [-arch x64|arm64] input.s\n");
        return 1;
    }
    
    /* Initialize */
    section_count = 0;
    symbol_count = 0;
    reloc_count = 0;
    output_size = 0;
    line_number = 0;
    strcpy(current_file, input_file);
    
    /* Add default sections */
    current_section = add_section(".text", SEC_CODE, 16);
    
    /* Open input file */
    input_fd = open(input_file, 0);
    if (input_fd < 0) {
        printf("Error: Cannot open %s\n", input_file);
        return 1;
    }
    
    /* Read and process file line by line */
    int pos = 0;
    int ch;
    while (read(input_fd, &ch, 1) == 1) {
        if (ch == '\n' || ch == '\r') {
            line[pos] = 0;
            process_line(line);
            pos = 0;
        } else if (pos < MAX_LINE - 1) {
            line[pos++] = ch;
        }
    }
    if (pos > 0) {
        line[pos] = 0;
        process_line(line);
    }
    
    close(input_fd);
    
    /* Create output file */
    output_fd = creat(output_file);
    if (output_fd < 0) {
        printf("Error: Cannot create %s\n", output_file);
        return 1;
    }
    
    /* Write object file */
    write_object_header(output_fd);
    write_sections(output_fd);
    write_symbols(output_fd);
    write_relocations(output_fd);
    
    close(output_fd);
    
    printf("Assembled %s to %s (%s, %d bytes)\n", 
           input_file, output_file, 
           is_arm64 ? "ARM64" : "x64",
           output_size);
    
    return 0;
}
