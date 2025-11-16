/**
 * Aurora OS - RAM-based File Cache Implementation
 * 
 * File caching system for storing infrequently accessed files in RAM
 */

#include "file_cache.h"
#include "../../kernel/memory/memory.h"
#include <stddef.h>

/* Cache state */
static cache_entry_t cache_entries[FILE_CACHE_MAX_ENTRIES];
static cache_entry_t* lru_head = NULL;
static cache_entry_t* lru_tail = NULL;
static file_cache_config_t cache_config;
static file_cache_stats_t cache_stats;
static uint32_t current_time = 0;

/* Helper functions */
static void str_copy(char* dest, const char* src, int max_len);
static int str_equal(const char* a, const char* b);
static void lru_add_to_front(cache_entry_t* entry);
static void lru_remove(cache_entry_t* entry);
static void lru_move_to_front(cache_entry_t* entry);

/**
 * Initialize file cache subsystem
 */
void file_cache_init(void) {
    /* Clear all cache entries */
    for (int i = 0; i < FILE_CACHE_MAX_ENTRIES; i++) {
        cache_entries[i].in_use = 0;
        cache_entries[i].data = NULL;
        cache_entries[i].original_size = 0;
        cache_entries[i].cached_size = 0;
        cache_entries[i].flags = 0;
        cache_entries[i].access_count = 0;
        cache_entries[i].last_access_time = 0;
        cache_entries[i].next = NULL;
        cache_entries[i].prev = NULL;
        for (int j = 0; j < FILE_CACHE_MAX_PATH_LEN; j++) {
            cache_entries[i].path[j] = 0;
        }
    }
    
    /* Initialize LRU list */
    lru_head = NULL;
    lru_tail = NULL;
    
    /* Set default configuration */
    cache_config.max_cache_size = FILE_CACHE_DEFAULT_SIZE;
    cache_config.min_file_size = FILE_CACHE_MIN_FILE_SIZE;
    cache_config.access_threshold = 2;
    cache_config.enable_compression = 1;
    cache_config.enable_cache = 1;
    
    /* Clear statistics */
    cache_stats.hits = 0;
    cache_stats.misses = 0;
    cache_stats.evictions = 0;
    cache_stats.total_cached = 0;
    cache_stats.bytes_used = 0;
    cache_stats.bytes_saved = 0;
    
    current_time = 0;
}

/**
 * Configure cache settings
 */
int file_cache_configure(file_cache_config_t* config) {
    if (!config) {
        return -1;
    }
    
    cache_config.max_cache_size = config->max_cache_size;
    cache_config.min_file_size = config->min_file_size;
    cache_config.access_threshold = config->access_threshold;
    cache_config.enable_compression = config->enable_compression;
    cache_config.enable_cache = config->enable_cache;
    
    return 0;
}

/**
 * Find cache entry by path
 */
static cache_entry_t* find_cache_entry(const char* path) {
    for (int i = 0; i < FILE_CACHE_MAX_ENTRIES; i++) {
        if (cache_entries[i].in_use && str_equal(cache_entries[i].path, path)) {
            return &cache_entries[i];
        }
    }
    return NULL;
}

/**
 * Find free cache entry slot
 */
static cache_entry_t* find_free_entry(void) {
    for (int i = 0; i < FILE_CACHE_MAX_ENTRIES; i++) {
        if (!cache_entries[i].in_use) {
            return &cache_entries[i];
        }
    }
    return NULL;
}

/**
 * Store file in cache
 */
int file_cache_store(const char* path, const void* data, uint32_t size) {
    if (!cache_config.enable_cache || !path || !data || size == 0) {
        return -1;
    }
    
    /* Check if file is large enough to cache */
    if (size < cache_config.min_file_size) {
        return -1;
    }
    
    /* Check if file is already cached */
    cache_entry_t* entry = find_cache_entry(path);
    if (entry) {
        /* Update existing entry */
        lru_move_to_front(entry);
        entry->access_count++;
        entry->last_access_time = current_time++;
        return 0;
    }
    
    /* Find or allocate cache entry */
    entry = find_free_entry();
    if (!entry) {
        /* Cache full - evict LRU entry */
        if (file_cache_evict_lru() != 0) {
            return -1;
        }
        entry = find_free_entry();
        if (!entry) {
            return -1;
        }
    }
    
    /* Allocate memory for cached data */
    uint8_t* cached_data = NULL;
    uint32_t cached_size = size;
    
    if (cache_config.enable_compression) {
        /* Try compression */
        uint8_t* compressed = (uint8_t*)kmalloc(size);
        if (compressed) {
            uint32_t compressed_size = compress_data((const uint8_t*)data, size, 
                                                     compressed, size);
            if (compressed_size > 0 && compressed_size < size) {
                /* Compression successful */
                cached_data = compressed;
                cached_size = compressed_size;
                entry->flags |= CACHE_FLAG_COMPRESSED;
                cache_stats.bytes_saved += (size - compressed_size);
            } else {
                /* Compression not beneficial - use original */
                kfree(compressed);
                cached_data = (uint8_t*)kmalloc(size);
                if (cached_data) {
                    for (uint32_t i = 0; i < size; i++) {
                        cached_data[i] = ((const uint8_t*)data)[i];
                    }
                }
            }
        }
    } else {
        /* No compression - copy data directly */
        cached_data = (uint8_t*)kmalloc(size);
        if (cached_data) {
            for (uint32_t i = 0; i < size; i++) {
                cached_data[i] = ((const uint8_t*)data)[i];
            }
        }
    }
    
    if (!cached_data) {
        return -1;
    }
    
    /* Check if adding this would exceed cache size limit */
    if (cache_stats.bytes_used + cached_size > cache_config.max_cache_size) {
        /* Evict until we have space */
        while (cache_stats.bytes_used + cached_size > cache_config.max_cache_size) {
            if (file_cache_evict_lru() != 0) {
                kfree(cached_data);
                return -1;
            }
        }
    }
    
    /* Initialize cache entry */
    str_copy(entry->path, path, FILE_CACHE_MAX_PATH_LEN);
    entry->data = cached_data;
    entry->original_size = size;
    entry->cached_size = cached_size;
    entry->access_count = 1;
    entry->last_access_time = current_time++;
    entry->in_use = 1;
    
    /* Add to LRU list */
    lru_add_to_front(entry);
    
    /* Update statistics */
    cache_stats.total_cached++;
    cache_stats.bytes_used += cached_size;
    
    return 0;
}

/**
 * Retrieve file from cache
 */
int file_cache_retrieve(const char* path, void* buffer, uint32_t* size) {
    if (!cache_config.enable_cache || !path || !buffer || !size) {
        return -1;
    }
    
    cache_entry_t* entry = find_cache_entry(path);
    if (!entry) {
        cache_stats.misses++;
        return -1;
    }
    
    /* Update access information */
    entry->access_count++;
    entry->last_access_time = current_time++;
    lru_move_to_front(entry);
    cache_stats.hits++;
    
    /* Decompress if needed */
    if (entry->flags & CACHE_FLAG_COMPRESSED) {
        uint32_t decompressed_size = decompress_data(entry->data, entry->cached_size,
                                                     (uint8_t*)buffer, *size);
        if (decompressed_size == 0 || decompressed_size != entry->original_size) {
            return -1;
        }
        *size = decompressed_size;
    } else {
        /* Copy data directly */
        if (*size < entry->original_size) {
            return -1;
        }
        for (uint32_t i = 0; i < entry->original_size; i++) {
            ((uint8_t*)buffer)[i] = entry->data[i];
        }
        *size = entry->original_size;
    }
    
    return 0;
}

/**
 * Remove file from cache
 */
int file_cache_remove(const char* path) {
    cache_entry_t* entry = find_cache_entry(path);
    if (!entry) {
        return -1;
    }
    
    /* Free cached data */
    if (entry->data) {
        kfree(entry->data);
        entry->data = NULL;
    }
    
    /* Update statistics */
    cache_stats.bytes_used -= entry->cached_size;
    if (entry->flags & CACHE_FLAG_COMPRESSED) {
        cache_stats.bytes_saved -= (entry->original_size - entry->cached_size);
    }
    
    /* Remove from LRU list */
    lru_remove(entry);
    
    /* Clear entry */
    entry->in_use = 0;
    entry->original_size = 0;
    entry->cached_size = 0;
    entry->flags = 0;
    
    return 0;
}

/**
 * Evict least recently used entry
 */
int file_cache_evict_lru(void) {
    if (!lru_tail || lru_tail->flags & CACHE_FLAG_LOCKED) {
        return -1;
    }
    
    cache_stats.evictions++;
    return file_cache_remove(lru_tail->path);
}

/**
 * Clear entire cache
 */
void file_cache_clear(void) {
    for (int i = 0; i < FILE_CACHE_MAX_ENTRIES; i++) {
        if (cache_entries[i].in_use) {
            file_cache_remove(cache_entries[i].path);
        }
    }
}

/**
 * Check if file exists in cache
 */
int file_cache_exists(const char* path) {
    return find_cache_entry(path) != NULL ? 1 : 0;
}

/**
 * Get cached file size
 */
uint32_t file_cache_get_size(const char* path) {
    cache_entry_t* entry = find_cache_entry(path);
    return entry ? entry->original_size : 0;
}

/**
 * Determine if file should be cached
 */
int file_cache_should_cache(const char* path, uint32_t size) {
    if (!cache_config.enable_cache) {
        return 0;
    }
    
    if (size < cache_config.min_file_size) {
        return 0;
    }
    
    cache_entry_t* entry = find_cache_entry(path);
    if (entry) {
        return entry->access_count >= cache_config.access_threshold ? 1 : 0;
    }
    
    return 0;
}

/**
 * Get cache statistics
 */
void file_cache_get_stats(file_cache_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    stats->hits = cache_stats.hits;
    stats->misses = cache_stats.misses;
    stats->evictions = cache_stats.evictions;
    stats->total_cached = cache_stats.total_cached;
    stats->bytes_used = cache_stats.bytes_used;
    stats->bytes_saved = cache_stats.bytes_saved;
}

/**
 * Reset cache statistics
 */
void file_cache_reset_stats(void) {
    cache_stats.hits = 0;
    cache_stats.misses = 0;
    cache_stats.evictions = 0;
    cache_stats.total_cached = 0;
}

/**
 * Simple RLE compression
 */
uint32_t compress_data(const uint8_t* input, uint32_t input_size, 
                       uint8_t* output, uint32_t output_max) {
    if (!input || !output || input_size == 0) {
        return 0;
    }
    
    uint32_t out_pos = 0;
    uint32_t in_pos = 0;
    
    while (in_pos < input_size && out_pos + 2 < output_max) {
        uint8_t value = input[in_pos];
        uint32_t count = 1;
        
        /* Count consecutive identical bytes (max 255) */
        while (in_pos + count < input_size && 
               input[in_pos + count] == value && 
               count < 255) {
            count++;
        }
        
        /* Store count and value */
        output[out_pos++] = (uint8_t)count;
        output[out_pos++] = value;
        in_pos += count;
    }
    
    /* Check if compression was beneficial */
    if (out_pos >= input_size) {
        return 0;
    }
    
    return out_pos;
}

/**
 * Simple RLE decompression
 */
uint32_t decompress_data(const uint8_t* input, uint32_t input_size,
                         uint8_t* output, uint32_t output_max) {
    if (!input || !output || input_size == 0) {
        return 0;
    }
    
    uint32_t out_pos = 0;
    uint32_t in_pos = 0;
    
    while (in_pos + 1 < input_size && out_pos < output_max) {
        uint32_t count = input[in_pos++];
        uint8_t value = input[in_pos++];
        
        for (uint32_t i = 0; i < count && out_pos < output_max; i++) {
            output[out_pos++] = value;
        }
    }
    
    return out_pos;
}

/* Helper function implementations */

/**
 * String copy helper
 */
static void str_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/**
 * String comparison helper
 */
static int str_equal(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/**
 * Add entry to front of LRU list
 */
static void lru_add_to_front(cache_entry_t* entry) {
    if (!entry) {
        return;
    }
    
    entry->next = lru_head;
    entry->prev = NULL;
    
    if (lru_head) {
        lru_head->prev = entry;
    }
    
    lru_head = entry;
    
    if (!lru_tail) {
        lru_tail = entry;
    }
}

/**
 * Remove entry from LRU list
 */
static void lru_remove(cache_entry_t* entry) {
    if (!entry) {
        return;
    }
    
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        lru_head = entry->next;
    }
    
    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        lru_tail = entry->prev;
    }
    
    entry->next = NULL;
    entry->prev = NULL;
}

/**
 * Move entry to front of LRU list
 */
static void lru_move_to_front(cache_entry_t* entry) {
    if (!entry || entry == lru_head) {
        return;
    }
    
    lru_remove(entry);
    lru_add_to_front(entry);
}
