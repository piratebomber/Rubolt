#include "../src/module.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

static Value time_now(Environment* env, Value* args, size_t arg_count) {
    return value_number((double)time(NULL));
}

static Value time_now_ms(Environment* env, Value* args, size_t arg_count) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return value_number((double)(tv.tv_sec * 1000 + tv.tv_usec / 1000));
}

static Value time_sleep(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) return value_null();
    
    double seconds = args[0].as.number;
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1000000000);
    
    nanosleep(&ts, NULL);
    return value_null();
}

static Value time_format(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) return value_null();
    
    time_t timestamp = (time_t)args[0].as.number;
    const char* format = "%Y-%m-%d %H:%M:%S";
    
    if (arg_count >= 2 && args[1].type == VAL_STRING) {
        format = args[1].as.string;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    char buffer[256];
    strftime(buffer, sizeof(buffer), format, tm_info);
    
    return value_string(buffer);
}

static Value time_parse(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_number(-1);
    
    const char* format = "%Y-%m-%d %H:%M:%S";
    if (arg_count >= 2 && args[1].type == VAL_STRING) {
        format = args[1].as.string;
    }
    
    struct tm tm_info = {0};
    if (strptime(args[0].as.string, format, &tm_info) == NULL) {
        return value_number(-1);
    }
    
    return value_number((double)mktime(&tm_info));
}

static Value time_year(Environment* env, Value* args, size_t arg_count) {
    time_t timestamp = time(NULL);
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) {
        timestamp = (time_t)args[0].as.number;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    return value_number((double)(tm_info->tm_year + 1900));
}

static Value time_month(Environment* env, Value* args, size_t arg_count) {
    time_t timestamp = time(NULL);
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) {
        timestamp = (time_t)args[0].as.number;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    return value_number((double)(tm_info->tm_mon + 1));
}

static Value time_day(Environment* env, Value* args, size_t arg_count) {
    time_t timestamp = time(NULL);
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) {
        timestamp = (time_t)args[0].as.number;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    return value_number((double)tm_info->tm_mday);
}

static Value time_hour(Environment* env, Value* args, size_t arg_count) {
    time_t timestamp = time(NULL);
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) {
        timestamp = (time_t)args[0].as.number;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    return value_number((double)tm_info->tm_hour);
}

static Value time_minute(Environment* env, Value* args, size_t arg_count) {
    time_t timestamp = time(NULL);
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) {
        timestamp = (time_t)args[0].as.number;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    return value_number((double)tm_info->tm_min);
}

static Value time_second(Environment* env, Value* args, size_t arg_count) {
    time_t timestamp = time(NULL);
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) {
        timestamp = (time_t)args[0].as.number;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    return value_number((double)tm_info->tm_sec);
}

void register_time_module(ModuleSystem* ms) {
    Module* m = module_system_load(ms, "time");
    module_register_native_function(m, "now", time_now);
    module_register_native_function(m, "now_ms", time_now_ms);
    module_register_native_function(m, "sleep", time_sleep);
    module_register_native_function(m, "format", time_format);
    module_register_native_function(m, "parse", time_parse);
    module_register_native_function(m, "year", time_year);
    module_register_native_function(m, "month", time_month);
    module_register_native_function(m, "day", time_day);
    module_register_native_function(m, "hour", time_hour);
    module_register_native_function(m, "minute", time_minute);
    module_register_native_function(m, "second", time_second);
}