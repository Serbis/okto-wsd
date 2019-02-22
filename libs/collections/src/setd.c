#include <string.h>
#include "../include/setd.h"

bool Set_add(SetD set, void *value) {
	_Set_lock(set);

	ListIterator *iterator = set->list->iterator(set->list);

	bool f = false;
	while(iterator->hasNext(iterator)) {
		if (memcmp(iterator->next(iterator), value, set->dimension) == 0) {
			f = true;
			break;
		}
	}

	free(iterator);
	if (!f) {
		set->list->prepend(set->list, value);
		_Set_unlock(set);
		return true;
	}

	_Set_unlock(set);

	return false;
}

bool Set_remove(SetD set, void *value) {
	_Set_lock(set);

	ListIterator *iterator = set->list->iterator(set->list);

	bool f = false;
	while(iterator->hasNext(iterator)) {
		void *n = iterator->next(iterator);
		if (memcmp(n, value, set->dimension) == 0) {
			f = true;
			iterator->remove(iterator);
			free(n);
			break;
		}
	}

	free(iterator);
	if (f) {
		_Set_unlock(set);
		return true;
	}

	_Set_unlock(set);

	return false;

}

void* Set_getAll(SetD set, uint32_t *size) {
	_Set_lock(set);

	void *blob = malloc(set->list->size);
	ListIterator *iterator = set->list->iterator(set->list);

	uint32_t counter = 0;
	while(iterator->hasNext(iterator)) {
		void *n = iterator->next(iterator);
		memcpy(blob[counter], n, set->dimension);
		counter = counter + set->dimension;
	}

	free(iterator);
	*size = set->list->size;

	_Set_unlock(set);

	return blob;
}

SetD* Set_new(uint32_t dimension, bool sync) {
	SetD *set = (SetD*) malloc(sizeof(SetD));
	List *list = new_List();
	set->list = list;
	set->dimension = dimension;
	set->sync = sync;

	if (sync) {
		#ifdef PTHREADS
		pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(mutex, NULL);
		set->mutex = mutex;
		#endif
	}

	return set;
}

#ifdef PTHREADS
void _Set_lock(SetD set) {
	if (set->sync)
		pthread_mutex_lock(set->mutex);
}

void _Set_unlock(SetD set) {
	if (set->sync)
		pthread_mutex_unlock(set->mutex);
}
#endif
