#include "kjson.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void test_obj()
{
    JSON o = JSONObject_new();
    JSONObject_set((JSONObject *) o, JSONString_new("a", 1), JSONBool_new(1));
    JSONObject_set((JSONObject *) o, JSONString_new("b", 1), JSONInt_new(100));
    JSONObject_set((JSONObject *) o, JSONString_new("c", 1), JSONDouble_new(3.14));
    size_t len;
    char *s = JSON_toString(o, &len);
    assert(strncmp(s, "{\"c\":3.14,\"a\":true,\"b\":100}", len) == 0);
    JSON_free(o);
    fprintf(stderr, "'%s'\n", s);
    free((char*)s);
}
static void test_array()
{
    JSON a = JSONArray_new();
    JSONArray_append((JSONArray*) a, (JSON) JSONString_new("a", 1));
    JSONArray_append((JSONArray*) a, (JSON) JSONString_new("b", 1));
    JSONArray_append((JSONArray*) a, (JSON) JSONString_new("c", 1));
    size_t len;
    char *s = JSON_toString(a, &len);
    assert(strncmp(s, "[\"a\",\"b\",\"c\"]", len) == 0);
    JSON_free(a);
    fprintf(stderr, "'%s'\n", s);
    free((char*)s);
}
static void test_int()
{
    size_t len;
    char *s;
    {
        JSON n = JSONInt_new(100);
        s = JSON_toString(n, &len);
        assert(strncmp(s, "100", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = JSONInt_new(INT32_MIN);
        s = JSON_toString(n, &len);
        assert(strncmp(s, "-2147483648", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = JSONInt_new(INT32_MAX);
        s = JSON_toString(n, &len);
        assert(strncmp(s, "2147483647", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
}
static void test_double()
{
    size_t len;
    char *s;
    {
        JSON n = JSONDouble_new(M_PI);
        s = JSON_toString(n, &len);
        char buf[128] = {};
        snprintf(buf, 128, "%g", M_PI);
        assert(strncmp(buf, buf, len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = JSONDouble_new(10.01);
        s = JSON_toString(n, &len);
        assert(strncmp(s, "10.01", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
}

int main(int argc, char const* argv[])
{
    test_int();
    test_double();
    test_array();
    test_obj();
    return 0;
}
