#ifndef INCLUDE_DATA_PROCESSOR_H_
#define INCLUDE_DATA_PROCESSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "../libs/collections/include/lbq.h"

#define QUEUE_SIZE 500

typedef struct QueueElement {
	uint16_t action;
	uint16_t size;
	uint8_t *data;
} QueueElement;

/** Thread args */
typedef struct DataProcessor_Args {
    LinkedBlockingQueue *cmdQueue;
    int sockfd;
    bool alive;
} DataProcessor_Args;

void DataProcessor_thread(void *args);

#endif
