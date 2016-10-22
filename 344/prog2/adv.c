#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


const int NUM_ROOMS = 7;

const char* ROOM_NAMES[10] = {"Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus",
                              "Neptune", "Pluto", "Sol"};

struct Room {
    char* name;
    int cnxns[6];
    int num_cnxns;
};


int create_rooms(struct Room[NUM_ROOMS]);
int find_start_room();

int main() {
    struct Room rooms[NUM_ROOMS]; 
    printf("creating rooms...");
    create_rooms(rooms);

    // read in data from room files
    int start_idx = find_start_room();
    printf("Start room index is %i\n", start_idx);

    return 0;
}

int find_start_room() {

    char ROOM_DIR[1024];
    sprintf(ROOM_DIR, "trthomas.rooms.%d/", getpid());

    int start_room_idx = -1;

    // find starting room
    for (int i = 0; i < 10; i++) {

        char filename[512]; // = "";
        sprintf(filename, "%s%s", ROOM_DIR, ROOM_NAMES[i]);
        FILE *fd = fopen(filename, "r");
        char *string = "START_ROOM\0";

        if (fd != NULL) {   // file exists
            char buf[11];

            fseek(fd, -11, SEEK_END);
            ssize_t len = fread(buf, sizeof(char), 10, fd);
            buf[11] = '\0';
            printf("buf = %s\n", buf);
            printf("comparing %s (len=%d) and %s (len=%d)\n", string, strlen(string), buf, strlen(buf));

            if (strcmp(buf, string) == 0) {
                printf("Start room found!  It is: %s\n", ROOM_NAMES[i]);
                start_room_idx = i;
                fclose(fd);
                break;
            }
            fclose(fd);
        }
    }
    return start_room_idx; 
}


int create_rooms(struct Room rooms[NUM_ROOMS]) {

    char ROOM_DIR[1024];
    sprintf(ROOM_DIR, "trthomas.rooms.%d/", getpid());

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
            if (ROOM_NAMES[r] == rooms[j].name) {
                unique = 0; 
                break;
            }
        }

        if (unique == 1) {
            rooms[i].name = ROOM_NAMES[r];
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

        char filename[512];
        sprintf(filename, "%s%s", ROOM_DIR, rooms[i].name);
        mkdir(ROOM_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
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



