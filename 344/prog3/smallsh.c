/*
 * CS 344, Fall 2016
 * Program 3 - smallsh
 * Tim Thomas
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT_SIZE 2048


/* This function checks if any background processes have completed.  It receives
 * a single pointer to an integer which holds the count of currently running
 * background processes.  If none have exited, immediately returns.  
 */
void check_background_procs(int* child_proc_count) {

    int childExitMethod = -5;
    while (*child_proc_count > 0) {
        int childPID = waitpid(-1, &childExitMethod, WNOHANG);
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
            (*child_proc_count)--;
        }
        else {
            break;
        }
    }
}


int main() {

    int exitStatus = 0;      // holds exit status of last completed foreground command
    int fg_terminated = 0;  // flag indicating if last foreground process was terminated with signal
    int num_bg_procs = 0;   
    int* ptr_num_bg_procs = &num_bg_procs;
    
    // file descriptors for input/output redirection
    int inputFD, outputFD;

    // flags indicating if stdin/stdout have been redirected
    int stdin_redirected = 0;
    int stdout_redirected = 0;

    // original file descriptors for stdin/stdout
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);

    // Define signal handler for parent process (i.e. the shell itself)
    struct sigaction SIGINT_action_parent;
    SIGINT_action_parent.sa_handler = SIG_IGN; 
    sigfillset(&SIGINT_action_parent.sa_mask);   // block/delay all signals arriving while this mask in place
    sigaction(SIGINT, &SIGINT_action_parent, NULL);

    while (1) {

        int is_background = 0;   // flag for whether command is to be a FG or BG process

        // check if any background processes have terminated
        check_background_procs(ptr_num_bg_procs);

        // Display command prompt
        printf(": ");
        fflush(stdout);

        // Read input command from stdin
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

        // strip newline at the end of the input string
        input[strcspn(input, "\n")] = 0;

        // begin tokenizing input.  After next line cmd contains first word of command.
        char* cmd = strtok(input, " "); 

        // handle built-in exit command
        if (strcmp(cmd, "exit") == 0) {

            // kill off all processes and exit
            kill(0, SIGTERM);
            exit(0);
        }

        // handle built-ins cd command
        else if (strcmp(cmd, "cd") == 0) {

            // next token contains path
            char* target = strtok(NULL, " ");

            // if no path given, target path is HOME directory
            if (target == NULL) {               
                target = getenv("HOME"); 
            }

            // change directory to target
            if (chdir(target) != 0) {
                printf("Error changing directory to: %s\n", target);
                exitStatus = 1;
            }
            else {
                exitStatus = 0;
            }
        }

        // handle built-ins status command
        // exitStatus contains exit value of previously executed command
        else if (strcmp(cmd, "status") == 0) {

            if (fg_terminated != 0) {             // terminated by signal 
                printf("terminated by signal ");
            }

            else {                                // terminated normally 
                printf("exit value ");
            }
            printf("%d\n", exitStatus);
            fflush(stdout);

            // reset flags and exit status
            fg_terminated = 0;
            exitStatus = 0;
        }


        // Handle all other cases (not a built-in, blank line or comment).  Use fork() and exec().
        else {

            char* args[512];   // array of strings for all arguments (similar to argv)
            args[0] = cmd;     // first word of command string
            exitStatus = 0;

            // build the argv array
            int i = 1;
            while (1) {
                char* arg = strtok(NULL, " ");

                // command is a single word
                if (arg == NULL) {
                    args[i++] = arg;
                    break;
                }

                // command should be executed in background
                else if (strcmp(arg, "&") == 0) {
                    is_background = 1;
                }

                // redirect stdout
                else if (strcmp(arg, ">") == 0) {  
                    char* outfile = strtok(NULL, " ");
                    outputFD = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (outputFD == -1) { perror(outfile); exitStatus = 1; break; }

                    // save current stdout so we can switch back after current command is processed
                    saved_stdout = dup(1); 

                    // redirect and set flag
                    int result = dup2(outputFD, 1);
                    if (result == -1) { perror("dup2"); exit(1); }
                    stdout_redirected = 1;
                }

                // redirect stdin
                else if (strcmp(arg, "<") == 0) {  
                    char* infile = strtok(NULL, " ");
                    inputFD = open(infile, O_RDONLY);
                    if (inputFD == -1) { perror(infile); exitStatus = 1; break; }
                      
                    // save current stdin so we can switch back after current command is processed
                    saved_stdin = dup(0); 

                    // redirect and set flag
                    int result = dup2(inputFD, 0);
                    if (result == -1) { perror("dup2"); exit(1); }
                    stdin_redirected = 1;
                }

                // if not NULL, &, < or >, just add to list of args
                else {
                    args[i++] = arg;
                }
            } 

            // check if error in command line args/files
            if (exitStatus == 1) {
                continue;
            }

            // spawn child process
            pid_t spawnPid = -5;
            int childExitMethod = -5;
            spawnPid = fork();

            switch(spawnPid) {

                case -1: {perror("Error spawning process.\n"); exit(1); break; }

                case 0: {  // child process

                    // create signal handler for child process (use default action)
                    struct sigaction SIGINT_action_child;
                    SIGINT_action_child.sa_handler = SIG_DFL; 
                    sigfillset(&SIGINT_action_child.sa_mask);   
                    sigaction(SIGINT, &SIGINT_action_child, NULL);

                    // if process is being sent to background, redirect stdin/stdout to
                    // /dev/null if they aren't already redirected
                    if (is_background != 0) {

                        // check if stdin redirected; if not, redirect to /dev/null
                        if (stdin_redirected == 0) {  
                            int dev_null_in = open("/dev/null", O_RDONLY);
                            dup2(dev_null_in, 0);
                            stdin_redirected = 1;
                        }

                        // check if stdout redirected; if not, redirect to /dev/null
                        if (stdout_redirected == 0) {  
                            int dev_null_out = open("/dev/null", O_WRONLY);
                            dup2(dev_null_out, 1);
                            stdout_redirected = 1;
                        }
                    }

                    // Execute command
                    if (execvp(args[0], args) < 0) {
                        perror(args[0]);
                        exit(1); 
                    }
                    break;
                }

                default: {  // parent process

                    // '&' was supplied, so wait for child process to finish
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

                    // '&' was not supplied, so just report PID, increment
                    // background process counter and continue
                    else {
                        printf("background pid is %d\n", spawnPid);
                        fflush(stdout);
                        num_bg_procs++;
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
