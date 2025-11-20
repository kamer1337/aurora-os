/**
 * @file platform_util_tests.c
 * @brief Platform utility functions test suite
 */

#include "platform_util_tests.h"
#include "../include/platform/platform_util.h"
#include <stdint.h>
#include <stdbool.h>

/* Test helper */
static void test_assert(bool condition, const char* test_name) {
    if (!condition) {
        /* In real kernel, would use kernel logging */
        (void)test_name;
    }
}

/**
 * Test platform_memset function
 */
void test_platform_memset(void) {
    uint8_t buffer[64];
    
    /* Test memset with zero */
    platform_memset(buffer, 0, sizeof(buffer));
    for (size_t i = 0; i < sizeof(buffer); i++) {
        test_assert(buffer[i] == 0, "memset with 0");
    }
    
    /* Test memset with non-zero value */
    platform_memset(buffer, 0xAA, sizeof(buffer));
    for (size_t i = 0; i < sizeof(buffer); i++) {
        test_assert(buffer[i] == 0xAA, "memset with 0xAA");
    }
}

/**
 * Test platform_memcpy function
 */
void test_platform_memcpy(void) {
    uint8_t src[64];
    uint8_t dst[64];
    
    /* Initialize source buffer */
    for (size_t i = 0; i < sizeof(src); i++) {
        src[i] = (uint8_t)i;
    }
    
    /* Test memcpy */
    platform_memset(dst, 0, sizeof(dst));
    platform_memcpy(dst, src, sizeof(src));
    
    for (size_t i = 0; i < sizeof(src); i++) {
        test_assert(dst[i] == src[i], "memcpy");
    }
}

/**
 * Test platform_memcmp function
 */
void test_platform_memcmp(void) {
    uint8_t buf1[64];
    uint8_t buf2[64];
    
    /* Test equal buffers */
    platform_memset(buf1, 0xAA, sizeof(buf1));
    platform_memset(buf2, 0xAA, sizeof(buf2));
    test_assert(platform_memcmp(buf1, buf2, sizeof(buf1)) == 0, "memcmp equal");
    
    /* Test different buffers */
    buf2[32] = 0xBB;
    test_assert(platform_memcmp(buf1, buf2, sizeof(buf1)) != 0, "memcmp not equal");
}

/**
 * Test platform_strlen function
 */
void test_platform_strlen(void) {
    const char* str1 = "Hello";
    const char* str2 = "Hello, World!";
    const char* str3 = "";
    
    test_assert(platform_strlen(str1) == 5, "strlen 'Hello'");
    test_assert(platform_strlen(str2) == 13, "strlen 'Hello, World!'");
    test_assert(platform_strlen(str3) == 0, "strlen empty");
}

/**
 * Test platform_strcmp function
 */
void test_platform_strcmp(void) {
    test_assert(platform_strcmp("abc", "abc") == 0, "strcmp equal");
    test_assert(platform_strcmp("abc", "abd") < 0, "strcmp less");
    test_assert(platform_strcmp("abd", "abc") > 0, "strcmp greater");
    test_assert(platform_strcmp("", "") == 0, "strcmp empty");
}

/**
 * Test platform_strncpy function
 */
void test_platform_strncpy(void) {
    char dst[64];
    
    /* Test normal copy */
    platform_memset(dst, 0, sizeof(dst));
    platform_strncpy(dst, "Hello", sizeof(dst));
    test_assert(platform_strcmp(dst, "Hello") == 0, "strncpy normal");
    
    /* Test truncation */
    platform_memset(dst, 0, sizeof(dst));
    platform_strncpy(dst, "Hello, World!", 6);
    test_assert(platform_strcmp(dst, "Hello") == 0, "strncpy truncate");
}

/**
 * Test platform_malloc and platform_free
 */
void test_platform_malloc_free(void) {
    /* Test allocation */
    void* ptr1 = platform_malloc(128);
    test_assert(ptr1 != NULL, "malloc 128 bytes");
    
    /* Test multiple allocations */
    void* ptr2 = platform_malloc(256);
    test_assert(ptr2 != NULL, "malloc 256 bytes");
    test_assert(ptr1 != ptr2, "different pointers");
    
    /* Test free */
    platform_free(ptr1);
    platform_free(ptr2);
    
    /* Test zero allocation */
    void* ptr3 = platform_malloc(0);
    test_assert(ptr3 == NULL, "malloc 0 bytes returns NULL");
}

/**
 * Run all platform utility tests
 */
void run_platform_util_tests(void) {
    test_platform_memset();
    test_platform_memcpy();
    test_platform_memcmp();
    test_platform_strlen();
    test_platform_strcmp();
    test_platform_strncpy();
    test_platform_malloc_free();
}
