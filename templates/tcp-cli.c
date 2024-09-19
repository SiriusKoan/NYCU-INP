#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SA struct sockaddr
#define PORT 30000
#define ADDR "127.0.0.1"

void run(int fd) {
    int N = 1000;
    char buff[N];
    bzero(buff, sizeof(buff));
    //read(fd, buff, sizeof(buff))
    //write(fd, buff, sizeof(buff));
}

int main() {
    int sockfd;
    //int PORT = atol(argv[2]);
    struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed.\n");
        exit(1);
    }
    else {
        printf("Socket successfully created.\n");
	}
    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ADDR);
    servaddr.sin_port = htons(PORT);
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connection failed.\n");
        exit(2);
    }
    else {
        printf("Connection established.\n");
	}
	run(sockfd);
	close(sockfd);
    return 0;
}


