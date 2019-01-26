#include <unistd.h>
#include <stdint.h>
#include "include/rf_transmitter.h"
#include "libs/collections/include/lbq.h"
#include "include/utils.h"
#include "include/logger.h"
#include "include/global.h"
#include "include/so_transmitter.h"
#include "include/wsd_packet.h"
#include "include/NRF24.h"
#include "math.h"

#ifdef TARGET_HW

#include "include/NRF24.h"

#endif

void RfTransmitter_CompleteError(uint32_t error, uint32_t tid, LinkedBlockingQueue *soTransmitterQueue) {
	//Send response to so_transmitter
	SoTransmitterQueueElem *sot = (SoTransmitterQueueElem*) malloc(sizeof(SoTransmitterQueueElem));
	sot->tid = tid;
	uint32_t etype = error;
	//Reverse etype to be from le
	etype = (etype & 0x00FF00FF) <<  8 | (etype & 0xFF00FF00) >>  8;
	etype = (etype & 0x0000FFFF) << 16 | (etype & 0xFFFF0000) >> 16;
	sot->type = WSD_TYPE_ERROR;
	sot->size = 4;
	uint32_t *errc = (uint32_t*) malloc(4);
	*errc = etype;
	sot->data = (uint8_t*) errc;

	soTransmitterQueue->enqueue(soTransmitterQueue, sot);
}

void RfTransmitter_thread(RfTransmitterArgs * args) {
	LinkedBlockingQueue *upQueue = args->upQueue;
	LinkedBlockingQueue *soTransmitterQueue = args->soTransmitterQueue;

	Logger_info("RfTransmitter_thread", "Rf transmitter started");

	LinkedBlockingQueue *rfTransmitQueues[5];
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

	while(1) {
		for (int i = 0; i < 5; i++) {
			LinkedBlockingQueue *q = rfTransmitQueues[i];
			while (q->size(q) > 0) {
				PipeQueueElem *elem = q->dequeue(q);

				#ifdef DEBUG
				char *bodyh = sprintfhex(elem->data, elem->size);
				Logger_debug("RfTransmitter_thread", "Transmit rf packet [ pipe=%d, data=%s] ", i + 1, bodyh);
				free(bodyh);
				#endif


				#ifdef TARGET_HW

				uint8_t retr = NRF24_Send(elem->data, i + 1) & 0xF;
				if (retr >= 15) {

					#ifdef DEBUG
					char *bodyh = sprintfhex(elem->data, elem->size);
					Logger_debug("RfTransmitter_thread", "Rf packet not transmitted [ pipe=%d, data=%s] ", i + 1, bodyh);
					free(bodyh);
					#endif

					RfTransmitter_CompleteError(ERROR_ADDR_UNREACHABLE, elem->tid, soTransmitterQueue);

				} else {

					#ifdef DEBUG
					char *bodyh = sprintfhex(elem->data, elem->size);
					Logger_debug("RfTransmitter_thread", "Rf packet transmitted [ pipe=%d, retr=%d, data=%s] ", i + 1, retr, bodyh);
					free(bodyh);
					#endif

					//uint8_t status = NRF24_ReadReg(STATUS);
					//Logger_debug("RfTransmitter_thread", "status=%d", status);
				}

				#endif

				free(elem->data);
				free(elem);
			}
		}

		if (upQueue->size(upQueue) > 0) {
			RfTransmitterQueueElem *elem = upQueue->dequeue(upQueue);
			int8_t pipe = 1;

			#ifdef TARGET_HW
			pipe = NRF24_GetPipeByAddress(elem->addr);
			#endif

			if (pipe != -1) {
				uint16_t frags = ceil((double) elem->size / 32);

				#ifdef DEBUG
				char *bodyh = sprintfhex(elem->data, elem->size);
				Logger_debug("RfTransmitter_thread", "Prepare payload fragments [ tid=%d, addr=%04X, pipe=%d, frags=%d, data=%s]", elem->tid, elem->addr, pipe, frags, bodyh);
				free(bodyh);
				#endif

				for (uint16_t i = 0; i < frags; i++) {
						uint16_t start = i * 32;
						uint8_t len = 32;
						if (start + 32 > elem->size)
							len =  elem->size - i * 32;

						#ifdef DEBUG
						char *bodyh = sprintfhex(elem->data, elem->size);
						Logger_debug("RfTransmitter_thread", "Push for transmit payload fragment [ tid=%d, addr=%04X, pipe=%d, frag=%d, start=%d, end=%d, data=%s]",  elem->tid, elem->addr, pipe, i + 1, start, start + len, bodyh);
						free(bodyh);
						#endif

						uint8_t *payload = (uint8_t*) malloc(32);
						memcpy(payload, elem->data + start, len);

						LinkedBlockingQueue *q = rfTransmitQueues[pipe - 1];
						PipeQueueElem *pqm = (PipeQueueElem*) malloc(sizeof(PipeQueueElem));
						pqm->ft = 0;
						pqm->tid = elem->tid;
						pqm->size = 32;
						pqm->data = payload;
						q->enqueue(q, pqm);
				}
			} else {
				RfTransmitter_CompleteError(ERROR_ADDR_NOT_DEFINED, elem->tid, soTransmitterQueue);

				#ifdef DEBUG
				char *bodyh = sprintfhex(elem->data, elem->size);
				Logger_info("RfTransmitter_thread", "Unable to transmit data because address is not defined [ tid=%d, addr=%04X, data=%s]",  elem->tid, elem->addr, bodyh);
				free(bodyh);
				#endif

				free(elem->data);
			}

			free(elem);
		}

		usleep(100000);
	}
}


