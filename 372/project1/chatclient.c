/*
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

//#include <arpa/inet.h>

#define MAX_MSG_SIZE 500

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[]) 
{

    int status;
    int sockFD, charsSent, charsRead;
    char handle[11];    // 10 chars + '\0'
    char buffer[MAX_MSG_SIZE];
    struct addrinfo hints, *servinfo, *p;   // will point to the results


    if (argc != 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(1); } // Check usage & args

    memset(&hints, 0, sizeof hints);   // make sure struct is empty
    hints.ai_family = AF_UNSPEC;       // don't care if IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    // get ready to connect
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            exit(1);
    }
    
    // loop through all servinfo results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        
        // Set up the socket
        if ((sockFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            error("ERROR opening socket");
            continue;
        }

        // Connect to server
        if (connect(sockFD, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockFD);
            error("ERROR connecting");
            continue;
        }

        break;
    }

    if (p == NULL) {
        error("Failed to connect");
        exit(2);
    }

    freeaddrinfo(servinfo);  // all done with this structure
    
    // Get user's handle 
    printf("Enter your desired handle (10 chars max): ");
    memset(handle, '\0', sizeof(handle));     // Clear out the buffer array
    fgets(handle, sizeof(handle) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
    handle[strcspn(handle, "\n")] = '\0';     // Remove the trailing \n that fgets adds
    strcat(handle, "> ");
    printf("Initiating connection. Enter \\quit to exit.");

    while (1) {   // main send/receive loop

	// Get message from user
	printf("\n%s ", handle);
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

	// Check if user wants to quit
	if (strcmp(buffer, "\\quit") == 0) {
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
	charsSent = send(sockFD, complete_msg, strlen(complete_msg), 0); // Write to the server
	if (charsSent < 0) error("ERROR writing to socket");
	if (charsSent < strlen(buffer)) printf("WARNING: Not all data written to socket!\n");
	
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(sockFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("ERROR reading from socket");
        fflush(stdout);
	printf("%s", buffer);
    }

    return 0;
}


    

