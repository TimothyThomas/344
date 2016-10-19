#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


int main() {

    const int NUM_ROOMS = 7;
    char room_dir[512];
    sprintf(room_dir, "trthomas.rooms.%i/", getpid());
    char* room_names[10];
    room_names[0] = "Mercury";
    room_names[1] = "Venus";
    room_names[2] = "Earth";
    room_names[3] = "Mars";
    room_names[4] = "Jupiter";
    room_names[5] = "Saturn";
    room_names[6] = "Uranus";
    room_names[7] = "Neptune";
    room_names[8] = "Pluto";
    room_names[9] = "Sol";

    struct room {
        char* name;
        int cnxns[6];
        int num_cnxns;
    };

    struct room rooms[NUM_ROOMS]; 

    srand(time(NULL));

    // initialize set of rooms
    for (int i = 0; i < NUM_ROOMS; i++) {
        rooms[i].name = "";
        rooms[i].num_cnxns = 0;
        for (int j = 0; j < 6; j++) {
            rooms[i].cnxns[j] = -1;
        }
    }

    // now randomly assign names to set of rooms
    int i = 0;
    while (i < NUM_ROOMS) {
        int r = rand() % 10;

        int unique = 1;
        for (int j = 0; j < i; j++) {
            if (room_names[r] == rooms[j].name) {
                unique = 0; 
                break;
            }
        }

        if (unique == 1) {
            rooms[i].name = room_names[r];
            i++;
        }
    }

    // create room 
    for (int i = 0; i < NUM_ROOMS; i++) {

        // generate 3 to 6 connections for each room
        int n = (rand() % 4) + 3;
        printf("Want %i connections for room %s. Currently have %i\n", n, rooms[i].name, rooms[i].num_cnxns);

        // add random connections
        while (rooms[i].num_cnxns < n) { 

            for (int j = 0; j < NUM_ROOMS; j++) {

                if (j == i) continue;  // room can't connect to itself

                // roll the dice to determine if room j should be connected
                int c = rand() % 2;
                if (c == 1) {   // add cnxn

                    // check if connection is unique
                    int unique = 1;
                    for (int k = 0; k < 6; k++) {
                        if (rooms[i].cnxns[k] == j) {
                            unique = 0;
                        }
                    }

                    // if unique, add connection
                    if (unique == 1) {
                        rooms[i].cnxns[rooms[i].num_cnxns] = j;
                        rooms[i].num_cnxns++;

                        // add connection to other room as well
                        rooms[j].cnxns[rooms[j].num_cnxns] = i;
                        rooms[j].num_cnxns++;
                    }
                }
            }
        }
    }

    // Write room files 
    for (int i = 0; i < NUM_ROOMS; i++) {

        char filename[512] = "";
        strcat(filename, room_dir);
        strcat(filename, rooms[i].name);
        mkdir(room_dir, S_IRWXU | S_IRWXG | S_IRWXO);
        FILE *file;
        file = fopen(filename, "w");
        fprintf(file, "ROOM NAME: %s\n", rooms[i].name);

        // Write connections
        for (int j = 0; j < rooms[i].num_cnxns; j++) {  // loop thru max possible connections (6)
            fprintf(file, "CONNECTION %i: %s\n", j+1, rooms[rooms[i].cnxns[j]].name);
        }
            
        // room type
        if (file == NULL) {
            printf("Something went wrong, file %s not created.\n", filename);
        }

        if (i == 0) {
            fprintf(file, "ROOM TYPE: START_ROOM\n");
        } 
        else if (i == NUM_ROOMS - 1) {
            fprintf(file, "ROOM TYPE: END_ROOM\n");
        } 
        else {
            fprintf(file, "ROOM TYPE: MID_ROOM\n");
        } 
        fclose(file);
    }
    return 0;
}
