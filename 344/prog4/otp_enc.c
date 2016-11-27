#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


int MAX_SEND_SIZE = 999;

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

void send_to_server(const char *text, int socketFD) {

    int msg_size_int;
    int offset = 0;          // keeps track of how many chars have been sent so far
    char complete_msg[strlen(text) + 3];  // add 2 for terminal chars and 1 for \0
    memset(complete_msg, '\0', sizeof(complete_msg));
    strcat(complete_msg, text);
    strcat(complete_msg, "$$");
    int remaining_chars = strlen(complete_msg);

    while (remaining_chars > 0) {

        // determine number of characters to send not including \0
        if (remaining_chars > MAX_SEND_SIZE) {
            msg_size_int = MAX_SEND_SIZE;
        }
        else {
            msg_size_int = remaining_chars; 
        }

        char transmission[msg_size_int + 1];   // add one for \0
        memset(transmission, '\0', sizeof(transmission));
        strncpy(transmission, complete_msg+offset, msg_size_int);
        
        int charsWritten = send(socketFD, transmission, msg_size_int, 0); 
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
        if (charsWritten < msg_size_int) printf("CLIENT: WARNING: Not all data written to socket!\n");

        printf("CLIENT: sending this message to server: \"%s\"\n", transmission);

        offset += msg_size_int;
        remaining_chars -= msg_size_int;
    }
}

int main(int argc, char *argv[])
{
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;

    if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key  port\n", argv[0]); exit(0); } // Check usage & args

    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
    if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(1); }
    memcpy((char*)serverHostInfo->h_addr, (char*)&serverAddress.sin_addr.s_addr, serverHostInfo->h_length); // Copy in the address

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (socketFD < 0) error("CLIENT: ERROR opening socket");
    
    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
            error("CLIENT: ERROR connecting");

    // send password
    char password[2];
    memset(password, '\0', 2);
    strcpy(password, "$");
    charsWritten = send(socketFD, password, 1, 0); 

    // wait for acknowledge (receive password:  '#')
    recv(socketFD, password, 1, 0);

    if (strcmp(password, "#") != 0) {
        printf("CLIENT: ERROR received incorrect password (%s) back from server.", password);
        exit(1);
    }
    else {
        printf("CLIENT: successfully connected to server.  Prepraring to send plaintext...\n");
    }

    // Get plaintext from file 
    char* buffer;    // buffer to hold text

    long file_length;
    FILE *plaintextFD = fopen(argv[1], "r");
    if (plaintextFD == NULL) {
        error("CLIENT: ERROR opening plaintext file");
    }
    else {
        fseek(plaintextFD, 0, SEEK_END);
        file_length = ftell(plaintextFD);   // subtract one since we don't want the newline at end
        fseek(plaintextFD, 0, SEEK_SET);
        buffer = malloc(file_length + 1);
        if (buffer) {
            memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
            fread(buffer, 1, file_length, plaintextFD);
            buffer[strcspn(buffer, "\n")] = '\0'; // strip trailing newline
        }
        fclose(plaintextFD);
    }
    
    /*
    fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
    */

    send_to_server(buffer, socketFD);

    /*
    // Send plaintext message to server
    charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
    if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
    */
    
    /*
    // Get key from file 
    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
    FILE *keyFD = fopen(argv[2], "r");
    if (keyFD == NULL) {
        error("CLINET: ERROR opening key file");
    }
    else {
        fread(buffer, 1, file_length, keyFD);
        fclose(keyFD);
    }
    
    // Send key to server
    charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
    if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
    */

    // Get return message from server
    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse

    /*
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
    if (charsRead < 0) error("CLIENT: ERROR reading from socket");
    printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
    */

    close(socketFD); // Close the socket
    return 0;
}
