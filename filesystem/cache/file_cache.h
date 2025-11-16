/**
 * Aurora OS - RAM-based File Cache Header
 * 
 * File caching system for storing infrequently accessed files in RAM
 * to save disk storage space with optional compression.
 */

#ifndef AURORA_FILE_CACHE_H
#define AURORA_FILE_CACHE_H

#include <stdint.h>
#include <stddef.h>

/* Cache configuration */
#define FILE_CACHE_MAX_ENTRIES 64
#define FILE_CACHE_MAX_PATH_LEN 256
#define FILE_CACHE_DEFAULT_SIZE (1024 * 1024)  /* 1MB default cache size */
#define FILE_CACHE_MIN_FILE_SIZE 512            /* Minimum file size to cache */

/* Cache flags */
#define CACHE_FLAG_COMPRESSED   0x01
#define CACHE_FLAG_DIRTY        0x02
#define CACHE_FLAG_LOCKED       0x04

/* Cache statistics */
typedef struct file_cache_stats {
    uint32_t hits;
    uint32_t misses;
    uint32_t evictions;
    uint32_t total_cached;
    uint32_t bytes_used;
    uint32_t bytes_saved;  /* Space saved through compression */
} file_cache_stats_t;

/* Cache entry structure */
typedef struct cache_entry {
    char path[FILE_CACHE_MAX_PATH_LEN];
    uint8_t* data;
    uint32_t original_size;
    uint32_t cached_size;
    uint32_t flags;
    uint32_t access_count;
    uint32_t last_access_time;
    struct cache_entry* next;
    struct cache_entry* prev;
    uint8_t in_use;
} cache_entry_t;

/* Cache configuration */
typedef struct file_cache_config {
    uint32_t max_cache_size;
    uint32_t min_file_size;
    uint32_t access_threshold;  /* Minimum accesses before caching */
    uint8_t enable_compression;
    uint8_t enable_cache;
} file_cache_config_t;

/* Cache initialization */
void file_cache_init(void);
int file_cache_configure(file_cache_config_t* config);

/* Cache operations */
int file_cache_store(const char* path, const void* data, uint32_t size);
int file_cache_retrieve(const char* path, void* buffer, uint32_t* size);
int file_cache_remove(const char* path);
int file_cache_evict_lru(void);
void file_cache_clear(void);

/* Cache queries */
int file_cache_exists(const char* path);
uint32_t file_cache_get_size(const char* path);
int file_cache_should_cache(const char* path, uint32_t size);

/* Statistics */
void file_cache_get_stats(file_cache_stats_t* stats);
void file_cache_reset_stats(void);

/* Compression helpers (simple RLE for now) */
uint32_t compress_data(const uint8_t* input, uint32_t input_size, 
                       uint8_t* output, uint32_t output_max);
uint32_t decompress_data(const uint8_t* input, uint32_t input_size,
                         uint8_t* output, uint32_t output_max);

#endif /* AURORA_FILE_CACHE_H */
