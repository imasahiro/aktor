#include "actor.h"
#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

static __shared__ ActorStage *global_stage;
static __shared__ unsigned global_max_cores = 0;

int ActorStage_init(int max_cores, int threads_per_core)
{
    if (max_cores < 0) {
        max_cores = 1;
    }
    if (threads_per_core < 0) {
        threads_per_core = 1;
    }
    size_t size = sizeof(Scheduler*) * max_cores;
    ActorStage *stage = (ActorStage *) calloc(1, sizeof(ActorStage));
    stage->sched = (Scheduler **) calloc(1, size);
    int i;
    for (i = 0; i < max_cores; i++) {
        stage->sched[i] = Scheduler_new(threads_per_core);
    }
    global_stage = stage;
    global_max_cores = max_cores;
    return 0;
}

int ActorStage_finish(void)
{
    ActorStage *stage = global_stage;
    int i;
    for (i = 0; i < global_max_cores; i++) {
        Scheduler_delete(stage->sched[i]);
        stage->sched[i] = NULL;
    }
    free(stage->sched);
    free(stage);
    return 0;
}

int ActorStage_wait(enum actor_stage_wait wait, int val)
{
    return 0;
}

Actor *Actor_new(JSON o, const struct actor_api *api)
{
    Actor *a = (Actor *) calloc(1, sizeof(Actor));
    a->api = api;
    a->mailbox = Queue_new(1);
    a->self = o;
    api->finit(a);
    return a;
}

void Actor_finalize(Actor *a)
{
    a->api->fexit(a);
    JSON_free((JSON)a->self);
    Queue_delete(a->mailbox);
    free(a);
}

int actor_task_run(Task *t)
{
    ThreadContext *ctx;
    Actor *a = (Actor *) TASK_DATA(t);
    Queue *q = a->mailbox;
    int res = 1;
    if (!Queue_isEmpty(q)) {
        ctx = Queue_getContext(q);
        JSON message = (JSON) Queue_deq(ctx, q);
        res = a->api->act(a, message);
        JSON_free(message);
    }
    return res;
}

void actor_task_destruct(Task *t)
{
}

struct task_api actor_task_api = {
    actor_task_run,
    actor_task_destruct
};

/*
 * sched_id is used for task dispaching. sched_id simulates
 * round robin dispatch and may be shared by all of threads
 * but I do not care.
 */
static __shared__ int sched_id = 0;
void Actor_act(Actor *a)
{
    ActorStage *stage = global_stage;
    sched_id = (sched_id+1) & (global_max_cores-1);
    Scheduler_enqueue(stage->sched[sched_id],
            Task_new((void*) a, &actor_task_api));
}

void Actor_send(Actor *a, JSON message)
{
    Queue *q = a->mailbox;
    ThreadContext *ctx = Queue_getContext(q);
    Queue_enq(ctx, q, (Data)message);
}

#ifdef __cplusplus
}
#endif
