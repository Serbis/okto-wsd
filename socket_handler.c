#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "libs/collections/include/rings.h"
#include "libs/collections/include/lbq.h"
#include "include/socket_handler.h"
#include "include/data_processor.h"
#include "include/packet.h"
#include "include/logger.h"

#define MODE_PREAMBLE 0
#define MODE_HEADER 1
#define MODE_BODY 2

void SocketHandler_thread(void *args) {
    bool clientThread_alive = true;
    int sockfd = *(int*) args;
    Logger_info("SocketHandler_thread", "Socket handler thread for sockdf '%d' was started", sockfd);
    LinkedBlockingQueue *cmdQueue = new_LQB(QUEUE_SIZE); //Free in cmd_processor
    RingBufferDef *inBuf  = RINGS_createRingBuffer(RING_BUFFER_SIZE, RINGS_OVERFLOW_SHIFT, true);

    DataProcessor_Args *cpa = malloc(sizeof(DataProcessor_Args));  //Free in cmd_processor
    cpa->cmdQueue = cmdQueue;
    cpa->sockfd = sockfd;
    cpa->alive = true;
    pthread_t thread;
    pthread_create(&thread, NULL, (void *) DataProcessor_thread, cpa);

    uint64_t prbits = PREAMBLE;
    uint8_t mode = MODE_PREAMBLE;
    uint16_t sbody = 0;
    uint16_t action = 100;


    while(clientThread_alive) {
            char ch;
            ssize_t r = read(sockfd, &ch, 1);
            if (r > 0) { // if some char was received
            	RINGS_write((uint8_t) ch, inBuf);
            	uint16_t dlen = RINGS_dataLenght(inBuf);
            	if (mode == MODE_PREAMBLE) { // If expected preamble form stream
            		if (dlen >= PREAMBLE_SIZE) { //If the buffer contain data with size of preamble or more
            			if (RINGS_cmpData(dlen - PREAMBLE_SIZE, (uint8_t*) &prbits, PREAMBLE_SIZE, inBuf) == 0) {
            				RINGS_dataClear(inBuf);
            				mode = MODE_HEADER;
            			}
            		}
            	} else if (mode == MODE_HEADER) {
            		if (dlen >= HEADER_SIZE) {
            			uint8_t *header = (uint8_t*) malloc(HEADER_SIZE);
            			RINGS_readAll((uint8_t*)header, inBuf);
            			action =  *header;
            			sbody = *(header + 2);
            			mode = MODE_BODY;
            			free(header);
            		}
            	} else {
            		if (dlen >= sbody) {
            			uint8_t *body = (uint8_t*) malloc(sbody);
            			QueueElement *elem = (QueueElement*) malloc(sizeof(QueueElement));
            			RINGS_readAll(body, inBuf);
            			elem->action = action;
            			elem->size = sbody;
            			elem->data = (void*) body;
            			Logger_debug("SocketHandler_thread", "Received new packet { length=%d, action=%d, body=... }", sbody);
            			cmdQueue->enqueue(cmdQueue, elem);
            			mode = MODE_PREAMBLE;
            		}
            	}
            } else {
                //Logger_info("ClientThread", "Socket is closed");
                clientThread_alive = false;
            }
        }

        cpa->alive = false;
        RINGS_Free(inBuf);
        close(sockfd);
}
