/* Minimal test to verify basic Small-C functionality */

int main() {
    /* Test 1: Basic function call */
    puts("Test 1: Basic puts");
    
    /* Test 2: Variable and arithmetic */
    int x;
    x = 10;
    x = x + 32;
    
    /* Test 3: Printf with number */
    printf("Test 2: x = %d\n", x);
    
    /* Test 4: Character output */
    putchar(65);  /* 'A' */
    putchar(10);  /* newline */
    
    /* Test 5: String variable */
    printf("Test 3: %s\n", "String test");
    
    return 0;
}
