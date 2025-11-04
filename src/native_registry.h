#ifndef RUBOLT_NATIVE_REGISTRY_H
#define RUBOLT_NATIVE_REGISTRY_H

#include "interpreter.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef Value (*RbNativeFn)(Environment*, Value*, size_t);

void native_registry_init(void);
void native_registry_free(void);
int  native_register(const char* name, RbNativeFn fn);
RbNativeFn native_find(const char* name);
void native_list(void);

#ifdef __cplusplus
}
#endif

#endif