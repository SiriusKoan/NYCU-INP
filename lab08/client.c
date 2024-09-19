/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define err_quit(m) { perror(m); exit(-1); }
#define PARTITION 1200
#define SIZE 256
#define BUFFSIZE 2048

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

static struct sockaddr_in sin;
static int s = -1;
int cur = 0;
int n_file = 0, n_part = 0;
char files[1001][40000];

struct FILES {
    int n;
    char* seg[PARTITION];
};

bool do_send(int sig) {
    // number of files -> length 6
    // number of partition -> length 2
    char buff[BUFFSIZE];
    bool f = true;
    if (n_part * SIZE >= sizeof(files[n_file])) {
        snprintf(buff, BUFFSIZE, "%06d%02d", n_file, n_part);
        f = false;
    }
    else {
        char tmp[BUFFSIZE] = {0};
        printf("len: %d\n", strlen(files[n_file]) - n_part * SIZE);
        strncpy(tmp, files[n_file] + n_part * SIZE, MIN(SIZE, strlen(files[n_file]) - n_part * SIZE));
        /*for (int i = 0; i < n_part * SIZE; i++) tmp[i] = files[n_file][i + n_part * SIZE];*/
        snprintf(buff, BUFFSIZE, "%06d%02d%s", n_file, n_part, tmp);
        f = true;
    }
    /*snprintf(buff, BUFFSIZE, "%06d%02d%s", n_file, n_part, "test data");*/
    printf("send %s\n", buff);
    if (sendto(s, buff, sizeof(buff) + 8, 0, (struct sockaddr*) &sin, sizeof(sin)) < 0) perror("sendto error");
    return f;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		return -fprintf(stderr, "usage: %s <path of files> <number of files> <port> <ip>\n", argv[0]);
	}
    int N = atol(argv[2]);
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], NULL, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

    char tmp[100];
    for (int i = 0; i < N; i++) {
        snprintf(tmp, 100, "%s/%06d", argv[1], i);
        int fd = open(tmp, O_RDONLY);
        printf("fd: %d\n", fd);
        /*char* file = malloc(10000);*/
        printf("read res: %d\n", read(fd, files[i], sizeof(files[i])));
    }

	while(n_file < N) {
        /*
		int rlen;
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char buf[2048];
		
		if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("recvfrom");
			continue;
		}
        */
        bool f = do_send(SIGALRM);
        n_part++;
        if (!f || n_part >= PARTITION) {
            n_part = 0;
            n_file++;
        }
	}

	close(s);
}
