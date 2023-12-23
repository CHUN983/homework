#include <signal.h>
#include "utility.h"
#define MAX_URL_LEN 1000

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

int request_proxy_server(char *client_host, char *client_port){
    int sockfd, len, ret;
    char buf[MAX_LINE];

    char url[MAX_URL_LEN];
    printf("Enter URL: ");
    fgets(url, MAX_URL_LEN, stdin);
    strtok(url, "\n"); // Remove the newline character

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

    // Send HTTP GET request
    sockfd = tcp_open_client(client_host, client_port);
    if (sockfd < 0) {
        fprintf(stderr, "Error opening client\n");
        return -1;
    }
    snprintf(buf, MAX_LINE, "GET /%s HTTP/1.0\r\n\r\n", path);
    send(sockfd, buf, strlen(buf), 0);


    int proxy_sockfd;
    proxy_sockfd= tcp_open_client(host, port);
    send(proxy_sockfd, buf, strlen(buf), 0);
    



    while (1) {
        if ((ret = readready(proxy_sockfd)) < 0) break;
        if (ret > 0) {
            if (readline(proxy_sockfd, buf, MAX_LINE) <= 0) break;
            fputs(buf, stdout);
        }
    }

    close(proxy_sockfd);
    close(sockfd);
    return 0;

}


int main(int argc, char *argv[]) {
	int sockfd, newsockfd, clilen, childpid;
	struct sockaddr_in cli_addr;

	sockfd = tcp_open_server(argv[1]);
	
	for( ; ; ) {
	/* Wait for a connection from a client process. (Concurrent Server)*/
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
		if(newsockfd < 0) exit(1); /* server: accept error */
		if((childpid  = fork()) < 0) exit(1); /* server: fork error */

			
		if(childpid == 0) {		/* child process	*/
			close(sockfd);		/* close original socket*/
			do_main(newsockfd);	/* process the request	*/
            request_proxy_server("172.30.148.62",argv[1]);
			exit(0);
		}

		close(newsockfd);		/* parent process	*/

	}
}
