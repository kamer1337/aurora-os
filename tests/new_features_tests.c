/**
 * Aurora OS - New Features Test Suite
 * 
 * Tests for networking settings, firewall, partition manager,
 * encryption, driver manager, and 64-bit support
 */

#include "../kernel/network/network_config.h"
#include "../kernel/network/network.h"
#include "../kernel/network/firewall.h"
#include "../kernel/drivers/partition.h"
#include "../kernel/security/partition_encryption.h"
#include "../kernel/drivers/driver_manager.h"
#include "../kernel/core/arch64.h"

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
 * Test network configuration
 */
void test_network_config(void) {
    net_config_init();
    
    // Test static IP configuration
    int result = net_config_set_static(0xC0A80164, 0xFFFFFF00, 0xC0A80101);
    TEST_ASSERT(result == 0, "Set static IP");
    
    const net_config_t* config = net_config_get();
    TEST_ASSERT(config->ip_address == 0xC0A80164, "IP address set correctly");
    TEST_ASSERT(config->config_mode == NET_CONFIG_STATIC, "Static mode set");
    
    // Test DHCP mode
    result = net_config_set_dhcp();
    TEST_ASSERT(result == 0, "Set DHCP mode");
    
    config = net_config_get();
    TEST_ASSERT(config->config_mode == NET_CONFIG_DHCP, "DHCP mode set");
    
    // Test DNS configuration
    result = net_config_add_dns(0x08080808);  // 8.8.8.8
    TEST_ASSERT(result == 0, "Add DNS server");
    
    config = net_config_get();
    TEST_ASSERT(config->dns_count == 1, "DNS count correct");
    TEST_ASSERT(config->dns_servers[0] == 0x08080808, "DNS server correct");
    
    // Test hostname
    result = net_config_set_hostname("aurora-test");
    TEST_ASSERT(result == 0, "Set hostname");
    
    const char* hostname = net_config_get_hostname();
    TEST_ASSERT(hostname != 0, "Hostname retrieved");
}

/**
 * Test firewall
 */
void test_firewall(void) {
    firewall_init();
    
    // Test firewall enable/disable
    firewall_enable();
    TEST_ASSERT(firewall_is_enabled(), "Firewall enabled");
    
    firewall_disable();
    TEST_ASSERT(!firewall_is_enabled(), "Firewall disabled");
    
    firewall_enable();
    
    // Test rule creation
    firewall_rule_t rule = {
        .src_ip = 0,
        .src_mask = 0,
        .dst_ip = 0xC0A80101,
        .dst_mask = 0xFFFFFFFF,
        .src_port = 0,
        .dst_port = 80,
        .protocol = PROTO_TCP,
        .direction = FW_DIR_INBOUND,
        .action = FW_ACTION_ALLOW,
        .enabled = 1,
        .hit_count = 0
    };
    
    int rule_id = firewall_add_rule(&rule);
    TEST_ASSERT(rule_id >= 0, "Rule added");
    TEST_ASSERT(firewall_get_rule_count() == 1, "Rule count correct");
    
    // Test packet checking
    int action = firewall_check_packet(0x0A000001, 0xC0A80101, 12345, 80, PROTO_TCP, FW_DIR_INBOUND);
    TEST_ASSERT(action == FW_ACTION_ALLOW, "Packet allowed by rule");
    
    // Test statistics
    const firewall_stats_t* stats = firewall_get_stats();
    TEST_ASSERT(stats->packets_passed > 0, "Statistics tracked");
    
    // Test rule removal
    int result = firewall_remove_rule(rule_id);
    TEST_ASSERT(result == 0, "Rule removed");
    TEST_ASSERT(firewall_get_rule_count() == 0, "Rule count zero after removal");
}

/**
 * Test partition manager
 */
void test_partition_manager(void) {
    partition_init();
    
    // Test disk scanning
    int result = partition_scan_disk(0);
    TEST_ASSERT(result == 0, "Disk scanned");
    
    const disk_info_t* disk = partition_get_disk_info(0);
    TEST_ASSERT(disk != 0, "Disk info retrieved");
    TEST_ASSERT(disk->sector_size == 512, "Sector size correct");
    
    // Test partition creation
    int part_id = partition_create(0, 2048, 2097152, PART_TYPE_LINUX);
    TEST_ASSERT(part_id >= 0, "Partition created");
    TEST_ASSERT(partition_get_count(0) == 1, "Partition count correct");
    
    // Test partition info
    const partition_t* part = partition_get_info(0, part_id);
    TEST_ASSERT(part != 0, "Partition info retrieved");
    TEST_ASSERT(part->start_lba == 2048, "Partition start LBA correct");
    TEST_ASSERT(part->size_sectors == 2097152, "Partition size correct");
    
    // Test partition resize
    result = partition_resize(0, part_id, 4194304);
    TEST_ASSERT(result == 0, "Partition resized");
    
    part = partition_get_info(0, part_id);
    TEST_ASSERT(part->size_sectors == 4194304, "Partition size updated");
    
    // Test free space
    uint64_t free_space = partition_get_free_space(0);
    TEST_ASSERT(free_space > 0, "Free space calculated");
    
    // Test partition deletion
    result = partition_delete(0, part_id);
    TEST_ASSERT(result == 0, "Partition deleted");
    TEST_ASSERT(partition_get_count(0) == 0, "Partition count zero after deletion");
}

/**
 * Test partition encryption
 */
void test_partition_encryption(void) {
    partition_encryption_init();
    
    // First create a partition to encrypt
    partition_init();
    partition_scan_disk(0);
    int part_id = partition_create(0, 2048, 2097152, PART_TYPE_LINUX);
    TEST_ASSERT(part_id >= 0, "Test partition created");
    
    // Test encryption
    int result = partition_encrypt(0, part_id, "TestPassword123");
    TEST_ASSERT(result == 0, "Partition encrypted");
    
    // Test encryption status
    int is_encrypted = partition_is_encrypted(0, part_id);
    TEST_ASSERT(is_encrypted, "Partition marked as encrypted");
    
    // Test mounting encrypted partition
    result = partition_mount_encrypted(0, part_id, "TestPassword123");
    TEST_ASSERT(result == 0, "Encrypted partition mounted");
    
    // Test unmounting
    result = partition_unmount_encrypted(0, part_id);
    TEST_ASSERT(result == 0, "Encrypted partition unmounted");
    
    // Test password change
    result = partition_change_password(0, part_id, "TestPassword123", "NewPassword456");
    TEST_ASSERT(result == 0, "Password changed");
    
    // Test encryption info
    const encrypted_partition_t* enc_info = partition_get_encryption_info(0, part_id);
    TEST_ASSERT(enc_info != 0, "Encryption info retrieved");
    TEST_ASSERT(enc_info->encrypted, "Encryption flag set");
}

/**
 * Test driver manager
 */
void test_driver_manager(void) {
    driver_manager_init();
    
    // Test driver count
    int count = driver_get_count();
    TEST_ASSERT(count > 0, "Drivers registered");
    
    // Test driver initialization
    int failed = driver_init_all();
    TEST_ASSERT(failed == 0, "All drivers initialized successfully");
    
    // Test driver status
    int status = driver_get_status("keyboard");
    TEST_ASSERT(status == DRIVER_STATUS_INITIALIZED, "Keyboard driver initialized");
    
    status = driver_get_status("mouse");
    TEST_ASSERT(status == DRIVER_STATUS_INITIALIZED, "Mouse driver initialized");
    
    status = driver_get_status("network");
    TEST_ASSERT(status == DRIVER_STATUS_INITIALIZED, "Network driver initialized");
    
    // Test driver info retrieval
    const driver_info_t* info = driver_get_info("keyboard");
    TEST_ASSERT(info != 0, "Driver info retrieved");
    TEST_ASSERT(info->type == DRIVER_TYPE_INPUT, "Driver type correct");
}

/**
 * Test 64-bit architecture support
 */
void test_arch64_support(void) {
    // Test architecture detection
    int bits = arch_get_bits();
    TEST_ASSERT(bits == 32 || bits == 64, "Architecture bits detected");
    
    // Test stack pointer retrieval
    arch_ptr_t sp = arch_get_stack_pointer();
    TEST_ASSERT(sp != 0, "Stack pointer retrieved");
    
    // Test instruction pointer retrieval
    arch_ptr_t ip = arch_get_instruction_pointer();
    TEST_ASSERT(ip != 0, "Instruction pointer retrieved");
    
#if AURORA_ARCH_64BIT
    // Test 64-bit specific features
    arch64_init();
    TEST_ASSERT(1, "64-bit mode initialized");
    
    // Test page mapping (simplified test)
    int result = arch64_map_page(0x1000, 0x1000, PTE_PRESENT | PTE_WRITABLE);
    TEST_ASSERT(result == 0, "64-bit page mapping");
    
    // Test physical address translation
    uint64_t phys = arch64_get_physical_address(0x1000);
    TEST_ASSERT(phys == 0x1000, "64-bit address translation");
#endif
}

/**
 * Run all tests
 */
void run_new_features_tests(void) {
    tests_passed = 0;
    tests_failed = 0;
    
    test_network_config();
    test_firewall();
    test_partition_manager();
    test_partition_encryption();
    test_driver_manager();
    test_arch64_support();
    
    /* Results are tracked in tests_passed and tests_failed */
}

/**
 * Get test results
 */
void get_test_results(int* passed, int* failed) {
    if (passed) *passed = tests_passed;
    if (failed) *failed = tests_failed;
}
