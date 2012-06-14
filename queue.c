/*
 * mutex based Queue
 * but not worked.
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct entry {
    struct entry *next;
    Data data;
} entry_t;

typedef struct Queue {
    volatile long queue_size;
    entry_t *queue_head;
    entry_t *queue_tail;
    pthread_mutex_t queue_lock;
    pthread_cond_t  queue_not_empty;
} tpool_t;

Queue* Queue_new(int i)
{
    Queue *q = NULL;
    q = (Queue *) malloc(sizeof(*q));
    q->queue_head = NULL;
    q->queue_tail = NULL;
    pthread_mutex_init(&q->queue_lock, NULL);
    pthread_cond_init(&q->queue_not_empty, NULL);

    return q;
}

int  Queue_isEmpty(Queue *q)
{
    pthread_mutex_lock(&q->queue_lock);
    fprintf(stderr, "I %d\n", q->queue_size);
    int res = (q->queue_size == 0);
    fprintf(stderr, "I %d\n", q->queue_size);
    pthread_mutex_unlock(&q->queue_lock);
    return res;
}

void Queue_enq(ThreadContext *ctx, Queue *q, Data arg)
{
    entry_t *e;
    pthread_mutex_lock(&q->queue_lock);

    e = (entry_t *) malloc(sizeof(entry_t));
    e->data = arg;
    e->next = NULL;

    if (q->queue_size == 0) {
        q->queue_head = q->queue_tail = e;
        q->queue_size++;
        pthread_cond_signal(&q->queue_not_empty);
    } else {
        q->queue_tail->next = e;
        q->queue_tail = e;
        q->queue_size++;
    }
    pthread_mutex_unlock(&q->queue_lock);
}

Data Queue_deq(ThreadContext *ctx, Queue *q)
{
    entry_t *e;
    pthread_mutex_lock(&q->queue_lock);
    while (q->queue_size == 0) {
        fprintf(stderr, "D %d\n", q->queue_size);
        pthread_cond_wait(&q->queue_not_empty, &q->queue_lock);
        fprintf(stderr, "D %d\n", q->queue_size);
    }

    e = q->queue_head;
    --q->queue_size;
    if (q->queue_size == 0) {
        q->queue_head = q->queue_tail = NULL;
    }
    else {
        q->queue_head = e->next;
    }

    pthread_mutex_unlock(&q->queue_lock);
    return e->data;
}

void Queue_delete(Queue *q)
{
    pthread_mutex_lock(&q->queue_lock);
    pthread_mutex_unlock(&q->queue_lock);
    pthread_cond_broadcast(&q->queue_not_empty);
    free(q);
}

void Queue_dump(Queue *q)
{
    pthread_mutex_lock(&q->queue_lock);
    entry_t *e = q->queue_head;
    while (e != NULL) {
        fprintf(stderr, "[%ld]", e->data);
        e = e->next;
    }
    fprintf(stderr, "\n");
    pthread_mutex_unlock(&q->queue_lock);
}

#ifdef __cplusplus
}
#endif
