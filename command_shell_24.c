/* Solution for command shell (simple) (fork, 24) topic
as part of the course COMP.CS.410: Operating Systems and Concurrency, fall 26
Author: Duc
Description: This program takes a program name from user, look it up from the PATH environment variable, 
and create a new process from the program using fork and exec system calls when the name is valid.
The validity of the program is checked by looping through each directory in the PATH environment, form the 
full path by combining the directory and the program name, and then check the execution permission of the file 
on that path. On success, the process will be spawned using fork and exec system calls. 
Failure is also recorded on every process of the script.
Google Search AI Mode is used to find necessary functions, give examples of how they are used, 
and review the final code for bug detection. */


#include <stdio.h> 
// for strcspn, strdup, strtok 
#include <string.h> 
// for PATH_MAX, NAME_MAX
#include <limits.h>
// for getenv
#include <stdlib.h>
// for access, execlp, pid_t
#include <unistd.h>
// for WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <sys/wait.h>

/* This function checks the validity of the program name provided by user 
in PATH environment variable.*/
int find_path(const char* cmd, char* full_path)
{
    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        // fprintf for application logic faults
        fprintf(stderr, "PATH environment variable can't be found :(\n");
        return 0;
    }

    // strdup = malloc + strcpy 
    char *path_copy = strdup(path_env); 
    if (path_copy == NULL)
    {
        // perror for system call failures
        perror("Heap allocation failed :(");
        return 0;
    }

    // use colon delimeter to separate different directories in the path environment 
    char *dir = strtok(path_copy, ":"); 

    while (dir != NULL)
    {
        // join the current directory with the program name to create the program's full path 
        snprintf(full_path, PATH_MAX, "%s/%s", dir, cmd); 

        // check the permission to execute of the program with that full path
        if (access(full_path, X_OK) == 0)
        {
            free(path_copy);
            return 1;
        }
        
        // NULL represents the continuation from the string that was left off
        dir = strtok(NULL, ":");
    }

    printf("Command '%s' is not in any PATH directory.\n", cmd); 
    // strdup allocates heap memory when replicating the path but doesn't deallocate it automatically
    free(path_copy); 

    return 0;
}

int main(void)
{
    char cmd [NAME_MAX]; 
    printf("What's the program name?\n"); 

    // the choice of fgets is to prevent buffer overflow 
    if (fgets(cmd, NAME_MAX, stdin) == NULL)
    {
        printf("Can't find the program name :(\n");
        return 0;
    }
    // replace the trailing newline char added by fgets with a null terminator
    // strcspn returns index of the first occurrence of the given char in a str 
    cmd[strcspn(cmd, "\n")] = '\0'; 

    char full_path [PATH_MAX];
    int found = find_path(cmd, full_path); 
    if (found)
    {
        // create a child process
        pid_t pid = fork(); 

        // pid > 0 represents the parent's id process, 0 for child process, and -1 for failure 
        if (pid < 0)
        {
            perror("Fork failed :("); 
            return EXIT_FAILURE; 
        }
        else if (pid == 0)
        {
            char *args[] = {cmd, NULL};
            // the process moves out of its parent house. 
            /* there is more than one form of exec and here the chosen variation contains v suffix 
            to utilize the full path.*/
            execv(full_path, args); 
            // these lines are executed only when the execv fails.
            perror("Exec failed"); 
            exit(EXIT_FAILURE);
        }
        else 
        {
            int status;     
            // wait until the child finishes its process
            pid_t child_pid = waitpid(pid, &status, 0); 

            /* check the child's exit status with predefined macros 
            WIFEXITED and WIFSIGNALED are boolean values that examine the child process's end status. 
            WEXITSTATUS and WTERMSIG represent the specific values that tell how child process ended.
            */
            if (WIFEXITED(status))
                printf("Child %d exited normally with code: %d\n", child_pid, WEXITSTATUS(status));
            else if (WIFSIGNALED(status))
                printf("Child %d crashed with code: %d\n", child_pid, WTERMSIG(status));
        }
    }

    return EXIT_SUCCESS;
}