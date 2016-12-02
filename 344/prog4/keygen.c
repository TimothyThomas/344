/* 
 * CS 344 Fall 2016
 * Program 4 -- keygen 
 * by Tim Thomas 
 *
 * Usage:   ./keygen num
 *
 * Note:    num should be an integer value
 *
 * Output:  sends to stdout num random characters consisting of letters and
 *          spaces and an extra newline appended at the end (so num + 1 chars total).
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char *argv[]) {

    // Parse command line arg and convert string to integer
    int KEY_LENGTH = strtol(argv[1], NULL, 10); 

    // seed random number generator
    srand(time(NULL));

    int i;
    char c;
    for (i = 0; i < KEY_LENGTH; i++) {

        c = rand() % 27;   // module 27 to get a value between 0 and 26

        if (c == 26) {    // make c a blank space
            c = 32; 
        }

        else {
            c += 65;     // make c an uppercase letter (ascii A=65)
        }

        printf("%c", c);
    }
    
    printf("\n");

    return 0;
}
