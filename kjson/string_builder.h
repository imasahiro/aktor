#include "array.h"
#include <string.h>

#ifndef KJSON_STRING_BUILDER_H_
#define KJSON_STRING_BUILDER_H_

DEF_ARRAY_STRUCT0(char, int);
DEF_ARRAY_T(char);
DEF_ARRAY_OP_NOPOINTER(char);

typedef struct string_builder {
    ARRAY(char) buf;
} string_builder;

static inline void string_builder_init(string_builder *builder)
{
    ARRAY_init(char, &builder->buf, 4);
}

static inline void string_builder_add(string_builder *builder, char c)
{
    ARRAY_add(char, &builder->buf, c);
}

static inline char *string_builder_tostring(string_builder *builder,
        size_t *len, int ensureZero)
{
    if (ensureZero) {
        ARRAY_add(char, &builder->buf, '\0');
    }
    char *list = builder->buf.list;
    *len = (size_t) builder->buf.size;
    builder->buf.list     = NULL;
    builder->buf.size     = 0;
    builder->buf.capacity = 0;
    return list;
}

#endif /* end of include guard */
