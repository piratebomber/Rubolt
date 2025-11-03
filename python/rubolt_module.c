#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/interpreter.h"

static PyObject* rubolt_run(PyObject* self, PyObject* args) {
    const char* source;
    if (!PyArg_ParseTuple(args, "s", &source)) {
        return NULL;
    }

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    size_t stmt_count;
    Stmt** statements = parse(&parser, &stmt_count);

    if (parser.had_error) {
        for (size_t i = 0; i < stmt_count; i++) {
            stmt_free(statements[i]);
        }
        free(statements);
        PyErr_SetString(PyExc_SyntaxError, "Parse error in Rubolt code");
        return NULL;
    }

    interpret(statements, stmt_count);

    for (size_t i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);

    Py_RETURN_NONE;
}

static PyObject* rubolt_run_file(PyObject* self, PyObject* args) {
    const char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        PyErr_SetString(PyExc_FileNotFoundError, "Could not open file");
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(file);
        PyErr_SetString(PyExc_MemoryError, "Not enough memory");
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    Lexer lexer;
    lexer_init(&lexer, buffer);

    Parser parser;
    parser_init(&parser, &lexer);

    size_t stmt_count;
    Stmt** statements = parse(&parser, &stmt_count);

    if (parser.had_error) {
        for (size_t i = 0; i < stmt_count; i++) {
            stmt_free(statements[i]);
        }
        free(statements);
        free(buffer);
        PyErr_SetString(PyExc_SyntaxError, "Parse error in Rubolt code");
        return NULL;
    }

    interpret(statements, stmt_count);

    for (size_t i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);
    free(buffer);

    Py_RETURN_NONE;
}

static PyMethodDef RuboltMethods[] = {
    {"run", rubolt_run, METH_VARARGS, "Run Rubolt code from a string"},
    {"run_file", rubolt_run_file, METH_VARARGS, "Run Rubolt code from a file"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef ruboltmodule = {
    PyModuleDef_HEAD_INIT,
    "rubolt",
    "Rubolt programming language Python bindings",
    -1,
    RuboltMethods
};

PyMODINIT_FUNC PyInit_rubolt(void) {
    return PyModule_Create(&ruboltmodule);
}
