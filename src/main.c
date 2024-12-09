#include "stdio.h"
#include "json.h"
#include "tokenise.h"
#include "assert.h"
#include "string.h"

struct json_object* extract_object(token* tokens, size_t* index);
struct json_array* extract_array(token* tokens, size_t* index);
char* json_value_to_string(struct json_value value);

bool parse_long(const char* str, long* value) {
    if(value == NULL) {
        return false;
    }

    char* end;
    long parsed_value = strtol(str, &end, 10);

    if(*end != '\0'){
        return false;
    }

    *value = parsed_value;
    return true;
}

bool parse_double(const char* str, double* value) {
    if(value == NULL) {
        return false;
    }

    char* end;
    double parsed_value = strtod(str, &end);

    if(*end != '\0'){
        return false;
    }

    *value = parsed_value;
    return true;
}

struct json_value get_json_value(token* tokens, size_t* index) {
    struct json_value value = {0};

    if (tokens[*index].type == END_OF_TOKENS || tokens[*index].type == ERROR_TOKEN) {
        fprintf(stderr, "Unexpected token at index %zu: %s\n", *index, tokens[*index].content);
        assert(false);
    }

    switch (tokens[*index].type) {
    case STRING_TOKEN: {

        value.value.string = tokens[*index].content; // Should be copied so it can get freed independent of tokens
        value.type = JSON_STRING;
        (*index)++;
        break;
    }
    case KEY_SYMBOL_TOKEN: {
        if (tokens[*index].content[0] == '{') {
            value.type = JSON_OBJECT;
            value.value.object = extract_object(tokens, index);
        } else if (tokens[*index].content[0] == '[') {
            value.type = JSON_ARRAY;
            value.value.array = extract_array(tokens, index);
        } else {
            fprintf(stderr,"Unexpected symbol token at index %zu: %s\n", *index, tokens[*index].content);
            assert(false);
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
            fprintf(stderr,"Unexpected keyword token at index %zu: %s\n", *index, content);
            assert(false);
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
            fprintf(stderr,"Invalid numeric value at index %zu: %s\n", *index - 1, content);
            assert(false);
        }
        break;
    }
    default:
        // Unknown or unsupported token type
        fprintf(stderr,"Unhandled token type at index %zu: %d\n", *index, tokens[*index].type);
        assert(false);
        break;
    }

    return value;
}


struct json_pair* get_json_pair(token* tokens, size_t* index) {

    if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ',' ) {
        (*index) += 1;
    }
    if(tokens[*index].type != STRING_TOKEN) {
        return NULL; //Error in format?
    }

    char* key = NULL;
    struct json_value value = {};

    struct json_pair* pair = malloc(sizeof(struct json_pair));

    key = tokens[*index].content; //Should copy

    *index += 2; // Just trust its a ':'

    value = get_json_value(tokens, index);

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

struct json_object* extract_object(token* tokens, size_t* index) {
    struct json_object* object = malloc(sizeof(struct json_object));
    if (!object) {
        fprintf(stderr,"Memory allocation failed for JSON object\n");
        exit(EXIT_FAILURE);
    }

    object->head = NULL;
    object->tail = NULL;

    // Ensure the current token is a '{'
    if (tokens[*index].type != KEY_SYMBOL_TOKEN || tokens[*index].content[0] != '{') {
        fprintf(stderr,"Expected '{' at index %zu, but got '%s'\n", *index, tokens[*index].content);
        free(object);
        exit(EXIT_FAILURE);
    }

    (*index)++; // Skip the '{' token

    while (tokens[*index].type != END_OF_TOKENS) {
        if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == '}') {
            (*index)++; // Skip the '}' token
            return object;
        }

        struct json_pair* next_pair = get_json_pair(tokens, index);
        if (!next_pair) {
            fprintf(stderr,"Error parsing key-value pair at index %zu\n", *index);
            free(object);
            exit(EXIT_FAILURE);
        }

        insert_pair(object, next_pair);
    }

    fprintf(stderr,"Unexpected end of tokens while parsing object, missing '}'\n");
    free(object);
    exit(EXIT_FAILURE);
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

struct json_array* extract_array(token* tokens, size_t* index) {
    struct json_array* array = malloc(sizeof(struct json_array));
    if (!array) {
        fprintf(stderr, "Memory allocation failed for JSON array\n");
        exit(EXIT_FAILURE);
    }

    (*index) += 1; // Skip the '[' token
    size_t size = get_array_size(tokens, index);

    array->size = size;
    array->values = malloc(sizeof(struct json_value) * size);
    if (!array->values) {
        fprintf(stderr, "Memory allocation failed for array values\n");
        free(array);
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < size; i++) {
        array->values[i] = get_json_value(tokens, index);

        if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ',') {
            (*index)++;
        } else if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ']') {
            break;
        } else if (i < size - 1) {
            fprintf(stderr,"Expected ',' or ']' at index %zu, but got '%s'\n", *index, tokens[*index].content);
            free(array->values);
            free(array);
            exit(EXIT_FAILURE);
        }
    }

    if (tokens[*index].type == KEY_SYMBOL_TOKEN && tokens[*index].content[0] == ']') {
        (*index)++;
    } else {
        fprintf(stderr,"Expected ']' at the end of the array at index %zu, but got '%s'\n", *index, tokens[*index].content);
        free(array->values);
        free(array);
        exit(EXIT_FAILURE);
    }

    return array;
}


char* object_to_string(struct json_object* object) {
    char* big_buffer = (char*)malloc(16*1024);
    char* small_buff;
    int written = sprintf(big_buffer, "{\n");

    struct json_pair* next = object->head;
    while (next != NULL)
    {
        small_buff = json_value_to_string(next->value);
        written += sprintf(big_buffer+written, "\"%s\": %s", next->key, small_buff);

        if(next->next != NULL){
            written += sprintf(big_buffer+written, ",");
        }

        written += sprintf(big_buffer+written, "\n");

        free(small_buff);
        next = next->next;
    }

    written = sprintf(big_buffer+written, "}\n");
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
        fprintf(stderr,"Unhandled type? %d\n", value.type);
        assert(false);
    }

    return buff;
}

struct json_object* parse_file(const char* path) {
    static const char* keywords[] = {"null", "true", "false"};
    static const char* key_symbols = "{}:,[]";

    if(!path) {
        return NULL;
    }

    FILE* fd = fopen(path, "r");
    if(fd == NULL) {
        return NULL;
    }

    fseek(fd, 0, SEEK_END);
    size_t file_size = ftell(fd);
    rewind(fd);

    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(fd);
        return NULL;
    }


    size_t bytes_read = fread(buffer, 1, file_size, fd);
    if (bytes_read != file_size) {
        free(buffer);
        fclose(fd);
        return NULL;
    }

    buffer[file_size] = '\0';


    token* tokens = tokenise(buffer, keywords, 3, key_symbols);

    size_t token_len = -1;
    while (tokens[++token_len].type != END_OF_TOKENS);

    size_t index = 0;
    struct json_object* object = extract_object(tokens, &index);

    return object;
}

int main(int argc, char const *argv[])
{

    if(argc < 2) {
        fprintf(stderr, "%s: Must provide file path!\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct json_object* object = parse_file(argv[1]);

    if(!object) {
        fprintf(stderr, "%s: Failed to parse file: \"%s\"\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    char* object_string = object_to_string(object);

    if (!object_string) {
        return EXIT_FAILURE;
    }

    printf("%s\n", object_string);

    return 0;
}
