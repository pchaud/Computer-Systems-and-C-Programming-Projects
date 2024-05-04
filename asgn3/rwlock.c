#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

#include "rwlock.h"

typedef struct rwlock {

    //was a uint 32
    int n;
    pthread_mutex_t lock;
    pthread_cond_t readcv;
    pthread_cond_t writecv;

    int active_readers;
    int wait_readers;
    int active_writer;
    int wait_writers;
    int readers_since_last_write;

    PRIORITY priority;

} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {

    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));

    if (rw == NULL) {
        return NULL;
    }

    rw->n = n;
    rw->priority = p;
    rw->active_readers = 0;
    rw->wait_readers = 0;
    rw->active_writer = 0;
    rw->wait_writers = 0;
    rw->readers_since_last_write = 0;

    pthread_mutex_init(&(rw->lock), NULL);
    pthread_cond_init(&(rw->readcv), NULL);
    pthread_cond_init(&(rw->writecv), NULL);

    return rw;
}

void rwlock_delete(rwlock_t **rw) {

    pthread_mutex_destroy(&(*rw)->lock);
    pthread_cond_destroy(&((*rw)->readcv));
    pthread_cond_destroy(&((*rw)->writecv));

    //freeing allocated memory and setting passed-in pointer to NULL
    free(*rw);
    *rw = NULL;
}

void reader_lock(rwlock_t *rw) {

    //acquire lock, set # of (incrmenet) waiting readers, while there is an active reader (wait)

    //basic idea:
    /*bool acquired = false
        while(!acquired){}
            internalmutexlock
            if(! writer){
                numreaders++
                acquired true
            }
            internal mutex unlock
        }*/

    //bools only used in lock functions

    //acquire lock
    pthread_mutex_lock(&(rw->lock));

    //increment # of waiting readers

    rw->wait_readers++;

    //N-WAY: allowing readers if no writes or if ratio hasn't overfilled
    if (rw->priority == N_WAY) {
        while (rw->active_writer > 0 || (rw->readers_since_last_write >= rw->n)) {
            pthread_cond_wait(&(rw->readcv), &(rw->lock));
        }

        //WRITERS: allow reader if there's no active writer in critical region
    } else if (rw->priority == WRITERS) {
        //WAS this  while(rw->active_writer > 0)
        while (rw->active_writer > 0 || rw->wait_writers > 0) {
            pthread_cond_wait(&rw->readcv, &rw->lock);
        }

        //wasn't here before
    } else if (rw->priority == READERS) {
        while (rw->active_writer > 0) {
            pthread_cond_wait(&rw->readcv, &rw->lock);
        }
    }

    rw->wait_readers--;

    rw->readers_since_last_write++;

    rw->active_readers++;

    pthread_mutex_unlock(&(rw->lock));
}

void reader_unlock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->lock));
    rw->active_readers--;

    //broke up original (w->active_writer > 0 || (rw->active_readers >= rw->n && rw->wait_writers > 0)
    if (rw->active_readers == 0) {
        if (rw->wait_writers > 0) {
            pthread_cond_signal(&rw->writecv);
        } else {
            pthread_cond_broadcast(&rw->readcv);
        }
    }

    pthread_mutex_unlock(&(rw->lock));
}

void writer_lock(rwlock_t *rw) {
    //acquire lock
    pthread_mutex_lock(&(rw->lock));

    //increment # of waiting readers
    rw->wait_writers++;

    if (rw->priority == READERS) {
        while (rw->active_readers > 0 || rw->active_writer > 0 || rw->wait_readers > 0) {
            pthread_cond_wait(&(rw->writecv), &(rw->lock));
        }
    } else if (rw->priority == WRITERS) {
        while (rw->active_writer > 0 || rw->active_readers > 0) {
            pthread_cond_wait(&(rw->writecv), &(rw->lock)); //added parentheses
        }
    } else if (rw->priority == N_WAY) {
        //was the exact same as readers conditions
        //while(rw->active_readers <= rw->n && rw->wait_readers > 0){ -> causes core dumped
        //while(rw->active_readers > rw->n || rw->active_writer > 0 || rw->wait_readers > 0){ -> writers more than readers ratio and STRESS fails
        //while(rw->active_readers > 0 || rw->active_writer > 0 || rw->wait_readers > 0){ -> writers less than readers (always 0) in this ratio

        //rw->active_writer > 0 || (rw->active_readers >= rw->n && rw->wait_writers >0) ->
        while (rw->active_readers > 0 || rw->active_writer > 0
               || ((rw->readers_since_last_write < rw->n) && rw->wait_readers > 0)) {
            pthread_cond_wait(&(rw->writecv), &(rw->lock));
        }
    }

    //acquire lock, set # of (incrmenet) waiting readers, while there is an active reader (wait)

    rw->wait_writers--;

    rw->readers_since_last_write = 0; //resetting

    rw->active_writer++;

    pthread_mutex_unlock(&(rw->lock));
}

void writer_unlock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->lock));

    rw->active_writer--;

    assert(rw->active_writer == 0);

    pthread_mutex_unlock(&(rw->lock));

    bool readers = rw->wait_readers > 0;

    bool writers = rw->wait_writers > 0;

    if (rw->priority == READERS) {
        //if reads is priority broadcast to readers
        if (readers) {
            pthread_cond_broadcast(&rw->readcv);
            //if it's not switch it over to write cv
        } else if (writers) {
            pthread_cond_signal(&rw->writecv);
        }
        //if writers is priority
    } else if (rw->priority == WRITERS) {
        // signal to write cv

        if (writers) {
            pthread_cond_signal(&rw->writecv);

            //if not writers -> signal to read cv
        } else if (readers) {
            pthread_cond_signal(&rw->readcv);
        }
        // if n way is priority
    } else if (rw->priority == N_WAY) {

        //broadcast to read cv
        if (readers) {
            pthread_cond_broadcast(&rw->readcv);

        } else if (writers) {
            //maybe switch for broadcast
            pthread_cond_signal(&rw->writecv);
        }
    }
}
