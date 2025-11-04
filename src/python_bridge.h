#ifndef RUBOLT_PYTHON_BRIDGE_H
#define RUBOLT_PYTHON_BRIDGE_H

#include <stddef.h>
#include <stdbool.h>

/* Python API loading status */
typedef enum {
    PY_API_NOT_LOADED,
    PY_API_LOADING,
    PY_API_LOADED,
    PY_API_ERROR
} PyApiStatus;

/* Python object wrapper */
typedef struct PyRuboltObject {
    void *py_object;           /* PyObject* */
    char *type_name;
    bool borrowed_ref;
} PyRuboltObject;

/* Rubolt value for Python */
typedef struct RuboltValue {
    enum {
        RB_TYPE_INT,
        RB_TYPE_FLOAT,
        RB_TYPE_STRING,
        RB_TYPE_BOOL,
        RB_TYPE_LIST,
        RB_TYPE_DICT,
        RB_TYPE_NONE,
        RB_TYPE_FUNCTION
    } type;
    union {
        long long i;
        double f;
        char *s;
        bool b;
        void *ptr;
    } data;
} RuboltValue;

/* Python bridge state */
typedef struct PythonBridge {
    PyApiStatus status;
    void *py_module;           /* Rubolt module in Python */
    void *py_dict;             /* Global Python dict */
    bool initialized;
    char *last_error;
} PythonBridge;

/* ========== BRIDGE LIFECYCLE ========== */

/* Initialize Python bridge */
bool pybridge_init(PythonBridge *bridge);

/* Shutdown Python bridge */
void pybridge_shutdown(PythonBridge *bridge);

/* Check if Python is available */
bool pybridge_is_available(void);

/* Get last error */
const char *pybridge_get_error(PythonBridge *bridge);

/* ========== PYTHON TO RUBOLT ========== */

/* Convert Python object to Rubolt value */
RuboltValue pybridge_to_rubolt(PyRuboltObject *py_obj);

/* Call Rubolt function from Python */
RuboltValue pybridge_call_rubolt_function(const char *func_name, RuboltValue *args, size_t arg_count);

/* Import Rubolt module in Python */
bool pybridge_import_rubolt_module(PythonBridge *bridge, const char *module_name);

/* ========== RUBOLT TO PYTHON ========== */

/* Convert Rubolt value to Python object */
PyRuboltObject *pybridge_to_python(RuboltValue value);

/* Call Python function from Rubolt */
RuboltValue pybridge_call_python_function(PythonBridge *bridge, const char *func_name, 
                                          RuboltValue *args, size_t arg_count);

/* Execute Python code */
bool pybridge_exec_python(PythonBridge *bridge, const char *code);

/* Evaluate Python expression */
RuboltValue pybridge_eval_python(PythonBridge *bridge, const char *expr);

/* Import Python module */
bool pybridge_import_python_module(PythonBridge *bridge, const char *module_name);

/* ========== OBJECT MANAGEMENT ========== */

/* Create Python object wrapper */
PyRuboltObject *pybridge_wrap_object(void *py_obj, bool borrowed);

/* Free Python object wrapper */
void pybridge_free_object(PyRuboltObject *obj);

/* Increment Python object reference */
void pybridge_incref(PyRuboltObject *obj);

/* Decrement Python object reference */
void pybridge_decref(PyRuboltObject *obj);

/* ========== ATTRIBUTE ACCESS ========== */

/* Get attribute from Python object */
RuboltValue pybridge_get_attr(PyRuboltObject *obj, const char *attr_name);

/* Set attribute on Python object */
bool pybridge_set_attr(PyRuboltObject *obj, const char *attr_name, RuboltValue value);

/* Check if object has attribute */
bool pybridge_has_attr(PyRuboltObject *obj, const char *attr_name);

/* ========== METHOD CALLS ========== */

/* Call method on Python object */
RuboltValue pybridge_call_method(PyRuboltObject *obj, const char *method_name, 
                                 RuboltValue *args, size_t arg_count);

/* ========== TYPE CONVERSIONS ========== */

/* Convert Python list to Rubolt array */
RuboltValue *pybridge_list_to_array(PyRuboltObject *list, size_t *size);

/* Convert Rubolt array to Python list */
PyRuboltObject *pybridge_array_to_list(RuboltValue *array, size_t size);

/* Convert Python dict to Rubolt hash */
void *pybridge_dict_to_hash(PyRuboltObject *dict);

/* Convert Rubolt hash to Python dict */
PyRuboltObject *pybridge_hash_to_dict(void *hash);

/* ========== EXCEPTION HANDLING ========== */

/* Check for Python exception */
bool pybridge_has_exception(void);

/* Get Python exception info */
char *pybridge_get_exception(void);

/* Clear Python exception */
void pybridge_clear_exception(void);

/* Raise Python exception from Rubolt */
void pybridge_raise_exception(const char *exc_type, const char *message);

/* ========== UTILITIES ========== */

/* Get Python version */
const char *pybridge_get_python_version(void);

/* Get Rubolt object type name */
const char *pybridge_type_name(PyRuboltObject *obj);

/* Check if object is callable */
bool pybridge_is_callable(PyRuboltObject *obj);

/* Get object string representation */
char *pybridge_to_string(PyRuboltObject *obj);

/* ========== EMBEDDING API ========== */

/* Add Rubolt function to Python */
bool pybridge_register_rubolt_function(PythonBridge *bridge, const char *name, 
                                       void *func_ptr, const char *doc);

/* Add Rubolt class to Python */
bool pybridge_register_rubolt_class(PythonBridge *bridge, const char *name, 
                                    void *class_def);

/* Expose Rubolt module to Python */
bool pybridge_expose_module(PythonBridge *bridge, const char *module_name);

/* ========== DYNAMIC LOADING ========== */

#ifdef _WIN32
#define PY_LIB_NAME "python3.dll"
#else
#define PY_LIB_NAME "libpython3.so"
#endif

/* Load Python library dynamically */
bool pybridge_load_library(const char *lib_path);

/* Unload Python library */
void pybridge_unload_library(void);

/* Global Python bridge */
extern PythonBridge *global_python_bridge;

#endif /* RUBOLT_PYTHON_BRIDGE_H */
