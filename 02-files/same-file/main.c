#include <linux/limits.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsiz);;

enum RET {
    IS_NONE,
    IS_LINK,
    IS_FILE,
};

enum RET next_link(char path[PATH_MAX]) {
    char next_link[PATH_MAX] = {};

    struct stat file_stat;
    lstat(path, &file_stat);
    if(errno == ENOENT) return IS_NONE;

    if(S_ISLNK(file_stat.st_mode)) {
        readlink(path, next_link, PATH_MAX);
        strncpy (path, next_link, PATH_MAX);

        return IS_LINK;
    }
    return IS_FILE;
}

enum RET check(char path[PATH_MAX]) {
    char path1[PATH_MAX] = {};
    char path2[PATH_MAX] = {};

    strncpy(path1, path, PATH_MAX - 1);
    strncpy(path2, path, PATH_MAX - 1);

    do {
        next_link(path1);
        next_link(path2);

        enum RET ret2 = next_link(path2);
        switch(ret2) {
            case IS_NONE: return IS_NONE;
            case IS_FILE: {
                strncpy(path, path2, PATH_MAX - 1);
                return IS_FILE;
            }
            default: break;
        }

    } while(strcmp(path1, path2) != 0);

    return IS_NONE;
}

bool is_same_file(const char* lhs_path, const char* rhs_path) {
    char lpath[PATH_MAX] = {};
    char rpath[PATH_MAX] = {};

    strncpy(lpath, lhs_path, PATH_MAX - 1);
    strncpy(rpath, rhs_path, PATH_MAX - 1);

    enum RET lret = check(lpath);
    enum RET rret = check(rpath);

    if((lret == IS_FILE) && (rret == IS_FILE)) {
        struct stat lstat;
        stat(lpath, &lstat);

        struct stat rstat;
        stat(rpath, &rstat);

        return (lstat.st_ino == rstat.st_ino);
    } else return false;
}

int main(int argc, const char* argv[]) {
    if(argc != 3) return -1;

    printf(is_same_file(argv[1], argv[2]) ? "yes\n" : "no\n");
    return 0;
}