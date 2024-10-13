#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    int fd = open(argv[1], O_RDONLY);

    size_t file_sz = 0;
    struct stat file_stat;

    size_t info_sz = 0;
    char *info = NULL;

    while(true) {
        stat(argv[1], &file_stat);

        if(file_stat.st_size != file_sz) {
            lseek(fd, file_sz, SEEK_SET);

            size_t data_sz = file_stat.st_size - file_sz;
            if(data_sz > info_sz) {
                info = reallocarray(info, (info_sz = data_sz) + 1, sizeof(char));
            }

            read(fd, info, data_sz);
            write(1, info, data_sz);

            file_sz = file_stat.st_size;
        }

    }
    return 0;
}