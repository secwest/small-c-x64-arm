/* Enhanced Small-C Preprocessor (sppe.c)
 * Supports: #include, #define (with parameters), #ifdef, #ifndef, 
 *           #else, #endif, #undef, #error, line continuation
 * Written in Small-C Enhanced dialect
 * This file can be preprocessed by spp.c for bootstrapping
 */

#define MAXLINE 512
#define MAXDEFINES 200
#define MAXNAMESIZE 64
#define MAXVALUESIZE 256
#define MAXPARAMS 10
#define MAXINCLUDES 16
#define MAXIFSTACK 32
#define TRUE 1
#define FALSE 0
#define NULL 0

/* Macro definition structure */
char defnames[MAXDEFINES][MAXNAMESIZE];
char defvalues[MAXDEFINES][MAXVALUESIZE];
char defparams[MAXDEFINES][MAXPARAMS][MAXNAMESIZE];
int defnparams[MAXDEFINES];
int defcount;

/* Include stack */
int includefd[MAXINCLUDES];
int includelevel;
int linenumber[MAXINCLUDES];
char filename[MAXINCLUDES][128];

/* Conditional compilation stack */
int ifstack[MAXIFSTACK];
int ifstacklevel;
int skipping;

/* Line buffer for continuation */
char linebuf[MAXLINE * 4];
int continuation;

/* Forward declarations */
int readline(int fd, char *buf, int max);
int startswith(char *str, char *prefix);
int whitespace(char c);
char *skipwhite(char *p);
char *skiptowhite(char *p);
int finddefine(char *name);
void processline(char *line);
void substitute(char *line);
int copyword(char *dst, char *src);
void processfile(int fd, char *name);
void expandmacro(char *dst, int defidx, char *args);
int parsemacroargs(char *src, char args[][MAXNAMESIZE]);
void error(char *msg);
void undefine(char *name);

int main(int argc, char **argv) {
    int fd;
    
    defcount = 0;
    includelevel = 0;
    ifstacklevel = 0;
    skipping = 0;
    continuation = 0;
    
    if (argc < 2) {
        puts("Usage: sppe filename.c");
        return 1;
    }
    
    fd = open(argv[1], 0);
    if (fd < 0) {
        error("Cannot open input file");
        return 1;
    }
    
    strcpy(filename[0], argv[1]);
    linenumber[0] = 0;
    
    /* Predefined macros */
    strcpy(defnames[defcount], "__SMALLC__");
    strcpy(defvalues[defcount], "1");
    defnparams[defcount] = -1; /* -1 means object-like macro */
    defcount++;
    
    processfile(fd, argv[1]);
    close(fd);
    
    if (ifstacklevel > 0) {
        error("Unterminated #ifdef/#ifndef");
    }
    
    return 0;
}

void processfile(int fd, char *name) {
    char line[MAXLINE];
    char *p;
    int len;
    
    includefd[includelevel] = fd;
    
    while ((len = readline(fd, line, MAXLINE)) > 0) {
        linenumber[includelevel]++;
        
        /* Handle line continuation */
        if (continuation) {
            strcat(linebuf, line);
        } else {
            strcpy(linebuf, line);
        }
        
        /* Check for continuation */
        p = linebuf + strlen(linebuf) - 1;
        while (p > linebuf && (*p == '\n' || *p == '\r')) p--;
        
        if (*p == '\\') {
            *p = 0;
            continuation = 1;
            continue;
        }
        
        continuation = 0;
        processline(linebuf);
    }
}

void processline(char *line) {
    char *p, *q;
    char incname[128];
    char defname[MAXNAMESIZE];
    char defvalue[MAXVALUESIZE];
    char params[MAXPARAMS][MAXNAMESIZE];
    int nparams;
    int fd;
    int i, j;
    int defined;
    
    p = skipwhite(line);
    
    /* Check for preprocessor directive */
    if (*p == '#') {
        p++;
        p = skipwhite(p);
        
        /* Handle #include */
        if (startswith(p, "include") && !skipping) {
            p = p + 7;
            p = skipwhite(p);
            
            if (*p == '"') {
                p++;
                i = 0;
                while (*p && *p != '"' && i < 127) {
                    incname[i++] = *p++;
                }
                incname[i] = 0;
                
                if (includelevel >= MAXINCLUDES - 1) {
                    error("Too many nested includes");
                    exit(1);
                }
                
                fd = open(incname, 0);
                if (fd < 0) {
                    fputs("Error: Cannot open include file: ", 2);
                    fputs(incname, 2);
                    fputc('\n', 2);
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
        /* Handle #define */
        else if (startswith(p, "define") && !skipping) {
            p = p + 6;
            p = skipwhite(p);
            
            /* Get macro name */
            i = 0;
            while (*p && !whitespace(*p) && *p != '(' && i < MAXNAMESIZE - 1) {
                defname[i++] = *p++;
            }
            defname[i] = 0;
            
            /* Check for function-like macro */
            nparams = -1; /* Object-like by default */
            if (*p == '(') {
                p++;
                nparams = 0;
                
                while (*p && *p != ')') {
                    p = skipwhite(p);
                    if (*p == ')') break;
                    
                    /* Get parameter name */
                    i = 0;
                    while (*p && !whitespace(*p) && *p != ',' && *p != ')' && i < MAXNAMESIZE - 1) {
                        params[nparams][i++] = *p++;
                    }
                    params[nparams][i] = 0;
                    nparams++;
                    
                    p = skipwhite(p);
                    if (*p == ',') p++;
                }
                
                if (*p == ')') p++;
            }
            
            p = skipwhite(p);
            
            /* Get macro value */
            i = 0;
            while (*p && *p != '\n' && i < MAXVALUESIZE - 1) {
                defvalue[i++] = *p++;
            }
            defvalue[i] = 0;
            
            /* Store definition */
            if (defcount < MAXDEFINES) {
                strcpy(defnames[defcount], defname);
                strcpy(defvalues[defcount], defvalue);
                defnparams[defcount] = nparams;
                
                for (j = 0; j < nparams; j++) {
                    strcpy(defparams[defcount][j], params[j]);
                }
                
                defcount++;
            }
        }
        /* Handle #undef */
        else if (startswith(p, "undef") && !skipping) {
            p = p + 5;
            p = skipwhite(p);
            
            i = 0;
            while (*p && !whitespace(*p) && i < MAXNAMESIZE - 1) {
                defname[i++] = *p++;
            }
            defname[i] = 0;
            
            undefine(defname);
        }
        /* Handle #ifdef */
        else if (startswith(p, "ifdef")) {
            p = p + 5;
            p = skipwhite(p);
            
            i = 0;
            while (*p && !whitespace(*p) && i < MAXNAMESIZE - 1) {
                defname[i++] = *p++;
            }
            defname[i] = 0;
            
            if (ifstacklevel >= MAXIFSTACK - 1) {
                error("Too many nested #ifdef");
                exit(1);
            }
            
            defined = (finddefine(defname) >= 0);
            ifstack[ifstacklevel++] = skipping;
            
            if (!defined && !skipping) {
                skipping = 1;
            }
        }
        /* Handle #ifndef */
        else if (startswith(p, "ifndef")) {
            p = p + 6;
            p = skipwhite(p);
            
            i = 0;
            while (*p && !whitespace(*p) && i < MAXNAMESIZE - 1) {
                defname[i++] = *p++;
            }
            defname[i] = 0;
            
            if (ifstacklevel >= MAXIFSTACK - 1) {
                error("Too many nested #ifndef");
                exit(1);
            }
            
            defined = (finddefine(defname) >= 0);
            ifstack[ifstacklevel++] = skipping;
            
            if (defined && !skipping) {
                skipping = 1;
            }
        }
        /* Handle #else */
        else if (startswith(p, "else")) {
            if (ifstacklevel <= 0) {
                error("#else without #ifdef/#ifndef");
                exit(1);
            }
            
            if (!ifstack[ifstacklevel - 1]) {
                skipping = !skipping;
            }
        }
        /* Handle #endif */
        else if (startswith(p, "endif")) {
            if (ifstacklevel <= 0) {
                error("#endif without #ifdef/#ifndef");
                exit(1);
            }
            
            skipping = ifstack[--ifstacklevel];
        }
        /* Handle #error */
        else if (startswith(p, "error") && !skipping) {
            p = p + 5;
            p = skipwhite(p);
            
            fputs("Error: #error ", 2);
            fputs(p, 2);
            exit(1);
        }
    }
    else if (!skipping) {
        /* Not a preprocessor directive, substitute macros and output */
        substitute(line);
        fputs(line, 1);
    }
}

void substitute(char *line) {
    char newline[MAXLINE * 2];
    char word[MAXNAMESIZE];
    char args[MAXPARAMS][MAXNAMESIZE];
    char expanded[MAXVALUESIZE * 2];
    char *src, *dst;
    int len, idx, nargs;
    
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
                /* Found a macro */
                src = src + len;
                
                if (defnparams[idx] >= 0) {
                    /* Function-like macro */
                    src = skipwhite(src);
                    if (*src == '(') {
                        src++;
                        nargs = parsemacroargs(src, args);
                        
                        /* Skip past arguments */
                        while (*src && *src != ')') src++;
                        if (*src == ')') src++;
                        
                        /* Expand macro with arguments */
                        expandmacro(expanded, idx, args);
                        strcpy(dst, expanded);
                        dst = dst + strlen(expanded);
                    }
                    else {
                        /* No parentheses, just copy the word */
                        strcpy(dst, word);
                        dst = dst + strlen(word);
                    }
                }
                else {
                    /* Object-like macro */
                    strcpy(dst, defvalues[idx]);
                    dst = dst + strlen(defvalues[idx]);
                }
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

void expandmacro(char *dst, int defidx, char args[][MAXNAMESIZE]) {
    char *src, *d;
    char word[MAXNAMESIZE];
    int i, j, len;
    
    src = defvalues[defidx];
    d = dst;
    
    while (*src) {
        if ((*src >= 'A' && *src <= 'Z') || 
            (*src >= 'a' && *src <= 'z') || 
            *src == '_') {
            /* Identifier */
            len = copyword(word, src);
            
            /* Check if it's a parameter */
            for (i = 0; i < defnparams[defidx]; i++) {
                if (strcmp(word, defparams[defidx][i]) == 0) {
                    /* Replace with argument */
                    strcpy(d, args[i]);
                    d = d + strlen(args[i]);
                    src = src + len;
                    break;
                }
            }
            
            if (i == defnparams[defidx]) {
                /* Not a parameter, copy as is */
                for (j = 0; j < len; j++) {
                    *d++ = *src++;
                }
            }
        }
        else {
            *d++ = *src++;
        }
    }
    *d = 0;
}

int parsemacroargs(char *src, char args[][MAXNAMESIZE]) {
    int nargs, i;
    int parens;
    char *p;
    
    nargs = 0;
    i = 0;
    parens = 0;
    
    p = skipwhite(src);
    
    while (*p && (*p != ')' || parens > 0)) {
        if (*p == '(') {
            parens++;
        }
        else if (*p == ')') {
            parens--;
        }
        else if (*p == ',' && parens == 0) {
            args[nargs][i] = 0;
            nargs++;
            i = 0;
            p++;
            p = skipwhite(p);
            continue;
        }
        
        if (i < MAXNAMESIZE - 1) {
            args[nargs][i++] = *p;
        }
        p++;
    }
    
    args[nargs][i] = 0;
    if (i > 0 || nargs > 0) nargs++;
    
    return nargs;
}

void undefine(char *name) {
    int idx, i, j;
    
    idx = finddefine(name);
    if (idx >= 0) {
        /* Remove by shifting remaining definitions */
        for (i = idx; i < defcount - 1; i++) {
            strcpy(defnames[i], defnames[i + 1]);
            strcpy(defvalues[i], defvalues[i + 1]);
            defnparams[i] = defnparams[i + 1];
            
            for (j = 0; j < defnparams[i]; j++) {
                strcpy(defparams[i][j], defparams[i + 1][j]);
            }
        }
        defcount--;
    }
}

int copyword(char *dst, char *src) {
    int len;
    
    len = 0;
    while ((*src >= 'A' && *src <= 'Z') || 
           (*src >= 'a' && *src <= 'z') || 
           (*src >= '0' && *src <= '9') || 
           *src == '_') {
        if (len < MAXNAMESIZE - 1) {
            dst[len] = *src;
        }
        src++;
        len++;
    }
    dst[len < MAXNAMESIZE ? len : MAXNAMESIZE - 1] = 0;
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
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char *skipwhite(char *p) {
    while (*p && (*p == ' ' || *p == '\t')) p++;
    return p;
}

char *skiptowhite(char *p) {
    while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
    return p;
}

void error(char *msg) {
    fputs("Error at ", 2);
    fputs(filename[includelevel], 2);
    fputc(':', 2);
    fputs(itoa(linenumber[includelevel]), 2);
    fputs(": ", 2);
    fputs(msg, 2);
    fputc('\n', 2);
}

/* Simple integer to string conversion */
char *itoa(int n) {
    static char buf[12];
    char *p;
    int neg;
    
    p = buf + 11;
    *p = 0;
    
    neg = 0;
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    
    do {
        *--p = '0' + (n % 10);
        n = n / 10;
    } while (n > 0);
    
    if (neg) *--p = '-';
    
    return p;
}
