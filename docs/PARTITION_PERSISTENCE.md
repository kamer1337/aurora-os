# Disk Partition Persistence

## Overview

Aurora OS now includes a robust partition persistence layer that automatically saves and loads partition table information to/from disk. This ensures that partition configurations survive system reboots and power cycles.

## Features

### 1. Automatic Persistence
- All partition operations (create, delete, resize) automatically trigger persistence to disk
- No manual intervention required
- Rollback support on write failures ensures data integrity

### 2. Data Integrity
- CRC32 checksum verification on load
- Magic signature validation (0x41555250 "AURP")
- Version tracking for future compatibility
- Corruption detection

### 3. Storage Format
- Partition table stored at LBA 1 (sector 1) of each disk
- 512-byte sector format
- Supports up to 7 partitions per disk
- Compatible with both MBR and GPT table types

## Architecture

### Persistent Partition Table Structure

```c
typedef struct {
    uint32_t magic;                      // Magic signature: 0x41555250 ("AURP")
    uint32_t version;                    // Table version (currently 1)
    uint32_t checksum;                   // CRC32 checksum
    uint8_t table_type;                  // PART_TABLE_MBR or PART_TABLE_GPT
    uint8_t partition_count;             // Number of partitions (0-7)
    uint8_t reserved[6];                 // Reserved for alignment
    partition_t partitions[MAX_PARTITIONS (8)];  // Partition entries
} persistent_partition_table_t;
```

### Partition Entry Structure

```c
typedef struct {
    uint8_t status;        // Boot indicator (PART_STATUS_INACTIVE or PART_STATUS_ACTIVE)
    uint8_t type;          // Partition type (LINUX, AURORA, FAT32, etc.)
    uint32_t start_lba;    // Starting LBA sector
    uint32_t size_sectors; // Size in sectors
    uint64_t size_bytes;   // Size in bytes
    uint8_t encrypted;     // Encryption flag
    char label[32];        // Partition label
} partition_t;
```

## API Reference

### Loading Partition Table

```c
int partition_read_table(uint8_t disk_id);
```

Reads the partition table from disk sector (LBA 1) and loads it into memory.

**Parameters:**
- `disk_id`: Disk identifier (0-3)

**Returns:**
- `0`: Success
- `-1`: Invalid disk_id
- `-2`: No storage device found
- `-3`: Read error
- `-4`: Checksum mismatch (corrupted data)

**Behavior:**
- If no valid partition table is found (no magic signature), initializes an empty table
- Verifies CRC32 checksum before loading
- Loads all partition entries into memory

### Saving Partition Table

```c
int partition_write_table(uint8_t disk_id);
```

Writes the in-memory partition table to disk sector (LBA 1).

**Parameters:**
- `disk_id`: Disk identifier (0-3)

**Returns:**
- `0`: Success
- `-1`: Invalid disk_id
- `-2`: No storage device found
- `-3`: Write error

**Behavior:**
- Calculates CRC32 checksum
- Writes to disk with automatic cache flush
- Called automatically by partition operations

### Automatic Persistence

All partition operations automatically persist changes:

```c
// Create partition - automatically persists
int part_id = partition_create(0, 2048, 1024000, PART_TYPE_LINUX);

// Resize partition - automatically persists
partition_resize(0, part_id, 2048000);

// Delete partition - automatically persists
partition_delete(0, part_id);

// Create new partition table - automatically persists
partition_create_mbr(0);
partition_create_gpt(0);
```

## Usage Examples

### Example 1: Creating and Persisting Partitions

```c
// Initialize partition manager
partition_init();
storage_init();

// Scan disk (loads any existing partition table)
partition_scan_disk(0);

// Try to load existing partition table
int result = partition_read_table(0);
if (result == 0) {
    printf("Loaded %d partitions from disk\n", partition_get_count(0));
} else if (result == -4) {
    printf("Partition table corrupted, initializing new table\n");
    partition_create_mbr(0);
}

// Create new partition (automatically persisted)
int part_id = partition_create(0, 2048, 1024000, PART_TYPE_LINUX);
if (part_id >= 0) {
    printf("Partition created and persisted to disk\n");
}

// Partition will survive reboot
```

### Example 2: Boot-time Recovery

```c
void boot_init_partitions(void) {
    partition_init();
    storage_init();
    
    // Scan all disks
    for (uint8_t disk_id = 0; disk_id < 4; disk_id++) {
        if (partition_scan_disk(disk_id) == 0) {
            // Try to load partition table
            int result = partition_read_table(disk_id);
            
            if (result == 0) {
                int count = partition_get_count(disk_id);
                printf("Disk %d: Loaded %d partitions\n", disk_id, count);
            } else if (result == -4) {
                printf("Disk %d: Corrupted partition table\n", disk_id);
            } else {
                printf("Disk %d: No partition table, initializing\n", disk_id);
                partition_create_mbr(disk_id);
            }
        }
    }
}
```

### Example 3: Safe Partition Modifications

```c
// The system automatically handles rollback on failures
int resize_partition_safely(uint8_t disk_id, uint8_t part_id, uint32_t new_size) {
    // Attempt resize
    int result = partition_resize(disk_id, part_id, new_size);
    
    if (result == -4) {
        // Automatic rollback occurred - partition unchanged
        printf("Failed to persist changes, partition unchanged\n");
        return -1;
    }
    
    if (result == 0) {
        printf("Partition resized and persisted successfully\n");
        return 0;
    }
    
    return result;
}
```

## Data Integrity Features

### CRC32 Checksum

The system uses a standard CRC32 polynomial (0xEDB88320) to compute checksums:

1. Checksum is calculated over the entire partition table structure
2. On write: checksum is computed and stored
3. On read: checksum is recomputed and compared with stored value
4. Mismatch indicates corruption

### Rollback on Failure

All modifying operations (create, delete, resize) implement rollback:

```c
// Example from partition_create
partition_t* new_part = &disk->partitions[disk->partition_count];
// ... initialize partition ...
disk->partition_count++;

// Persist to disk
int result = partition_write_table(disk_id);
if (result != 0) {
    // Rollback on write failure
    disk->partition_count--;
    return -5;  // Failed to persist
}
```

## Limitations and Future Work

### Current Limitations

1. **Single Sector**: Partition table limited to 512 bytes (7 partitions max)
2. **LBA 1 Only**: Only supports one partition table per disk
3. **No Backup**: No secondary/backup partition table (unlike GPT)
4. **No Journaling**: Changes are not journaled

### Future Enhancements

1. **Extended Tables**: Support for more partitions using multiple sectors
2. **Backup Tables**: Secondary partition table for redundancy (GPT-style)
3. **Journaling**: Transaction log for safer updates
4. **Compression**: Compress partition table for more entries
5. **Encryption**: Encrypt partition table metadata

## Testing

The partition persistence feature includes comprehensive test coverage:

### Test Suites

1. **test_partition_persistence**: Basic save/load verification
2. **test_automatic_persistence**: Verify auto-save on operations
3. **test_partition_table_type_persistence**: MBR/GPT type handling
4. **test_empty_table_persistence**: Empty table edge cases
5. **test_corruption_detection**: Corruption detection and recovery
6. **test_maximum_partitions_persistence**: Maximum capacity testing

### Running Tests

```bash
# Build with tests
make all

# Run partition persistence tests
# (Tests are included in the kernel binary)
```

## Security Considerations

### Data Integrity

- CRC32 provides good error detection but not cryptographic security
- Checksum prevents accidental corruption but not malicious tampering
- Consider adding HMAC or digital signatures for security-critical deployments

### Sector Protection

- LBA 1 should be protected from user writes
- File system should not allocate LBA 1 for file storage
- Boot loader should not overwrite LBA 1

### Backup Recommendations

- Implement periodic backups of partition tables
- Store backup in alternative location (end of disk, external media)
- Implement recovery tools for corrupted tables

## Troubleshooting

### Checksum Mismatch Error

**Symptom**: `partition_read_table()` returns -4

**Causes**:
1. Disk corruption
2. Power loss during write
3. Hardware failure

**Recovery**:
```c
// Initialize new partition table
partition_init();
partition_scan_disk(disk_id);
partition_create_mbr(disk_id);  // or partition_create_gpt(disk_id)
```

### Partition Not Persisting

**Symptom**: Partitions disappear after reboot

**Causes**:
1. Storage device not initialized
2. Write failures
3. Insufficient permissions

**Debug**:
```c
// Check write result
int part_id = partition_create(0, 2048, 1024000, PART_TYPE_LINUX);
if (part_id == -5) {
    printf("Failed to persist partition table\n");
    // Check storage device status
}
```

## Implementation Notes

### Thread Safety

Current implementation is **not thread-safe**. Use external locking if accessing from multiple threads:

```c
// Example with mutex
mutex_lock(&partition_mutex);
int result = partition_create(0, 2048, 1024000, PART_TYPE_LINUX);
mutex_unlock(&partition_mutex);
```

### Performance

- **Write Operations**: ~1ms per partition operation (one sector write)
- **Read Operations**: ~1ms on boot (one sector read)
- **Cache Flush**: Adds ~1-5ms depending on storage device

### Memory Usage

- Static allocation: `4 * sizeof(disk_info_t)` ≈ 3KB
- Per-disk overhead: ~800 bytes
- No dynamic allocation

## References

- CRC32 Polynomial: IEEE 802.3 / PNG specification
- MBR Specification: IBM PC DOS
- GPT Specification: UEFI 2.x
- Aurora OS Storage Driver: `kernel/drivers/storage.h`
- Partition Manager: `kernel/drivers/partition.h`

## Changelog

### Version 1.0 (Current)
- Initial implementation
- CRC32 checksum support
- Automatic persistence
- Support for up to 7 partitions
- MBR and GPT table type support
- Rollback on write failure
- Comprehensive test suite

---

*Last Updated: December 2025*
*Aurora OS Documentation*
