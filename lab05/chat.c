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
    time_t rand_t;
    srand((unsigned) time(&rand_t));
    signal(SIGPIPE, SIG_IGN);
    int sockfd, connfd, listenfd, maxfd, maxi;
    int nready, client_list[FD_SETSIZE];
    char client_name[FD_SETSIZE][20];
    char client_from[FD_SETSIZE][100];
    int online = 0;
    fd_set rset, allset;
    socklen_t len;
    struct sockaddr_in servaddr, client;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) printf("setsockopt(SO_REUSEADDR) failed");
    int PORT = atol(argv[1]);
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

    // main loo p
    while (1) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char cur_time[1000];
        snprintf(cur_time, sizeof(cur_time), "%d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        cur_time[strlen(cur_time) - 1] = '\0';
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
            // welcome
            char welcome_msg[] = " *** Welcome to the simple CHAT server\n";
            write(connfd, welcome_msg, strlen(welcome_msg));
            // name
            int id = rand() % 10000;
            snprintf(client_name[i], sizeof(client_name[i]), "%d", id);
            char name_msg[2000];
            online++;
            write(connfd, cur_time, strlen(cur_time));
            snprintf(name_msg, sizeof(name_msg), " *** Total %d users online now. Your name is %d\n", online, id);
            write(connfd, name_msg, strlen(name_msg));
            // store from
            snprintf(client_from[i], sizeof(client_from[i]), "%s:%d", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
            // nofity all
            for (int j = 0; j < FD_SETSIZE; j++) {
                if (client_list[j] < 0) continue;
                if (i == j) continue;
                write(client_list[j], cur_time, strlen(cur_time));
                char land_msg[200];
                snprintf(land_msg, sizeof(land_msg), " *** User <%s> has just landed on the server\n", client_name[i]);
                write(client_list[j], land_msg, strlen(land_msg));
            }
            printf("* client connected from %s:%d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
            if (--nready <= 0) continue;
        }
        int M = 10000;
        int n;
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
                    online--;
                    for (int j = 0; j < FD_SETSIZE; j++) {
                        if (client_list[j] < 0) continue;
                        write(client_list[j], cur_time, strlen(cur_time));
                        char leave_msg[200];
                        snprintf(leave_msg, sizeof(leave_msg), " *** User <%s> has left the server\n", client_name[i]);
                        write(client_list[j], leave_msg, strlen(leave_msg));
                        printf("* client %s disconnected\n", client_from[i]);
                    }
                }
                else {
                    buff[strlen(buff) - 1] = '\0';
                    if (strcmp(buff, "/who") == 0) {
                        char bar[100] = "-------------------------------------------\n";
                        write(sockfd, bar, strlen(bar));
                        for (int j = 0; j < FD_SETSIZE; j++) {
                            char user_info[200];
                            if (client_list[j] < 0) continue;
                            if (i == j) snprintf(user_info, sizeof(user_info), "* %s\t%s\n", client_name[j], client_from[j]);
                            else snprintf(user_info, sizeof(user_info), "  %s\t%s\n", client_name[j], client_from[j]);

                            write(sockfd, user_info, strlen(user_info));
                        }
                        write(sockfd, bar, strlen(bar));
                    }
                    else if (buff[0] == '/' && buff[1] == 'n' && buff[2] == 'a' && buff[3] == 'm' && buff[4] == 'e') {
                        char old_name[100];
                        strcpy(old_name, client_name[i]);
                        strcpy(client_name[i], buff + 6);
                        write(sockfd, cur_time, strlen(cur_time));
                        char change_name_msg[100];
                        snprintf(change_name_msg, sizeof(change_name_msg), " *** Nickname changed to <%s>\n", client_name[i]);
                        write(sockfd, change_name_msg, strlen(change_name_msg));
                        for (int j = 0; j < FD_SETSIZE; j++) {
                            if (i == j) continue;
                            write(client_list[j], cur_time, strlen(cur_time));
                            char change_name_msg2[200];
                            snprintf(change_name_msg2, sizeof(change_name_msg2), " *** User <%s> renamed to <%s>\n", old_name, client_name[i]);
                            write(client_list[j], change_name_msg2, strlen(change_name_msg2));
                        }
                    }
                    else if (buff[0] == '/') {
                        write(sockfd, cur_time, strlen(cur_time));
                        char err_msg[200];
                        snprintf(err_msg, sizeof(err_msg), "*** Unknown or incomplete command <%s>\n", buff);
                        write(sockfd, err_msg, strlen(err_msg));
                    }
                    else {
                        for (int j = 0; j < FD_SETSIZE; j++) {
                            if (i == j) continue;
                            write(client_list[j], cur_time, strlen(cur_time));
                            char username[100];
                            snprintf(username, sizeof(username), " <%s> ", client_name[i]);
                            write(client_list[j], username, strlen(username));
                            write(client_list[j], buff, sizeof(buff));
                        }
                    }
                }
                if (--nready <= 0) break;
            }
        }
    }
    close(listenfd);
    return 0;
}


