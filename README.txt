

- CSCE 3530 Lab 2: Proxy server that requests webpage content and sends it back to the client that requested it.
		   FURTHERMORE: Also keeps a cached record of up to five webpages and has a blacklist file
		   that blocks webpages and informs the client that the requested webpage is blocked.

PLEASE NOTE: when running the program, use "make all" to compile and to run the program,
			 do: ./pserver <port> for the server and ./client <port> for the client,
			 and use "make clean" to clean up the output files. The default port that I used for testing purposes
			 was port 24853, but you can use any port.
			 
IMPORTANT: A known bug/issue is that sometimes, the server will display "address in use" if it
was terminated via CTRL+Z or CTRL+C during the previous runtime. Reopening the terminal solves this issue.
Another small bug is that if there are already fived cached webpages and you enter in a sixth one, the client
will receive the webpage content, but the proxy server might lag a bit behind or stay stuck. CTRL+C fixes this
issue and you will see that the first website entered is now gone in the list.txt file, and the most recently
cached webpage will have its own file. The first website entered has a cached file with its webpage content,
and the next time that the server is turned on, the old cached files will be deleted.


--------------------------------------------------------------------------------------
								Comments and code usage:								
--------------------------------------------------------------------------------------
I wrote comments throughout my code that were in order, so they would be easy to read
and easy to follow. The program can be viewed beginning at server.c, where the user first
reads in the number of arguments and ensures that the port number can be specified during
runtime. After this, the webpageToSend, urlFromClient, host and uri variables are declared
which will hold the information necessary to send an http request. The socket is then
initialized, and the server socket is bound to the address. After we establish our connection,
the server is ready to accept incoming connections from the client.

Moving over to the client ->
	The client checks for the number of arguments as well, making sure that the user enters
	in a specified port number. After this, a socket connection to the proxy server is established
	and we are ready to send over our http request to the server. The client
	requests the user to input a URL, and sends that information to the proxy server.
	The client will remain waiting for a response from the server until it sends one with the
	contents of the webpage. Going back to the server:

Once the URL is grabbed from the client, it is parsed by checking to see if it contains two
slashes or not, which indicates if it has https or http in the string. If it does, then the
url is separated by the slashes and put into tokens where the host name and URI information
is extracted. If the URL does not have two slashes, then the same code runs, except it looks
for if the URL has www in it or not. Once we have our host and URIs ready to go, the server
creates a TCP connection on port 80 to the website and writes a request asking for the website
information via an HTTP GET request. The website responds back, the server reads this response,
passes it to the webpageContent character array and sends it to the client. It prints out the
webpageContent on its own side before the client does to ensure that both responses are the same
and that there is no loss of data. The socket connections are closed afterwards, and the
server and client connections are also terminated on the linux machines.

Furthermore, the server caches five recent webpages that the client requests, and if there is
another website requested after the initial five, then the server will create a temporary list,
add the requested page, move the first four lines from the original list.txt over to that temporary 
list file, remove the original list.txt file and then rename the temporary list to list.txt.

The websites I used to test this program were:
	
	eastday.com 20191024212729
	soso.com 20191024212212
	babytree.com 20191024212117
	go.com 20191024210836
	example.com 20191024210119
	cse.unt.edu 20191024203918

If you enter those links into my program and check the cached files, you will see that it keeps only
the five most recent pages and deletes the older ones.



