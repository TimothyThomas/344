#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


int MAX_MSG_SIZE = 999;

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
void encrypt(const char *plaintext, const char *key, char *encryption, const int length) {
    int i;
    char c, p, k;
    for (i = 0; i < length; i++) {
        k = key[i] - 65;
        p = plaintext[i] - 65;

        if (k == -33) { k = 26; }   // blank space, set to 26 since A-Z is now 0-25.
        else if (k < 0 || k > 25) { 
            error("Illegal characters detected in key.");
        }
        else {}

        if (p == -33) { p = 26; }   // blank space, set to 26 since A-Z is now 0-25.
        else if (p < 0 || p > 25) { 
            error("Illegal characters detected in plaintext.");
        }
        else {}

        c = (p + k) % 27;
        
        if (c == 26) { c = 32; }     // convert to ascii blank space
        else { c += 65; }            // convert to ascii A-Z

        encryption[i] = c;
    }
}


int main(int argc, char *argv[])
{
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char plaintext_buffer[MAX_MSG_SIZE];
    char size_buffer[4];      // holds size of incoming message (3 plus \0 so max is 999);
    char key_buffer[MAX_MSG_SIZE];
    char encryption_buffer[MAX_MSG_SIZE];
    struct sockaddr_in serverAddress, clientAddress;

    if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

    // Set up the address struct for this process (the server)
    memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

    // Set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (listenSocketFD < 0) error("ERROR opening socket");

    // Enable the socket to begin listening
    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
            error("ERROR on binding");

    listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

    // Start looping and accepting connections
    while (1) {

        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0) error("ERROR on accept");
        printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));

        // spawn child process
        pid_t spawnPid = -5;
        int childExitMethod = -5;
        spawnPid = fork();

        switch(spawnPid) {

            case -1: { perror("Error spawning process.\n"); exit(1); break; }

            case 0: {   //child process

                // child verify communication with otp_enc
                char password[2];
                memset(password, '\0', 2);
                charsRead = recv(establishedConnectionFD, password, 1, 0); 

                if (strcmp(password, "$") != 0) { 
                    printf("SERVER: Could not connect to otp_enc_d. Incorrect password.\n"); 
                    continue; 
                }
                else {
                    printf("SERVER: Successfully made connection between otp_enc_d and otp_enc.\n");
                    strcpy(password, "#");
                    // acknowledge by sending back '#'
                    send(establishedConnectionFD, password, 1, 0);
                }


                // child Get the plaintext message from the client and display it
                char complete_msg[70000], read_buffer[MAX_MSG_SIZE+1];
                memset(complete_msg, '\0', sizeof(complete_msg));
                while (strstr(complete_msg, "$$") == NULL) {  // until terminal is found
                    memset(read_buffer, '\0', sizeof(read_buffer));
                    charsRead = recv(establishedConnectionFD, read_buffer, sizeof(read_buffer)-1, 0); 
                    strcat(complete_msg, read_buffer); 
                    if (charsRead < 0) error("ERROR reading from socket");
                }

                // strip terminal characters
                int terminalLocation = strstr(complete_msg, "$$") - complete_msg;
                complete_msg[terminalLocation] = '\0';
                printf("SERVER: I received this plaintext from the client: \"%s\"\n", complete_msg);

                // child receive key
                // child check that key is big enough
                // child perform encryption
                // child send ciphertext

                exit(0);
                break;
            }

            default: {       // parent process

                // wait for child to finish. 
                // TODO: might need to remove this to handle multiple connections.
                waitpid(spawnPid, &childExitMethod, 0);
                break;
            }
        }

        /*
        // Get the key from the client and display it
        memset(key_buffer, '\0', MAX_MSG_SIZE);
        charsRead = recv(establishedConnectionFD, key_buffer, MAX_MSG_SIZE-1, 0); 
        if (charsRead < 0) error("ERROR reading from socket");
        printf("SERVER: I received this key from the client: \"%s\"\n", key_buffer);

        // Encrypt the message
        memset(encryption_buffer, '\0', MAX_MSG_SIZE);
        encrypt(plaintext_buffer, key_buffer, encryption_buffer, MAX_MSG_SIZE);
         
        printf("SERVER: encrypted message: \"%s\"\n", encryption_buffer);
        */

        // Send a Success message back to the client
        /*
        charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
        if (charsRead < 0) error("ERROR writing to socket");
        */
        close(establishedConnectionFD); // Close the existing socket which is connected to the client
    }

    close(listenSocketFD); // Close the listening socket
    return 0; 
}
