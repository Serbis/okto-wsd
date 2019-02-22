/*
 * set.h
 *
 *  Created on: 11 февр. 2019 г.
 *      Author: serbis
 */

#ifndef LIBS_COLLECTIONS_INCLUDE_SETD_H_
#define LIBS_COLLECTIONS_INCLUDE_SETD_H_

#define PTHREADS

#include <stdint.h>
#include <stdbool.h>
#ifdef PTHREADS
#include <pthread.h>
#include <stdlib.h>
#endif
#include "../list.h"



typedef struct {
	List *list;
	bool sync;
	uint32_t dimension;
	#ifdef PTHREADS
	pthread_mutex_t *mutex;
	#endif
} SetD;

bool Set_add(SetD set, void *value);
bool Set_remove(SetD set, void *value);
void* Set_getAll(SetD set, uint32_t *size);
SetD* Set_new(uint32_t dimension, bool sync);

#endif /* LIBS_COLLECTIONS_INCLUDE_SETD_H_ */
