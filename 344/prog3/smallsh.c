#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 2048

int main() {

    while (1) {

        printf(": ");

        char input[MAX_INPUT_SIZE + 1];
        fgets(input, MAX_INPUT_SIZE, stdin); 
        input[strcspn(input, "\n")] = 0;

        char* cmd = strtok(input, " "); 

        // Check for built-in commands
        if (strcmp(cmd, "exit") == 0) {

            // TODO: kill off all processes
            exit(0);
        }

        else if (strcmp(cmd, "cd") == 0) {
            char* newpath = strtok(NULL, " ");
            if (chdir(newpath) != 0) {
                printf("Error changing directory to: %s\n", newpath);
            }
        }

        else if (strcmp(cmd, "status") == 0) {

            // print status
            printf("print status\n");
        }

        // Not a built-in command.  Execute with fork() and exec()
        else {
            char* args[512];
            char* arg;
            args[0] = cmd;

            // build the argv array
            int i = 1;
            do {
                args[i++] =  strtok(NULL, " ");
            } while (args[i] != NULL);

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
                    break;
                }
            }
        }
    }

    return 0;
}
