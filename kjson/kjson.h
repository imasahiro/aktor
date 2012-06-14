#include <stdio.h>
#include "numbox.h"

#define USE_NUMBOX
#ifndef KJSON_H_
#define KJSON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum json_type {
    /** ($type & 1 == 0) means $type extends Number */
    JSON_Null   =  6, /* 110 */
    JSON_Bool   =  4, /* 100 */
    JSON_Int    =  2, /* 010 */
    JSON_Double =  0, /* 000 */
    JSON_Object =  3, /* 011 */
    JSON_String =  1, /* 001 */
    JSON_Array  =  5  /* 101 */
} json_type;

union JSON;
typedef union JSON * JSON;

typedef struct JSONString {
#ifndef USE_NUMBOX
    json_type type;
#endif
    int length;
    char *str;
} JSONString;

typedef struct JSONArray {
#ifndef USE_NUMBOX
    json_type type;
#endif
    int  length;
    int  capacity;
    JSON *list;
} JSONArray;

#ifndef USE_NUMBOX
typedef struct JSONNumber {
    json_type type;
    long val;
} JSONNumber;
#else
typedef Value JSONNumber;
#endif

typedef JSONNumber JSONInt;
typedef JSONNumber JSONDouble;
typedef JSONNumber JSONBool;

struct poolmap_t;
typedef struct JSONObject {
#ifndef USE_NUMBOX
    json_type type;
#endif
    struct poolmap_t *child;
} JSONObject;

union JSON {
#ifndef USE_NUMBOX
    struct JSON_base {
        json_type type;
    } base;
#endif
    JSONString str;
    JSONArray  ary;
    JSONNumber num;
    JSONObject obj;
};

#ifdef USE_NUMBOX
typedef JSONNumber JSONNull;
#else
typedef union JSON JSONNull;
#endif

/* [Converter API] */
#define JSON_CONVERTER(T) static inline JSON##T *toJSON##T (JSON o) { return (JSON##T*) o; }
JSON_CONVERTER(Object)
JSON_CONVERTER(Array)
JSON_CONVERTER(String)
JSON_CONVERTER(Bool)
JSON_CONVERTER(Number)
JSON_CONVERTER(Double)
JSON_CONVERTER(Null)
#undef CONVERTER

/* [Getter API] */
unsigned JSON_length(JSON json);
JSON *JSON_getArray(JSON json, char *key, size_t *len);
char *JSON_getString(JSON json, char *key, size_t *len);
double JSON_getDouble(JSON json, char *key);
int JSON_getBool(JSON json, char *key);
int JSON_getInt(JSON json, char *key);
JSON JSON_get(JSON json, char *key);

static inline char *JSONString_get(JSON json, size_t *len)
{
    JSONString *s = toJSONString(json);
#ifdef USE_NUMBOX
    s = toJSONString(toStr(toVal(s)));
#endif
    *len = s->length;
    return s->str;
}

static inline int32_t JSONInt_get(JSON json)
{
#ifdef USE_NUMBOX
    return toInt32(toVal(json));
#else
    return (int32_t)((JSONNumber*) json)->val;
#endif
}

static inline double JSONDouble_get(JSON json)
{
#ifdef USE_NUMBOX
    return toDouble(toVal(json));
#else
    union v { double f; long v; } v;
    v.v = ((JSONNumber*)json)->val;
    return v.f;
#endif
}

static inline int JSONBool_get(JSON json)
{
#ifdef USE_NUMBOX
    return toBool(toVal(json));
#else
    return ((JSONNumber*)json)->val;
#endif
}

/* [New API] */
JSON JSONNull_new();
JSON JSONArray_new();
JSON JSONObject_new();
JSON JSONDouble_new(double val);
JSON JSONString_new(char *s, size_t len);
JSON JSONInt_new(long val);
JSON JSONBool_new(long val);

/* [Other API] */
void JSONObject_set(JSONObject *o, JSON key, JSON value);
void JSON_free(JSON o);
void JSON_dump(FILE *fp, JSON json);
JSON parseJSON(char *s, char *e);
JSON parseJSON_fromFILE(char *file);

#ifdef USE_NUMBOX
static inline json_type JSON_type(JSON json) {
    Value v; v.bits = (uint64_t)json;
    uint64_t tag = Tag(v);
    return (IsDouble((v)))?
        JSON_Double : (enum json_type) ((tag >> TagBitShift) & 7);
}
#define JSON_set_type(json, type) assert(0 && "Not supported")
#else
#define JSON_type(json) (((JSON) (json))->base.type)
#define JSON_set_type(json, T) (((JSON) (json))->base.type) = T
#endif
#define JSON_TYPE_CHECK(T, O) (JSON_type(((JSON)O)) == JSON_##T)
#ifdef USE_NUMBOX
#define ARRAY_INIT(A) ((A = toJSONArray(toAry(toVal((JSON)A)))) != NULL)
#else
#define ARRAY_INIT(A) (A)
#endif

#define JSON_ARRAY_EACH(A, I, E) \
    if (!JSON_TYPE_CHECK(Array, (JSON)A)) {} else\
        if (!ARRAY_INIT(A)) {}\
        else\
        for (I = (A)->list,\
            E = (A)->list+(A)->length; I < E; ++I)

typedef struct JSONObject_iterator {
    long index;
    JSONObject *obj;
} JSONObject_iterator;

int JSONObject_iterator_init(JSONObject_iterator *itr, JSONObject *obj);
JSONString *JSONObject_iterator_next(JSONObject_iterator *itr, JSON *val);

#define OBJECT_INIT(O, ITR) (JSONObject_iterator_init(ITR, O))
#define JSON_OBJECT_EACH(O, ITR, KEY, VAL)\
    if (!JSON_TYPE_CHECK(Object, (JSON)O)) {} else\
    if (!OBJECT_INIT(O, &ITR)) {}\
    else\
    for (KEY = JSONObject_iterator_next(&ITR, &VAL); KEY;\
            KEY = JSONObject_iterator_next(&ITR, &VAL))

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
