#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_NAME "/sudoku.sock"
#define SA struct sockaddr *

bool is_placeable(int grid[9][9], int row, int col, int n) {
    for (int i = 0; i < 9; i++) if (grid[row][i] == n) return false;
    for (int i = 0; i < 9; i++) if (grid[i][col] == n) return false;
    int r = row - row % 3;
    int c = col - col % 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grid[r + i][j + c] == n) return false;
        }
    }
    return true;
}

bool solve(int grid[9][9], int row, int col) {
    if (row == 8 && col == 9) return true;
    if (col == 9) {
        row++;
        col = 0;
    }
    // already filled so go to next
    if (grid[row][col] > 0) return solve(grid, row, col + 1);
    for (int n = 1; n <= 9; n++) {
        if (is_placeable(grid, row, col, n)) {
            grid[row][col] = n;
            if (solve(grid, row, col + 1)) return true;
        }
        grid[row][col] = 0;
    }
    return false;
}

int main() {
    int BUF_SIZE = 200;
    int ret;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("Client socket fd = %d\n", fd);
	if (fd == -1) {
		printf("Socket create failed.\n");
		exit(1);
    }
	memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
    ret = connect(fd, (const SA) &addr, sizeof(struct sockaddr_un));
    if (ret < 0) {
        printf("Connection error.\n");
        exit(1);
    }
    
    // start interacting
    // request and get sudoku problem
    memset(buf, 0, BUF_SIZE);
    strcpy(buf, "S");
    ret = write(fd, buf, BUF_SIZE);
    ret = read(fd, buf, BUF_SIZE);
    printf("%s\n", buf + 4); // skip OK:
    // parse to int matrix
    int grid[9][9] = {0};
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (buf[i * 9 + j + 4] == '.') grid[i][j] = 0;
            else grid[i][j] = buf[i * 9 + j + 4] - '0';
        }
    }
    printf("-------\n");
    // write the answer
    solve(grid, 0, 0);
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            memset(buf, 0, BUF_SIZE);
            snprintf(buf, BUF_SIZE, "V %d %d %d", i, j, grid[i][j]);
            ret = write(fd, buf, BUF_SIZE);
            ret = read(fd, buf, BUF_SIZE);
        }
    }
    // check
    memset(buf, 0, BUF_SIZE);
    buf[0] = 'C';
    buf[1] = 0;
    ret = write(fd, buf, BUF_SIZE);
    ret = read(fd, buf, BUF_SIZE);
    printf("%s\n", buf);
    return 0;
}

