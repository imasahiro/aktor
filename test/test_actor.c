#include "actor.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char const* argv[])
{
    if (ActorStage_init(4, 4)) {
        fprintf(stderr, "ActorStage initialization failed.\n");
        return 1;
    }
    fprintf(stderr, "%ld\n", sizeof(ActorStage));
    fprintf(stderr, "%ld\n", sizeof(Actor));
    ActorStage_finish();
    return 0;
}

#ifdef __cplusplus
}
#endif
