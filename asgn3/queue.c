#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include "queue.h"

//used lecture slides 14: slide 31 specifically for the functionality I implemented below
typedef struct queue {

    void **buf;
    int size;
    int count;
    int in;
    int out;
    pthread_mutex_t lock;
    pthread_cond_t full;
    pthread_cond_t empty;

} queue_t;

queue_t *queue_new(int size) {
    //allocating memoory for queue
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));

    //setting necessary int fields
    q->size = size;
    q->count = 0;
    q->in = 0;
    q->out = 0;

    //allocating memory for buffer
    q->buf = (void **) malloc(size * sizeof(void *));

    //initializing mutex and cv's
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->empty, NULL);
    pthread_cond_init(&q->full, NULL);

    //returning newly made queue
    return (q);
}

void queue_delete(queue_t **q) {

    if (q) {

        //freeing memory in buffer
        free((*q)->buf);

        //destructing the condition variables and locks used
        pthread_mutex_destroy(&(*q)->lock);
        pthread_cond_destroy(&(*q)->empty);
        pthread_cond_destroy(&(*q)->full);

        //freeing allocated memory and setting passed-in pointer to NULL
        free(*q);
        *q = NULL;
    }
}

bool queue_push(queue_t *q, void *elem) {

    //TODO: might be issue for deadlock !!!
    // Both functions should return true unless their q argument is equal to NULL
    if (q == NULL) {
        return false;
    }

    //L.ACQUIRE() : locks mutex for queue
    pthread_mutex_lock(&q->lock);

    //waits until queue isn't full
    while (q->count == q->size) {
        //FULL.WAIT(1)
        pthread_cond_wait(&q->full, &q->lock);
    }

    //BUFFER[IN] = ITEM : places new element in buffer
    q->buf[q->in] = elem;

    //IN = (IN +1) % N : updating in index in queue
    q->in = (q->in + 1) % (q->size);

    //COUNT += 1 : incrementing number of elements
    q->count += 1;

    //EMPTY.SIGNAL(1): signals queue isn't empty anymore to other threads
    pthread_cond_signal(&q->empty);

    //L.RELEASE() : unlocks mutex for other threads to access queue
    pthread_mutex_unlock(&q->lock);

    return true;
}

bool queue_pop(queue_t *q, void **elem) {

    //Both functions should return true unless their q argument is equal to NULL
    if (q == NULL) {
        return false;
    }

    //L.ACQUIRE(): locking mutex for thread to access queue freely
    pthread_mutex_lock(&q->lock);

    //loops/waits until queue isn't empty
    while (q->count == 0) {
        //EMPTY.WAIT(1)
        pthread_cond_wait(&q->empty, &q->lock);
    }

    //gets element from buffer
    *elem = (q->buf[q->out]);

    //updating out index
    q->out = (q->out + 1) % (q->size);

    //decrementing count of elements in queue
    q->count -= 1;

    //FULL_SIGNAL(1): signals to other threads that queue isn't full anymore
    pthread_cond_signal(&q->full);

    //L.RELEASE(): unlocks mutex to allow threads in
    pthread_mutex_unlock(&q->lock);

    return true;
}
