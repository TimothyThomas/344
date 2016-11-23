/* 
 * CS 344 Fall 2016
 * Program 4 -- keygen 
 * by Tim Thomas 
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>



int main(int argc, char *argv[]) {

    int KEY_LENGTH = strtol(argv[1], NULL, 10) + 1;  // add 1 to account for '\n' at end
    char key[KEY_LENGTH];
    memset(key, '\0', KEY_LENGTH);

    // seed random number generator
    srand(time(NULL));

    int i;
    char c;
    for (i = 0; i < KEY_LENGTH - 1; i++) {
        c = rand() % 27;
        if (c == 26) {    // make this a space
            c = 32;       // ascii code for space
        }
        else {
            c += 65;
        }
        key[i] = c;
    }
    
    key[KEY_LENGTH-1] = '\n';
    printf("%s\n", key);

    return 0;
}
