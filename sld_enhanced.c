/* sld_enhanced.c - Enhanced Simple Linker for Small-C */
/* Extended version with better ELF support and more features */

/* ELF constants */
#define EI_NIDENT 16
#define ET_EXEC 2
#define EM_X86_64 62
#define EM_AARCH64 183
#define EV_CURRENT 1
#define PT_LOAD 1
#define PT_GNU_STACK 0x6474e551
#define PF_X 1
#define PF_W 2
#define PF_R 4

/* Section types */
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9

/* Section flags */
#define SHF_WRITE 1
#define SHF_ALLOC 2
#define SHF_EXECINSTR 4

/* Symbol binding */
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2

/* Symbol types */
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4

/* x64 relocation types */
#define R_X86_64_NONE 0
#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_GOT32 3
#define R_X86_64_PLT32 4
#define R_X86_64_32 10
#define R_X86_64_32S 11
#define R_X86_64_PC64 24
#define R_X86_64_GOTPCREL 9

/* ARM64 relocation types */
#define R_AARCH64_NONE 0
#define R_AARCH64_ABS64 257
#define R_AARCH64_ABS32 258
#define R_AARCH64_CALL26 283
#define R_AARCH64_JUMP26 282
#define R_AARCH64_ADR_PREL_PG_HI21 275
#define R_AARCH64_ADD_ABS_LO12_NC 277
#define R_AARCH64_LDST64_ABS_LO12_NC 286

/* Base address and alignment */
#define BASE_ADDR 0x400000
#define PAGE_SIZE 0x1000
#define SECTION_ALIGN 16

/* Maximum sizes */
#define MAX_SECTIONS 64
#define MAX_SYMBOLS 2048
#define MAX_RELOCS 4096
#define MAX_NAME 256
#define MAX_FILES 32
#define BUF_SIZE 262144 /* 256KB */
#define OUTPUT_SIZE 1048576 /* 1MB */

/* Global data */
char output[OUTPUT_SIZE];
int output_size;

/* Section tracking */
struct {
    char name[MAX_NAME];
    int type;
    int flags;
    int file_offset;
    int size;
    int vaddr;
    int align;
    char *data;
} sections[MAX_SECTIONS];
int section_count;

/* Symbol tracking */
struct {
    char name[MAX_NAME];
    int value;
    int size;
    int section;
    int type;
    int binding;
    int defined;
} symbols[MAX_SYMBOLS];
int symbol_count;

/* Relocation tracking */
struct {
    int offset;
    int symbol;
    int type;
    int addend;
    int section;
} relocs[MAX_RELOCS];
int reloc_count;

/* Architecture flag */
int is_arm64 = 0;

/* Helper functions */
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
    write_u32(buf + 4, 0);
}

/* Find or add section */
int find_section(char *name) {
    int i;
    for (i = 0; i < section_count; i++) {
        if (strcmp(sections[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int add_section(char *name, int type, int flags, int align) {
    int idx = find_section(name);
    if (idx >= 0) return idx;
    
    strcpy(sections[section_count].name, name);
    sections[section_count].type = type;
    sections[section_count].flags = flags;
    sections[section_count].align = align;
    sections[section_count].size = 0;
    sections[section_count].data = output + output_size;
    return section_count++;
}

/* Find or add symbol */
int find_symbol(char *name) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int add_symbol(char *name, int section, int value, int size, int type, int binding) {
    int idx = find_symbol(name);
    if (idx < 0) {
        idx = symbol_count++;
        strcpy(symbols[idx].name, name);
        symbols[idx].section = section;
        symbols[idx].value = value;
        symbols[idx].size = size;
        symbols[idx].type = type;
        symbols[idx].binding = binding;
        symbols[idx].defined = 0;
    }
    return idx;
}

/* Process ELF object file */
int process_object(char *filename) {
    static char filebuf[BUF_SIZE];
    int fd, size, i, j;
    char *shstrtab, *strtab;
    int shnum, shoff, shentsize;
    int machine;
    
    fd = open(filename, 0);
    if (fd < 0) {
        printf("Error: Cannot open %s\n", filename);
        return -1;
    }
    
    size = read(fd, filebuf, BUF_SIZE);
    close(fd);
    
    /* Check ELF header */
    if (memcmp(filebuf, "\177ELF", 4) != 0) {
        puts("Error: Not an ELF file\n");
        return -1;
    }
    
    /* Check architecture */
    machine = read_u16(filebuf + 18);
    if (machine == EM_AARCH64) {
        is_arm64 = 1;
    } else if (machine != EM_X86_64) {
        puts("Error: Unsupported architecture\n");
        return -1;
    }
    
    /* Get section headers */
    shoff = read_u32(filebuf + 32);
    shentsize = read_u16(filebuf + 46);
    shnum = read_u16(filebuf + 48);
    
    /* Get section name string table */
    i = read_u16(filebuf + 50);
    shstrtab = filebuf + read_u32(filebuf + shoff + i * shentsize + 16);
    
    /* First pass: collect sections */
    for (i = 1; i < shnum; i++) {
        char *shdr = filebuf + shoff + i * shentsize;
        char *name = shstrtab + read_u32(shdr);
        int type = read_u32(shdr + 4);
        int flags = read_u32(shdr + 8);
        int offset = read_u32(shdr + 16);
        int sh_size = read_u32(shdr + 20);
        int align = read_u32(shdr + 32);
        
        if (type == SHT_PROGBITS && (flags & SHF_ALLOC)) {
            int sect = add_section(name, type, flags, align);
            
            /* Append data */
            memcpy(sections[sect].data + sections[sect].size, 
                   filebuf + offset, sh_size);
            sections[sect].size += sh_size;
            output_size += sh_size;
            
            /* Align output */
            while (output_size & (SECTION_ALIGN - 1)) {
                output[output_size++] = 0;
            }
        } else if (type == SHT_NOBITS && (flags & SHF_ALLOC)) {
            /* BSS section */
            int sect = add_section(name, type, flags, align);
            sections[sect].size += sh_size;
        }
    }
    
    /* Second pass: collect symbols and relocations */
    for (i = 0; i < shnum; i++) {
        char *shdr = filebuf + shoff + i * shentsize;
        int type = read_u32(shdr + 4);
        int offset = read_u32(shdr + 16);
        int sh_size = read_u32(shdr + 20);
        int link = read_u32(shdr + 24);
        int info = read_u32(shdr + 28);
        int entsize = read_u32(shdr + 36);
        
        if (type == SHT_SYMTAB) {
            /* Get string table for symbols */
            strtab = filebuf + read_u32(filebuf + shoff + link * shentsize + 16);
            
            /* Process symbols */
            for (j = 0; j < sh_size; j += entsize) {
                char *sym = filebuf + offset + j;
                char *name = strtab + read_u32(sym);
                int value = read_u32(sym + 4);
                int size = read_u32(sym + 8);
                int st_info = read_u8(sym + 12);
                int shndx = read_u16(sym + 14);
                int binding = st_info >> 4;
                int stype = st_info & 0xF;
                
                if (*name && binding != STB_LOCAL) {
                    /* Map section index */
                    int sect = -1;
                    if (shndx > 0 && shndx < shnum) {
                        char *sect_name = shstrtab + 
                            read_u32(filebuf + shoff + shndx * shentsize);
                        sect = find_section(sect_name);
                    }
                    
                    int idx = add_symbol(name, sect, value, size, stype, binding);
                    if (sect >= 0) {
                        symbols[idx].defined = 1;
                    }
                }
            }
        } else if (type == SHT_RELA) {
            /* Get target section */
            char *target_name = shstrtab + 
                read_u32(filebuf + shoff + info * shentsize);
            int target_sect = find_section(target_name);
            
            if (target_sect >= 0) {
                /* Process relocations */
                for (j = 0; j < sh_size; j += entsize) {
                    char *rel = filebuf + offset + j;
                    relocs[reloc_count].offset = read_u32(rel);
                    relocs[reloc_count].type = read_u32(rel + 8);
                    relocs[reloc_count].symbol = read_u32(rel + 12);
                    relocs[reloc_count].addend = read_u32(rel + 16);
                    relocs[reloc_count].section = target_sect;
                    reloc_count++;
                }
            }
        }
    }
    
    return 0;
}

/* Layout sections in memory */
void layout_sections() {
    int vaddr = BASE_ADDR + PAGE_SIZE;
    int i;
    
    /* Layout code sections first */
    for (i = 0; i < section_count; i++) {
        if (sections[i].flags & SHF_EXECINSTR) {
            sections[i].vaddr = vaddr;
            vaddr += sections[i].size;
            vaddr = (vaddr + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
        }
    }
    
    /* Then data sections */
    vaddr = (vaddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    for (i = 0; i < section_count; i++) {
        if (!(sections[i].flags & SHF_EXECINSTR) && sections[i].type != SHT_NOBITS) {
            sections[i].vaddr = vaddr;
            vaddr += sections[i].size;
            vaddr = (vaddr + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
        }
    }
    
    /* Finally BSS */
    for (i = 0; i < section_count; i++) {
        if (sections[i].type == SHT_NOBITS) {
            sections[i].vaddr = vaddr;
            vaddr += sections[i].size;
            vaddr = (vaddr + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
        }
    }
}

/* Apply relocations */
void apply_relocations() {
    int i;
    
    for (i = 0; i < reloc_count; i++) {
        int sect = relocs[i].section;
        int offset = relocs[i].offset;
        int sym = relocs[i].symbol;
        int type = relocs[i].type;
        int addend = relocs[i].addend;
        
        /* Calculate addresses */
        int pc = sections[sect].vaddr + offset;
        int target = 0;
        
        if (sym < symbol_count && symbols[sym].defined) {
            int sym_sect = symbols[sym].section;
            if (sym_sect >= 0) {
                target = sections[sym_sect].vaddr + symbols[sym].value;
            }
        } else if (sym < symbol_count) {
            printf("Warning: undefined symbol %s\n", symbols[sym].name);
            continue;
        }
        
        /* Apply relocation based on type */
        char *loc = sections[sect].data + offset;
        
        if (is_arm64) {
            /* ARM64 relocations */
            int *instr = (int *)loc;
            
            switch (type) {
            case R_AARCH64_ABS64:
                write_u64(loc, target + addend);
                break;
                
            case R_AARCH64_ABS32:
                write_u32(loc, target + addend);
                break;
                
            case R_AARCH64_CALL26:
            case R_AARCH64_JUMP26:
                /* 26-bit PC-relative branch */
                *instr = (*instr & 0xFC000000) | 
                    (((target + addend - pc) >> 2) & 0x3FFFFFF);
                break;
                
            case R_AARCH64_ADR_PREL_PG_HI21:
                /* ADRP - page address */
                {
                    int page_offset = ((target + addend) & ~0xFFF) - (pc & ~0xFFF);
                    int immlo = (page_offset >> 12) & 3;
                    int immhi = (page_offset >> 14) & 0x7FFFF;
                    *instr = (*instr & 0x9F00001F) | (immlo << 29) | (immhi << 5);
                }
                break;
                
            case R_AARCH64_ADD_ABS_LO12_NC:
            case R_AARCH64_LDST64_ABS_LO12_NC:
                /* Low 12 bits */
                *instr = (*instr & 0xFFC003FF) | (((target + addend) & 0xFFF) << 10);
                break;
            }
        } else {
            /* x86-64 relocations */
            switch (type) {
            case R_X86_64_64:
            case R_X86_64_PC64:
                write_u64(loc, target + addend);
                break;
                
            case R_X86_64_32:
            case R_X86_64_32S:
                write_u32(loc, target + addend);
                break;
                
            case R_X86_64_PC32:
            case R_X86_64_PLT32:
            case R_X86_64_GOTPCREL:
                write_u32(loc, target + addend - pc);
                break;
            }
        }
    }
}

/* Write output executable */
int write_executable(char *filename) {
    static char header[PAGE_SIZE];
    int fd, i;
    int entry_addr = BASE_ADDR + PAGE_SIZE;
    int phnum = 3; /* Code, data, stack */
    
    /* Find entry point */
    i = find_symbol("_start");
    if (i >= 0 && symbols[i].defined) {
        entry_addr = sections[symbols[i].section].vaddr + symbols[i].value;
    } else {
        i = find_symbol("main");
        if (i >= 0 && symbols[i].defined) {
            entry_addr = sections[symbols[i].section].vaddr + symbols[i].value;
        }
    }
    
    /* Create output file */
    fd = creat(filename);
    if (fd < 0) {
        printf("Error: Cannot create %s\n", filename);
        return -1;
    }
    
    /* Build ELF header */
    memset(header, 0, sizeof(header));
    memcpy(header, "\177ELF\2\1\1", 7);
    write_u16(header + 16, ET_EXEC);
    write_u16(header + 18, is_arm64 ? EM_AARCH64 : EM_X86_64);
    write_u32(header + 20, EV_CURRENT);
    write_u64(header + 24, entry_addr);
    write_u64(header + 32, 64); /* Program header offset */
    write_u64(header + 40, 0);  /* Section header offset */
    write_u32(header + 48, 0);  /* Flags */
    write_u16(header + 52, 64); /* ELF header size */
    write_u16(header + 54, 56); /* Program header size */
    write_u16(header + 56, phnum);
    
    /* Program header 1: Code segment */
    char *ph = header + 64;
    write_u32(ph, PT_LOAD);
    write_u32(ph + 4, PF_R | PF_X);
    write_u64(ph + 8, 0);
    write_u64(ph + 16, BASE_ADDR);
    write_u64(ph + 24, BASE_ADDR);
    
    /* Calculate code size */
    int code_size = PAGE_SIZE;
    for (i = 0; i < section_count; i++) {
        if (sections[i].flags & SHF_EXECINSTR) {
            int end = sections[i].vaddr - BASE_ADDR + sections[i].size;
            if (end > code_size) code_size = end;
        }
    }
    write_u64(ph + 32, code_size);
    write_u64(ph + 40, code_size);
    write_u64(ph + 48, PAGE_SIZE);
    
    /* Program header 2: Data segment */
    ph += 56;
    write_u32(ph, PT_LOAD);
    write_u32(ph + 4, PF_R | PF_W);
    
    /* Find data range */
    int data_start = 0x7FFFFFFF;
    int data_end = 0;
    for (i = 0; i < section_count; i++) {
        if (!(sections[i].flags & SHF_EXECINSTR) && sections[i].size > 0) {
            if (sections[i].vaddr < data_start) 
                data_start = sections[i].vaddr;
            if (sections[i].vaddr + sections[i].size > data_end)
                data_end = sections[i].vaddr + sections[i].size;
        }
    }
    
    if (data_start < data_end) {
        write_u64(ph + 8, data_start - BASE_ADDR);
        write_u64(ph + 16, data_start);
        write_u64(ph + 24, data_start);
        write_u64(ph + 32, data_end - data_start);
        write_u64(ph + 40, data_end - data_start + PAGE_SIZE); /* Extra for BSS */
        write_u64(ph + 48, PAGE_SIZE);
    }
    
    /* Program header 3: GNU stack */
    ph += 56;
    write_u32(ph, PT_GNU_STACK);
    write_u32(ph + 4, PF_R | PF_W);
    
    /* Write header */
    write(fd, header, PAGE_SIZE);
    
    /* Write sections */
    int file_offset = PAGE_SIZE;
    for (i = 0; i < section_count; i++) {
        if (sections[i].type != SHT_NOBITS && sections[i].size > 0) {
            int padding = sections[i].vaddr - BASE_ADDR - file_offset;
            while (padding-- > 0) {
                write(fd, "", 1);
                file_offset++;
            }
            write(fd, sections[i].data, sections[i].size);
            file_offset += sections[i].size;
        }
    }
    
    close(fd);
    chmod(filename, 0755);
    
    return 0;
}

/* Main function */
int main(int argc, char *argv[]) {
    int i;
    char *output_file = "a.out";
    int file_count = 0;
    char *files[MAX_FILES];
    
    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] == '-') {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            files[file_count++] = argv[i];
        }
    }
    
    if (file_count == 0) {
        puts("Usage: sld_enhanced [-o output] file1.o file2.o ...\n");
        return 1;
    }
    
    /* Initialize */
    section_count = 0;
    symbol_count = 0;
    reloc_count = 0;
    output_size = 0;
    
    /* Process all object files */
    for (i = 0; i < file_count; i++) {
        if (process_object(files[i]) < 0) {
            return 1;
        }
    }
    
    /* Layout sections */
    layout_sections();
    
    /* Apply relocations */
    apply_relocations();
    
    /* Write output */
    if (write_executable(output_file) < 0) {
        return 1;
    }
    
    printf("Linked %s successfully (%d sections, %d symbols)\n", 
           output_file, section_count, symbol_count);
    
    return 0;
}
