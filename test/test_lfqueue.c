#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include "lfqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

static int test_parallel(void);

Queue *q;
int main(int argc, char const* argv[])
{
    int i, max = 1000;
    q = Queue_new();
    ThreadContext ctx = {0, NULL};
    for (i = 0; i < max; i++) {
        Queue_enq(&ctx, q, i);
#if 0
        Queue_dump(q);
#endif
    }

    for (i = 0; i < max; i++) {
        Data d = Queue_deq(&ctx, q);
        assert(i == d);
#if 0
        Queue_dump(q);
#endif
    }
    test_parallel();
    return 0;
}

#define MAX 20000000
#define CORE 8
static inline void atomic_add(volatile long *v, int n)
{
    __sync_fetch_and_add(v, n);
}
static inline void atomic_sub(volatile long *v, int n)
{
    __sync_fetch_and_sub(v, n);
}
static volatile long counter = 0;

Queue *lfqueue;
#define enqueue(Q, D) Queue_enq(&ctx, Q, D)
#define dequeue(Q, D) Queue_deq(&ctx, Q)
void *e(void *arg)
{
    long offset = (long) arg;
    long i;
    ThreadContext ctx = {};
    for(i=0 ;i<MAX/CORE; i++){
        long v = i + offset;
        enqueue(lfqueue, v);
        atomic_sub(&counter, v);
    }
    return NULL;
}

void *d(void *arg)
{
    void *v = NULL;
    ThreadContext ctx = {};
    while (!Queue_isEmpty(lfqueue)) {
        dequeue(lfqueue, &v);
        atomic_add(&counter, (long) v);
    }
    return NULL;
}

static inline uint64_t gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int test_parallel(void)
{
    lfqueue = Queue_new();
    int64_t enquestart,enqueend,dequestart,dequeend;
    pthread_t threads[CORE];
    printf("%d thread initialized, %d items  \n",CORE, MAX);
    printf("lockfree\t|");
    // lockfree queue start
    enquestart = gettime();
    int i;
    for(i = 0; i < CORE; i++){
        pthread_create(&threads[i], NULL, e, (void*)(0L+i*MAX/CORE));
    }
    for (i = 0; i < CORE; ++i) {
        pthread_join(threads[i], NULL);
    }
    enqueend = gettime();
    printf("  enque: %lld    ", enqueend - enquestart);
    dequestart = gettime();
    for(i = 0; i < CORE; i++){
        pthread_create(&threads[i], NULL, d, (void*)(0L+i));
    }
    for (i = 0; i < CORE; ++i) {
        pthread_join(threads[i], NULL);
    }
    dequeend = gettime();
    printf(" deque: %lld", dequeend - dequestart);
    printf(" counter=%ld\n", counter);
    assert(Queue_isEmpty(lfqueue));
#if 1
    counter = 0;
    printf("sequencial\t|");
    enquestart = gettime();
    for (i = 0; i < CORE; ++i) {
        e((void*)((uintptr_t)i*MAX/CORE));
    }
    enqueend = gettime();
    printf("  enque: %lld    ", enqueend - enquestart);
    dequestart = gettime();

    for (i = 0; i < CORE; ++i) {
        d(0);
    }
    dequeend = gettime();
    printf(" deque: %lld", dequeend - dequestart);
    printf(" counter=%ld\n", counter);
#endif
    return 0;
}
#ifdef __cplusplus
}
#endif
