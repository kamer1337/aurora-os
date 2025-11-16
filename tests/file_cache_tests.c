/**
 * Aurora OS - File Cache Tests
 * 
 * Test suite for RAM-based file caching system
 */

#include "file_cache_tests.h"
#include "../filesystem/cache/file_cache.h"
#include <stddef.h>

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper function for test assertions */
static void assert_true(int condition, const char* test_name) {
    if (condition) {
        tests_passed++;
    } else {
        tests_failed++;
    }
}

/**
 * Test cache initialization
 */
void test_cache_init(void) {
    file_cache_init();
    
    file_cache_stats_t stats;
    file_cache_get_stats(&stats);
    
    assert_true(stats.hits == 0, "cache_init: hits should be 0");
    assert_true(stats.misses == 0, "cache_init: misses should be 0");
    assert_true(stats.total_cached == 0, "cache_init: total_cached should be 0");
    assert_true(stats.bytes_used == 0, "cache_init: bytes_used should be 0");
}

/**
 * Test basic cache store and retrieve
 */
void test_cache_store_retrieve(void) {
    file_cache_init();
    
    const char* path = "/test_file.txt";
    uint8_t data[1024];
    for (int i = 0; i < 1024; i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Store file in cache */
    int result = file_cache_store(path, data, 1024);
    assert_true(result == 0, "cache_store: should succeed");
    
    /* Check if file exists in cache */
    assert_true(file_cache_exists(path) == 1, "cache_exists: file should be cached");
    
    /* Retrieve file from cache */
    uint8_t buffer[1024];
    uint32_t size = 1024;
    result = file_cache_retrieve(path, buffer, &size);
    assert_true(result == 0, "cache_retrieve: should succeed");
    assert_true(size == 1024, "cache_retrieve: size should match");
    
    /* Verify data integrity */
    int data_matches = 1;
    for (int i = 0; i < 1024; i++) {
        if (buffer[i] != (uint8_t)(i & 0xFF)) {
            data_matches = 0;
            break;
        }
    }
    assert_true(data_matches == 1, "cache_retrieve: data should match");
}

/**
 * Test cache removal
 */
void test_cache_remove(void) {
    file_cache_init();
    
    const char* path = "/test_remove.txt";
    uint8_t data[512];
    for (int i = 0; i < 512; i++) {
        data[i] = (uint8_t)i;
    }
    
    /* Store and verify */
    file_cache_store(path, data, 512);
    assert_true(file_cache_exists(path) == 1, "cache_remove: file should exist");
    
    /* Remove file */
    int result = file_cache_remove(path);
    assert_true(result == 0, "cache_remove: should succeed");
    assert_true(file_cache_exists(path) == 0, "cache_remove: file should not exist");
}

/**
 * Test LRU eviction
 */
void test_cache_lru_eviction(void) {
    file_cache_init();
    
    /* Configure small cache for testing */
    file_cache_config_t config;
    config.max_cache_size = 4096;  /* 4KB limit */
    config.min_file_size = 512;
    config.access_threshold = 1;
    config.enable_compression = 0;
    config.enable_cache = 1;
    file_cache_configure(&config);
    
    /* Store multiple files */
    uint8_t data[1024];
    for (int i = 0; i < 1024; i++) {
        data[i] = (uint8_t)i;
    }
    
    file_cache_store("/file1.txt", data, 1024);
    file_cache_store("/file2.txt", data, 1024);
    file_cache_store("/file3.txt", data, 1024);
    
    /* Access file1 to move it to front */
    uint8_t buffer[1024];
    uint32_t size = 1024;
    file_cache_retrieve("/file1.txt", buffer, &size);
    
    /* Store another file - should evict file2 (LRU) */
    file_cache_store("/file4.txt", data, 1024);
    
    /* Verify eviction happened */
    file_cache_stats_t stats;
    file_cache_get_stats(&stats);
    assert_true(stats.evictions > 0, "cache_lru: should have evictions");
}

/**
 * Test compression
 */
void test_cache_compression(void) {
    file_cache_init();
    
    /* Configure cache with compression */
    file_cache_config_t config;
    config.max_cache_size = 1024 * 1024;
    config.min_file_size = 512;
    config.access_threshold = 1;
    config.enable_compression = 1;
    config.enable_cache = 1;
    file_cache_configure(&config);
    
    /* Create highly compressible data (repeated pattern) */
    uint8_t data[2048];
    for (int i = 0; i < 2048; i++) {
        data[i] = 0xAA;  /* Repeated byte */
    }
    
    /* Store file */
    int result = file_cache_store("/compressed.txt", data, 2048);
    assert_true(result == 0, "cache_compression: store should succeed");
    
    /* Retrieve and verify */
    uint8_t buffer[2048];
    uint32_t size = 2048;
    result = file_cache_retrieve("/compressed.txt", buffer, &size);
    assert_true(result == 0, "cache_compression: retrieve should succeed");
    assert_true(size == 2048, "cache_compression: size should match");
    
    /* Verify data */
    int data_matches = 1;
    for (int i = 0; i < 2048; i++) {
        if (buffer[i] != 0xAA) {
            data_matches = 0;
            break;
        }
    }
    assert_true(data_matches == 1, "cache_compression: data should match");
    
    /* Check if compression saved space */
    file_cache_stats_t stats;
    file_cache_get_stats(&stats);
    assert_true(stats.bytes_saved > 0, "cache_compression: should save space");
}

/**
 * Test cache statistics
 */
void test_cache_statistics(void) {
    file_cache_init();
    
    uint8_t data[1024];
    for (int i = 0; i < 1024; i++) {
        data[i] = (uint8_t)i;
    }
    
    /* Store files */
    file_cache_store("/stats1.txt", data, 1024);
    file_cache_store("/stats2.txt", data, 1024);
    
    /* Hit: retrieve existing file */
    uint8_t buffer[1024];
    uint32_t size = 1024;
    file_cache_retrieve("/stats1.txt", buffer, &size);
    
    /* Miss: try to retrieve non-existent file */
    file_cache_retrieve("/nonexistent.txt", buffer, &size);
    
    /* Get statistics */
    file_cache_stats_t stats;
    file_cache_get_stats(&stats);
    
    assert_true(stats.hits >= 1, "cache_stats: should have hits");
    assert_true(stats.misses >= 1, "cache_stats: should have misses");
    assert_true(stats.total_cached >= 2, "cache_stats: should have cached files");
}

/**
 * Test cache clear
 */
void test_cache_clear(void) {
    file_cache_init();
    
    uint8_t data[512];
    for (int i = 0; i < 512; i++) {
        data[i] = (uint8_t)i;
    }
    
    /* Store multiple files */
    file_cache_store("/clear1.txt", data, 512);
    file_cache_store("/clear2.txt", data, 512);
    file_cache_store("/clear3.txt", data, 512);
    
    /* Clear cache */
    file_cache_clear();
    
    /* Verify all files are removed */
    assert_true(file_cache_exists("/clear1.txt") == 0, "cache_clear: file1 should not exist");
    assert_true(file_cache_exists("/clear2.txt") == 0, "cache_clear: file2 should not exist");
    assert_true(file_cache_exists("/clear3.txt") == 0, "cache_clear: file3 should not exist");
    
    file_cache_stats_t stats;
    file_cache_get_stats(&stats);
    assert_true(stats.bytes_used == 0, "cache_clear: bytes_used should be 0");
}

/**
 * Test small file filtering
 */
void test_cache_size_filter(void) {
    file_cache_init();
    
    /* Configure cache with minimum file size */
    file_cache_config_t config;
    config.max_cache_size = 1024 * 1024;
    config.min_file_size = 1024;  /* 1KB minimum */
    config.access_threshold = 1;
    config.enable_compression = 0;
    config.enable_cache = 1;
    file_cache_configure(&config);
    
    /* Try to cache small file (should fail) */
    uint8_t small_data[512];
    for (int i = 0; i < 512; i++) {
        small_data[i] = (uint8_t)i;
    }
    
    int result = file_cache_store("/small.txt", small_data, 512);
    assert_true(result != 0, "cache_size_filter: small file should not be cached");
    
    /* Cache larger file (should succeed) */
    uint8_t large_data[2048];
    for (int i = 0; i < 2048; i++) {
        large_data[i] = (uint8_t)i;
    }
    
    result = file_cache_store("/large.txt", large_data, 2048);
    assert_true(result == 0, "cache_size_filter: large file should be cached");
}

/**
 * Run all file cache tests
 */
void run_file_cache_tests(void) {
    tests_passed = 0;
    tests_failed = 0;
    
    test_cache_init();
    test_cache_store_retrieve();
    test_cache_remove();
    test_cache_lru_eviction();
    test_cache_compression();
    test_cache_statistics();
    test_cache_clear();
    test_cache_size_filter();
}

/**
 * Get test results
 */
void get_cache_test_results(int* passed, int* failed) {
    if (passed) *passed = tests_passed;
    if (failed) *failed = tests_failed;
}
