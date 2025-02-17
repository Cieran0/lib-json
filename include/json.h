#pragma once
#include "stddef.h"
#include "stdbool.h"

enum json_type {
    JSON_ERROR = 0,
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
    char* error;
};

struct json_array {
    struct json_value* values;
    size_t size;
    char* error;
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
    } as;
};

struct json_pair {
    char* key;
    struct json_value value;
    struct json_pair* next;
};

typedef void* (*allocator)(size_t);

struct json_value json_parse_string(const char* json_string, allocator alloc);
char* json_value_to_string(struct json_value value);