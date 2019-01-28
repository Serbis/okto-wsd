#include <unistd.h>
#include "include/so_transmitter.h"
#include "include/logger.h"
#include "include/global.h"
#include "include/wsd_packet.h"
#include "include/utils.h"

void SoTransmitter_thread(SoTransmitterArgs * args) {
	LinkedBlockingQueue *upQueue = args->upQueue;
	Map *tidSoMap = args->tidSoMap;
	Logger_info("SoTransmitter_thread", "SoTransmitter thread was started");

	while(1) {
		if (upQueue->size(upQueue) > 0) {
			SoTransmitterQueueElem *elem = upQueue->dequeue(upQueue);

			char *tidk = itoa2(elem->tid);
			int32_t *socket = (int32_t*) MAP_get(tidk, tidSoMap);
			free(tidk);

			WsdPacket *packet = (WsdPacket*) malloc(sizeof(WsdPacket));
			packet->tid = elem->tid;
			packet->type = elem->type;
			packet->length = elem->size;
			packet->body = elem->data;

			uint16_t sbin = 0;
			uint8_t *bin = WsdPacket_toBinary(packet, &sbin);


			if (socket != NULL) {
				#ifdef DEBUG
				char *bhex = sprintfhex(packet->body, packet->length);
				Logger_debug("SoTransmitter_thread", "Transmit to env wsd packet [ tid=%d, type=%d, length=%d, data=%s]", packet->tid, packet->type, packet->length, bhex);
				free(bhex);
				#endif

				write(*socket, bin, sbin);
				fsync(*socket);
			} else  {
				#ifdef DEBUG
				char *bhex = sprintfhex(packet->body, packet->length);
				Logger_debug("SoTransmitter_thread", "Unable to transmit to env wsd packet becuse tid is rotten [ tid=%d, type=%d, length=%d, data=%s]", packet->tid, packet->type, packet->length, bhex);
				free(bhex);
				#endif
			}

			//TODO если сокета уже нет в мепе, то будет возвращен 0, попытка записи туда приведет к краху




			free(elem->data);
			free(elem);
			free(bin);
		}

		usleep(100000);
	}
}
