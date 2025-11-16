/**
 * Aurora OS - File Cache Tests Header
 * 
 * Test suite for RAM-based file caching system
 */

#ifndef FILE_CACHE_TESTS_H
#define FILE_CACHE_TESTS_H

#include <stdint.h>

/* Test functions */
void test_cache_init(void);
void test_cache_store_retrieve(void);
void test_cache_remove(void);
void test_cache_lru_eviction(void);
void test_cache_compression(void);
void test_cache_statistics(void);
void test_cache_clear(void);
void test_cache_size_filter(void);

/* Run all tests */
void run_file_cache_tests(void);
void get_cache_test_results(int* passed, int* failed);

#endif /* FILE_CACHE_TESTS_H */
