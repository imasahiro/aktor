#include "actor.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static __shared__ ActorStage *global_stage;
static __shared__ int global_max_cores = -1;

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

Actor *Actor_new(JSONObject *o, const struct actor_api *api)
{
    Actor *a = (Actor *) calloc(1, sizeof(Actor));
    a->api = api;
    a->mailbox = Queue_new();
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

#ifdef __cplusplus
}
#endif
