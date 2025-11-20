/**
 * @file platform_util.h
 * @brief Platform utility functions - standard library dependencies
 * 
 * Provides standard C library functions for freestanding environment
 * using kernel memory allocator and optimized implementations
 */

#ifndef PLATFORM_UTIL_H
#define PLATFORM_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include "../../kernel/memory/memory.h"

/**
 * Memory allocation using kernel allocator
 */
static inline void* platform_malloc(size_t size) {
    return kmalloc(size);
}

/**
 * Memory deallocation using kernel allocator
 */
static inline void platform_free(void* ptr) {
    kfree(ptr);
}

/**
 * Set memory to a value
 */
static inline void platform_memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
}

/**
 * Copy memory from source to destination
 */
static inline void platform_memcpy(void* dest, const void* src, size_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

/**
 * Compare memory regions
 */
static inline int platform_memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

/**
 * Copy string with maximum length
 */
static inline void platform_strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    if (i < n) {
        dest[i] = '\0';
    }
}

/**
 * Compare two strings
 */
static inline int platform_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint8_t*)s1 - *(const uint8_t*)s2;
}

/**
 * Get string length
 */
static inline size_t platform_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

#endif /* PLATFORM_UTIL_H */
