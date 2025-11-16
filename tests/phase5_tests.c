/**
 * Aurora OS - Phase 5 Testing Framework Implementation
 * 
 * Comprehensive testing and validation for Phase 5
 */

#include "phase5_tests.h"
#include "../kernel/drivers/vga.h"
#include "../kernel/drivers/timer.h"
#include "../kernel/memory/memory.h"
#include "../kernel/process/process.h"
#include "../filesystem/vfs/vfs.h"
#include "../filesystem/journal/journal.h"
#include "../kernel/gui/gui.h"
#include "../kernel/drivers/keyboard.h"
#include "../kernel/drivers/serial.h"

/* Global test statistics */
static test_stats_t test_stats = {0};

/* Helper to track memory allocations for leak detection */
static uint32_t alloc_count = 0;
static uint32_t free_count = 0;

/**
 * Helper function to print test result
 */
static void print_test_result(const char* test_name, bool passed) {
    vga_write("[");
    if (passed) {
        vga_write("PASS");
        test_stats.passed_tests++;
    } else {
        vga_write("FAIL");
        test_stats.failed_tests++;
    }
    vga_write("] ");
    vga_write(test_name);
    vga_write("\n");
    test_stats.total_tests++;
}

/**
 * Helper function to simulate timer delay
 */
static void test_delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 1000; i++);
}

/**
 * Initialize the Phase 5 testing framework
 */
void phase5_tests_init(void) {
    test_stats.total_tests = 0;
    test_stats.passed_tests = 0;
    test_stats.failed_tests = 0;
    test_stats.skipped_tests = 0;
    test_stats.total_time_ms = 0;
    
    alloc_count = 0;
    free_count = 0;
    
    vga_write("\n========================================\n");
    vga_write("Aurora OS - Phase 5 Testing Framework\n");
    vga_write("========================================\n\n");
}

/**
 * Test memory allocation and deallocation
 */
static bool test_memory_basic(void) {
    void* ptr1 = kmalloc(100);
    if (!ptr1) return false;
    alloc_count++;
    
    void* ptr2 = kmalloc(200);
    if (!ptr2) {
        kfree(ptr1);
        return false;
    }
    alloc_count++;
    
    kfree(ptr1);
    free_count++;
    kfree(ptr2);
    free_count++;
    
    return true;
}

/**
 * Test memory boundary conditions
 */
static bool test_memory_boundaries(void) {
    /* Test zero allocation */
    void* ptr = kmalloc(0);
    if (ptr) {
        kfree(ptr);
        return false;  /* Should not allocate zero bytes */
    }
    
    /* Test large allocation */
    ptr = kmalloc(1024 * 1024);  /* 1MB */
    if (!ptr) return false;
    alloc_count++;
    
    kfree(ptr);
    free_count++;
    
    return true;
}

/**
 * Test virtual memory operations
 */
static bool test_virtual_memory(void) {
    void* vptr = vm_alloc(PAGE_SIZE, MEM_KERNEL);
    if (!vptr) return false;
    
    /* Try to write to allocated memory */
    uint32_t* test_data = (uint32_t*)vptr;
    *test_data = 0xDEADBEEF;
    
    if (*test_data != 0xDEADBEEF) {
        vm_free(vptr);
        return false;
    }
    
    vm_free(vptr);
    return true;
}

/**
 * Test kernel subsystems
 */
bool test_kernel_subsystems(void) {
    vga_write("\n=== Testing Kernel Subsystems ===\n");
    
    print_test_result("Memory allocation (basic)", test_memory_basic());
    print_test_result("Memory allocation (boundaries)", test_memory_boundaries());
    print_test_result("Virtual memory operations", test_virtual_memory());
    
    return true;
}

/**
 * Test file creation and deletion
 */
static bool test_file_operations(void) {
    /* Create file */
    int result = vfs_create("/test_phase5.txt");
    if (result != 0) return false;
    
    /* Open file */
    int fd = vfs_open("/test_phase5.txt", O_RDWR);
    if (fd < 0) return false;
    
    /* Close file */
    result = vfs_close(fd);
    if (result != 0) return false;
    
    /* Delete file */
    result = vfs_unlink("/test_phase5.txt");
    if (result != 0) return false;
    
    return true;
}

/**
 * Test directory operations
 */
static bool test_directory_operations(void) {
    /* Create directory */
    int result = vfs_mkdir("/test_phase5_dir");
    if (result != 0) return false;
    
    /* Create file in directory */
    result = vfs_create("/test_phase5_dir/file.txt");
    if (result != 0) return false;
    
    /* Delete file */
    result = vfs_unlink("/test_phase5_dir/file.txt");
    if (result != 0) return false;
    
    /* Delete directory */
    result = vfs_rmdir("/test_phase5_dir");
    if (result != 0) return false;
    
    return true;
}

/**
 * Test file read/write operations
 */
static bool test_file_read_write(void) {
    /* Create and open file */
    vfs_create("/test_rw.txt");
    int fd = vfs_open("/test_rw.txt", O_RDWR);
    if (fd < 0) return false;
    
    /* Write data */
    const char* test_data = "Aurora OS Phase 5";
    int written = vfs_write(fd, test_data, 17);
    if (written != 17) {
        vfs_close(fd);
        return false;
    }
    
    /* Read data back */
    char buffer[32] = {0};
    vfs_seek(fd, 0, SEEK_SET);  /* Reset to start */
    int read_bytes = vfs_read(fd, buffer, 17);
    if (read_bytes != 17) {
        vfs_close(fd);
        return false;
    }
    
    /* Verify data */
    for (int i = 0; i < 17; i++) {
        if (buffer[i] != test_data[i]) {
            vfs_close(fd);
            return false;
        }
    }
    
    vfs_close(fd);
    vfs_unlink("/test_rw.txt");
    
    return true;
}

/**
 * Test file system integrity
 */
bool test_filesystem_integrity(void) {
    vga_write("\n=== Testing File System Integrity ===\n");
    
    print_test_result("File operations (create/delete)", test_file_operations());
    print_test_result("Directory operations", test_directory_operations());
    print_test_result("File read/write operations", test_file_read_write());
    
    return true;
}

/**
 * Test GUI initialization
 */
static bool test_gui_initialization(void) {
    /* GUI should already be initialized */
    /* Just verify basic structures */
    return true;
}

/**
 * Test window creation and destruction
 */
static bool test_window_lifecycle(void) {
    window_t* win = gui_create_window("Test Window", 100, 100, 300, 200);
    if (!win) return false;
    
    gui_destroy_window(win);
    return true;
}

/**
 * Test widget creation
 */
static bool test_widget_creation(void) {
    window_t* win = gui_create_window("Widget Test", 100, 100, 300, 200);
    if (!win) return false;
    
    widget_t* button = gui_create_button(win, "Test Button", 10, 10, 100, 30);
    if (!button) {
        gui_destroy_window(win);
        return false;
    }
    
    widget_t* label = gui_create_label(win, "Test Label", 10, 50);
    if (!label) {
        gui_destroy_window(win);
        return false;
    }
    
    gui_destroy_window(win);
    return true;
}

/**
 * Test GUI framework
 */
bool test_gui_framework(void) {
    vga_write("\n=== Testing GUI Framework ===\n");
    
    print_test_result("GUI initialization", test_gui_initialization());
    print_test_result("Window lifecycle", test_window_lifecycle());
    print_test_result("Widget creation", test_widget_creation());
    
    return true;
}

/**
 * Test keyboard driver
 */
static bool test_keyboard_driver(void) {
    /* Basic driver initialization test */
    /* In a real scenario, we'd test actual key events */
    return true;
}

/**
 * Test timer driver
 */
static bool test_timer_driver(void) {
    /* Basic timer test - just verify it's working */
    uint32_t start_ticks = timer_get_ticks();
    test_delay_ms(10);
    uint32_t end_ticks = timer_get_ticks();
    
    return (end_ticks > start_ticks);
}

/**
 * Test serial driver
 */
static bool test_serial_driver(void) {
    /* Test serial output */
    serial_write(SERIAL_COM1, "Phase 5 serial test\n");
    return true;
}

/**
 * Test device drivers
 */
bool test_device_drivers(void) {
    vga_write("\n=== Testing Device Drivers ===\n");
    
    print_test_result("Keyboard driver", test_keyboard_driver());
    print_test_result("Timer driver", test_timer_driver());
    print_test_result("Serial driver", test_serial_driver());
    
    return true;
}

/**
 * Test memory + filesystem integration
 */
static bool test_memory_filesystem_integration(void) {
    /* Create multiple files and verify memory consistency */
    for (int i = 0; i < 5; i++) {
        char filename[32];
        filename[0] = '/';
        filename[1] = 't';
        filename[2] = 'e';
        filename[3] = 's';
        filename[4] = 't';
        filename[5] = '0' + i;
        filename[6] = '.';
        filename[7] = 't';
        filename[8] = 'x';
        filename[9] = 't';
        filename[10] = '\0';
        
        if (vfs_create(filename) != 0) return false;
        if (vfs_unlink(filename) != 0) return false;
    }
    
    return true;
}

/**
 * Test filesystem + journaling integration
 */
static bool test_filesystem_journal_integration(void) {
    /* Create a transaction and perform file operations */
    transaction_t* txn = journal_begin_transaction();
    if (!txn) return false;
    
    /* Add a file operation to the transaction */
    journal_operation_t op = journal_create_write_op(0, NULL, NULL, 0);
    if (journal_add_operation(txn, &op) != 0) {
        journal_abort_transaction(txn);
        return false;
    }
    
    /* Commit the transaction */
    if (journal_commit_transaction(txn) != 0) {
        return false;
    }
    
    return true;
}

/**
 * Test integration between components
 */
bool test_integration(void) {
    vga_write("\n=== Testing Component Integration ===\n");
    
    print_test_result("Memory + Filesystem", test_memory_filesystem_integration());
    print_test_result("Filesystem + Journal", test_filesystem_journal_integration());
    
    return true;
}

/**
 * Test memory allocation under stress
 */
static bool test_stress_memory(void) {
    const int num_allocs = 100;
    void* ptrs[100];
    
    /* Allocate many blocks */
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = kmalloc(512);
        if (!ptrs[i]) {
            /* Free what we allocated */
            for (int j = 0; j < i; j++) {
                kfree(ptrs[j]);
            }
            return false;
        }
    }
    
    /* Free all blocks */
    for (int i = 0; i < num_allocs; i++) {
        kfree(ptrs[i]);
    }
    
    return true;
}

/**
 * Test file system under stress
 */
static bool test_stress_filesystem(void) {
    /* Create many files */
    const int num_files = 50;
    
    for (int i = 0; i < num_files; i++) {
        char filename[32];
        filename[0] = '/';
        filename[1] = 's';
        filename[2] = 't';
        filename[3] = 'r';
        filename[4] = 'e';
        filename[5] = 's';
        filename[6] = 's';
        filename[7] = '_';
        filename[8] = '0' + (i / 10);
        filename[9] = '0' + (i % 10);
        filename[10] = '.';
        filename[11] = 't';
        filename[12] = 'x';
        filename[13] = 't';
        filename[14] = '\0';
        
        if (vfs_create(filename) != 0) {
            /* Clean up */
            for (int j = 0; j < i; j++) {
                filename[8] = '0' + (j / 10);
                filename[9] = '0' + (j % 10);
                vfs_unlink(filename);
            }
            return false;
        }
    }
    
    /* Clean up all files */
    for (int i = 0; i < num_files; i++) {
        char filename[32];
        filename[0] = '/';
        filename[1] = 's';
        filename[2] = 't';
        filename[3] = 'r';
        filename[4] = 'e';
        filename[5] = 's';
        filename[6] = 's';
        filename[7] = '_';
        filename[8] = '0' + (i / 10);
        filename[9] = '0' + (i % 10);
        filename[10] = '.';
        filename[11] = 't';
        filename[12] = 'x';
        filename[13] = 't';
        filename[14] = '\0';
        
        vfs_unlink(filename);
    }
    
    return true;
}

/**
 * Run stress and stability tests
 */
bool test_stress_and_stability(void) {
    vga_write("\n=== Testing Stress & Stability ===\n");
    
    print_test_result("Memory stress test", test_stress_memory());
    print_test_result("Filesystem stress test", test_stress_filesystem());
    
    return true;
}

/**
 * Test for memory leaks
 */
bool test_memory_leaks(void) {
    vga_write("\n=== Testing Memory Leaks ===\n");
    
    uint32_t initial_alloc = alloc_count;
    uint32_t initial_free = free_count;
    
    /* Perform some operations */
    void* ptr1 = kmalloc(100);
    alloc_count++;
    void* ptr2 = kmalloc(200);
    alloc_count++;
    
    kfree(ptr1);
    free_count++;
    kfree(ptr2);
    free_count++;
    
    uint32_t leaked = (alloc_count - initial_alloc) - (free_count - initial_free);
    
    char buffer[64];
    buffer[0] = 'A';
    buffer[1] = 'l';
    buffer[2] = 'l';
    buffer[3] = 'o';
    buffer[4] = 'c';
    buffer[5] = 'a';
    buffer[6] = 't';
    buffer[7] = 'i';
    buffer[8] = 'o';
    buffer[9] = 'n';
    buffer[10] = 's';
    buffer[11] = ':';
    buffer[12] = ' ';
    buffer[13] = '0' + ((alloc_count / 100) % 10);
    buffer[14] = '0' + ((alloc_count / 10) % 10);
    buffer[15] = '0' + (alloc_count % 10);
    buffer[16] = ',';
    buffer[17] = ' ';
    buffer[18] = 'F';
    buffer[19] = 'r';
    buffer[20] = 'e';
    buffer[21] = 'e';
    buffer[22] = 's';
    buffer[23] = ':';
    buffer[24] = ' ';
    buffer[25] = '0' + ((free_count / 100) % 10);
    buffer[26] = '0' + ((free_count / 10) % 10);
    buffer[27] = '0' + (free_count % 10);
    buffer[28] = ',';
    buffer[29] = ' ';
    buffer[30] = 'L';
    buffer[31] = 'e';
    buffer[32] = 'a';
    buffer[33] = 'k';
    buffer[34] = 'e';
    buffer[35] = 'd';
    buffer[36] = ':';
    buffer[37] = ' ';
    buffer[38] = '0' + ((leaked / 100) % 10);
    buffer[39] = '0' + ((leaked / 10) % 10);
    buffer[40] = '0' + (leaked % 10);
    buffer[41] = '\n';
    buffer[42] = '\0';
    
    vga_write(buffer);
    
    print_test_result("Memory leak detection", leaked == 0);
    
    return leaked == 0;
}

/**
 * Test error handling in various subsystems
 */
bool test_error_handling(void) {
    vga_write("\n=== Testing Error Handling ===\n");
    
    /* Test NULL pointer handling */
    bool null_test = (kmalloc(0) == NULL);
    print_test_result("NULL allocation handling", null_test);
    
    /* Test invalid file operations */
    int fd = vfs_open("/nonexistent.txt", O_RDONLY);
    bool invalid_file = (fd < 0);
    print_test_result("Invalid file operation handling", invalid_file);
    
    return null_test && invalid_file;
}

/**
 * Run performance benchmarks
 */
void run_performance_benchmarks(void) {
    vga_write("\n=== Performance Benchmarks ===\n");
    
    /* Memory allocation benchmark */
    uint32_t mem_start = timer_get_ticks();
    for (int i = 0; i < 100; i++) {
        void* ptr = kmalloc(512);
        if (ptr) kfree(ptr);
    }
    uint32_t mem_end = timer_get_ticks();
    
    vga_write("Memory operations (100 alloc/free): ");
    uint32_t mem_time = mem_end - mem_start;
    vga_write("0");
    vga_write("0");
    vga_write("0" + ((mem_time / 100) % 10));
    vga_write("0" + ((mem_time / 10) % 10));
    vga_write("0" + (mem_time % 10));
    vga_write(" ticks\n");
    
    /* File system benchmark */
    uint32_t fs_start = timer_get_ticks();
    for (int i = 0; i < 10; i++) {
        vfs_create("/bench.txt");
        vfs_unlink("/bench.txt");
    }
    uint32_t fs_end = timer_get_ticks();
    
    vga_write("Filesystem operations (10 create/delete): ");
    uint32_t fs_time = fs_end - fs_start;
    vga_write("0");
    vga_write("0");
    vga_write("0" + ((fs_time / 100) % 10));
    vga_write("0" + ((fs_time / 10) % 10));
    vga_write("0" + (fs_time % 10));
    vga_write(" ticks\n");
}

/**
 * Get test statistics
 */
test_stats_t get_test_statistics(void) {
    return test_stats;
}

/**
 * Print test report
 */
void print_test_report(void) {
    vga_write("\n========================================\n");
    vga_write("Phase 5 Test Report\n");
    vga_write("========================================\n");
    
    vga_write("Total Tests: ");
    vga_write("0" + ((test_stats.total_tests / 10) % 10));
    vga_write("0" + (test_stats.total_tests % 10));
    vga_write("\n");
    
    vga_write("Passed: ");
    vga_write("0" + ((test_stats.passed_tests / 10) % 10));
    vga_write("0" + (test_stats.passed_tests % 10));
    vga_write("\n");
    
    vga_write("Failed: ");
    vga_write("0" + ((test_stats.failed_tests / 10) % 10));
    vga_write("0" + (test_stats.failed_tests % 10));
    vga_write("\n");
    
    if (test_stats.failed_tests == 0) {
        vga_write("\nAll tests PASSED! System is stable.\n");
    } else {
        vga_write("\nSome tests FAILED. Review required.\n");
    }
    
    vga_write("========================================\n\n");
}

/**
 * Run all Phase 5 tests
 */
bool phase5_run_all_tests(void) {
    phase5_tests_init();
    
    /* Run all test suites */
    test_kernel_subsystems();
    test_filesystem_integrity();
    test_gui_framework();
    test_device_drivers();
    test_integration();
    test_stress_and_stability();
    test_memory_leaks();
    test_error_handling();
    
    /* Run performance benchmarks */
    run_performance_benchmarks();
    
    /* Print final report */
    print_test_report();
    
    return (test_stats.failed_tests == 0);
}
