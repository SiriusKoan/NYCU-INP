#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#define SA struct sockaddr

int sockfd;
int PORT = -1;
char ADDR[100] = {0};
char write_buff[100000];
pid_t childpids[10] = {0};

void* run() {
    /*printf("Child running\n");*/
    int sockfd_child = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_child == -1) printf("Child sock failed.\n");
    /*else printf("Child sock created.\n");*/
    struct sockaddr_in servaddr_child;
    bzero(&servaddr_child, sizeof(servaddr_child));
    servaddr_child.sin_family = AF_INET;
    servaddr_child.sin_addr.s_addr = inet_addr(ADDR);
    servaddr_child.sin_port = htons(PORT + 1);
    if (connect(sockfd_child, (SA*)&servaddr_child, sizeof(servaddr_child)) != 0) {
        printf("Child connect failed.\n");
        exit(1);
    }
    while (1) {
        write(sockfd_child, write_buff, sizeof(write_buff));
        sleep(0.2);
    }
}

void handle_term(int sig) {
    /*printf("killed\n");*/
    char buff[] = "/report\n";
    write(sockfd, buff, sizeof(buff));
    char result[10000];
    read(sockfd, result, sizeof(result));
    /*printf("final report: %s\n", result);*/
    printf("report: %s", result);
    exit(0);
}

int main(int argc, char* argv[]) {
    //int PORT = atol(argv[2]);
    struct sockaddr_in servaddr;
    memset(write_buff, '*', sizeof(write_buff));
    write_buff[sizeof(write_buff) - 1] = 0;
    signal(SIGTERM, handle_term);
    signal(SIGPIPE, SIG_IGN);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket 1 creation failed.\n");
        exit(1);
    }
    else {
        /*printf("Socket 1 successfully created.\n");*/
    }
    PORT = atol(argv[2]);
    strcpy(ADDR, argv[1]);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ADDR);
    servaddr.sin_port = htons(PORT);
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connection failed.\n");
        exit(2);
    }
    else {
        /*printf("Connection established.\n");*/
    }
    char buff[] = "/reset\n";
    char tmp[10000];
    write(sockfd, buff, sizeof(buff));
    read(sockfd, tmp, sizeof(tmp));
    int n_thread = 80;
    pthread_t t[n_thread];
    int input;
    for (int i = 0; i < n_thread; i++) {
        pthread_create(&t[i], NULL, run, (void*)&input);
    }
    for (int i = 0; i < n_thread; i++) pthread_join(t[i], NULL);
    close(sockfd);
    sleep(15);
    return 0;
}


