#include "../include/lbq8.h"
#include "../../oscl/include/malloc.h"
#include "../../oscl/include/threads.h"

void LBQ8_enqueue(void *self, uint8_t item) {
    //TODO Блокировка очереди с проверкой работы экзекутора и акторов
    LinkedBlockingQueue8 *this = (LinkedBlockingQueue8*) self;
    Node8 *node = pmalloc(sizeof(Node8));

    MutexLock(this->mutex);

    node->item = item;
    node->next = NULL;

    if (this->last == NULL) {
        this->last = node;
        this->head = node;
    } else {
        this->last->next = node;
        this->last = node;
    }

    this->count = (uint16_t) (this->count + 1);

    MutexUnlock(this->mutex);
}


uint8_t LBQ8_dequeue(void *self) {
    LinkedBlockingQueue8 *this = (LinkedBlockingQueue8*) self;

    MutexLock(this->mutex);

    if (this->head != NULL) {
        Node8 *head = this->head;

        uint8_t item = head->item;

        if (this->head->next != NULL) {
            this->head = head->next;
            pfree(head);
        } else {
            pfree(head);
            this->head = NULL;
            this->last = NULL;
        }

        this->count = (uint16_t) (this->count - 1);
        MutexUnlock(this->mutex);
        return item;
    } else {
        MutexUnlock(this->mutex);
        return NULL;
    }
}

uint16_t LBQ8_size(void *self) {
    LinkedBlockingQueue8 *this = (LinkedBlockingQueue8*) self;
    MutexLock(this->mutex);
    uint16_t size = this->count;
    MutexUnlock(this->mutex);
    return size;

}

//Внимание, до вызова функции очередь должна быть полностью очищена
void del_LQB8(LinkedBlockingQueue8 *queue) {
    pfree(queue->mutex);
    pfree(queue);
}

LinkedBlockingQueue8* new_LQB8(uint16_t capacity) {
    LinkedBlockingQueue8* queue = (LinkedBlockingQueue8*) pmalloc(sizeof(LinkedBlockingQueue8));
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = NULL;
    queue->last = NULL;
    queue->mutex = NewMutex();

    queue->enqueue = LBQ8_enqueue;
    queue->dequeue = LBQ8_dequeue;
    queue->size = LBQ8_size;

    return queue;
}
