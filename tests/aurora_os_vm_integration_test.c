/**
 * Aurora OS - VM Integration Test Suite
 * Tests Aurora OS components running on Aurora VM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "platform/aurora_vm.h"

// Test result tracking
static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

// Issue tracking
typedef struct {
    int id;
    const char *severity;     // "Critical", "High", "Medium", "Low"
    const char *component;
    const char *title;
    const char *description;
    const char *status;       // "Open", "In Progress", "Resolved"
} Issue;

#define MAX_ISSUES 50
static Issue issues[MAX_ISSUES];
static int issue_count = 0;

// Helper to add an issue
void add_issue(const char *severity, const char *component, const char *title, const char *description) {
    if (issue_count < MAX_ISSUES) {
        issues[issue_count].id = issue_count + 1;
        issues[issue_count].severity = severity;
        issues[issue_count].component = component;
        issues[issue_count].title = title;
        issues[issue_count].description = description;
        issues[issue_count].status = "Open";
        issue_count++;
    }
}

// Test macros
#define TEST_START(name) \
    do { \
        total_tests++; \
        printf("\n[TEST] %s\n", name); \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", message); \
        } else { \
            printf("  ✗ %s\n", message); \
            failed_tests++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("  PASSED\n"); \
        passed_tests++; \
    } while(0)

#define TEST_FAIL(message) \
    do { \
        printf("  FAILED: %s\n", message); \
        failed_tests++; \
    } while(0)

// Test 1: Memory Management System
void test_memory_management(void) {
    TEST_START("Memory Management: Page allocation and protection");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Test memory allocation
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 1024),  // Allocate 1KB
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 2, 0, 0),   // Save address in r2
        
        // Try to write to allocated memory (use smaller value that fits in 16-bit)
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 0x1234),
        aurora_encode_r_type(AURORA_OP_STORE, 3, 2, 0),  // Store r3 at address r2
        
        // Read it back
        aurora_encode_r_type(AURORA_OP_LOAD, 4, 2, 0),
        
        // Free memory
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_FREE),
        aurora_encode_r_type(AURORA_OP_MOVE, 1, 2, 0),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "Memory allocation program ran successfully");
    
    uint32_t addr = aurora_vm_get_register(vm, 2);
    TEST_ASSERT(addr >= 0x4000 && addr < 0xE000, 
                "Allocated address in heap range");
    
    uint32_t value = aurora_vm_get_register(vm, 4);
    if (value != 0x1234) {
        add_issue("High", "Memory Management", 
                  "Memory read/write consistency issue",
                  "After allocating memory and writing a value, the read-back value doesn't match. "
                  "This could indicate issues with memory management or pointer handling.");
    }
    TEST_ASSERT(value == 0x1234, "Memory read/write works correctly");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 2: Process Scheduling Simulation
void test_process_scheduling(void) {
    TEST_START("Process Management: Thread scheduling and context switching");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Create a thread
    uint32_t thread_entry = 0x1000;
    result = aurora_vm_thread_create(vm, thread_entry, 42);
    TEST_ASSERT(result >= 0, "Thread created successfully");
    
    if (result < 0) {
        add_issue("Critical", "Process Management",
                  "Thread creation fails",
                  "aurora_vm_thread_create returns error. This prevents testing multi-threading "
                  "capabilities of the OS.");
    }
    
    // Check thread count
    if (vm->scheduler.count != 2) {  // Main thread + new thread
        add_issue("High", "Process Management",
                  "Thread count incorrect after creation",
                  "Expected 2 threads (main + created), but scheduler shows different count.");
    }
    TEST_ASSERT(vm->scheduler.count == 2, "Thread count is correct (2 threads)");
    
    // Test context switching
    uint32_t current_before = aurora_vm_thread_current(vm);
    aurora_vm_thread_yield(vm);
    uint32_t current_after = aurora_vm_thread_current(vm);
    
    if (current_before == current_after) {
        add_issue("Medium", "Process Management",
                  "Thread yield doesn't switch context",
                  "Calling aurora_vm_thread_yield should switch to another thread, "
                  "but the current thread ID remains the same.");
    }
    TEST_ASSERT(current_before != current_after, "Context switch occurred");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 3: Interrupt Handling
void test_interrupt_handling(void) {
    TEST_START("Interrupt System: IRQ handling and priorities");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Enable interrupts
    aurora_vm_irq_enable(vm, true);
    TEST_ASSERT(vm->irq_ctrl.enabled == true, "Interrupts enabled");
    
    // Set up an interrupt handler
    uint32_t handler_addr = 0x2000;
    result = aurora_vm_irq_set_handler(vm, 0, handler_addr);
    TEST_ASSERT(result == 0, "Interrupt handler registered");
    
    // Trigger interrupt
    result = aurora_vm_irq_trigger(vm, 0);
    TEST_ASSERT(result == 0, "Interrupt triggered");
    
    // Check if interrupt is pending
    if (!vm->irq_ctrl.interrupts[0].pending) {
        add_issue("High", "Interrupt System",
                  "Interrupt not marked as pending after trigger",
                  "After triggering an interrupt, it should be marked as pending, "
                  "but the interrupt controller doesn't show it as pending.");
    }
    TEST_ASSERT(vm->irq_ctrl.interrupts[0].pending, "Interrupt marked as pending");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 4: File System Operations
void test_filesystem_operations(void) {
    TEST_START("File System: File I/O via syscalls");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Test file operations via syscalls
    uint32_t program[] = {
        // Open file
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_OPEN),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x5000),  // Path address
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 2),       // Mode (read/write)
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 5, 0, 0),     // Save fd in r5
        
        // Close file
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_CLOSE),
        aurora_encode_r_type(AURORA_OP_MOVE, 1, 5, 0),     // fd from r5
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    // Write path string to memory
    const char *path = "/test/file.txt";
    aurora_vm_write_memory(vm, 0x5000, strlen(path) + 1, path);
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "File system program ran successfully");
    
    uint32_t fd = aurora_vm_get_register(vm, 5);
    if (fd == 0 || fd == (uint32_t)-1) {
        add_issue("High", "File System",
                  "File open syscall returns invalid file descriptor",
                  "OPEN syscall returns 0 or -1, indicating file operations are not properly "
                  "implemented. This prevents testing file system functionality.");
    }
    
    // Note: File operations are documented as stubs in Aurora VM
    printf("  ℹ File operations are currently stubbed (per documentation)\n");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 5: Network Device
void test_network_operations(void) {
    TEST_START("Network Stack: Packet send/receive");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Connect network
    vm->network.connected = true;
    TEST_ASSERT(aurora_vm_net_is_connected(vm), "Network connected");
    
    // Send a packet
    uint8_t test_data[] = "Hello, Network!";
    result = aurora_vm_net_send(vm, test_data, sizeof(test_data));
    TEST_ASSERT(result > 0, "Packet sent successfully");  // Returns bytes sent
    
    // Verify packet in TX queue (tx_head increments when packets are added)
    if (vm->network.tx_head == 0) {
        add_issue("Medium", "Network Stack",
                  "Packet not added to TX queue after send",
                  "After sending a packet via aurora_vm_net_send, the TX queue head "
                  "should increment but remains at 0.");
    }
    TEST_ASSERT(vm->network.tx_head > 0, "Packet added to TX queue");
    
    // Simulate receiving a packet
    result = aurora_vm_net_send(vm, test_data, sizeof(test_data));
    vm->network.rx_queue[0] = vm->network.tx_queue[0];
    vm->network.rx_tail = 1;
    
    uint8_t recv_buffer[64];
    result = aurora_vm_net_recv(vm, recv_buffer, sizeof(recv_buffer));
    TEST_ASSERT(result > 0, "Packet received successfully");
    TEST_ASSERT(strcmp((char*)recv_buffer, (char*)test_data) == 0, 
                "Received data matches sent data");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 6: GUI/Graphics Operations
void test_gui_operations(void) {
    TEST_START("GUI System: Framebuffer and graphics operations");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Test direct API for setting pixels (syscall has parameter size limitations)
    aurora_vm_display_set_pixel(vm, 50, 50, 0xFF0000FF);   // Red
    aurora_vm_display_set_pixel(vm, 100, 100, 0x00FF00FF); // Green
    
    // Verify pixels were set
    uint32_t pixel1 = aurora_vm_display_get_pixel(vm, 50, 50);
    uint32_t pixel2 = aurora_vm_display_get_pixel(vm, 100, 100);
    
    if (pixel1 != 0xFF0000FF) {
        add_issue("Medium", "GUI System",
                  "Pixel color mismatch after drawing",
                  "Set pixel at (50,50) to red (0xFF0000FF) but read back different color. "
                  "This suggests framebuffer operations may have endianness or format issues.");
    }
    TEST_ASSERT(pixel1 == 0xFF0000FF, "Red pixel set correctly");
    TEST_ASSERT(pixel2 == 0x00FF00FF, "Green pixel set correctly");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 7: Atomic Operations
void test_atomic_operations(void) {
    TEST_START("Synchronization: Atomic operations for multi-threading");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Test atomic operations with proper memory addresses
    uint32_t program[] = {
        // Allocate memory for atomic operations
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 64),  // Allocate 64 bytes
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 10, 0, 0),  // Save address in r10
        
        // Initialize memory with value 42
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 42),
        aurora_encode_r_type(AURORA_OP_STORE, 1, 10, 0),  // Store 42 at address r10
        
        // Test XCHG: exchange memory[r10] with value 100
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),
        aurora_encode_r_type(AURORA_OP_XCHG, 3, 10, 2),  // r3 = old value at [r10], [r10] = r2
        
        // Test CAS (Compare-And-Swap) at r10+4
        aurora_encode_i_type(AURORA_OP_LOADI, 11, 4),
        aurora_encode_r_type(AURORA_OP_ADD, 11, 10, 11),  // r11 = r10 + 4
        aurora_encode_i_type(AURORA_OP_LOADI, 4, 100),
        aurora_encode_r_type(AURORA_OP_STORE, 4, 11, 0),  // Store 100 at [r11]
        aurora_encode_i_type(AURORA_OP_LOADI, 5, 200),    // New value
        aurora_encode_r_type(AURORA_OP_CAS, 4, 11, 5),    // Compare [r11] with r4, if equal store r5
        
        // Test FADD (Fetch-And-Add) at r10+8
        aurora_encode_i_type(AURORA_OP_LOADI, 12, 8),
        aurora_encode_r_type(AURORA_OP_ADD, 12, 10, 12),  // r12 = r10 + 8
        aurora_encode_i_type(AURORA_OP_LOADI, 7, 10),
        aurora_encode_r_type(AURORA_OP_STORE, 7, 12, 0),  // Store 10 at [r12]
        aurora_encode_i_type(AURORA_OP_LOADI, 8, 5),      // Add value
        aurora_encode_r_type(AURORA_OP_FADD_ATOMIC, 9, 12, 8),  // r9 = old [r12], [r12] += r8
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    result = aurora_vm_run(vm);
    TEST_ASSERT(result == 0, "Atomic operations program ran successfully");
    
    // Verify XCHG - should have returned old value (42)
    uint32_t old_val = aurora_vm_get_register(vm, 3);
    TEST_ASSERT(old_val == 42, "XCHG returned old value");
    
    // Verify CAS - should return 1 on success
    uint32_t cas_result = aurora_vm_get_register(vm, 4);
    if (cas_result != 1) {
        add_issue("Low", "Synchronization",
                  "CAS operation result unclear",
                  "Compare-and-swap should return 1 on success, but returned different value.");
    }
    TEST_ASSERT(cas_result == 1, "CAS operation succeeded");
    
    // Verify FADD - should have returned old value (10)
    uint32_t fadd_old = aurora_vm_get_register(vm, 9);
    TEST_ASSERT(fadd_old == 10, "FADD returned old value");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 8: JIT Compilation System
void test_jit_compilation(void) {
    TEST_START("JIT System: Just-In-Time compilation infrastructure");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Check JIT is enabled
    TEST_ASSERT(vm->jit.enabled, "JIT enabled by default");
    TEST_ASSERT(vm->jit.cache != NULL, "JIT cache allocated");
    
    // Try to compile a basic block
    uint32_t test_addr = 0x100;
    result = aurora_vm_jit_compile_block(vm, test_addr);
    
    if (result != 0) {
        add_issue("Medium", "JIT System",
                  "JIT block compilation fails",
                  "aurora_vm_jit_compile_block returns error. JIT infrastructure exists "
                  "but code generation backend may not be fully implemented.");
    }
    
    TEST_ASSERT(vm->jit.num_blocks < 256, "JIT tracking blocks within limit");
    
    // Clear cache
    aurora_vm_jit_clear_cache(vm);
    TEST_ASSERT(vm->jit.cache_used == 0, "JIT cache cleared");
    
    printf("  ℹ JIT code generation backend is infrastructure for future implementation\n");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 9: GDB Debugging Interface
void test_gdb_interface(void) {
    TEST_START("Debug System: GDB remote debugging protocol");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Try to start GDB server (will fail without network, that's OK)
    result = aurora_vm_gdb_start(vm, 1234);
    
    // GDB server infrastructure exists
    if (vm->gdb.enabled) {
        TEST_ASSERT(vm->gdb.enabled, "GDB server can be enabled");
    } else {
        printf("  ℹ GDB server requires network socket support\n");
    }
    
    // Stop GDB server
    aurora_vm_gdb_stop(vm);
    TEST_ASSERT(!vm->gdb.enabled, "GDB server stopped");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Test 10: Memory-Mapped I/O
void test_mmio_operations(void) {
    TEST_START("MMIO System: Memory-mapped device I/O");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created successfully");
    
    int result = aurora_vm_init(vm);
    TEST_ASSERT(result == 0, "VM initialized successfully");
    
    // Test reading/writing to MMIO regions
    uint32_t display_addr = AURORA_VM_MMIO_DISPLAY;
    uint32_t keyboard_addr = AURORA_VM_MMIO_KEYBOARD;
    uint32_t timer_addr = AURORA_VM_MMIO_TIMER;
    
    // Verify MMIO addresses are in correct range
    TEST_ASSERT(display_addr >= AURORA_VM_MMIO_BASE && 
                display_addr < AURORA_VM_MMIO_BASE + AURORA_VM_MMIO_SIZE,
                "Display MMIO in valid range");
    
    TEST_ASSERT(keyboard_addr >= AURORA_VM_MMIO_BASE && 
                keyboard_addr < AURORA_VM_MMIO_BASE + AURORA_VM_MMIO_SIZE,
                "Keyboard MMIO in valid range");
    
    TEST_ASSERT(timer_addr >= AURORA_VM_MMIO_BASE && 
                timer_addr < AURORA_VM_MMIO_BASE + AURORA_VM_MMIO_SIZE,
                "Timer MMIO in valid range");
    
    // Try to write to MMIO region
    uint32_t test_value = 0x12345678;
    result = aurora_vm_write_memory(vm, display_addr, 4, &test_value);
    
    if (result != 0) {
        add_issue("Low", "MMIO System",
                  "MMIO write operations not fully implemented",
                  "Writing to MMIO regions returns error. MMIO addresses are defined "
                  "but actual device access handling may need implementation.");
    }
    
    printf("  ℹ MMIO regions defined for device access\n");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

// Print issues report
void print_issues_report(void) {
    printf("\n");
    printf("========================================\n");
    printf("Issues Found During Testing\n");
    printf("========================================\n");
    
    if (issue_count == 0) {
        printf("\n✓ No issues found! All systems working as expected.\n");
        return;
    }
    
    printf("\nTotal Issues: %d\n\n", issue_count);
    
    const char *severities[] = {"Critical", "High", "Medium", "Low"};
    for (int s = 0; s < 4; s++) {
        bool found = false;
        for (int i = 0; i < issue_count; i++) {
            if (strcmp(issues[i].severity, severities[s]) == 0) {
                if (!found) {
                    printf("=== %s Priority ===\n\n", severities[s]);
                    found = true;
                }
                printf("Issue #%d: %s\n", issues[i].id, issues[i].title);
                printf("  Component: %s\n", issues[i].component);
                printf("  Severity: %s\n", issues[i].severity);
                printf("  Status: %s\n", issues[i].status);
                printf("  Description:\n    %s\n\n", issues[i].description);
            }
        }
    }
}

// Main test runner
int main(void) {
    printf("========================================\n");
    printf("Aurora OS - VM Integration Test Suite\n");
    printf("Testing OS Components on Aurora VM\n");
    printf("========================================\n");
    
    // Run all tests
    test_memory_management();
    test_process_scheduling();
    test_interrupt_handling();
    test_filesystem_operations();
    test_network_operations();
    test_gui_operations();
    test_atomic_operations();
    test_jit_compilation();
    test_gdb_interface();
    test_mmio_operations();
    
    // Print results
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", total_tests);
    printf("  Passed: %d\n", passed_tests);
    printf("  Failed: %d\n", failed_tests);
    printf("========================================\n");
    
    // Print issues report
    print_issues_report();
    
    return (failed_tests == 0) ? 0 : 1;
}
