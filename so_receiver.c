#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "libs/collections/include/rings.h"
#include "libs/collections/include/lbq.h"
#include "include/so_receiver.h"
#include "include/gate_in.h"
#include "include/logger.h"
#include "include/utils.h"
#include "include/global.h"
#include "include/wsd_packet.h"

#define MODE_PREAMBLE 0
#define MODE_HEADER 1
#define MODE_BODY 2

/**
 * Receives raw data from a socket and parses driver packages. Disassembled packets are placed in
 * the downstream buffer leading to the gate_in packet handler.
 */
void SoReceiver_thread(SoReceiverThreadArgs *args) {
    bool alive = true;
    RingBufferDef *inBuf  = RINGS_createRingBuffer(RING_BUFFER_SIZE, RINGS_OVERFLOW_SHIFT, true);
    Logger_info("SoReceiver_thread", "Socket handler thread for sockdf '%d' was started", args->socket);

    uint64_t prbits = WSD_PREAMBLE_R;
    uint8_t mode = MODE_PREAMBLE;
    WsdPacket *packet = NULL;
    uint16_t sbody = 0;


    while(alive) {
            char ch;
            ssize_t r = read(args->socket, &ch, 1);
            if (r > 0) { // if some char was received
            	RINGS_write((uint8_t) ch, inBuf);
            	uint16_t dlen = RINGS_dataLenght(inBuf);
            	if (mode == MODE_PREAMBLE) { // If expected preamble form stream
            		if (dlen >= WSD_PREAMBLE_SIZE) { //If the buffer contain data with size of preamble or more
            			int r = RINGS_cmpData(dlen - WSD_PREAMBLE_SIZE, (uint8_t*) &prbits, WSD_PREAMBLE_SIZE, inBuf);
            			if (r == 0) {
            				RINGS_dataClear(inBuf);
            				mode = MODE_HEADER;
            			}
            		}
            	} else if (mode == MODE_HEADER) {
            		if (dlen >= WSD_HEADER_SIZE) {
            			uint8_t *header = (uint8_t*) malloc(WSD_HEADER_SIZE);
            			packet = (WsdPacket*) malloc(sizeof(WsdPacket));
            			RINGS_readAll((uint8_t*)header, inBuf);
            			WsdPacket_parsePacketHeader(packet, header, 0);
            			sbody = packet->length;
            			//if (sbody > 128)
            			//	mode = MODE_PREAMBLE;
            			mode = MODE_BODY;
            			free(header);
            		}
            	} else {
            		if (dlen >= sbody) {
            			uint8_t *body = (uint8_t*) malloc(sbody);
            			RINGS_readAll(body, inBuf);
            			GateInQueueElement *elem = (GateInQueueElement*) malloc(sizeof(GateInQueueElement));
            			elem->socket = args->socket;
            			elem->tid = packet->tid;
            			elem->action = packet->type;
            			elem->size = sbody;
            			elem->data = body;
            			#ifdef DEBUG
            			char *bodyh = sprintfhex(body, sbody);
            			Logger_debug("SoReceiver_thread", "Received new wsd packet [ length=%d, type=%d, body=%s]", sbody, packet->type, bodyh);
            			free(bodyh);
            			#endif
            			args->downQueue->enqueue(args->downQueue, elem);
            			free(packet);
            			mode = MODE_PREAMBLE;
            		}
            	}
            } else {
            	alive = false;
            }
        }

        RINGS_Free(inBuf);
        free(inBuf);
        close(args->socket);
        Logger_info("SoReceiver_thread", "Socket %d is closed", args->socket);
}
