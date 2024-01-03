#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/spi/spidev.h>
#include <linux/types.h>

/* ./dac_app /dev/spidevB.D <val> */
int do_mag(int fd, unsigned v) {
  int status;
  unsigned val;
  unsigned char rx_buf[2];
  unsigned char tx_buf[2];

  struct spi_ioc_transfer xfer;

  // val = strtoul(argv[2], NULL, 0);
  val = v;
  val <<= 2;
  val &= 0xFFC;

  memset(rx_buf, 0, sizeof rx_buf);
  memset(rx_buf, 0, sizeof rx_buf);
  memset(&xfer, 0, sizeof xfer);

  tx_buf[0] = (val >> 8) & 0xFF;
  tx_buf[1] = val & 0xFF;

  xfer.len = 2;
  xfer.tx_buf = tx_buf;
  xfer.rx_buf = rx_buf;

  status = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
  if (status < 0) {
    printf("SPI_IOC_MESSAGE");
    return -1;
  }

  val = (rx_buf[0] << 8) | rx_buf[1];
  val >>= 2;
  printf("Pre val = %d\n", val);

  return 0;
}
int main(int argc, char **argv) {
  int fd;
  unsigned val;
  /*
      if (3 != argc) {
          printf("Usage: %s /dev/spidevB.D <val>\n", argv[0]);
          return 0;
      }
   */
  fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    printf("can not open %s\n", argv[1]);
    return -1;
  }
  for (val = 300; val <= 1100; val += 100) {
    do_mag(fd, val);
    usleep(500000);
  }

  return 0;
}
