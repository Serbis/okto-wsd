#include "../include/treeset.h"

/** Return hash of an element */
uint32_t _TreeSet_hash(void *value, uint32_t size) {
	uint32_t h = 0;
	if (h == 0 && size > 0) {
		uint8_t *val = (uint8_t*) value;

	    for (int i = 0; i < size; i++) {
	    	h = 31 * h + val[i];
	    }

	}

	return h;
}

TreeSetNode* _TreeSet_node(uint32_t hash, uint32_t size, void *value) {
	TreeSetNode *node = (TreeSetNode*) malloc(sizeof(TreeSetNode));
	node->hash = hash;
	node->size = size;
	node->value = value;
	node->left = NULL;
	node->right = NULL;

	return node;
}



#ifdef PTHREADS
void _TreeSet_lock(TreeSet *set) {
	if (set->sync)
		pthread_mutex_lock(set->mutex);
}

void _TreeSet_unlock(TreeSet *set) {
	if (set->sync)
		pthread_mutex_unlock(set->mutex);
}
#endif

void TreeSetIterator_lock(TreeSetIterator *iterator) {

}

void TreeSetIterator_unlock(TreeSetIterator *iterator) {

}

bool TreeSetIterator_hasNext(TreeSetIterator *iterator) {
	return false;
}

void* TreeSetIterator_next(TreeSetIterator *iterator, uint32_t *size) {
	return NULL;
}

bool TreeSet_put(TreeSet *set, void *value, uint32_t size) {
	_TreeSet_lock(set);

	uint32_t hash = _TreeSet_hash(value, size);

	if (set->root == NULL) {
		TreeSetNode *node = _TreeSet_node(hash, size, value);
		set->root = node;
		set->size++;
		_TreeSet_unlock(set);
		return true;
	} else {
		TreeSetNode *cnode = set->root;

		while(1) {
			if (cnode->hash == hash) {
				cnode->value = value;
				cnode->size = size;

				set->size++;

				_TreeSet_unlock(set);
				return true;
			} else {
				if (hash < cnode->hash) {
					if (cnode->left == NULL) {
						TreeSetNode *node = _TreeSet_node(hash, size, value);
						cnode->left = node;

						set->size++;

						_TreeSet_unlock(set);
						return true;
					} else {
						cnode = cnode->left;
					}
				} else {
					if (cnode->right == NULL) {
						TreeSetNode *node = _TreeSet_node(hash, size, value);
						cnode->right = node;

						set->size++;

						_TreeSet_unlock(set);
						return true;
					} else {
						cnode = cnode->right;
					}
				}
			}
		}
	}
}

bool TreeSet_remove(TreeSet *set, void *value, uint32_t size) {
	_TreeSet_lock(set);

	uint32_t hash = _TreeSet_hash(value, size);

	if (set->root == NULL) {
		_TreeSet_unlock(set);
		return false;
	} else {
		TreeSetNode *cnode = set->root;
		TreeSetNode *parent = NULL;
		int8_t side = 0;

		while(1) {


			if (cnode->left == NULL && cnode->right == NULL) {
				if (parent != NULL) {
					if (side > 0)
						parent->right = NULL;
					else if (side < 0)
						parent->left = NULL;
				}
			} else if (cnode->left == NULL || cnode->right == NULL) {
				if (parent != NULL) {
					if (side > 0) {
						if (cnode->left == NULL)
							parent->right = cnode->right;
						else
							parent->right = cnode->left;
					} else if (side < 0) {
						if (cnode->left == NULL)
							parent->left = cnode->right;
						else
							parent->left = cnode->left;
					}
				}
			} else {

			}
		}
	}

}

TreeSetIterator* TreeSet_iterator(TreeSet *set) {
	return NULL;
}

/**
 * Create new treeset
 *
 * @param sync use mutex lock at opearte with this treeset
 */
TreeSet* TreeSet_new(bool sync) {
	TreeSet *set = (TreeSet*) malloc(sizeof(TreeSet));
	set->root = NULL;
	set->size = 0;
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



