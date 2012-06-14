#include "kjson/kjson.h"
#include "sched.h"
#include "queue.h"

#ifndef ACTOR_H
#define ACTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define __shared__
#define __per_thread__

typedef struct ActorStage {
    Scheduler __per_thread__ **sched;
} ActorStage;

enum actor_stage_wait {
    wait_nop = 0,
    wait_node,
    wait_thread,
    wait_cores,
    wait_failed = -1
};

int ActorStage_init(int max_cores, int threads_per_core);
int ActorStage_finish(void);
int ActorStage_wait(enum actor_stage_wait wait, int val);

typedef union JSON Message;

struct Actor;
struct actor_api {
    int  (*act)(struct Actor *, Message *);
    void (*finit)(struct Actor *);
    void (*fexit)(struct Actor *);
};

typedef Queue MailBox;
typedef struct Actor {
    const struct actor_api *api;
    MailBox *mailbox;
    JSON self;
} Actor;

Actor *Actor_new(JSON o, const struct actor_api *api);
void Actor_finalize(Actor *a);
void Actor_act(Actor *a);
void Actor_send(Actor *a, JSON message);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
