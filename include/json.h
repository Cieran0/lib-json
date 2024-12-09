#pragma once
#include "stddef.h"
#include "stdbool.h"

enum json_type {
    JSON_UNKNOWN = 0,
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_INT,
    JSON_DECIMAL,
    JSON_ARRAY,
    JSON_STRING,
    JSON_OBJECT
} ;



struct json_object {
    struct json_pair* head;
    struct json_pair* tail;
};

struct json_array {
    struct json_value* values;
    size_t size;
};

struct json_value {
    enum json_type type;
    union {
        bool boolean;
        long integer;
        double decimal;
        char* string;
        struct json_object* object;
        struct json_array* array;
    } value;
};

struct json_pair {
    char* key;
    struct json_value value;
    struct json_pair* next;
};