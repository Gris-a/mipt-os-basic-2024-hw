#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "utf8_file.h"

#define MAX_CODE_SZ 6

#define ROTL_UINT32_T(num, k) ((num) << (k)) | ((num) >> (32 - (k)))
#define ROTR_UINT32_T(num, k) ((num) >> (k)) | ((num) << (32 - (k)))

#define FIRST_N_BITS(num, n) ((num) & ((1 << (n)) - 1))


int utf8_write(utf8_file_t* f, const uint32_t* str, size_t count) {
    for(size_t i = 0; i < count; i++) {
        uint32_t code = str[i];

        size_t n_bytes;
        uint8_t mask;

        if(code < 0x00000080) {
            n_bytes = 1;
            mask = 0x00;
        } else if(code < 0x00000800) {
            n_bytes = 2;
            mask = 0xC0;
        } else if(code < 0x00010000) {
            n_bytes = 3;
            mask = 0xE0;
        } else if(code < 0x00200000) {
            n_bytes = 4;
            mask = 0xF0;
        } else if(code < 0x04000000) {
            n_bytes = 5;
            mask = 0xF8;
        } else if(code < 0x80000000) {
            n_bytes = 6;
            mask = 0xFC;
        } else {
            return -1;
        }

        code = ROTR_UINT32_T(code, 6 * (n_bytes - 1));
        uint8_t first_byte = (n_bytes == 1) ? FIRST_N_BITS(code, 7) | mask
                                            : FIRST_N_BITS(code, (7 - n_bytes)) | mask;

        uint8_t bytes[MAX_CODE_SZ] = {first_byte};

        for(size_t i = 1; i < n_bytes; i++) {
            bytes[i] = FIRST_N_BITS(code = ROTL_UINT32_T(code, 6), 6) | 0x80;
        }

        for(int i = 0; i < n_bytes; i++)
        {
            int out = write(f->fd, bytes + i, 1);

            if(out != 1) {
                errno = EIO;
                return -1;
            }
        }
    }
    return count;
}

int utf8_read(utf8_file_t* f, uint32_t* res, size_t count) {
    for(size_t i = 0; i < count; i++) {
        uint8_t first_byte;
        int out = read(f->fd, &first_byte, 1);

        if(out == 0) {
            return i;
        } else if(out != 1) {
            errno = EIO;
            return -1;
        }

        int n_bytes = 0;
        for(uint8_t mask = 1 << 7; first_byte & mask; mask >>= 1) n_bytes++;
        n_bytes = (n_bytes == 0) ? 1 : n_bytes;

        if(n_bytes > 6) {
            errno = EIO;
            return -1;
        }

        uint32_t symbol = FIRST_N_BITS(first_byte, 8 - n_bytes);

        for(int j = 1; j < n_bytes; j++) {
            uint8_t byte;
            int out = read(f->fd, &byte, 1);

            if(out != 1) {
                errno = EIO;
                return -1;
            }

            symbol = (symbol << 6) | FIRST_N_BITS(byte, 6);
        }
        res[i] = symbol;
    }
    return count;
}

utf8_file_t* utf8_fromfd(int fd) {
    utf8_file_t *file = (utf8_file_t *)calloc(1, sizeof(utf8_file_t));

    if(!file) {
        errno = ENOMEM;
        return NULL;
    }

    file->fd = fd;
    return file;
}