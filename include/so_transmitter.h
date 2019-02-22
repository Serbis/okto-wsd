#ifndef INCLUDE_SO_TRANSMITTER_H_
#define INCLUDE_SO_TRANSMITTER_H_

#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/map2.h"
#include "../libs/collections/include/list.h"

typedef struct SoTransmitterQueueElem {
	uint32_t tid;
	uint8_t type;
	bool uniall; // Send this data to the all sockets
	uint16_t size;
	uint8_t *data;
} SoTransmitterQueueElem;

typedef struct SoTransmitterArgs {
	LinkedBlockingQueue *upQueue;
	Map *tidSoMap;
	Map *globSoMap;
} SoTransmitterArgs;

void SoTransmitter_thread(SoTransmitterArgs * args);

#endif
