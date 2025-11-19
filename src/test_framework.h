#ifndef RUBOLT_TEST_FRAMEWORK_H
#define RUBOLT_TEST_FRAMEWORK_H

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TEST_RESULT_PASS,
    TEST_RESULT_FAIL,
    TEST_RESULT_SKIP,
    TEST_RESULT_ERROR
} TestResult;

typedef struct {
    char* name;
    char* description;
    TestResult result;
    char* error_message;
    double execution_time;
    size_t assertion_count;
    size_t failed_assertions;
} TestCase;

typedef struct {
    char* name;
    TestCase* tests;
    size_t test_count;
    size_t passed;
    size_t failed;
    size_t skipped;
    size_t errors;
    double total_time;
} TestSuite;

typedef struct {
    TestSuite* suites;
    size_t suite_count;
    size_t total_tests;
    size_t total_passed;
    size_t total_failed;
    size_t total_skipped;
    size_t total_errors;
    double total_execution_time;
} TestRunner;

typedef struct {
    Value expected;
    Value actual;
    char* message;
    bool passed;
} Assertion;

// Test framework operations
void test_runner_init(TestRunner* runner);
void test_runner_free(TestRunner* runner);
TestSuite* test_runner_add_suite(TestRunner* runner, const char* name);
void test_runner_run_all(TestRunner* runner);
void test_runner_print_results(TestRunner* runner);

// Test suite operations
void test_suite_init(TestSuite* suite, const char* name);
void test_suite_free(TestSuite* suite);
TestCase* test_suite_add_test(TestSuite* suite, const char* name, const char* description);
void test_suite_run(TestSuite* suite);

// Test case operations
void test_case_init(TestCase* test, const char* name, const char* description);
void test_case_free(TestCase* test);
void test_case_run(TestCase* test, Environment* env);

// Assertion functions
bool assert_true(bool condition, const char* message);
bool assert_false(bool condition, const char* message);
bool assert_equal(Value expected, Value actual, const char* message);
bool assert_not_equal(Value expected, Value actual, const char* message);
bool assert_null(Value value, const char* message);
bool assert_not_null(Value value, const char* message);
bool assert_greater_than(Value a, Value b, const char* message);
bool assert_less_than(Value a, Value b, const char* message);
bool assert_contains(Value container, Value item, const char* message);
bool assert_throws(void (*func)(void), const char* expected_error, const char* message);

// Property-based testing
typedef struct {
    Value (*generator)(void);
    bool (*property)(Value input);
    size_t test_count;
    size_t max_shrink_attempts;
} PropertyTest;

typedef struct {
    Value* values;
    size_t count;
    size_t capacity;
} ValueGenerator;

// Property test operations
PropertyTest* property_test_create(Value (*generator)(void), bool (*property)(Value));
void property_test_free(PropertyTest* test);
bool property_test_run(PropertyTest* test, size_t iterations);

// Value generators
Value generate_int(int min, int max);
Value generate_string(size_t min_length, size_t max_length);
Value generate_list(size_t min_size, size_t max_size, Value (*element_gen)(void));
Value generate_bool();
Value generate_float(double min, double max);

// Shrinking for property tests
Value shrink_int(Value value);
Value shrink_string(Value value);
Value shrink_list(Value value);

// Benchmark framework
typedef struct {
    char* name;
    void (*setup)(void);
    void (*benchmark)(void);
    void (*teardown)(void);
    size_t iterations;
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
} Benchmark;

typedef struct {
    Benchmark* benchmarks;
    size_t benchmark_count;
} BenchmarkSuite;

// Benchmark operations
void benchmark_suite_init(BenchmarkSuite* suite);
void benchmark_suite_free(BenchmarkSuite* suite);
Benchmark* benchmark_suite_add(BenchmarkSuite* suite, const char* name,
                              void (*setup)(void), void (*benchmark)(void), void (*teardown)(void));
void benchmark_suite_run(BenchmarkSuite* suite, size_t iterations);
void benchmark_suite_print_results(BenchmarkSuite* suite);

// Mock framework
typedef struct MockCall {
    char* function_name;
    Value* args;
    size_t arg_count;
    Value return_value;
    struct MockCall* next;
} MockCall;

typedef struct {
    MockCall* expected_calls;
    MockCall* actual_calls;
    size_t call_count;
} MockObject;

// Mock operations
MockObject* mock_create();
void mock_free(MockObject* mock);
void mock_expect_call(MockObject* mock, const char* function_name, Value* args, size_t arg_count, Value return_value);
Value mock_call(MockObject* mock, const char* function_name, Value* args, size_t arg_count);
bool mock_verify(MockObject* mock);

#endif