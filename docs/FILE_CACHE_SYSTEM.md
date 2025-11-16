# Aurora OS - File Cache System Documentation

## Overview

The File Cache System is a RAM-based caching solution designed to store infrequently accessed files in memory, helping to save disk storage space and improve access times for cached files. The system features intelligent LRU (Least Recently Used) eviction, optional data compression, and configurable caching policies.

## Features

### Core Capabilities

- **RAM-based Storage**: Files are stored entirely in memory for fast access
- **LRU Eviction**: Automatically removes least recently used files when cache is full
- **Data Compression**: Optional RLE (Run-Length Encoding) compression to save memory
- **Configurable Policies**: Customizable cache size, minimum file size, and access thresholds
- **Statistics Tracking**: Comprehensive statistics for hits, misses, evictions, and space savings

### Performance Benefits

- **Fast Access**: Cached files are retrieved from RAM instead of disk
- **Space Efficiency**: Compression can reduce memory usage significantly for certain file types
- **Intelligent Caching**: Only caches files that meet size and access criteria
- **Automatic Management**: LRU eviction ensures most relevant files stay cached

## Architecture

### Cache Entry Structure

Each cached file is represented by a cache entry containing:

```c
typedef struct cache_entry {
    char path[FILE_CACHE_MAX_PATH_LEN];  // File path
    uint8_t* data;                       // Cached data
    uint32_t original_size;              // Original file size
    uint32_t cached_size;                // Compressed size (if compressed)
    uint32_t flags;                      // Cache flags (compressed, dirty, locked)
    uint32_t access_count;               // Number of accesses
    uint32_t last_access_time;           // Last access timestamp
    struct cache_entry* next;            // LRU list next
    struct cache_entry* prev;            // LRU list prev
    uint8_t in_use;                      // Entry in use flag
} cache_entry_t;
```

### Configuration

The cache system can be configured with:

```c
typedef struct file_cache_config {
    uint32_t max_cache_size;        // Maximum cache size in bytes (default: 1MB)
    uint32_t min_file_size;         // Minimum file size to cache (default: 512 bytes)
    uint32_t access_threshold;      // Minimum accesses before caching (default: 2)
    uint8_t enable_compression;     // Enable compression (default: 1)
    uint8_t enable_cache;           // Enable cache (default: 1)
} file_cache_config_t;
```

## API Reference

### Initialization

```c
void file_cache_init(void);
```
Initialize the file cache subsystem with default settings.

```c
int file_cache_configure(file_cache_config_t* config);
```
Configure cache with custom settings. Returns 0 on success, -1 on error.

### Cache Operations

```c
int file_cache_store(const char* path, const void* data, uint32_t size);
```
Store a file in the cache. Returns 0 on success, -1 on error.

```c
int file_cache_retrieve(const char* path, void* buffer, uint32_t* size);
```
Retrieve a file from the cache. Returns 0 on success, -1 on error. Updates `size` with actual size retrieved.

```c
int file_cache_remove(const char* path);
```
Remove a file from the cache. Returns 0 on success, -1 on error.

```c
int file_cache_evict_lru(void);
```
Evict the least recently used file from cache. Returns 0 on success, -1 on error.

```c
void file_cache_clear(void);
```
Clear all entries from the cache.

### Cache Queries

```c
int file_cache_exists(const char* path);
```
Check if a file is cached. Returns 1 if cached, 0 otherwise.

```c
uint32_t file_cache_get_size(const char* path);
```
Get the original size of a cached file. Returns 0 if not found.

```c
int file_cache_should_cache(const char* path, uint32_t size);
```
Determine if a file should be cached based on current policy. Returns 1 if should cache, 0 otherwise.

### Statistics

```c
void file_cache_get_stats(file_cache_stats_t* stats);
```
Get current cache statistics including hits, misses, evictions, bytes used, and bytes saved.

```c
void file_cache_reset_stats(void);
```
Reset cache statistics counters.

## VFS Integration

The cache system is integrated with the Virtual File System (VFS) layer:

```c
int vfs_cache_file(const char* path);
```
Cache a file through VFS. Automatically reads file content and stores in cache.

```c
int vfs_uncache_file(const char* path);
```
Remove a file from cache through VFS.

```c
int vfs_is_cached(const char* path);
```
Check if a file is cached through VFS.

## Usage Examples

### Basic Usage

```c
// Initialize cache
file_cache_init();

// Store a file
uint8_t data[1024] = { /* ... */ };
file_cache_store("/path/to/file.txt", data, 1024);

// Retrieve a file
uint8_t buffer[1024];
uint32_t size = 1024;
if (file_cache_retrieve("/path/to/file.txt", buffer, &size) == 0) {
    // File retrieved successfully
    // size now contains the actual size
}

// Remove from cache
file_cache_remove("/path/to/file.txt");
```

### Custom Configuration

```c
// Configure cache with custom settings
file_cache_config_t config;
config.max_cache_size = 2 * 1024 * 1024;  // 2MB cache
config.min_file_size = 1024;               // Cache files >= 1KB
config.access_threshold = 3;               // Cache after 3 accesses
config.enable_compression = 1;             // Enable compression
config.enable_cache = 1;                   // Cache enabled

file_cache_configure(&config);
```

### Using VFS Integration

```c
// Initialize VFS (automatically initializes cache)
vfs_init();

// Cache a file through VFS
if (vfs_cache_file("/important/file.dat") == 0) {
    // File cached successfully
}

// Check if file is cached
if (vfs_is_cached("/important/file.dat")) {
    // File is in cache
}

// Remove from cache
vfs_uncache_file("/important/file.dat");
```

### Monitoring Statistics

```c
// Get cache statistics
file_cache_stats_t stats;
file_cache_get_stats(&stats);

// Display statistics
printf("Cache Hits: %u\n", stats.hits);
printf("Cache Misses: %u\n", stats.misses);
printf("Evictions: %u\n", stats.evictions);
printf("Files Cached: %u\n", stats.total_cached);
printf("Bytes Used: %u\n", stats.bytes_used);
printf("Bytes Saved (compression): %u\n", stats.bytes_saved);
```

## Compression Algorithm

The cache uses a simple Run-Length Encoding (RLE) compression algorithm:

- **Compression**: Consecutive identical bytes are stored as a count-value pair
- **Efficiency**: Works best for files with repeated patterns (e.g., large zero blocks)
- **Format**: Each run is stored as: [count (1 byte)][value (1 byte)]
- **Fallback**: If compression doesn't reduce size, original data is stored uncompressed

### Compression Functions

```c
uint32_t compress_data(const uint8_t* input, uint32_t input_size, 
                       uint8_t* output, uint32_t output_max);
```
Compress data using RLE. Returns compressed size, or 0 if compression failed.

```c
uint32_t decompress_data(const uint8_t* input, uint32_t input_size,
                         uint8_t* output, uint32_t output_max);
```
Decompress RLE-compressed data. Returns decompressed size, or 0 if decompression failed.

## Configuration Options

### Default Settings

```c
#define FILE_CACHE_MAX_ENTRIES 64
#define FILE_CACHE_DEFAULT_SIZE (1024 * 1024)  // 1MB
#define FILE_CACHE_MIN_FILE_SIZE 512            // 512 bytes
```

### Cache Flags

```c
#define CACHE_FLAG_COMPRESSED   0x01  // Entry is compressed
#define CACHE_FLAG_DIRTY        0x02  // Entry has been modified
#define CACHE_FLAG_LOCKED       0x04  // Entry cannot be evicted
```

## Testing

Comprehensive test suite is available in `tests/file_cache_tests.c`:

- `test_cache_init()` - Test initialization
- `test_cache_store_retrieve()` - Test basic store/retrieve operations
- `test_cache_remove()` - Test cache removal
- `test_cache_lru_eviction()` - Test LRU eviction policy
- `test_cache_compression()` - Test compression functionality
- `test_cache_statistics()` - Test statistics tracking
- `test_cache_clear()` - Test cache clearing
- `test_cache_size_filter()` - Test file size filtering

Run tests with:
```c
run_file_cache_tests();
int passed, failed;
get_cache_test_results(&passed, &failed);
```

## Performance Considerations

### Memory Usage

- Each cache entry uses approximately 280 bytes of overhead (metadata)
- Maximum 64 entries can be cached simultaneously
- Total memory usage = overhead + sum of cached file sizes
- Compression can significantly reduce memory usage for suitable files

### Best Practices

1. **Cache Size**: Set based on available RAM and workload
2. **Minimum File Size**: Set to avoid caching tiny files (overhead > benefit)
3. **Access Threshold**: Higher values prevent caching rarely accessed files
4. **Compression**: Enable for text files, disable for already compressed files
5. **Monitoring**: Regularly check statistics to tune configuration

## Implementation Files

- `filesystem/cache/file_cache.h` - Header file with API definitions
- `filesystem/cache/file_cache.c` - Implementation
- `tests/file_cache_tests.h` - Test suite header
- `tests/file_cache_tests.c` - Test suite implementation

## Future Enhancements

Potential improvements for future versions:

- **Advanced Compression**: Add LZ77, LZ4, or other algorithms
- **Async Operations**: Background compression/decompression
- **Priority Levels**: Priority-based eviction instead of pure LRU
- **Persistence**: Option to persist cache across reboots
- **Memory Mapping**: Direct memory mapping for very large files
- **Cache Warming**: Pre-cache files based on usage patterns
- **Multi-level Cache**: L1 (uncompressed) and L2 (compressed) caching

## License

Part of Aurora OS - Proprietary
