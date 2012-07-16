#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "actor.h"

#ifdef __cplusplus
extern "C" {
#endif

void my_actor_init(Actor *a) { /* do nothing */ }
void my_actor_exit(Actor *a) { /* do nothing */ }

int my_actor_act(Actor *a, Message *message)
{
    if (JSON_type(message) == JSON_Int32) {
        fprintf(stderr, "actor:%d, %d\n",
                JSONInt_get(a->self),
                JSONInt_get(message));
    }
    fprintf(stderr, "hi\n");
    return 0;
}

const struct actor_api my_actor_api = {
    my_actor_act,
    my_actor_init,
    my_actor_exit
};

int main(int argc, char const* argv[])
{
    if (ActorStage_init(4, 4)) {
        fprintf(stderr, "ActorStage initialization failed.\n");
        return 1;
    }
    fprintf(stderr, "%ld\n", sizeof(ActorStage));
    fprintf(stderr, "%ld\n", sizeof(Actor));

    char data[128];
    char data1[] = "1024";
    snprintf(data, 128, "%d", 10);
    JSON json = parseJSON(data, data+strlen(data));
    JSON message = parseJSON(data1, data1+strlen(data1));
    Actor *a0 = Actor_new(json, &my_actor_api);
    Actor_act(a0);
    Actor_send(a0, message);
    while (!Queue_isEmpty(a0->mailbox)) {
        usleep(1);
    }
    Actor_finalize(a0);
    ActorStage_finish();
    return 0;
}

#ifdef __cplusplus
}
#endif
