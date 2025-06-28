/* sld_linux_arm64.c - Simple Linker for Small-C (Linux ARM64) */
/* A minimal ELF linker that can be compiled by the Small-C compiler */

/* ELF constants */
#define EI_NIDENT 16
#define ET_EXEC 2
#define EM_AARCH64 183
#define EV_CURRENT 1
#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHF_ALLOC 2
#define SHF_EXECINSTR 4
#define STB_GLOBAL 1
#define STT_FUNC 2

/* ARM64 relocation types */
#define R_AARCH64_ABS64 257
#define R_AARCH64_CALL26 283
#define R_AARCH64_JUMP26 282
#define R_AARCH64_ADR_PREL_PG_HI21 275
#define R_AARCH64_ADD_ABS_LO12_NC 277

/* Base address for executable */
#define BASE_ADDR 0x400000
#define PAGE_SIZE 0x1000

/* Maximum sizes */
#define MAX_SECTIONS 32
#define MAX_SYMBOLS 1024
#define MAX_RELOCS 2048
#define MAX_NAME 256
#define BUF_SIZE 65536

/* Section info */
char section_names[MAX_SECTIONS][MAX_NAME];
int section_offsets[MAX_SECTIONS];
int section_sizes[MAX_SECTIONS];
int section_addrs[MAX_SECTIONS];
int section_types[MAX_SECTIONS];
int section_count;

/* Symbol info */
char symbol_names[MAX_SYMBOLS][MAX_NAME];
int symbol_values[MAX_SYMBOLS];
int symbol_sections[MAX_SYMBOLS];
int symbol_types[MAX_SYMBOLS];
int symbol_count;

/* Relocation info */
int reloc_offsets[MAX_RELOCS];
int reloc_symbols[MAX_RELOCS];
int reloc_types[MAX_RELOCS];
int reloc_addends[MAX_RELOCS];
int reloc_sections[MAX_RELOCS];
int reloc_count;

/* Output buffer */
char output[BUF_SIZE];
int output_size;

/* Current virtual address */
int current_vaddr;

/* Read little-endian values */
int read_u8(char *buf) {
    return buf[0] & 0xFF;
}

int read_u16(char *buf) {
    return (buf[0] & 0xFF) | ((buf[1] & 0xFF) << 8);
}

int read_u32(char *buf) {
    return (buf[0] & 0xFF) | ((buf[1] & 0xFF) << 8) |
           ((buf[2] & 0xFF) << 16) | ((buf[3] & 0xFF) << 24);
}

/* Write little-endian values */
void write_u8(char *buf, int val) {
    buf[0] = val & 0xFF;
}

void write_u16(char *buf, int val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}

void write_u32(char *buf, int val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

void write_u64(char *buf, int val) {
    write_u32(buf, val);
    write_u32(buf + 4, 0); /* High 32 bits always 0 for our addresses */
}

/* ARM64 instruction encoding helpers */
int encode_branch_offset(int offset) {
    /* Branch offset is in units of 4 bytes */
    return (offset >> 2) & 0x3FFFFFF;
}

int encode_adr_imm(int offset) {
    /* ADR immediate encoding for page offset */
    int page_offset = (offset >> 12) & 0x1FFFFF;
    return page_offset;
}

/* Find symbol by name */
int find_symbol(char *name) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Read ELF object file */
int read_object(char *filename) {
    int fd, size, i, j;
    char buf[BUF_SIZE];
    char *shstrtab;
    int shstrtab_offset;
    int symtab_offset, symtab_size, symtab_entsize;
    int strtab_offset;
    int shnum, shoff, shentsize;
    
    fd = open(filename, 0);
    if (fd < 0) {
        puts("Error: Cannot open ");
        puts(filename);
        puts("\n");
        return -1;
    }
    
    size = read(fd, buf, BUF_SIZE);
    close(fd);
    
    /* Check ELF magic */
    if (buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
        puts("Error: Not an ELF file\n");
        return -1;
    }
    
    /* Get section header info */
    shoff = read_u32(buf + 32); /* Use 32-bit offset for simplicity */
    shentsize = read_u16(buf + 46);
    shnum = read_u16(buf + 48);
    
    /* Get section header string table */
    i = read_u16(buf + 50);
    shstrtab_offset = read_u32(buf + shoff + i * shentsize + 16);
    shstrtab = buf + shstrtab_offset;
    
    /* Process sections */
    for (i = 0; i < shnum; i++) {
        char *shdr = buf + shoff + i * shentsize;
        int sh_name = read_u32(shdr);
        int sh_type = read_u32(shdr + 4);
        int sh_flags = read_u32(shdr + 8);
        int sh_offset = read_u32(shdr + 16);
        int sh_size = read_u32(shdr + 20);
        char *name = shstrtab + sh_name;
        
        if (sh_type == SHT_PROGBITS && (sh_flags & SHF_ALLOC)) {
            /* Code or data section */
            strcpy(section_names[section_count], name);
            section_offsets[section_count] = output_size;
            section_sizes[section_count] = sh_size;
            section_types[section_count] = sh_type;
            
            /* Copy section data */
            memcpy(output + output_size, buf + sh_offset, sh_size);
            output_size = output_size + sh_size;
            
            /* Align to 16 bytes */
            while (output_size & 15) {
                output[output_size] = 0;
                output_size = output_size + 1;
            }
            
            section_count = section_count + 1;
        } else if (sh_type == SHT_SYMTAB) {
            symtab_offset = sh_offset;
            symtab_size = sh_size;
            symtab_entsize = read_u32(shdr + 36);
            strtab_offset = read_u32(buf + shoff + read_u32(shdr + 24) * shentsize + 16);
        }
    }
    
    /* Process symbols */
    if (symtab_offset) {
        char *strtab = buf + strtab_offset;
        for (i = 0; i < symtab_size; i = i + symtab_entsize) {
            char *sym = buf + symtab_offset + i;
            int st_name = read_u32(sym);
            int st_value = read_u32(sym + 4);
            int st_shndx = read_u16(sym + 14);
            char *name = strtab + st_name;
            
            if (*name && st_shndx < section_count) {
                strcpy(symbol_names[symbol_count], name);
                symbol_values[symbol_count] = st_value;
                symbol_sections[symbol_count] = st_shndx;
                symbol_count = symbol_count + 1;
            }
        }
    }
    
    /* Process relocations */
    for (i = 0; i < shnum; i++) {
        char *shdr = buf + shoff + i * shentsize;
        int sh_type = read_u32(shdr + 4);
        
        if (sh_type == SHT_RELA) {
            int sh_offset = read_u32(shdr + 16);
            int sh_size = read_u32(shdr + 20);
            int sh_info = read_u32(shdr + 28);
            int entsize = read_u32(shdr + 36);
            
            for (j = 0; j < sh_size; j = j + entsize) {
                char *rel = buf + sh_offset + j;
                reloc_offsets[reloc_count] = read_u32(rel);
                reloc_types[reloc_count] = read_u32(rel + 8);
                reloc_symbols[reloc_count] = read_u32(rel + 12);
                reloc_addends[reloc_count] = read_u32(rel + 16);
                reloc_sections[reloc_count] = sh_info;
                reloc_count = reloc_count + 1;
            }
        }
    }
    
    return 0;
}

/* Apply relocations */
void apply_relocations() {
    int i;
    
    for (i = 0; i < reloc_count; i++) {
        int section = reloc_sections[i];
        int offset = section_offsets[section] + reloc_offsets[i];
        int sym = reloc_symbols[i];
        int type = reloc_types[i];
        int addend = reloc_addends[i];
        int sym_section = symbol_sections[sym];
        int sym_addr = section_addrs[sym_section] + symbol_values[sym];
        int pc = section_addrs[section] + reloc_offsets[i];
        int *instr = (int *)(output + offset);
        
        if (type == R_AARCH64_ABS64) {
            /* 64-bit absolute */
            write_u64(output + offset, sym_addr + addend);
        } else if (type == R_AARCH64_CALL26 || type == R_AARCH64_JUMP26) {
            /* 26-bit PC-relative branch */
            int branch_offset = sym_addr + addend - pc;
            int encoded = encode_branch_offset(branch_offset);
            *instr = (*instr & 0xFC000000) | encoded;
        } else if (type == R_AARCH64_ADR_PREL_PG_HI21) {
            /* ADRP instruction - page address */
            int page_base = pc & ~0xFFF;
            int target_page = (sym_addr + addend) & ~0xFFF;
            int page_offset = target_page - page_base;
            int immlo = (page_offset >> 12) & 0x3;
            int immhi = (page_offset >> 14) & 0x7FFFF;
            *instr = (*instr & 0x9F00001F) | (immlo << 29) | (immhi << 5);
        } else if (type == R_AARCH64_ADD_ABS_LO12_NC) {
            /* ADD immediate - low 12 bits */
            int imm = (sym_addr + addend) & 0xFFF;
            *instr = (*instr & 0xFFC003FF) | (imm << 10);
        }
    }
}

/* Write ELF header */
void write_elf_header(char *buf) {
    /* ELF magic */
    buf[0] = 0x7F;
    buf[1] = 'E';
    buf[2] = 'L';
    buf[3] = 'F';
    buf[4] = 2; /* 64-bit */
    buf[5] = 1; /* Little endian */
    buf[6] = 1; /* ELF version */
    buf[7] = 0; /* Generic ABI */
    
    /* Padding */
    memset(buf + 8, 0, 8);
    
    /* File type and machine */
    write_u16(buf + 16, ET_EXEC);
    write_u16(buf + 18, EM_AARCH64);
    write_u32(buf + 20, EV_CURRENT);
    
    /* Entry point */
    write_u64(buf + 24, BASE_ADDR + 0x1000);
    
    /* Program header offset */
    write_u64(buf + 32, 64);
    
    /* Section header offset (none) */
    write_u64(buf + 40, 0);
    
    /* Flags */
    write_u32(buf + 48, 0);
    
    /* Header sizes */
    write_u16(buf + 52, 64); /* ELF header size */
    write_u16(buf + 54, 56); /* Program header size */
    write_u16(buf + 56, 2);  /* Program header count */
    write_u16(buf + 58, 0);  /* Section header size */
    write_u16(buf + 60, 0);  /* Section header count */
    write_u16(buf + 62, 0);  /* String table index */
}

/* Write program headers */
void write_program_headers(char *buf) {
    /* First program header - all segments */
    write_u32(buf + 64, PT_LOAD);
    write_u32(buf + 68, PF_R | PF_X);
    write_u64(buf + 72, 0);          /* Offset */
    write_u64(buf + 80, BASE_ADDR);  /* Virtual address */
    write_u64(buf + 88, BASE_ADDR);  /* Physical address */
    write_u64(buf + 96, PAGE_SIZE + output_size); /* Size in file */
    write_u64(buf + 104, PAGE_SIZE + output_size); /* Size in memory */
    write_u64(buf + 112, PAGE_SIZE); /* Alignment */
    
    /* Second program header - data segment */
    write_u32(buf + 120, PT_LOAD);
    write_u32(buf + 124, PF_R | PF_W);
    write_u64(buf + 128, PAGE_SIZE + output_size);
    write_u64(buf + 136, BASE_ADDR + PAGE_SIZE + output_size);
    write_u64(buf + 144, BASE_ADDR + PAGE_SIZE + output_size);
    write_u64(buf + 152, PAGE_SIZE); /* Reserve space for data */
    write_u64(buf + 160, PAGE_SIZE);
    write_u64(buf + 168, PAGE_SIZE);
}

int main(int argc, char *argv[]) {
    int fd, i;
    char header[PAGE_SIZE];
    char *outfile;
    int start_sym;
    
    if (argc < 3) {
        puts("Usage: sld -o output input1.o input2.o ...\n");
        return 1;
    }
    
    /* Parse arguments */
    if (strcmp(argv[1], "-o") != 0) {
        puts("Error: Expected -o option\n");
        return 1;
    }
    outfile = argv[2];
    
    /* Initialize */
    output_size = 0;
    section_count = 0;
    symbol_count = 0;
    reloc_count = 0;
    current_vaddr = BASE_ADDR + PAGE_SIZE;
    
    /* Read all object files */
    for (i = 3; i < argc; i++) {
        if (read_object(argv[i]) < 0) {
            return 1;
        }
    }
    
    /* Assign addresses to sections */
    for (i = 0; i < section_count; i++) {
        section_addrs[i] = current_vaddr;
        current_vaddr = current_vaddr + section_sizes[i];
        
        /* Align */
        while (current_vaddr & 15) {
            current_vaddr = current_vaddr + 1;
        }
    }
    
    /* Apply relocations */
    apply_relocations();
    
    /* Find _start symbol */
    start_sym = find_symbol("_start");
    if (start_sym < 0) {
        puts("Error: _start symbol not found\n");
        return 1;
    }
    
    /* Create output file */
    fd = creat(outfile);
    if (fd < 0) {
        puts("Error: Cannot create output file\n");
        return 1;
    }
    
    /* Write ELF header and program headers */
    memset(header, 0, PAGE_SIZE);
    write_elf_header(header);
    write_program_headers(header);
    
    /* Update entry point to _start */
    write_u64(header + 24, section_addrs[symbol_sections[start_sym]] + 
              symbol_values[start_sym]);
    
    /* Write header and code */
    write(fd, header, PAGE_SIZE);
    write(fd, output, output_size);
    
    close(fd);
    
    /* Make executable */
    chmod(outfile, 0755);
    
    puts("Linked ");
    puts(outfile);
    puts(" successfully\n");
    
    return 0;
}
