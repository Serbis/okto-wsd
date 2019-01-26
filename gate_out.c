#include <unistd.h>
#include "libs/collections/include/rings.h"
#include "include/exb_packet.h"
#include "include/wsd_packet.h"
#include "include/gate_out.h"
#include "include/logger.h"
#include "include/utils.h"
#include "include/global.h"
#include "include/so_transmitter.h"

#define MODE_PREAMBLE 0
#define MODE_HEADER 1
#define MODE_BODY 2

void GateOut_thread(GateOutThreadArgs *args) {
	LinkedBlockingQueue *upQueue = args->upQueue;
	LinkedBlockingQueue *soTransmitterQueue = args->soTransmitterQueue;
	Logger_info("GateOut_thread", "Gate_out thread was started");

	uint64_t prbits = EXB_PREAMBLE_R; //Revesed to big-endian preable
	uint8_t mode = MODE_PREAMBLE;
	ExbPacket *packet = NULL;
	uint16_t sbody = 0;
	RingBufferDef *inBuf  = RINGS_createRingBuffer(256, RINGS_OVERFLOW_SHIFT, true);
	inBuf->reader = 1;
	inBuf->writer = 0;

	while(1) {
		if (upQueue->size(upQueue) > 0) {
			uint8_t *payload = (uint8_t*) upQueue->dequeue(upQueue);
			for (uint8_t i = 0; i < 32; i++) {
				uint8_t ch = payload[i];

				RINGS_write((uint8_t) ch, inBuf);
				uint16_t dlen = RINGS_dataLenght(inBuf);

				if (mode == MODE_PREAMBLE) { // If expected preamble form stream
					if (dlen >= EXB_PREAMBLE_SIZE) { //If the buffer contain data with size of preamble or more
						int r = RINGS_cmpData(dlen - EXB_PREAMBLE_SIZE, (uint8_t*) &prbits, EXB_PREAMBLE_SIZE, inBuf);
					    if (r == 0) {
					    	//RINGS_dataClear(inBuf);
					    	RINGS_shiftReader(dlen - EXB_PREAMBLE_SIZE, inBuf);
							mode = MODE_HEADER;
						}
					}
				} else if (mode == MODE_HEADER) {
					 if (dlen >= EXB_HEADER_SIZE + EXB_PREAMBLE_SIZE) {
						 uint8_t *header = (uint8_t*) malloc(EXB_HEADER_SIZE);
				         packet = (ExbPacket*) malloc(sizeof(ExbPacket));
				         RINGS_extractData(inBuf->reader + EXB_PREAMBLE_SIZE, EXB_HEADER_SIZE, header, inBuf);
				         ExbPacket_parsePacketHeader(packet, header, 0);
				         sbody = packet->length;
				         //if (sbody > 128)
				         	 //	mode = MODE_PREAMBLE;
				         mode = MODE_BODY;
				         free(header);
				     }
				 } else {
				 	if (dlen >= sbody + EXB_HEADER_SIZE + EXB_PREAMBLE_SIZE) {
				 		int stotal = sbody + EXB_HEADER_SIZE + EXB_PREAMBLE_SIZE + 4;
				    	uint8_t *blob = (uint8_t*) malloc(stotal);
				        RINGS_readAll(blob + 4, inBuf);

				        blob[0] = 0;
				        blob[1] = 0;
				        blob[2] = 0;
				        blob[3] = 0;


						#ifdef DEBUG
				        char *bhex = sprintfhex(blob, stotal);
				        Logger_debug("GateOut_thread", "Received exb packet from rf [ tid=%d, data=%s]", packet->tid, bhex);
				        free(bhex);
						#endif

				        SoTransmitterQueueElem *sot = (SoTransmitterQueueElem*) malloc(sizeof(SoTransmitterQueueElem));
				        sot->tid = packet->tid;
				        sot->type = WSD_TYPE_RECEIVE;
				        sot->size = stotal;
				        sot->data = (uint8_t*) blob;

				        soTransmitterQueue->enqueue(soTransmitterQueue, sot);

				        //free(blob);
				        RINGS_dataClear(inBuf);
				        free(packet);

				        mode = MODE_PREAMBLE;
				    }
				 }
			}
			free(payload);
		}
		usleep(50000);
	}
}
