#ifndef INCLUDE_SO_RECEIVER_H_
#define INCLUDE_SO_RECEIVER_H_

#include "../libs/collections/include/lbq.h"
#include "../libs/collections/include/list.h"
#include "../libs/collections/include/map2.h"

#define RING_BUFFER_SIZE 65535

typedef struct SoReceiverThreadArgs {
	int socket;
	LinkedBlockingQueue *downQueue;
	Map *globSoMap;
} SoReceiverThreadArgs;

void SoReceiver_thread(SoReceiverThreadArgs *args);

#endif
