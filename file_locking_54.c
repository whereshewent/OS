/* Solution for file locking (filesystem, 54) topic
as part of the course COMP.CS.410: Operating Systems and Concurrency, fall 26
Contributors: Duc (writer), Vilppu (tester)
Description: This program performs the functionality of cat command combined with an advisory lock 
during execution. The read and close file is similar to what was done in the mmap solution previously. 
When a new file is opened, the lock is configured and used for that file. Then we can do the same thing 
cat does by reading from the descriptor and writing to the standard output with (read, write system calls) 
at kernel level. Note that there are many error events that need to be handled during read and write 
execution for the output to be accurate and here only interrupt handler is resolved as of its simplicity. 
After the file has been printed to the standard output, the lock is release and cleared. 
Google Search AI Mode is used to find necessary functions, give examples of how they are used, 
and review the final code for bug detection. */


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
// for struct flock 
#include <fcntl.h>
#include <unistd.h>
// for EINTR 
#include <errno.h>

#define PAGE_SIZE 4096 
#define MESSAGE_SIZE 256 

int secure_cat(const char* file_name)
{
    int status = EXIT_SUCCESS; 
    int fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        const char *msg = "No such file or directory\n";
        write(STDERR_FILENO, msg, strlen(msg)); 
        return EXIT_FAILURE; 
    }

    // define and configure the lock 
    struct flock lock; 
    /* the original file isn't modified as its content will be written to standard output,
    so a read lock is enough*/
    lock.l_type = F_RDLCK;
    // where the lock becomes valid, SEEK_SET is for the beginning of file 
    lock.l_whence = SEEK_SET;
    // starting offset (bytes) of the block, relative to l_whence
    lock.l_start = 0; 
    // 0 means to the end of file :D
    lock.l_len = 0;

    // set the lock and check the operation result 
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("Failed to acquire lock :(");
        close(fd); 
        return EXIT_FAILURE;
    } 

    int msg_len; 
    char buffer[PAGE_SIZE], err_msg[MESSAGE_SIZE]; 
    ssize_t bytes_rd, bytes_wr, wr_count; 

    /* the reason to use these loops when read and write with system calls is to make sure there is no missing bytes when
    read and write and if there is, log the error code for debugging. it stems from the fact that these system call get 
    success returns even if the number of bytes to be read and write is lower than the requested amount.
    interrupt signal handler is just one edge case that is tackled here and there are many others that need 
    to be handled for the program to be secure. */
    while ((bytes_rd = read(fd, buffer, PAGE_SIZE)) != 0)
    {
        // handle read error cases
        if (bytes_rd == -1)
        {
            // if the process is disrupted by an interrupt handler, retry 
            if (errno == EINTR)  
                continue; 
            
            // otherwise write the error code to the standard error along with its description
            msg_len = snprintf(err_msg, MESSAGE_SIZE, "Read failed (Error %d): %s\n", errno, strerror(errno));
            write(STDERR_FILENO, err_msg, msg_len);
            status = EXIT_FAILURE;
            break;
        }

        // if read successfully, then copy and write the output to the standard output
        bytes_wr = 0;
        while (bytes_wr < bytes_rd) 
        {
            wr_count = write(STDOUT_FILENO, buffer + bytes_wr, bytes_rd - bytes_wr);
            // handle interrupt signal as for read execution 
            if (wr_count == -1)
            {
                if (errno == EINTR)
                    continue; 
                
                // error logging as done with read above
                msg_len = snprintf(err_msg, MESSAGE_SIZE, "Write failed (Error %d): %s\n", errno, strerror(errno));
                write(STDERR_FILENO, err_msg, msg_len);
                status = EXIT_FAILURE;
                break;
            }
            bytes_wr += wr_count; 
        }
        
        if (status == EXIT_FAILURE) 
            break;
    }
    
    // release and clear the lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    close(fd);
    return status;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "%s: Filename wasn't provided\n", argv[0]); 
        return EXIT_FAILURE;
    } 

    int final_status = EXIT_SUCCESS;     
    for (int i = 1; i < argc; ++i)
    {
        if (secure_cat(argv[i]) == EXIT_FAILURE)
            final_status = EXIT_FAILURE;
    }

    return final_status;
}