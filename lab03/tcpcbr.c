#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define SA struct sockaddr

static struct timeval _t0;
static long long bytesent = 0;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

void handler(int s) {
	struct timeval _t1;
	double t0, t1;
	gettimeofday(&_t1, NULL);
	t0 = tv2s(&_t0);
	t1 = tv2s(&_t1);
	fprintf(stderr, "\n%lu.%06lu %llu bytes sent in %.6fs (%.6f Mbps; %.6f MBps)\n",
			_t1.tv_sec, _t1.tv_usec, bytesent, t1-t0, 8.0*(bytesent/1000000.0)/(t1-t0), (bytesent/1000000.0)/(t1-t0));
	exit(0);
}

int main(int argc, char *argv[]) {

	signal(SIGINT,  handler);
	signal(SIGTERM, handler);

    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("140.113.213.213");
    servaddr.sin_port = htons(10003);
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) printf("Failed\n");
    else printf("OK.\n");
    double mbps;
    if (argc == 1) mbps = 1;
    else mbps = atof(argv[1]);
    char buff[10000] = {0};
    long long desired = 0;
    struct timeval cur_time;
	gettimeofday(&_t0, NULL);
    long int start = _t0.tv_sec * 1000 + _t0.tv_usec / 1000;
    while (1) {
        gettimeofday(&cur_time, NULL);
        long int diff = cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000 - start;
        /*printf("%ld\n", diff);*/
        desired = 948 * mbps * diff;
        /*printf("%ld %ld\n", bytesent, desired);*/
        long long need = desired - bytesent;
        /*printf("sent: %lld\n", need);*/
        bytesent += need;
        write(sockfd, buff, need);
        struct timespec t = { 0, 1 };
        nanosleep(&t, NULL);
    }
    close(sockfd);

    return 0;
}
