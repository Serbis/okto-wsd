#ifndef INCLUDE_SO_TRANSMITTER_H_
#define INCLUDE_SO_TRANSMITTER_H_

#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/map2.h"

typedef struct SoTransmitterQueueElem {
	uint32_t tid;
	uint8_t type;
	uint16_t size;
	uint8_t *data;
} SoTransmitterQueueElem;

typedef struct SoTransmitterArgs {
	LinkedBlockingQueue *upQueue;
	Map *tidSoMap;
} SoTransmitterArgs;

void SoTransmitter_thread(SoTransmitterArgs * args);

#endif
