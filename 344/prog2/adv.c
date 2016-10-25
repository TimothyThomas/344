#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


const int NUM_ROOMS = 7;
char* ROOM_NAMES[10] = {"Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus",
                              "Neptune", "Pluto", "Sol"};

struct Room {
    char* name;
    int cnxns[6];
    int num_cnxns;
    char type;
};


int create_room_files(struct Room[NUM_ROOMS]);
int read_room_data(struct Room[NUM_ROOMS]);
int find_start_room(struct Room[NUM_ROOMS]);
void prompt_user(struct Room);

int main() {

    setbuf(stdout, NULL);  // disable buffering on stdout
    struct Room rooms[NUM_ROOMS]; 
    printf("creating rooms...");
    create_room_files(rooms);

    int i;

    // re-initialize rooms struct
    for (i = 0; i < NUM_ROOMS; i++) {
        rooms[i].name = "";
        rooms[i].num_cnxns = 0;
    }
    
    read_room_data(rooms);
    int idx_cur = find_start_room(rooms);
    
    int num_steps = 0;
    int steps[1024];
    int game_over = 0;
    struct Room cur_room = rooms[idx_cur];

    // Game loop
    while (game_over == 0) {

        prompt_user(cur_room);
        
        // get user input
        char input[20];
        fgets(input, 20, stdin); 
        input[strcspn(input, "\n")] = 0;

        // check if input matches current connections
        int found = 0;
        for (i = 0; i < cur_room.num_cnxns; i++) {
            if (strcmp(input, ROOM_NAMES[cur_room.cnxns[i]]) == 0) { 
                printf("\n");
                found = 1;
                steps[num_steps++] = cur_room.cnxns[i];
                //num_steps++;
                break;
            }
        }
        if (found == 0) {
            printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }
        else {  
            // find index of next room
            for (i = 0; i < NUM_ROOMS; i++) {
                if (strcmp(input, rooms[i].name) == 0) {
                    cur_room = rooms[i];
                    break;
                }
            }

            // check if next room is END ROOM
            if (cur_room.type == 'E') {
                printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
                printf("YOU TOOK %i STEPS. YOUR PATH TO VICTORY WAS:\n", num_steps);

                // Print path taken
                for (i = 0; i < num_steps; i++) {
                    printf("%s\n", ROOM_NAMES[steps[i]]);
                }

                game_over = 1;
            }
        }
    }

    return 0;
}


void prompt_user(struct Room cur_room) {
    printf("CURRENT LOCATION: %s\n", cur_room.name);
    printf("POSSIBLE CONNECTIONS: ");
    int i;
    for (i = 0; i < cur_room.num_cnxns; i++) {
        if (i < cur_room.num_cnxns - 1) {
            printf("%s, ", ROOM_NAMES[cur_room.cnxns[i]]);
        }
        else {
            printf("%s.\n", ROOM_NAMES[cur_room.cnxns[i]]);
        }
    }
    printf("WHERE TO? >");
}


int read_room_data(struct Room rooms[NUM_ROOMS]) {
    int r = 0;
    int c, i;
    char ROOM_DIR[1024];
    sprintf(ROOM_DIR, "trthomas.rooms.%d/", getpid());

    for (i = 0; i < 10; i++) {

        char filename[512];
        sprintf(filename, "%s%s", ROOM_DIR, ROOM_NAMES[i]);
        FILE *fp = fopen(filename, "r");

        if (NULL != fp) {
            rooms[r].name = ROOM_NAMES[i];
            rooms[r].num_cnxns = 0;

            // read first line
            do {
                c = fgetc(fp);
                if (c == '\n') {
                    break;
                }
            } while(1);

            // read in connections
            int still_more_connections = 1;
            char name[50];
            do {
                c = fgetc(fp);

                int n = 0;
                if (c == ':') {   
                    c = fgetc(fp); // read blank
                    do {
                        c = fgetc(fp);  
                        if (c == '\n') {    
                            c = fgetc(fp);
                            if (c != 'C') {  
                                still_more_connections = 0;
                            }
                            break;
                        }
                        name[n++] = c; 
                    } while(1);
                    name[n++] = '\0';

                    // add connection
                    int k;
                    for (k = 0; k < 10; k++) {
                        if (strcmp(name, ROOM_NAMES[k]) == 0) {
                            rooms[r].cnxns[rooms[r].num_cnxns] = k;
                            rooms[r].num_cnxns++;
                        }
                    }
                }
            } while(still_more_connections);
            
            // read room type
            do {
                c = fgetc(fp);
                if (c == ':')
                    break;
            } while(1);

            do { 
                if (c == ':') {  
                    c = fgetc(fp);  // read blank
                    c = fgetc(fp);
                    rooms[r].type = c;  // just hold 'S, M or E' for type
                    break;
                }
            } while(1);

            fclose(fp);
            r++;
        }
    }
/*
    printf("Room data read from files: \n");
    for (int i = 0; i < NUM_ROOMS; i++) {
        printf("\nName: %s", rooms[i].name);
        printf("\nConnections: ");
        for (int j = 0; j < rooms[i].num_cnxns; j++) {
            printf("%s, ", ROOM_NAMES[rooms[i].cnxns[j]]);
        }
        printf("\nType: %c\n", rooms[i].type);
    }
    */

    return 0;
}

int find_start_room(struct Room rooms[NUM_ROOMS]) {

    int start_room_idx = -1;
    int i;

    for (i = 0; i < NUM_ROOMS; i++) {
        if (rooms[i].type == 'S') {
            start_room_idx = i;
            break;
        }
    }

    return start_room_idx; 
}


int create_room_files(struct Room rooms[NUM_ROOMS]) {

    char ROOM_DIR[1024];
    sprintf(ROOM_DIR, "trthomas.rooms.%d/", getpid());

    srand(time(NULL));

    // initialize set of rooms
    int i, j;
    for (i = 0; i < NUM_ROOMS; i++) {
        rooms[i].name = "";
        rooms[i].num_cnxns = 0;
        int j;
        for (j = 0; j < 6; j++) {
            rooms[i].cnxns[j] = -1;
        }
    }

    // now randomly assign names to set of rooms
    i = 0;
    while (i < NUM_ROOMS) {
        int r = rand() % 10;

        int unique = 1;
        for (j = 0; j < i; j++) {
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
    for (i = 0; i < NUM_ROOMS; i++) {

        // generate 3 to 6 connections for each room
        int n = (rand() % 4) + 3;
//        printf("Want %i connections for room %s. Currently have %i\n", n, rooms[i].name, rooms[i].num_cnxns);

        // add random connections
        while (rooms[i].num_cnxns < n) { 

            for (j = 0; j < NUM_ROOMS; j++) {

                if (j == i) continue;  // room can't connect to itself

                // roll the dice to determine if room j should be connected
                int c = rand() % 2;
                if (c == 1) {   // add cnxn

                    // check if connection is unique
                    int unique = 1;
                    int k;
                    for (k = 0; k < 6; k++) {
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
    for (i = 0; i < NUM_ROOMS; i++) {

        char filename[512];
        sprintf(filename, "%s%s", ROOM_DIR, rooms[i].name);
        mkdir(ROOM_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
        FILE *file;
        file = fopen(filename, "w");
        fprintf(file, "ROOM NAME: %s\n", rooms[i].name);

        // Write connections
        for (j = 0; j < rooms[i].num_cnxns; j++) {  
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
