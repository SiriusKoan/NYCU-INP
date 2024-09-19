#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define SA struct sockaddr

void run(int fd) {
    long long N = 400000;
    char buff[N];
    char tmp_buff[1000];
    write(fd, "GO\n", 3);
    sleep(1);
    bzero(tmp_buff, sizeof(tmp_buff));
    read(fd, tmp_buff, 107);
    //printf("%s\n", buff);
    long long sum = 0;
    /*
    bzero(buff, sizeof(buff));
    read(fd, buff, sizeof(buff));
    for (int i = 0; i < N; i++) printf("%c", buff[i]);
    */
    while (1) {
        bzero(buff, sizeof(buff));
        read(fd, buff, sizeof(buff));
        //printf("%s", buff);
        //for (int i = 0; i < N; i++) printf("%c", buff[i]);
        //printf("%lld\n", strstr(buff, "==== END DATA ====") - buff);
        long long diff = strstr(buff, "==== END DATA ====") - buff;
        //printf("%lld\n", sum);
        if (diff > N || diff < 0) sum += strlen(buff);
        else {
            sum += diff;
            break;
        }
    }
    sum -= 2;
    printf("%d\n", sum);
    bzero(buff, sizeof(buff));
    char tmp[30] = {0};
    snprintf(tmp, 30, "%d", sum);
    tmp[strlen(tmp)] = '\n';
    /*printf("%d\n", sizeof(tmp));*/
    write(fd, tmp, strlen(tmp));
    sleep(0.2);
    bzero(buff, sizeof(buff));
    read(fd, buff, sizeof(buff));
    printf("%s\n", buff);
    //printf("%s\n", buff);
    /*
    int cur = 0;
    while (sum) {
        int t = sum % 10;
        tmp[cur++] = t + 48;
        sum /= 10;
    }
    */
    //printf("\n\n%s\n", tmp);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
	else printf("socket created.\n");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("140.113.213.213");
    servaddr.sin_port = htons(10002);
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
	else printf("connection established.\n");
    run(sockfd);
    close(sockfd);
    return 0;
}


