#include <stdio.h>
#include<sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>

#define SA struct sockaddr
#define ADDR "127.0.0.1"

void handle(int sig) {
    int stat;
    waitpid(-1, &stat, WNOHANG);
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("Usage: ./nkat [port] [command]");
        exit(1);
    }
    int PORT = atol(argv[1]);
    char* arg[argc];
    for (int i = 0; i < argc; i++) arg[i] = NULL;
    arg[0] = strdup(argv[2]);
    for (int i = 3; i < argc; i++) {
        arg[i - 2] = strdup(argv[i]);
        printf("%s\n", argv[i]);
    }
    signal(SIGCHLD, handle);
    int sockfd, connfd;
    socklen_t len;
    pid_t childpid;
    struct sockaddr_in servaddr, client;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed.\n");
        exit(1);
    }
    else printf("Socket successfully created.\n");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) == -1) {
        printf("Socket bind failed.\n");
        exit(1);
    }
    else printf("Socket successfully binded.\n");
    if (listen(sockfd, 5) != 0) {
        printf("Listen failed.\n");
        exit(1);
    }
    else printf("Listening.\n");
    while (1) {
        len = sizeof(client);
        connfd = accept(sockfd, (SA*)&client, &len);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            else printf("accept failed.\n");
        }
        else printf("Server accept the client.\n");
        if ((childpid = fork()) == 0) {
            printf("New connection from %s:%d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDIN_FILENO);

            int s = execvp(arg[0], arg);
            if (s < 0) fprintf(stderr, "Run failed.\n");
            close(sockfd);
            exit(0);
        }
    }
    close(connfd);
    return 0;
}


