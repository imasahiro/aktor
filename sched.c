#include "lfqueue.h"
#include "sched.h"
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int  default_run(Task *t) { return 0; }
void default_destruct(Task *t) { (void)t; }

typedef struct JoinTask {
    struct Task base;
} JoinTask;

int  join_run(Task *t) { *t->join = 1; return 0; }
void join_destruct(Task *t) { (void)t; }

struct task_api join_task_api = {
    join_run,
    join_destruct
};

typedef struct SchedulerWorker {
    pthread_t thread;
    Queue *q;
    int join;
} Worker;

struct Scheduler {
    ThreadContext ctx;
    size_t active_workers;
    Queue   *q;
    Worker **workers;
};

Task *Task_new(void *data, struct task_api *api)
{
    Task *t = (Task *) calloc(1, sizeof(Task));
    t->join = 0;
    t->data = data;
    memcpy(&t->api, api, sizeof(struct task_api));
    return t;
}

static void Task_delete(Task *t)
{
    t->api.destruct(t->data);
    free(t);
}

static Worker *Worker_new(Queue *q)
{
    Worker *w = (Worker *) calloc(1, sizeof(Worker));
    w->join = 0;
    w->q    = q;
    return w;
}

static void Worker_delete(Worker *w)
{
    free(w);
}

static void *Worker_exec(void *arg);

Scheduler *Scheduler_new(unsigned max_workers)
{
    unsigned i;
    if (max_workers) {
    }
    Scheduler *s = (Scheduler *) calloc(1, sizeof(Scheduler));
    s->q = Queue_new(1);
    assert(max_workers > 0);
    s->workers = (Worker **) calloc(1, sizeof(Worker*) * max_workers);
    for (i = 0; i < max_workers; ++i) {
        Worker *w = Worker_new(s->q);
        pthread_create(&w->thread, NULL, Worker_exec, w);
        s->workers[i] = w;
        ++s->active_workers;
    }
    return s;
}

void Scheduler_delete(Scheduler *sched)
{
    if (sched->active_workers) {
        Scheduler_join(sched);
    }
}

void Scheduler_enqueue(Scheduler *s, Task *task)
{
    ThreadContext *ctx = Queue_getContext(s->q);
    Queue_enq(ctx, s->q, (Data) task);
}

void Scheduler_join(Scheduler *s) {
    size_t i;
    ThreadContext *ctx = Queue_getContext(s->q);
    for (i = 0; i < s->active_workers; ++i) {
        JoinTask *task = (JoinTask *) Task_new(0, &join_task_api);
        Queue_enq(ctx, s->q, (Data) task);
    }
    for (i = 0; i < s->active_workers; ++i) {
        Worker *w = s->workers[i];
        pthread_join(w->thread, NULL);
        Worker_delete(w);
        s->workers[i] = NULL;
    }
    s->active_workers = 0;
}

static void *Worker_exec(void *arg)
{
    Worker *w = (Worker*)(arg);
    ThreadContext ctx = {0, NULL};
    Queue *q = w->q;
    while (!w->join) {
        Task *t;
        if ((t = (Task*)Queue_deq(&ctx, q)) != NULL) {
            t->join = &w->join;
            if (t->api.run(t)) {
                Queue_enq(&ctx, q, (Data) t);
            } else {
                Task_delete(t);
            }
        }
    }
    return NULL;
}
#ifdef __cplusplus
}
#endif
