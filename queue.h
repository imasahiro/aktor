/****************************************************************************
 * Copyright (c) 2012, Masahiro Ide. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include <stdint.h>
#include <string.h>
#ifndef LFQUEUE_H_
#define LFQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t Data;
struct Node;

typedef struct ThreadContext {
    int myver;
    struct Node *mynode;
} ThreadContext;

struct Queue;
typedef struct Queue Queue;

Queue *Queue_new(int thread_context_malloc);
void Queue_enq(ThreadContext *ctx, Queue *q, Data d);
Data Queue_deq(ThreadContext *ctx, Queue *q);
void Queue_dump(Queue *q);
int  Queue_isEmpty(Queue *q);
void Queue_delete(Queue *q);

ThreadContext *Queue_getContext(Queue *q);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
