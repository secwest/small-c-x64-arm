/*
 * Runtime Library Test for Basic Small-C Compiler
 * This tests that scc.c properly generates calls to runtime functions
 */

int main() {
    char buffer[50];
    char name[30];
    int num;
    int result;
    int fd;
    
    /* Test puts - basic string output */
    puts("Testing Small-C Runtime Library");
    puts("===============================");
    puts("");
    
    /* Test printf - formatted output */
    printf("Testing printf with number: %d\n", 42);
    printf("Testing printf with hex: %x\n", 255);
    printf("Testing printf with char: %c\n", 65);  /* 'A' */
    printf("Testing printf with string: %s\n", "Hello!");
    puts("");
    
    /* Test interactive I/O */
    printf("Enter your name: ");
    gets(name);
    printf("Hello, %s!\n", name);
    
    printf("Enter a number: ");
    gets(buffer);
    num = atoi(buffer);
    printf("You entered: %d\n", num);
    printf("Times two: %d\n", num * 2);
    puts("");
    
    /* Test string functions */
    strcpy(buffer, "Test String");
    printf("strcpy result: %s\n", buffer);
    printf("strlen result: %d\n", strlen(buffer));
    
    if (strcmp("abc", "abc") == 0) {
        puts("strcmp test 1: PASS");
    } else {
        puts("strcmp test 1: FAIL");
    }
    
    if (strcmp("abc", "def") < 0) {
        puts("strcmp test 2: PASS");
    } else {
        puts("strcmp test 2: FAIL");
    }
    puts("");
    
    /* Test file operations */
    puts("Testing file operations...");
    fd = creat("smallc_test.txt");
    if (fd >= 0) {
        write(fd, "Small-C file test\n", 18);
        write(fd, "It works!\n", 10);
        close(fd);
        puts("  File created successfully");
        
        /* Read it back */
        fd = open("smallc_test.txt", 0);
        if (fd >= 0) {
            result = read(fd, buffer, 49);
            buffer[result] = 0;  /* null terminate */
            close(fd);
            puts("  File contents:");
            printf("%s", buffer);
        } else {
            puts("  Error opening file for reading");
        }
    } else {
        puts("  Error creating file");
    }
    puts("");
    
    /* Test memory functions */
    memset(buffer, 88, 5);  /* 'X' */
    buffer[5] = 0;
    printf("memset test: %s\n", buffer);
    
    strcpy(buffer, "Source");
    memcpy(buffer + 10, buffer, 7);
    printf("memcpy test: %s\n", buffer + 10);
    puts("");
    
    /* Test printn directly */
    printf("Testing printn: ");
    printn(12345);
    putchar(10);  /* newline */
    
    printf("Testing negative: ");
    printn(-999);
    putchar(10);
    
    puts("");
    puts("All tests completed!");
    
    return 0;
}
