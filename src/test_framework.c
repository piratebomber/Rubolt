#include "test_framework.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

static TestCase* current_test = NULL;

void test_runner_init(TestRunner* runner) {
    runner->suites = NULL;
    runner->suite_count = 0;
    runner->total_tests = 0;
    runner->total_passed = 0;
    runner->total_failed = 0;
    runner->total_skipped = 0;
    runner->total_errors = 0;
    runner->total_execution_time = 0.0;
}

void test_runner_free(TestRunner* runner) {
    for (size_t i = 0; i < runner->suite_count; i++) {
        test_suite_free(&runner->suites[i]);
    }
    free(runner->suites);
}

TestSuite* test_runner_add_suite(TestRunner* runner, const char* name) {
    runner->suites = realloc(runner->suites, (runner->suite_count + 1) * sizeof(TestSuite));
    TestSuite* suite = &runner->suites[runner->suite_count];
    test_suite_init(suite, name);
    runner->suite_count++;
    return suite;
}

void test_runner_run_all(TestRunner* runner) {
    printf("Running test suites...\n\n");
    
    clock_t start = clock();
    
    for (size_t i = 0; i < runner->suite_count; i++) {
        test_suite_run(&runner->suites[i]);
        
        runner->total_tests += runner->suites[i].test_count;
        runner->total_passed += runner->suites[i].passed;
        runner->total_failed += runner->suites[i].failed;
        runner->total_skipped += runner->suites[i].skipped;
        runner->total_errors += runner->suites[i].errors;
    }
    
    clock_t end = clock();
    runner->total_execution_time = ((double)(end - start)) / CLOCKS_PER_SEC;
}

void test_runner_print_results(TestRunner* runner) {
    printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("TEST RESULTS\n");
    printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    
    for (size_t i = 0; i < runner->suite_count; i++) {
        TestSuite* suite = &runner->suites[i];
        printf("Suite: %s\n", suite->name);
        printf("  Tests: %zu, Passed: %zu, Failed: %zu, Skipped: %zu, Errors: %zu\n",
               suite->test_count, suite->passed, suite->failed, suite->skipped, suite->errors);
        printf("  Time: %.3fs\n\n", suite->total_time);
    }
    
    printf("TOTAL: %zu tests, %zu passed, %zu failed, %zu skipped, %zu errors\n",
           runner->total_tests, runner->total_passed, runner->total_failed,
           runner->total_skipped, runner->total_errors);
    printf("Total time: %.3fs\n", runner->total_execution_time);
    
    if (runner->total_failed > 0 || runner->total_errors > 0) {
        printf("\nFAILED TESTS:\n");
        for (size_t i = 0; i < runner->suite_count; i++) {
            TestSuite* suite = &runner->suites[i];
            for (size_t j = 0; j < suite->test_count; j++) {
                TestCase* test = &suite->tests[j];
                if (test->result == TEST_RESULT_FAIL || test->result == TEST_RESULT_ERROR) {
                    printf("  %s::%s - %s\n", suite->name, test->name, 
                           test->error_message ? test->error_message : "No message");
                }
            }
        }
    }
    
    printf("\n");
}

void test_suite_init(TestSuite* suite, const char* name) {
    suite->name = strdup(name);
    suite->tests = NULL;
    suite->test_count = 0;
    suite->passed = 0;
    suite->failed = 0;
    suite->skipped = 0;
    suite->errors = 0;
    suite->total_time = 0.0;
}

void test_suite_free(TestSuite* suite) {
    free(suite->name);
    for (size_t i = 0; i < suite->test_count; i++) {
        test_case_free(&suite->tests[i]);
    }
    free(suite->tests);
}

TestCase* test_suite_add_test(TestSuite* suite, const char* name, const char* description) {
    suite->tests = realloc(suite->tests, (suite->test_count + 1) * sizeof(TestCase));
    TestCase* test = &suite->tests[suite->test_count];
    test_case_init(test, name, description);
    suite->test_count++;
    return test;
}

void test_suite_run(TestSuite* suite) {
    printf("Running suite: %s\n", suite->name);
    
    clock_t suite_start = clock();
    
    for (size_t i = 0; i < suite->test_count; i++) {
        TestCase* test = &suite->tests[i];
        current_test = test;
        
        clock_t test_start = clock();
        
        // Create test environment
        Environment env;
        environment_init(&env);
        
        test_case_run(test, &env);
        
        environment_free(&env);
        
        clock_t test_end = clock();
        test->execution_time = ((double)(test_end - test_start)) / CLOCKS_PER_SEC;
        
        // Update suite statistics
        switch (test->result) {
            case TEST_RESULT_PASS:
                suite->passed++;
                printf("  ✓ %s (%.3fs)\n", test->name, test->execution_time);
                break;
            case TEST_RESULT_FAIL:
                suite->failed++;
                printf("  ✗ %s - %s (%.3fs)\n", test->name, 
                       test->error_message ? test->error_message : "Failed", 
                       test->execution_time);
                break;
            case TEST_RESULT_SKIP:
                suite->skipped++;
                printf("  - %s (skipped)\n", test->name);
                break;
            case TEST_RESULT_ERROR:
                suite->errors++;
                printf("  ! %s - %s (%.3fs)\n", test->name,
                       test->error_message ? test->error_message : "Error",
                       test->execution_time);
                break;
        }
    }
    
    clock_t suite_end = clock();
    suite->total_time = ((double)(suite_end - suite_start)) / CLOCKS_PER_SEC;
    
    printf("Suite completed: %zu/%zu passed (%.3fs)\n\n", 
           suite->passed, suite->test_count, suite->total_time);
}

void test_case_init(TestCase* test, const char* name, const char* description) {
    test->name = strdup(name);
    test->description = strdup(description);
    test->result = TEST_RESULT_PASS;
    test->error_message = NULL;
    test->execution_time = 0.0;
    test->assertion_count = 0;
    test->failed_assertions = 0;
}

void test_case_free(TestCase* test) {
    free(test->name);
    free(test->description);
    free(test->error_message);
}

void test_case_run(TestCase* test, Environment* env) {
    // Test execution would be implemented based on test body
    // For now, this is a placeholder that would execute test statements
    test->result = TEST_RESULT_PASS;
}

bool assert_true(bool condition, const char* message) {
    if (current_test) {
        current_test->assertion_count++;
        if (!condition) {
            current_test->failed_assertions++;
            current_test->result = TEST_RESULT_FAIL;
            if (!current_test->error_message) {
                current_test->error_message = strdup(message ? message : "Expected true, got false");
            }
            return false;
        }
    }
    return true;
}

bool assert_false(bool condition, const char* message) {
    return assert_true(!condition, message ? message : "Expected false, got true");
}

bool assert_equal(Value expected, Value actual, const char* message) {
    if (current_test) {
        current_test->assertion_count++;
        
        bool equal = false;
        if (expected.type == actual.type) {
            switch (expected.type) {
                case VAL_NULL:
                    equal = true;
                    break;
                case VAL_BOOL:
                    equal = expected.as.boolean == actual.as.boolean;
                    break;
                case VAL_NUMBER:
                    equal = fabs(expected.as.number - actual.as.number) < 1e-10;
                    break;
                case VAL_STRING:
                    equal = strcmp(expected.as.string, actual.as.string) == 0;
                    break;
            }
        }
        
        if (!equal) {
            current_test->failed_assertions++;
            current_test->result = TEST_RESULT_FAIL;
            if (!current_test->error_message) {
                char* error_msg = malloc(256);
                snprintf(error_msg, 256, "%s - Expected: %s, Actual: %s",
                        message ? message : "Values not equal",
                        value_to_string(expected),
                        value_to_string(actual));
                current_test->error_message = error_msg;
            }
            return false;
        }
    }
    return true;
}

bool assert_not_equal(Value expected, Value actual, const char* message) {
    // Inverse of assert_equal
    bool equal = assert_equal(expected, actual, NULL);
    if (equal && current_test) {
        current_test->result = TEST_RESULT_FAIL;
        if (!current_test->error_message) {
            current_test->error_message = strdup(message ? message : "Values should not be equal");
        }
        return false;
    }
    return !equal;
}

bool assert_null(Value value, const char* message) {
    return assert_true(value.type == VAL_NULL, message ? message : "Expected null value");
}

bool assert_not_null(Value value, const char* message) {
    return assert_false(value.type == VAL_NULL, message ? message : "Expected non-null value");
}

bool assert_greater_than(Value a, Value b, const char* message) {
    if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
        return assert_true(a.as.number > b.as.number, 
                          message ? message : "Expected a > b");
    }
    return assert_true(false, "Cannot compare non-numeric values");
}

bool assert_less_than(Value a, Value b, const char* message) {
    if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
        return assert_true(a.as.number < b.as.number,
                          message ? message : "Expected a < b");
    }
    return assert_true(false, "Cannot compare non-numeric values");
}

// Property-based testing implementation
PropertyTest* property_test_create(Value (*generator)(void), bool (*property)(Value)) {
    PropertyTest* test = malloc(sizeof(PropertyTest));
    test->generator = generator;
    test->property = property;
    test->test_count = 100; // Default iterations
    test->max_shrink_attempts = 100;
    return test;
}

void property_test_free(PropertyTest* test) {
    free(test);
}

bool property_test_run(PropertyTest* test, size_t iterations) {
    for (size_t i = 0; i < iterations; i++) {
        Value input = test->generator();
        
        if (!test->property(input)) {
            // Property failed, try to shrink
            Value shrunk = input;
            for (size_t j = 0; j < test->max_shrink_attempts; j++) {
                Value candidate = shrink_value(shrunk);
                if (!test->property(candidate)) {
                    shrunk = candidate;
                } else {
                    break;
                }
            }
            
            printf("Property test failed with input: %s\n", value_to_string(shrunk));
            return false;
        }
    }
    return true;
}

// Value generators
Value generate_int(int min, int max) {
    int value = min + rand() % (max - min + 1);
    return value_number((double)value);
}

Value generate_string(size_t min_length, size_t max_length) {
    size_t length = min_length + rand() % (max_length - min_length + 1);
    char* str = malloc(length + 1);
    
    for (size_t i = 0; i < length; i++) {
        str[i] = 'a' + rand() % 26;
    }
    str[length] = '\0';
    
    Value result = value_string(str);
    free(str);
    return result;
}

Value generate_bool() {
    return value_bool(rand() % 2 == 0);
}

Value generate_float(double min, double max) {
    double value = min + ((double)rand() / RAND_MAX) * (max - min);
    return value_number(value);
}

// Benchmark framework
void benchmark_suite_init(BenchmarkSuite* suite) {
    suite->benchmarks = NULL;
    suite->benchmark_count = 0;
}

void benchmark_suite_free(BenchmarkSuite* suite) {
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        free(suite->benchmarks[i].name);
    }
    free(suite->benchmarks);
}

Benchmark* benchmark_suite_add(BenchmarkSuite* suite, const char* name,
                              void (*setup)(void), void (*benchmark)(void), void (*teardown)(void)) {
    suite->benchmarks = realloc(suite->benchmarks, 
                               (suite->benchmark_count + 1) * sizeof(Benchmark));
    
    Benchmark* bench = &suite->benchmarks[suite->benchmark_count];
    bench->name = strdup(name);
    bench->setup = setup;
    bench->benchmark = benchmark;
    bench->teardown = teardown;
    bench->iterations = 0;
    bench->total_time = 0.0;
    bench->min_time = INFINITY;
    bench->max_time = 0.0;
    bench->avg_time = 0.0;
    
    suite->benchmark_count++;
    return bench;
}

void benchmark_suite_run(BenchmarkSuite* suite, size_t iterations) {
    printf("Running benchmarks...\n\n");
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        Benchmark* bench = &suite->benchmarks[i];
        bench->iterations = iterations;
        
        printf("Benchmark: %s\n", bench->name);
        
        for (size_t j = 0; j < iterations; j++) {
            if (bench->setup) bench->setup();
            
            clock_t start = clock();
            bench->benchmark();
            clock_t end = clock();
            
            if (bench->teardown) bench->teardown();
            
            double time = ((double)(end - start)) / CLOCKS_PER_SEC;
            bench->total_time += time;
            
            if (time < bench->min_time) bench->min_time = time;
            if (time > bench->max_time) bench->max_time = time;
        }
        
        bench->avg_time = bench->total_time / iterations;
        
        printf("  Iterations: %zu\n", iterations);
        printf("  Total time: %.6fs\n", bench->total_time);
        printf("  Average time: %.6fs\n", bench->avg_time);
        printf("  Min time: %.6fs\n", bench->min_time);
        printf("  Max time: %.6fs\n", bench->max_time);
        printf("\n");
    }
}

// Mock framework
MockObject* mock_create() {
    MockObject* mock = malloc(sizeof(MockObject));
    mock->expected_calls = NULL;
    mock->actual_calls = NULL;
    mock->call_count = 0;
    return mock;
}

void mock_free(MockObject* mock) {
    MockCall* call = mock->expected_calls;
    while (call) {
        MockCall* next = call->next;
        free(call->function_name);
        free(call->args);
        free(call);
        call = next;
    }
    
    call = mock->actual_calls;
    while (call) {
        MockCall* next = call->next;
        free(call->function_name);
        free(call->args);
        free(call);
        call = next;
    }
    
    free(mock);
}

void mock_expect_call(MockObject* mock, const char* function_name, 
                     Value* args, size_t arg_count, Value return_value) {
    MockCall* call = malloc(sizeof(MockCall));
    call->function_name = strdup(function_name);
    call->args = malloc(arg_count * sizeof(Value));
    memcpy(call->args, args, arg_count * sizeof(Value));
    call->arg_count = arg_count;
    call->return_value = return_value;
    call->next = mock->expected_calls;
    mock->expected_calls = call;
}

Value mock_call(MockObject* mock, const char* function_name, Value* args, size_t arg_count) {
    // Record actual call
    MockCall* actual = malloc(sizeof(MockCall));
    actual->function_name = strdup(function_name);
    actual->args = malloc(arg_count * sizeof(Value));
    memcpy(actual->args, args, arg_count * sizeof(Value));
    actual->arg_count = arg_count;
    actual->next = mock->actual_calls;
    mock->actual_calls = actual;
    mock->call_count++;
    
    // Find expected call and return value
    MockCall* expected = mock->expected_calls;
    while (expected) {
        if (strcmp(expected->function_name, function_name) == 0 &&
            expected->arg_count == arg_count) {
            // Check arguments match
            bool args_match = true;
            for (size_t i = 0; i < arg_count; i++) {
                if (!values_equal(expected->args[i], args[i])) {
                    args_match = false;
                    break;
                }
            }
            
            if (args_match) {
                return expected->return_value;
            }
        }
        expected = expected->next;
    }
    
    return value_null(); // No matching expectation
}

bool mock_verify(MockObject* mock) {
    // Verify all expected calls were made
    MockCall* expected = mock->expected_calls;
    while (expected) {
        bool found = false;
        MockCall* actual = mock->actual_calls;
        
        while (actual) {
            if (strcmp(expected->function_name, actual->function_name) == 0 &&
                expected->arg_count == actual->arg_count) {
                bool args_match = true;
                for (size_t i = 0; i < expected->arg_count; i++) {
                    if (!values_equal(expected->args[i], actual->args[i])) {
                        args_match = false;
                        break;
                    }
                }
                
                if (args_match) {
                    found = true;
                    break;
                }
            }
            actual = actual->next;
        }
        
        if (!found) {
            return false;
        }
        expected = expected->next;
    }
    
    return true;
}