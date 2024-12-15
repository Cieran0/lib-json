#include "stdio.h"
#include "json.h"
#include "tokenise.h"
#include "assert.h"
#include "string.h"
#include "math.h"

#include "parse.h"

double json_pow(double x, int y) {
    double out = 1;
    for (size_t i = 0; i < y; i++)
    {
        out*=x;
    }
    return out;
}

char* substr(const char* string, size_t start, size_t end) {
    if(!string) {
        return NULL;
    }

    size_t length = strlen(string);

    if(length == 0 && start == 0 && end == 0) {
        char* output = (char*)malloc(1);
        output[0] = '\0';
        return output;
    }
    
    if(start > end || start >= length || end >= length) {
        return NULL;
    }

    size_t size = end - start + 1;

    char* output = (char*)malloc(size + 1);
    memcpy(output, string+start, size);
    output[size]= '\0';

    return output;
}

bool __parse_long(const char* str, long* value) {
    if(value == NULL) {
        return false;
    }

    size_t len = strlen(str);

    if(len == 0) {
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

bool parse_long(const char* str, long* value) {

    size_t len = strlen(str);

    size_t start = 0;
    if(str[start] == '-') {
        start++;
    }

    if(str[start] == '0' && len-start != 1) {
        return false;
    }

    if(str[0] == '+'){
        return false;
    }


    return __parse_long(str, value);
}

bool parse_fraction(const char* str, long* value) {

    if(str[0] == '+'){
        return false;
    }

    return __parse_long(str, value);
}

bool parse_exponent(const char* str, long* value) {

    return __parse_long(str, value);
}

bool parse_double(const char* str, double* value) {
    if(value == NULL) {
        return false;
    }

    size_t len = strlen(str);

    long before_dot;
    long after_dot;
    long after_expo;
    long exponent = 0;
    double mantissa = 0;

    int dot_pos = -1;
    int expo_pos = -1;

    for (size_t i = 0; i < len; i++) {
        if(str[i] == '.') {
            dot_pos = i;
        } else if (str[i] == 'e' || str[i] == 'E') {
            expo_pos = i;
        }

        if(dot_pos != -1 && expo_pos != -1) {
            break;
        }
    }

    if(expo_pos != -1 && (expo_pos-dot_pos) <= 1) {
        return false;
    }

    if(expo_pos == len-1 || dot_pos == len-1) {
        return false;
    }

    char* before_dot_str = NULL;
    char* after_dot_str = NULL;
    char* expo_str = NULL;
    char* left = (char*)str;
    bool left_free = true;

    if(expo_pos != -1) {
        expo_str = substr(str, expo_pos+1, len-1);
        left = substr(str, 0, expo_pos-1);
        left_free = false;
    }

    if(dot_pos != -1) {
        before_dot_str = substr(left, 0, dot_pos-1);
        after_dot_str = substr(left, dot_pos+1, strlen(left)-1);

        if(!left_free) {
            free(left);
            left_free = true;
        }
    } else {
        before_dot_str = left;
    }

    char buff[256] = {0};

    int written = sprintf(buff, "%s", before_dot_str);
    if(after_dot_str) {
        written+= sprintf(buff+written, ".%s", after_dot_str);
    }
    if(expo_str) {
        written+= sprintf(buff+written, "e%s", expo_str);
    }

    if(!parse_long(before_dot_str, &before_dot)) {
        return false; //TODO: free properly
    }    

    if(after_dot_str != NULL && !(parse_fraction(after_dot_str, &after_dot))) {
        return false; //TODO: free properly
    }    

    if(expo_str != NULL && !(parse_exponent(expo_str, &exponent))) {
        return false; //TODO: free properly
    }    

    int len_after_dot = len - dot_pos - 1;
    if (after_dot_str != NULL) {
        long factor = json_pow(10, len_after_dot);
        *value = ((double)before_dot) + (((double)after_dot) / factor);
    } else {
        *value = (double)before_dot;
    }

    if(expo_pos != -1) {
        double factor = json_pow(10, exponent);
        *value = ((double)*value) * factor;
    }

    if(!left_free) {
        free(left);
    }

    return true;
}
