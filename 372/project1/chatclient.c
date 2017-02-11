/*
 *  Timothy Thomas
 *  CS 372, Winter 2017
 *  Project 1
 *
 *  A simple chat client that connects to a server via TCP.
 *
 *  usage:  chatclient server_hostname server_port
 *
 *  Code and comments heavily based on Beej's Guide to Network Programming
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <netinet/in.h>

#define MAX_MSG_SIZE 500    // maximum bytes sent in a transmission

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues


/* 
 * This functions prompts the user to input a handle to be used during the chat.
 * Handle will be stored in h.
 */
void get_user_handle(char *h, int size) {
    printf("Enter your desired handle (10 chars max): ");
    memset(h, '\0', size);     // Clear out the buffer array
    fgets(h, size - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
    h[strcspn(h, "\n")] = '\0';     // Remove the trailing \n that fgets adds
    strcat(h, "> ");
}


/* 
 * This function displays the prompt and reads from standard input the user's
 * message to be sent.
 */
void get_user_message(char *handle, char *msg, int size_msg) {
    printf("\n%s ", handle);
    memset(msg, '\0', size_msg); // Clear out the buffer array
    fgets(msg, size_msg - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
    msg[strcspn(msg, "\n")] = '\0'; // Remove the trailing \n that fgets adds
}


/* 
 * This function contains the main send/receive chat loop.  At this point a connection has
 * been established with the server and communication can take place with via socket sockFD.  
 */
void chat_loop(int sockFD, char *handle) {

    char buffer[MAX_MSG_SIZE];   // holds text to be sent or received
    int charsSent, charsRead;    

    printf("Initiating connection.\nEnter \\quit to exit.");
    
    // Start send/receive loop.  Continue until either client or server transmits "\quit"
    while (1) {

        // Get message from user
        get_user_message(handle, buffer, sizeof(buffer));

        // Check if user wants to quit
        if (strcmp(buffer, "\\quit") == 0) {
            printf("Terminating connection and exiting.\n");
            close(sockFD);
            exit(0);
        }

        // construct message (prepend "handle> " to user input message)
        char complete_msg[strlen(buffer) + strlen(handle) + 1]; // add 1 for \0
        memset(complete_msg, '\0', sizeof(complete_msg));

        // concat handle and buffer
        strcat(complete_msg, handle);
        strcat(complete_msg, buffer);

        // Send message to server
        charsSent = send(sockFD, complete_msg, strlen(complete_msg), 0); 
        if (charsSent < 0) error("ERROR writing to socket");
        if (charsSent < strlen(buffer)) printf("WARNING: Not all data written to socket!\n");

        // Get return message from server
        memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
        charsRead = recv(sockFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
        if (charsRead < 0) error("ERROR reading from socket");
        
        // Check if server wants to quit
        if (strcmp(buffer, "server> \\quit") == 0) {
            printf("Server terminated connection.  Exiting.\n");
            close(sockFD);
            exit(0);
        }

        // Flush output and display message received from server
        fflush(stdout);
        printf("%s", buffer);
    }
    return;
}


int main(int argc, char *argv[]) 
{
    int status;                        // status code used for error info
    int sockFD;                        // socket file descriptor used to communicate with server
    char handle[11];                   // 10 chars + '\0'
    struct addrinfo hints, *servinfo;  // will point to the results

    if (argc != 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(1); } // Check usage & args

    memset(&hints, 0, sizeof hints);   // make sure struct is empty
    hints.ai_family = AF_UNSPEC;       // don't care if IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;   // TCP stream sockets

    // get the necessary connection information put into servinfo using getaddrinfo() 
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            exit(1);
    }

    // Set up the socket
    if ((sockFD = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
        error("ERROR opening socket");
    }

    // Connect to server
    if (connect(sockFD, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        close(sockFD);
        error("ERROR connecting");
    }
    
    // all done with this structure so free it
    freeaddrinfo(servinfo);  
    
    // Ready to chat
    get_user_handle(handle, sizeof(handle));
    chat_loop(sockFD, handle);

    return 0;
}
