#ifndef SCHED_H_
#define SCHED_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Task;
struct task_api {
    void (*run)(struct Task *task);
    void (*destruct)(struct Task *task);
};

typedef struct Task {
    struct task_api api;
    void *data;
    int  *join;
} Task;

struct Task *Task_new(void *data, struct task_api *api);

struct Scheduler;
typedef struct Scheduler Scheduler;
struct Scheduler *Scheduler_new(unsigned max_workers);
void Scheduler_delete(struct Scheduler *sched);
void Scheduler_enqueue(struct Scheduler *sched, struct Task *task);
void Scheduler_join(struct Scheduler *sched);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* end of include guard */
