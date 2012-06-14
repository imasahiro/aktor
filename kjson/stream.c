#include "stream.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline input_stream *new_input_stream(void **args, input_stream_api *api)
{
    input_stream *ins = (input_stream *) calloc(1, sizeof(input_stream));
    api->finit(ins, args);
    ins->fdeinit = api->fdeinit;
    ins->fnext   = api->fnext;
    ins->fprev   = api->fprev;
    ins->feos    = api->feos;
    return ins;
}

static void string_input_stream_init(input_stream *ins, void **args)
{
    char *text;
    size_t len;
    text = (char *) args[0];
    len  = (size_t) args[1];
    ins->d0.str = text;
    ins->d1.str = text + len + 1;
}

static char string_input_stream_next(input_stream *ins)
{
    return *(ins->d0.str)++;
}

static void string_input_stream_prev(input_stream *ins, char c)
{
    --(ins->d0.str);
    *ins->d0.str = c;
}

static bool string_input_stream_eos(input_stream *ins)
{
    return ins->d0.str != ins->d1.str;
}

static void string_input_stream_deinit(input_stream *ins)
{
    ins->d0.str = ins->d1.str = NULL;
}

struct file_data {
    char *head;
    char *tail;
    size_t len;
    char buffer[];
};

static void file_input_stream_init(input_stream *ins, void **args)
{
    struct file_data *f;
    size_t bufsize = (size_t) args[1];
    f = (struct file_data *) malloc(bufsize+sizeof(char*)*3);
    ins->d0.str = (char*) fopen((char *) args[0], "r");
    ins->d1.ptr = (void*) f;
    f->head = f->buffer;
    f->tail = f->buffer;
    f->len  = bufsize;
}

static char file_input_stream_next(input_stream *ins)
{
    struct file_data *f = (struct file_data *) ins->d1.ptr;
    if (f->head < f->tail) {
        assert(*(f->head) != 0);
        return *(f->head)++;
    }
    size_t len = fread(f->buffer, 1, f->len, ins->d0.fp);
    f->head = f->buffer;
    f->tail = f->buffer + len;
    return *(f->head)++;
}

static void file_input_stream_prev(input_stream *ins, char c)
{
    struct file_data *f = (struct file_data *) ins->d1.ptr;
    assert(f->head != f->buffer && "TODO");
    --(f->head);
    *f->head = c;
}

static bool file_input_stream_eos(input_stream *ins)
{
    struct file_data *f = (struct file_data *) ins->d1.ptr;
    return f->head <= f->tail || !feof(ins->d0.fp);
}

static void file_input_stream_deinit(input_stream *ins)
{
    fclose(ins->d0.fp);
    free(ins->d1.ptr);
}

input_stream *new_string_input_stream(char *buf, size_t len)
{
    void *args[] = {
        (void*)buf, (void*)len
    };
    static const input_stream_api string_input_stream_api = {
        string_input_stream_init,
        string_input_stream_next,
        string_input_stream_prev,
        string_input_stream_eos,
        string_input_stream_deinit
    };
    input_stream *ins = new_input_stream(args, &string_input_stream_api);
    return ins;
}

input_stream *new_file_input_stream(char *filename, size_t bufsize)
{
    void *args[] = {
        (void*)filename, (void*)bufsize
    };
    static const input_stream_api file_input_stream_api = {
        file_input_stream_init,
        file_input_stream_next,
        file_input_stream_prev,
        file_input_stream_eos,
        file_input_stream_deinit
    };
    input_stream *ins = new_input_stream(args, &file_input_stream_api);
    return ins;
}

void input_stream_delete(input_stream *ins)
{
    ins->fdeinit(ins);
    free(ins);
}

#ifdef __cplusplus
}
#endif
