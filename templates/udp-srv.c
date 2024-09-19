#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

int main(int argc, char* argv[]) {
    int MAXSIZE = 1000;
    int sockfd, PORT;
    if (argc > 1) PORT = atol(argv[2]);
    else PORT = 11111;
    char buf[MAXSIZE];
    struct sockaddr_in servaddr, cliaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Socket creation failed.\n");
        exit(1);
    }
    /*
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(rcv_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        printf("Set timeout option failed.\n");
    }
    */
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT);
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Bind failed.\n");
        exit(1);
    }
    int n;
    int clilen = sizeof(cliaddr);
    while (1) {
		n = recvfrom(sockfd, (char*)buf, MAXSIZE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &clilen);
        // do something
    }
    return 0;
}

