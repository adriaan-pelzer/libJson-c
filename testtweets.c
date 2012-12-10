#include <assert.h>
#include "json.h"
#include "tweets.h"
#include "addr.h"

#define DAEMON_NAME "libJson"

int main(void) {
    int rc = EXIT_FAILURE;
    jsonStruct_p jS = NULL;
    uvalue value = {.booleanVal = -1};
    json_type type = 0;
    char **paths = NULL;

#ifdef DEBUG
    setlogmask (LOG_UPTO (LOG_DEBUG));
    openlog (DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
#else
    setlogmask (LOG_UPTO (LOG_INFO));
    openlog (DAEMON_NAME, LOG_CONS, LOG_USER);
#endif

    DOA("get json struct", parseJson, jS, NULL, tweet2, NULL, 0);
    DOA("get tweet value", getElementType, type, -1, jS, "root|text");
    assert((json_type) type == json_type_string);
    value = getElementVal(jS, "root|text");
    assert(value.booleanVal != -1);
    assert(!strncmp(value.stringVal, "lecturing at the \"analyzing big data with twitter\" class at @cal with @othman  http://t.co/bfj7zkDJ", strlen(value.stringVal)));
    value = getElementVal(jS, "root|user|screen_name");
    assert(!strncmp(value.stringVal, "raffi", strlen(value.stringVal)));
    FF(jS, freeJsonStruct);
    jS = NULL;

    DOA("allocate memory for paths", malloc, paths, NULL, sizeof(char *));
    DOA("allocate memory for path", strdup, paths[0], NULL, "root|text");
    DOA("get json struct", parseJson, jS, NULL, tweet2, (const char **) paths, 1);
    DOA("get tweet value", getElementType, type, -1, jS, "root|text");
    assert((json_type) type == json_type_string);
    value = getElementVal(jS, "root|text");
    assert(value.booleanVal != -1);
    assert(!strncmp(value.stringVal, "lecturing at the \"analyzing big data with twitter\" class at @cal with @othman  http://t.co/bfj7zkDJ", strlen(value.stringVal)));
    value = getElementVal(jS, "root|user|screen_name");
    assert(value.booleanVal == -1);
    F(paths[0]);
    F(paths);
    FF(jS, freeJsonStruct);
    jS = NULL;

    DOA("allocate memory for paths", malloc, paths, NULL, sizeof(char *) * 3);
    DOA("allocate memory for path", strdup, paths[0], NULL, "root|text");
    DOA("allocate memory for path", strdup, paths[1], NULL, "root|user");
    DOA("allocate memory for path", strdup, paths[2], NULL, "root|user|screen_name");
    DOA("get json struct", parseJson, jS, NULL, tweet2, (const char **) paths, 3);
    DOA("get tweet value", getElementType, type, -1, jS, "root|text");
    assert((json_type) type == json_type_string);
    value = getElementVal(jS, "root|text");
    assert(value.booleanVal != -1);
    assert(!strncmp(value.stringVal, "lecturing at the \"analyzing big data with twitter\" class at @cal with @othman  http://t.co/bfj7zkDJ", strlen(value.stringVal)));
    value = getElementVal(jS, "root|user|screen_name");
    assert(!strncmp(value.stringVal, "raffi", strlen(value.stringVal)));
    F(paths[0]);
    F(paths[1]);
    F(paths[2]);
    F(paths);
    FF(jS, freeJsonStruct);
    jS = NULL;

    DOA("allocate memory for paths", malloc, paths, NULL, sizeof(char *) * 6);
    DOA("allocate memory for path", strdup, paths[0], NULL, "root|text");
    DOA("allocate memory for path", strdup, paths[1], NULL, "root|entities");
    DOA("allocate memory for path", strdup, paths[2], NULL, "root|entities|urls");
    DOA("allocate memory for path", strdup, paths[3], NULL, "root|entities|urls|%d");
    DOA("allocate memory for path", strdup, paths[4], NULL, "root|entities|urls|%d|indices");
    DOA("allocate memory for path", strdup, paths[5], NULL, "root|entities|urls|%d|indices|%d");
    DOA("get json struct", parseJson, jS, NULL, tweet2, (const char **) paths, 6);
    DOA("get tweet value", getElementType, type, -1, jS, "root|text");
    assert((json_type) type == json_type_string);
    value = getElementVal(jS, "root|text");
    assert(value.booleanVal != -1);
    assert(!strncmp(value.stringVal, "lecturing at the \"analyzing big data with twitter\" class at @cal with @othman  http://t.co/bfj7zkDJ", strlen(value.stringVal)));
    value = getElementVal(jS, "root|entities|urls");
    assert(value.booleanVal == 1);
    value = getElementVal(jS, "root|entities|urls|0");
    assert(value.booleanVal == 1);
    value = getElementVal(jS, "root|entities|urls|0|indices");
    assert(value.intVal == 2);
    value = getElementVal(jS, "root|entities|urls|0|indices|0");
    assert(value.intVal == 79);
    F(paths[0]);
    F(paths[1]);
    F(paths[2]);
    F(paths[3]);
    F(paths[4]);
    F(paths[5]);
    F(paths);
    FF(jS, freeJsonStruct);
    jS = NULL;

    rc = EXIT_SUCCESS;
over:
    FF(jS, freeJsonStruct);
    return rc;
}
