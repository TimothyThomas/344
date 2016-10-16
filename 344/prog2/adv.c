#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


// fopen, fclose, fseek

int main() {

    char room_dir[512];
    sprintf(room_dir, "trthomas.rooms.%i/", getpid());
    char* rooms[10];
    int NUM_ROOMS = 7;
    rooms[0] = "Mercury";
    rooms[1] = "Venus";
    rooms[2] = "Earth";
    rooms[3] = "Mars";
    rooms[4] = "Jupiter";
    rooms[5] = "Saturn";
    rooms[6] = "Uranus";
    rooms[7] = "Neptune";
    rooms[8] = "Pluto";
    rooms[9] = "Sol";



    for (int i = 1; i <= NUM_ROOMS; i++) {
        char filename[512];
        filename[0] = '\0';
        strcat(filename, room_dir);
        strcat(filename, rooms[i]);
        mkdir(room_dir, S_IRWXU | S_IRWXG | S_IRWXO);

        FILE *file;
        file = fopen(filename, "w");
        if (file == NULL) {
            printf("Something went wrong, file %s not created.\n", filename);
        }
        fclose(file);

    }

    return 0;
}
