#ifndef INCLUDE_DATA_PROCESSOR_H_
#define INCLUDE_DATA_PROCESSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/map2.h"

#define QUEUE_SIZE 500

typedef struct GateInQueueElement {
	int socket;
	uint32_t tid;
	uint16_t action;
	uint16_t size;
	uint8_t *data;
} GateInQueueElement;

/** Thread args */
typedef struct GateInThreadArgs {
    LinkedBlockingQueue *upQueue;
    LinkedBlockingQueue *rfTransmitterQueue;
    LinkedBlockingQueue *soTransmitterQueue;
    Map *tidSoMap;
} GateInThreadArgs;

void GateIn_thread(GateInThreadArgs *args);

#endif
