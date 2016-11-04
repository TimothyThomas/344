#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 2048

int main() {

    while (1) {

        char input[MAX_INPUT_SIZE + 1];

        printf(": ");

        fgets(input, MAX_INPUT_SIZE, stdin); 
        input[strcspn(input, "\n")] = 0;

        printf("User input: %s\n", input);

        // Check for built-in commands
        if (strcmp(input, "exit") == 0) {

            // kill off all processes
            //
            exit(0);
        }
    }

    return 0;
}
