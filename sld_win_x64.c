/* sld_win_x64.c - Simple PE Linker for Small-C (Windows x64) */
/* Creates Windows PE executables from COFF object files */

/* PE/COFF constants */
#define IMAGE_DOS_SIGNATURE 0x5A4D /* MZ */
#define IMAGE_NT_SIGNATURE 0x00004550 /* PE\0\0 */
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020

/* PE characteristics */
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3 /* Console app */
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT 0x0100
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE 0x8000

/* Section characteristics */
#define IMAGE_SCN_CNT_CODE 0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000

/* Relocation types */
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

/* Base addresses */
#define IMAGE_BASE 0x140000000
#define SECTION_ALIGN 0x1000
#define FILE_ALIGN 0x200

/* Sizes */
#define MAX_SECTIONS 16
#define MAX_SYMBOLS 1024
#define MAX_RELOCS 2048
#define MAX_NAME 256
#define BUF_SIZE 65536

/* Section data */
char section_names[MAX_SECTIONS][MAX_NAME];
int section_sizes[MAX_SECTIONS];
int section_rvas[MAX_SECTIONS];
int section_file_offsets[MAX_SECTIONS];
int section_characteristics[MAX_SECTIONS];
char *section_data[MAX_SECTIONS];
int section_count;

/* Symbol data */
char symbol_names[MAX_SYMBOLS][MAX_NAME];
int symbol_values[MAX_SYMBOLS];
int symbol_sections[MAX_SYMBOLS];
int symbol_defined[MAX_SYMBOLS];
int symbol_count;

/* Relocation data */
int reloc_offsets[MAX_RELOCS];
int reloc_symbols[MAX_RELOCS];
int reloc_types[MAX_RELOCS];
int reloc_sections[MAX_RELOCS];
int reloc_count;

/* Output buffer */
char output[BUF_SIZE];
int output_size;

/* Helpers */
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
    write_u32(buf + 4, 0); /* High part always 0 for our addresses */
}

/* Align value */
int align_up(int val, int align) {
    return (val + align - 1) & ~(align - 1);
}

/* Find symbol */
int find_symbol(char *name) {
    int i;
    for (i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Read COFF object file */
int read_coff(char *filename, char *buf) {
    int fd, size, i, j;
    int machine, nsections, nsymbols, symtab_offset;
    char *symtab, *strtab;
    
    fd = open(filename, 0);
    if (fd < 0) {
        puts("Error: Cannot open ");
        puts(filename);
        puts("\n");
        return -1;
    }
    
    size = read(fd, buf, BUF_SIZE);
    close(fd);
    
    /* Read COFF header */
    machine = read_u16(buf);
    if (machine != IMAGE_FILE_MACHINE_AMD64) {
        puts("Error: Not an x64 COFF file\n");
        return -1;
    }
    
    nsections = read_u16(buf + 2);
    symtab_offset = read_u32(buf + 8);
    nsymbols = read_u32(buf + 12);
    
    /* Get string table (after symbol table) */
    symtab = buf + symtab_offset;
    strtab = symtab + nsymbols * 18;
    
    /* Process sections */
    char *section_hdr = buf + 20; /* After COFF header */
    for (i = 0; i < nsections; i++) {
        char name[9];
        int vsize, raw_size, raw_offset, reloc_offset, nrelocs;
        int characteristics;
        
        /* Read section header */
        memcpy(name, section_hdr, 8);
        name[8] = 0;
        vsize = read_u32(section_hdr + 8);
        raw_size = read_u32(section_hdr + 16);
        raw_offset = read_u32(section_hdr + 20);
        reloc_offset = read_u32(section_hdr + 24);
        nrelocs = read_u16(section_hdr + 32);
        characteristics = read_u32(section_hdr + 36);
        
        /* Handle long section names */
        if (name[0] == '/') {
            int offset = atoi(name + 1);
            strcpy(section_names[section_count], strtab + offset);
        } else {
            strcpy(section_names[section_count], name);
        }
        
        section_sizes[section_count] = vsize ? vsize : raw_size;
        section_characteristics[section_count] = characteristics;
        section_data[section_count] = output + output_size;
        
        /* Copy section data */
        if (raw_size > 0) {
            memcpy(output + output_size, buf + raw_offset, raw_size);
            output_size = output_size + raw_size;
            
            /* Pad to alignment */
            while (output_size & 15) {
                output[output_size++] = 0;
            }
        }
        
        /* Process relocations */
        char *reloc = buf + reloc_offset;
        for (j = 0; j < nrelocs; j++) {
            reloc_offsets[reloc_count] = read_u32(reloc);
            reloc_symbols[reloc_count] = read_u32(reloc + 4);
            reloc_types[reloc_count] = read_u16(reloc + 8);
            reloc_sections[reloc_count] = section_count;
            reloc_count++;
            reloc = reloc + 10;
        }
        
        section_count++;
        section_hdr = section_hdr + 40;
    }
    
    /* Process symbols */
    for (i = 0; i < nsymbols; i++) {
        char *sym = symtab + i * 18;
        char name[256];
        int value = read_u32(sym + 8);
        int section = read_u16(sym + 12);
        int type = read_u16(sym + 14);
        int storage = read_u8(sym + 16);
        int naux = read_u8(sym + 17);
        
        /* Get symbol name */
        if (read_u32(sym) == 0) {
            /* Long name in string table */
            strcpy(name, strtab + read_u32(sym + 4));
        } else {
            /* Short name */
            memcpy(name, sym, 8);
            name[8] = 0;
        }
        
        /* Store external symbols */
        if (storage == 2 && *name && section > 0) { /* External */
            strcpy(symbol_names[symbol_count], name);
            symbol_values[symbol_count] = value;
            symbol_sections[symbol_count] = section - 1;
            symbol_defined[symbol_count] = section > 0;
            symbol_count++;
        }
        
        /* Skip auxiliary symbols */
        i = i + naux;
    }
    
    return 0;
}

/* Apply relocations */
void apply_relocations() {
    int i;
    
    for (i = 0; i < reloc_count; i++) {
        int section = reloc_sections[i];
        int offset = reloc_offsets[i];
        int symbol = reloc_symbols[i];
        int type = reloc_types[i];
        char *loc = section_data[section] + offset;
        
        /* Get target address */
        int target = 0;
        if (symbol < symbol_count && symbol_defined[symbol]) {
            int sym_section = symbol_sections[symbol];
            target = IMAGE_BASE + section_rvas[sym_section] + symbol_values[symbol];
        }
        
        /* Apply relocation */
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
            {
                int adjustment = type - IMAGE_REL_AMD64_REL32;
                int pc = IMAGE_BASE + section_rvas[section] + offset + 4 + adjustment;
                write_u32(loc, target - pc);
            }
            break;
        }
    }
}

/* Write PE executable */
int write_pe(char *filename) {
    char header[1024];
    int fd, i;
    int entry_rva = 0x1000; /* Default */
    int code_size = 0, data_size = 0, bss_size = 0;
    
    /* Find entry point */
    i = find_symbol("mainCRTStartup");
    if (i < 0) i = find_symbol("WinMainCRTStartup");
    if (i < 0) i = find_symbol("_start");
    if (i < 0) i = find_symbol("main");
    
    if (i >= 0 && symbol_defined[i]) {
        int section = symbol_sections[i];
        entry_rva = section_rvas[section] + symbol_values[i];
    }
    
    /* Calculate section sizes */
    for (i = 0; i < section_count; i++) {
        if (section_characteristics[i] & IMAGE_SCN_CNT_CODE) {
            code_size = code_size + section_sizes[i];
        } else if (section_characteristics[i] & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            data_size = data_size + section_sizes[i];
        } else if (section_characteristics[i] & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            bss_size = bss_size + section_sizes[i];
        }
    }
    
    /* Create output file */
    fd = creat(filename);
    if (fd < 0) {
        puts("Error: Cannot create output file\n");
        return -1;
    }
    
    memset(header, 0, sizeof(header));
    
    /* DOS header */
    write_u16(header, IMAGE_DOS_SIGNATURE);
    write_u16(header + 0x3C, 0x80); /* PE header offset */
    
    /* DOS stub */
    strcpy(header + 0x40, "This program cannot be run in DOS mode.\r\r\n$");
    
    /* PE signature */
    write_u32(header + 0x80, IMAGE_NT_SIGNATURE);
    
    /* COFF header */
    char *coff = header + 0x84;
    write_u16(coff, IMAGE_FILE_MACHINE_AMD64);
    write_u16(coff + 2, section_count);
    write_u32(coff + 4, 0); /* Timestamp */
    write_u32(coff + 8, 0); /* Symbol table */
    write_u32(coff + 12, 0); /* Number of symbols */
    write_u16(coff + 16, 0xF0); /* Size of optional header */
    write_u16(coff + 18, IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE);
    
    /* Optional header */
    char *opt = header + 0x98;
    write_u16(opt, 0x20B); /* PE32+ magic */
    write_u8(opt + 2, 14); /* Linker major version */
    write_u8(opt + 3, 0); /* Linker minor version */
    write_u32(opt + 4, code_size);
    write_u32(opt + 8, data_size);
    write_u32(opt + 12, bss_size);
    write_u32(opt + 16, entry_rva);
    write_u32(opt + 20, 0x1000); /* Base of code */
    write_u64(opt + 24, IMAGE_BASE);
    write_u32(opt + 32, SECTION_ALIGN);
    write_u32(opt + 36, FILE_ALIGN);
    write_u16(opt + 40, 6); /* OS major */
    write_u16(opt + 42, 0); /* OS minor */
    write_u16(opt + 48, 6); /* Subsystem major */
    write_u16(opt + 50, 0); /* Subsystem minor */
    write_u32(opt + 56, 0); /* Win32 version */
    write_u32(opt + 60, align_up(0x1000 + output_size, SECTION_ALIGN)); /* Image size */
    write_u32(opt + 64, 0x400); /* Headers size */
    write_u32(opt + 68, 0); /* Checksum */
    write_u16(opt + 72, IMAGE_SUBSYSTEM_WINDOWS_CUI);
    write_u16(opt + 74, IMAGE_DLLCHARACTERISTICS_NX_COMPAT | 
                         IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE);
    write_u64(opt + 76, 0x100000); /* Stack reserve */
    write_u64(opt + 84, 0x1000); /* Stack commit */
    write_u64(opt + 92, 0x100000); /* Heap reserve */
    write_u64(opt + 100, 0x1000); /* Heap commit */
    write_u32(opt + 108, 0); /* Loader flags */
    write_u32(opt + 112, 16); /* Number of directories */
    
    /* Section headers */
    char *sect = header + 0x188;
    int file_offset = 0x400;
    int rva = 0x1000;
    
    for (i = 0; i < section_count; i++) {
        /* Section name */
        strncpy(sect, section_names[i], 8);
        
        /* Virtual size and address */
        write_u32(sect + 8, section_sizes[i]);
        write_u32(sect + 12, rva);
        section_rvas[i] = rva;
        
        /* File size and offset */
        if (!(section_characteristics[i] & IMAGE_SCN_CNT_UNINITIALIZED_DATA)) {
            write_u32(sect + 16, align_up(section_sizes[i], FILE_ALIGN));
            write_u32(sect + 20, file_offset);
            section_file_offsets[i] = file_offset;
            file_offset = file_offset + align_up(section_sizes[i], FILE_ALIGN);
        }
        
        /* Characteristics */
        write_u32(sect + 36, section_characteristics[i]);
        
        sect = sect + 40;
        rva = rva + align_up(section_sizes[i], SECTION_ALIGN);
    }
    
    /* Apply relocations with final RVAs */
    apply_relocations();
    
    /* Write headers */
    write(fd, header, 0x400);
    
    /* Write sections */
    for (i = 0; i < section_count; i++) {
        if (!(section_characteristics[i] & IMAGE_SCN_CNT_UNINITIALIZED_DATA)) {
            char padding[FILE_ALIGN];
            int size = section_sizes[i];
            int padded_size = align_up(size, FILE_ALIGN);
            
            write(fd, section_data[i], size);
            
            /* Pad to file alignment */
            memset(padding, 0, padded_size - size);
            write(fd, padding, padded_size - size);
        }
    }
    
    close(fd);
    
    puts("Created ");
    puts(filename);
    puts(" successfully\n");
    
    return 0;
}

int main(int argc, char *argv[]) {
    char buf[BUF_SIZE];
    char *output_file = "a.exe";
    int i;
    
    if (argc < 2) {
        puts("Usage: sld_win_x64 [-o output.exe] file1.obj file2.obj ...\n");
        return 1;
    }
    
    /* Initialize */
    section_count = 0;
    symbol_count = 0;
    reloc_count = 0;
    output_size = 0;
    
    /* Parse arguments and read object files */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            if (read_coff(argv[i], buf) < 0) {
                return 1;
            }
        }
    }
    
    /* Assign RVAs to sections */
    int rva = 0x1000;
    for (i = 0; i < section_count; i++) {
        section_rvas[i] = rva;
        rva = rva + align_up(section_sizes[i], SECTION_ALIGN);
    }
    
    /* Write PE file */
    return write_pe(output_file);
}
