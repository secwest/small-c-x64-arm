/*
 * Cross-Platform Small-C Example
 * This program works identically on Linux and Windows
 * Demonstrates the platform-independent nature of Small-C
 */

/* File I/O test - works on both platforms */
int test_files() {
    int fd;
    char buffer[100];
    int n;
    
    puts("Testing file operations...");
    
    /* Create a file */
    fd = creat("test_file.txt");
    if (fd < 0) {
        puts("  Error: Could not create file");
        return -1;
    }
    
    /* Write to file */
    write(fd, "Hello from Small-C!\n", 20);
    write(fd, "This works on Linux and Windows!\n", 34);
    close(fd);
    puts("  Created and wrote to test_file.txt");
    
    /* Read it back */
    fd = open("test_file.txt", 0);
    if (fd < 0) {
        puts("  Error: Could not open file");
        return -1;
    }
    
    n = read(fd, buffer, 99);
    buffer[n] = '\0';
    close(fd);
    
    puts("  File contents:");
    puts(buffer);
    
    return 0;
}

/* Platform detection based on behavior */
int detect_platform() {
    /* This is a bit of a hack, but it demonstrates that
       the same code runs on both platforms */
    puts("Platform Detection:");
    puts("  Small-C runs identically on Linux and Windows");
    puts("  The only difference is in the system call layer");
    puts("");
    return 0;
}

/* Interactive menu system */
int menu() {
    char choice[10];
    int running = 1;
    
    while (running) {
        puts("\nCross-Platform Small-C Demo");
        puts("===========================");
        puts("1. Test console I/O");
        puts("2. Test file operations");
        puts("3. Test math operations");
        puts("4. Platform information");
        puts("5. Exit");
        printf("\nChoice: ");
        
        gets(choice);
        
        if (choice[0] == '1') {
            char name[50];
            puts("\nConsole I/O Test");
            printf("Enter your name: ");
            gets(name);
            printf("Hello, %s!\n", name);
            
        } else if (choice[0] == '2') {
            puts("");
            test_files();
            
        } else if (choice[0] == '3') {
            int a, b;
            char num[20];
            
            puts("\nMath Operations Test");
            printf("Enter first number: ");
            gets(num);
            a = atoi(num);
            
            printf("Enter second number: ");
            gets(num);
            b = atoi(num);
            
            printf("\nResults:\n");
            printf("  %d + %d = %d\n", a, b, a + b);
            printf("  %d - %d = %d\n", a, b, a - b);
            printf("  %d * %d = %d\n", a, b, a * b);
            if (b != 0) {
                printf("  %d / %d = %d\n", a, b, a / b);
                printf("  %d %% %d = %d\n", a, b, a % b);
            }
            
        } else if (choice[0] == '4') {
            puts("");
            detect_platform();
            puts("Runtime Library Functions:");
            puts("  - Same runtime.c on all platforms");
            puts("  - Platform-specific syscall_*.s files");
            puts("  - Identical behavior across systems");
            
        } else if (choice[0] == '5') {
            running = 0;
        }
    }
    
    return 0;
}

/* Main program */
int main() {
    puts("====================================");
    puts("   Small-C Cross-Platform Demo");
    puts("====================================");
    puts("");
    puts("This program demonstrates that Small-C");
    puts("works identically on Linux and Windows!");
    puts("");
    
    /* Run the menu */
    menu();
    
    puts("\nGoodbye from Small-C!");
    return 0;
}

/*
 * Building this program:
 * 
 * Linux:
 *   ./scc crossplatform.c > crossplatform.s
 *   as crossplatform.s -o crossplatform.o
 *   ld -static syscall_linux_x64.o runtime.o crossplatform.o -o crossplatform
 *   ./crossplatform
 * 
 * Windows (cross-compile from Linux):
 *   ./scc crossplatform.c > crossplatform.s
 *   x86_64-w64-mingw32-as crossplatform.s -o crossplatform.o
 *   x86_64-w64-mingw32-ld syscall_win_x64.o runtime.o crossplatform.o -o crossplatform.exe
 * 
 * The same source code works on both platforms!
 */
