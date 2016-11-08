#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 2048

int main() {

    int is_foreground = 1;   // flag for whether command is to be a FG or BG process
    int exitStatus = 0;

    while (1) {

        printf(": ");
        fflush(stdout);

        char input[MAX_INPUT_SIZE + 1];
        fgets(input, MAX_INPUT_SIZE, stdin); 

        // handle case where input is a blank line
        if (strcmp(input, "\n") == 0) {
            exitStatus = 0;
            continue;
        }

        // handle case where input begins with '#' 
        if (input[0] == '#') {
            exitStatus = 0;
            continue;
        }

        input[strcspn(input, "\n")] = 0;

        // file descriptors for input/output redirection
        int inputFD, outputFD;

        // original file descriptors for stdin/stdout
        int saved_stdout = dup(1);
        int saved_stdin = dup(0);

        // exit status of smallshell command

        char* cmd = strtok(input, " "); 

        //printf("Input is: %s\n", cmd);

        // Check for built-in commands
        if (strcmp(cmd, "exit") == 0) {

            // TODO: kill off all processes
            exitStatus = 0;
            exit(0);
        }

        else if (strcmp(cmd, "cd") == 0) {
            char* newpath = strtok(NULL, " ");
            if (chdir(newpath) != 0) {
                printf("Error changing directory to: %s\n", newpath);
                exitStatus = 1;
            }
            else {
                exitStatus = 0;
            }
        }

        else if (strcmp(cmd, "status") == 0) {

            // print status
            printf("%d\n", exitStatus);
            fflush(stdout);
            exitStatus = 0;
        }

        //else if (strcmp(cmd, "") == 0) {
        /*
        else if (cmd[0] == '\0') { 
            printf("Blank line entered\n");
            fflush(stdout);
            exitStatus = 0;
        }
        */

        // Not a built-in command.  Execute with fork() and exec()
        else {
            char* args[512];
            args[0] = cmd;

            // build the argv array
            int i = 1;
            int error = 0;
            while (1) {
                char* arg = strtok(NULL, " ");

                if (arg == NULL) {
                    args[i++] = arg;
                    break;
                }

                else if (strcmp(arg, "&") == 0) {
                    is_foreground = 0;
                }

                else if (strcmp(arg, ">") == 0) {  // redirect stdout 
                    char* outfile = strtok(NULL, " ");
                    outputFD = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (outputFD == -1) { perror("open()"); error = 1; break; }

                    // save current stdout so we can switch back after current command is processed
                    saved_stdout = dup(1); 

                    int result = dup2(outputFD, 1);
                    if (result == -1) { perror("dup2"); exit(2); }
                }

                else if (strcmp(arg, "<") == 0) {  // redirect stdin 
                    char* infile = strtok(NULL, " ");
                    inputFD = open(infile, O_RDONLY);
                    if (inputFD == -1) { perror("open()"); error = 1; break; }
                      
                    // save current stdin so we can switch back after current command is processed
                    saved_stdin = dup(0); 

                    int result = dup2(inputFD, 0);
                    if (result == -1) { perror("dup2"); exit(2); }
                }

                else {
                    args[i++] = arg;
                }
            } 

            // check if error in command line args/files
            if (error == 1) {
                exitStatus = 1;
                continue;
            }

            // spawn child process
            pid_t spawnPid = -5;
            int childExitStatus = -5;
            spawnPid = fork();
            switch(spawnPid) {
                case -1: {perror("Error spawning process.\n"); exit(1); break; }
                case 0: {
                    execvp(args[0], args);
                    //perror("No such file or directory.\n");
                    perror(args[0]);
                    exit(2); break;
                }
                default: {
                    pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0);
                    exitStatus = WEXITSTATUS(childExitStatus); 
                    break;
                }
            }

            // restore original stdin and stdout (to terminal)
            dup2(saved_stdout, 1);
            dup2(saved_stdin, 0);
        }
    }

    return 0;
}


/*
int execute_child(char* argc, char* argv[]) {
    int status = 0;
    // spawn child process
    pid_t spawnPid = -5;
    int childExitStatus = -5;
    spawnPid = fork();
    switch(spawnPid) {
        case -1: {perror("Error spawning process.\n"); exit(1); break; }
        case 0: {
            execvp(args[0], args);
            perror("Child process exec failure!\n");
            exit(2); break;
        }
        default: {
            pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0);
            exitStatus = WEXITSTATUS(childExitStatus); 
            break;
        }
    }



    return status;
}
*/
