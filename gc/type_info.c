#include "type_info.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global type registry */
TypeRegistry *global_type_registry = NULL;

/* Initialize type registry */
void type_registry_init(TypeRegistry *registry) {
    registry->types = NULL;
    registry->type_count = 0;
}

/* Register a type */
void type_register(TypeRegistry *registry, TypeInfo *type_info) {
    if (!type_info || type_info->registered) return;
    
    /* Add to linked list */
    type_info->next = registry->types;
    registry->types = type_info;
    type_info->registered = true;
    registry->type_count++;
}

/* Find a registered type by name */
TypeInfo *type_find(TypeRegistry *registry, const char *name) {
    TypeInfo *current = registry->types;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Create a primitive field descriptor */
FieldInfo field_primitive(const char *name, size_t offset, size_t size) {
    FieldInfo field;
    field.name = name;
    field.type = FIELD_PRIMITIVE;
    field.offset = offset;
    field.size = size;
    field.referenced_type = NULL;
    field.array_length = 0;
    return field;
}

/* Create a pointer field descriptor */
FieldInfo field_pointer(const char *name, size_t offset, TypeInfo *target_type) {
    FieldInfo field;
    field.name = name;
    field.type = FIELD_POINTER;
    field.offset = offset;
    field.size = sizeof(void *);
    field.referenced_type = target_type;
    field.array_length = 0;
    return field;
}

/* Create an array field descriptor */
FieldInfo field_array(const char *name, size_t offset, TypeInfo *element_type, size_t length) {
    FieldInfo field;
    field.name = name;
    field.type = FIELD_ARRAY;
    field.offset = offset;
    field.size = sizeof(void *);
    field.referenced_type = element_type;
    field.array_length = length;
    return field;
}

/* Create a string field descriptor */
FieldInfo field_string(const char *name, size_t offset) {
    FieldInfo field;
    field.name = name;
    field.type = FIELD_STRING;
    field.offset = offset;
    field.size = sizeof(char *);
    field.referenced_type = NULL;
    field.array_length = 0;
    return field;
}

/* Create an embedded struct field descriptor */
FieldInfo field_embedded(const char *name, size_t offset, TypeInfo *embedded_type) {
    FieldInfo field;
    field.name = name;
    field.type = FIELD_EMBEDDED;
    field.offset = offset;
    field.size = embedded_type ? embedded_type->size : 0;
    field.referenced_type = embedded_type;
    field.array_length = 0;
    return field;
}

/* Recursive helper to traverse embedded types */
static void traverse_embedded(TypeInfo *type, void *object, PointerVisitor visitor, void *context);

/* Traverse object and call visitor for each pointer field */
void type_traverse_pointers(TypeInfo *type, void *object, PointerVisitor visitor, void *context) {
    if (!type || !object || !visitor) return;
    
    for (size_t i = 0; i < type->field_count; i++) {
        FieldInfo *field = &type->fields[i];
        void *field_addr = (char *)object + field->offset;
        
        switch (field->type) {
            case FIELD_POINTER: {
                /* Direct pointer field */
                void **ptr_field = (void **)field_addr;
                if (*ptr_field) {
                    visitor(object, *ptr_field, context);
                }
                break;
            }
            
            case FIELD_ARRAY: {
                /* Array of pointers */
                void **array = *(void ***)field_addr;
                if (array) {
                    size_t length = field->array_length;
                    if (length == 0) {
                        /* Dynamic array - can't determine length without metadata */
                        /* Would need additional size tracking */
                        break;
                    }
                    for (size_t j = 0; j < length; j++) {
                        if (array[j]) {
                            visitor(object, array[j], context);
                        }
                    }
                }
                break;
            }
            
            case FIELD_STRING: {
                /* String is a char* - managed pointer */
                char **str_field = (char **)field_addr;
                if (*str_field) {
                    visitor(object, *str_field, context);
                }
                break;
            }
            
            case FIELD_EMBEDDED: {
                /* Embedded struct - recursively traverse */
                if (field->referenced_type) {
                    traverse_embedded(field->referenced_type, field_addr, visitor, context);
                }
                break;
            }
            
            case FIELD_PRIMITIVE:
                /* No pointers to traverse */
                break;
        }
    }
}

/* Recursive helper to traverse embedded types */
static void traverse_embedded(TypeInfo *type, void *object, PointerVisitor visitor, void *context) {
    type_traverse_pointers(type, object, visitor, context);
}

/* Check if an object contains any pointers */
bool type_has_pointers(TypeInfo *type) {
    if (!type) return false;
    
    for (size_t i = 0; i < type->field_count; i++) {
        FieldInfo *field = &type->fields[i];
        switch (field->type) {
            case FIELD_POINTER:
            case FIELD_ARRAY:
            case FIELD_STRING:
                return true;
            case FIELD_EMBEDDED:
                if (field->referenced_type && type_has_pointers(field->referenced_type)) {
                    return true;
                }
                break;
            case FIELD_PRIMITIVE:
                break;
        }
    }
    return false;
}

/* Get number of pointer fields in type */
size_t type_count_pointers(TypeInfo *type) {
    if (!type) return 0;
    
    size_t count = 0;
    for (size_t i = 0; i < type->field_count; i++) {
        FieldInfo *field = &type->fields[i];
        switch (field->type) {
            case FIELD_POINTER:
            case FIELD_STRING:
                count++;
                break;
            case FIELD_ARRAY:
                count += field->array_length;
                break;
            case FIELD_EMBEDDED:
                if (field->referenced_type) {
                    count += type_count_pointers(field->referenced_type);
                }
                break;
            case FIELD_PRIMITIVE:
                break;
        }
    }
    return count;
}

/* Print type information (debugging) */
void type_print_info(TypeInfo *type) {
    if (!type) {
        printf("TypeInfo: NULL\n");
        return;
    }
    
    printf("Type: %s\n", type->name);
    printf("  Size: %zu bytes\n", type->size);
    printf("  Fields: %zu\n", type->field_count);
    
    for (size_t i = 0; i < type->field_count; i++) {
        FieldInfo *field = &type->fields[i];
        printf("    [%zu] %s: ", i, field->name);
        
        switch (field->type) {
            case FIELD_PRIMITIVE:
                printf("primitive (%zu bytes)\n", field->size);
                break;
            case FIELD_POINTER:
                printf("pointer -> %s\n", 
                       field->referenced_type ? field->referenced_type->name : "unknown");
                break;
            case FIELD_ARRAY:
                printf("array[%zu] of %s\n", 
                       field->array_length,
                       field->referenced_type ? field->referenced_type->name : "unknown");
                break;
            case FIELD_STRING:
                printf("string\n");
                break;
            case FIELD_EMBEDDED:
                printf("embedded %s\n",
                       field->referenced_type ? field->referenced_type->name : "unknown");
                break;
        }
    }
}
