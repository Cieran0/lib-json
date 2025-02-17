#include "stdio.h"
#include "json.h"
#include "tokenise.h"
#include "assert.h"
#include "string.h"
#include "math.h"

#include "parse.h"

char json_error_buffer[512];

struct json_object* extract_object(token* tokens, size_t* index, allocator alloc);
struct json_array* extract_array(token* tokens, size_t* index, allocator alloc);
char* json_value_to_string(struct json_value value);


struct json_value get_json_value(token* tokens, size_t* index, allocator alloc) {
    struct json_value json_error = {
        .type = JSON_ERROR,
        .value.string = json_error_buffer
    };

    struct json_value value = {0};

    if (tokens[*index].type == END_OF_TOKENS) {
        sprintf(json_error_buffer, "End of tokens? %zu: %s", *index, tokens[*index].content);
        return json_error;
    }

    if (tokens[*index].type == END_OF_TOKENS || tokens[*index].type == ERROR_TOKEN) {
        sprintf(json_error_buffer, "Unexpected token at index %zu: %s", *index, tokens[*index].content);
        return json_error;
    }

    switch (tokens[*index].type) {
    case STRING_TOKEN: {
        size_t size = strlen(tokens[*index].content);
        value.value.string = alloc(size + 1); 
        strncpy(value.value.string, tokens[*index].content, size+1);
        value.type = JSON_STRING;
        (*index)++;
        break;
    }
    case KEY_SYMBOL_TOKEN: {
        if (tokens[*index].content[0] == '{') {
            struct json_object* object = extract_object(tokens, index, alloc);

            if(object == NULL) {
                sprintf(json_error_buffer, "Memory allocation failure");
                return json_error;
            }

            if (object->error != NULL) {
                strncpy(json_error.value.string, object->error, 255);

                //free_json_object(object);

                return json_error;
            }

            value.type = JSON_OBJECT;
            value.value.object = object;
        } else if (tokens[*index].content[0] == '[') {
            struct json_array* array = extract_array(tokens, index, alloc);

            if(array == NULL) {
                sprintf(json_error_buffer, "Memory allocation failure");
                return json_error;
            }

            if (array->error != NULL) {

                //free_json_arry(array);

                return json_error;
            }

            value.type = JSON_ARRAY;
            value.value.array = array;

        } else {
            sprintf(json_error_buffer,"Unexpected symbol token at index %zu: %s", *index, tokens[*index].content);
            return json_error;
        }
        break;
    }
    case KEY_WORD_TOKEN: {
        const char* content = tokens[*index].content;
        (*index)++;
        if (strcmp(content, "null") == 0) {
            value.type = JSON_NULL;
            value.value.object = NULL;
        } else if (strcmp(content, "true") == 0 || strcmp(content, "false") == 0) {
            value.type = JSON_BOOLEAN;
            value.value.boolean = strcmp(content, "true") == 0;
        } else {
            sprintf(json_error_buffer, "Unexpected keyword token at index %zu: %s", *index, content);
            return json_error;
        }
        break;
    }
    case OTHER_TOKEN: {
        const char* content = tokens[*index].content;
        (*index)++;
        long long_value;
        double double_value;

        if (parse_long(content, &long_value)) {
            value.type = JSON_INT;
            value.value.integer = long_value;
        } else if (parse_double(content, &double_value)) {
            value.type = JSON_DECIMAL;
            value.value.decimal = double_value;
        } else {
            // Invalid numeric value
            sprintf(json_error_buffer, "Invalid numeric value at index %zu: %s", *index - 1, content);
            return json_error;
        }
        break;
    }
    default:
        // Unknown or unsupported token type
        sprintf(json_error_buffer,"Unhandled token type at index %zu: %d", *index, tokens[*index].type);
        return json_error;
        break;
    }

    return value;
}


struct json_pair* get_json_pair(token* tokens, size_t* index, allocator alloc) {

    if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ',' ) {
        (*index) += 1;
    }
    if(tokens[*index].type != STRING_TOKEN) {
        return NULL; //Error in format?
    }

    char* key = NULL;
    struct json_value value = {};

    struct json_pair* pair = alloc(sizeof(struct json_pair));

    size_t size = strlen(tokens[*index].content);
    key = alloc(size + 1);
    strncpy(key, tokens[*index].content, size+1);

    *index += 1;

    if(tokens[*index].type != KEY_SYMBOL_TOKEN || tokens[*index].content[0] != ':') {
        //free(pair);
        return NULL; //TODO: Fix this to give a proper error message!
    }

    *index += 1;


    value = get_json_value(tokens, index, alloc);

    pair->key=key;
    pair->value = value;
    pair->next = NULL;
    
    return pair;
}


void insert_pair(struct json_object* object, struct json_pair* pair) {
    if(pair == NULL){
        return;
    }
    if(object->head == NULL){
        object->head = pair;
        object->tail = pair;
        return;
    }


    object->tail->next = pair;
    object->tail = object->tail->next;
}

struct json_object* extract_object(token* tokens, size_t* index, allocator alloc) {
    struct json_object* object = alloc(sizeof(struct json_object));
    if (!object) {
        return NULL;
    }

    object->head = NULL;
    object->tail = NULL;
    object->error = NULL;

    // Ensure the current token is a '{'
    if (tokens[*index].type != KEY_SYMBOL_TOKEN || tokens[*index].content[0] != '{') {
        sprintf(json_error_buffer,"Expected '{' at index %zu, but got '%s'", *index, tokens[*index].content);
        object->error = json_error_buffer;
        return object;
    }

    (*index)++; // Skip the '{' token

    while (tokens[*index].type != END_OF_TOKENS) {

        if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == '}') {
            (*index)++; // Skip the '}' token
            return object;
        }

        struct json_pair* next_pair = get_json_pair(tokens, index, alloc);

        if (!next_pair) {
            sprintf(json_error_buffer,"Error parsing key-value pair at index %zu", *index);
            object->error = json_error_buffer;
            return object;
        }

        if (next_pair->value.type == JSON_ERROR) {
            object->error = json_error_buffer;
            return object;
        }

        insert_pair(object, next_pair);
    }

    sprintf(json_error_buffer,"Unexpected end of tokens while parsing object, missing '}'");
    object->error = json_error_buffer;
    return object;
}


size_t get_array_size(token* tokens, size_t* index) {

    if(tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ']'){
        return 0;
    }

    
    size_t open_array = 1;
    size_t open_object = 0;
    size_t size = 1;
    bool done = false;

    for (size_t i = *index; tokens[i].type != END_OF_TOKENS && !done; i++)
    {
        if(tokens[i].type != KEY_SYMBOL_TOKEN) {
            continue;
        }

        switch (tokens[i].content[0])
        {
        case '[': {
            open_array++;
            break;
        }
        case '{': {
            open_object++;
            break;
        }
        case ']': {
            open_array--;
            if(open_array == 0) {
                done = true;
            }
            break;
        }
        case '}': {
            open_object--;
            break;
        }
        case ',': {
            if(open_array == 1 && open_object == 0) {
                size++;
            }
            break;
        }
        
        default:
            break;
        }

    }

    return size;
}

struct json_array* extract_array(token* tokens, size_t* index, allocator alloc) {
    struct json_array* array = alloc(sizeof(struct json_array));

    if (!array) {
        return NULL;
    }

    array->error = NULL;

    (*index) += 1; // Skip the '[' token
    size_t size = get_array_size(tokens, index);

    array->size = size;
    array->values = alloc(sizeof(struct json_value) * size);
    if (!array->values) {
        sprintf(json_error_buffer, "Memory allocation failed for array values");
        array->error = json_error_buffer;
        return array;
    }

    for (size_t i = 0; i < size; i++) {
        array->values[i] = get_json_value(tokens, index, alloc);

        if(array->values[i].type == JSON_ERROR) {
            array->error = json_error_buffer;
            return array;
        }

        if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ',') {
            (*index)++;
        } else if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ']') {
            break;
        } else if (i < size - 1) {
            sprintf(json_error_buffer, "Expected ',' or ']' at index %zu, but got '%s'", *index, tokens[*index].content);
            array->error = json_error_buffer;
            return array;
        }
    }

    if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ']') {
        (*index)++;
    } else {
        sprintf(json_error_buffer, "Expected ']' at the end of the array at index %zu, but got '%s'", *index, tokens[*index].content);
        array->error = json_error_buffer;
        return array;
    }

    return array;
}


char* object_to_string(struct json_object* object) {
    char* big_buffer = (char*)malloc(16*1024);
    char* small_buff;
    int written = sprintf(big_buffer, "{");

    struct json_pair* next = object->head;
    while (next != NULL)
    {
        small_buff = json_value_to_string(next->value);
        written += sprintf(big_buffer+written, "\"%s\": %s", next->key, small_buff);

        if(next->next != NULL){
            written += sprintf(big_buffer+written, ",");
        }

        written += sprintf(big_buffer+written, " ");

        free(small_buff);
        next = next->next;
    }

    written = sprintf(big_buffer+written, "}");
    return big_buffer;
}

char* json_value_to_string(struct json_value value) {

    char* buff = (char*)malloc(256);
    if(value.type == JSON_STRING) {
        sprintf(buff, "\"%s\"", value.value.string);
    } else if (value.type == JSON_BOOLEAN) {
        sprintf(buff, "%s", (value.value.boolean ? "true" : "false"));
    } else if (value.type == JSON_NULL) {
        sprintf(buff, "null");
    } else if (value.type == JSON_DECIMAL) {
        sprintf(buff, "%g", value.value.decimal);
    } else if (value.type == JSON_INT) {
        sprintf(buff, "%ld", value.value.integer);
    } else if (value.type == JSON_OBJECT) {
        free(buff);
        return object_to_string(value.value.object);
    } else if (value.type == JSON_ARRAY) {
        if(value.value.array->size == 0 ) {
            sprintf(buff, "[]");
            return buff;
        }

        char* array_buff = (char*)malloc(256 * value.value.array->size);
        int written = sprintf(array_buff, "[ ");
        for (size_t i = 0; i < value.value.array->size; i++)
        {
            char* s_buff = json_value_to_string(value.value.array->values[i]);
            written += sprintf(array_buff+written, "%s", s_buff);

            free(s_buff);
            if(i + 1 >= value.value.array->size) {
                continue;
            }

            written += sprintf(array_buff+written, ", ");
        }
        sprintf(array_buff+written, "]");
        free(buff);
        return array_buff;
    }
    else {
        fprintf(stderr,"Unhandled type? %d", value.type);
        assert(false);
    }

    return buff;
}

bool is_ascii_only(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        if ((unsigned char)str[i] > 0x7F) {
            // Non-ASCII character found
            return false;
        }
    }
    return true;
}

struct json_value json_parse_string(const char* json_string, allocator alloc) {
    static const char* keywords[] = {"null", "true", "false"};
    static const char* key_symbols = "{}:,[]";

    struct json_value json_error = {
        .type = JSON_ERROR,
        .value.string = json_error_buffer
    };


    if(!is_ascii_only(json_string)) {
        sprintf(json_error_buffer, "UTF-8 Not supported!");
        return json_error;
    }

    token* tokens = tokenise(json_string, keywords, 3, key_symbols);

    size_t token_len = -1;
    while (tokens[++token_len].type != END_OF_TOKENS);;

    size_t index = 0;
    struct json_value value = get_json_value(tokens, &index, alloc);

    if(index != token_len) {
        sprintf(json_error_buffer, "Trailing"); //TODO: better error? Idk what it should say
        return json_error;
    }

    free(tokens);

    return value;
}