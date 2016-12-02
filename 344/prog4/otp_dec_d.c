/* 
 * CS 344 Fall 2016
 * Program 4 -- otp_dec_d
 * by Tim Thomas 
 *
 * Usage:   ./otp_dec_d port 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>


int MAX_MSG_SIZE = 999;    // max number of characters that can be sent/recieved at once
int LOGGING_ON = 0;        // flag to control logging to stdout

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/* 
 * This functions sends the text string pointed to by "text" to the socket file 
 * descriptor pointed to by socketFD.
 */
void send_to_client(const char *text, int socketFD) {

    int msg_size_int;
    int offset = 0;          // keeps track of how many chars have been sent so far
    char complete_msg[strlen(text) + 3];  // add 2 for terminal chars and 1 for \0
    memset(complete_msg, '\0', sizeof(complete_msg));
    strcat(complete_msg, text);
    strcat(complete_msg, "%%");
    int remaining_chars = strlen(complete_msg);

    while (remaining_chars > 0) {

        // determine number of characters to send not including \0
        if (remaining_chars > MAX_MSG_SIZE) {
            msg_size_int = MAX_MSG_SIZE;
        }
        else {
            msg_size_int = remaining_chars; 
        }

        char transmission[msg_size_int + 1];   // add one for \0
        memset(transmission, '\0', sizeof(transmission));
        strncpy(transmission, complete_msg+offset, msg_size_int);
        
        int charsWritten = send(socketFD, transmission, msg_size_int, 0); 
        if (charsWritten < 0) error("SERVER: ERROR writing to socket");
        if (charsWritten < msg_size_int) printf("SERVER: WARNING: Not all data written to socket!\n");

        if (LOGGING_ON) {
            printf("SERVER: sending this message to client: \"%s\"\n", transmission);
            fflush(stdout);
        }

        offset += msg_size_int;
        remaining_chars -= msg_size_int;
    }
}


/*
 * This function decrypts text stored in ciphertext using modulo arithmetic
 * with the characters stored in key.  The result is stored in plaintext.
 */
void decrypt(const char *ciphertext, const char *key, char *plaintext) {
    int i;
    char c, p, k;
    
    // loop through each character in ciphertext since it is shorter than key
    for (i = 0; i < strlen(ciphertext); i++) {
        k = key[i] - 65;
        c = ciphertext[i] - 65;

        if (k == -33) { k = 26; }   // blank space, set to 26 since A-Z is now 0-25.

        if (c == -33) { c = 26; }   // blank space, set to 26 since A-Z is now 0-25.

        if ((c - k) < 0) {
            p = (c - k ) + 27;
        }

        else {
            p = (c - k) % 27;
        }

        if (p == 26) { p = 32; }     // convert to ascii blank space
        else { p += 65; }            // convert to ascii A-Z

        plaintext[i] = p;
    }
}


int main(int argc, char *argv[])
{
    // initialize everything needed for setting up sockets
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
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

        if (LOGGING_ON) {
            printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));
            fflush(stdout);
        }

        // spawn child process
        pid_t spawnPid = -5;
        spawnPid = fork();

        switch(spawnPid) {

            case -1: { perror("Error spawning process.\n"); exit(1); break; }

            case 0: {   //child process

                // child verify communication with otp_dec
                char password[2];
                memset(password, '\0', 2);
                charsRead = recv(establishedConnectionFD, password, 1, 0); 

                // Check that correct password received
                if (strcmp(password, "%") != 0) { 
                    fprintf(stderr, "ERROR: Connection of otp_enc to otp_dec_d not allowed.\n"); 
                    fflush(stdout);
                    exit(1); 
                }
                else {
                    if (LOGGING_ON) {
                        printf("SERVER: Successfully made connection between otp_enc_d and otp_enc.\n");
                        fflush(stdout);
                    }
                    strcpy(password, "%");
                    // acknowledge by sending back '%'
                    send(establishedConnectionFD, password, 1, 0);
                }

                // child Get the ciphertext message from the client and display it
                char complete_msg[70000], read_buffer[MAX_MSG_SIZE+1];
                memset(complete_msg, '\0', sizeof(complete_msg));
                while (strstr(complete_msg, "%%") == NULL) {  // until terminal is found
                    memset(read_buffer, '\0', sizeof(read_buffer));
                    charsRead = recv(establishedConnectionFD, read_buffer, sizeof(read_buffer)-1, 0); 
                    strcat(complete_msg, read_buffer); 
                    if (charsRead < 0) error("ERROR reading from socket");
                }
                
                // Acknowledge plaintext received to split up the communication
                // or else the key may bleed into the plaintext reads
                send(establishedConnectionFD, password, 1, 0);

                // strip terminal characters
                int terminalLocation = strstr(complete_msg, "%%") - complete_msg;
                complete_msg[terminalLocation] = '\0';
                if (LOGGING_ON) {
                    printf("SERVER: I received this ciphertext from the client: \"%s\"\n", complete_msg);
                    fflush(stdout);
                }

                // child receive key
                char complete_key[70000];
                memset(complete_key, '\0', sizeof(complete_key));
                while (strstr(complete_key, "%%") == NULL) {  // until terminal is found
                    memset(read_buffer, '\0', sizeof(read_buffer));
                    charsRead = recv(establishedConnectionFD, read_buffer, sizeof(read_buffer)-1, 0); 
                    strcat(complete_key, read_buffer); 
                    if (charsRead < 0) error("ERROR reading from socket");
                }

                if (LOGGING_ON) {
                    printf("SERVER: I received this key from the client: \"%s\"\n", complete_key);
                    fflush(stdout);
                }

                // Note: don't need to strip terminal chars from complete_key since decrypt() is
                // based on length of complete_msg.(which was already validated
                // in otp_enc to be less than or equal to the length of complete_key.
                
                // child perform decryption
                char complete_plaintext[70000];
                memset(complete_plaintext, '\0', sizeof(complete_plaintext));
                decrypt(complete_msg, complete_key, complete_plaintext);

                if (LOGGING_ON) {
                    printf("SERVER: decrypted message: \"%s\"\n", complete_plaintext);
                    fflush(stdout);
                }
                
                // child send back plaintext 
                send_to_client(complete_plaintext, establishedConnectionFD);

                exit(0);
                break;
            }
        }

        close(establishedConnectionFD); // Close the existing socket which is connected to the client
    }

    close(listenSocketFD); // Close the listening socket
    return 0; 
}
