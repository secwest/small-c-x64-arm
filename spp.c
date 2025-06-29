/* Simple Small-C Preprocessor (spp.c)
 * Supports #include and #define (without parameters)
 * Written in Small-C dialect WITHOUT any preprocessor directives
 * This allows it to be compiled directly by Small-C for bootstrapping
 */

/* Constants - no preprocessor allowed for bootstrap */
/* MAXLINE = 256 */
/* MAXDEFINES = 100 */
/* MAXNAMESIZE = 32 */
/* MAXVALUESIZE = 128 */
/* MAXINCLUDES = 8 */

/* Global variables */
char line[256];
char defnames[100][32];
char defvalues[100][128];
int defcount;
int includefd[8];
int includelevel;
int linenumber[8];
char filename[8][64];

/* Forward declarations */
int readline(int fd, char *buf, int max);
int startswith(char *str, char *prefix);
int whitespace(char c);
char *skipwhite(char *p);
char *skipnonwhite(char *p);
int finddefine(char *name);
void processline(char *line);
void substitute(char *line);
int copyword(char *dst, char *src);
void processfile(int fd, char *name);

int main(int argc, char **argv) {
    int fd;
    
    defcount = 0;
    includelevel = 0;
    
    if (argc < 2) {
        puts("Usage: spp filename.c");
        return 1;
    }
    
    fd = open(argv[1], 0);
    if (fd < 0) {
        puts("Error: Cannot open input file");
        return 1;
    }
    
    strcpy(filename[0], argv[1]);
    linenumber[0] = 0;
    processfile(fd, argv[1]);
    close(fd);
    
    return 0;
}

void processfile(int fd, char *name) {
    includefd[includelevel] = fd;
    
    while (readline(fd, line, 256) > 0) {
        linenumber[includelevel]++;
        processline(line);
    }
}

void processline(char *line) {
    char *p;
    char incname[64];
    char defname[32];
    char defvalue[128];
    int fd;
    int i;
    
    p = skipwhite(line);
    
    /* Check for preprocessor directive */
    if (*p == '#') {
        p++;
        p = skipwhite(p);
        
        /* Handle include */
        if (startswith(p, "include")) {
            p = p + 7; /* skip "include" */
            p = skipwhite(p);
            
            if (*p == '"') {
                p++;
                i = 0;
                while (*p && *p != '"' && i < 63) {
                    incname[i++] = *p++;
                }
                incname[i] = 0;
                
                if (includelevel >= 7) {
                    puts("Error: Too many nested includes");
                    exit(1);
                }
                
                fd = open(incname, 0);
                if (fd < 0) {
                    puts("Error: Cannot open include file: ");
                    puts(incname);
                    exit(1);
                }
                
                includelevel++;
                strcpy(filename[includelevel], incname);
                linenumber[includelevel] = 0;
                processfile(fd, incname);
                close(fd);
                includelevel--;
            }
        }
        /* Handle define */
        else if (startswith(p, "define")) {
            p = p + 6; /* skip "define" */
            p = skipwhite(p);
            
            /* Get macro name */
            i = 0;
            while (*p && !whitespace(*p) && i < 31) {
                defname[i++] = *p++;
            }
            defname[i] = 0;
            
            p = skipwhite(p);
            
            /* Get macro value */
            i = 0;
            while (*p && *p != '\n' && i < 127) {
                defvalue[i++] = *p++;
            }
            defvalue[i] = 0;
            
            /* Store definition */
            if (defcount < 100) {
                strcpy(defnames[defcount], defname);
                strcpy(defvalues[defcount], defvalue);
                defcount++;
            }
        }
    }
    else {
        /* Not a preprocessor directive, substitute macros and output */
        substitute(line);
        puts(line);
    }
}

void substitute(char *line) {
    char newline[256];
    char word[32];
    char *src, *dst;
    int len, idx;
    
    src = line;
    dst = newline;
    
    while (*src) {
        if ((*src >= 'A' && *src <= 'Z') || 
            (*src >= 'a' && *src <= 'z') || 
            *src == '_') {
            /* Start of identifier */
            len = copyword(word, src);
            idx = finddefine(word);
            
            if (idx >= 0) {
                /* Found a macro, substitute */
                strcpy(dst, defvalues[idx]);
                dst = dst + strlen(defvalues[idx]);
                src = src + len;
            }
            else {
                /* Not a macro, copy word */
                while (len-- > 0) {
                    *dst++ = *src++;
                }
            }
        }
        else {
            /* Not an identifier character */
            *dst++ = *src++;
        }
    }
    *dst = 0;
    strcpy(line, newline);
}

int copyword(char *dst, char *src) {
    int len;
    
    len = 0;
    while ((*src >= 'A' && *src <= 'Z') || 
           (*src >= 'a' && *src <= 'z') || 
           (*src >= '0' && *src <= '9') || 
           *src == '_') {
        if (len < 31) {
            dst[len] = *src;
        }
        src++;
        len++;
    }
    dst[len < 32 ? len : 31] = 0;
    return len;
}

int finddefine(char *name) {
    int i;
    
    for (i = 0; i < defcount; i++) {
        if (strcmp(name, defnames[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int readline(int fd, char *buf, int max) {
    int n, i;
    char c;
    
    i = 0;
    while (i < max - 1) {
        n = read(fd, &c, 1);
        if (n <= 0) break;
        
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = 0;
    return i;
}

int startswith(char *str, char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

int whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

char *skipwhite(char *p) {
    while (*p && (*p == ' ' || *p == '\t')) p++;
    return p;
}

char *skipnonwhite(char *p) {
    while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
    return p;
}
