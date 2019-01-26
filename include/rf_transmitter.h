#ifndef INCLUDE_RF_TRANSMITTER_H_
#define INCLUDE_RF_TRANSMITTER_H_

#include <stdint.h>
#include <time.h>
#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/map2.h"

typedef struct RfTransmitterQueueElem {
	uint32_t addr;
	uint32_t tid;
	uint16_t size;
	uint8_t *data;
} RfTransmitterQueueElem;

typedef struct PipeQueueElem {
	time_t ft;
	uint16_t tid;
	uint16_t size;
	uint8_t *data;
} PipeQueueElem;

typedef struct RfTransmitterArgs {
	LinkedBlockingQueue *upQueue;
	LinkedBlockingQueue *soTransmitterQueue;
} RfTransmitterArgs;

void RfTransmitter_thread(RfTransmitterArgs * args);

#endif
