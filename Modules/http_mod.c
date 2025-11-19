#include "../src/module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    char* data;
    size_t size;
} HttpResponse;

static size_t write_callback(void* contents, size_t size, size_t nmemb, HttpResponse* response) {
    size_t total_size = size * nmemb;
    char* ptr = realloc(response->data, response->size + total_size + 1);
    
    if (!ptr) return 0;
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

static Value http_get(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    
    CURL* curl = curl_easy_init();
    if (!curl) return value_null();
    
    HttpResponse response = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, args[0].as.string);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        free(response.data);
        return value_null();
    }
    
    Value result = value_string(response.data ? response.data : "");
    free(response.data);
    return result;
}

static Value http_post(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) 
        return value_null();
    
    CURL* curl = curl_easy_init();
    if (!curl) return value_null();
    
    HttpResponse response = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, args[0].as.string);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args[1].as.string);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    // Set content type if provided
    struct curl_slist* headers = NULL;
    if (arg_count >= 3 && args[2].type == VAL_STRING) {
        char content_type[256];
        snprintf(content_type, sizeof(content_type), "Content-Type: %s", args[2].as.string);
        headers = curl_slist_append(headers, content_type);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    CURLcode res = curl_easy_perform(curl);
    
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        free(response.data);
        return value_null();
    }
    
    Value result = value_string(response.data ? response.data : "");
    free(response.data);
    return result;
}

static Value http_put(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) 
        return value_null();
    
    CURL* curl = curl_easy_init();
    if (!curl) return value_null();
    
    HttpResponse response = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, args[0].as.string);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args[1].as.string);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        free(response.data);
        return value_null();
    }
    
    Value result = value_string(response.data ? response.data : "");
    free(response.data);
    return result;
}

static Value http_delete(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    
    CURL* curl = curl_easy_init();
    if (!curl) return value_null();
    
    HttpResponse response = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, args[0].as.string);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        free(response.data);
        return value_null();
    }
    
    Value result = value_string(response.data ? response.data : "");
    free(response.data);
    return result;
}

void register_http_module(ModuleSystem* ms) {
    Module* m = module_system_load(ms, "http");
    module_register_native_function(m, "get", http_get);
    module_register_native_function(m, "post", http_post);
    module_register_native_function(m, "put", http_put);
    module_register_native_function(m, "delete", http_delete);
}