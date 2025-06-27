/* Example program for Modern Small-C */

/* Simple string length function */
int strlen(char *s) {
    int n;
    n = 0;
    while (*s) {
        s = s + 1;
        n = n + 1;
    }
    return n;
}

/* Print a number */
int printn(int n) {
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    if (n >= 10) {
        printn(n / 10);
    }
    putchar('0' + (n % 10));
    return 0;
}

/* Print a string */
int puts(char *s) {
    while (*s) {
        putchar(*s);
        s = s + 1;
    }
    putchar('\n');
    return 0;
}

/* Fibonacci calculation */
int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

/* Main program */
int main() {
    char msg[20];
    int i;
    int result;
    
    puts("Small-C Compiler Demo");
    puts("====================");
    
    /* String operations */
    puts("String length of 'Hello': ");
    printn(strlen("Hello"));
    putchar('\n');
    
    /* Loops and arrays */
    puts("Counting from 1 to 5:");
    i = 1;
    while (i <= 5) {
        printn(i);
        putchar(' ');
        i = i + 1;
    }
    putchar('\n');
    
    /* Fibonacci */
    puts("Fibonacci numbers:");
    i = 0;
    while (i < 10) {
        result = fib(i);
        printn(result);
        putchar(' ');
        i = i + 1;
    }
    putchar('\n');
    
    return 0;
}

/* Required system call wrapper for putchar */
int putchar(int c) {
    /* This would need to be implemented in assembly
       or linked with a runtime library */
    return c;
}
