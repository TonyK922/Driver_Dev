
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd;

/*
 * ./button_test /dev/100ask_button0
 *
 */
int main(int argc, char** argv)
{
    /*    int           val;
       struct pollfd fds[1];
       int           timeout_ms = 5000;
       int           ret;
       int           flags; */

    int buf[2];

    /* 1. 判断参数 */
    if (argc != 2) {
        printf("Usage: %s <dev>\n", argv[0]);
        return -1;
    }

    /* 2. 打开文件 */
    fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
        return -1;
    }

    while (1) {
        if (read(fd, &buf, 8) == 8)
            printf("read ds18b20: %d.%d\n", buf[0], buf[1]);
        else
            printf("read ds18b20: -1\n");

        sleep(2);
    }

    close(fd);

    return 0;
}
