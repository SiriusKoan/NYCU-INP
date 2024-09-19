#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct {
    uint32_t magic;
    int32_t  off_str;
    int32_t  off_dat;
    uint32_t n_files;
} __attribute((packed)) pako_header_t;

typedef struct {
    uint32_t filename_offset;
    uint32_t file_size;
    uint32_t content_offset;
    unsigned char checksum[8];
} __attribute((packed)) FILE_E;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Argument error.\n");
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    pako_header_t* hdr = malloc(sizeof(pako_header_t));
    read(fd, hdr, sizeof(pako_header_t));
    printf("size: %d\n", hdr->n_files);
    FILE_E* file = malloc(sizeof(FILE_E));
    for (int i = 0; i < hdr->n_files; i++) {
        lseek(fd, 16 + 20 * i, SEEK_SET);
        read(fd, file, sizeof(FILE_E));
        file->file_size = __builtin_bswap32(file->file_size);
        /*printf("%lx\n", f)le->checksum);*/

        lseek(fd, hdr->off_str + file->filename_offset, SEEK_SET);
        char filename[256];
        char ch[1];
        int cur = 0;
        while (1) {
            read(fd, ch, 1);
            if (!(*ch)) break;
            filename[cur] = *ch;
            cur++;
        }
        filename[cur] = '\0';
        printf("%s %d\n", filename, file->file_size);

        lseek(fd, hdr->off_dat + file->content_offset, SEEK_SET);
        char content[4000000];
        read(fd, content, file->file_size);

        int seg = file->file_size / 8;
        unsigned char checksum[8] = {0};
        // align to 8
        for (int i = 0; i < file->file_size; i++) {
            checksum[i % 8] ^= content[i];
        }
        bool f = true;
        for (int i = 0; i < 8; i++) {
            if (checksum[i] != file->checksum[7 - i]) f = false;
        }

        if (f) {
            char path[1000];
            strcpy(path, argv[2]);
            strcat(path, "/");
            strcat(path, filename);
            int fd_out = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            write(fd_out, content, file->file_size);
            close(fd_out);
        }
    }
    return 0;
}

