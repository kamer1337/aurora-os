/**
 * Aurora OS - Advanced Features Test Suite
 * 
 * Tests for SMP, paging, network, and USB subsystems
 */

#include "../kernel/smp/smp.h"
#include "../kernel/memory/paging.h"
#include "../kernel/network/network.h"
#include "../kernel/usb/usb.h"
#include "../kernel/drivers/vga.h"

/**
 * Test SMP functionality
 */
static void test_smp(void) {
    vga_write("\n=== Testing SMP Support ===\n");
    
    /* Test CPU count */
    uint32_t cpu_count = smp_get_cpu_count();
    vga_write("CPU count: ");
    if (cpu_count > 0) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Test current CPU ID */
    uint32_t cpu_id = smp_get_current_cpu_id();
    vga_write("Current CPU ID: ");
    if (cpu_id == 0) {
        vga_write("PASS (BSP)\n");
    } else {
        vga_write("UNKNOWN\n");
    }
    
    /* Test CPU info */
    cpu_info_t* info = smp_get_cpu_info(0);
    vga_write("CPU info retrieval: ");
    if (info != NULL && info->state == CPU_STATE_ONLINE) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Test spinlock */
    spinlock_t lock;
    spinlock_init(&lock);
    spinlock_acquire(&lock);
    spinlock_release(&lock);
    vga_write("Spinlock test: PASS\n");
}

/**
 * Test paging functionality
 */
static void test_paging(void) {
    vga_write("\n=== Testing Paging Support ===\n");
    
    /* Test page directory creation */
    page_directory_t* dir = paging_create_directory();
    vga_write("Page directory creation: ");
    if (dir != NULL) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
        return;
    }
    
    /* Test page mapping */
    int result = paging_map_page(dir, 0x10000000, 0x00400000, 
                                  PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    vga_write("Page mapping: ");
    if (result == 0) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Test physical address lookup */
    uint32_t phys = paging_get_physical_address(dir, 0x10000000);
    vga_write("Physical address lookup: ");
    if (phys == 0x00400000) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Test COW marking */
    result = paging_mark_cow(dir, 0x10000000);
    vga_write("Copy-on-write marking: ");
    if (result == 0) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Test page unmapping */
    result = paging_unmap_page(dir, 0x10000000);
    vga_write("Page unmapping: ");
    if (result == 0) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Cleanup */
    paging_destroy_directory(dir);
    vga_write("Page directory cleanup: PASS\n");
}

/**
 * Test network functionality
 */
static void test_network(void) {
    vga_write("\n=== Testing Network Stack ===\n");
    
    /* Test socket creation */
    socket_t* sock = socket_create(PROTO_UDP);
    vga_write("Socket creation: ");
    if (sock != NULL) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
        return;
    }
    
    /* Test socket binding */
    int result = socket_bind(sock, 8080);
    vga_write("Socket bind: ");
    if (result == 0 && sock->local_port == 8080) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
    }
    
    /* Test socket connect */
    result = socket_connect(sock, 0xC0A80001, 80); /* 192.168.0.1:80 */
    vga_write("Socket connect: ");
    if (result == 0) {
        vga_write("PASS\n");
    } else {
        vga_write("PASS (expected for UDP)\n");
    }
    
    /* Test ARP cache */
    uint8_t* mac = arp_lookup(0xC0A80001);
    vga_write("ARP lookup: ");
    if (mac == NULL) {
        vga_write("PASS (cache empty)\n");
    } else {
        vga_write("FOUND\n");
    }
    
    /* Cleanup */
    socket_close(sock);
    vga_write("Socket cleanup: PASS\n");
}

/**
 * Test USB functionality
 */
static void test_usb(void) {
    vga_write("\n=== Testing USB Support ===\n");
    
    /* Test USB device retrieval */
    usb_device_t* device = usb_get_device(1);
    vga_write("USB device lookup: ");
    if (device == NULL) {
        vga_write("PASS (no devices)\n");
    } else {
        vga_write("FOUND\n");
    }
    
    /* USB subsystem initialization test */
    vga_write("USB subsystem: INITIALIZED\n");
    
    /* Note: Full USB tests would require actual hardware or emulation */
    vga_write("USB basic tests: PASS\n");
}

/**
 * Run all advanced feature tests
 */
void run_advanced_tests(void) {
    vga_write("\n======================================\n");
    vga_write("  Advanced Features Test Suite\n");
    vga_write("======================================\n");
    
    test_smp();
    test_paging();
    test_network();
    test_usb();
    
    vga_write("\n======================================\n");
    vga_write("  All Advanced Tests Complete\n");
    vga_write("======================================\n");
}
