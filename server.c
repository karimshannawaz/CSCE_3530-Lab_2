
/**
	Lab 2 CSCE 3530
	Proxy server that connects to the website, sends an http request,
	and forwards that request to the client.
	ALSO: caches up to 5 recent webpages and blocks sites that are
	in the blacklist.
	I have commented throughout my code and hopefully that makes things
	clear.

 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <regex.h>
#include <stdbool.h>
#include <dirent.h> 
#include <sys/stat.h>

// Initializes the server and listens to requests on the network.
int initializeServer(int port);

// Prints the error message when a certain condition is met, and prints the message of error.
void printErrorMessage(char condition, const char * error_mgs);

// Returns true if the specified word starts with the second parameter.
bool startsWith(const char* word, const char* startsWith);

// Returns true if the specified word in the first parameter contains the word in the second param.
bool contains(char* word, char* containsWord);

// Returns true if the specified file exists.
bool fileExists(const char* file);

// Returns the content of the webpage that the client requested from us.
void getWebpageContent(char* uri, char* host, char* webpageContent);

// Returns true if the current time is within range of the times passed as the parameters.
bool withinRange(char* start, char* end);

// Returns the current time in YYYYMMDDhhmmss format
char* getCurrentTime();

char* getCachedContent(char* cachedURL);

void cleanCachedFiles();

void cacheWebpage(char* host, char* webpageContent);

// Main function which runs the program.
int main(int argc, char ** argv) {
    // Makes sure that the command line arguments are sufficient for
	// this program.
    if (argc != 2) {
        printf("Invalid command lind arguments; Usage: ./pserver <port>\n");
        return 1;
    }     
	
	// Create the list file if it doesn't exist.
	if(!fileExists("./list.txt")) {
		FILE *fp = fopen("list.txt", "ab+");
		fclose(fp);
	}
	
	// Create the blacklist file if it doesn't exist.
	if(!fileExists("./blacklist.txt")) {
		FILE *fp = fopen("blacklist.txt", "ab+");
		fclose(fp);
	}
	
	// Cleanup the cached files in the same directory if they're no longer in the list file.
	cleanCachedFiles();
	
	// Declare our strings which we will use to build the website content
	// and send it back to the client.
	char webpageToSend[128000];
	char urlFromClient[2048];
	char host[512];
	char uri[1536];
	
	// Holds the cached version of the URL.
	char cachedURL[512];
	
	// Set to true if the URL requested is blacklisted.
	bool isBlacklisted = false;
	
	// Set to true if the URL requested is already cached.
	bool isCached = false;
   
    // Starts to accept requests from the network.
    int sockfd = initializeServer(atoi(argv[1]));
    printf("Socket established; waiting on client to connect...\n");
	
	// Incoming connections are handled here; any newly connected client will appear
	// and have its own unique connection ID.
	int connectionFD = accept(sockfd, (struct sockaddr*) NULL, NULL);
	printf("Client connected.\n");
	
	// Once the client is connected, we are ready to grab the URL from the client.
	// The server reads the url sent and passes it to 'urlFromClient.'
	bzero(urlFromClient, 2048);
	read(connectionFD, urlFromClient, 2048);
	printf("Received URL from client: %s\n", urlFromClient);
	
	// The two code blocks below parse the URL and pass the host name to
	// the host char array and the URI to the uri char array.
	// URL starts with http:// or https://
	if(contains(urlFromClient, "//")) {
		// Splits up the URL into tokens separated by two slashes.
		char * tokens;
		tokens = strtok(urlFromClient, "//");
		char splitHostName[512];
		tokens = strtok(NULL, "//");
		
		// Host name is found and passed to the host char array.
		strncpy(splitHostName, tokens, 512);
		// Handles whether the URL contains www. or not and skips ahead 4 characters
		// in the host name if it does.
		if(startsWith(splitHostName, "www.")) {
			strncpy(host, &splitHostName[4], 512);
		}
		else {
			strncpy(host, splitHostName, 512);
		}
		tokens = strtok(NULL, "//");
		// No URI exists
		if(tokens == NULL) {
			strncpy(uri, "/", 1536);
		}
		// URI was found after the next slash /
		else {
			strncpy(uri, "/", 1536);
			strcat(uri, tokens);
		}
		printf("Host name: %s\n", host);
		printf("URI: %s\n", uri);
	}
	// Handles if the URL starts with www or not
	else {
		char * tokens;
		// Splits up the URL into tokens separated by one slash.
		tokens = strtok(urlFromClient, "/");
		strncpy(host, startsWith(urlFromClient, "www.") ? &tokens[4] : tokens, 512);
		tokens = strtok(NULL, "/");
		// No URI exists
		if(tokens == NULL) {
			strncpy(uri, "/", 1536);
		}
		// URI was found after the next slash /
		else {
			strncpy(uri, "/", 1536);
			strcat(uri, tokens);
		}
		printf("Host: %s\n", host);
		printf("URI: %s\n", uri);
		
	}
	
	
	// READS THE LIST FILE
	{
		FILE *fp;
		char urlInFile[512];
		char* fileName = "./list.txt";
	 
		fp = fopen(fileName, "r");
		if (fp == NULL){
			printf("Unable to open the list file: %s", fileName);
			return 1;
		}
		int index = 0;
		while (fgets(urlInFile, 512, fp) != NULL) {
			if(startsWith(urlInFile, "\n")) {
				continue;
			}
			urlInFile[strcspn(urlInFile, "\r\n")] = 0;
			if(contains(urlInFile, host)) {
				char* tokens;
				tokens = strtok(urlInFile, " ");
				tokens = strtok(NULL, " ");
				// Copied
				strncpy(cachedURL, tokens, 512);
				isCached = true;
			}
			index++;
		}
		fclose(fp);
	}
	// END READ LIST FILE
	
	
	// READS THE BLACKLIST FILE
	FILE *fp;
    char urlInFile[512];
    char* blFile = "./blacklist.txt";
 
    fp = fopen(blFile, "r");
    if (fp == NULL){
        printf("Unable to open the blacklist file: %s", blFile);
        return 1;
    }
	int index = 0;
    while (fgets(urlInFile, 512, fp) != NULL) {
		if(startsWith(urlInFile, "\n")) {
			continue;
		}
		urlInFile[strcspn(urlInFile, "\r\n")] = 0;
		//printf("Index: %d : %s\n", index, urlInFile);
		if(contains(urlInFile, host)) {
			//printf("Blacklist URL %s found in the list at index %d\n", urlInFile, index);
			char* tokens;
			tokens = strtok(urlInFile, " ");
			char startTime[128];
			tokens = strtok(NULL, " ");
			// Copied to startTime
			strncpy(startTime, tokens, 128);
			char endTime[128];
			tokens = strtok(NULL, " ");
			// Copied to endTime
			strncpy(endTime, tokens, 128);
			
			// If the current user time is within range of the start and end times,
			// then we blacklist the site.
			if(withinRange(startTime, endTime)) {
				isBlacklisted = true;
			}
		}
		index++;
	}
    fclose(fp);
	// END READ BLACKLIST FILE
	
	// Sends the webpage back to the client after it requests it from the website.
	while(1) {
		printf(isBlacklisted ? "Website is blocked, we will inform the client about this.\n\n" :
			   isCached ? "Sending cached webpage back to the client...\n\n" :
			   "Starting to send webpage back to client...\n");
		
		// Ask for the website from the webpage
		bzero(webpageToSend, 128000);
		
		if(isBlacklisted) {
			strncpy(webpageToSend, "The requested URL is blocked. Please try again later.\n", 256);
		}
		else if(isCached) {
			strncpy(webpageToSend, getCachedContent(cachedURL), 128000);
		}
		else {
			getWebpageContent(uri, host, webpageToSend);
		}
		// Writes the contents of the webpage to the client.
		write(connectionFD, webpageToSend, strlen(webpageToSend));
		
		// Close our connection once we are done.
		close(connectionFD); 
		break;
    }
	
	// Closes our socket once we are done using it.
    close(sockfd);
	return 0;
}

// Initializes the server and listens to requests on the network.
int initializeServer(int port) {
	
	// Creates our server socket and opens it for connections.
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serverAddress;

    printErrorMessage(sockfd == -1, "Failed to initialize the server socket.");

	// Sets our server's address here.
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
	// Sets the port to be the same as the client.
    serverAddress.sin_port = htons(port);

    // Binds the socket to the address.
    printErrorMessage(bind(sockfd, (struct sockaddr *) &serverAddress, 
		sizeof(serverAddress)) == -1, "Failed to bind the server.");
    
	int option = 1;
    
	// This enables us to reuse our socket.
    printErrorMessage(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
		&option, sizeof(option)) == -1, "Failed on setsocketoption.");
    
	// We can listen to a maximum of 10 connections here.
    printErrorMessage(listen(sockfd, 10) != 0, "Failed to listen to the server.");

    return sockfd;
}

// Prints the error message when a certain condition is met, and prints the message of error.
void printErrorMessage(char condition, const char * error_mgs) {
	// Only prints if the condition is met.
	if (!condition) 
		return;
	perror(error_mgs);
	exit(1);
}

// Returns true if the specified word starts with the second parameter.
bool startsWith(const char* word, const char* startsWith) {
	// Compares the two strings to see if there's a match.
	if(strncmp(word, startsWith, strlen(startsWith)) == 0) 
		return 1;
	return 0;
}

// Returns true if the specified word in the first parameter contains the word in the second param.
bool contains(char* word, char* containsWord) {
	if (strstr(word, containsWord) != NULL)
		return true;
	return false;
}

// Returns true if the specified file exists.
bool fileExists(const char* file) {
    struct stat buffer;
    int exists = stat(file, &buffer);
    if(exists == 0)
        return true;
    else
        return false;
}

// Returns 1 if the current time is within range of the times passed as the parameters.
bool withinRange(char* start, char* end) {
	
	long long startToLL = atoll(start);
	long long endToLL = atoll(end);
	long long currentTimeToLL = atoll(getCurrentTime());
	
	if(currentTimeToLL >= startToLL && currentTimeToLL <= endToLL) {
		return true;
	}
	
	return false;
}

// Returns the current time in YYYYMMDDhhmmss format
char* getCurrentTime() {
	time_t getTime;
	char currentTime[64];
	struct tm* timeInformation;

	time(&getTime);
	timeInformation = localtime(&getTime);

	strftime(currentTime, 64, "%Y%m%d%H%M%S", timeInformation);
	
	return strdup(currentTime);
}

char* getCachedContent(char* cachedURL) {
	
	// READS THE CACHED FILE
	FILE *fp;
    char content[128];
    char blFile[128];
	
	char toReturn[128000];
	
	snprintf(blFile, sizeof(blFile), "./%s.txt", cachedURL);
	
    fp = fopen(blFile, "r");
    if (fp == NULL){
        printf("Unable to open or find the cached file: %s\n", blFile);
		exit(1);
    }
	
	strcat(toReturn, "***** This is a cached version of your requested URL. *****\n\n");
	
    while (fgets(content, 128, fp) != NULL) {
		printf("%s", content);
		strcat(toReturn, content);
	}
    fclose(fp);
	printf("\n");
	// END READ CACHED FILE
	
	return strdup(toReturn);
}

void cleanCachedFiles() {
	
	// RECEIVED FROM GEEKSFORGEEKS.ORG.
	struct dirent *de;  // Pointer for directory entry 
	DIR* dr = opendir("."); 
    if (dr == NULL) { 
        printf("Could not open current directory to check cached files."); 
        return; 
    }
	
	while ((de = readdir(dr)) != NULL)  {
		bool foundFile = false;
		if(startsWith(de->d_name, "2019")) {
			// printf("%s\n", de->d_name);
			// READS THE LIST FILE FOR CACHED FILES
			FILE *fp;
			char urlInFile[512];
			char* fileName = "./list.txt";
		 
			fp = fopen(fileName, "r");
			if (fp == NULL){
				printf("Unable to open the list file: %s", fileName);
				return;
			}
			int index = 0;
			while (fgets(urlInFile, 512, fp) != NULL) {
				if(startsWith(urlInFile, "\n")) {
					continue;
				}
				urlInFile[strcspn(urlInFile, "\r\n")] = 0;
				char* tokens;
				tokens = strtok(urlInFile, " ");
				char cachedURL[512];
				tokens = strtok(NULL, " ");
				// Copied to cachedURL
				strncpy(cachedURL, tokens, 512);
				if(contains(de->d_name, cachedURL)) {
					foundFile = true;
				}
				index++;
			}
			fclose(fp);
			// END READ LIST FILE
			if(!foundFile) {
				remove(de->d_name);
				printf("Deleted old cached webpage: %s\n", de->d_name);
			}
		}
	}
  
    closedir(dr);
}

void cacheWebpage(char* host, char* webpageContent) {
	char* time = getCurrentTime();
	// READS THE LIST FILE FOR CACHED FILES
	FILE *fp;
	char lineInFile[512];
	char* fileName = "./list.txt";
		 
	fp = fopen(fileName, "r");
	if (fp == NULL){
		printf("Unable to open the list file: %s", fileName);
		return;
	}
	int index = 0;
	while (fgets(lineInFile, 512, fp) != NULL) {
		if(index > 4)
			break;
		if(startsWith(lineInFile, "\n"))
			continue;
		lineInFile[strcspn(lineInFile, "\r\n")] = 0;
		index++;
	}
	fclose(fp);
	
	// If nothing was found in the file, write first cached site.
	if(index == 0) {
		// Write the host and time to the list file
		FILE * fp;
		fp = fopen("./list.txt", "w");
		fprintf(fp, "%s %s", host, time);
		fclose(fp);
		
		// Then, we write the webpage content to the time file.
		char newFile[512];
		strncpy(newFile, time, 512);
		strcat(newFile, ".txt\0");
		fp = fopen(newFile, "ab+");
		fprintf(fp, "%s", webpageContent);
		fclose(fp);

		printf("Cached webpage: %s to file: %s.txt\n", host, time);
	}
	// There were webpages in the file already, so we copy and paste
	else {
		// Write the host and time to the list file
		FILE* listfp = fopen("./listTemp.txt", "ab+");
		fprintf(listfp, "%s %s\n", host, time);
			
		// Then, we write the webpage content to the time file.
		char newFile[512];
		strncpy(newFile, time, 512);
		strcat(newFile, ".txt\0");
		FILE* timefp = fopen(newFile, "ab+");
		fprintf(timefp, "%s", webpageContent);
		fclose(timefp);
	   
	   
		FILE* mainfp;
		bzero(lineInFile, 512);
		mainfp = fopen("./list.txt", "r");
		index = 0;
		while (fgets(lineInFile, 512, mainfp) != NULL) {
			if(index > 3) {
				break;
			}
			if(startsWith(lineInFile, "\n")) {
			   continue;
			}
			lineInFile[strcspn(lineInFile, "\r\n")] = 0;
			fprintf(listfp, "%s\n", lineInFile);
			index++;
		}
		fclose(mainfp);
		fclose(listfp);
		
		remove("list.txt");
		rename("listTemp.txt", "list.txt");
		printf("Cached webpage: %s to file: %s.txt\n", host, time);
   }
   // END READ LIST FILE
}

// Returns the content of the webpage that the client requested from us.
void getWebpageContent(char* uri, char* host, char* webpageContent) {
	
	// The HTTP request includes the 
	char httpRequest[2000];
	// This is the entire content of the website that we are trying to grab.
	char contentGrabbed[128000];
	
	// Connects to the tcp server through port 80.
	int portNum = 80;
	
	// Initialize our socket variables.
	int socketFD;
	struct sockaddr_in serverAddress;
	struct hostent* webServer;
	
	// Sets the server to be the host passed as the parameter.
	webServer = gethostbyname(host);
	
	// Performs a check to verify if the web server is valid or not.
	if(webServer == NULL) {
		printf("Invalid response from the proxy server, try another URL.\n");
		return;
	}
	
	// Creates the webserver socket and opens it for connections
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	printErrorMessage(socketFD == -1, 
		"Error: failed to start the http request on: socket");
		
	// Zeroes out and sets the address via the specified port.
	bzero((char*) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char*) webServer->h_addr, (char*) 
		&serverAddress.sin_addr.s_addr, webServer->h_length);
		serverAddress.sin_port = htons(portNum);
	
	// Attempts to connect to the website and returns an error if it can't.
	printErrorMessage(connect(socketFD, (struct sockaddr*) 
		&serverAddress, sizeof(serverAddress)) < 0, "Could not establish a connection to the website.\n");
	
	// This is where we create our HTTP request with the host and URI that
	// we grabbed from the URL sent by the client. This is a GET request
	// which returns the HTML document if the webpage doesn't have problems or hasn't moved.
	sprintf(httpRequest, 
		"GET %s HTTP/1.1\r\nHost: %s\r\n\r\n\r\n", uri, host);
		
	// Writes the request to the website and asks the website for the page content.
	printErrorMessage(write(socketFD, httpRequest, strlen(httpRequest)) < 0, "Could not write a request to the website.\n");
	
	// Zeroes out our content and the request
	bzero(contentGrabbed, 128000);
	bzero(httpRequest, 2000);
	
	// .. And reads the content from the webpage.
	read(socketFD, contentGrabbed, sizeof(contentGrabbed));
	
	// Copies the content that we read from the webpage onto the webpageContent char array
	// This will be sent to the client.
	strcpy(webpageContent, contentGrabbed);
	
	
	// We handle caching here by determining if the code contains 200 or not.
	char* token;
	char* webPtr;
	char* freeSpace;
	freeSpace = webPtr = strdup(webpageContent);
	int index = 0;
	while ((token = strsep(&webPtr, "\n"))) {
		if(index == 0) {
			// Found HTTP code 200, we will cache here.
			if(contains(token, "200")) {
				printf("\n***** This webpage's HTTP response code is 200 and it will be cached. *****\n");
				cacheWebpage(host, webpageContent);
			}
			// HTTP code 200 not found, we will not cache but still send the response
			// to the client.
			else {
				// Do nothing here for now.
				printf("\n*** This webpage's HTTP response code is NOT 200 and it will NOT be cached. ***\n");
				printf("*** The requested webpage response will still be sent back to the client. ***\n");
			}
		}
		index++;
	}
	free(freeSpace);
	
	
	// Printing the same message to the server too to verify that 
	// the client and server both received the same content that
	// was requested.
	printf("\n%s\n", webpageContent);
	
	// Close our socket once we are done using it.
	close(socketFD);
}
