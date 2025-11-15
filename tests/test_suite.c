/**
 * Aurora OS - Test Suite
 * 
 * Tests for Phase 2 and Phase 3 implementations
 */

#include "../kernel/memory/memory.h"
#include "../kernel/process/process.h"
#include "../filesystem/vfs/vfs.h"
#include "../filesystem/journal/journal.h"
#include "../kernel/drivers/vga.h"

/**
 * Test memory allocation
 */
void test_memory(void) {
    vga_write("\n=== Testing Memory Management ===\n");
    
    /* Test kmalloc */
    void* ptr1 = kmalloc(100);
    if (ptr1) {
        vga_write("kmalloc(100): PASS\n");
        kfree(ptr1);
        vga_write("kfree: PASS\n");
    } else {
        vga_write("kmalloc(100): FAIL\n");
    }
    
    /* Test multiple allocations */
    void* ptr2 = kmalloc(200);
    void* ptr3 = kmalloc(300);
    if (ptr2 && ptr3) {
        vga_write("Multiple allocations: PASS\n");
        kfree(ptr2);
        kfree(ptr3);
    } else {
        vga_write("Multiple allocations: FAIL\n");
    }
    
    /* Test virtual memory */
    void* vptr = vm_alloc(PAGE_SIZE, MEM_KERNEL);
    if (vptr) {
        vga_write("vm_alloc: PASS\n");
        vm_free(vptr);
    } else {
        vga_write("vm_alloc: FAIL\n");
    }
}

/**
 * Simple test process function
 */
void test_process_func(void) {
    /* Process does nothing, just exists */
}

/**
 * Test process management
 */
void test_processes(void) {
    vga_write("\n=== Testing Process Management ===\n");
    
    /* Test process creation */
    process_t* proc1 = process_create(test_process_func, 1);
    if (proc1) {
        vga_write("process_create: PASS\n");
        
        /* Test process termination */
        process_terminate(proc1->pid);
        vga_write("process_terminate: PASS\n");
    } else {
        vga_write("process_create: FAIL\n");
    }
    
    /* Test multiple process creation */
    process_t* proc2 = process_create(test_process_func, 2);
    process_t* proc3 = process_create(test_process_func, 3);
    if (proc2 && proc3) {
        vga_write("Multiple process creation: PASS\n");
        process_terminate(proc2->pid);
        process_terminate(proc3->pid);
    } else {
        vga_write("Multiple process creation: FAIL\n");
    }
}

/**
 * Test file system operations
 */
void test_filesystem(void) {
    vga_write("\n=== Testing File System ===\n");
    
    /* Test file creation */
    int result = vfs_create("/testfile.txt");
    if (result == 0) {
        vga_write("vfs_create: PASS\n");
    } else {
        vga_write("vfs_create: FAIL\n");
    }
    
    /* Test file opening */
    int fd = vfs_open("/testfile.txt", O_RDWR);
    if (fd >= 0) {
        vga_write("vfs_open: PASS\n");
        
        /* Test file close */
        result = vfs_close(fd);
        if (result == 0) {
            vga_write("vfs_close: PASS\n");
        } else {
            vga_write("vfs_close: FAIL\n");
        }
    } else {
        vga_write("vfs_open: FAIL\n");
    }
    
    /* Test directory creation */
    result = vfs_mkdir("/testdir");
    if (result == 0) {
        vga_write("vfs_mkdir: PASS\n");
    } else {
        vga_write("vfs_mkdir: FAIL\n");
    }
    
    /* Test file deletion */
    result = vfs_unlink("/testfile.txt");
    if (result == 0) {
        vga_write("vfs_unlink: PASS\n");
    } else {
        vga_write("vfs_unlink: FAIL\n");
    }
}

/**
 * Test journaling system
 */
void test_journaling(void) {
    vga_write("\n=== Testing Journaling ===\n");
    
    /* Test transaction creation */
    transaction_t* txn = journal_begin_transaction();
    if (txn) {
        vga_write("journal_begin_transaction: PASS\n");
        
        /* Test adding operation */
        journal_operation_t op = journal_create_write_op(0, NULL, NULL, 0);
        int result = journal_add_operation(txn, &op);
        if (result == 0) {
            vga_write("journal_add_operation: PASS\n");
        } else {
            vga_write("journal_add_operation: FAIL\n");
        }
        
        /* Test transaction commit */
        result = journal_commit_transaction(txn);
        if (result == 0) {
            vga_write("journal_commit_transaction: PASS\n");
        } else {
            vga_write("journal_commit_transaction: FAIL\n");
        }
    } else {
        vga_write("journal_begin_transaction: FAIL\n");
    }
    
    /* Test transaction abort */
    txn = journal_begin_transaction();
    if (txn) {
        int result = journal_abort_transaction(txn);
        if (result == 0) {
            vga_write("journal_abort_transaction: PASS\n");
        } else {
            vga_write("journal_abort_transaction: FAIL\n");
        }
    }
}

/**
 * Run all tests
 */
void run_tests(void) {
    vga_write("\n========================================\n");
    vga_write("Aurora OS - Phase 2 & 3 Test Suite\n");
    vga_write("========================================\n");
    
    test_memory();
    test_processes();
    test_filesystem();
    test_journaling();
    
    vga_write("\n========================================\n");
    vga_write("Test Suite Complete\n");
    vga_write("========================================\n\n");
}
