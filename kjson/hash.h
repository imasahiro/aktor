#include <stdint.h>
#ifndef KJSON_HASH_
#define KJSON_HASH_

static inline uint32_t djbhash(const char *p, uint32_t len)
{
    uint32_t hash = 5381;
    const char *const e = p + len;
    while (p < e) {
        hash = ((hash << 5) + hash) + *p++;
    }
    return (hash & 0x7fffffff);
}

static inline uint32_t hash0(uint32_t hash, const char *p, uint32_t len)
{
    const char *const e = p + len;
    while (p < e) {
        hash = (*p++) + 31 * hash;
    }
    return (hash & 0x7fffffff);
}

#endif /* end of include guard */
