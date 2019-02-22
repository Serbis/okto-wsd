#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "include/rf.h"
#include "include/global.h"
#include "include/logger.h"
#include "libs/collections/include/lbq.h"

#ifdef TARGET_HW
	#include "include/NRF24.h"
	#include <wiringPi.h>
#endif

LinkedBlockingQueue *rfTransmitQueues[5];


/** Transmit data to air
 *
 * @param data payload for transmition
 * @param size size of palyload
 */
bool RF_transmit(uint8_t *data, uint16_t size) {
	LinkedBlockingQueue *downQueue = rfTransmitQueues[0];

	uint16_t frags = ceil((double)size / 32);
	Logger_debug("RF_transmit", "Prepare payload of %d fragments", frags);

	for (uint16_t i = 0; i < frags; i++) {
			uint16_t start = i * 32;
			uint8_t len = 32;
			if (start + 32 > size)
				len = size - i * 32;

			Logger_debug("RF_transmit", "Push for transmit payload feragment %d..%d", start, start + len);
			uint8_t *payload = (uint8_t*) malloc(32);
			memcpy(payload, data + start, len);
			downQueue->enqueue(downQueue, payload);
			//NRF24_Send(payload);
		}

	return true;
}

/** Initilize harware */
int RF_init() {
	LinkedBlockingQueue *p1_transmitQueue = new_LQB(100);
	LinkedBlockingQueue *p2_transmitQueue = new_LQB(100);
	LinkedBlockingQueue *p3_transmitQueue = new_LQB(100);
	LinkedBlockingQueue *p4_transmitQueue = new_LQB(100);
	LinkedBlockingQueue *p5_transmitQueue = new_LQB(100);
	rfTransmitQueues[0] = p1_transmitQueue;
	rfTransmitQueues[1] = p2_transmitQueue;
	rfTransmitQueues[2] = p3_transmitQueue;
	rfTransmitQueues[3] = p4_transmitQueue;
	rfTransmitQueues[4] = p5_transmitQueue;

	#ifdef TARGET_HW

	wiringPiSetup ();
	pinMode (21, OUTPUT);
	pinMode (22, INPUT);

    struct SpiDescriptor *spidev = SPI_open("/dev/spidev0.0", 0, 8, 500000);

    if (spidev->error < 0) {
    	Logger_error("RF_init", "Unable to open spidev, error code %d", spidev->fd);
	    return 1;
	}

	NRF_spid = spidev;
	NRF_cePin = 21;

	NRF24_init();
	int checkr = 0;// NRF24_init_check();

	if (checkr != 0) {
		Logger_error("RF_init", "Unable to init nrf24, error code %d", checkr);
		return 2;
	}

	#endif

	return 0;
}

int RF_Receive(uint8_t *payload) {
	#ifdef TARGET_HW

	//Logger_debug("RF_Receive", "Follow to receive mode");

    //return NRF24_Receive(payload);

	#endif

    while (true) { sleep(1); }

	return 0;
}
