/*
 * Test Suite for Enhanced Small-C Compiler
 * Tests all enhanced features and error handling
 */

/* Test function parameters and local initialization */
int test_params(int a, int b, int c) {
    int sum = a + b + c;  /* Local initialization */
    return sum;
}

/* Test character literals and operations */
int test_chars() {
    char ch = 'A';
    char newline = '\n';
    char tab = '\t';
    char zero = '\0';
    
    if (ch >= 'A' && ch <= 'Z') {
        return 1;
    }
    return 0;
}

/* Test compound assignments */
int test_compound() {
    int x = 10;
    
    x += 5;   /* Should be 15 */
    x -= 3;   /* Should be 12 */
    x *= 2;   /* Should be 24 */
    x /= 4;   /* Should be 6 */
    
    return x;
}

/* Test nested loops and break/continue */
int test_nested_loops() {
    int i, j;
    int count = 0;
    
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (i == 1 && j == 1) {
                continue;
            }
            count++;
            if (count >= 5) {
                break;
            }
        }
        if (count >= 5) {
            break;
        }
    }
    
    return count;
}

/* Test string handling */
int test_strings() {
    char *s1 = "Hello";
    char *s2 = "World";
    char buffer[50];
    
    /* Test escape sequences */
    char *escaped = "Line 1\nLine 2\tTabbed\n";
    
    strcpy(buffer, s1);
    if (strcmp(buffer, s1) == 0) {
        return strlen(buffer);
    }
    
    return 0;
}

/* Test array operations */
int test_arrays() {
    int arr[10];
    int i;
    int sum = 0;
    
    /* Initialize array */
    for (i = 0; i < 10; i++) {
        arr[i] = i * i;
    }
    
    /* Sum array elements */
    for (i = 0; i < 10; i++) {
        sum += arr[i];
    }
    
    return sum;
}

/* Test logical operators */
int test_logical() {
    int a = 5;
    int b = 10;
    int c = 0;
    
    if (a && b) {
        c++;
    }
    
    if (a || 0) {
        c++;
    }
    
    if (!0) {
        c++;
    }
    
    if ((a < b) && (b > 0)) {
        c++;
    }
    
    return c;  /* Should be 4 */
}

/* Test function with many parameters */
int many_params(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}

/* Test recursive function */
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

/* Main test program */
int main() {
    int result;
    int total = 0;
    int tests_passed = 0;
    
    puts("Enhanced Small-C Compiler Test Suite");
    puts("====================================\n");
    
    /* Test 1: Function parameters */
    printf("Test 1: Function parameters... ");
    result = test_params(10, 20, 30);
    if (result == 60) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 60, got %d)\n", result);
    }
    total++;
    
    /* Test 2: Character literals */
    printf("Test 2: Character literals... ");
    result = test_chars();
    if (result == 1) {
        puts("PASSED");
        tests_passed++;
    } else {
        puts("FAILED");
    }
    total++;
    
    /* Test 3: Compound assignments */
    printf("Test 3: Compound assignments... ");
    result = test_compound();
    if (result == 6) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 6, got %d)\n", result);
    }
    total++;
    
    /* Test 4: Nested loops */
    printf("Test 4: Nested loops... ");
    result = test_nested_loops();
    if (result == 5) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 5, got %d)\n", result);
    }
    total++;
    
    /* Test 5: String operations */
    printf("Test 5: String operations... ");
    result = test_strings();
    if (result == 5) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 5, got %d)\n", result);
    }
    total++;
    
    /* Test 6: Array operations */
    printf("Test 6: Array operations... ");
    result = test_arrays();
    if (result == 285) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 285, got %d)\n", result);
    }
    total++;
    
    /* Test 7: Logical operators */
    printf("Test 7: Logical operators... ");
    result = test_logical();
    if (result == 4) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 4, got %d)\n", result);
    }
    total++;
    
    /* Test 8: Many parameters */
    printf("Test 8: Many parameters... ");
    result = many_params(1, 2, 3, 4, 5, 6);
    if (result == 21) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 21, got %d)\n", result);
    }
    total++;
    
    /* Test 9: Recursion */
    printf("Test 9: Recursion (fibonacci)... ");
    result = fibonacci(10);
    if (result == 55) {
        puts("PASSED");
        tests_passed++;
    } else {
        printf("FAILED (expected 55, got %d)\n", result);
    }
    total++;
    
    /* Test 10: Comments */
    // This is a single-line comment
    /* This is a 
       multi-line comment */
    printf("Test 10: Comments... ");
    puts("PASSED");
    tests_passed++;
    total++;
    
    /* Summary */
    puts("\n====================================");
    printf("Tests passed: %d/%d\n", tests_passed, total);
    
    if (tests_passed == total) {
        puts("\nAll tests PASSED!");
        return 0;
    } else {
        puts("\nSome tests FAILED!");
        return 1;
    }
}
