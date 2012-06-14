#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef KJSON_STREAM_H
#define KJSON_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

struct input_stream;
typedef void (*istream_init)(struct input_stream *, void **args);
typedef void (*istream_deinit)(struct input_stream *);
typedef char (*istream_next)(struct input_stream *);
typedef void (*istream_prev)(struct input_stream *, char c);
typedef bool (*istream_eos)(struct input_stream *);

union io_data {
    char *str;
    FILE *fp;
    uintptr_t u;
    void *ptr;
};

typedef struct input_stream {
    union io_data d0;
    union io_data d1;
    istream_next   fnext;
    istream_eos    feos;
    istream_prev   fprev;
    istream_deinit fdeinit;
} input_stream;

typedef const struct input_stream_api_t {
    istream_init   finit;
    istream_next   fnext;
    istream_prev fprev;
    istream_eos    feos;
    istream_deinit fdeinit;
} input_stream_api;

typedef struct input_stream_iterator {
    input_stream *ins;
    istream_next fnext;
    istream_eos  feos;
    istream_prev fprev;
} input_stream_iterator;

input_stream *new_string_input_stream(char *buf, size_t len);
input_stream *new_file_input_stream(char *filename, size_t bufsize);

void input_stream_delete(input_stream *ins);

static inline void input_stream_iterator_init(input_stream *ins, input_stream_iterator *itr)
{
    itr->ins   = ins;
    itr->fnext = ins->fnext;
    itr->feos  = ins->feos;
}

static inline void input_stream_unput(input_stream *ins, char c)
{
    ins->fprev(ins, c);
}

#define for_each_istream(INS, ITR, CUR)\
        input_stream_iterator_init(INS, &ITR);\
        for (CUR = itr.fnext(INS); itr.feos(INS); CUR = itr.fnext(INS))
#define for_each_istream_iterator(ITR, CUR) \
    for (CUR = (ITR)->fnext((ITR)->ins); (ITR)->feos((ITR)->ins);\
            CUR = (ITR)->fnext((ITR)->ins))

#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
