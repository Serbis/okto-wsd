#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/gate_in.h"
#include "include/logger.h"
#include "include/exb_packet.h"
#include "include/rf_transmitter.h"
#include "include/so_transmitter.h"
#include "include/wsd_packet.h"
#include "include/global.h"
#include "include/utils.h"
#include "include/NRF24.h"

#ifdef TARGET_HW
extern pthread_mutex_t *rf_mutex;
#endif

void GateIn_completeError(uint32_t error, uint32_t tid, LinkedBlockingQueue *soTransmitterQueue) {
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

void GateIn_completeEmptyResult(uint32_t tid, LinkedBlockingQueue *soTransmitterQueue) {
	//Send response to so_transmitter
	SoTransmitterQueueElem *sot = (SoTransmitterQueueElem*) malloc(sizeof(SoTransmitterQueueElem));
	sot->tid = tid;
	sot->type = WSD_TYPE_RESULT;
	sot->size = 1;
	uint8_t *empty = (uint8_t*) malloc(1);
	sot->data = empty;

	soTransmitterQueue->enqueue(soTransmitterQueue, sot);
}

/** Handles driver commands. It is the main decision point. */
void GateIn_thread(GateInThreadArgs *args) {
	LinkedBlockingQueue *upQueue = args->upQueue;
	LinkedBlockingQueue *rfTransmitterQueue = args->rfTransmitterQueue;
	LinkedBlockingQueue *soTransmitterQueue = args->soTransmitterQueue;
	Logger_info("GateIn_thread", "Gate_in thread was started");

	while(1) {
		if (upQueue->size(upQueue) > 0) {
			GateInQueueElement *elem = upQueue->dequeue(upQueue);

			//Store tid-socket relation
			char *tidk = itoa2(elem->tid);
			int32_t *sockp = (int32_t*) malloc(4);
			*sockp = elem->socket;
			MAP_add(tidk, sockp, args->tidSoMap);
			free(tidk);

			/* Transmits data to the air. In the body of the operation, the first four bytes define the address of
			 * the front. After them should be a valid ExB package. Transaction ID is selected from the package.
			 * The transaction identifier is associated with the socket to send a response packet to it, if one is
			 * received. The address is used to specify the parameters in the rf transmitter.
			 */
			if (elem->action == WSD_TYPE_TRANSMIT) {
				uint32_t addr = 0;
				uint32_t tid = 0;

				memcpy(&addr, elem->data, 4);
				memcpy(&tid, elem->data + 4 + EXB_PREAMBLE_SIZE, 4);

				//Reverse addr to le from be
				addr = (addr & 0x00FF00FF) <<  8 | (addr & 0xFF00FF00) >>  8;
				addr = (addr & 0x0000FFFF) << 16 | (addr & 0xFFFF0000) >> 16;

				//Reverse tid to le from be
				tid = (tid & 0x00FF00FF) <<  8 | (tid & 0xFF00FF00) >>  8;
				tid = (tid & 0x0000FFFF) << 16 | (tid & 0xFFFF0000) >> 16;

				//Send data to rf transmition
				uint16_t sexb = elem->size - 4;
				uint8_t *bf = (uint8_t*) malloc(sexb);
				memcpy(bf, elem->data + 4, sexb);

				RfTransmitterQueueElem *el = (RfTransmitterQueueElem*) malloc(sizeof(RfTransmitterQueueElem));
				el->tid = tid;
				el->addr = addr;
				el->size = sexb;
				el->data = bf;

				#ifdef DEBUG
				char *bodyh = sprintfhex(bf, sexb);
				Logger_debug("GateIn_thread", "Processed TRANSMIT action [ addr=%04X, tid=%d, expb=%s]", addr, tid, bodyh);
				free(bodyh);
				#endif

				rfTransmitterQueue->enqueue(rfTransmitterQueue, el);

			/** Set nrf pipe matrix. Pipe matrix is a blob of the 40 bytes length, contains int values in BE
			 *  format. Each element represents an one addres in pipe sequense p0_targ, p0_self, p1_targ,
			 *  p1_self and etc. Addresses classified as targ, used for determine exb sender and loaded in
			 *  RX_ADDR_X registers. Addresses classified as self, used in tx mode for determine transmitter
			 *  address - what exb will be considered as a transmitter in rx mode. This action return RESULT packet if
			 *  action was succes or ERROR if dos not. Errors may be ERROR_BROKEN_PIPE_MATRIX - if pipe matrix
			 *  from packet less than 40 bytes. ERROR_BAD_PIPE_MSB - if addresses MBS (in BE) not all uniques.
			 *  ERROR_CHIP_NOT_RESPOND - if chip does not respond or does not set pipe registers.
			 *  */
			} else if (elem->action == WSD_TYPE_SET_PIPES_MATRIX) {
				// Check for bad matrix size
				if (elem->size < 40) {
					Logger_error("GateIn_thread", "Unable to process action WSD_TYPE_SET_PIPES_MATRIX because matrix is corrupted");

					GateIn_completeError(ERROR_BROKEN_PIPE_MATRIX, elem->tid, soTransmitterQueue);
					continue;
				}

				// Create pipe matrinx
				uint32_t p1_targ = 0;
				uint32_t p1_self = 0;
				uint32_t p2_targ = 0;
				uint32_t p2_self = 0;
				uint32_t p3_targ = 0;
				uint32_t p3_self = 0;
				uint32_t p4_targ = 0;
				uint32_t p4_self = 0;
				uint32_t p5_targ = 0;
				uint32_t p5_self = 0;

				// Copy data from the packet body
				memcpy(&p1_targ, elem->data, 4);
				memcpy(&p1_self, elem->data + 4, 4);
				memcpy(&p2_targ, elem->data + 8, 4);
				memcpy(&p2_self, elem->data + 12, 4);
				memcpy(&p3_targ, elem->data + 16, 4);
				memcpy(&p3_self, elem->data + 20, 4);
				memcpy(&p4_targ, elem->data + 24, 4);
				memcpy(&p4_self, elem->data + 28, 4);
				memcpy(&p5_targ, elem->data + 32, 4);
				memcpy(&p5_self, elem->data + 36, 4);

				// Reverse from be to le
				p1_targ = (p1_targ & 0x00FF00FF) <<  8 | (p1_targ & 0xFF00FF00) >>  8;
				p1_targ = (p1_targ & 0x0000FFFF) << 16 | (p1_targ & 0xFFFF0000) >> 16;

				p1_self = (p1_self & 0x00FF00FF) <<  8 | (p1_self & 0xFF00FF00) >>  8;
				p1_self = (p1_self & 0x0000FFFF) << 16 | (p1_self & 0xFFFF0000) >> 16;

				p2_targ = (p2_targ & 0x00FF00FF) <<  8 | (p2_targ & 0xFF00FF00) >>  8;
				p2_targ = (p2_targ & 0x0000FFFF) << 16 | (p2_targ & 0xFFFF0000) >> 16;

				p2_self = (p2_self & 0x00FF00FF) <<  8 | (p2_self & 0xFF00FF00) >>  8;
				p2_self = (p2_self & 0x0000FFFF) << 16 | (p2_self & 0xFFFF0000) >> 16;

				p3_targ = (p3_targ & 0x00FF00FF) <<  8 | (p3_targ & 0xFF00FF00) >>  8;
				p3_targ = (p3_targ & 0x0000FFFF) << 16 | (p3_targ & 0xFFFF0000) >> 16;

				p3_self = (p3_self & 0x00FF00FF) <<  8 | (p3_self & 0xFF00FF00) >>  8;
				p3_self = (p3_self & 0x0000FFFF) << 16 | (p3_self & 0xFFFF0000) >> 16;

				p4_targ = (p4_targ & 0x00FF00FF) <<  8 | (p4_targ & 0xFF00FF00) >>  8;
				p4_targ = (p4_targ & 0x0000FFFF) << 16 | (p4_targ & 0xFFFF0000) >> 16;

				p4_self = (p4_self & 0x00FF00FF) <<  8 | (p4_self & 0xFF00FF00) >>  8;
				p4_self = (p4_self & 0x0000FFFF) << 16 | (p4_self & 0xFFFF0000) >> 16;

				p5_targ = (p5_targ & 0x00FF00FF) <<  8 | (p5_targ & 0xFF00FF00) >>  8;
				p5_targ = (p5_targ & 0x0000FFFF) << 16 | (p5_targ & 0xFFFF0000) >> 16;

				p5_self = (p5_self & 0x00FF00FF) <<  8 | (p5_self & 0xFF00FF00) >>  8;
				p5_self = (p5_self & 0x0000FFFF) << 16 | (p5_self & 0xFFFF0000) >> 16;


				// Check msb uniqueness
				bool ident = false;
				uint8_t msb[5] = { p1_targ, p2_targ, p3_targ, p4_targ, p5_targ };
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 5; j++) {
						if (j != i) {
							if (msb[i] == msb[j]) {
								ident = true;
								break;
							}
						}
					}
				}

				if (ident) {
					Logger_error("GateIn_thread", "Unable to process action WSD_TYPE_SET_PIPES_MATRIX because MSB is not all uniquie");
					GateIn_completeError(ERROR_BAD_PIPE_MSB, elem->tid, soTransmitterQueue);
					continue;
				}


				// Set pipes configs
				a_p1_targ = p1_targ;
				a_p1_self = p1_self;
				a_p2_targ = p2_targ;
				a_p2_self = p2_self;
				a_p3_targ = p3_targ;
				a_p3_self = p3_self;
				a_p4_targ = p4_targ;
				a_p4_self = p4_self;
				a_p5_targ = p5_targ;
				a_p5_self = p5_self;

				#ifdef TARGET_HW

				// Reinit nrf pipes config
				pthread_mutex_lock(rf_mutex);
				NRF24_Write_Buf(RX_ADDR_P1, (uint8_t*) &a_p1_targ, TX_ADR_WIDTH);
				NRF24_Write_Buf(RX_ADDR_P2, (uint8_t*) &a_p2_targ, 1);
				NRF24_Write_Buf(RX_ADDR_P3, (uint8_t*) &a_p3_targ, 1);
				NRF24_Write_Buf(RX_ADDR_P4, (uint8_t*) &a_p4_targ, 1);
				NRF24_Write_Buf(RX_ADDR_P5, (uint8_t*) &a_p5_targ, 1);

				uint32_t c_p1 = 0;
				uint8_t c_p2 = 0;
				uint8_t c_p3 = 0;
				uint8_t c_p4 = 0;
				uint8_t c_p5 = 0;

				NRF24_Read_Buf(RX_ADDR_P1, (uint8_t*) &c_p1, TX_ADR_WIDTH);
				NRF24_Read_Buf(RX_ADDR_P2, (uint8_t*) &c_p2, 1);
				NRF24_Read_Buf(RX_ADDR_P3, (uint8_t*) &c_p3, 1);
				NRF24_Read_Buf(RX_ADDR_P4, (uint8_t*) &c_p4, 1);
				NRF24_Read_Buf(RX_ADDR_P5, (uint8_t*) &c_p5, 1);

				pthread_mutex_unlock(rf_mutex);

				if (c_p1 != a_p1_targ || c_p2 != (uint8_t) *(&a_p2_targ)  || c_p3 != (uint8_t) *(&a_p3_targ) || c_p4 != (uint8_t) *(&a_p4_targ) || c_p5 != (uint8_t) *(&a_p5_targ)) {
					Logger_error("GateIn_thread", "Unable to process action WSD_TYPE_SET_PIPES_MATRIX chip does not respond");
					GateIn_completeError(ERROR_CHIP_NOT_RESPOND, elem->tid, soTransmitterQueue);
					continue;
				}

				#endif

				Logger_info("GateIn_thread", "Processed  action WSD_TYPE_SET_PIPES_MATRIX for matrix [ [%04X, %04X], [%04X, %04X], [%04X, %04X], [%04X, %04X], [%04X, %04X] ]", p1_targ, p1_self, p2_targ, p2_self, p3_targ, p3_self, p4_targ, p4_self, p5_targ, p5_self);
				GateIn_completeEmptyResult(elem->tid, soTransmitterQueue);
			} else {

			}

			free(elem->data);
			free(elem);
		}
		usleep(100000);
	}
}
