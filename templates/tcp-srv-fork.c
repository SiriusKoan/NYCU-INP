#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define SA struct sockaddr
#define PORT 30000
#define ADDR "127.0.0.1"

void handle_children(int sig) {
    int stat;
    waitpid(-1, &stat, WNOHANG);
}

void run(int fd) {
    int N = 1000;
    char buff[N];
    bzero(buff, sizeof(buff));
    //read(fd, buff, sizeof(buff))
    //write(fd, buff, sizeof(buff));
}

int main(int argc, char* argv[]) {
    // declare
    int sockfd, connfd;
    socklen_t len;
    pid_t childpid;
    struct sockaddr_in servaddr, client;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //int PORT = atol(argv[1]);
    // pre
    signal(SIGCHLD, handle_children);
    signal(SIGPIPE, SIG_IGN);
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) printf("setsockopt(SO_REUSEADDR) failed");
    // socket creation
    if (sockfd == -1) {
        printf("Socket creation failed.\n");
        exit(1);
    }
    else {
        printf("Socket successfully created.\n");
	}
    // server
    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //servaddr.sin_addr.s_addr = inet_addr(ADDR);
    servaddr.sin_port = htons(PORT);
    // bind
	if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Socket bind failed.\n");
        exit(1);
    }
    else {
        printf("Socket successfully binded.\n");
	}
    // listen
    if (listen(sockfd, 5) != 0) {
        printf("Listen failed.\n");
        exit(1);
    }
    else {
        printf("Listening.\n");
    }
    // main loop
    while (1) {
        len = sizeof(client);
        connfd = accept(sockfd, (SA*)&client, &len);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            printf("Server accept failed.\n");
            exit(1);
        }
        else {
            printf("Server accept the client.\n");
        }
        if ((childpid = fork()) == 0) {
            printf("New connection from %s:%d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
            run(connfd);
            close(connfd);
            exit(0);
        }
    }
    close(sockfd);
    return 0;
}


