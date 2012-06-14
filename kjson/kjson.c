#include "kjson.h"
#include "stream.h"
#include "string_builder.h"
#include "map.h"
#include "hash.h"

#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

static uintptr_t json_keygen1(char *key, uint32_t klen)
{
    (void)klen;
    return (uintptr_t) key;
}

static uintptr_t json_keygen0(char *key, uint32_t klen)
{
    JSONString *s = toJSONString((JSON)key);
    (void)klen;
#ifdef USE_NUMBOX
    s = toJSONString(toStr(toVal(s)));
#endif
    return djbhash(s->str, s->length);
}

static int json_keycmp(uintptr_t k0, uintptr_t k1)
{
    JSONString *s0 = toJSONString((JSON)k0);
    JSONString *s1 = toJSONString((JSON)k1);
#ifdef USE_NUMBOX
    s0 = toJSONString(toStr(toVal(s0)));
    s1 = toJSONString(toStr(toVal(s1)));
#endif
    return s0->length == s1->length &&
        strncmp(s0->str, s1->str, s0->length) == 0;
}

static void json_recfree(pmap_record_t *r)
{
    JSON json = (JSON) r->v;
#ifdef USE_NUMBOX
    if (IsDouble((toVal(json))))
        return;
    if ((JSON_type(json) & 0x1) == 0x0)
        return;
    json = (JSON) toObj(toVal(json));
#endif
    JSON_free(json);
    free(json);
}

#define JSON_NEW(T) (JSON##T *) JSON_new(JSON_##T)

static inline JSON JSON_new(kjson_type type)
{
    JSON json = (JSON) calloc(1, sizeof(union JSON));
#ifndef USE_NUMBOX
    JSON_set_type(json, type);
#endif
    return json;
}

static inline JSON toJSON(Value v) { return (JSON) v.pval; }

static JSON JSONString_new2(string_builder *builder)
{
    JSONString *o = JSON_NEW(String);
    o->str = string_builder_tostring(builder, (size_t*)&o->length, 1);
    o->length -= 1;
#ifdef USE_NUMBOX
    return toJSON(ValueS((JSON)o));
#else
    return (JSON) o;
#endif
}

JSON JSONString_new(char *s, size_t len)
{
    string_builder sb; string_builder_init(&sb);
    char *const e = s + len;
    while (s < e) {
        string_builder_add(&sb, *s++);
    }
    return JSONString_new2(&sb);
}

JSON JSONNull_new()
{
#ifdef USE_NUMBOX
    return toJSON(ValueN());
#else
    return JSON_NEW(Null);
#endif
}

JSON JSONObject_new()
{
    JSONObject *o = JSON_NEW(Object);
    o->child = poolmap_new(0, json_keygen0,
            json_keygen1, json_keycmp, json_recfree);
#ifdef USE_NUMBOX
    o = toJSONObject(toJSON(ValueO((JSON)o)));
#endif
    return (JSON) o;
}

JSON JSONArray_new()
{
    JSONArray *o = JSON_NEW(Array);
    o->length   = 0;
    o->capacity = 0;
    o->list   = NULL;
#ifdef USE_NUMBOX
    o = toJSONArray(toJSON(ValueA((JSON)o)));
#endif
    return (JSON) o;
}

JSON JSONDouble_new(double val)
{
    JSONNumber *o;
#ifndef USE_NUMBOX
    union v { double f; long v; } v;
    v.f = val;
    o = JSON_NEW(Double);
    o->val = v.v;
    JSON_set_type(o, JSON_Double);
#else
    o = (JSONNumber *) ValueF(val).pval;
#endif
    return (JSON) o;
}

JSON JSONInt_new(long val)
{
    JSONNumber *o;
#ifndef USE_NUMBOX
    o = JSON_NEW(Int);
    o->val = val;
    JSON_set_type(o, JSON_Int);
#else
    o = (JSONNumber *) ValueI(val).pval;
#endif
    return (JSON) o;
}

JSON JSONBool_new(long val)
{
    JSONNumber *o;
#ifndef USE_NUMBOX
    o = JSON_NEW(Bool);
    o->val = val;
    JSON_set_type(o, JSON_Bool);
#else
    o = (JSONNumber *) ValueB(val).pval;
#endif
    return (JSON) o;
}

static void JSONObject_free(JSON json)
{
    JSONObject *o = toJSONObject(json);
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    poolmap_delete(o->child);
}

static void JSONString_free(JSON json)
{
    JSONString *o = toJSONString(json);
#ifdef USE_NUMBOX
    o = toJSONString(toStr(toVal((JSON)o)));
#endif
    free(o->str);
}

static void JSONArray_free(JSON o)
{
    JSONArray *a = toJSONArray(o);
    JSON *s, *e;
    JSON_ARRAY_EACH(a, s, e) {
        JSON_free(*s);
    }
    free(a->list);
}

void JSON_free(JSON o)
{
#ifdef USE_NUMBOX
    if (IsDouble((toVal(o))))
        return;
#endif
    switch (JSON_type(o)) {
        case JSON_Object:
            JSONObject_free(o);
            break;
        case JSON_String:
            JSONString_free(o);
            break;
        case JSON_Array:
            JSONArray_free(o);
            break;
        default:
            break;
    }
}

static void JSONArray_append(JSONArray *a, JSON o)
{
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    if (a->length + 1 >= a->capacity) {
        uint32_t newsize = 1 << SizeToKlass(a->capacity * 2 + 1);
        a->list = realloc(a->list, newsize * sizeof(JSON));
        a->capacity = newsize;
    }
    a->list[a->length++] = o;
}

static void _JSONObject_set(JSONObject *o, JSONString *key, JSON value)
{
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    assert(key && value);
    assert(JSON_type(value) < 8);
    poolmap_set(o->child, (char *) key, 0, value);
}

void JSONObject_set(JSONObject *o, JSON key, JSON value)
{
    assert(JSON_TYPE_CHECK(Object, o));
    assert(JSON_TYPE_CHECK(String, key));
    _JSONObject_set(o, toJSONString(key), value);
}

/* Parser functions */
#define NEXT(itr) itr->fnext(itr->ins)
#define EOS(itr)  itr->feos(itr->ins)
static JSON parseNull(input_stream_iterator *itr, char c);
static JSON parseNumber(input_stream_iterator *itr, char c);
static JSON parseBoolean(input_stream_iterator *itr, char c);
static JSON parseObject(input_stream_iterator *itr, char c);
static JSON parseArray(input_stream_iterator *itr, char c);
static JSON parseString(input_stream_iterator *itr, char c);

static char skip_space(input_stream_iterator *itr, char c)
{
    if (c)
        goto L_top;
    for_each_istream_iterator(itr, c) {
        L_top:;
        switch (c) {
            case ' ':
            case '\r':
            case '\n':
            case '\t':
                continue;
            default:
                return c;
        }
    }
    return -1;
}

static JSON parseChild(input_stream_iterator *itr, char c)
{
    c = skip_space(itr, c);
    switch (c) {
        case '{':
            return parseObject(itr, c);
        case '"':
            return parseString(itr, c);
        case '[':
            return parseArray(itr, c);
        case '0':case '1':case '2':case '3':case '4':
        case '5':case '6':case '7':case '8':case '9':
        case '-':
            return parseNumber(itr, c);
        case 't':case 'f':
            return parseBoolean(itr, c);
        case 'n':
            return parseNull(itr, c);
    }
    return NULL;
}

static JSON parseString(input_stream_iterator *itr, char c)
{
    assert(c == '"' && "Missing open quote at start of JSONString");
    string_builder sb; string_builder_init(&sb);
    int bs = 0;
    for(c = NEXT(itr); EOS(itr); c = NEXT(itr)) {
        switch (c) {
            case '\\':
                bs += 1;
                break;
            case '"':
                if (bs % 2 == 0) {
                    goto L_end;
                }
                break;
            default:
                bs = 0;
                break;
        }
        string_builder_add(&sb, c);
    }
    L_end:;
    return (JSON)JSONString_new2(&sb);
}

static JSON parseObject(input_stream_iterator *itr, char c)
{
    assert(c == '{' && "Missing open brace '{' at start of json object");
    JSON json = JSONObject_new();
    for (c = skip_space(itr, 0); EOS(itr); c = skip_space(itr, 0)) {
        JSONString *key = NULL;
        JSON val = NULL;
        assert(c == '"' && "Missing open quote for element key");
        key = (JSONString *) parseString(itr, c);
        c = skip_space(itr, 0);
        assert(c == ':' && "Missing ':' after key in object");
        val = parseChild(itr, 0);
        _JSONObject_set(toJSONObject(json), key, val);
        c = skip_space(itr, 0);
        if (c == '}') {
            break;
        }
        assert(c == ',' && "Missing comma or end of JSON Object '}'");
    }
    return json;
}

static JSON parseArray(input_stream_iterator *itr, char c)
{
    JSON json = JSONArray_new();
    JSONArray *a = toJSONArray(json);
    assert(c == '[' && "Missing open brace '[' at start of json array");
    c = skip_space(itr, 0);
    if (c == ']') {
        /* array with no elements "[]" */
        return json;
    }
    for (; EOS(itr); c = skip_space(itr, 0)) {
        JSON val = parseChild(itr, c);
        JSONArray_append(a, val);
        c = skip_space(itr, 0);
        if (c == ']') {
            break;
        }
        assert(c == ',' && "Missing comma or end of JSON Array ']'");
    }
    return json;
}

static JSON parseBoolean(input_stream_iterator *itr, char c)
{
    int val = 0;
    if (c == 't') {
        if(NEXT(itr) == 'r' && NEXT(itr) == 'u' && NEXT(itr) == 'e') {
            val = 1;
        }
    }
    else if (c == 'f') {
        if (NEXT(itr) == 'a' && NEXT(itr) == 'l' &&
                NEXT(itr) == 's' && NEXT(itr) == 'e') {
        }
    }
    else {
        assert(0 && "Cannot parse JSON bool variable");
    }
    return JSONBool_new(val);
}

static JSON parseNumber(input_stream_iterator *itr, char c)
{
    assert((c == '-' || ('0' <= c && c <= '9')) && "It do not seem as Number");
    string_builder sb; string_builder_init(&sb);
    kjson_type type = JSON_Int;
    if (c == '-') { string_builder_add(&sb, c);c = NEXT(itr); }
    if (c == '0') {
        string_builder_add(&sb, c);
        c = NEXT(itr);
    }
    else if ('1' <= c && c <= '9') {
        for (; '0' <= c && c <= '9' && EOS(itr); c = NEXT(itr)) {
            string_builder_add(&sb, c);
        }
    }
    if (c == '.') {
        type = JSON_Double;
        string_builder_add(&sb, c);
        for (c = NEXT(itr); '0' <= c && c <= '9' &&
                EOS(itr); c = NEXT(itr)) {
            string_builder_add(&sb, c);
        }
    }
    if (c == 'e' || c == 'E') {
        string_builder_add(&sb, c);
        c = NEXT(itr);
        if (c == '+' || c == '-') {
            string_builder_add(&sb, c);
            c = NEXT(itr);
        }
        type = JSON_Double;
        for (; '0' <= c && c <= '9' && EOS(itr); c = NEXT(itr)) {
            string_builder_add(&sb, c);
        }
    }
    input_stream_unput(itr->ins, c);
    JSON n;
    size_t len;
    char *s = string_builder_tostring(&sb, &len, 0);
    char *e = s + len;
    if (type == JSON_Double) {
        double d = strtod(s, &e);
        n = JSONDouble_new(d);
    } else {
        long val = strtol(s, &e, 10);
        n = JSONInt_new(val);
    }
    free(s);
    return n;
}

static JSON parseNull(input_stream_iterator *itr, char c)
{
    if (c == 'n') {
        if(NEXT(itr) == 'u' && NEXT(itr) == 'l' && NEXT(itr) == 'l') {
            return JSONNull_new();
        }
    }
    assert(0 && "Cannot parse JSON null variable");
    return NULL;
}

static JSON parse(input_stream *ins)
{
    input_stream_iterator itr;
    char c = 0;
    for_each_istream(ins, itr, c) {
        JSON json;
        if ((c = skip_space(&itr, c)) == 0) {
            break;
        }
        if ((json = parseChild(&itr, c)) != NULL)
            return json;
    }
    return NULL;
}

#undef EOS
#undef NEXT

/* [Dump functions] */
static void JSONString_dump(FILE *fp, JSONString *json)
{
#ifdef USE_NUMBOX
    json = toJSONString(toStr(toVal((JSON)json)));
#endif
    fprintf(stderr, "\"%s\"", json->str);
}

static void JSONNull_dump(FILE *fp, JSONNull *json)
{
    fputs("null", stderr);
}

static void JSONBool_dump(FILE *fp, JSONNumber *json)
{
#ifdef USE_NUMBOX
    fprintf(stderr, "%s", toBool(toVal(json))?"ture":"false");
#else
    fprintf(stderr, "%s", json->val?"ture":"false");
#endif
}

static void JSONArray_dump(FILE *fp, JSONArray *a)
{
    JSON *s, *e;
    fputs("[", stderr);
    JSON_ARRAY_EACH(a, s, e) {
        JSON_dump(fp, *s);
        fputs(",", stderr);
    }
    fputs("]", stderr);
}

static void JSONInt_dump(FILE *fp, JSONNumber *json)
{
#ifdef USE_NUMBOX
    fprintf(stderr, "%d", toInt32(toVal(json)));
#else
    fprintf(stderr, "%d", (int)json->val);
#endif

}

static void JSONDouble_dump(FILE *fp, JSONNumber *json)
{
#ifdef USE_NUMBOX
    fprintf(stderr, "%g", toDouble(toVal(json)));
#else
    union v { double f; long v; } v;
    v.v = json->val;
    fprintf(stderr, "%g", v.f);
#endif
}

static void JSONObject_dump(FILE *fp, JSONObject *o)
{
    pmap_record_t *r;
    poolmap_iterator itr = {0};
    fputs("{", fp);
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    while ((r = poolmap_next(o->child, &itr)) != NULL) {
        fputs("", fp);
        JSONString_dump(fp, (JSONString*)r->k);
        fputs(" : ", fp);
        JSON_dump(fp, (JSON)r->v);
        fputs(",", fp);
    }
    fputs("}", fp);
}

void JSON_dump(FILE *fp, JSON json)
{
#ifdef USE_NUMBOX
    if (IsDouble((toVal(json)))) {
        JSONDouble_dump(fp, (JSONDouble*)json);
        return;
    }
#endif
    switch (JSON_type(json)) {
#define CASE(T, O) case JSON_##T: JSON##T##_dump(fp, (JSON##T *) O); break
        CASE(Object, json);
        CASE(Array, json);
        CASE(String, json);
        CASE(Int, json);
        CASE(Double, json);
        CASE(Bool, json);
        CASE(Null, json);
        default:
            assert(0 && "NO dump func");
#undef CASE
    }
}

static JSON parseJSON_stream(input_stream *ins)
{
    return parse(ins);
}

JSON parseJSON(char *s, char *e)
{
    input_stream *ins = new_string_input_stream(s, e - s);
    return parseJSON_stream(ins);
}

JSON parseJSON_fromFILE(char *file)
{
    input_stream *ins = new_file_input_stream(file, 1024);
    return parseJSON_stream(ins);
}

static JSON _JSON_get(JSON json, char *key)
{
    JSONObject *o = toJSONObject(json);
    JSONString s = {};
    s.length = strlen(key);
    s.str    = key;
    assert(JSON_type(json) == JSON_Object);
#ifdef USE_NUMBOX
    Value v = ValueS((JSON)&s);
    o = toJSONObject(toObj(toVal((JSON)o)));
#else
    JSON_set_type((JSON) &s, JSON_String);
#endif

    pmap_record_t *r = poolmap_get(o->child, (char *)
#ifdef USE_NUMBOX
            v.pval
#else
            &s
#endif
            , 0);
    return (JSON) r->v;
}

JSON JSON_get(JSON json, char *key)
{
    return _JSON_get(json, key);
}

int JSON_getInt(JSON json, char *key)
{
    JSON v = _JSON_get(json, key);
#ifdef USE_NUMBOX
    return toInt32(toVal(v));
#else
    return ((JSONInt*)v)->val;
#endif
}

int JSON_getBool(JSON json, char *key)
{
    JSON v = _JSON_get(json, key);
#ifdef USE_NUMBOX
    return toBool(toVal(v));
#else
    return ((JSONBool*)v)->val;
#endif

}

double JSON_getDouble(JSON json, char *key)
{
    JSON v = _JSON_get(json, key);
#ifdef USE_NUMBOX
    return toDouble(toVal(v));
#else
    union v { double f; long v; } val;
    val.v = toJSONDouble(v)->val;
    return val.f;
#endif
}

char *JSON_getString(JSON json, char *key, size_t *len)
{
    JSONString *s = (JSONString *) _JSON_get(json, key);
#ifdef USE_NUMBOX
    s = (JSONString *) toStr(toVal((void*)s));
#endif
    *len = s->length;
    return s->str;
}

JSON *JSON_getArray(JSON json, char *key, size_t *len)
{
    JSONArray *a = (JSONArray *) _JSON_get(json, key);
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    *len = a->length;
    return a->list;
}

unsigned JSON_length(JSON json)
{
    JSONArray *a = toJSONArray(json);
    assert((JSON_type(json) & 0x3) == 0x1);
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    return a->length;
}

int JSONObject_iterator_init(JSONObject_iterator *itr, JSONObject *obj)
{
    JSON json = (JSON)obj;
    if (!JSON_type(json) ==  JSON_Object)
        return 0;
#ifdef USE_NUMBOX
    itr->obj = toJSONObject(toObj(toVal(obj)));
#else
    itr->obj = obj;
#endif
    itr->index = 0;
    return 1;
}

JSONString *JSONObject_iterator_next(JSONObject_iterator *itr, JSON *val)
{
    JSONObject *o = itr->obj;
    pmap_record_t *r;
    while ((r = poolmap_next(o->child, (poolmap_iterator*) itr)) != NULL) {
        *val = (JSON)r->v;
        return (JSONString*)r->k;
    }
    *val = NULL;
    return NULL;
}

#ifdef __cplusplus
}
#endif
