#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MTU 1500

char* files[1005];

void read_files(char* path, int n) {
    for (int i = 0; i < n; i++) {
        char tmp[100];
        snprintf(tmp, sizeof(tmp), "%s/%06d", path, i);
        int fd = open(tmp, O_RDONLY);
        char* buf = malloc(sizeof(char) * 40000);
        read(fd, buf, sizeof(char) * 40000);
        files[i] = buf;
        close(fd);
    }
}

int main(int argc, char *argv[]){
    int sd;
    struct sockaddr_in addr;
    int n;
    char buffer[MTU];
    memset(buffer, 0, MTU);
    struct iphdr *ip = (struct iphdr *) buffer;

    if(argc != 4) {
        printf("usage: %s {files_location} {number of files} {addr}\n", argv[0]);
        exit(1);
    }

    int n_of_files = atol(argv[2]);
    read_files(argv[1], n_of_files);

    sd = socket(AF_INET, SOCK_RAW, 161);
    if(sd < 0) {
        perror("socket");
        exit(1);
    }

    int one = 1;
    const int *val = &one;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, val, sizeof(one)) < 0) perror("setsockopt 1 error");
    if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) perror("setsockopt 2 error");
    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, val, sizeof(one)) < 0) perror("setsockopt 3 error");
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[3]);
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 16; // low delay
    //ip->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr);
    ip->id = htons(54321);
    ip->ttl = 64;
    ip->protocol = 161;
    ip->saddr = inet_addr(argv[3]);
    ip->daddr = inet_addr(argv[3]);

    for (int i = 0; i < n_of_files; i++) {
        int cur = 0;
        while (cur < (int)strlen(files[i])) {
            char* data = buffer + sizeof(struct iphdr);
            snprintf(data, 1000, "%s", files[i] + cur);
            cur += 999;
            ip->tot_len = sizeof(struct iphdr) + strlen(data);
            n = sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n < 0) {
                perror("sendto");
                exit(1);
            }
            //printf("send data: file %d, ptr: %d, data: %s\n", i, cur, data);
            sleep(0.1);
        }
        char* data = buffer + sizeof(struct iphdr);
        snprintf(data, 1000, "end");
        ip->tot_len = sizeof(struct iphdr) + strlen(data);
        sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr*)&addr, sizeof(addr));
    }

    close(sd);
    sleep(2);
    return 0;
}
