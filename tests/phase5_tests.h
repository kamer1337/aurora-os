/**
 * Aurora OS - Phase 5 Testing Framework
 * 
 * Comprehensive testing and validation for Phase 5
 */

#ifndef AURORA_PHASE5_TESTS_H
#define AURORA_PHASE5_TESTS_H

#include <stdint.h>
#include <stdbool.h>

/* Test result structure */
typedef struct {
    const char* test_name;
    bool passed;
    const char* error_message;
    uint32_t execution_time_ms;
} test_result_t;

/* Test suite statistics */
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint32_t total_time_ms;
} test_stats_t;

/* Test function pointer type */
typedef bool (*test_function_t)(void);

/* Performance metrics structure */
typedef struct {
    const char* operation_name;
    uint32_t iterations;
    uint32_t total_time_ms;
    uint32_t min_time_ms;
    uint32_t max_time_ms;
    uint32_t avg_time_ms;
} perf_metric_t;

/**
 * Initialize the Phase 5 testing framework
 */
void phase5_tests_init(void);

/**
 * Run all Phase 5 tests
 * @return Overall test success status
 */
bool phase5_run_all_tests(void);

/**
 * Run kernel subsystem tests
 */
bool test_kernel_subsystems(void);

/**
 * Run file system integrity tests
 */
bool test_filesystem_integrity(void);

/**
 * Run GUI framework tests
 */
bool test_gui_framework(void);

/**
 * Run device driver tests
 */
bool test_device_drivers(void);

/**
 * Run integration tests
 */
bool test_integration(void);

/**
 * Run stress tests
 */
bool test_stress_and_stability(void);

/**
 * Run performance benchmarks
 */
void run_performance_benchmarks(void);

/**
 * Get test statistics
 */
test_stats_t get_test_statistics(void);

/**
 * Print test report
 */
void print_test_report(void);

/**
 * Memory leak detection
 */
bool test_memory_leaks(void);

/**
 * Error handling validation
 */
bool test_error_handling(void);

#endif /* AURORA_PHASE5_TESTS_H */
