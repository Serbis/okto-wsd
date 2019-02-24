#include <unistd.h>
#include <string.h>
#include <time.h>
#include "include/rf_receiver.h"
#include "include/NRF24.h"
#include "include/logger.h"
#include "libs/collections/include/lbq.h"
#include "include/global.h"
#include "include/utils.h"
#include "include/gate_out.h"

#ifdef TARGET_HW

extern pthread_mutex_t *rf_mutex;

#endif



void RfReceiver_thread(RfReceiverArgs *args) {
	Logger_info("RfReceiver_thread", "Rf receiver was started");

	LinkedBlockingQueue *downQueue = args->downQueue;

	// Last incoming data timestamp
	time_t lastDataTs = time(NULL) * 1000;

	//int counter - 0;
	uint8_t p[32] = { 0x3B, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x61, 0x62, 0x63, 0x2D, 0x2D, 0x2D, 0x2D, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00 };

	int counter = 0;
	/*for (int i = 0; i < 1; i++) {
		uint8_t *payload = (uint8_t*) malloc(TX_PLOAD_WIDTH);
		for (int i = 0; i < 32; i++) {
			payload[i] = p[i];
		}
		counter++;
		downQueue->enqueue(downQueue, payload);
	}*/

	while(1) {
		if (counter > 5000) {
			uint8_t *payload = (uint8_t*) malloc(TX_PLOAD_WIDTH);
			for (int i = 0; i < 32; i++) {
				payload[i] = p[i];
			}

			GateOutQueueElement *elem = (GateOutQueueElement*) malloc(sizeof(GateOutQueueElement));
			elem->addr = 0x11111101;
			elem->data = payload;

			downQueue->enqueue(downQueue, elem);
			counter = 0;
		}

		#ifdef TARGET_HW

		pthread_mutex_lock(rf_mutex);

		time_t taw = time(NULL) * 1000;

		// If no data was received for the last x ms, assume that ntf24 is hung
		if (taw - lastDataTs > MAX_SILENCE_TIME) {
			Logger_info("RfReceiver_thread", "Chip hang reset");
			NRF24_init(true);
			lastDataTs = taw;
		}

		if (NRF24_available()) {
			uint8_t *payload = (uint8_t*) malloc(TX_PLOAD_WIDTH);
			uint32_t addr;
			NRF24_Receive(payload, &addr);

			#ifdef DEBUG
            char *bhex = sprintfhex(payload, 32);
            Logger_debug("RfReceiver_thread", "Received new rf packet [ %s]", bhex);
            free(bhex);
			#endif

            GateOutQueueElement *elem = (GateOutQueueElement*) malloc(sizeof(GateOutQueueElement));
            elem->addr = addr;
            elem->data = payload;

            lastDataTs = taw;

            downQueue->enqueue(downQueue, elem);
			//for (int i = 0; i < TX_PLOAD_WIDTH; i++) {
			//	RINGS_write(payload[i], ring);
			//}
			//free(payload);
		}
		pthread_mutex_unlock(rf_mutex);

		#endif


		counter = counter + 10;
		usleep(10000);
	}
}
