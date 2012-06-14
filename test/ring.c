#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "actor.h"

#ifdef __cplusplus
extern "C" {
#endif

static void ring_actor_init(Actor *a) { /* do nothing */ }
static void ring_actor_exit(Actor *a) { /* do nothing */ }

Actor **node_list;
static int ring_actor_act(Actor *a, Message *message)
{
    if (JSON_type(message) == JSON_Int) {
        int index = JSONInt_get(a->self);
        Actor *next = node_list[index-1];
        fprintf(stderr, "actor: %d=>%d\n", index, index-1);
        Actor_send(next, JSONInt_new(index-1));
    }
    return 0;
}

int ring_actor_act_2(Actor *a, Message *message)
{
    if (JSON_type(message) == JSON_Int) {
        int *data = (int *) a->self;
        *data = 1;
    }
    return 0;
}

static const struct actor_api ring_actor_api = {
    ring_actor_act,
    ring_actor_init,
    ring_actor_exit
};

static const struct actor_api ring_finish_actor_api = {
    ring_actor_act_2,
    ring_actor_init,
    ring_actor_exit
};

int main(int argc, char const* argv[])
{
    if (ActorStage_init(4, 4)) {
        fprintf(stderr, "ActorStage initialization failed.\n");
        return 1;
    }

    int i, finish = 0;
    static const int node_size = 24;
    JSON message = JSONInt_new(node_size);
    node_list = (Actor **) malloc(sizeof(Actor*)*node_size);
    for (i = 1; i < node_size; i++) {
        JSON json = JSONInt_new(i);
        node_list[i] = Actor_new(json, &ring_actor_api);
        Actor_act(node_list[i]);
    }
    node_list[0] = Actor_new((JSON)&finish, &ring_finish_actor_api);
    Actor_act(node_list[0]);
    Actor_send(node_list[node_size-1], message);

    while (finish == 0) {
        mfence();
    }
    for (i = 0; i < node_size; i++) {
        Actor_finalize(node_list[i]);
    }
    ActorStage_finish();
    return 0;
}

#ifdef __cplusplus
}
#endif
