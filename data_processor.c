#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/data_processor.h"
#include "include/rf.h"
#include "include/logger.h"

#define ACTION_TRANSMIT 0

void DataProcessor_thread(void *args) {
	DataProcessor_Args *params = (DataProcessor_Args*) args;

	LinkedBlockingQueue *cmdQueue = params->cmdQueue;
	while(params->alive) {
		if (cmdQueue->size(cmdQueue) > 0) {
			QueueElement *elem = cmdQueue->dequeue(cmdQueue);
			uint8_t action = elem->data[0];

			if (elem->action == ACTION_TRANSMIT) {
				Logger_debug("DataProcessor_thread", "Received ACTION_TRANSMIT with data ...");
				RF_transmit(elem->data, elem->size);
			} else {

			}

			free(elem->data);
			free(elem);
		}
	}
}
