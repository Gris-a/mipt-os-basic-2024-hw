#include "storage.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

int mk_dir(char *path) {
    char *read_ptr      = path;
    bool end_of_path = (*read_ptr == '\0');

    while(!end_of_path) {
        read_ptr++;

        while((*read_ptr != '/') && (*read_ptr !='\0')) read_ptr++;
        if(*read_ptr == '\0') end_of_path = true;


        int status = 0;
        if(end_of_path) {
            int fd = openat(AT_FDCWD, path, O_CREAT | O_RDWR, 0755);
            close(fd);
            status = (fd == -1);
        }
        else {
            *read_ptr = '\0';
            status = mkdir(path, 0755);
            *read_ptr = '/';
        }

        if((status != 0) && ((errno != EEXIST) || end_of_path)) return -1;
        errno = 0;
    }

    return 0;
}


void storage_init(storage_t* storage, const char* root_path) {
    storage->root_path = (char *)calloc(strlen(root_path) + 1, sizeof(char));
    strcpy(storage->root_path, root_path);
}

void storage_destroy(storage_t* storage) {
    free(storage->root_path);
}

version_t storage_set(storage_t* storage, storage_key_t key, storage_value_t value) {
    size_t key_len = strlen(key);
    bool no_name   = ((key_len % SUBDIR_NAME_SIZE) == 0);

    size_t dir_name_len = key_len + key_len / SUBDIR_NAME_SIZE + no_name;
    char *dir_name      = (char *)calloc(dir_name_len + 1, sizeof(char));

    size_t key_offset = 0;
    for(size_t i = 0; key_offset < key_len; i++) {
        for(size_t j = 0; (j < SUBDIR_NAME_SIZE) && (key_offset < key_len); j++, i++, key_offset++) {
            dir_name[i] = key[key_offset];
        }
        dir_name[i] = '/';
    }

    if(no_name) dir_name[dir_name_len - 1] = 'a';
    dir_name[dir_name_len] = '\0';

    bool exists = true;
    int file = open(dir_name, O_RDWR);
    if(file == -1) {
        errno = 0;
        exists = false;

        mk_dir(dir_name);
        file = open(dir_name, O_RDWR);
    }

    version_t version = 1;
    if(exists) {
        lseek(file, -sizeof(version), SEEK_END);
        read(file, &version, sizeof(version));
        version++;
    }

    size_t value_len = strlen(value) + 1;

    write(file, value, value_len);
    write(file, &value_len, sizeof(value_len));
    write(file, &version, sizeof(version));

    free(dir_name);
    close(file);
    return version;
}

version_t storage_get(storage_t* storage, storage_key_t key, returned_value_t returned_value) {
    size_t key_len = strlen(key);

    bool no_name   = ((key_len % SUBDIR_NAME_SIZE) == 0);

    size_t dir_name_len = key_len + key_len / SUBDIR_NAME_SIZE + no_name;
    char *dir_name      = (char *)calloc(dir_name_len + 1, sizeof(char));

    size_t key_offset = 0;
    for(size_t i = 0; key_offset < key_len; i++) {
        for(size_t j = 0; (j < SUBDIR_NAME_SIZE) && (key_offset < key_len); j++, i++, key_offset++) {
            dir_name[i] = key[key_offset];
        }
        dir_name[i] = '/';
    }

    if(no_name) dir_name[dir_name_len - 1] = 'a';
    dir_name[dir_name_len] = '\0';

    int file = open(dir_name, O_RDWR);
    free(dir_name);
    if(file == -1) return 0;

    version_t version = 0;
    lseek(file, -sizeof(version), SEEK_END);
    read(file, &version, sizeof(version));

    size_t value_len = 0;
    lseek(file, -sizeof(version) - sizeof(value_len), SEEK_END);
    read(file, &value_len, sizeof(value_len));


    lseek(file, -sizeof(version) - sizeof(value_len) - value_len, SEEK_END);
    read(file, returned_value, value_len);

    close(file);
    return version;
}

version_t storage_get_by_version(storage_t* storage, storage_key_t key, version_t version, returned_value_t returned_value) {
    size_t key_len = strlen(key);
    bool no_name   = ((key_len % SUBDIR_NAME_SIZE) == 0);

    size_t dir_name_len = key_len + key_len / SUBDIR_NAME_SIZE + no_name;
    char *dir_name      = (char *)calloc(dir_name_len + 1, sizeof(char));

    size_t key_offset = 0;
    for(size_t i = 0; key_offset < key_len; i++) {
        for(size_t j = 0; (j < SUBDIR_NAME_SIZE) && (key_offset < key_len); j++, i++, key_offset++) {
            dir_name[i] = key[key_offset];
        }
        dir_name[i] = '/';
    }

    if(no_name) dir_name[dir_name_len - 1] = 'a';
    dir_name[dir_name_len] = '\0';

    int file = open(dir_name, O_RDWR);
    if(file == -1) return 0;
    free(dir_name);

    version_t cur_version = 0;
    size_t value_len      = 0;

    off_t offset = lseek(file, 0, SEEK_END);

    while(cur_version != version)
    {
        lseek(file, offset - sizeof(cur_version), SEEK_SET);
        read(file, &cur_version, sizeof(cur_version));

        lseek(file, offset - sizeof(cur_version) - sizeof(value_len), SEEK_SET);
        read(file, &value_len, sizeof(value_len));

        offset -= sizeof(cur_version) + sizeof(value_len) + value_len;
        lseek(file, offset, SEEK_SET);
    }

    read(file, returned_value, value_len);

    close(file);
    return version;
}
