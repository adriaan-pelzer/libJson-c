#include <stdio.h>
#include <stdlib.h>
#include <json.h>
#include <syslog.h>
#define _POSIX_C_SOURCE 200809L
#include <string.h>

#ifndef _JSON_H_
#define _JSON_H_

/* Initialise logging in your main like this:
 *
 * #define DAEMON_NAME "something_or_other"
 *
 * #ifdef DEBUG
 *     setlogmask (LOG_UPTO (LOG_DEBUG));
 *     openlog (DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
 * #else
 *     setlogmask (LOG_UPTO (LOG_INFO));
 *     openlog (DAEMON_NAME, LOG_CONS, LOG_USER);
 * #endif
 */

typedef union value {
    char booleanVal;
    double doubleVal;
    long long intVal;
    unsigned long long idVal;
    char *stringVal;
} uvalue;

typedef struct jsonElm {
    char *path;
    char *key;
    json_type type;
    uvalue value;
} *jsonElm_p;

typedef struct jsonStruct {
    jsonElm_p elements;
    size_t element_len;
} *jsonStruct_p;

jsonStruct_p parseJson(const char *jsonText, const char **paths, size_t pathcount);
void freeJsonStruct(jsonStruct_p jS);
int isPath(const jsonStruct_p jS, const char *path);
int getElementType(const jsonStruct_p jS, const char *path);
jsonElm_p getElement(const jsonStruct_p jS, const char *path);
uvalue getElementVal(const jsonStruct_p jS, const char *path);

#endif
