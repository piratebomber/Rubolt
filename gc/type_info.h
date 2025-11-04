#ifndef RUBOLT_TYPE_INFO_H
#define RUBOLT_TYPE_INFO_H

#include <stddef.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct TypeInfo TypeInfo;
typedef struct FieldInfo FieldInfo;

/* Field types */
typedef enum {
    FIELD_PRIMITIVE,      /* Non-pointer primitive (int, float, etc.) */
    FIELD_POINTER,        /* Pointer to another managed object */
    FIELD_ARRAY,          /* Array of objects */
    FIELD_STRING,         /* String (char*) */
    FIELD_EMBEDDED,       /* Embedded struct (not pointer) */
} FieldType;

/* Field information describing a single field in a struct */
struct FieldInfo {
    const char *name;           /* Field name */
    FieldType type;             /* Field type */
    size_t offset;              /* Offset from struct start */
    size_t size;                /* Size of field */
    TypeInfo *referenced_type;  /* For pointers/embedded - type of target */
    size_t array_length;        /* For arrays - number of elements (0 = dynamic) */
};

/* Type information describing an object type */
struct TypeInfo {
    const char *name;           /* Type name */
    size_t size;                /* Total size of type */
    size_t field_count;         /* Number of fields */
    FieldInfo *fields;          /* Array of field descriptors */
    void (*destructor)(void *); /* Optional destructor */
    bool registered;            /* Is this type registered? */
    struct TypeInfo *next;      /* For type registry linked list */
};

/* Type registry */
typedef struct TypeRegistry {
    TypeInfo *types;            /* Linked list of registered types */
    size_t type_count;          /* Number of registered types */
} TypeRegistry;

/* Initialize type registry */
void type_registry_init(TypeRegistry *registry);

/* Register a type */
void type_register(TypeRegistry *registry, TypeInfo *type_info);

/* Find a registered type by name */
TypeInfo *type_find(TypeRegistry *registry, const char *name);

/* Create a field descriptor */
FieldInfo field_primitive(const char *name, size_t offset, size_t size);
FieldInfo field_pointer(const char *name, size_t offset, TypeInfo *target_type);
FieldInfo field_array(const char *name, size_t offset, TypeInfo *element_type, size_t length);
FieldInfo field_string(const char *name, size_t offset);
FieldInfo field_embedded(const char *name, size_t offset, TypeInfo *embedded_type);

/* Helper macros for field definition */
#define FIELD_OFFSET(type, field) offsetof(type, field)
#define FIELD_SIZE(type, field) sizeof(((type *)0)->field)

/* Traverse object and call visitor for each pointer field */
typedef void (*PointerVisitor)(void *object, void *pointer_field, void *context);
void type_traverse_pointers(TypeInfo *type, void *object, PointerVisitor visitor, void *context);

/* Check if an object contains any pointers */
bool type_has_pointers(TypeInfo *type);

/* Get number of pointer fields in type */
size_t type_count_pointers(TypeInfo *type);

/* Print type information (debugging) */
void type_print_info(TypeInfo *type);

/* Global type registry */
extern TypeRegistry *global_type_registry;

#endif /* RUBOLT_TYPE_INFO_H */
