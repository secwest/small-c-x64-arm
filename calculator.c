/*
 * Simple Calculator Application
 * Demonstrates a complete program using Small-C
 * 
 * Features:
 * - Interactive command-line interface
 * - Basic arithmetic operations
 * - Memory storage
 * - Expression evaluation
 */

/* Token types for expression parser */
#define T_NUM   1
#define T_PLUS  2
#define T_MINUS 3
#define T_MULT  4
#define T_DIV   5
#define T_LPAREN 6
#define T_RPAREN 7
#define T_EOF   8

/* Global variables */
char input[100];
char *ptr;
int token;
int tokval;
int memory;

/* Function prototypes (Small-C style) */
int expr();
int term();
int factor();
int gettoken();

/* Skip whitespace */
int skipws() {
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }
    return 0;
}

/* Get next token from input */
int gettoken() {
    skipws();
    
    if (*ptr == '\0') {
        return T_EOF;
    }
    
    /* Numbers */
    if (*ptr >= '0' && *ptr <= '9') {
        tokval = 0;
        while (*ptr >= '0' && *ptr <= '9') {
            tokval = tokval * 10 + (*ptr - '0');
            ptr++;
        }
        return T_NUM;
    }
    
    /* Operators */
    if (*ptr == '+') { ptr++; return T_PLUS; }
    if (*ptr == '-') { ptr++; return T_MINUS; }
    if (*ptr == '*') { ptr++; return T_MULT; }
    if (*ptr == '/') { ptr++; return T_DIV; }
    if (*ptr == '(') { ptr++; return T_LPAREN; }
    if (*ptr == ')') { ptr++; return T_RPAREN; }
    
    /* Unknown character */
    puts("Error: Unknown character '");
    putchar(*ptr);
    puts("'\n");
    return T_EOF;
}

/* Parse expression: expr = term (('+' | '-') term)* */
int expr() {
    int result;
    int right;
    int op;
    
    result = term();
    
    while (token == T_PLUS || token == T_MINUS) {
        op = token;
        token = gettoken();
        right = term();
        
        if (op == T_PLUS) {
            result = result + right;
        } else {
            result = result - right;
        }
    }
    
    return result;
}

/* Parse term: term = factor (('*' | '/') factor)* */
int term() {
    int result;
    int right;
    int op;
    
    result = factor();
    
    while (token == T_MULT || token == T_DIV) {
        op = token;
        token = gettoken();
        right = factor();
        
        if (op == T_MULT) {
            result = result * right;
        } else {
            if (right == 0) {
                puts("Error: Division by zero\n");
                return 0;
            }
            result = result / right;
        }
    }
    
    return result;
}

/* Parse factor: factor = number | '(' expr ')' | 'M' */
int factor() {
    int result;
    
    if (token == T_NUM) {
        result = tokval;
        token = gettoken();
        return result;
    }
    
    if (token == T_LPAREN) {
        token = gettoken();
        result = expr();
        if (token != T_RPAREN) {
            puts("Error: Missing closing parenthesis\n");
            return 0;
        }
        token = gettoken();
        return result;
    }
    
    /* Check for memory recall */
    if (*ptr == 'M' || *ptr == 'm') {
        ptr++;
        token = gettoken();
        return memory;
    }
    
    puts("Error: Expected number or '('\n");
    return 0;
}

/* Evaluate expression */
int evaluate(char *s) {
    int result;
    
    ptr = s;
    token = gettoken();
    
    if (token == T_EOF) {
        return 0;
    }
    
    result = expr();
    
    if (token != T_EOF) {
        puts("Error: Unexpected characters after expression\n");
        return 0;
    }
    
    return result;
}

/* Print help message */
int help() {
    puts("Simple Calculator Commands:\n");
    puts("  Expression  - Evaluate arithmetic expression");
    puts("  M          - Recall memory value");
    puts("  MS value   - Store value in memory");
    puts("  MC         - Clear memory");
    puts("  MR         - Display memory value");
    puts("  HELP       - Show this help");
    puts("  QUIT       - Exit calculator\n");
    puts("Examples:");
    puts("  2 + 3 * 4");
    puts("  (10 + 5) / 3");
    puts("  M * 2 + 1\n");
    return 0;
}

/* String comparison (simple version) */
int streq(char *s1, char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) {
            return 0;
        }
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

/* Convert string to uppercase */
int strupper(char *s) {
    while (*s) {
        if (*s >= 'a' && *s <= 'z') {
            *s = *s - ('a' - 'A');
        }
        s++;
    }
    return 0;
}

/* Main calculator loop */
int main() {
    int result;
    int running;
    
    puts("=================================");
    puts("    Small-C Calculator v1.0");
    puts("=================================");
    puts("Type 'HELP' for commands\n");
    
    memory = 0;
    running = 1;
    
    while (running) {
        printf("calc> ");
        gets(input);
        
        /* Convert to uppercase for commands */
        strupper(input);
        
        /* Check for commands */
        if (streq(input, "QUIT") || streq(input, "EXIT")) {
            running = 0;
        } else if (streq(input, "HELP")) {
            help();
        } else if (streq(input, "MC")) {
            memory = 0;
            puts("Memory cleared\n");
        } else if (streq(input, "MR")) {
            printf("Memory = %d\n\n", memory);
        } else if (input[0] == 'M' && input[1] == 'S' && input[2] == ' ') {
            /* Memory store */
            result = evaluate(input + 3);
            memory = result;
            printf("Stored %d in memory\n\n", memory);
        } else if (input[0] != '\0') {
            /* Evaluate expression */
            result = evaluate(input);
            printf("= %d\n\n", result);
        }
    }
    
    puts("Goodbye!");
    return 0;
}
