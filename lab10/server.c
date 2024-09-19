#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MTU 1500

char files[1005][100000] = {0};

void write_files(char* path, int n) {
    for (int i = 0; i < n; i++) {
        char tmp[100];
        snprintf(tmp, sizeof(tmp), "%s/%06d", path, i);
        int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(fd, files[i], strlen(files[i]));
        close(fd);
    }
}

int main(int argc, char *argv[]){
    int sd;
    /*struct sockaddr_in rcvaddr;*/
    /*unsigned int len = sizeof(struct sockaddr);*/
    int n;
    char buffer[MTU];
    memset(buffer, 0, MTU);

    if(argc != 4) {
        printf("usage: %s {files_location} {number of files} {addr}\n", argv[0]);
        exit(1);
    }

    int n_of_files = atol(argv[2]);

    sd = socket(PF_INET, SOCK_RAW, 161);
    if(sd < 0) {
        perror("socket");
        exit(1);
    }
    
    for (int i = 0; i < n_of_files; i++) {
        while (1) {
            memset(buffer, 0, MTU);
            n = recv(sd, buffer, sizeof(buffer), 0);
            if (n < 0) {
                perror("receive error");
                continue;
            }
            /*printf("receive: %s\n", buffer + 20);*/
            if (strcmp(buffer + 20, "end") == 0) break;
            strcat(files[i], buffer + 20);
        }
    }
    write_files(argv[1], n_of_files);

    close(sd);
    return 0;
}
