/*
 * Error Test Cases for Enhanced Small-C Compiler
 * Each section tests different error conditions
 * Uncomment one section at a time to test error handling
 */

/* Test 1: Unterminated comment 
int test1() {
    /* This comment is not closed
    return 0;
}
*/

/* Test 2: Unterminated string */
/*
int test2() {
    char *s = "This string is not closed;
    return 0;
}
*/

/* Test 3: Array size too large */
/*
int test3() {
    int huge_array[100000];  // Should trigger "Array size too large" error
    return 0;
}
*/

/* Test 4: Negative array size */
/*
int test4() {
    int bad_array[-10];  // Should trigger "Array size must be positive" error
    return 0;
}
*/

/* Test 5: Too many parameters */
/*
int too_many(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
    // Should trigger "Too many parameters" error (MAXARGS is 8)
    return 0;
}
*/

/* Test 6: Duplicate symbol */
/*
int test6() {
    int x;
    int x;  // Should trigger "Duplicate symbol definition" error
    return 0;
}
*/

/* Test 7: Integer overflow */
/*
int test7() {
    int huge = 99999999999999999999;  // Should trigger "Integer constant too large" error
    return 0;
}
*/

/* Test 8: Unknown escape sequence */
/*
int test8() {
    char *s = "Bad escape: \q";  // Should trigger "Unknown escape sequence" warning
    return 0;
}
*/

/* Test 9: Too many nested loops */
/*
int test9() {
    while (1) {
        while (1) {
            while (1) {
                // ... nest 20+ while loops to exceed MAXWHILE
                // Should trigger "Too many nested loops" error
            }
        }
    }
    return 0;
}
*/

/* Test 10: Missing main function */
/*
int not_main() {
    return 0;
}
// Should trigger "main function not defined" error
*/

/* Test 11: String/identifier too long */
/*
int test11() {
    char *very_long_identifier_name_that_exceeds_the_namesize_limit_of_32_characters = "test";
    // Should trigger "Symbol name truncated" warning
    return 0;
}
*/

/* Test 12: Function with too many arguments in call */
/*
int simple(int a) {
    return a;
}

int test12() {
    return simple(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);  // Should trigger "Too many function arguments" error
    return 0;
}
*/

/* Valid main for testing - comment out when testing missing main */
int main() {
    puts("Error handling test - uncomment test cases above");
    return 0;
}
