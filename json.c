#include "addr.h"
#include "json.h"

#define STRDUP(TO, FROM) if (FROM != NULL) { DOA("allocate memory for " #TO, malloc, TO, NULL, strlen(FROM) + 1); snprintf(TO, strlen(FROM) + 1, "%s", FROM); }

jsonStruct_p copyJsonStruct(jsonStruct_p jS) {
    jsonStruct_p _rc = NULL, rc = NULL;
    size_t i = 0;

    if (jS) {
        DOA("allocate memory for copied jsonStruct", malloc, _rc, NULL, sizeof(struct jsonStruct));
        memset(_rc, 0, sizeof(struct jsonStruct));
        if (jS->element_len > 0) {
            _rc->element_len = jS->element_len;

            DOA("allocate memory for copied jsonStruct elements", malloc, _rc->elements, NULL, sizeof(struct jsonElm) * jS->element_len);

            for (i = 0; i < jS->element_len; i++) {
                STRDUP(_rc->elements[i].path, jS->elements[i].path);
                STRDUP(_rc->elements[i].key, jS->elements[i].key);
                _rc->elements[i].type = jS->elements[i].type;
                switch(jS->elements[i].type) {
                    case json_type_null:
                        break;
                    case json_type_boolean:
                        _rc->elements[i].value.booleanVal = jS->elements[i].value.booleanVal;
                        break;
                    case json_type_double:
                        _rc->elements[i].value.doubleVal = jS->elements[i].value.doubleVal;
                        break;
                    case json_type_int:
                        if ((jS->elements[i].key != NULL) && (!strncmp(jS->elements[i].key, "id", 2))) {
                            _rc->elements[i].value.idVal = jS->elements[i].value.idVal;
                        } else {
                            _rc->elements[i].value.intVal = jS->elements[i].value.intVal;
                        }
                        break;
                    case json_type_object:
                        _rc->elements[i].value.booleanVal = jS->elements[i].value.booleanVal;
                        break;
                    case json_type_array:
                        _rc->elements[i].value.intVal = jS->elements[i].value.intVal;
                        break;
                    case json_type_string:
                        STRDUP(_rc->elements[i].value.stringVal, jS->elements[i].value.stringVal);
                        break;
                    default:
                        syslog(P_ERR, "Json type unknown: %d", jS->elements[i].type);
                        goto over;
                }
                _rc->elements[i].type = jS->elements[i].type;
            }
        }
    }

    rc = _rc;
over:
    FFF(_rc, freeJsonStruct, _rc && (rc == NULL));
    return rc;
}

void freeJsonStruct(jsonStruct_p jS) {
    size_t i = 0;

    if (jS) {
        if (jS->elements) {
            for (i = 0; i < jS->element_len; i++) {
                F(jS->elements[i].path);
                F(jS->elements[i].key);
                if (jS->elements[i].type == json_type_string) F(jS->elements[i].value.stringVal);
            }
            F(jS->elements);
        }
        F(jS);
    }
}

static int parse(jsonStruct_p *jS, json_object *obj, int indent, const char *ctx, const char **paths, size_t pathcount);

static int createPathFormat(const char *path, char **pathFormat) {
    int rc = -1;
    char *pF = NULL;
    char hasWord = 0;
    char currdigits[256];
    size_t pathlen = 0, i = 0, j = 0, k = 0, currdigitlen = 0;

    for (i = 0; i < strlen(path); i++) {
        if ((path[i] <= '9') && (path[i] >= '0')) {
            if (hasWord) { pathlen++; } else { currdigitlen++; }
        } else if (path[i] == '|') {
            if (hasWord == 1) { pathlen += currdigitlen; } else { if ((i != 0) && (path[i - 1] != '|')) pathlen += 2; }
            hasWord = 0; currdigitlen = 0; pathlen++;
        } else {
            pathlen += currdigitlen + 1; currdigitlen = 0; hasWord = 1;
        }
    }

    if (hasWord == 1) { pathlen += currdigitlen; } else { pathlen += 2; }
    hasWord = 0; currdigitlen = 0;

    DOA("allocate memory for new path format", malloc, pF, NULL, pathlen + 1);

    for (i = 0; i < strlen(path); i++) {
        if ((path[i] <= '9') && (path[i] >= '0')) {
            if (hasWord) { pF[j++] = path[i]; } else { currdigits[currdigitlen++] = path[i]; }
        } else if (path[i] == '|') {
            if (hasWord == 1) { for (k = 0; k < currdigitlen; k++) { pF[j++] = currdigits[k]; } } else { if ((i != 0) && (path[i - 1] != '|')) { pF[j++] = '%'; pF[j++] = 'd'; } }
            pF[j++] = '|'; hasWord = 0; currdigitlen = 0;
        } else {
            for (k = 0; k < currdigitlen; k++) { pF[j++] = currdigits[k]; }
            hasWord = 1; pF[j++] = path[i]; currdigitlen = 0;
        }
    }

    if (hasWord == 1) { for (k = 0; k < currdigitlen; k++) { pF[j++] = currdigits[k]; } } else { pF[j++] = '%'; pF[j++] = 'd'; }
    pF[j] = '\0';

    rc = 0;
    *pathFormat = pF;
over:
    if (rc != 0) {
        F(pF);
    }
    return rc;
}

static int respond_to_type(jsonStruct_p *jS, json_object *val, const char *key, int indent, const char *ctx, const char **paths, size_t pathcount) {
    int rc = -1;
    jsonStruct_p _jS = *jS;
    jsonElm_p jE = NULL;
    json_type type = json_object_get_type(val);
    int i = 0, l = 0;
    char *new_ctx = NULL;
    char *new_ctx_fmt = NULL;

    if ((key != NULL) && (ctx != NULL)) {
        DOA("allocate memory for new_ctx", malloc, new_ctx, NULL, strlen(ctx) + strlen(key) + 2);
        snprintf(new_ctx, strlen(ctx) + strlen(key) + 2, "%s|%s", (char *) ctx, (char *) key);
    } else if (ctx != NULL) {
        DOA("allocate memory for new_ctx", malloc, new_ctx, NULL, strlen(ctx) + 1);
        snprintf(new_ctx, strlen(ctx) + 1, "%s", (char *) ctx);
    }

    syslog(P_DBG, ">>> Responding to context %s", new_ctx);

    DONT("convert ctx to format", createPathFormat, 0, new_ctx, &new_ctx_fmt);

    syslog(P_DBG, ">>> context format: %s", new_ctx_fmt);

    if (paths != NULL) {
        char found = 0;

        for (i = 0; i < pathcount; i++) {
            if (strstr(paths[i], "%d")) {
                syslog(P_DBG, ">>> Comparing whitelist path %s with path format %s", paths[i], new_ctx_fmt);

                if ((strlen(new_ctx_fmt) == strlen(paths[i])) && !strcmp(new_ctx_fmt, paths[i])) {
                    found = 1;
                    break;
                }
            } else {
                syslog(P_DBG, ">>> Comparing whitelist path %s with path %s", paths[i], new_ctx);

                if ((strlen(new_ctx) == strlen(paths[i])) && !strcmp(new_ctx, paths[i])) {
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            rc = 0;
            goto over;
        }
    }

    syslog(P_DBG, ">>> Allocating a new element");

    if (_jS == NULL) {
        DOA("allocate new memory for main jsonStruct", malloc, _jS, NULL, sizeof(struct jsonStruct));
        memset(_jS, 0, sizeof(struct jsonStruct));
        DOA("allocate new memory for jsonElm array", malloc, _jS->elements, NULL, sizeof(struct jsonElm));
    } else {
        DOA("reallocate memory for jsonElm array", realloc, _jS->elements, NULL, _jS->elements, sizeof(struct jsonElm) * (_jS->element_len + 1));
    }

    jE = &(_jS->elements[_jS->element_len]);
    memset(jE, 0, sizeof(struct jsonElm));

    STRDUP(jE->path, new_ctx);
    STRDUP(jE->key, key);

    jE->type = type;

    switch(type) {
        case json_type_null:
            syslog(P_DBG, ">>> Element is type null");
            break;
        case json_type_boolean:
            syslog(P_DBG, ">>> Element is type boolean");
            jE->value.booleanVal = (char) json_object_get_boolean(val);
            break;
        case json_type_double:
            syslog(P_DBG, ">>> Element is type double");
            jE->value.doubleVal = (double) json_object_get_double(val);
            break;
        case json_type_int:
            if ((key != NULL) && (!strncmp(key, "id", 2))) {
                syslog(P_DBG, ">>> Element is type int (id)");
                jE->value.idVal = (unsigned long long) json_object_get_int64(val);
            } else {
                syslog(P_DBG, ">>> Element is type int");
                jE->value.intVal = (long long) json_object_get_int64(val);
            }
            break;
        case json_type_object:
            syslog(P_DBG, ">>> Element is type object");
            jE->value.booleanVal = (char) 1;
            _jS->element_len++;
            *jS = _jS;
            parse(jS, val, indent + 1, (const char *) new_ctx, paths, pathcount);
            break;
        case json_type_array:
            syslog(P_DBG, ">>> Element is type array");
            l = json_object_array_length(val);
            jE->value.intVal = (long long) l;
            _jS->element_len++;
            *jS = _jS;
            for (i = 0; i < l; i++) {
                char index[256];
                snprintf(index, 256, "%d", (int) i);
                DONT("parse array element", respond_to_type, 0, jS, json_object_array_get_idx(val, i), index, indent + 1, (const char *) new_ctx, paths, pathcount);
            }
            break;
        case json_type_string:
            syslog(P_DBG, ">>> Element is type string");
            STRDUP(jE->value.stringVal, json_object_get_string(val));
            break;
        default:
            syslog(P_ERR, "Json type unknown: %d", jE->type);
            goto over;
    }

    if((type != json_type_object) && (type != json_type_array)) { _jS->element_len++; *jS = _jS; }
    rc = 0;
over:
    F(new_ctx_fmt);
    F(new_ctx);
    if (rc != 0) {
        if (jE != NULL) {
            F(jE->path);
            F(jE->key);
            if (jE->type == json_type_string) F(jE->value.stringVal);
        }

        if (_jS->element_len == 0) {
            F(_jS->elements);
            F(_jS);
        } else {
            DOAS("reallocate memory for json elements - return flawed one", realloc, _jS->elements, NULL, _jS->elements, sizeof(struct jsonElm) * _jS->element_len);
        }
    }
    return rc;
}

static int parse(jsonStruct_p *jS, json_object *obj, int indent, const char *ctx, const char **paths, size_t pathcount) {
    int rc = -1;
    json_type type = json_object_get_type(obj);

    switch(type) {
        case json_type_object:
            {
                json_object_object_foreach(obj, key, val) {
                    DONT("parse json element (object)", respond_to_type, 0, jS, val, key, indent, ctx, paths, pathcount);
                }
            }
            break;
        default:
            DONT("parse json element (non-object)", respond_to_type, 0, jS, obj, NULL, indent, ctx, paths, pathcount);
    }

    rc = 0;
over:
    return rc;
}

jsonStruct_p parseJson(const char *jsonText, const char **paths, size_t pathcount) {
    jsonStruct_p rc = NULL, _rc = NULL;
    struct json_object *json = NULL;
    enum json_tokener_error error = json_tokener_success;

    DOA("parse json string", json_tokener_parse_verbose, json, NULL, jsonText, &error);
    DONT("parse json object", parse, 0, &_rc, json, 0, "root", paths, pathcount);

    rc = _rc;
over:
    FFF(_rc, freeJsonStruct, _rc && (rc == NULL));
    FF(json, json_object_put);
    return rc;
}

int isPath(const jsonStruct_p jS, const char *path) {
    int rc = -1;
    size_t i = 0;
    char found = 0;

    for (i = 0; i < jS->element_len; i++) {
        syslog(P_DBG, "Testing element '%s'", jS->elements[i].path);

        if (!strncmp(jS->elements[i].path, path, strlen(path))) {
            syslog(P_DBG, "Found");
            found = 1;
            break;
        }
    }

    if (!found) {
        rc = 0;
        goto over;
    }

    rc = 1;
over:
    return rc;
}

int getElementType(const jsonStruct_p jS, const char *path) {
    int rc = -1;
    json_type type = json_type_null;
    size_t i = 0;
    char found = 0;

    for (i = 0; i < jS->element_len; i++) {
        syslog(P_DBG, "Compare element path %s to search path %s", jS->elements[i].path, path);
        if (!strncmp(jS->elements[i].path, path, strlen(path))) {
            syslog(P_DBG, "It matches");
            type = jS->elements[i].type;
            found = 1;
            break;
        }
    }

    if (!found) {
        syslog(P_ERR, "No such element found");
        goto over;
    }

    rc = (int) type;
over:
    return rc;
}

jsonElm_p getElement(const jsonStruct_p jS, const char *path) {
    jsonElm_p rc = NULL;
    size_t i = 0;
    char found = 0;

    for (i = 0; i < jS->element_len; i++) {
        syslog(P_DBG, "Compare element path %s to search path %s", jS->elements[i].path, path);
        if (!strncmp(jS->elements[i].path, path, strlen(path))) {
            syslog(P_DBG, "It matches");
            found = 1;
            rc = &jS->elements[i];
            break;
        }
    }

    if (!found) {
        syslog(P_ERR, "No such element found");
    }

    return rc;
}

uvalue getElementVal(const jsonStruct_p jS, const char *path) {
    uvalue rc = {.booleanVal = -1}, _rc = {.booleanVal = -1};
    size_t i = 0;
    char found = 0;

    for (i = 0; i < jS->element_len; i++) {
        syslog(P_DBG, "Compare element path %s to search path %s", jS->elements[i].path, path);
        if (!strncmp(jS->elements[i].path, path, strlen(path))) {
            syslog(P_DBG, "It matches");
            _rc = jS->elements[i].value;
            found = 1;
            break;
        }
    }

    if (!found) {
        syslog(P_ERR, "No such element found");
        goto over;
    }

    rc = _rc;
over:
    return rc;
}
