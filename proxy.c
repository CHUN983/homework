// proxy.c
#include <signal.h>
#include "utility.h"


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

void do_main(int newsockfd) {
	int		ret;
	char		buf[MAX_LINE];

	while((ret = readready(newsockfd)) >= 0) {
		if(ret == 0) continue;
		if(readline(newsockfd, buf, MAX_LINE) <= 0) break;
		fputs(buf, stdout);
		if(buf[0] == '@') {
			int		len;

			len = strlen(buf);
			send(newsockfd, buf+1, len-1, 0);
		} else if(strncmp(buf, "QUIT", 4) == 0) {
			kill(getppid(), SIGKILL);
			break;
		} else if(strncmp(buf, "GET ", 4) == 0) {
			char	bufskip[MAX_LINE];
			char	*msg =
				"HTTP/1.0 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"\r\nTest OK\n";
            char *my_msg="Welcome! I'm A1085501 from Taiwan.";

			while(readline(newsockfd, bufskip, MAX_LINE))
				if(strncmp(bufskip, "\r\n", 2) == 0) break;
			send(newsockfd, msg, strlen(msg), 0);
			send(newsockfd, buf, strlen(buf), 0);
            send(newsockfd, my_msg,strlen(my_msg), 0);
			break;
		}
	}
}

int http_get(char *url, char *proxy_host, char *proxy_port) {
    int sockfd, ret;
    char buf[MAX_LINE];

    // Parse the URL to extract host and port
    char host[MAX_URL_LEN];
    char path[MAX_URL_LEN];
    char *port = "80"; // Default HTTP port

    if (sscanf(url, "http://%[^:/]:%hhd/%s", host, port, path) == 3) {
        // URL has host, port, and path
    } else if (sscanf(url, "http://%[^:/]/%s", host, path) == 2) {
        // URL has host and path
    } else if (sscanf(url, "http://%[^:/]:%hhd", host, port) == 2) {
        // URL has host and port
    } else if (sscanf(url, "http://%[^:/]", host) == 1) {
        // URL has only host
    } else {
        fprintf(stderr, "Invalid URL format\n");
        return -1;
    }

    // Connect to the proxy server (172.30.148.62:8080)
    sockfd = tcp_open_client(proxy_host, proxy_port);
    if (sockfd < 0) {
        fprintf(stderr, "Error connecting to proxy server %s:%s\n", proxy_host, proxy_port);
        return -1;
    }

    // Send HTTP GET request to the proxy server
    snprintf(buf, MAX_LINE, "GET %s HTTP/1.0\r\n\r\n", url);
    send(sockfd, buf, strlen(buf), 0);

    // Receive and print the response from the proxy server
    while (1) {
        if ((ret = readready(sockfd)) < 0) break;
        if (ret > 0) {
            if (readline(sockfd, buf, MAX_LINE) <= 0) break;
            fputs(buf, stdout);
        }
    }
    printf("\n---------------------------\n");

    // while (1) {
    //     int ret = readready(sockfd);
    //     if (ret < 0) break;
    //     if (ret > 0) {
    //         if (readline(sockfd, buf, MAX_LINE) <= 0) break;
    //         send(client_sockfd, buf, strlen(buf), 0);
    //     }
    // }

    close(sockfd);
    return 0;
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

            // Prompt the user to enter a URL
            char url[MAX_URL_LEN];
            printf("Enter URL: ");
            fgets(url, MAX_URL_LEN, stdin);
            strtok(url, "\n"); // Remove the newline character
            http_get(url, "172.30.148.62", "8080");

            close(sockfd); /* close original socket */
			do_main(newsockfd);//通知開啟proxy server(webserv.c)
            exit(0);
        }

        close(newsockfd); /* parent process */
    }
}

//http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html
