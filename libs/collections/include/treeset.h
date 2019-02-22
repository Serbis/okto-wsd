#ifndef LIBS_COLLECTIONS_INCLUDE_TREESET_H_
#define LIBS_COLLECTIONS_INCLUDE_TREESET_H_

#define PTHREADS

#include <stdint.h>
#include <stdbool.h>
#ifdef PTHREADS
#include <pthread.h>
#include <stdlib.h>
#endif

typedef struct TreeSetNode TreeSetNode;

struct TreeSetNode {
	uint32_t hash;
	uint32_t size;
	void *value;
	TreeSetNode *left;
	TreeSetNode *right;
};

typedef struct {
	TreeSetNode *root;
	uint32_t size;
	bool sync;
	#ifdef PTHREADS
	pthread_mutex_t *mutex;
	#endif
} TreeSet;

typedef struct {

} TreeSetIterator;

void TreeSetIterator_lock(TreeSetIterator *iterator);
void TreeSetIterator_unlock(TreeSetIterator *iterator);
bool TreeSetIterator_hasNext(TreeSetIterator *iterator);
void* TreeSetIterator_next(TreeSetIterator *iterator, uint32_t *size);

bool TreeSet_put(TreeSet *set, void *value, uint32_t size);
bool TreeSet_remove(TreeSet *set, void *value, uint32_t size);
TreeSetIterator* TreeSet_iterator(TreeSet *set);
TreeSet* TreeSet_new(bool sync);

#endif /* LIBS_COLLECTIONS_INCLUDE_TREESET_H_ */
