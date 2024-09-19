#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
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
#include "irc.h"

#define SA struct sockaddr
#define ADDR "127.0.0.1"

int main(int argc, char* argv[]) {
    // declare
    int sockfd, connfd, listenfd, maxfd, maxi, nready;
    int client_list[FD_SETSIZE];
    fd_set rset, allset;
    socklen_t len;
    struct sockaddr_in servaddr, client;
    if (argc < 2) {
        printf("Usage: ./irc [PORT]");
        exit(1);
    }
    int PORT = atol(argv[1]);
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //Channel* channels[CHANNELS] = {NULL};
    //User* users[FD_SETSIZE] = {NULL};
    char nick[FD_SETSIZE][MAXLEN] = {0};
    int online = 0;
    // pre
    signal(SIGPIPE, SIG_IGN);
    int _flag;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &_flag, sizeof(int)) < 0) printf("setsockopt(SO_REUSEADDR) failed");
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
    //bzero(&servaddr, sizeof(servaddr));
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
            // ends here
            if (--nready <= 0) continue;
        }
        int n, M = 10000;
        char buff[M];
        bzero(buff, sizeof(buff));
        for (int i = 0; i <= maxi; i++) {
            if ((sockfd = client_list[i]) < 0) continue;
            if (FD_ISSET(sockfd, &rset)) {
                bzero(buff, sizeof(buff));
                if ((n = read(sockfd, buff, M)) == 0) {
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    for (int j = 0; j < CHANNELS; j++) if (channels[j] && on_channel(i, j)) leave_channel(i, j);
                    client_list[i] = -1;
                    users[i] = NULL;
                    nick[i][0] = 0;
                    online--;
                }
                else {
                    printf("received (%ld): %s", strlen(buff), buff);
                    if (buff[strlen(buff) - 2] == '\r') buff[strlen(buff) - 2] = 0;
                    else buff[strlen(buff) - 1] = 0;
                    //for (int j = 0; j < (int)strlen(buff); j++) if (buff[j] == '\r' || buff[j] == '\n') buff[j] = 0;
                    int n_of_arg = 0;
                    int cur = 0, prv = 0;
                    char command[20][100] = {0};
                    bool f = true;
                    while (cur < (int)strlen(buff)) {
                        if (buff[cur] == ':') {
                            cur++;
                            prv++;
                            f = false;
                            continue;
                        }
                        if (f && buff[cur] == ' ') {
                            cur++;
                            prv = cur;
                            n_of_arg++;
                        }
                        else {
                            command[n_of_arg][cur - prv] = buff[cur];
                            cur++;
                        }
                    }
                    n_of_arg++;
                    /*for (int k = 0; k < n_of_arg; k++) printf("%s!", command[k]);*/
                    char tmp[MAXLEN];
                    if (!users[i]) {
                        if (!strcmp(command[0], "NICK") == 0 && !strcmp(command[0], "USER") == 0) {
                            write(sockfd, ERR_NOTREGISTERED, strlen(ERR_NOTREGISTERED));
                            continue;
                        }
                    }
                    if (strcmp(command[0], "NICK") == 0) {
                        if (n_of_arg != 2) {
                            snprintf(tmp, MAXLEN, ERR_NONICKNAMEGIVEN, nick[i]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        bool f = true;
                        for (int j = 0; j < FD_SETSIZE; j++) {
                            if (users[j] && strcmp(users[j]->nickname, command[1]) == 0) {
                                snprintf(tmp, MAXLEN, ERR_NICKCOLLISION, nick[i], command[1]);
                                write(sockfd, tmp, strlen(tmp));
                                f = false;
                                break;
                            }
                        }
                        if (f) strcpy(nick[i], command[1]);
                    }
                    else if (strcmp(command[0], "USER") == 0) {
                        if (n_of_arg != 5) {
                            snprintf(tmp, MAXLEN, ERR_NEEDMOREPARAMS, nick[i], command[0]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        online++;
                        User* user = malloc(sizeof(User));
                        user->id = sockfd;
                        strcpy(user->nickname, command[1]);
                        strcpy(user->realname, command[4] + 1);
                        strcpy(user->host, inet_ntoa(client.sin_addr));
                        users[i] = user;
                        char tmp2[2000];
                        snprintf(tmp2, 2000, WELCOME_MSG, nick[i], nick[i], online, nick[i], nick[i], nick[i], nick[i], nick[i], nick[i], nick[i], nick[i], nick[i], nick[i], nick[i]);
                        write(sockfd, tmp2, strlen(tmp2));
                    }
                    else if (strcmp(command[0], "PING") == 0) {
                        snprintf(tmp, MAXLEN, "PONG\n");
                        write(sockfd, tmp, strlen(tmp));
                    }
                    else if (strcmp(command[0], "LIST") == 0) {
                        // list start
                        snprintf(tmp, MAXLEN, RPL_LISTSTART, nick[i]);
                        write(sockfd, tmp, strlen(tmp));
                        // list content
                        for (int j = 0; j < CHANNELS; j++) if (channels[j]) {
                            if (n_of_arg > 1 && strcmp(channels[j]->name, command[1])) continue;
                            snprintf(tmp, MAXLEN, RPL_LIST, nick[i], channels[j]->name, channels[j]->num, channels[j]->topic);
                            write(sockfd, tmp, strlen(tmp));
                        }
                        // end list
                        snprintf(tmp, MAXLEN, RPL_LISTEND, nick[i]);
                        write(sockfd, tmp, strlen(tmp));
                    }
                    else if (strcmp(command[0], "JOIN") == 0) {
                        if (n_of_arg == 1) {
                            snprintf(tmp, MAXLEN, ERR_NEEDMOREPARAMS, nick[i], command[0]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        int ch_i = find_channel(command[1]);
                        if (ch_i < 0) {
                            ch_i = create_channel(command[1]);
                            channels[ch_i]->people[channels[ch_i]->num] = users[i];
                            channels[ch_i]->num++;
                        }
                        else {
                            channels[ch_i]->people[channels[ch_i]->num] = users[i];
                            channels[ch_i]->num++;
                        }
                        snprintf(tmp, MAXLEN, JOIN_MSG, nick[i], channels[ch_i]->name);
                        for (int k = 0; k < channels[ch_i]->num; k++) write(channels[ch_i]->people[k]->id, tmp, strlen(tmp));
                        if (strlen(channels[ch_i]->topic) > 0) snprintf(tmp, MAXLEN, RPL_TOPIC, nick[i], channels[ch_i]->name, channels[ch_i]->topic);
                        else snprintf(tmp, MAXLEN, RPL_NOTOPIC, nick[i], channels[ch_i]->name);
                        write(sockfd, tmp, strlen(tmp));
                        char users_tmp[300] = "";
                        for (int j = 0; j < channels[ch_i]->num; j++) {
                            strcat(users_tmp, channels[ch_i]->people[j]->nickname);
                            strcat(users_tmp, " ");
                        }
                        snprintf(tmp, MAXLEN, RPL_NAMREPLY, nick[i], channels[ch_i]->name, users_tmp);
                        write(sockfd, tmp, strlen(tmp));
                        snprintf(tmp, MAXLEN, RPL_ENDOFNAMES, nick[i], channels[ch_i]->name);
                        write(sockfd, tmp, strlen(tmp));
                    }
                    else if (strcmp(command[0], "TOPIC") == 0) {
                        if (n_of_arg == 1) {
                            snprintf(tmp, MAXLEN, ERR_NEEDMOREPARAMS, nick[i], command[0]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        int ch_i = find_channel(command[1]);
                        if (ch_i < 0) {
                            snprintf(tmp, MAXLEN, ERR_NOSUCHCHANNEL, nick[i], command[1]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        if (!on_channel(i, ch_i)) {
                            snprintf(tmp, MAXLEN, ERR_NOTONCHANNEL, nick[i], channels[ch_i]->name);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        if (n_of_arg == 2) {
                            if (strlen(channels[ch_i]->topic) > 0) snprintf(tmp, MAXLEN, RPL_TOPIC, nick[i], channels[ch_i]->name, channels[ch_i]->topic);
                            else snprintf(tmp, MAXLEN, RPL_NOTOPIC, nick[i], channels[ch_i]->name);
                            write(sockfd, tmp, strlen(tmp));
                        }
                        else {
                            strcpy(channels[ch_i]->topic, command[2]);
                            snprintf(tmp, MAXLEN, RPL_TOPIC, nick[i], channels[ch_i]->name, channels[ch_i]->topic);
                            write(sockfd, tmp, strlen(tmp));
                        }
                    }
                    else if (strcmp(command[0], "NAMES") == 0) {
                        // for all names on all channels
                        for (int j = 0; j < CHANNELS; j++) if (channels[j]) {
                            char users_tmp[300] = "";
                            for (int k = 0; k < channels[j]->num; k++) {
                                strcat(users_tmp, channels[j]->people[k]->nickname);
                                strcat(users_tmp, " ");
                            }
                            snprintf(tmp, MAXLEN, RPL_NAMREPLY, nick[i], channels[j]->name, users_tmp);
                            write(sockfd, tmp, strlen(tmp));
                            snprintf(tmp, MAXLEN, RPL_ENDOFNAMES, nick[i], channels[j]->name);
                            write(sockfd, tmp, strlen(tmp));
                        }

                    }
                    else if (strcmp(command[0], "PART") == 0) {
                        if (n_of_arg == 1) {
                            snprintf(tmp, MAXLEN, ERR_NEEDMOREPARAMS, nick[i], command[0]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        int ch_i = find_channel(command[1]);
                        if (ch_i < 0) {
                            snprintf(tmp, MAXLEN, ERR_NOSUCHCHANNEL, nick[i], command[0]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        if (!on_channel(i, ch_i)) {
                            snprintf(tmp, MAXLEN, ERR_NOTONCHANNEL, nick[i], channels[ch_i]->name);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        leave_channel(i, ch_i);
                        snprintf(tmp, MAXLEN, PART_MSG, nick[i], channels[ch_i]->name);
                        for (int k = 0; k < channels[ch_i]->num; k++) write(channels[ch_i]->people[k]->id, tmp, strlen(tmp));
                    }
                    else if (strcmp(command[0], "USERS") == 0) {
                        snprintf(tmp, MAXLEN, RPL_USERSSTART, nick[i]);
                        write(sockfd, tmp, strlen(tmp));
                        for (int j = 0; j < FD_SETSIZE; j++) if (users[j]) {
                            snprintf(tmp, MAXLEN, RPL_USERS, nick[i], users[j]->nickname, users[j]->host);
                            write(sockfd, tmp, strlen(tmp));
                        }
                        snprintf(tmp, MAXLEN, RPL_ENDOFUSERS, nick[i]);
                        write(sockfd, tmp, strlen(tmp));
                    }
                    else if (strcmp(command[0], "PRIVMSG") == 0) {
                        if (n_of_arg == 1) {
                            snprintf(tmp, MAXLEN, ERR_NORECIPIENT, nick[i], command[0]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        if (n_of_arg == 2) {
                            snprintf(tmp, MAXLEN, ERR_NOTEXTTOSEND, nick[i]);
                            write(sockfd, tmp, strlen(tmp));
                            continue;
                        }
                        int ch_i = find_channel(command[1]);
                        if (ch_i < 0) {
                            snprintf(tmp, MAXLEN, ERR_NOSUCHNICK, nick[i], command[1]);
                            write(sockfd, tmp, strlen(tmp));
                        }
                        else {
                            snprintf(tmp, MAXLEN, PRIVMSG, nick[i], channels[ch_i]->name, command[2]);
                            for (int k = 0; k < channels[ch_i]->num; k++) if (channels[ch_i]->people[k]->id != sockfd) write(channels[ch_i]->people[k]->id, tmp, strlen(tmp));
                        }
                    }
                    else if (strcmp(command[0], "QUIT") == 0) {
                        close(sockfd);
                        FD_CLR(sockfd, &allset);
                        for (int j = 0; j < CHANNELS; j++) if (channels[j] && on_channel(i, j)) leave_channel(i, j);
                        client_list[i] = -1;
                        users[i] = NULL;
                        nick[i][0] = 0;
                        online--;
                    }
                    else {
                        snprintf(tmp, MAXLEN, ERR_UNKNOWNCOMMAND, nick[i], command[0]);
                        write(sockfd, tmp, strlen(tmp));
                    }
                }
                if (--nready <= 0) break;
            }
        }
    }
    close(listenfd);
    return 0;
}


