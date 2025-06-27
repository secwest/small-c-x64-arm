/*
 * Test program for Small-C compiler and runtime
 * Tests all major features of the runtime library
 */

int main() {
    char buf[100];
    char name[50];
    int fd;
    int n;
    int i;
    
    /* Test basic I/O */
    puts("Small-C Runtime Test Program");
    puts("===========================");
    puts("");
    
    /* Test printf */
    printf("Testing printf:\n");
    printf("  Integer: %d\n", 42);
    printf("  Negative: %d\n", -17);
    printf("  Hex: %x\n", 255);
    printf("  Character: %c\n", 'A');
    printf("  String: %s\n", "Hello, World!");
    printf("  Percent: %%\n");
    puts("");
    
    /* Test string functions */
    puts("Testing string functions:");
    strcpy(buf, "Test string");
    printf("  strcpy result: %s\n", buf);
    printf("  strlen of '%s': %d\n", buf, strlen(buf));
    
    if (strcmp("hello", "hello") == 0) {
        puts("  strcmp: 'hello' == 'hello' (correct)");
    }
    
    if (strcmp("abc", "xyz") < 0) {
        puts("  strcmp: 'abc' < 'xyz' (correct)");
    }
    puts("");
    
    /* Test interactive input */
    printf("Enter your name: ");
    gets(name);
    printf("Hello, %s!\n", name);
    puts("");
    
    /* Test file operations */
    puts("Testing file operations:");
    
    /* Create and write to a file */
    fd = creat("test.txt");
    if (fd < 0) {
        puts("  Error: Could not create test.txt");
    } else {
        fputs("This is a test file.\n", fd);
        fputs("It has multiple lines.\n", fd);
        fputs("Created by Small-C!\n", fd);
        close(fd);
        puts("  Created and wrote to test.txt");
    }
    
    /* Read the file back */
    fd = open("test.txt", 0);  /* O_RDONLY */
    if (fd < 0) {
        puts("  Error: Could not open test.txt");
    } else {
        puts("  Contents of test.txt:");
        while ((n = read(fd, buf, 99)) > 0) {
            buf[n] = '\0';
            printf("%s", buf);
        }
        close(fd);
    }
    puts("");
    
    /* Test memory functions */
    puts("Testing memory functions:");
    memset(buf, 'X', 10);
    buf[10] = '\0';
    printf("  memset with 'X': %s\n", buf);
    
    strcpy(buf, "Source");
    memcpy(buf + 10, buf, 7);
    printf("  memcpy result: %s\n", buf + 10);
    puts("");
    
    /* Test utility functions */
    puts("Testing utility functions:");
    printf("  abs(-42) = %d\n", abs(-42));
    printf("  min(10, 20) = %d\n", min(10, 20));
    printf("  max(10, 20) = %d\n", max(10, 20));
    printf("  atoi('123') = %d\n", atoi("123"));
    printf("  atoi('-456') = %d\n", atoi("-456"));
    puts("");
    
    /* Test loops and arrays */
    puts("Fibonacci sequence (first 10 numbers):");
    n = 0;
    i = 1;
    printf("  %d %d ", n, i);
    
    for (n = 0; n < 8; n++) {
        int temp;
        temp = i;
        i = i + n;
        n = temp;
        printf("%d ", i);
    }
    puts("\n");
    
    puts("Test completed successfully!");
    
    return 0;
}
