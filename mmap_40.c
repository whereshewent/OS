/* Solution for Mmap (virtual memory, 40) topic
as part of the course COMP.CS.410: Operating Systems and Concurrency, fall 26
Author: Duc
Description: This program performs the functionality of wordcount (wc) command
using mmap() system call. It is done by utilizing system calls 
to open and close files and then use mmap to allocate heap memory of the file's size on RAM.
The content of the file can be then manipulated as a string. 
The output with the flags is adjusted to mimic with that of the original function on macOS. 
Failure is also recorded in every process.
Google Search AI Mode is used to find necessary functions, give examples of how they are used, 
and review the final code for bug detection. */


#include <stdio.h> 
#include <stdlib.h>
// for open() 
#include <unistd.h> 
// for open()'s parameter
#include <fcntl.h>
// for fstat()
#include <sys/stat.h>
// for mmap(), munmap()
#include <sys/mman.h>
// for isspace()
#include <ctype.h>

// print the byte count 
int flag_c = 0; 
// print the line count
int flag_l = 0; 
// print the character count 
int flag_m = 0; 
// print the word count
int flag_w = 0;
// print all (no flags)
int show_all = 0; 

void process_file(const char* filename)
{
    long bytes, lines = 0, chars = 0, words = 0;
    // Open file in Read-only mode
    int fd = open(filename, O_RDONLY);
    // If no flag is mentioned then show all metrics
    show_all = (!flag_c) & (!flag_l) & (!flag_w);

    if (fd == -1)
    {
        // print the file name next to the error in the format str: error
        perror(filename); 
        return;
    }
    // buffer to store data of the file 
    struct stat sb; 
    if (fstat(fd, &sb) == -1)
    {
        perror("Can't get file size"); 
        close(fd);
        return;
    }

    // file size in bytes
    bytes = (long)sb.st_size; 
    if (bytes == 0)
    {
        if (show_all || flag_l) printf("%8ld", lines);  
        if (show_all || flag_w) printf("%8ld", words); 
        if (show_all || flag_c) printf("%8ld", bytes); 
        // Char count is an optional flag
        if (flag_m) printf("%8ld ", chars); 

        close(fd);
        printf(" %s\n", filename); 
        return; 
    }
    /* map current file into address spaces in RAM
    PROT_READ means pages can be read
    MAP_PRIVATE maps to a copy so that any changes won't affect the original file */
    char* file_in_mem = mmap(NULL, bytes, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_in_mem == MAP_FAILED)
    {
        perror("Can't map file to memory");
        close(fd);
        return;
    }

    char current_byte; 
    int in_word = 0;
    for (long i = 0; i < bytes; ++i)
    {
        current_byte = file_in_mem[i]; 
        // Avoid counting continuation byte in a UTF-8 character 
        if ((current_byte & 0xC0) != 0x80)
            ++chars; 

        // The number of lines is equal to the number of new line characters
        if (current_byte == '\n')
            ++lines;
        
        /* If the current character is ' ', '\t', '\n', ... stop the current word count. 
        otherwise counting a word only at its first character. */
        if (isspace(current_byte))
            in_word = 0;
        else if (!in_word)
        {
            in_word = 1;
            ++words;
        }
    }

    if (show_all || flag_l) printf("%8ld", lines);  
    if (show_all || flag_w) printf("%8ld", words); 
    if (show_all || flag_c) printf("%8ld", bytes); 

    // Char count is an optional flag
    if (flag_m) printf("%8ld ", chars); 
    printf(" %s\n", filename); 
    
    // Unmap the file and release the address spaces in RAM 
    munmap(file_in_mem, bytes); 
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <file1> [file2] [file3] ...\n", argv[0]); 
        return EXIT_FAILURE;
    }

    int file_index_start = 1;
    char mode; 
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            mode = argv[i][1];
            if (mode == 'c')
                flag_c = 1;
            else if (mode == 'l')
                flag_l = 1;
            else if (mode == 'm')
                flag_m = 1;
            else if (mode == 'w')
                flag_w = 1;
            else 
            {
                printf("Unkown flag: %s\n", argv[i]); 
                return EXIT_FAILURE;
            }
        }
        else 
        {   
            file_index_start = i;
            break;
        }
    }

    // If the command only contains flags without filenames 
    if (file_index_start >= argc)
    {
        printf("Error: Missing filename parameters.\n");
        return EXIT_FAILURE;
    }

    for (int i = file_index_start; i < argc; ++i)
        process_file(argv[i]);

    return EXIT_SUCCESS;
}