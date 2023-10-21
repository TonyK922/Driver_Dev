#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    int  fd;
    char status = 0;
    char val;

    /* /ledtest /dev/myled on/off */
    if (argc != 3) {
        printf("Usage:%s <dev> <on|off>\n", argv[0]);
        printf("e.g.%s /dev/myled on\n", argv[0]);
        printf("e.g.%s /dev/myled off\n", argv[0]);
        printf("e.g.%s /dev/myled read\n", argv[0]);
        exit(1);
    }

    /* open */
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("Error opening\n");
        return -1;
    }

    /* write */
    if (strcmp(argv[2], "read") == 0) {
        read(fd, &val, 1);
        printf("read val = %d %s\n", val, val ? "off" : "on");
    }
    else {
        if (strcmp(argv[2], "on") == 0) {
            status = 1;
        }
        else if (strcmp(argv[2], "off") == 0) {
            status = 0;
        }
        write(fd, &status, 1);
    }
    return 0;
}