#include <unistd.h>
#include <string.h>
#include "include/rf_receiver.h"
#include "include/NRF24.h"
#include "include/logger.h"
#include "libs/collections/include/lbq.h"
#include "include/global.h"
#include "include/utils.h"

#ifdef TARGET_HW

extern pthread_mutex_t *rf_mutex;

#endif



void RfReceiver_thread(RfReceiverArgs *args) {
	Logger_info("RfReceiver_thread", "Rf receiver was started");

	LinkedBlockingQueue *downQueue = args->downQueue;

	/*uint8_t p[32] = { 0x3B, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x03, 0x04, 0x05, 0x06, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00 };

	int counter = 1;
	for (int i = 0; i < 200; i++) {
		uint8_t *payload = (uint8_t*) malloc(TX_PLOAD_WIDTH);
		for (int i = 0; i < 32; i++) {
			payload[i] = p[i];
		}
		printf("RECV=%d\n", counter);
		fflush(stdout);
		if (counter == 32)
			printf("x");
		counter++;
		downQueue->enqueue(downQueue, payload);
	}

	printf("DQSIZE=%d\n", downQueue->size(downQueue));
			fflush(stdout);*/

	while(1) {

		#ifdef TARGET_HW

		pthread_mutex_lock(rf_mutex);
		if (NRF24_available()) {
			uint8_t *payload = (uint8_t*) malloc(TX_PLOAD_WIDTH);
			NRF24_Receive(payload);

			#ifdef DEBUG
            char *bhex = sprintfhex(payload, 32);
            Logger_debug("RfReceiver_thread", "Received new rf packet [ %s]", bhex);
            free(bhex);
			#endif

            downQueue->enqueue(downQueue, payload);
			//for (int i = 0; i < TX_PLOAD_WIDTH; i++) {
			//	RINGS_write(payload[i], ring);
			//}
			//free(payload);
		}
		pthread_mutex_unlock(rf_mutex);

		#endif



		usleep(10000);
	}
}
