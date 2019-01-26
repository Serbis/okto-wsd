#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <unistd.h>
#include "include/spi.h"

struct SpiDescriptor* SPI_open(const char *device, int mode, int bpw, int speed) {
	int fd;

    struct SpiDescriptor *dev = (struct SpiDescriptor*) malloc(sizeof(struct SpiDescriptor));

	if ((fd = open (device, O_RDWR)) < 0) {
        dev->error = 1;
		return dev;
    }

 	if (ioctl (fd, SPI_IOC_WR_MODE, &mode)            < 0) {
        dev->error = 2;
		return dev;

    }
	         
	if (ioctl (fd, SPI_IOC_WR_BITS_PER_WORD, &bpw)	  < 0) {
        dev->error = 4;
		return dev;

    }
		
	if (ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)   < 0) {
        dev->error = 3;
		return dev;

    }
		

    dev->fd = fd;
    dev->mode = mode;
    dev->bpw = bpw;
    dev->speed = speed;
    dev->error = 0;

	return dev;
}

int SPI_transmit(struct SpiDescriptor *dev, uint8_t *data, int len) {
	  struct spi_ioc_transfer spi ;

      memset (&spi, 0, sizeof (spi)) ;
	   
	  spi.tx_buf        = (unsigned long) data;
	  spi.rx_buf        = (unsigned long) data;
	  spi.len           = len;
	  spi.delay_usecs   = dev->mode;
	  spi.speed_hz      = dev->speed;
	  spi.bits_per_word = dev->bpw;
	  
	  return ioctl (dev->fd, SPI_IOC_MESSAGE(1), &spi) ;
}

