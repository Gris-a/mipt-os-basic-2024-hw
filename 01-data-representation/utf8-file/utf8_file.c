#include <unistd.h>
#include <errno.h>

#include "utf8_file.h"

#define MAX_CODE_SZ 6
#define MAX_ONE_BYTE_CODE 0x7F

#define FORMAT_BYTE(byte) (byte & 0x3F) | 0x80
#define NEXT_BYTE(code) code >>= 6
#define FORMAT_LAST(byte, code_sz) (((1 << code_sz) - 1) << (8 - code_sz)) | byte

#define CODE_FIRST(code_sz, byte) byte & ((1 << (8 - code_sz)) - 1)
#define CODE_NEXT(code, byte) code = (code << 6) | (byte & 0x3F)

int utf8_write(utf8_file_t* f, const uint32_t* str, size_t count) {
    for(size_t i = 0; i < count; i++) {
        uint32_t code = str[i];
        if(code <= MAX_ONE_BYTE_CODE) {
            uint8_t byte = code;
            int out = write(f->fd, &byte, 1);

            if(out != 1) {
                errno = EIO;
                return -1;
            }

            continue;
        }

        int n_bytes = 0;
        uint8_t bytes[MAX_CODE_SZ] = {};

        do {
            bytes[n_bytes++] = FORMAT_BYTE(code);
            NEXT_BYTE(code);
        } while(code >= (1 << (6 - n_bytes)));

        n_bytes++;
        bytes[n_bytes - 1] = FORMAT_LAST(code, n_bytes);

        for(int i = n_bytes - 1; i >= 0; i--)
        {
            int out = write(f->fd, bytes + i, 1);

            if(out != 1) { // TODO
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

        uint32_t symbol = CODE_FIRST(n_bytes, first_byte);

        for(int j = 1; j < n_bytes; j++) {
            uint8_t byte;
            int out = read(f->fd, &byte, 1);

            if(out != 1) {
                errno = EIO;
                return -1;
            }
            CODE_NEXT(symbol, byte);
        }
        res[i] = symbol;
    }
    return count;
}

utf8_file_t* utf8_fromfd(int fd) {
    utf8_file_t *file = (utf8_file_t *)calloc(1, sizeof(utf8_file_t));
    if(!file)
    {
        errno = ENOMEM;
        return NULL;
    }

    file->fd = fd;
    return file;
}