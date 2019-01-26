#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

#define WSD_PREAMBLE_SIZE 8
#define WSD_HEADER_SIZE 7 //Without preable
#define WSD_PREAMBLE 0x3FAAAAAAAAAAAAAA
#define WSD_PREAMBLE_R 0xAAAAAAAAAAAAAA3F //Reversed because driver is LE but packet is BE

#define WSD_TYPE_TRANSMIT 0
#define WSD_TYPE_RECEIVE 1
#define WSD_TYPE_ERROR 2
#define WSD_TYPE_SET_PIPES_MATRIX 3
#define WSD_TYPE_RESULT 4

#define ERROR_ADDR_UNREACHABLE 1000
#define ERROR_ADDR_NOT_DEFINED 1001
#define ERROR_BROKEN_PIPE_MATRIX 1002
#define ERROR_BAD_PIPE_MSB 1003
#define ERROR_CHIP_NOT_RESPOND 1004

typedef struct WsdPacket {
	uint64_t preamble;
	uint32_t tid;
	uint8_t type;
	uint16_t length;
	void *body;
} WsdPacket;

uint8_t* WsdPacket_toBinary(WsdPacket *packet, uint16_t *size);
void WsdPacket_parsePacketHeader(WsdPacket *packet, uint8_t *buffer, uint16_t size);

#endif
