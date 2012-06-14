#include "sched.h"
#include <stdio.h>

struct MyTask {
    Task base;
};

#define TASK_DATA(TASK) ((Task*)(TASK)->data)
void mytask_run(Task *t)
{
    long data = (long) TASK_DATA(t);
    fprintf(stderr, "%ld\n", data);
}

void mytask_destruct(Task *t) {
}

struct task_api mytask_api = {
    mytask_run,
    mytask_destruct
};

#define MAX 20000
#define CORE 4
int main(int argc, char const* argv[])
{
    Scheduler *sched = Scheduler_new(CORE);
    long i;
    for (i = 0; i < MAX; ++i) {
        Scheduler_enqueue(sched, Task_new((void*)i, &mytask_api));
    }
    Scheduler_delete(sched);
    return 0;
}
