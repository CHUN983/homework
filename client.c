#include"utility.h"

int tcp_open_client(char *host, char *port) {
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

int main(int argc, char *argv[]) {
	int		sockfd, len, ret;
	char		buf[MAX_LINE];

	sockfd = tcp_open_client(argv[1], argv[2]);
	while(1) {
		if((ret = readready(sockfd)) < 0) break;
		if(ret > 0) {
			if(readline(sockfd, buf, MAX_LINE) <= 0) break;
			fputs(buf, stdout);
		}
		if((ret = readready(0)) < 0) break;
		if(ret > 0) {
			if(fgets(buf, MAX_LINE, stdin) == NULL) break;
			len = strlen(buf);
			send(sockfd, buf, len, 0);
			if(strncmp(buf, "QUIT", 4) == 0) break;
		}
	}
	close(sockfd);
	return 0;
}
