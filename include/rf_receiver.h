#ifndef RF_RECEIVER_H_
#define RF_RECEIVER_H_

#include "../libs/collections/include/lbq.h"

#define MAX_SILENCE_TIME 90000

typedef struct RfReceiverArgs {
	LinkedBlockingQueue *downQueue;
} RfReceiverArgs;

void RfReceiver_thread(RfReceiverArgs *args);

#endif
