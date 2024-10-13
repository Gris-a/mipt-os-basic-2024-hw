#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    errno = 0;

    bool is_p = false;

    unsigned basic_mode = 0755;
    unsigned mode = basic_mode;

    const struct option long_opt[] = {
        (struct option){"mode", required_argument, NULL, 'm'},
        (struct option){NULL  , no_argument      , NULL, 0}
    };
    int index = 0;

    int opt;
    while((opt = getopt_long(argc, argv, "m:p", long_opt, &index)) != -1) {
        switch(opt) {
            case 'p': {
                is_p = true;
                break;
            }
            case 'm': {
                mode = strtol(optarg, NULL, 8);
                break;
            }
            default: return -1;
        }
    }

    if(optind == argc) return -1;
    for(; optind < argc; optind++) {
        char *path = argv[optind];

        if(is_p) {
            char *read_ptr   = path;
            bool end_of_path = (*read_ptr == '\0');

            while(!end_of_path) {
                read_ptr++;

                while((*read_ptr != '/') && (*read_ptr !='\0')) read_ptr++;
                if(*read_ptr == '\0') end_of_path = true;

                *read_ptr = '\0';

                int status = mkdir(path, end_of_path ? mode : basic_mode);
                if((status != 0) && ((errno != EEXIST) || end_of_path)) return -1;
                errno = 0;

                *read_ptr = '/';
            }
        } else {
            int status = mkdir(path, mode);
            if(status != 0) return -1;
        }
    }

    return 0;
}