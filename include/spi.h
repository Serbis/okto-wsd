#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

/** Scribe open spi device */
struct SpiDescriptor {
    int fd;
    int mode;
    int bpw;
    int speed;
    int error;
};

struct SpiDescriptor* SPI_open(const char *dev, int mode, int bpw, int speed);
int SPI_transmit(struct SpiDescriptor *dev, uint8_t *data, int len); 

#endif
