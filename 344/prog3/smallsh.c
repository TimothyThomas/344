#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT_SIZE 2048


int main() {

    int exitStatus = 0;
    int childPID = 0;
    int bgPIDs[10000];
    int num_procs = 0; 
    int fg_terminated = 0;  // flag indicating if last foreground process was terminated with signal
    
    // file descriptors for input/output redirection
    int inputFD, outputFD;

    // flags indicating if stdin/stdout have been redirected
    int stdin_redirected = 0;
    int stdout_redirected = 0;

    // original file descriptors for stdin/stdout
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);

    struct sigaction SIGINT_action_parent;
    SIGINT_action_parent.sa_handler = SIG_IGN; 
    sigfillset(&SIGINT_action_parent.sa_mask);   // block/delay all signals arriving while this mask in place
    sigaction(SIGINT, &SIGINT_action_parent, NULL);

    while (1) {

        int is_background = 0;   // flag for whether command is to be a FG or BG process

        // check if background processes have terminated
        int childExitMethod = -5;
        for (int i = 0; i < num_procs; i++) {
            childPID = waitpid(-1, &childExitMethod, WNOHANG);
            if (childPID != 0) {

                if (WIFEXITED(childExitMethod) != 0) {
                    printf("Background PID %d is done: exit value %d\n", childPID, WEXITSTATUS(childExitMethod));
                    fflush(stdout);
                }

                else if (WIFSIGNALED(childExitMethod) != 0) {
                    printf("Background PID %d is done: terminated by signal %d\n", 
                           childPID, WTERMSIG(childExitMethod));
                    fflush(stdout);
                }

                else {
                    printf("Background PID %d is done, but did not exit normally or with signal.\n", childPID);
                    fflush(stdout);
                }

                // shift all PIDs down in array
                for (int j = i+1; j < num_procs; j++) {
                    bgPIDs[j-1] = bgPIDs[j];
                }
                num_procs--;
            }
        }

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

        char* cmd = strtok(input, " "); 

        // Check for built-in commands
        if (strcmp(cmd, "exit") == 0) {

            // kill off all processes
            kill(0, SIGTERM);
            exitStatus = 0;
            exit(0);
        }

        else if (strcmp(cmd, "cd") == 0) {
            char* newpath = strtok(NULL, " ");
            if (newpath == NULL) {                // cd followed by nothing; go to $HOME
                newpath = getenv("HOME"); 
            }

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
            if (fg_terminated != 0) {
                printf("terminated by signal ");
            }
            else {
                printf("exit value ");
            }
            printf("%d\n", exitStatus);
            fflush(stdout);
            fg_terminated = 0;
            exitStatus = 0;
        }


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
                    is_background = 1;
                }

                else if (strcmp(arg, ">") == 0) {  // redirect stdout 
                    char* outfile = strtok(NULL, " ");
                    outputFD = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (outputFD == -1) { perror(outfile); error = 1; break; }

                    // save current stdout so we can switch back after current command is processed
                    saved_stdout = dup(1); 

                    int result = dup2(outputFD, 1);
                    if (result == -1) { perror("dup2"); exit(1); }
                    stdout_redirected = 1;
                }

                else if (strcmp(arg, "<") == 0) {  // redirect stdin 
                    char* infile = strtok(NULL, " ");
                    inputFD = open(infile, O_RDONLY);
                    if (inputFD == -1) { perror(infile); error = 1; break; }
                      
                    // save current stdin so we can switch back after current command is processed
                    saved_stdin = dup(0); 

                    int result = dup2(inputFD, 0);
                    if (result == -1) { perror("dup2"); exit(1); }
                    stdin_redirected = 1;
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
            int childExitMethod = -5;
            spawnPid = fork();
            switch(spawnPid) {
                case -1: {perror("Error spawning process.\n"); exit(1); break; }
                case 0: {  // child process
                    struct sigaction SIGINT_action_child;
                    SIGINT_action_child.sa_handler = SIG_DFL; 
                    sigfillset(&SIGINT_action_child.sa_mask);   // block/delay all signals arriving while this mask in place
                    sigaction(SIGINT, &SIGINT_action_child, NULL);

                    // if process is being sent to background, redirect stdin/stdout to
                    // /dev/null if they aren't already redirected
                    if (is_background != 0) {

                        if (stdin_redirected == 0) {  // stdin not redirected for this command
                            int dev_null_in = open("/dev/null", O_RDONLY);
                            dup2(dev_null_in, 0);
                            close(dev_null_in);
                            stdin_redirected = 1;
                        }

                        if (stdout_redirected == 0) {  // stdout not redirected for this command
                            int dev_null_out = open("/dev/null", O_WRONLY);
                            dup2(dev_null_out, 1);
                            close(dev_null_out);
                            stdout_redirected = 1;
                        }
                    }

                    if (execvp(args[0], args) < 0) {
                        perror(args[0]);
                        exit(1); 
                    }
                    break;
                }
                default: {  // parent process

                    if (is_background == 0) {
                        waitpid(spawnPid, &childExitMethod, 0);
                        exitStatus = WEXITSTATUS(childExitMethod); 

                        // check and notify if foreground process terminated by signal
                        if (WIFSIGNALED(childExitMethod) != 0) {
                            printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
                            fflush(stdout);
                            exitStatus = WTERMSIG(childExitMethod);
                            fg_terminated = 1;
                        }
                    }
                    else {
                        printf("Starting background process: %d\n", spawnPid);
                        fflush(stdout);
                        bgPIDs[num_procs] = spawnPid;
                        num_procs++;
                    }
                    break;
                }
            }

            // restore original stdin and stdout (to terminal)
            dup2(saved_stdout, 1);
            dup2(saved_stdin, 0);
            stdin_redirected = 0;
            stdout_redirected = 0;

        }
    }

    return 0;
}
