#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>


int main(void)
{
    int fd = open("a", O_CREAT | O_RDWR);
}