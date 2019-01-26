#ifndef INCLUDE_RF_H_
#define INCLUDE_RF_H_

#include <stdint.h>
#include <stdbool.h>

bool RF_transmit(uint8_t *data, uint16_t size);
int RF_Receive(uint8_t *payload);
int RF_init();

#endif
