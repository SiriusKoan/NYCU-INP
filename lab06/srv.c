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

int main(int argc, char* argv[]) {
    // declare
    int sockfd, sockfd2, connfd, connfd2, listenfd, listenfd2, maxfd, maxfd2, maxi, maxi2;
    fd_set rset, allset, rset2, allset2;
    socklen_t len;
    struct sockaddr_in servaddr, servaddr2, client, client2;
    struct timeval zero = {0, 0};
    int client_list[FD_SETSIZE], client_list2[FD_SETSIZE];
    if (argc < 2) {
        printf("Usage: ./srv [PORT]");
        exit(1);
    }
    int PORT = atol(argv[1]);
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
    // pre
    signal(SIGPIPE, SIG_IGN);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) printf("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(listenfd2, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) printf("setsockopt(SO_REUSEADDR) failed");
    // socket creation
    if (listenfd == -1 || listenfd2 == -1) {
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
    servaddr.sin_port = htons(PORT);
    bzero(&servaddr2, sizeof(servaddr2));
	servaddr2.sin_family = AF_INET;
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr2.sin_port = htons(PORT + 1);
    // bind
	if (bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Socket bind failed: port %d.\n", PORT);
        exit(1);
    }
    else {
        printf("Socket successfully binded: port %d.\n", PORT);
	}

	if (bind(listenfd2, (SA*)&servaddr2, sizeof(servaddr2)) != 0) {
        printf("Socket bind failed: port %d.\n", PORT + 1);
        exit(1);
    }
    else {
        printf("Socket successfully binded: port %d.\n", PORT + 1);
	}
    // listen
    if (listen(listenfd, 5) != 0) {
        printf("Listen 1 failed.\n");
        exit(1);
    }
    else {
        printf("1 Listening.\n");
    }
    if (listen(listenfd2, 5) != 0) {
        printf("Listen 2 failed.\n");
        exit(1);
    }
    else {
        printf("2 Listening.\n");
    }
    // select
    maxfd = listenfd;
    maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++) client_list[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    maxfd2 = listenfd2;
    maxi2 = -1;
    for (int i = 0; i < FD_SETSIZE; i++) client_list2[i] = -1;
    FD_ZERO(&allset2);
    FD_SET(listenfd2, &allset2);

    long long counter = 0;
    double start_time = 0;
    int cli_n = 0;
    // main loop
    while (1) {
/*printf("DEBUG line %d\n", __LINE__);*/
        rset = allset;
        rset2 = allset2;
        select(maxfd + 1, &rset, NULL, NULL, &zero);
        select(maxfd2 + 1, &rset2, NULL, NULL, &zero);
        int M = 100000;
        char buff[M];
        char write_buff[M];
        int n;
        if (FD_ISSET(listenfd2, &rset2)) {
            len = sizeof(client2);
            connfd2 = accept(listenfd2, (SA*)&client2, &len);
            int i;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client_list2[i] < 0) {
                    client_list2[i] = connfd2;
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("Too many clients.\n");
                exit(1);
            }
            FD_SET(connfd2, &allset2);
            if (connfd2 > maxfd2) maxfd2 = connfd2;
            if (i > maxi2) maxi2 = i;
            // check pass
            printf("* [2] client connected from %s:%d\n", inet_ntoa(client2.sin_addr), (int)ntohs(client2.sin_port));
            cli_n++;
        }
        bzero(buff, sizeof(buff));
        for (int i = 0; i <= maxi2; i++) {
            if ((sockfd2 = client_list2[i]) < 0) continue;
            if (FD_ISSET(sockfd2, &rset2)) {
                bzero(buff, sizeof(buff));
                if ((n = read(sockfd2, buff, M)) == 0) {
                    close(sockfd2);
                    FD_CLR(sockfd2, &allset2);
                    client_list2[i] = -1;
                    cli_n--;
                }
                else {
                    //printf("%lu data come in\n", strlen(buff));
                    counter += n;
                }
            }
        }

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
            printf("* [1] client connected from %s:%d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
        }
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
                    struct timespec ts;
                    timespec_get(&ts, TIME_UTC);
                    long int sec = ts.tv_sec, msec = ts.tv_nsec / 1000;
                    buff[strlen(buff) - 1] = '\0';
                    printf("received: %s\n", buff);
                    char timestamp[100];
                    snprintf(timestamp, 100, "%ld.%06ld", sec, msec);
                    if (strcmp(buff, "/reset") == 0) {
                        start_time = sec + msec / 1e6;
                        snprintf(write_buff, M, "%s RESET %lld\n", timestamp, counter);
                        counter = 0;
                    }
                    else if (strcmp(buff, "/ping") == 0) {
                        snprintf(write_buff, M, "%s PONG\n", timestamp);
                    }
                    else if (strcmp(buff, "/report") == 0) {
                        double elapsed_time = (sec + msec / 1e6) - start_time;
                        snprintf(write_buff, M, "%s REPORT %lfs %lfMbps\n", timestamp, elapsed_time, 8 * counter / 1e6 / elapsed_time);
                    }
                    else if (strcmp(buff, "/clients") == 0) {
                        snprintf(write_buff, M, "%s CLIENTS %d\n", timestamp, cli_n);
                    }
                    else continue;
                    write(sockfd, write_buff, strlen(write_buff));
                }
            }
        }

    }
    close(listenfd);
    close(listenfd2);
    return 0;
}

