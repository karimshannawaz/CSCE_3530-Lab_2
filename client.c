
/**
	Lab 2 CSCE 3530
	Client that connects to the proxy server.

 */

#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

// Connects to the TCP server using the specified hostname (cse01) and the port (Default: 24853)
int connectToServer(int portNumber);

// Prints the error message when a certain condition is met, and prints the message of error.
void printErrorMessage(char condition, const char * errorMessage);

// Returns true if the specified word in the first parameter contains the word in the second param.
bool contains(char* word, char* containsWord);

int main(int argc, char ** argv) {
    // Makes sure that the command line arguments are sufficient for
	// this program.
    if (argc != 2) {
        printf("Invalid command lind arguments; Usage: ./client <port>\n");
        return 1;
    }
	
    // The proxy server port that we will connect to
    const int tcpServerPort = atoi(argv[1]);
   
    // Establishes the connection to the proxy server through cse01
    int sockfd = connectToServer(tcpServerPort);
	
	// Closes our connection once we're done with it.
    close(sockfd);
    return 0;
}

int connectToServer(int portNumber) {
	// Declare our current socket
    int sockfd;
	
	// The webpage info that the server sends back.
	char webpageContent[128000];
	
	// The address information for the connector
    struct sockaddr_in serverAddress;
    struct hostent* server;

	// Creates the socket here.
    printErrorMessage((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1, "Failed to connect through the socket.");
    printErrorMessage((server = gethostbyname("cse01")) == NULL, "Failed to connect to cse01.");

    // Here we create the server address.
	// Zeros out the structure to create the address.
    bzero(&(serverAddress.sin_zero), 8);

	// Byte order of the host.
    serverAddress.sin_family = AF_INET;
	// Sets the port to connect through.
    serverAddress.sin_port = htons(portNumber);
	
	// Finally establishes a connection to the server.
    bcopy((char *) server->h_addr, (char *) &serverAddress.sin_addr.s_addr, server->h_length);
	printErrorMessage(connect(sockfd, (struct sockaddr *)&serverAddress, 
		sizeof(struct sockaddr)) == -1, "Failed to connect to the server.");
	
	// Our web URL might be long, so 1000 is conservative.
	// Here, we ask the user to type in the URL. It can be in
	// any format that the user desires (http://____.com, https://______.com, https://www.____.com, _____.com, etc.)
	char webURL[1000];
	printf("Type in the URL to send: ");
	// URL is now stored in the character array created before.
	scanf("%[^\n]%*c", webURL);
	
	// URL is sent to the server through the socket.
	send(sockfd, webURL, strlen(webURL), 0);
	printf("Sent URL: %s to the server.\n\n", webURL);
	
	// Waits for the server to send the webpage content back, and prints it out.
	// The server should also be printing it out at the same time as the client to verify.
	while(read(sockfd, webpageContent, sizeof(webpageContent) > 0)) {
		printf("%s", webpageContent);
	}
	printf("\n");
	return sockfd;
}

// Prints the error message when a certain condition is met, and prints the message of error.
void printErrorMessage(char condition, const char * errorMessage) {
	// Only prints if the condition is met.
	if (!condition) 
		return;
	perror(errorMessage);
	exit(1);
}

// Returns true if the specified word in the first parameter contains the word in the second param.
bool contains(char* word, char* containsWord) {
	if (strstr(word, containsWord) != NULL)
		return true;
	return false;
}






