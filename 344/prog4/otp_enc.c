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

void send_to_server(const char *text, int length, int socketFD) {

    // first send how many characters to expect
    char msg_size_str[4];    // string holding size of message (3 bytes plus \0 so max is 999);
    memset(msg_size_str, '\0', sizeof(msg_size_str));
    snprintf(msg_size_str, sizeof(msg_size_str), "%03d", strlen(text));

    int charsWritten = send(socketFD, msg_size_str, strlen(msg_size_str), 0);
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
    if (charsWritten < strlen(msg_size_str)) printf("CLIENT: WARNING: Not all data written to socket!\n");
    printf("CLIENT: informing server to expect \"%s\" characters.\n", msg_size_str);
    
    
    charsWritten = send(socketFD, text, strlen(text), 0); 
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
    if (charsWritten < strlen(text)) printf("CLIENT: WARNING: Not all data written to socket!\n");
    printf("CLIENT: sending this message to server: \"%s\"\n", text);
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

    // Get plaintext from file 
    char* buffer;    // buffer to hold text

    long file_length;
    FILE *plaintextFD = fopen(argv[1], "r");
    if (plaintextFD == NULL) {
        error("CLINET: ERROR opening plaintext file");
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

    send_to_server(buffer, strlen(buffer), socketFD);

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
