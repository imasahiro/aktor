#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "lfqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EntryTag {
    int32_t ver;
    int32_t count;
} EntryTag;

typedef struct ExitTag {
    int16_t count;
    int16_t outstandingTransfers;
    int8_t nlP;
    int8_t toBeFreed;
} __attribute__((aligned(8))) ExitTag;

typedef struct Node {
    struct Node *pred;
    struct Node *next;
    Data d;
    ExitTag exit;
} Node;

typedef struct Loc {
    Node *ptr0;
    Node *ptr1;
    EntryTag entry;
} Loc;

struct Queue {
    Loc Tail;
    Loc Head;
};

#define CURRENT(loc, ver)        ((ver % 2 == 0)?( (loc)->ptr0):( (loc)->ptr1))
#define NONCURRENT(loc, ver)     ((ver % 2 != 0)?( (loc)->ptr0):( (loc)->ptr1))
#define NONCURRENTADDR(loc, ver) ((ver % 2 != 0)?(&(loc)->ptr0):(&(loc)->ptr1))
#define EXITTAG_INIT(e) do {\
    e.count = 0;\
    e.outstandingTransfers = 2;\
    e.nlP = 0;\
    e.toBeFreed = 0;\
} while (0)

#define CLEAN(exit) (exit.count == 0 && exit.outstandingTransfers == 0)
#define FREEABLE(exit) (CLEAN(exit) && exit.nlP && exit.toBeFreed)

#define ToU64(v) (*((uint64_t*)&v))

static Node *Node_new(Data d);
static void Node_delete(Node *node);

static inline bool CAS(uint64_t *ptr, uint64_t oldv, uint64_t newv)
{
    uint64_t result = __sync_val_compare_and_swap(ptr, oldv, newv);
    return (result == oldv);
}

static Node *LL(Loc *loc, int *myver, Node **mynode)
{
    EntryTag e, newe;
    do {
        e = loc->entry;
        *myver  = e.ver;
        *mynode = CURRENT(loc, e.ver);
        newe.ver   = e.ver;
        newe.count = e.count + 1;
    } while (!CAS((uint64_t*)&loc->entry, ToU64(e), ToU64(newe)));
    return (*mynode);
}

static void Transfer(Node *node, int count)
{
    ExitTag exit, post;
    do {
        exit = node->exit;
        post.count = exit.count + count;
        post.outstandingTransfers = exit.outstandingTransfers - 1;
        post.nlP = exit.nlP;
        post.toBeFreed = exit.toBeFreed;
    } while (!CAS((uint64_t*)&node->exit, ToU64(exit), ToU64(post)));
}

static void SetNLPred(Node *node)
{
    ExitTag exit, post;
    do {
        exit = node->exit;
        post.count = exit.count;
        post.outstandingTransfers = exit.outstandingTransfers;
        post.nlP = true;
        post.toBeFreed = exit.toBeFreed;
    } while (!CAS((uint64_t*)&node->exit, ToU64(exit), ToU64(post)));
    if (FREEABLE(post)) {
        Node_delete(node);
    }
}

static void SetToBeFreed(Node *node)
{
    ExitTag exit, post;
    do {
        exit = node->exit;
        post.count = exit.count;
        post.outstandingTransfers = exit.outstandingTransfers;
        post.nlP = exit.nlP;
        post.toBeFreed = true;
    } while (!CAS((uint64_t*)&node->exit, ToU64(exit), ToU64(post)));
    if (FREEABLE(post)) {
        Node_delete(node);
    }
}

static void Release(Node *node)
{
    Node *pred = node->pred;
    ExitTag exit, post;
    do {
        exit = node->exit;
        post.count = exit.count - 1;
        post.outstandingTransfers = exit.outstandingTransfers;
        post.nlP = exit.nlP;
        post.toBeFreed = exit.toBeFreed;
    } while (!CAS((uint64_t*)&node->exit, ToU64(exit), ToU64(post)));
    if (CLEAN(post)) {
        SetNLPred(pred);
    }
    if (FREEABLE(post)) {
        Node_delete(node);
    }
}

static void Unlink(Loc *loc, int myver, Node *mynode)
{
    Release(mynode);
}

static bool SC(Loc *loc, Node *node, int myver, Node *mynode)
{
    EntryTag e, newe;
    Node *pred = mynode->pred;
    bool success = CAS((uint64_t*)NONCURRENTADDR(loc, myver), (uint64_t)pred, (uint64_t)node);

    e = loc->entry;
    while (e.ver == myver) {
        newe.ver   = e.ver + 1;
        newe.count = 0;
        if (CAS((uint64_t*)&loc->entry, ToU64(e), ToU64(newe))) {
            Transfer(mynode, e.count);
        }
        e = loc->entry;
    }
    Release(mynode);
    return success;
}

static Node *Node_new(Data d)
{
    Node *n = (Node *) malloc(sizeof(Node));
    n->next = NULL;
    n->d    = d;
    EXITTAG_INIT(n->exit);
    return n;
}

static void Node_delete(Node *n)
{
    free(n);
}

void Queue_enq(ThreadContext *ctx, Queue *q, Data d)
{
    Node *node = Node_new(d);
    while (true) {
        Node *tail = LL(&q->Tail, &ctx->myver, &ctx->mynode);
        node->pred = tail;
        if (CAS((uint64_t*)&tail->next, (uint64_t)NULL, (uint64_t)node)) {
            SC(&q->Tail, node, ctx->myver, ctx->mynode);
            break;
        } else {
            SC(&q->Tail, tail->next, ctx->myver, ctx->mynode);
        }
    }
}

Data Queue_deq(ThreadContext *ctx, Queue *q)
{
    while (true) {
        Node *head = LL(&q->Head, &ctx->myver, &ctx->mynode);
        Node *next = head->next;
        if (next == NULL) {
            Unlink(&q->Head, ctx->myver, ctx->mynode);
            return 0;
        }
        if (SC(&q->Head, next, ctx->myver, ctx->mynode)) {
            Data d = next->d;
            SetToBeFreed(next);
            return d;
        }
    }
}

Queue *Queue_new(void)
{
    Queue *q = malloc(sizeof(*q));
    q->Tail.entry.ver = 0;
    q->Tail.entry.count = 0;
    q->Tail.ptr0 = Node_new(0);
    q->Tail.ptr1 = Node_new(0);
    q->Tail.ptr0->pred = q->Tail.ptr1;
    EXITTAG_INIT(q->Tail.ptr0->exit);
    q->Tail.ptr0->next = NULL;
    q->Tail.ptr1->exit.count = 0;
    q->Tail.ptr1->exit.outstandingTransfers = 0;
    q->Tail.ptr1->exit.nlP = 0;
    q->Tail.ptr1->exit.toBeFreed = 0;
    q->Head = q->Tail;

    assert(sizeof(EntryTag) <= sizeof(uint64_t));
    assert(sizeof(ExitTag) <= sizeof(uint64_t));
    return q;
}

void Queue_dump(Queue *q)
{
    const Loc *loc = &q->Head;
    Node *cur = CURRENT(loc, loc->entry.ver);
    while ((cur = cur->next) != NULL) {
        fprintf(stderr, "[%ld]", cur->d);
    }
    fprintf(stderr, "\n");
}

int Queue_isEmpty(Queue *q)
{
    const Loc *loc = &q->Head;
    Node *cur = CURRENT(loc, loc->entry.ver);
    return cur->next == NULL;
}

void Queue_delete(Queue *q)
{
    bzero(q, sizeof(*q));
    free(q);
}

#ifdef __cplusplus
}
#endif
