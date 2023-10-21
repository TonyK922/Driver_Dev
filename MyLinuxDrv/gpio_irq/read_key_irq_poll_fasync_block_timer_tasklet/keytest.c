
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ./button_test /dev/100ask_button0
 *
 */

static int fd;

static void sig_func(int signo) {}

int main(int argc, char** argv)
{
    int flags;
    int val;
    // int timeout_ms = 5000;

    // struct pollfd fds[1];

    /* 1. 判断参数 */
    if (argc != 2) {
        printf("Usage: %s <dev>\n", argv[0]);
        return -1;
    }

    // signal(SIGIO, sig_func);

    /* 2. 打开文件 */
    fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        printf("can not open file %s\n", argv[1]);
        return -1;
    }

    for (int i = 0; i < 10; i++) {
        if (read(fd, &val, 4) == 4) {
            printf("get key : 0x%x\n", val);
        }
        else {
            printf("get button: error\n");
        }
    }

    // fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    // fds[0].fd     = fd;
    // fds[0].events = POLLIN;

    while (1) {
        /* 3. 读文件 */
        /*read(fd, &val, 4);
         ret = poll(fds, 1, timeout_ms);
         if ((1 == ret) && (fds[0].revents & POLLIN)) {
            for (int i = 0; i < 1; i++) {
                read(fds[i].fd, &val, 4);
                printf("get key : 0x%x\n", val);
            }
        }
        else if (0 == ret) {
            printf("Timeout\n");
        }
*/
        if (read(fd, &val, 4) == 4) {
            printf("get key : 0x%x\n", val);
        }
        else {
            printf("get button: error\n");
        }
    }

    close(fd);
    return 0;
}
