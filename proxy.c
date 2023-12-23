// proxy.c
#include "utility.h"
#define MAX_URL_LEN 1000

int tcp_open_client(char *host, char* port) {
	int			sockfd;
	struct sockaddr_in	serv_addr;

	/* Fill in "serv_addr" with the address of the server */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family	  = AF_INET;
	serv_addr.sin_addr.s_addr = my_inet_addr(host);
	serv_addr.sin_port	  = htons(atoi(port));

	/* Open a TCP socket (an Internet stream socket). */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0
	   || connect(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0)
		return -1;
	return sockfd;
}

int tcp_open_server(char *port) {
	int			sockfd;
	struct sockaddr_in	serv_addr;

	/* Open a TCP socket (an Internet stream socket). */
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	/* Bind our local address so that the client can send to us. */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family	  = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port	  = htons(atoi(port));

	if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		return -1;
	listen(sockfd, 5);
	return sockfd;
}

void proxy_request(int client_sockfd, char *server_host, char *server_port, char *url) {
    int server_sockfd;
    char buf[MAX_LINE];

    // Connect to the local server (localhost:8080)
    server_sockfd = tcp_open_client(server_host, server_port);
    if (server_sockfd < 0) {
        fprintf(stderr, "Error connecting to %s:%s\n", server_host, server_port);
        return;
    }

    // Forward the client request to the local server
    while (1) {
        int ret = readready(client_sockfd);
        if (ret < 0) break;
        if (ret > 0) {
            if (readline(client_sockfd, buf, MAX_LINE) <= 0) break;
            send(server_sockfd, buf, strlen(buf), 0);
        }
    }

    // Receive the response from the local server and forward it to the client
    while (1) {
        int ret = readready(server_sockfd);
        if (ret < 0) break;
        if (ret > 0) {
            if (readline(server_sockfd, buf, MAX_LINE) <= 0) break;
            send(client_sockfd, buf, strlen(buf), 0);
        }
    }

    close(server_sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int sockfd, newsockfd, clilen, childpid;
    struct sockaddr_in cli_addr;

    sockfd = tcp_open_server(argv[1]);

    for (;;) {
        // Wait for a connection from a client process (Concurrent Server)
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) exit(1); /* server: accept error */
        if ((childpid = fork()) < 0) exit(1); /* server: fork error */

        if (childpid == 0) { /* child process */

            close(sockfd); /* close original socket */

            // Prompt the user to enter a URL
            char url[MAX_URL_LEN];
            printf("Enter URL: ");
            fgets(url, MAX_URL_LEN, stdin);
            strtok(url, "\n"); // Remove the newline character

            proxy_request(newsockfd, "localhost", "8080", url); /* process the request */
            exit(0);
        }

        close(newsockfd); /* parent process */
    }
}

//http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html
