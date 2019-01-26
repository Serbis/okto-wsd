#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

#define PREAMBLE_SIZE 8
#define HEADER_SIZE 4 //Without preable
#define PREAMBLE 0x3FAAAAAAAAAAAAAA

struct Packet {
	uint64_t preamble;
	uint16_t size;
	void *body;
};

#endif
