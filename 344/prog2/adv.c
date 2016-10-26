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


int create_room_files();
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


/* 
 * Display user with current room information and prompt for next room.
*/ 
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


/*
 * This function accepts an array of Room structs.  It then populates the data
 * of these structs by reading/parsing the data in the room files created by
 * create_room_files function.
*/

int read_room_data(struct Room rooms[NUM_ROOMS]) {
    int r = 0;
    int c, i;
    char ROOM_DIR[1024];
    sprintf(ROOM_DIR, "trthomas.rooms.%d/", getpid());

    // Loop through each of the 10 possible room names and look for a match
    for (i = 0; i < 10; i++) {

        // assume there is a file with ROOM_NAME[i]
        char filename[512];
        sprintf(filename, "%s%s", ROOM_DIR, ROOM_NAMES[i]);
        FILE *fp = fopen(filename, "r");

        // If file descriptor is not null, that means a room with room name i exists
        if (NULL != fp) {
            rooms[r].name = ROOM_NAMES[i];
            rooms[r].num_cnxns = 0;

            // read first line from file (and ignore it since we already know the name)
            do {
                c = fgetc(fp);
                if (c == '\n') {
                    break;
                }
            } while(1);

            // Read subsequent lines defining the connections.
            // still_more_connections is a flag to indicate if there are more
            // conections to process
            int still_more_connections = 1;  
            char name[50];                   // connecting room name string read in from file
            do {
                c = fgetc(fp);               // read a character

                int n = 0;           // n holds index of current char being read into name string
                if (c == ':') {           

                    // a colon was encountered, that means the next word is a connecting room
                    c = fgetc(fp);    // read and ignore blank space after colon

                    // continue reading characters into name string until newline encountered
                    do {
                        c = fgetc(fp);  

                        if (c == '\n') {    

                            // check if next char is a 'C' (meaning more
                            // connections to process).  If not a 'C', the only
                            // other possibility is an 'R' which means we're
                            // done processing connections and can move on to
                            // room type.
                            c = fgetc(fp);

                            if (c != 'C') {  
                                still_more_connections = 0;
                            }
                            break;     
                        }

                        // char was not a newline, so append to string and keep looping
                        name[n++] = c; 
                    } while(1);


                    // finished processing the current connection 
                    name[n++] = '\0';  // terminate string

                    // add connection to room i struct by storing its index
                    // within ROOM_NAMES
                    int k;
                    for (k = 0; k < 10; k++) {
                        if (strcmp(name, ROOM_NAMES[k]) == 0) {
                            rooms[r].cnxns[rooms[r].num_cnxns] = k;
                            rooms[r].num_cnxns++;
                        }
                    }
                }
            } while(still_more_connections);
            
            // Process room type (last line of file).
            // Start by just reading up until the colon.
            do {
                c = fgetc(fp);
                if (c == ':')
                    break;
            } while(1);

            // Only need to read/store first char after colon to get type since type
            // is either START, MID or END 
            do { 
                if (c == ':') {  
                    c = fgetc(fp);      // read blank space
                    c = fgetc(fp);      // read first char indicating type 
                    rooms[r].type = c;  
                    break;
                }
            } while(1);

            fclose(fp);  
            r++;            // room array index
        }
    }

    return 0;
}


/* 
 * This function accepts an array of Room structs and finds the array index of the room 
 * with type START_ROOM.
 */
int find_start_room(struct Room rooms[NUM_ROOMS]) {

    int start_room_idx = -1;
    int i;

    // Loop until room type matches 'S' (room types are stored as 'S', 'M' or 'E'
    for (i = 0; i < NUM_ROOMS; i++) {
        if (rooms[i].type == 'S') {
            start_room_idx = i;
            break;
        }
    }

    return start_room_idx; 
}


/* 
 * This function takes no arguments.  It creates several files
 * representing the various rooms in the directory trthomas.rooms.pid, where pid
 * is the process id. The number of connections between rooms and which rooms
 * get connected is randomly generated with the restriction that there must be
 * at least 3 connections (all two-way) to a given room so that the graph is
 * fully connected.
*/ 
int create_room_files() {

    // set up directory for room files
    char ROOM_DIR[1024];
    sprintf(ROOM_DIR, "trthomas.rooms.%d/", getpid());

    // seed random number generator
    srand(time(NULL));

    // initialize data for each room
    struct Room rooms[NUM_ROOMS];
    int i, j;
    for (i = 0; i < NUM_ROOMS; i++) {
        rooms[i].name = "";
        rooms[i].num_cnxns = 0;
        for (j = 0; j < 6; j++) {
            rooms[i].cnxns[j] = -1;   
        }
    }

    // randomly assign names from the set of 10 names, to the set of 7 rooms to
    // be used in the game
    i = 0;
    while (i < NUM_ROOMS) {

        // generate a random number between 0 and 9 (corresponding to indices of ROOM_NAMES array
        int r = rand() % 10;    

        // verify this name is not already in use
        int unique = 1;
        for (j = 0; j < i; j++) {
            if (ROOM_NAMES[r] == rooms[j].name) {
                unique = 0; 
                break;
            }
        }

        // if name not already in use, assign it to room i and move on to next room
        if (unique == 1) {
            rooms[i].name = ROOM_NAMES[r];
            i++;
        }
    }


    // Randomly create 3 to 6 connections to/from each room 
    for (i = 0; i < NUM_ROOMS; i++) {

        // generate 3 to 6 connections for each room
        int n = (rand() % 4) + 3;

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

    // Write room data to files 
    for (i = 0; i < NUM_ROOMS; i++) {

        // create directory for room files
        char filename[512];
        sprintf(filename, "%s%s", ROOM_DIR, rooms[i].name);
        mkdir(ROOM_DIR, S_IRWXU | S_IRWXG | S_IRWXO);

        // create file names to correspond to room name
        FILE *file;
        file = fopen(filename, "w");

        // Write room name
        fprintf(file, "ROOM NAME: %s\n", rooms[i].name);

        // Write room connections
        for (j = 0; j < rooms[i].num_cnxns; j++) {  
            fprintf(file, "CONNECTION %i: %s\n", j+1, rooms[rooms[i].cnxns[j]].name);
        }
            
        // Write room type
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
