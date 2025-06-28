/* sld_win_enhanced.c - Enhanced Windows PE Linker for Small-C */
/* Supports both x64 and ARM64 Windows executables */

/* PE/COFF constants */
#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020
#define IMAGE_FILE_32BIT_MACHINE 0x0100

/* Subsystems */
#define IMAGE_SUBSYSTEM_WINDOWS_GUI 2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3

/* DLL characteristics */
#define IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA 0x0020
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT 0x0100
#define IMAGE_DLLCHARACTERISTICS_NO_SEH 0x0400
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE 0x8000

/* Section characteristics */
#define IMAGE_SCN_CNT_CODE 0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000

/* AMD64 Relocation types */
#define IMAGE_REL_AMD64_ABSOLUTE 0x0000
#define IMAGE_REL_AMD64_ADDR64 0x0001
#define IMAGE_REL_AMD64_ADDR32 0x0002
#define IMAGE_REL_AMD64_ADDR32NB 0x0003
#define IMAGE_REL_AMD64_REL32 0x0004
#define IMAGE_REL_AMD64_REL32_1 0x0005
#define IMAGE_REL_AMD64_REL32_2 0x0006
#define IMAGE_REL_AMD64_REL32_3 0x0007
#define IMAGE_REL_AMD64_REL32_4 0x0008
#define IMAGE_REL_AMD64_REL32_5 0x0009
#define IMAGE_REL_AMD64_SECTION 0x000A
#define IMAGE_REL_AMD64_SECREL 0x000B

/* ARM64 Relocation types */
#define IMAGE_REL_ARM64_ABSOLUTE 0x0000
#define IMAGE_REL_ARM64_ADDR32 0x0001
#define IMAGE_REL_ARM64_ADDR32NB 0x0002
#define IMAGE_REL_ARM64_BRANCH26 0x0003
#define IMAGE_REL_ARM64_PAGEBASE_REL21 0x0004
#define IMAGE_REL_ARM64_REL21 0x0005
#define IMAGE_REL_ARM64_PAGEOFFSET_12A 0x0006
#define IMAGE_REL_ARM64_PAGEOFFSET_12L 0x0007
#define IMAGE_REL_ARM64_SECTION 0x000D
#define IMAGE_REL_ARM64_ADDR64 0x000E
#define IMAGE_REL_ARM64_BRANCH19 0x000F

/* Storage classes */
#define IMAGE_SYM_CLASS_EXTERNAL 2
#define IMAGE_SYM_CLASS_STATIC 3
#define IMAGE_SYM_CLASS_LABEL 6

/* Base addresses and alignment */
#define IMAGE_BASE 0x140000000
#define SECTION_ALIGN 0x1000
#define FILE_ALIGN 0x200

/* Maximum sizes */
#define MAX_SECTIONS 32
#define MAX_SYMBOLS 2048
#define MAX_RELOCS 4096
#define MAX_NAME 256
#define MAX_FILES 32
#define BUF_SIZE 262144
#define OUTPUT_SIZE 1048576

/* Global data */
char output[OUTPUT_SIZE];
int output_size;
int is_arm64 = 0;
int subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

/* Section tracking */
struct {
    char name[MAX_NAME];
    int size;
    int rva;
    int file_offset;
    int characteristics;
    char *data;
    int data_size;
} sections[MAX_SECTIONS];
int section_count;

/* Symbol tracking */
struct {
    char name[MAX_NAME];
    int value;
    int section;
    int type;
    int storage;
    int defined;
} symbols[MAX_SYMBOLS];
int symbol_count;

/* Relocation tracking */
struct {
    int offset;
    int symbol;
    int type;
    int section;
} relocs[MAX_RELOCS];
int reloc_count;

/* Helper functions */
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

int align_up(int val, int align) {
    return (val + align - 1) & ~(align - 1);
}

/* Find section by name */
int find_section(char *name) {
    int i;
    for (i = 0; i < section_count; i++) {
        if (strcmp(sections[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Add or merge section */
int add_section(char *name, int characteristics) {
    int idx = find_section(name);
    if (idx >= 0) {
        /* Merge characteristics */
        sections[idx].characteristics |= characteristics;
        return idx;
    }
    
    /* New section */
    strcpy(sections[section_count].name, name);
    sections[section_count].characteristics = characteristics;
    sections[section_count].size = 0;
    sections[section_count].data = output + output_size;
    sections[section_count].data_size = 0;
    return section_count++;
}

/* Find symbol */
int find_symbol(char *name) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Add symbol */
int add_symbol(char *name, int section, int value, int storage) {
    int idx = find_symbol(name);
    if (idx < 0) {
        idx = symbol_count++;
        strcpy(symbols[idx].name, name);
    }
    
    /* Update if better definition */
    if (storage == IMAGE_SYM_CLASS_EXTERNAL && section > 0) {
        symbols[idx].section = section - 1;
        symbols[idx].value = value;
        symbols[idx].storage = storage;
        symbols[idx].defined = 1;
    }
    
    return idx;
}

/* Process COFF object file */
int process_coff(char *filename) {
    static char buf[BUF_SIZE];
    int fd, size, i, j;
    int machine, nsections, nsymbols, symtab_offset;
    char *symtab, *strtab;
    int section_map[MAX_SECTIONS];
    
    fd = open(filename, 0);
    if (fd < 0) {
        printf("Error: Cannot open %s\n", filename);
        return -1;
    }
    
    size = read(fd, buf, BUF_SIZE);
    close(fd);
    
    /* Read COFF header */
    machine = read_u16(buf);
    if (machine == IMAGE_FILE_MACHINE_ARM64) {
        is_arm64 = 1;
    } else if (machine != IMAGE_FILE_MACHINE_AMD64) {
        printf("Error: Unsupported machine type %x\n", machine);
        return -1;
    }
    
    nsections = read_u16(buf + 2);
    symtab_offset = read_u32(buf + 8);
    nsymbols = read_u32(buf + 12);
    
    /* Get string table */
    symtab = buf + symtab_offset;
    strtab = symtab + nsymbols * 18;
    
    /* Process sections */
    char *shdr = buf + 20;
    for (i = 0; i < nsections; i++) {
        char name[256];
        int vsize = read_u32(shdr + 8);
        int raw_size = read_u32(shdr + 16);
        int raw_offset = read_u32(shdr + 20);
        int reloc_offset = read_u32(shdr + 24);
        int nrelocs = read_u16(shdr + 32);
        int characteristics = read_u32(shdr + 36);
        
        /* Get section name */
        if (shdr[0] == '/') {
            /* Long name */
            strcpy(name, strtab + atoi(shdr + 1));
        } else {
            memcpy(name, shdr, 8);
            name[8] = 0;
        }
        
        /* Skip debug sections */
        if (strncmp(name, ".debug", 6) == 0 || strcmp(name, ".pdata") == 0) {
            section_map[i] = -1;
            shdr += 40;
            continue;
        }
        
        /* Add/merge section */
        int sect = add_section(name, characteristics);
        section_map[i] = sect;
        
        /* Append data */
        if (raw_size > 0) {
            memcpy(sections[sect].data + sections[sect].data_size, 
                   buf + raw_offset, raw_size);
            sections[sect].data_size += raw_size;
            sections[sect].size += vsize ? vsize : raw_size;
            output_size += raw_size;
            
            /* Align */
            while (output_size & 15) {
                output[output_size++] = 0;
                sections[sect].data_size++;
            }
        } else if (vsize > 0) {
            /* BSS section */
            sections[sect].size += vsize;
        }
        
        /* Process relocations */
        if (nrelocs > 0 && reloc_offset > 0) {
            char *rel = buf + reloc_offset;
            int base_offset = sections[sect].data_size - raw_size;
            
            for (j = 0; j < nrelocs; j++) {
                relocs[reloc_count].offset = read_u32(rel) + base_offset;
                relocs[reloc_count].symbol = read_u32(rel + 4);
                relocs[reloc_count].type = read_u16(rel + 8);
                relocs[reloc_count].section = sect;
                reloc_count++;
                rel += 10;
            }
        }
        
        shdr += 40;
    }
    
    /* Process symbols */
    for (i = 0; i < nsymbols; i++) {
        char *sym = symtab + i * 18;
        char name[256];
        int value = read_u32(sym + 8);
        int section = read_u16(sym + 12);
        int storage = read_u8(sym + 16);
        int naux = read_u8(sym + 17);
        
        /* Get symbol name */
        if (read_u32(sym) == 0) {
            strcpy(name, strtab + read_u32(sym + 4));
        } else {
            memcpy(name, sym, 8);
            name[8] = 0;
        }
        
        /* Map section index */
        if (section > 0 && section <= nsections) {
            section = section_map[section - 1] + 1;
        }
        
        /* Add symbol if relevant */
        if (*name && storage == IMAGE_SYM_CLASS_EXTERNAL) {
            add_symbol(name, section, value, storage);
        }
        
        i += naux;
    }
    
    return 0;
}

/* Layout sections */
void layout_sections() {
    int rva = 0x1000;
    int i;
    
    /* Sort sections: .text, .rdata, .data, .bss */
    char *order[] = {".text", ".rdata", ".data", ".bss", NULL};
    char **p;
    
    for (p = order; *p; p++) {
        i = find_section(*p);
        if (i >= 0 && sections[i].size > 0) {
            sections[i].rva = rva;
            rva += align_up(sections[i].size, SECTION_ALIGN);
        }
    }
    
    /* Other sections */
    for (i = 0; i < section_count; i++) {
        if (sections[i].rva == 0 && sections[i].size > 0) {
            sections[i].rva = rva;
            rva += align_up(sections[i].size, SECTION_ALIGN);
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
        
        if (sym >= symbol_count) continue;
        
        /* Get addresses */
        int target = 0;
        if (symbols[sym].defined && symbols[sym].section >= 0) {
            target = IMAGE_BASE + sections[symbols[sym].section].rva + 
                     symbols[sym].value;
        } else {
            printf("Warning: undefined symbol '%s'\n", symbols[sym].name);
            continue;
        }
        
        int pc = IMAGE_BASE + sections[sect].rva + offset;
        char *loc = sections[sect].data + offset;
        int *instr = (int *)loc;
        
        /* Apply based on architecture */
        if (is_arm64) {
            switch (type) {
            case IMAGE_REL_ARM64_ADDR64:
                write_u64(loc, target);
                break;
            case IMAGE_REL_ARM64_ADDR32:
                write_u32(loc, target);
                break;
            case IMAGE_REL_ARM64_ADDR32NB:
                write_u32(loc, target - IMAGE_BASE);
                break;
            case IMAGE_REL_ARM64_BRANCH26:
                *instr = (*instr & 0xFC000000) | (((target - pc) >> 2) & 0x3FFFFFF);
                break;
            case IMAGE_REL_ARM64_BRANCH19:
                *instr = (*instr & 0xFF00001F) | ((((target - pc) >> 2) & 0x7FFFF) << 5);
                break;
            case IMAGE_REL_ARM64_PAGEBASE_REL21:
                {
                    int page_offset = (target & ~0xFFF) - (pc & ~0xFFF);
                    int immlo = (page_offset >> 12) & 3;
                    int immhi = (page_offset >> 14) & 0x7FFFF;
                    *instr = (*instr & 0x9F00001F) | (immlo << 29) | (immhi << 5);
                }
                break;
            case IMAGE_REL_ARM64_PAGEOFFSET_12A:
                *instr = (*instr & 0xFFC003FF) | ((target & 0xFFF) << 10);
                break;
            case IMAGE_REL_ARM64_PAGEOFFSET_12L:
                {
                    int shift = (*instr >> 30) & 3;
                    *instr = (*instr & 0xFFC003FF) | (((target & 0xFFF) >> shift) << 10);
                }
                break;
            }
        } else {
            switch (type) {
            case IMAGE_REL_AMD64_ADDR64:
                write_u64(loc, target);
                break;
            case IMAGE_REL_AMD64_ADDR32:
            case IMAGE_REL_AMD64_ADDR32NB:
                write_u32(loc, target - IMAGE_BASE);
                break;
            case IMAGE_REL_AMD64_REL32:
            case IMAGE_REL_AMD64_REL32_1:
            case IMAGE_REL_AMD64_REL32_2:
            case IMAGE_REL_AMD64_REL32_3:
            case IMAGE_REL_AMD64_REL32_4:
            case IMAGE_REL_AMD64_REL32_5:
                write_u32(loc, target - pc - 4 - (type - IMAGE_REL_AMD64_REL32));
                break;
            }
        }
    }
}

/* Write PE executable */
int write_pe(char *filename) {
    static char header[0x400];
    int fd, i;
    int entry_rva = 0;
    int code_size = 0, idata_size = 0, data_size = 0, bss_size = 0;
    
    /* Find entry point */
    char *entry_names[] = {"mainCRTStartup", "WinMainCRTStartup", 
                          "wmainCRTStartup", "_start", "main", NULL};
    char **p;
    for (p = entry_names; *p; p++) {
        i = find_symbol(*p);
        if (i >= 0 && symbols[i].defined) {
            entry_rva = sections[symbols[i].section].rva + symbols[i].value;
            /* GUI vs console detection */
            if (strstr(*p, "WinMain")) {
                subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
            }
            break;
        }
    }
    
    /* Calculate sizes */
    for (i = 0; i < section_count; i++) {
        if (sections[i].characteristics & IMAGE_SCN_CNT_CODE) {
            code_size += sections[i].size;
        } else if (sections[i].characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            if (sections[i].characteristics & IMAGE_SCN_MEM_WRITE) {
                data_size += sections[i].size;
            } else {
                idata_size += sections[i].size;
            }
        } else if (sections[i].characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            bss_size += sections[i].size;
        }
    }
    
    /* Create file */
    fd = creat(filename);
    if (fd < 0) {
        printf("Error: Cannot create %s\n", filename);
        return -1;
    }
    
    /* Build headers */
    memset(header, 0, sizeof(header));
    
    /* DOS header */
    write_u16(header, IMAGE_DOS_SIGNATURE);
    write_u16(header + 0x3C, 0x80);
    strcpy(header + 0x40, "This program cannot be run in DOS mode.\r\r\n$");
    
    /* PE signature */
    write_u32(header + 0x80, IMAGE_NT_SIGNATURE);
    
    /* COFF header */
    write_u16(header + 0x84, is_arm64 ? IMAGE_FILE_MACHINE_ARM64 : 
              IMAGE_FILE_MACHINE_AMD64);
    write_u16(header + 0x86, section_count);
    write_u32(header + 0x88, 0); /* Timestamp */
    write_u32(header + 0x8C, 0); /* Symbol table */
    write_u32(header + 0x90, 0); /* Number of symbols */
    write_u16(header + 0x94, 0xF0); /* Optional header size */
    write_u16(header + 0x96, IMAGE_FILE_EXECUTABLE_IMAGE | 
              IMAGE_FILE_LARGE_ADDRESS_AWARE);
    
    /* Optional header */
    char *opt = header + 0x98;
    write_u16(opt, 0x20B); /* PE32+ */
    write_u8(opt + 2, 14);
    write_u8(opt + 3, 0);
    write_u32(opt + 4, code_size);
    write_u32(opt + 8, idata_size);
    write_u32(opt + 12, bss_size);
    write_u32(opt + 16, entry_rva);
    write_u32(opt + 20, sections[0].rva); /* Base of code */
    write_u64(opt + 24, IMAGE_BASE);
    write_u32(opt + 32, SECTION_ALIGN);
    write_u32(opt + 36, FILE_ALIGN);
    write_u16(opt + 40, 10); /* OS version */
    write_u16(opt + 42, 0);
    write_u16(opt + 48, 10); /* Subsystem version */
    write_u16(opt + 50, 0);
    write_u32(opt + 56, 0);
    
    /* Calculate image size */
    int image_size = 0x1000;
    for (i = 0; i < section_count; i++) {
        if (sections[i].size > 0) {
            int end = sections[i].rva + align_up(sections[i].size, SECTION_ALIGN);
            if (end > image_size) image_size = end;
        }
    }
    write_u32(opt + 60, image_size);
    write_u32(opt + 64, 0x400); /* Headers size */
    write_u32(opt + 68, 0); /* Checksum */
    write_u16(opt + 72, subsystem);
    write_u16(opt + 74, IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA |
                         IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE |
                         IMAGE_DLLCHARACTERISTICS_NX_COMPAT |
                         IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE);
    write_u64(opt + 76, 0x100000); /* Stack reserve */
    write_u64(opt + 84, 0x1000);   /* Stack commit */
    write_u64(opt + 92, 0x100000); /* Heap reserve */
    write_u64(opt + 100, 0x1000);  /* Heap commit */
    write_u32(opt + 108, 0);
    write_u32(opt + 112, 16); /* Number of directories */
    
    /* Section headers */
    char *sect = header + 0x188;
    int file_offset = 0x400;
    
    for (i = 0; i < section_count; i++) {
        if (sections[i].size == 0) continue;
        
        strncpy(sect, sections[i].name, 8);
        write_u32(sect + 8, sections[i].size);
        write_u32(sect + 12, sections[i].rva);
        
        if (sections[i].data_size > 0) {
            write_u32(sect + 16, align_up(sections[i].data_size, FILE_ALIGN));
            write_u32(sect + 20, file_offset);
            sections[i].file_offset = file_offset;
            file_offset += align_up(sections[i].data_size, FILE_ALIGN);
        }
        
        write_u32(sect + 36, sections[i].characteristics);
        sect += 40;
    }
    
    /* Apply relocations */
    apply_relocations();
    
    /* Write file */
    write(fd, header, 0x400);
    
    /* Write sections */
    for (i = 0; i < section_count; i++) {
        if (sections[i].data_size > 0) {
            static char padding[FILE_ALIGN];
            int pad = align_up(sections[i].data_size, FILE_ALIGN) - 
                     sections[i].data_size;
            
            write(fd, sections[i].data, sections[i].data_size);
            if (pad > 0) {
                memset(padding, 0, pad);
                write(fd, padding, pad);
            }
        }
    }
    
    close(fd);
    
    printf("Created %s (%s, %d sections, %d symbols)\n", 
           filename, is_arm64 ? "ARM64" : "x64", section_count, symbol_count);
    
    return 0;
}

/* Main function */
int main(int argc, char *argv[]) {
    char *output_file = "a.exe";
    char *files[MAX_FILES];
    int file_count = 0;
    int i;
    
    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-subsystem:console") == 0) {
            subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
        } else if (strcmp(argv[i], "-subsystem:windows") == 0) {
            subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
        } else if (argv[i][0] == '-') {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            files[file_count++] = argv[i];
        }
    }
    
    if (file_count == 0) {
        puts("Usage: sld_win_enhanced [-o output.exe] [-subsystem:console|windows] file1.obj ...\n");
        return 1;
    }
    
    /* Initialize */
    section_count = 0;
    symbol_count = 0;
    reloc_count = 0;
    output_size = 0;
    
    /* Process all object files */
    for (i = 0; i < file_count; i++) {
        if (process_coff(files[i]) < 0) {
            return 1;
        }
    }
    
    /* Layout and link */
    layout_sections();
    
    /* Write output */
    return write_pe(output_file);
}
