#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

extern int asprintf(char **restrict strp, const char *restrict fmt, ...);

int rmrf(const char *dir_name);

int main(int argc, char *argv[]) {
    bool is_r = false;

    int opt;
    while((opt = getopt(argc, argv, "r")) != -1) {
        switch(opt) {
            case 'r': {
                is_r = true;
                break;
            }
            default: return -1;
        }
    }

    if(optind == argc) return -1;
    for(; optind < argc; optind++) {
        char *path = argv[optind];

        if(is_r) {
            return rmrf(path);
        } else {
            struct stat file_stat;
            int stat_status = stat(path, &file_stat);
            if((stat_status != 0) || (S_ISDIR(file_stat.st_mode))) return -1;

            int remove_status = remove(path);
            if((remove_status != 0)) return -1;
        }
    }

    return 0;
}

int rmrf(const char *dir_name) {
    if(!dir_name) return 0;

    struct stat file_stat;
    int stat_status = stat(dir_name, &file_stat);
    if(stat_status != 0) return -1;

    if(S_ISREG(file_stat.st_mode)) {
        int remove_status = remove(dir_name);
        if((remove_status != 0)) return -1;

        return 0;
    }

    DIR *directory = opendir(dir_name);
    if(!directory) return -1;

    struct dirent *dir;
    while((dir = readdir(directory))) {
        if(*dir->d_name == '.') continue;

        char *path;
        asprintf(&path, "%s/%s", dir_name, dir->d_name);
        int status = rmrf(path);
        free(path);

        if(status != 0) {
            closedir(directory);
            return status;
        }
    }

    int close_status  = closedir(directory);
    int remove_status = remove(dir_name);
    if((close_status != 0) || (remove_status != 0)) return -1;

    return 0;
}