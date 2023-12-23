// webget.c
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

int http_get(char *url) {
    int sockfd, len, ret;
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
    
    sockfd = tcp_open_client(host, port);
    if (sockfd < 0) {
        fprintf(stderr, "Error opening client\n");
        return -1;
    }

    // Send HTTP GET request
    snprintf(buf, MAX_LINE, "GET /%s HTTP/1.0\r\n\r\n", path);
    send(sockfd, buf, strlen(buf), 0);

    // Receive and print the response
    while (1) {
        if ((ret = readready(sockfd)) < 0) break;
        if (ret > 0) {
            if (readline(sockfd, buf, MAX_LINE) <= 0) break;
            fputs(buf, stdout);
        }
    }

    close(sockfd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return 1;
    }

    char *url = argv[1];

    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "Invalid URL. It should start with 'http://'\n");
        return 1;
    }

    return http_get(url);
}

//輸入http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html
