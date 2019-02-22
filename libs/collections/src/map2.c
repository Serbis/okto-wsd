#include <memory.h>
#include "../include/map2.h"
#include "../../oscl/include/malloc.h"
#include "../../oscl/include/data.h"

void _MAP_lock(Map *map) {
	#ifdef PTHREADS_LOCK
	pthread_mutex_lock(map->mutex);
	#endif
}

void _MAP_unlock(Map *map) {
	#ifdef PTHREADS_LOCK
	pthread_mutex_unlock(map->mutex);
	#endif
}


void* MAP_get(char* key, Map *map) {
	_MAP_lock(map);

    ListIterator *iterator = map->inner->iterator(map->inner);
    while (iterator->hasNext(iterator)) {
        MapItem *item = iterator->next(iterator);
        if (strcmp(item->key, key) == 0) {
            pfree(iterator);
            _MAP_unlock(map);
            return item->value;
        }
    }

    pfree(iterator);

    _MAP_unlock(map);

    return NULL;
}

void MAP_add(char* key, void* value, Map *map) {
	_MAP_lock(map);

    MapItem *item = pmalloc(sizeof(MapItem));
    item->key = strcpy2(key);
    item->value = value;

    map->inner->prepend(map->inner, item);

    _MAP_unlock(map);
}

void* MAP_remove(char* key, Map *map) {
	_MAP_lock(map);

    ListIterator *iterator = map->inner->iterator(map->inner);
    uint16_t index = 0;
    while (iterator->hasNext(iterator)) {
        MapItem *item = iterator->next(iterator);
        if (strcmp(item->key, key) == 0) {
            pfree(iterator);
            map->inner->remove(map->inner, index);
            void *value = item->value;
            pfree(item->key);
            pfree(item);

            _MAP_unlock(map);

            return value;
        } else {
            index++;
        }
    }

    pfree(iterator);

    _MAP_unlock(map);

    return NULL;
}

//TODO Удаление коллекции

bool MAP_contain(char *key, Map *map) {
	_MAP_lock(map);

    ListIterator *iterator = map->inner->iterator(map->inner);

    while (iterator->hasNext(iterator)) {
        MapItem *item = iterator->next(iterator);
        if (strcmp(item->key, key) == 0) {
            pfree(iterator);
            _MAP_unlock(map);
            return true;
        }
    }

    pfree(iterator);

    _MAP_unlock(map);

    return false;
}

//Внимание, перед вызовом данной функции, нужно очистить inner от элементов
Map* MAP_del(Map *map) {
    del_List(map->inner);
	#ifdef PTHREADS_LOCK
    pfree(map->mutex);
	#endif
    pfree(map);
}

Map* MAP_new() {
    Map* map = (Map*) pmalloc(sizeof(Map));
    map->inner = new_List();

	#ifdef PTHREADS_LOCK
	pthread_mutex_t *mutex = pmalloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	map->mutex = mutex;
	#endif

    return map;
}

MapIterator* MAP_ITERATOR_new(Map *map) {
    MapIterator *iterator = pmalloc(sizeof(MapIterator));
    iterator->map = map;
    iterator->nextNode = map->inner->head;
    iterator->lastRet = -1;

    return iterator;
}

/** Lock map mutex for travers through iterator. Any concurrecnt operation at map will be blocked while
 *  until iterator be unlicked */
void MAP_ITERATOR_lock(MapIterator *iterator) {
	_MAP_lock(iterator->map);
}

void MAP_ITERATOR_unlock(MapIterator *iterator) {
	_MAP_unlock(iterator->map);
}

bool MAP_ITERATOR_hasNext(MapIterator *iterator) {
    return iterator->nextNode != NULL;
}

void* MAP_ITERATOR_next(MapIterator *iterator) {
    Node *next = iterator->nextNode;
    iterator->nextNode = next->next;
    iterator->lastRet++;

    return ((MapItem*)next->item)->value;
}

void MAP_ITERATOR_remove(MapIterator *iterator) {
    MapItem *item = iterator->map->inner->remove(iterator->map->inner, (uint16_t) iterator->lastRet);
    pfree(item->key);
    pfree(item);
    iterator->lastRet--;
}
