#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * ./button_test /dev/100ask_button0
 *
 */

#define BUFF_SIZE (8 * 1024)

static int fd;

// static void sig_func(int signo) {}

int main(int argc, char** argv)
{
    char* buf;
    int   len;
    char  buff[128];
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
    /* mmap */
    buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == buf) {
        printf("mmap failed\n");
        close(fd);
        return -1;
    }
    printf("buf mmap vaddr %p\n", buf);
    printf("buf data from drv: %s\n", buf); /* old */

    /* write data*/
    strcpy(buf, "hello,this is a test!\n");

    /* read & compare */
    read(fd, buff, 128); /* read old data */

    if (0 == strcmp(buf, buff))
        printf("the same string\n");
    else {
        printf("not the same string\n");
    }
    printf("context in buff arr: %s", buff);
    printf("context in buf mmap: %s", buf);

    while (1) {
        sleep(10);
    }

    munmap(buf, BUFF_SIZE);
    close(fd);
    return 0;
}
