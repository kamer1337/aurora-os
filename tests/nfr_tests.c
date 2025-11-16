/**
 * Aurora OS - Non-Functional Requirements Module Tests
 * 
 * Test suite for NFR monitoring and metrics
 */

#include "../kernel/core/nfr.h"
#include <stdint.h>
#include <stdbool.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Simple test assertion */
static void test_assert(bool condition, const char* test_name) {
    if (condition) {
        tests_passed++;
    } else {
        tests_failed++;
    }
}

/**
 * Test NFR initialization
 */
static bool test_nfr_init(void) {
    nfr_init();
    
    nfr_metrics_t* metrics = nfr_get_metrics();
    test_assert(metrics != 0, "NFR metrics should not be null");
    test_assert(metrics->performance.total_operations == 0, "Initial operations should be 0");
    test_assert(metrics->reliability.reliability_score == 100.0f, "Initial reliability should be 100");
    test_assert(metrics->usability.usability_score == 100.0f, "Initial usability should be 100");
    
    return true;
}

/**
 * Test performance metrics update
 */
static bool test_performance_metrics(void) {
    nfr_reset_metrics();
    
    /* Simulate some operations */
    nfr_update_performance(100);  /* 100 us latency */
    nfr_update_performance(200);  /* 200 us latency */
    nfr_update_performance(150);  /* 150 us latency */
    
    nfr_metrics_t* metrics = nfr_get_metrics();
    test_assert(metrics->performance.total_operations == 3, "Should have 3 operations");
    test_assert(metrics->performance.min_latency_us == 100, "Min latency should be 100");
    test_assert(metrics->performance.max_latency_us == 200, "Max latency should be 200");
    test_assert(metrics->performance.avg_latency_us >= 100 && 
                metrics->performance.avg_latency_us <= 200, "Avg latency should be between 100-200");
    
    return true;
}

/**
 * Test scalability metrics update
 */
static bool test_scalability_metrics(void) {
    nfr_update_scalability();
    
    nfr_metrics_t* metrics = nfr_get_metrics();
    test_assert(metrics->scalability.total_memory_kb > 0, "Total memory should be > 0");
    test_assert(metrics->scalability.used_memory_kb > 0, "Used memory should be > 0");
    test_assert(metrics->scalability.active_processes > 0, "Should have active processes");
    test_assert(metrics->scalability.memory_usage_percent <= 100, "Memory usage should be <= 100%");
    
    return true;
}

/**
 * Test reliability metrics
 */
static bool test_reliability_metrics(void) {
    nfr_reset_metrics();
    
    /* Record some errors */
    nfr_record_error(false, true);  /* Non-critical, recovered */
    nfr_record_error(false, true);  /* Non-critical, recovered */
    nfr_record_error(true, false);  /* Critical, not recovered */
    
    nfr_metrics_t* metrics = nfr_get_metrics();
    test_assert(metrics->reliability.total_errors == 3, "Should have 3 errors");
    test_assert(metrics->reliability.critical_errors == 1, "Should have 1 critical error");
    test_assert(metrics->reliability.recoverable_errors == 2, "Should have 2 recoverable errors");
    test_assert(metrics->reliability.successful_recoveries == 2, "Should have 2 successful recoveries");
    test_assert(metrics->reliability.reliability_score < 100.0f, "Reliability should decrease with errors");
    
    return true;
}

/**
 * Test usability metrics
 */
static bool test_usability_metrics(void) {
    nfr_reset_metrics();
    
    /* Record some interactions */
    nfr_record_interaction(50, true);   /* 50ms, successful */
    nfr_record_interaction(100, true);  /* 100ms, successful */
    nfr_record_interaction(200, false); /* 200ms, failed */
    
    nfr_metrics_t* metrics = nfr_get_metrics();
    test_assert(metrics->usability.user_interactions == 3, "Should have 3 interactions");
    test_assert(metrics->usability.successful_interactions == 2, "Should have 2 successful interactions");
    test_assert(metrics->usability.failed_interactions == 1, "Should have 1 failed interaction");
    test_assert(metrics->usability.avg_response_time_ms >= 50 && 
                metrics->usability.avg_response_time_ms <= 200, "Avg response time should be 50-200ms");
    
    return true;
}

/**
 * Test score calculations
 */
static bool test_score_calculations(void) {
    nfr_reset_metrics();
    
    /* Populate with some data */
    nfr_update_performance(100);
    nfr_update_scalability();
    nfr_record_error(false, true);
    nfr_record_interaction(50, true);
    
    uint8_t perf_score = nfr_get_performance_score();
    uint8_t scale_score = nfr_get_scalability_score();
    uint8_t rel_score = nfr_get_reliability_score();
    uint8_t use_score = nfr_get_usability_score();
    uint8_t overall_score = nfr_get_overall_score();
    
    test_assert(perf_score <= 100, "Performance score should be <= 100");
    test_assert(scale_score <= 100, "Scalability score should be <= 100");
    test_assert(rel_score <= 100, "Reliability score should be <= 100");
    test_assert(use_score <= 100, "Usability score should be <= 100");
    test_assert(overall_score <= 100, "Overall score should be <= 100");
    
    /* Overall score should be average of all scores */
    uint8_t expected_avg = (perf_score + scale_score + rel_score + use_score) / 4;
    test_assert(overall_score == expected_avg, "Overall score should be average of component scores");
    
    return true;
}

/**
 * Test update all functionality
 */
static bool test_update_all(void) {
    nfr_reset_metrics();
    
    uint32_t initial_uptime = nfr_get_metrics()->reliability.uptime_seconds;
    
    /* Wait a bit and update */
    for (volatile int i = 0; i < 100000; i++);
    nfr_update_all();
    
    nfr_metrics_t* metrics = nfr_get_metrics();
    test_assert(metrics->reliability.uptime_seconds >= initial_uptime, "Uptime should increase or stay same");
    test_assert(metrics->performance.context_switches > 0, "Context switches should increment");
    
    return true;
}

/**
 * Run all NFR tests
 */
void run_nfr_tests(void) {
    tests_passed = 0;
    tests_failed = 0;
    
    /* Run test suite */
    test_nfr_init();
    test_performance_metrics();
    test_scalability_metrics();
    test_reliability_metrics();
    test_usability_metrics();
    test_score_calculations();
    test_update_all();
}

/**
 * Get test results
 */
void get_nfr_test_results(int* passed, int* failed) {
    if (passed) *passed = tests_passed;
    if (failed) *failed = tests_failed;
}
