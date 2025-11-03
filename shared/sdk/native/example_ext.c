#include "rubolt_api.h"
#include <string.h>
#include <stdio.h>

// Example: String reverse function
static RbValue native_reverse(int argc, RbValue* argv) {
    if (argc != 1) {
        rb_raise_error("reverse() takes exactly 1 argument");
        return rb_null();
    }
    
    if (rb_typeof(argv[0]) != RB_TYPE_STRING) {
        rb_raise_error("reverse() argument must be a string");
        return rb_null();
    }
    
    const char* input = rb_as_string(argv[0]);
    int len = strlen(input);
    char* reversed = malloc(len + 1);
    
    for (int i = 0; i < len; i++) {
        reversed[i] = input[len - 1 - i];
    }
    reversed[len] = '\0';
    
    RbValue result = rb_string(reversed);
    free(reversed);
    return result;
}

// Example: Sum of list
static RbValue native_sum_list(int argc, RbValue* argv) {
    if (argc != 1) {
        rb_raise_error("sum_list() takes exactly 1 argument");
        return rb_null();
    }
    
    if (rb_typeof(argv[0]) != RB_TYPE_LIST) {
        rb_raise_error("sum_list() argument must be a list");
        return rb_null();
    }
    
    RbValue list = argv[0];
    int len = rb_list_len(list);
    int64_t sum = 0;
    
    for (int i = 0; i < len; i++) {
        RbValue item = rb_list_get(list, i);
        if (rb_typeof(item) == RB_TYPE_INT) {
            sum += rb_as_int(item);
        }
    }
    
    return rb_int(sum);
}

// Module registration
static RbModuleFunc example_funcs[] = {
    {"reverse", native_reverse},
    {"sum_list", native_sum_list}
};

void rb_init_example_ext(void) {
    rb_register_module("example_ext", example_funcs, 2);
}
