/**
 * @file platform_util_tests.h
 * @brief Platform utility functions test suite header
 */

#ifndef PLATFORM_UTIL_TESTS_H
#define PLATFORM_UTIL_TESTS_H

/**
 * Run all platform utility tests
 */
void run_platform_util_tests(void);

/**
 * Test platform_memset function
 */
void test_platform_memset(void);

/**
 * Test platform_memcpy function
 */
void test_platform_memcpy(void);

/**
 * Test platform_memcmp function
 */
void test_platform_memcmp(void);

/**
 * Test platform_strlen function
 */
void test_platform_strlen(void);

/**
 * Test platform_strcmp function
 */
void test_platform_strcmp(void);

/**
 * Test platform_strncpy function
 */
void test_platform_strncpy(void);

/**
 * Test platform_malloc and platform_free
 */
void test_platform_malloc_free(void);

#endif /* PLATFORM_UTIL_TESTS_H */
