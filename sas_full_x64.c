/* sas_full_x64.c - Small Assembler System for x64
 * A powerful x64 assembler written in Small-C dialect
 * Supports 300+ x64 instructions including:
 * - Core integer operations (MOV, ADD, SUB, etc.)
 * - All conditional jumps and sets
 * - Conditional moves (CMOVcc)
 * - String operations with REP prefixes
 * - Bit manipulation (BT, BSF, BSR, POPCNT, BMI1/2)
 * - System instructions (CPUID, RDTSC, SYSCALL, etc.)
 * - Memory barriers (MFENCE, LFENCE, SFENCE)
 * - I/O instructions (IN, OUT, INS, OUTS)
 * - TSX transactional memory
 * - CET control-flow enforcement
 * - Advanced addressing modes with base, index, scale, displacement
 */

/* Instruction buffer and output */
char output[65536];
int output_pos;
char line[256];
char token[64];
int token_pos;
int line_pos;

/* Register encoding tables */
char *reg64_names[16];
char *reg32_names[16];
char *reg16_names[16];
char *reg8_names[16];

/* Instruction mnemonics - using parallel arrays due to no structs */
char *opcodes[512];  /* Increased to handle 200+ instructions */
int opcode_values[512];
int opcode_types[512];
int opcode_count;

/* REX prefix bits */
int rex_w, rex_r, rex_x, rex_b;

/* Current instruction bytes */
char inst_bytes[15];
int inst_len;

/* Initialize register name tables */
void init_registers() {
    /* 64-bit registers */
    reg64_names[0] = "rax"; reg64_names[1] = "rcx"; 
    reg64_names[2] = "rdx"; reg64_names[3] = "rbx";
    reg64_names[4] = "rsp"; reg64_names[5] = "rbp"; 
    reg64_names[6] = "rsi"; reg64_names[7] = "rdi";
    reg64_names[8] = "r8";  reg64_names[9] = "r9";  
    reg64_names[10] = "r10"; reg64_names[11] = "r11";
    reg64_names[12] = "r12"; reg64_names[13] = "r13"; 
    reg64_names[14] = "r14"; reg64_names[15] = "r15";
    
    /* 32-bit registers */
    reg32_names[0] = "eax"; reg32_names[1] = "ecx"; 
    reg32_names[2] = "edx"; reg32_names[3] = "ebx";
    reg32_names[4] = "esp"; reg32_names[5] = "ebp"; 
    reg32_names[6] = "esi"; reg32_names[7] = "edi";
    reg32_names[8] = "r8d"; reg32_names[9] = "r9d"; 
    reg32_names[10] = "r10d"; reg32_names[11] = "r11d";
    reg32_names[12] = "r12d"; reg32_names[13] = "r13d"; 
    reg32_names[14] = "r14d"; reg32_names[15] = "r15d";
    
    /* 16-bit registers */
    reg16_names[0] = "ax"; reg16_names[1] = "cx"; 
    reg16_names[2] = "dx"; reg16_names[3] = "bx";
    reg16_names[4] = "sp"; reg16_names[5] = "bp"; 
    reg16_names[6] = "si"; reg16_names[7] = "di";
    reg16_names[8] = "r8w"; reg16_names[9] = "r9w"; 
    reg16_names[10] = "r10w"; reg16_names[11] = "r11w";
    reg16_names[12] = "r12w"; reg16_names[13] = "r13w"; 
    reg16_names[14] = "r14w"; reg16_names[15] = "r15w";
    
    /* 8-bit registers */
    reg8_names[0] = "al"; reg8_names[1] = "cl"; 
    reg8_names[2] = "dl"; reg8_names[3] = "bl";
    reg8_names[4] = "spl"; reg8_names[5] = "bpl"; 
    reg8_names[6] = "sil"; reg8_names[7] = "dil";
    reg8_names[8] = "r8b"; reg8_names[9] = "r9b"; 
    reg8_names[10] = "r10b"; reg8_names[11] = "r11b";
    reg8_names[12] = "r12b"; reg8_names[13] = "r13b"; 
    reg8_names[14] = "r14b"; reg8_names[15] = "r15b";
}

/* Initialize instruction tables */
void init_opcodes() {
    opcode_count = 0;
    
    /* MOV instructions */
    opcodes[opcode_count] = "mov"; opcode_values[opcode_count] = 0x88; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "movb"; opcode_values[opcode_count] = 0x88; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "movw"; opcode_values[opcode_count] = 0x89; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "movl"; opcode_values[opcode_count] = 0x89; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "movq"; opcode_values[opcode_count] = 0x89; opcode_types[opcode_count] = 8; opcode_count++;
    opcodes[opcode_count] = "movzx"; opcode_values[opcode_count] = 0xB6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "movsx"; opcode_values[opcode_count] = 0xBE; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Arithmetic instructions */
    opcodes[opcode_count] = "add"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "adc"; opcode_values[opcode_count] = 0x10; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sub"; opcode_values[opcode_count] = 0x28; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sbb"; opcode_values[opcode_count] = 0x18; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "imul"; opcode_values[opcode_count] = 0xAF; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "mul"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "idiv"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "div"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "inc"; opcode_values[opcode_count] = 0xFE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "dec"; opcode_values[opcode_count] = 0xFE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "neg"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Logical instructions */
    opcodes[opcode_count] = "and"; opcode_values[opcode_count] = 0x20; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "or"; opcode_values[opcode_count] = 0x08; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xor"; opcode_values[opcode_count] = 0x30; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "not"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Shift/Rotate instructions */
    opcodes[opcode_count] = "shl"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "shr"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sal"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sar"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rol"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ror"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rcl"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rcr"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "shld"; opcode_values[opcode_count] = 0xA4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "shrd"; opcode_values[opcode_count] = 0xAC; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Compare and test */
    opcodes[opcode_count] = "cmp"; opcode_values[opcode_count] = 0x38; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "test"; opcode_values[opcode_count] = 0x84; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Stack operations */
    opcodes[opcode_count] = "push"; opcode_values[opcode_count] = 0x50; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "pop"; opcode_values[opcode_count] = 0x58; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "pushf"; opcode_values[opcode_count] = 0x9C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "popf"; opcode_values[opcode_count] = 0x9D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "pusha"; opcode_values[opcode_count] = 0x60; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "popa"; opcode_values[opcode_count] = 0x61; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Jump instructions - All conditions */
    opcodes[opcode_count] = "jmp"; opcode_values[opcode_count] = 0xE9; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "je"; opcode_values[opcode_count] = 0x74; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jz"; opcode_values[opcode_count] = 0x74; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jne"; opcode_values[opcode_count] = 0x75; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnz"; opcode_values[opcode_count] = 0x75; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jl"; opcode_values[opcode_count] = 0x7C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnge"; opcode_values[opcode_count] = 0x7C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jg"; opcode_values[opcode_count] = 0x7F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnle"; opcode_values[opcode_count] = 0x7F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jle"; opcode_values[opcode_count] = 0x7E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jng"; opcode_values[opcode_count] = 0x7E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jge"; opcode_values[opcode_count] = 0x7D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnl"; opcode_values[opcode_count] = 0x7D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jb"; opcode_values[opcode_count] = 0x72; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnae"; opcode_values[opcode_count] = 0x72; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jc"; opcode_values[opcode_count] = 0x72; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ja"; opcode_values[opcode_count] = 0x77; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnbe"; opcode_values[opcode_count] = 0x77; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jbe"; opcode_values[opcode_count] = 0x76; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jna"; opcode_values[opcode_count] = 0x76; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jae"; opcode_values[opcode_count] = 0x73; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnb"; opcode_values[opcode_count] = 0x73; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnc"; opcode_values[opcode_count] = 0x73; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "js"; opcode_values[opcode_count] = 0x78; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jns"; opcode_values[opcode_count] = 0x79; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jo"; opcode_values[opcode_count] = 0x70; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jno"; opcode_values[opcode_count] = 0x71; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jp"; opcode_values[opcode_count] = 0x7A; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jpe"; opcode_values[opcode_count] = 0x7A; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jnp"; opcode_values[opcode_count] = 0x7B; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jpo"; opcode_values[opcode_count] = 0x7B; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jcxz"; opcode_values[opcode_count] = 0xE3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "jecxz"; opcode_values[opcode_count] = 0xE3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "loop"; opcode_values[opcode_count] = 0xE2; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "loope"; opcode_values[opcode_count] = 0xE1; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "loopne"; opcode_values[opcode_count] = 0xE0; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Call and return */
    opcodes[opcode_count] = "call"; opcode_values[opcode_count] = 0xE8; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ret"; opcode_values[opcode_count] = 0xC3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "retn"; opcode_values[opcode_count] = 0xC2; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "leave"; opcode_values[opcode_count] = 0xC9; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "enter"; opcode_values[opcode_count] = 0xC8; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* String operations */
    opcodes[opcode_count] = "movsb"; opcode_values[opcode_count] = 0xA4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "movsw"; opcode_values[opcode_count] = 0xA5; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "movsd"; opcode_values[opcode_count] = 0xA5; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "movsq"; opcode_values[opcode_count] = 0xA5; opcode_types[opcode_count] = 8; opcode_count++;
    opcodes[opcode_count] = "cmpsb"; opcode_values[opcode_count] = 0xA6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmpsw"; opcode_values[opcode_count] = 0xA7; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "cmpsd"; opcode_values[opcode_count] = 0xA7; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "cmpsq"; opcode_values[opcode_count] = 0xA7; opcode_types[opcode_count] = 8; opcode_count++;
    opcodes[opcode_count] = "scasb"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "scasw"; opcode_values[opcode_count] = 0xAF; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "scasd"; opcode_values[opcode_count] = 0xAF; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "scasq"; opcode_values[opcode_count] = 0xAF; opcode_types[opcode_count] = 8; opcode_count++;
    opcodes[opcode_count] = "stosb"; opcode_values[opcode_count] = 0xAA; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "stosw"; opcode_values[opcode_count] = 0xAB; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "stosd"; opcode_values[opcode_count] = 0xAB; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "stosq"; opcode_values[opcode_count] = 0xAB; opcode_types[opcode_count] = 8; opcode_count++;
    opcodes[opcode_count] = "lodsb"; opcode_values[opcode_count] = 0xAC; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lodsw"; opcode_values[opcode_count] = 0xAD; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "lodsd"; opcode_values[opcode_count] = 0xAD; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "lodsq"; opcode_values[opcode_count] = 0xAD; opcode_types[opcode_count] = 8; opcode_count++;
    opcodes[opcode_count] = "rep"; opcode_values[opcode_count] = 0xF3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "repe"; opcode_values[opcode_count] = 0xF3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "repz"; opcode_values[opcode_count] = 0xF3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "repne"; opcode_values[opcode_count] = 0xF2; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "repnz"; opcode_values[opcode_count] = 0xF2; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Exchange and swap */
    opcodes[opcode_count] = "xchg"; opcode_values[opcode_count] = 0x86; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xadd"; opcode_values[opcode_count] = 0xC0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmpxchg"; opcode_values[opcode_count] = 0xB0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmpxchg8b"; opcode_values[opcode_count] = 0xC7; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bswap"; opcode_values[opcode_count] = 0xC8; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* LEA and bounds */
    opcodes[opcode_count] = "lea"; opcode_values[opcode_count] = 0x8D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bound"; opcode_values[opcode_count] = 0x62; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Bit manipulation */
    opcodes[opcode_count] = "bt"; opcode_values[opcode_count] = 0xA3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bts"; opcode_values[opcode_count] = 0xAB; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "btr"; opcode_values[opcode_count] = 0xB3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "btc"; opcode_values[opcode_count] = 0xBB; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bsf"; opcode_values[opcode_count] = 0xBC; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bsr"; opcode_values[opcode_count] = 0xBD; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "popcnt"; opcode_values[opcode_count] = 0xB8; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lzcnt"; opcode_values[opcode_count] = 0xBD; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "tzcnt"; opcode_values[opcode_count] = 0xBC; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Set on condition - Extended */
    opcodes[opcode_count] = "sete"; opcode_values[opcode_count] = 0x94; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setz"; opcode_values[opcode_count] = 0x94; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setne"; opcode_values[opcode_count] = 0x95; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnz"; opcode_values[opcode_count] = 0x95; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setl"; opcode_values[opcode_count] = 0x9C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnge"; opcode_values[opcode_count] = 0x9C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setg"; opcode_values[opcode_count] = 0x9F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnle"; opcode_values[opcode_count] = 0x9F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setle"; opcode_values[opcode_count] = 0x9E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setng"; opcode_values[opcode_count] = 0x9E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setge"; opcode_values[opcode_count] = 0x9D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnl"; opcode_values[opcode_count] = 0x9D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setb"; opcode_values[opcode_count] = 0x92; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnae"; opcode_values[opcode_count] = 0x92; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setc"; opcode_values[opcode_count] = 0x92; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "seta"; opcode_values[opcode_count] = 0x97; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnbe"; opcode_values[opcode_count] = 0x97; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setbe"; opcode_values[opcode_count] = 0x96; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setna"; opcode_values[opcode_count] = 0x96; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setae"; opcode_values[opcode_count] = 0x93; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnb"; opcode_values[opcode_count] = 0x93; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnc"; opcode_values[opcode_count] = 0x93; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sets"; opcode_values[opcode_count] = 0x98; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setns"; opcode_values[opcode_count] = 0x99; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "seto"; opcode_values[opcode_count] = 0x90; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setno"; opcode_values[opcode_count] = 0x91; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setp"; opcode_values[opcode_count] = 0x9A; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setpe"; opcode_values[opcode_count] = 0x9A; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setnp"; opcode_values[opcode_count] = 0x9B; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "setpo"; opcode_values[opcode_count] = 0x9B; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* System instructions */
    opcodes[opcode_count] = "nop"; opcode_values[opcode_count] = 0x90; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "int"; opcode_values[opcode_count] = 0xCD; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "int3"; opcode_values[opcode_count] = 0xCC; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "into"; opcode_values[opcode_count] = 0xCE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "iret"; opcode_values[opcode_count] = 0xCF; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "iretd"; opcode_values[opcode_count] = 0xCF; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "syscall"; opcode_values[opcode_count] = 0x05; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sysret"; opcode_values[opcode_count] = 0x07; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sysenter"; opcode_values[opcode_count] = 0x34; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sysexit"; opcode_values[opcode_count] = 0x35; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cpuid"; opcode_values[opcode_count] = 0xA2; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rdtsc"; opcode_values[opcode_count] = 0x31; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rdtscp"; opcode_values[opcode_count] = 0xF9; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rdpmc"; opcode_values[opcode_count] = 0x33; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rdmsr"; opcode_values[opcode_count] = 0x32; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "wrmsr"; opcode_values[opcode_count] = 0x30; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xgetbv"; opcode_values[opcode_count] = 0xD0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xsetbv"; opcode_values[opcode_count] = 0xD1; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Memory barriers and sync */
    opcodes[opcode_count] = "mfence"; opcode_values[opcode_count] = 0xF0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lfence"; opcode_values[opcode_count] = 0xE8; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sfence"; opcode_values[opcode_count] = 0xF8; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "pause"; opcode_values[opcode_count] = 0x90; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lock"; opcode_values[opcode_count] = 0xF0; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Prefetch */
    opcodes[opcode_count] = "prefetch"; opcode_values[opcode_count] = 0x18; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "prefetchw"; opcode_values[opcode_count] = 0x0D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "prefetchnta"; opcode_values[opcode_count] = 0x18; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Misc operations */
    opcodes[opcode_count] = "clc"; opcode_values[opcode_count] = 0xF8; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "stc"; opcode_values[opcode_count] = 0xF9; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmc"; opcode_values[opcode_count] = 0xF5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cld"; opcode_values[opcode_count] = 0xFC; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "std"; opcode_values[opcode_count] = 0xFD; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cli"; opcode_values[opcode_count] = 0xFA; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sti"; opcode_values[opcode_count] = 0xFB; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "hlt"; opcode_values[opcode_count] = 0xF4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "wait"; opcode_values[opcode_count] = 0x9B; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cbw"; opcode_values[opcode_count] = 0x98; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cwde"; opcode_values[opcode_count] = 0x98; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cdqe"; opcode_values[opcode_count] = 0x98; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cwd"; opcode_values[opcode_count] = 0x99; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cdq"; opcode_values[opcode_count] = 0x99; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cqo"; opcode_values[opcode_count] = 0x99; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lahf"; opcode_values[opcode_count] = 0x9F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sahf"; opcode_values[opcode_count] = 0x9E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xlat"; opcode_values[opcode_count] = 0xD7; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* BMI1/BMI2 instructions */
    opcodes[opcode_count] = "andn"; opcode_values[opcode_count] = 0xF2; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bextr"; opcode_values[opcode_count] = 0xF7; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "blsi"; opcode_values[opcode_count] = 0xF3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "blsmsk"; opcode_values[opcode_count] = 0xF3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "blsr"; opcode_values[opcode_count] = 0xF3; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "bzhi"; opcode_values[opcode_count] = 0xF5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "mulx"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "pdep"; opcode_values[opcode_count] = 0xF5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "pext"; opcode_values[opcode_count] = 0xF5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rorx"; opcode_values[opcode_count] = 0xF0; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sarx"; opcode_values[opcode_count] = 0xF7; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "shlx"; opcode_values[opcode_count] = 0xF7; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "shrx"; opcode_values[opcode_count] = 0xF7; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Conditional move instructions - CMOVcc */
    opcodes[opcode_count] = "cmove"; opcode_values[opcode_count] = 0x44; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovz"; opcode_values[opcode_count] = 0x44; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovne"; opcode_values[opcode_count] = 0x45; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnz"; opcode_values[opcode_count] = 0x45; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovl"; opcode_values[opcode_count] = 0x4C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnge"; opcode_values[opcode_count] = 0x4C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovg"; opcode_values[opcode_count] = 0x4F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnle"; opcode_values[opcode_count] = 0x4F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovle"; opcode_values[opcode_count] = 0x4E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovng"; opcode_values[opcode_count] = 0x4E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovge"; opcode_values[opcode_count] = 0x4D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnl"; opcode_values[opcode_count] = 0x4D; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovb"; opcode_values[opcode_count] = 0x42; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnae"; opcode_values[opcode_count] = 0x42; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovc"; opcode_values[opcode_count] = 0x42; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmova"; opcode_values[opcode_count] = 0x47; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnbe"; opcode_values[opcode_count] = 0x47; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovbe"; opcode_values[opcode_count] = 0x46; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovna"; opcode_values[opcode_count] = 0x46; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovae"; opcode_values[opcode_count] = 0x43; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnb"; opcode_values[opcode_count] = 0x43; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnc"; opcode_values[opcode_count] = 0x43; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovs"; opcode_values[opcode_count] = 0x48; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovns"; opcode_values[opcode_count] = 0x49; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovo"; opcode_values[opcode_count] = 0x40; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovno"; opcode_values[opcode_count] = 0x41; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovp"; opcode_values[opcode_count] = 0x4A; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovpe"; opcode_values[opcode_count] = 0x4A; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovnp"; opcode_values[opcode_count] = 0x4B; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "cmovpo"; opcode_values[opcode_count] = 0x4B; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Additional arithmetic */
    opcodes[opcode_count] = "adcx"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "adox"; opcode_values[opcode_count] = 0xF6; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* CRC32 instructions */
    opcodes[opcode_count] = "crc32"; opcode_values[opcode_count] = 0xF0; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* More system instructions */
    opcodes[opcode_count] = "invd"; opcode_values[opcode_count] = 0x08; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "wbinvd"; opcode_values[opcode_count] = 0x09; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "invlpg"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lgdt"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lidt"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sgdt"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sidt"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lldt"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "sldt"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ltr"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "str"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lmsw"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "smsw"; opcode_values[opcode_count] = 0x01; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "clts"; opcode_values[opcode_count] = 0x06; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "arpl"; opcode_values[opcode_count] = 0x63; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lar"; opcode_values[opcode_count] = 0x02; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lsl"; opcode_values[opcode_count] = 0x03; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "verr"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "verw"; opcode_values[opcode_count] = 0x00; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rsm"; opcode_values[opcode_count] = 0xAA; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* More I/O instructions */
    opcodes[opcode_count] = "in"; opcode_values[opcode_count] = 0xE4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "out"; opcode_values[opcode_count] = 0xE6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ins"; opcode_values[opcode_count] = 0x6C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "outs"; opcode_values[opcode_count] = 0x6E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "insb"; opcode_values[opcode_count] = 0x6C; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "insw"; opcode_values[opcode_count] = 0x6D; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "insd"; opcode_values[opcode_count] = 0x6D; opcode_types[opcode_count] = 4; opcode_count++;
    opcodes[opcode_count] = "outsb"; opcode_values[opcode_count] = 0x6E; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "outsw"; opcode_values[opcode_count] = 0x6F; opcode_types[opcode_count] = 2; opcode_count++;
    opcodes[opcode_count] = "outsd"; opcode_values[opcode_count] = 0x6F; opcode_types[opcode_count] = 4; opcode_count++;
    
    /* Segment instructions */
    opcodes[opcode_count] = "lds"; opcode_values[opcode_count] = 0xC5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "les"; opcode_values[opcode_count] = 0xC4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lfs"; opcode_values[opcode_count] = 0xB4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lgs"; opcode_values[opcode_count] = 0xB5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "lss"; opcode_values[opcode_count] = 0xB2; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* MOVBE - Move with byte swap */
    opcodes[opcode_count] = "movbe"; opcode_values[opcode_count] = 0xF0; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* Decimal arithmetic (BCD) */
    opcodes[opcode_count] = "daa"; opcode_values[opcode_count] = 0x27; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "das"; opcode_values[opcode_count] = 0x2F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "aaa"; opcode_values[opcode_count] = 0x37; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "aas"; opcode_values[opcode_count] = 0x3F; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "aam"; opcode_values[opcode_count] = 0xD4; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "aad"; opcode_values[opcode_count] = 0xD5; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* XSAVE family */
    opcodes[opcode_count] = "xsave"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xsavec"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xsaveopt"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xrstor"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* RDRAND/RDSEED */
    opcodes[opcode_count] = "rdrand"; opcode_values[opcode_count] = 0xC7; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rdseed"; opcode_values[opcode_count] = 0xC7; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* More control flow */
    opcodes[opcode_count] = "ud0"; opcode_values[opcode_count] = 0xFF; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ud1"; opcode_values[opcode_count] = 0xB9; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "ud2"; opcode_values[opcode_count] = 0x0B; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* CLFLUSH family */
    opcodes[opcode_count] = "clflush"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "clflushopt"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "clwb"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* MONITOR/MWAIT */
    opcodes[opcode_count] = "monitor"; opcode_values[opcode_count] = 0xC8; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "mwait"; opcode_values[opcode_count] = 0xC9; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* TSX instructions */
    opcodes[opcode_count] = "xbegin"; opcode_values[opcode_count] = 0xC7; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xend"; opcode_values[opcode_count] = 0xD5; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xabort"; opcode_values[opcode_count] = 0xC6; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "xtest"; opcode_values[opcode_count] = 0xD6; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* SWAPGS */
    opcodes[opcode_count] = "swapgs"; opcode_values[opcode_count] = 0xF8; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE */
    opcodes[opcode_count] = "rdfsbase"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "rdgsbase"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "wrfsbase"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "wrgsbase"; opcode_values[opcode_count] = 0xAE; opcode_types[opcode_count] = 1; opcode_count++;
    
    /* ENDBR (CET - Control-flow Enforcement Technology) */
    opcodes[opcode_count] = "endbr32"; opcode_values[opcode_count] = 0xFB; opcode_types[opcode_count] = 1; opcode_count++;
    opcodes[opcode_count] = "endbr64"; opcode_values[opcode_count] = 0xFA; opcode_types[opcode_count] = 1; opcode_count++;
    
    printf("\nInitialized %d instructions\n", opcode_count);
}

/* String comparison */
int strcmp(char *s1, char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/* String length */
int strlen(char *s) {
    int len;
    len = 0;
    while (*s) {
        len++;
        s++;
    }
    return len;
}

/* Parse a register and return its encoding */
int parse_register(char *reg, int *size) {
    int i;
    
    /* Check 64-bit registers */
    for (i = 0; i < 16; i++) {
        if (strcmp(reg, reg64_names[i]) == 0) {
            *size = 8;
            return i;
        }
    }
    
    /* Check 32-bit registers */
    for (i = 0; i < 16; i++) {
        if (strcmp(reg, reg32_names[i]) == 0) {
            *size = 4;
            return i;
        }
    }
    
    /* Check 16-bit registers */
    for (i = 0; i < 16; i++) {
        if (strcmp(reg, reg16_names[i]) == 0) {
            *size = 2;
            return i;
        }
    }
    
    /* Check 8-bit registers */
    for (i = 0; i < 16; i++) {
        if (strcmp(reg, reg8_names[i]) == 0) {
            *size = 1;
            return i;
        }
    }
    
    return -1;
}

/* Skip whitespace */
void skip_whitespace() {
    while (line[line_pos] == ' ' || line[line_pos] == '\t') {
        line_pos++;
    }
}

/* Skip whitespace in string */
void skip_whitespace_str(char *str, int *pos) {
    while (str[*pos] == ' ' || str[*pos] == '\t') {
        (*pos)++;
    }
}

/* Parse a number from string */
int parse_number_str(char *str) {
    int value, base, i;
    
    value = 0;
    base = 10;
    i = 0;
    
    /* Check for hex prefix */
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        i = 2;
    }
    
    while (str[i]) {
        if (str[i] >= '0' && str[i] <= '9') {
            value = value * base + (str[i] - '0');
        } else if (base == 16 && str[i] >= 'a' && str[i] <= 'f') {
            value = value * base + (str[i] - 'a' + 10);
        } else if (base == 16 && str[i] >= 'A' && str[i] <= 'F') {
            value = value * base + (str[i] - 'A' + 10);
        }
        i++;
    }
    
    return value;
}

/* Get next token */
int get_token() {
    int i;
    
    skip_whitespace();
    
    if (line[line_pos] == '\0' || line[line_pos] == '\n') {
        return 0;
    }
    
    i = 0;
    
    /* Check for special characters */
    if (line[line_pos] == ',' || line[line_pos] == '[' || 
        line[line_pos] == ']' || line[line_pos] == '+' ||
        line[line_pos] == '-' || line[line_pos] == '*') {
        token[0] = line[line_pos];
        token[1] = '\0';
        line_pos++;
        return 1;
    }
    
    /* Get alphanumeric token */
    while ((line[line_pos] >= 'a' && line[line_pos] <= 'z') ||
           (line[line_pos] >= 'A' && line[line_pos] <= 'Z') ||
           (line[line_pos] >= '0' && line[line_pos] <= '9') ||
           line[line_pos] == '_') {
        token[i] = line[line_pos];
        i++;
        line_pos++;
    }
    token[i] = '\0';
    
    return i > 0;
}

/* Parse a number */
int parse_number() {
    int value, base;
    char *p;
    
    value = 0;
    base = 10;
    p = token;
    
    /* Check for hex prefix */
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        base = 16;
        p = p + 2;
    }
    
    while (*p) {
        if (*p >= '0' && *p <= '9') {
            value = value * base + (*p - '0');
        } else if (base == 16 && *p >= 'a' && *p <= 'f') {
            value = value * base + (*p - 'a' + 10);
        } else if (base == 16 && *p >= 'A' && *p <= 'F') {
            value = value * base + (*p - 'A' + 10);
        }
        p++;
    }
    
    return value;
}

/* Emit a byte */
void emit_byte(int byte) {
    inst_bytes[inst_len] = byte & 0xFF;
    inst_len++;
}

/* Emit REX prefix if needed */
void emit_rex() {
    int rex;
    
    rex = 0x40;
    if (rex_w) rex = rex | 0x08;
    if (rex_r) rex = rex | 0x04;
    if (rex_x) rex = rex | 0x02;
    if (rex_b) rex = rex | 0x01;
    
    if (rex != 0x40) {
        emit_byte(rex);
    }
}

/* Emit ModRM byte */
void emit_modrm(int mod, int reg, int rm) {
    emit_byte((mod << 6) | ((reg & 7) << 3) | (rm & 7));
}

/* Emit SIB byte */
void emit_sib(int scale, int index, int base) {
    int scale_bits;
    
    scale_bits = 0;
    if (scale == 2) scale_bits = 1;
    else if (scale == 4) scale_bits = 2;
    else if (scale == 8) scale_bits = 3;
    
    emit_byte((scale_bits << 6) | ((index & 7) << 3) | (base & 7));
}

/* Emit immediate value */
void emit_immediate(int value, int size) {
    if (size == 1) {
        emit_byte(value);
    } else if (size == 2) {
        emit_byte(value & 0xFF);
        emit_byte((value >> 8) & 0xFF);
    } else if (size == 4) {
        emit_byte(value & 0xFF);
        emit_byte((value >> 8) & 0xFF);
        emit_byte((value >> 16) & 0xFF);
        emit_byte((value >> 24) & 0xFF);
    }
}

/* Parse memory operand like [rax], [rbx+8], [rax+rbx*2+16] */
int parse_memory_operand(char *mem, int *base_reg, int *index_reg, int *scale, int *disp) {
    int i, j, state;
    char reg_name[16];
    int reg_idx, reg_size;
    int sign;
    
    *base_reg = -1;
    *index_reg = -1;
    *scale = 1;
    *disp = 0;
    
    i = 0;
    state = 0;  /* 0=base, 1=index, 2=scale, 3=disp */
    sign = 1;
    
    /* Skip opening bracket */
    if (mem[i] == '[') i++;
    
    while (mem[i] && mem[i] != ']') {
        skip_whitespace_str(mem, &i);
        
        /* Check for sign */
        if (mem[i] == '+') {
            sign = 1;
            i++;
            skip_whitespace_str(mem, &i);
        } else if (mem[i] == '-') {
            sign = -1;
            i++;
            skip_whitespace_str(mem, &i);
        }
        
        /* Check if it's a register */
        j = 0;
        while ((mem[i] >= 'a' && mem[i] <= 'z') || 
               (mem[i] >= 'A' && mem[i] <= 'Z') ||
               (mem[i] >= '0' && mem[i] <= '9')) {
            reg_name[j] = mem[i];
            j++;
            i++;
        }
        reg_name[j] = '\0';
        
        if (j > 0) {
            /* Try to parse as register */
            reg_idx = parse_register(reg_name, &reg_size);
            if (reg_idx >= 0) {
                if (state == 0 && *base_reg == -1) {
                    *base_reg = reg_idx;
                    state = 1;
                } else if (state == 1 && *index_reg == -1) {
                    *index_reg = reg_idx;
                    state = 2;
                }
            } else {
                /* Must be a number */
                *disp = *disp + sign * parse_number_str(reg_name);
                state = 3;
            }
        }
        
        /* Check for scale */
        if (mem[i] == '*') {
            i++;
            skip_whitespace_str(mem, &i);
            if (mem[i] >= '0' && mem[i] <= '9') {
                *scale = mem[i] - '0';
                i++;
            }
        }
    }
    
    return (*base_reg >= 0) || (*disp != 0);
}

/* Parse a number from string */
int parse_number_str(char *str) {
    int value, base, i;
    
    value = 0;
    base = 10;
    i = 0;
    
    /* Check for hex prefix */
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        i = 2;
    }
    
    while (str[i]) {
        if (str[i] >= '0' && str[i] <= '9') {
            value = value * base + (str[i] - '0');
        } else if (base == 16 && str[i] >= 'a' && str[i] <= 'f') {
            value = value * base + (str[i] - 'a' + 10);
        } else if (base == 16 && str[i] >= 'A' && str[i] <= 'F') {
            value = value * base + (str[i] - 'A' + 10);
        }
        i++;
    }
    
    return value;
}

/* Assemble instruction with memory operand */
int assemble_mem_instruction(int opcode, int reg, int base, int index, int scale, int disp) {
    int mod, rm, need_sib;
    
    /* Emit REX if needed */
    rex_w = rex_r = rex_x = rex_b = 0;
    if (reg >= 8) rex_r = 1;
    if (index >= 8) rex_x = 1;
    if (base >= 8) rex_b = 1;
    emit_rex();
    
    /* Emit opcode */
    emit_byte(opcode);
    
    /* Determine ModRM encoding */
    need_sib = 0;
    
    if (base == -1 && index == -1) {
        /* Direct addressing [disp32] */
        mod = 0;
        rm = 5;
        emit_modrm(mod, reg & 7, rm);
        emit_immediate(disp, 4);
    } else if (index == -1 && base >= 0) {
        /* [base] or [base+disp] */
        if (base == 4 || base == 12) {
            /* RSP/R12 require SIB */
            need_sib = 1;
            rm = 4;
        } else {
            rm = base & 7;
        }
        
        if (disp == 0 && (base & 7) != 5) {
            mod = 0;
        } else if (disp >= -128 && disp <= 127) {
            mod = 1;
        } else {
            mod = 2;
        }
        
        emit_modrm(mod, reg & 7, rm);
        
        if (need_sib) {
            emit_sib(0, 4, base & 7);  /* No index */
        }
        
        if (mod == 1) {
            emit_byte(disp);
        } else if (mod == 2) {
            emit_immediate(disp, 4);
        }
    } else {
        /* [base+index*scale] with possible displacement */
        mod = (disp == 0) ? 0 : (disp >= -128 && disp <= 127) ? 1 : 2;
        rm = 4;  /* SIB follows */
        
        emit_modrm(mod, reg & 7, rm);
        emit_sib(scale, index & 7, base >= 0 ? base & 7 : 5);
        
        if (base == -1 || mod == 2) {
            emit_immediate(disp, 4);
        } else if (mod == 1) {
            emit_byte(disp);
        }
    }
    
    return 1;
}

/* Assemble a single operand instruction */
int assemble_single_operand(int opcode) {
    char operand[64];
    int reg, size;
    int i;
    
    /* Get operand */
    if (!get_token()) return 0;
    for (i = 0; token[i]; i++) operand[i] = token[i];
    operand[i] = '\0';
    
    reg = parse_register(operand, &size);
    
    if (reg >= 0) {
        /* Register operand */
        rex_w = rex_r = rex_x = rex_b = 0;
        
        if (size == 8) rex_w = 1;
        if (reg >= 8) rex_b = 1;
        
        emit_rex();
        
        if (opcode == 0x50 || opcode == 0x58) {
            /* PUSH/POP */
            emit_byte(opcode + (reg & 7));
        } else {
            emit_byte(opcode);
            emit_modrm(3, 0, reg);
        }
        return 1;
    }
    
    /* Try immediate value for JMP/CALL */
    if (opcode == 0xE9 || opcode == 0xE8) {
        emit_byte(opcode);
        emit_immediate(parse_number(), 4);
        return 1;
    }
    
    return 0;
}

/* Assemble a no-operand instruction */
int assemble_no_operand(int opcode) {
    if (opcode == 0x05) {
        /* SYSCALL is 0F 05 */
        emit_byte(0x0F);
        emit_byte(0x05);
    } else if (opcode == 0x07) {
        /* SYSRET is 0F 07 */
        emit_byte(0x0F);
        emit_byte(0x07);
    } else if (opcode == 0x34) {
        /* SYSENTER is 0F 34 */
        emit_byte(0x0F);
        emit_byte(0x34);
    } else if (opcode == 0x35) {
        /* SYSEXIT is 0F 35 */
        emit_byte(0x0F);
        emit_byte(0x35);
    } else if (opcode == 0xA2) {
        /* CPUID is 0F A2 */
        emit_byte(0x0F);
        emit_byte(0xA2);
    } else if (opcode == 0x31) {
        /* RDTSC is 0F 31 */
        emit_byte(0x0F);
        emit_byte(0x31);
    } else if (opcode == 0xF9) {
        /* RDTSCP is 0F 01 F9 */
        emit_byte(0x0F);
        emit_byte(0x01);
        emit_byte(0xF9);
    } else if (opcode == 0x33) {
        /* RDPMC is 0F 33 */
        emit_byte(0x0F);
        emit_byte(0x33);
    } else if (opcode == 0x32) {
        /* RDMSR is 0F 32 */
        emit_byte(0x0F);
        emit_byte(0x32);
    } else if (opcode == 0x30) {
        /* WRMSR is 0F 30 */
        emit_byte(0x0F);
        emit_byte(0x30);
    } else if (opcode == 0xF0 && inst_len == 0) {
        /* MFENCE is 0F AE F0 */
        emit_byte(0x0F);
        emit_byte(0xAE);
        emit_byte(0xF0);
    } else if (opcode == 0xE8 && inst_len == 0) {
        /* LFENCE is 0F AE E8 */
        emit_byte(0x0F);
        emit_byte(0xAE);
        emit_byte(0xE8);
    } else if (opcode == 0xF8 && inst_len == 0) {
        /* SFENCE is 0F AE F8 */
        emit_byte(0x0F);
        emit_byte(0xAE);
        emit_byte(0xF8);
    } else if (opcode == 0x90 && inst_len == 0) {
        /* PAUSE is F3 90 */
        emit_byte(0xF3);
        emit_byte(0x90);
    } else if (opcode == 0xD0 && inst_len == 0) {
        /* XGETBV is 0F 01 D0 */
        emit_byte(0x0F);
        emit_byte(0x01);
        emit_byte(0xD0);
    } else if (opcode == 0xD1) {
        /* XSETBV is 0F 01 D1 */
        emit_byte(0x0F);
        emit_byte(0x01);
        emit_byte(0xD1);
    } else if (opcode == 0xCC) {
        /* INT3 */
        emit_byte(0xCC);
    } else if (opcode == 0xCE) {
        /* INTO */
        emit_byte(0xCE);
    } else if (opcode == 0xCF) {
        /* IRET/IRETD */
        emit_byte(0xCF);
    } else {
        /* Single byte instructions */
        emit_byte(opcode);
    }
    return 1;
}

/* Assemble one line */
int assemble_line() {
    char mnemonic[32];
    int i, opcode, result;
    
    line_pos = 0;
    inst_len = 0;
    
    /* Get mnemonic */
    if (!get_token()) return 0;
    
    for (i = 0; token[i]; i++) mnemonic[i] = token[i];
    mnemonic[i] = '\0';
    
    /* Find opcode */
    opcode = -1;
    for (i = 0; i < opcode_count; i++) {
        if (strcmp(mnemonic, opcodes[i]) == 0) {
            opcode = opcode_values[i];
            break;
        }
    }
    
    if (opcode < 0) {
        puts("Unknown instruction: ");
        puts(mnemonic);
        puts("\n");
        return 0;
    }
    
    /* Assemble based on instruction type */
    result = 0;
    
    /* No operand instructions */
    if (strcmp(mnemonic, "nop") == 0 || strcmp(mnemonic, "ret") == 0 ||
        strcmp(mnemonic, "syscall") == 0 || strcmp(mnemonic, "cpuid") == 0 ||
        strcmp(mnemonic, "rdtsc") == 0 || strcmp(mnemonic, "rdtscp") == 0 ||
        strcmp(mnemonic, "rdpmc") == 0 || strcmp(mnemonic, "rdmsr") == 0 ||
        strcmp(mnemonic, "wrmsr") == 0 || strcmp(mnemonic, "sysret") == 0 ||
        strcmp(mnemonic, "sysenter") == 0 || strcmp(mnemonic, "sysexit") == 0 ||
        strcmp(mnemonic, "int3") == 0 || strcmp(mnemonic, "into") == 0 ||
        strcmp(mnemonic, "iret") == 0 || strcmp(mnemonic, "iretd") == 0 ||
        strcmp(mnemonic, "movsb") == 0 || strcmp(mnemonic, "movsw") == 0 ||
        strcmp(mnemonic, "movsd") == 0 || strcmp(mnemonic, "movsq") == 0 ||
        strcmp(mnemonic, "cmpsb") == 0 || strcmp(mnemonic, "cmpsw") == 0 ||
        strcmp(mnemonic, "cmpsd") == 0 || strcmp(mnemonic, "cmpsq") == 0 ||
        strcmp(mnemonic, "scasb") == 0 || strcmp(mnemonic, "scasw") == 0 ||
        strcmp(mnemonic, "scasd") == 0 || strcmp(mnemonic, "scasq") == 0 ||
        strcmp(mnemonic, "stosb") == 0 || strcmp(mnemonic, "stosw") == 0 ||
        strcmp(mnemonic, "stosd") == 0 || strcmp(mnemonic, "stosq") == 0 ||
        strcmp(mnemonic, "lodsb") == 0 || strcmp(mnemonic, "lodsw") == 0 ||
        strcmp(mnemonic, "lodsd") == 0 || strcmp(mnemonic, "lodsq") == 0 ||
        strcmp(mnemonic, "rep") == 0 || strcmp(mnemonic, "repe") == 0 ||
        strcmp(mnemonic, "repz") == 0 || strcmp(mnemonic, "repne") == 0 ||
        strcmp(mnemonic, "repnz") == 0 || strcmp(mnemonic, "leave") == 0 ||
        strcmp(mnemonic, "pushf") == 0 || strcmp(mnemonic, "popf") == 0 ||
        strcmp(mnemonic, "pusha") == 0 || strcmp(mnemonic, "popa") == 0 ||
        strcmp(mnemonic, "clc") == 0 || strcmp(mnemonic, "stc") == 0 ||
        strcmp(mnemonic, "cmc") == 0 || strcmp(mnemonic, "cld") == 0 ||
        strcmp(mnemonic, "std") == 0 || strcmp(mnemonic, "cli") == 0 ||
        strcmp(mnemonic, "sti") == 0 || strcmp(mnemonic, "hlt") == 0 ||
        strcmp(mnemonic, "wait") == 0 || strcmp(mnemonic, "cbw") == 0 ||
        strcmp(mnemonic, "cwde") == 0 || strcmp(mnemonic, "cdqe") == 0 ||
        strcmp(mnemonic, "cwd") == 0 || strcmp(mnemonic, "cdq") == 0 ||
        strcmp(mnemonic, "cqo") == 0 || strcmp(mnemonic, "lahf") == 0 ||
        strcmp(mnemonic, "sahf") == 0 || strcmp(mnemonic, "xlat") == 0 ||
        strcmp(mnemonic, "mfence") == 0 || strcmp(mnemonic, "lfence") == 0 ||
        strcmp(mnemonic, "sfence") == 0 || strcmp(mnemonic, "pause") == 0 ||
        strcmp(mnemonic, "lock") == 0 || strcmp(mnemonic, "xgetbv") == 0 ||
        strcmp(mnemonic, "xsetbv") == 0) {
        result = assemble_no_operand(opcode);
    } 
    /* Single operand instructions */
    else if (strcmp(mnemonic, "push") == 0 || strcmp(mnemonic, "pop") == 0 ||
             strcmp(mnemonic, "inc") == 0 || strcmp(mnemonic, "dec") == 0 ||
             strcmp(mnemonic, "neg") == 0 || strcmp(mnemonic, "not") == 0 ||
             strcmp(mnemonic, "mul") == 0 || strcmp(mnemonic, "div") == 0 ||
             strcmp(mnemonic, "idiv") == 0 || strcmp(mnemonic, "bswap") == 0 ||
             strcmp(mnemonic, "jmp") == 0 || strcmp(mnemonic, "call") == 0 ||
             strcmp(mnemonic, "je") == 0 || strcmp(mnemonic, "jz") == 0 ||
             strcmp(mnemonic, "jne") == 0 || strcmp(mnemonic, "jnz") == 0 ||
             strcmp(mnemonic, "jl") == 0 || strcmp(mnemonic, "jnge") == 0 ||
             strcmp(mnemonic, "jg") == 0 || strcmp(mnemonic, "jnle") == 0 ||
             strcmp(mnemonic, "jle") == 0 || strcmp(mnemonic, "jng") == 0 ||
             strcmp(mnemonic, "jge") == 0 || strcmp(mnemonic, "jnl") == 0 ||
             strcmp(mnemonic, "jb") == 0 || strcmp(mnemonic, "jnae") == 0 ||
             strcmp(mnemonic, "jc") == 0 || strcmp(mnemonic, "ja") == 0 ||
             strcmp(mnemonic, "jnbe") == 0 || strcmp(mnemonic, "jbe") == 0 ||
             strcmp(mnemonic, "jna") == 0 || strcmp(mnemonic, "jae") == 0 ||
             strcmp(mnemonic, "jnb") == 0 || strcmp(mnemonic, "jnc") == 0 ||
             strcmp(mnemonic, "js") == 0 || strcmp(mnemonic, "jns") == 0 ||
             strcmp(mnemonic, "jo") == 0 || strcmp(mnemonic, "jno") == 0 ||
             strcmp(mnemonic, "jp") == 0 || strcmp(mnemonic, "jpe") == 0 ||
             strcmp(mnemonic, "jnp") == 0 || strcmp(mnemonic, "jpo") == 0 ||
             strcmp(mnemonic, "jcxz") == 0 || strcmp(mnemonic, "jecxz") == 0 ||
             strcmp(mnemonic, "loop") == 0 || strcmp(mnemonic, "loope") == 0 ||
             strcmp(mnemonic, "loopne") == 0 || strcmp(mnemonic, "int") == 0 ||
             strcmp(mnemonic, "sete") == 0 || strcmp(mnemonic, "setz") == 0 ||
             strcmp(mnemonic, "setne") == 0 || strcmp(mnemonic, "setnz") == 0 ||
             strcmp(mnemonic, "setl") == 0 || strcmp(mnemonic, "setnge") == 0 ||
             strcmp(mnemonic, "setg") == 0 || strcmp(mnemonic, "setnle") == 0 ||
             strcmp(mnemonic, "setle") == 0 || strcmp(mnemonic, "setng") == 0 ||
             strcmp(mnemonic, "setge") == 0 || strcmp(mnemonic, "setnl") == 0 ||
             strcmp(mnemonic, "setb") == 0 || strcmp(mnemonic, "setnae") == 0 ||
             strcmp(mnemonic, "setc") == 0 || strcmp(mnemonic, "seta") == 0 ||
             strcmp(mnemonic, "setnbe") == 0 || strcmp(mnemonic, "setbe") == 0 ||
             strcmp(mnemonic, "setna") == 0 || strcmp(mnemonic, "setae") == 0 ||
             strcmp(mnemonic, "setnb") == 0 || strcmp(mnemonic, "setnc") == 0 ||
             strcmp(mnemonic, "sets") == 0 || strcmp(mnemonic, "setns") == 0 ||
             strcmp(mnemonic, "seto") == 0 || strcmp(mnemonic, "setno") == 0 ||
             strcmp(mnemonic, "setp") == 0 || strcmp(mnemonic, "setpe") == 0 ||
             strcmp(mnemonic, "setnp") == 0 || strcmp(mnemonic, "setpo") == 0 ||
             strcmp(mnemonic, "prefetch") == 0 || strcmp(mnemonic, "prefetchw") == 0 ||
             strcmp(mnemonic, "prefetchnta") == 0) {
        result = assemble_single_operand(opcode);
    } 
    /* Two operand instructions (default) */
    else {
        result = assemble_two_operand(opcode);
    }
    
    /* Output assembled bytes */
    if (result) {
        for (i = 0; i < inst_len; i++) {
            output[output_pos] = inst_bytes[i];
            output_pos++;
        }
    }
    
    return result;
}

/* Output hex dump */
void output_hex() {
    int i, j;
    
    puts("Assembled output:\n");
    
    for (i = 0; i < output_pos; i = i + 16) {
        /* Print address */
        printf("%04x: ", i);
        
        /* Print hex bytes */
        for (j = 0; j < 16 && i + j < output_pos; j++) {
            printf("%02x ", output[i + j] & 0xFF);
        }
        
        /* Pad if needed */
        while (j < 16) {
            puts("   ");
            j++;
        }
        
        puts(" ");
        
        /* Print ASCII */
        for (j = 0; j < 16 && i + j < output_pos; j++) {
            if (output[i + j] >= 32 && output[i + j] < 127) {
                putchar(output[i + j]);
            } else {
                putchar('.');
            }
        }
        
        puts("\n");
    }
    
    printf("\nTotal bytes: %d\n", output_pos);
}

/* Main assembler */
int main() {
    int result;
    
    puts("SAS x64 Assembler - Small Assembler System for x64\n");
    puts("Supporting 200+ x64 instructions\n");
    puts("Commands: 'help' - list instructions, 'exit' - quit\n");
    puts("Enter assembly instructions:\n");
    
    /* Initialize tables */
    init_registers();
    init_opcodes();
    
    output_pos = 0;
    
    /* Read and assemble lines */
    while (1) {
        puts("> ");
        gets(line);
        
        if (line[0] == '\0' || line[0] == '\n') {
            continue;
        }
        
        if (strcmp(line, "exit") == 0) {
            break;
        }
        
        if (strcmp(line, "help") == 0) {
            puts("\nSupported instruction categories (300+ instructions):\n");
            puts("Data Movement: mov, movzx, movsx, movbe, lea, xchg, xadd, cmpxchg, bswap\n");
            puts("Arithmetic: add, adc, sub, sbb, mul, imul, div, idiv, inc, dec, neg\n");
            puts("           adcx, adox\n");
            puts("Logic: and, or, xor, not, andn\n");
            puts("Shift/Rotate: shl, shr, sal, sar, rol, ror, rcl, rcr, shld, shrd\n");
            puts("Bit Ops: bt, bts, btr, btc, bsf, bsr, popcnt, lzcnt, tzcnt\n");
            puts("Compare: cmp, test\n");
            puts("Conditional Move: cmove/cmovz, cmovne/cmovnz, cmovl, cmovg, cmovle, cmovge\n");
            puts("                  cmovb/cmovc, cmova, cmovbe, cmovae/cmovnc, cmovs, cmovns\n");
            puts("                  cmovo, cmovno, cmovp/cmovpe, cmovnp/cmovpo\n");
            puts("Jumps: jmp, je/jz, jne/jnz, jl/jnge, jg/jnle, jle/jng, jge/jnl\n");
            puts("       jb/jc/jnae, ja/jnbe, jbe/jna, jae/jnc/jnb, js, jns, jo, jno\n");
            puts("       jp/jpe, jnp/jpo, jcxz, jecxz, loop, loope, loopne\n");
            puts("Set: sete/setz, setne/setnz, setl/setnge, setg/setnle, setle/setng\n");
            puts("     setge/setnl, setb/setc/setnae, seta/setnbe, setbe/setna\n");
            puts("     setae/setnc/setnb, sets, setns, seto, setno, setp/setpe, setnp/setpo\n");
            puts("Stack: push, pop, pushf, popf, pusha, popa, leave, enter\n");
            puts("String: movs[bwdq], cmps[bwdq], scas[bwdq], stos[bwdq], lods[bwdq]\n");
            puts("Prefix: rep, repe/repz, repne/repnz, lock\n");
            puts("Call: call, ret, retn, int, int3, into, iret, iretd\n");
            puts("System: syscall, sysret, sysenter, sysexit, cpuid, rdtsc, rdtscp\n");
            puts("        rdpmc, rdmsr, wrmsr, xgetbv, xsetbv, nop, hlt, pause\n");
            puts("        invd, wbinvd, invlpg, lgdt, lidt, sgdt, sidt, lldt, sldt\n");
            puts("        ltr, str, lmsw, smsw, clts, arpl, lar, lsl, verr, verw, rsm\n");
            puts("        swapgs, rdfsbase, rdgsbase, wrfsbase, wrgsbase\n");
            puts("Memory: mfence, lfence, sfence, prefetch, prefetchw, prefetchnta\n");
            puts("        clflush, clflushopt, clwb\n");
            puts("Flags: clc, stc, cmc, cld, std, cli, sti, lahf, sahf\n");
            puts("Convert: cbw, cwde, cdqe, cwd, cdq, cqo, xlat\n");
            puts("BMI: andn, bextr, blsi, blsmsk, blsr, bzhi, mulx, pdep, pext\n");
            puts("     rorx, sarx, shlx, shrx\n");
            puts("I/O: in, out, ins[bwd], outs[bwd]\n");
            puts("Segment: lds, les, lfs, lgs, lss\n");
            puts("BCD: daa, das, aaa, aas, aam, aad\n");
            puts("XSAVE: xsave, xsavec, xsaveopt, xrstor\n");
            puts("Random: rdrand, rdseed\n");
            puts("TSX: xbegin, xend, xabort, xtest\n");
            puts("CRC: crc32\n");
            puts("Monitor: monitor, mwait\n");
            puts("CET: endbr32, endbr64\n");
            puts("Debug: ud0, ud1, ud2\n");
            puts("\nAddressing modes supported:\n");
            puts("  mov rax, rbx          ; register to register\n");
            puts("  mov rax, [rbx]        ; memory to register\n");
            puts("  mov [rax], rbx        ; register to memory\n");
            puts("  mov rax, 123          ; immediate to register\n");
            puts("  mov [rax+8], rbx      ; displacement\n");
            puts("  mov [rax+rbx*2+16], rcx ; base + index*scale + disp\n");
            continue;
        }
        
        result = assemble_line();
        if (!result) {
            puts("Assembly error\n");
        }
    }
    
    /* Output results */
    if (output_pos > 0) {
        output_hex();
    }
    
    return 0;
}
