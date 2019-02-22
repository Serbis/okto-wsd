#ifndef INCLUDE_GATE_OUT_H_
#define INCLUDE_GATE_OUT_H_

#include <stdint.h>
#include <stdbool.h>
#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/map2.h"

typedef struct {
	uint32_t addr;
	uint8_t *data;
} GateOutQueueElement;

/** Thread args */
typedef struct {
	LinkedBlockingQueue *upQueue;
    LinkedBlockingQueue *soTransmitterQueue;
    Map *tidSoMap;
} GateOutThreadArgs;

void GateOut_thread(GateOutThreadArgs *args);

#endif
