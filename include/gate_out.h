#ifndef INCLUDE_GATE_OUT_H_
#define INCLUDE_GATE_OUT_H_

#include <stdint.h>
#include <stdbool.h>
#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/map2.h"

/** Thread args */
typedef struct GateOutThreadArgs {
	LinkedBlockingQueue *upQueue;
    LinkedBlockingQueue *soTransmitterQueue;
    Map *tidSoMap;
} GateOutThreadArgs;

void GateOut_thread(GateOutThreadArgs *args);

#endif
