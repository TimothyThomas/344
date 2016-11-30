#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


int MAX_MSG_SIZE = 999;
int LOGGING_ON = 0;        // flag to control logging to stdout

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

void check_for_illegal_chars(const char *buffer, const char *filename) {
    int i;
    char c;
    for (i = 0; i < strlen(buffer); i++) {
        c = buffer[i];
        if (c < 65 || c > 90) {
            if (c !=32 ) {
                fprintf(stderr, "ERROR: illegal characters detected in %s.\n", filename);
                exit(1);
            }
        }
    }
}


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
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
        if (charsWritten < msg_size_int) printf("CLIENT: WARNING: Not all data written to socket!\n");

        if (LOGGING_ON) {
            printf("CLIENT: sending this message to server: \"%s\"\n", transmission);
            fflush(stdout);
        }

        offset += msg_size_int;
        remaining_chars -= msg_size_int;
    }
}

int main(int argc, char *argv[])
{
    int socketFD, portNumber;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;

    if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key  port\n", argv[0]); exit(0); } // Check usage & args
    
    // Get plaintext from file 
    char* plaintext_buffer;    // buffer to hold text

    long plaintext_length;
    FILE *plaintextFD = fopen(argv[1], "r");
    if (plaintextFD == NULL) {
        error("CLIENT: ERROR opening plaintext file");
    }
    else {
        fseek(plaintextFD, 0, SEEK_END);
        plaintext_length = ftell(plaintextFD);   
        fseek(plaintextFD, 0, SEEK_SET);
        plaintext_buffer = malloc(plaintext_length + 1);    // add 1 for \0
        if (plaintext_buffer) {
            memset(plaintext_buffer, '\0', sizeof(plaintext_buffer)); // Clear out the buffer array
            fread(plaintext_buffer, 1, plaintext_length, plaintextFD);
            plaintext_buffer[strcspn(plaintext_buffer, "\n")] = '\0'; // strip trailing newline
        }
        fclose(plaintextFD);
    }

    
    // Get key from file 
    char* key_buffer;    // buffer to hold text
    long key_length;
    FILE *keyFD = fopen(argv[2], "r");
    if (keyFD == NULL) {
        error("CLIENT: ERROR opening key file");
    }
    else {
        fseek(keyFD, 0, SEEK_END);
        key_length = ftell(keyFD);   
        fseek(keyFD, 0, SEEK_SET);
        key_buffer = malloc(key_length + 1);   // add 1 for \0
        if (key_buffer) {
            memset(key_buffer, '\0', sizeof(key_buffer)); // Clear out the buffer array
            fread(key_buffer, 1, plaintext_length, plaintextFD);
            key_buffer[strcspn(key_buffer, "\n")] = '\0'; // strip trailing newline
        }
        fclose(keyFD);
    }

    // ensure key length is >= plaintext length
    if (strlen(plaintext_buffer) > strlen(key_buffer)) {
        fprintf(stderr, "ERROR: key is too short.\n");
        exit(2);
    }

    // Check for bad characters in key/plaintext files
    check_for_illegal_chars(plaintext_buffer, argv[1]);
    check_for_illegal_chars(key_buffer, argv[2]);

    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
    if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(1); }
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (socketFD < 0) error("CLIENT: ERROR opening socket");
    
    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
        fprintf(stderr, "ERROR connecting to server on port %d\n", portNumber);
        fflush(stdout);
        exit(2);
    }

    // send password
    char password[2];
    memset(password, '\0', 2);
    strcpy(password, "$");
    send(socketFD, password, 1, 0); 

    // wait for acknowledge (receive password:  '$')
    recv(socketFD, password, 1, 0);

    if (strcmp(password, "$") != 0) {
        fprintf(stderr, "CLIENT: ERROR connection rejected.\n");
        fflush(stdout);
        exit(1);
    }
    else {
        if (LOGGING_ON) {
            printf("CLIENT: successfully connected to server.  Prepraring to send plaintext...\n");
            fflush(stdout);
        }
    }

    // trim key so that it is equal in length to plaintext
    char key[strlen(plaintext_buffer) + 1];
    memset(key, '\0', strlen(plaintext_buffer) + 1);
    strncpy(key, key_buffer, strlen(plaintext_buffer)); 
    
    // Send plaintext to server
    send_to_server(plaintext_buffer, socketFD);
    
    // Send key to server
    send_to_server(key, socketFD);

    // receive ciphertext from server
    char cipher[70000], read_buffer[MAX_MSG_SIZE+1];
    memset(cipher, '\0', sizeof(cipher));
    while (strstr(cipher, "$$") == NULL) {  // until terminal is found
        memset(read_buffer, '\0', sizeof(read_buffer));
        int charsRead = recv(socketFD, read_buffer, sizeof(read_buffer)-1, 0); 
        strcat(cipher, read_buffer); 
        if (charsRead < 0) error("ERROR reading from socket");
    }
    
    // strip terminal characters from cipher
    int terminalLocation = strstr(cipher, "$$") - cipher;
    cipher[terminalLocation] = '\0';

    // send cipher to stdout
    printf("%s\n", cipher);
    fflush(stdout);

    close(socketFD); // Close the socket
    return 0;
}
