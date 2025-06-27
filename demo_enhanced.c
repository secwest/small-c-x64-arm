/*
 * Enhanced Small-C Demo Program
 * Demonstrates features of the enhanced compiler:
 * - Function parameters
 * - Local variable initialization
 * - Character literals
 * - Compound assignment operators
 * - Comments
 * - For loops
 */

/* Simple math functions with parameters */
int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    int result = 0;  /* Local initialization */
    int i;
    
    /* Multiplication by repeated addition */
    for (i = 0; i < y; i++) {
        result += x;  /* Compound assignment */
    }
    
    return result;
}

/* String length with parameter */
int strlen(char *str) {
    int len = 0;
    
    while (*str) {
        len++;
        str++;
    }
    
    return len;
}

/* Character utilities */
int isdigit(int c) {
    return c >= '0' && c <= '9';  /* Character literals */
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

/* String to integer conversion */
int atoi(char *s) {
    int n = 0;
    int neg = 0;
    
    /* Skip whitespace */
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    
    /* Check for negative */
    if (*s == '-') {
        neg = 1;
        s++;
    }
    
    /* Convert digits */
    while (isdigit(*s)) {
        n *= 10;
        n += *s - '0';
        s++;
    }
    
    return neg ? -n : n;
}

/* Factorial with recursion */
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

/* Quick sort implementation */
int partition(int *arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j;
    int temp;
    
    for (j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            /* Swap */
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    
    /* Place pivot */
    temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    
    return i + 1;
}

int quicksort(int *arr, int low, int high) {
    int pi;
    
    if (low < high) {
        pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
    
    return 0;
}

/* Print array */
int print_array(int *arr, int size) {
    int i;
    
    putchar('[');
    for (i = 0; i < size; i++) {
        printn(arr[i]);
        if (i < size - 1) {
            puts(", ");
        }
    }
    puts("]\n");
    
    return 0;
}

/* Main demonstration */
int main() {
    int numbers[10];
    int i;
    char buffer[50];
    int result;
    
    puts("Enhanced Small-C Compiler Demo");
    puts("==============================\n");
    
    /* Test arithmetic functions */
    puts("Arithmetic Functions:");
    result = add(15, 27);
    printf("  add(15, 27) = %d\n", result);
    
    result = multiply(12, 8);
    printf("  multiply(12, 8) = %d\n", result);
    
    result = factorial(6);
    printf("  factorial(6) = %d\n", result);
    puts("");
    
    /* Test string functions */
    puts("String Functions:");
    printf("  strlen(\"Hello, World!\") = %d\n", strlen("Hello, World!"));
    printf("  atoi(\"  -123\") = %d\n", atoi("  -123"));
    puts("");
    
    /* Test character functions */
    puts("Character Functions:");
    printf("  toupper('a') = %c\n", toupper('a'));
    printf("  toupper('Z') = %c\n", toupper('Z'));
    printf("  isdigit('5') = %d\n", isdigit('5'));
    printf("  isdigit('A') = %d\n", isdigit('A'));
    puts("");
    
    /* Test sorting */
    puts("Sorting Demo:");
    
    /* Initialize array with test data */
    numbers[0] = 64;
    numbers[1] = 34;
    numbers[2] = 25;
    numbers[3] = 12;
    numbers[4] = 22;
    numbers[5] = 11;
    numbers[6] = 90;
    numbers[7] = 88;
    numbers[8] = 76;
    numbers[9] = 3;
    
    puts("  Original array:");
    print_array(numbers, 10);
    
    quicksort(numbers, 0, 9);
    
    puts("  Sorted array:");
    print_array(numbers, 10);
    puts("");
    
    /* Test compound assignments */
    puts("Compound Assignments:");
    i = 10;
    printf("  i = %d\n", i);
    i += 5;
    printf("  i += 5: i = %d\n", i);
    i *= 2;
    printf("  i *= 2: i = %d\n", i);
    i -= 10;
    printf("  i -= 10: i = %d\n", i);
    i /= 4;
    printf("  i /= 4: i = %d\n", i);
    puts("");
    
    /* Interactive test */
    printf("Enter a number: ");
    gets(buffer);
    result = atoi(buffer);
    printf("You entered: %d\n", result);
    printf("Its factorial is: %d\n", factorial(result));
    
    puts("\nDemo completed successfully!");
    
    return 0;
}
