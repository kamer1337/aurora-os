/**
 * Aurora OS - Partition Persistence Test Suite
 * 
 * Tests for partition table persistence functionality
 */

#include "../kernel/drivers/partition.h"
#include "../kernel/drivers/storage.h"
#include <stdint.h>
#include <stddef.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
    } while(0)

/**
 * Test partition table persistence (save and load)
 */
void test_partition_persistence(void) {
    partition_init();
    storage_init();
    
    // Initialize a test disk
    int result = partition_scan_disk(0);
    TEST_ASSERT(result == 0, "Disk initialized");
    
    // Create several partitions
    int part1 = partition_create(0, 2048, 1024000, PART_TYPE_LINUX);
    TEST_ASSERT(part1 >= 0, "First partition created");
    
    int part2 = partition_create(0, 1026048, 512000, PART_TYPE_AURORA);
    TEST_ASSERT(part2 >= 0, "Second partition created");
    
    int part3 = partition_create(0, 1538048, 256000, PART_TYPE_FAT32);
    TEST_ASSERT(part3 >= 0, "Third partition created");
    
    // Verify partition count
    TEST_ASSERT(partition_get_count(0) == 3, "Partition count correct before persistence");
    
    // Get info before write
    const partition_t* orig_part1 = partition_get_info(0, 0);
    const partition_t* orig_part2 = partition_get_info(0, 1);
    const partition_t* orig_part3 = partition_get_info(0, 2);
    
    TEST_ASSERT(orig_part1 != NULL, "First partition info retrieved");
    TEST_ASSERT(orig_part2 != NULL, "Second partition info retrieved");
    TEST_ASSERT(orig_part3 != NULL, "Third partition info retrieved");
    
    // Write partition table to disk (this happens automatically in partition_create)
    // We test explicit write here
    result = partition_write_table(0);
    TEST_ASSERT(result == 0, "Partition table written to disk");
    
    // Clear in-memory partition table (simulate reboot)
    partition_init();
    partition_scan_disk(0);
    TEST_ASSERT(partition_get_count(0) == 0, "Partition count cleared after init");
    
    // Read partition table from disk
    result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Partition table read from disk");
    
    // Verify partition count after reload
    TEST_ASSERT(partition_get_count(0) == 3, "Partition count restored after persistence");
    
    // Verify partition data integrity
    const partition_t* loaded_part1 = partition_get_info(0, 0);
    const partition_t* loaded_part2 = partition_get_info(0, 1);
    const partition_t* loaded_part3 = partition_get_info(0, 2);
    
    TEST_ASSERT(loaded_part1 != NULL, "First partition restored");
    TEST_ASSERT(loaded_part2 != NULL, "Second partition restored");
    TEST_ASSERT(loaded_part3 != NULL, "Third partition restored");
    
    // Check first partition details
    TEST_ASSERT(loaded_part1->start_lba == 2048, "Partition 1 start LBA restored");
    TEST_ASSERT(loaded_part1->size_sectors == 1024000, "Partition 1 size restored");
    TEST_ASSERT(loaded_part1->type == PART_TYPE_LINUX, "Partition 1 type restored");
    
    // Check second partition details
    TEST_ASSERT(loaded_part2->start_lba == 1026048, "Partition 2 start LBA restored");
    TEST_ASSERT(loaded_part2->size_sectors == 512000, "Partition 2 size restored");
    TEST_ASSERT(loaded_part2->type == PART_TYPE_AURORA, "Partition 2 type restored");
    
    // Check third partition details
    TEST_ASSERT(loaded_part3->start_lba == 1538048, "Partition 3 start LBA restored");
    TEST_ASSERT(loaded_part3->size_sectors == 256000, "Partition 3 size restored");
    TEST_ASSERT(loaded_part3->type == PART_TYPE_FAT32, "Partition 3 type restored");
}

/**
 * Test automatic persistence on partition operations
 */
void test_automatic_persistence(void) {
    partition_init();
    storage_init();
    partition_scan_disk(0);
    
    // Create a partition (should auto-persist)
    int part_id = partition_create(0, 2048, 1000000, PART_TYPE_LINUX);
    TEST_ASSERT(part_id >= 0, "Partition created with auto-persist");
    
    // Simulate reboot
    partition_init();
    partition_scan_disk(0);
    int result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Partition table loaded after reboot");
    TEST_ASSERT(partition_get_count(0) == 1, "Auto-persisted partition restored");
    
    // Resize partition (should auto-persist)
    result = partition_resize(0, 0, 2000000);
    TEST_ASSERT(result == 0, "Partition resized with auto-persist");
    
    // Simulate reboot
    partition_init();
    partition_scan_disk(0);
    result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Partition table loaded after resize");
    
    const partition_t* part = partition_get_info(0, 0);
    TEST_ASSERT(part->size_sectors == 2000000, "Resized partition size persisted");
    
    // Delete partition (should auto-persist)
    result = partition_delete(0, 0);
    TEST_ASSERT(result == 0, "Partition deleted with auto-persist");
    
    // Simulate reboot
    partition_init();
    partition_scan_disk(0);
    result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Partition table loaded after delete");
    TEST_ASSERT(partition_get_count(0) == 0, "Deleted partition not restored");
}

/**
 * Test partition table type persistence
 */
void test_partition_table_type_persistence(void) {
    partition_init();
    storage_init();
    partition_scan_disk(0);
    
    // Create MBR partition table
    int result = partition_create_mbr(0);
    TEST_ASSERT(result == 0, "MBR partition table created");
    
    const disk_info_t* disk = partition_get_disk_info(0);
    TEST_ASSERT(disk->table_type == PART_TABLE_MBR, "Table type set to MBR");
    
    // Simulate reboot and verify
    partition_init();
    partition_scan_disk(0);
    result = partition_read_table(0);
    TEST_ASSERT(result == 0, "MBR table loaded");
    
    disk = partition_get_disk_info(0);
    TEST_ASSERT(disk->table_type == PART_TABLE_MBR, "MBR table type persisted");
    
    // Create GPT partition table
    result = partition_create_gpt(0);
    TEST_ASSERT(result == 0, "GPT partition table created");
    
    disk = partition_get_disk_info(0);
    TEST_ASSERT(disk->table_type == PART_TABLE_GPT, "Table type set to GPT");
    
    // Simulate reboot and verify
    partition_init();
    partition_scan_disk(0);
    result = partition_read_table(0);
    TEST_ASSERT(result == 0, "GPT table loaded");
    
    disk = partition_get_disk_info(0);
    TEST_ASSERT(disk->table_type == PART_TABLE_GPT, "GPT table type persisted");
}

/**
 * Test empty partition table persistence
 */
void test_empty_table_persistence(void) {
    partition_init();
    storage_init();
    partition_scan_disk(0);
    
    // Write empty partition table
    int result = partition_write_table(0);
    TEST_ASSERT(result == 0, "Empty partition table written");
    
    // Simulate reboot
    partition_init();
    partition_scan_disk(0);
    result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Empty partition table loaded");
    TEST_ASSERT(partition_get_count(0) == 0, "Empty table has no partitions");
}

/**
 * Test corruption detection (simulated)
 */
void test_corruption_detection(void) {
    partition_init();
    storage_init();
    partition_scan_disk(0);
    
    // Note: In a real implementation, we would corrupt the on-disk data
    // For now, we just verify that reading a non-existent table works
    
    // Read from disk without writing (no valid table exists)
    int result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Reading non-existent table initializes empty table");
    TEST_ASSERT(partition_get_count(0) == 0, "Non-existent table results in empty partitions");
}

/**
 * Test maximum partition persistence
 */
void test_maximum_partitions_persistence(void) {
    partition_init();
    storage_init();
    partition_scan_disk(0);
    
    // Create maximum number of partitions (7 to fit in 512 bytes)
    const int max_to_test = 7;
    for (int i = 0; i < max_to_test; i++) {
        uint32_t start_lba = 2048 + (i * 100000);
        int part_id = partition_create(0, start_lba, 50000, PART_TYPE_LINUX);
        TEST_ASSERT(part_id == i, "Partition created successfully");
    }
    
    TEST_ASSERT(partition_get_count(0) == max_to_test, "All partitions created");
    
    // Simulate reboot
    partition_init();
    partition_scan_disk(0);
    int result = partition_read_table(0);
    TEST_ASSERT(result == 0, "Maximum partitions table loaded");
    TEST_ASSERT(partition_get_count(0) == max_to_test, "All partitions restored");
    
    // Verify each partition
    for (int i = 0; i < max_to_test; i++) {
        const partition_t* part = partition_get_info(0, i);
        TEST_ASSERT(part != NULL, "Partition info retrieved");
        
        uint32_t expected_start = 2048 + (i * 100000);
        TEST_ASSERT(part->start_lba == expected_start, "Partition start LBA correct");
        TEST_ASSERT(part->size_sectors == 50000, "Partition size correct");
    }
}

/**
 * Run all partition persistence tests
 */
void run_partition_persistence_tests(void) {
    tests_passed = 0;
    tests_failed = 0;
    
    test_partition_persistence();
    test_automatic_persistence();
    test_partition_table_type_persistence();
    test_empty_table_persistence();
    test_corruption_detection();
    test_maximum_partitions_persistence();
}

/**
 * Get test results
 */
void get_partition_persistence_test_results(int* passed, int* failed) {
    *passed = tests_passed;
    *failed = tests_failed;
}
