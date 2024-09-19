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
#include <sys/select.h>
#include <time.h>

#define SA struct sockaddr
#define ADDR "127.0.0.1"

bool prefix(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

int main(int argc, char* argv[]) {
    // declare
    int sockfd, connfd, listenfd, maxfd, maxi, nready;
    fd_set rset, allset;
    socklen_t len;
    struct sockaddr_in servaddr, client;
    int PORT = atol(argv[1]);
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // pre
    signal(SIGPIPE, SIG_IGN);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) printf("setsockopt(SO_REUSEADDR) failed");
    // socket creation
    if (listenfd == -1) {
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
	if (bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Socket bind failed.\n");
        exit(1);
    }
    else {
        printf("Socket successfully binded.\n");
	}
    // listen
    if (listen(listenfd, 5) != 0) {
        printf("Listen failed.\n");
        exit(1);
    }
    else {
        printf("Listening.\n");
    }
    // select
    maxfd = listenfd;
    maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++) {
        client_list[i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // main loop
    while (1) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(listenfd, &rset)) {
            len = sizeof(client);
            connfd = accept(listenfd, (SA*)&client, &len);
            int i;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client_list[i] < 0) {
                    client_list[i] = connfd;
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("Too many clients.\n");
                exit(1);
            }
            FD_SET(connfd, &allset);
            if (connfd > maxfd) maxfd = connfd;
            if (i > maxi) maxi = i;
            // check pass
            printf("* client connected from %s:%d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
            if (--nready <= 0) continue;
        }
        char buff[M];
        bzero(buff, sizeof(buff));
        for (int i = 0; i <= maxi; i++) {
            if ((sockfd = client_list[i]) < 0) continue;
            if (FD_ISSET(sockfd, &rset)) {
                bzero(buff, sizeof(buff));
                if ((n = read(sockfd, buff, M)) == 0) {
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client_list[i] = -1;
                }
                else {
                }
                if (--nready <= 0) break;
            }
        }
    }
    close(listenfd);
    return 0;
}


