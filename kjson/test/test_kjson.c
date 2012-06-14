#include "kjson.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>

static void test_file(const char *file)
{
    fprintf(stderr, "--- {{ test %s --- \n", file);
    JSON json = parseJSON_fromFILE((char*)file);
    JSON_dump(stderr, json);
    JSON_free(json);
    fprintf(stderr, "\n--- }} test %s --- \n", file);
}

static const char *names[] = {
    "foo", "bar", "mono"
};
static const int lines[] = {
    10, 20, -40
};
static const double versions[] = {
    0.2, 0.3, 0.4
};

static char data[] =
"{\"app\": [\n"
"    {\n"
"        \"name\": \"foo\",\n"
"        \"line\": 10,\n"
"        \"version\": 0.2,\n"
"        \"flag\": true\n"
"    },\n"
"    {\n"
"        \"name\": \"bar\",\n"
"        \"line\": 20,\n"
"        \"version\": 0.3,\n"
"        \"flag\": true\n"
"    },\n"
"    {\n"
"        \"name\": \"mono\",\n"
"        \"line\": -40,\n"
"        \"version\": 0.4,\n"
"        \"flag\": null\n"
"    }]\n"
"}";

static void test_string(void)
{
    JSON json = parseJSON(data, data+sizeof(data));
    JSONArray *child = toJSONArray(JSON_get(json, "app"));
    assert(child != NULL);
    assert(JSON_type((JSON)child) == JSON_Array);
    assert(JSON_length((JSON)child) == 3);
    JSON *I, *E;
    int i = 0;


    JSON_ARRAY_EACH(child, I, E) {
        assert(JSON_get(*I, "name") != NULL);
        assert(JSON_get(*I, "line") != NULL);
        assert(JSON_get(*I, "version") != NULL);
        assert(JSON_get(*I, "flag") != NULL);
        size_t len;
        char *name = JSON_getString(*I, "name", &len);
        int   line = JSON_getInt(*I, "line");
        double ver  = JSON_getDouble(*I, "version");
        assert(len == strlen(names[i]) && strncmp(name, names[i], len) == 0);
        assert(line == lines[i]);
        assert(ver  == versions[i]);
        fprintf(stderr, "%s %d %f\n", name, line, ver);
        i++;
    }
    JSON_free(json);
}

static char data2[] = 
"{\n"
"    \"name\": \"foo\",\n"
"    \"line\": 10,\n"
"    \"version\": 0.2,\n"
"    \"flag\": true\n"
"}\n";

static void test_object_iterator(void)
{
    JSONObject *o = (JSONObject*) parseJSON(data2, data2+sizeof(data2));
    assert(JSON_type((JSON)o) == JSON_Object);
    //assert(JSON_length((JSON)child) == 3);
    /* XXX this test depends on imprementation of map data. */
    static const json_type types[] = {
        JSON_Bool,
        JSON_Int,
        JSON_String,
        JSON_Double
    };

    int i = 0;
    JSONString *Key;
    JSON Val;
    JSONObject_iterator Itr;

    JSON_OBJECT_EACH(o, Itr, Key, Val) {
        assert(JSON_type((JSON)Key) == JSON_String);
        assert(JSON_type(Val) == types[i]);
        ++i;
        fprintf(stderr, "<");
        JSON_dump(stderr, (JSON)Key);
        fprintf(stderr, ":");
        JSON_dump(stderr, Val);
        fprintf(stderr, ">\n");
    }
    JSON_free((JSON)o);
}

int main(int argc, char const* argv[])
{
    const char **files;
    const char *files_default[] = {
        "./test/test01.json",
        "./test/test02.json",
        "./test/test03.json",
        "./test/test04.json",
        "./test/test05.json",
        "./test/test06.json",
        "./test/test07.json",
        "./test/test08.json",
        "./test/test09.json",
        "./test/test10.json",
        "./test/benchmark1.json",
        "./test/benchmark2.json",
        "./test/benchmark3.json",
        "./test/benchmark4.json",
        "./test/benchmark5.json",
    };
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif
    size_t size = ARRAY_SIZE(files_default);
    if (argc > 1) {
        files = argv+1;
        size  = argc-1;
    } else {
        files = files_default;
    }
    int i;
    for (i = 0; i < size; i++) {
        test_file(files[i]);
    }
    test_string();
    test_object_iterator();
    return 0;
}
