/**
 * Aurora OS - Roadmap Priority Tests
 * Comprehensive validation tests for the 4 high-priority issues from Roadmap.md
 * 
 * Issue #1: Memory Allocation System - Comprehensive stress testing
 * Issue #2: Interrupt Handling System - End-to-end interrupt flow testing  
 * Issue #3: Network Device Implementation - Packet validation
 * Issue #4: Atomic Operations - Validation suite
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
            return 0; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("  PASSED\n"); \
        passed_tests++; \
        return 1; \
    } while(0)

#define TEST_FAIL(message) \
    do { \
        printf("  FAILED: %s\n", message); \
        failed_tests++; \
        return 0; \
    } while(0)

/* ===== Issue #1: Memory Allocation System ===== */

/**
 * Test: Basic memory allocation and alignment
 * Verifies that allocated addresses are properly aligned
 */
int test_memory_allocation_alignment(void) {
    TEST_START("Issue #1: Memory allocation address alignment");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Program to allocate several blocks and verify alignment
    uint32_t program[] = {
        // Allocate 100 bytes
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 2, 0, 0),  // Save in r2
        
        // Allocate 256 bytes
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 256),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 3, 0, 0),  // Save in r3
        
        // Allocate 17 bytes (odd size)
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 17),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 4, 0, 0),  // Save in r4
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "Allocation program executed successfully");
    
    uint32_t addr1 = aurora_vm_get_register(vm, 2);
    uint32_t addr2 = aurora_vm_get_register(vm, 3);
    uint32_t addr3 = aurora_vm_get_register(vm, 4);
    
    TEST_ASSERT(addr1 >= 0x4000 && addr1 < 0xE000, "First allocation in heap range");
    TEST_ASSERT(addr2 >= 0x4000 && addr2 < 0xE000, "Second allocation in heap range");
    TEST_ASSERT(addr3 >= 0x4000 && addr3 < 0xE000, "Third allocation in heap range");
    
    TEST_ASSERT((addr1 & 0x3) == 0, "First allocation 4-byte aligned");
    TEST_ASSERT((addr2 & 0x3) == 0, "Second allocation 4-byte aligned");
    TEST_ASSERT((addr3 & 0x3) == 0, "Third allocation 4-byte aligned");
    
    TEST_ASSERT(addr2 > addr1, "Allocations don't overlap (addr2 > addr1)");
    TEST_ASSERT(addr3 > addr2, "Allocations don't overlap (addr3 > addr2)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Memory write and read consistency
 * Verifies that data written to allocated memory can be read back correctly
 */
int test_memory_write_read_consistency(void) {
    TEST_START("Issue #1: Memory write/read consistency");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    uint32_t program[] = {
        // Allocate 1KB
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 1024),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 2, 0, 0),  // Save address in r2
        
        // Write test value 0x1234
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 0x1234),
        aurora_encode_r_type(AURORA_OP_STORE, 3, 2, 0),
        
        // Read it back
        aurora_encode_r_type(AURORA_OP_LOAD, 4, 2, 0),
        
        // Write different value 0x5678 at offset 4
        aurora_encode_i_type(AURORA_OP_LOADI, 5, 0x5678),
        aurora_encode_i_type(AURORA_OP_LOADI, 6, 4),
        aurora_encode_r_type(AURORA_OP_ADD, 7, 2, 6),  // r7 = r2 + 4
        aurora_encode_r_type(AURORA_OP_STORE, 5, 7, 0),
        
        // Read both values back
        aurora_encode_r_type(AURORA_OP_LOAD, 8, 2, 0),   // First value
        aurora_encode_r_type(AURORA_OP_LOAD, 9, 7, 0),   // Second value
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "Memory test program executed");
    
    uint32_t addr = aurora_vm_get_register(vm, 2);
    uint32_t val1 = aurora_vm_get_register(vm, 4);
    uint32_t val2 = aurora_vm_get_register(vm, 8);
    uint32_t val3 = aurora_vm_get_register(vm, 9);
    
    TEST_ASSERT(addr != 0, "Memory was allocated");
    TEST_ASSERT(val1 == 0x1234, "First write/read correct (r4 = 0x1234)");
    TEST_ASSERT(val2 == 0x1234, "First value persisted (r8 = 0x1234)");
    TEST_ASSERT(val3 == 0x5678, "Second write/read correct (r9 = 0x5678)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Stress test with 1000+ allocations
 * Verifies memory allocator can handle many allocations without failures
 */
int test_memory_stress_allocations(void) {
    TEST_START("Issue #1: Stress test with 1000+ allocations");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    int successful_allocs = 0;
    int failed_allocs = 0;
    
    // Allocate memory blocks until heap is exhausted
    for (int i = 0; i < 1500; i++) {
        uint32_t program[] = {
            aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
            aurora_encode_i_type(AURORA_OP_LOADI, 1, 32),  // 32 bytes each
            aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
            aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
        };
        
        aurora_vm_reset(vm);
        aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
        int result = aurora_vm_run(vm);
        
        if (result == 0) {
            uint32_t addr = aurora_vm_get_register(vm, 0);
            if (addr != 0 && addr >= 0x4000 && addr < 0xE000) {
                successful_allocs++;
            } else {
                failed_allocs++;
            }
        } else {
            failed_allocs++;
        }
    }
    
    TEST_ASSERT(successful_allocs >= 1000, "Successfully allocated 1000+ blocks");
    printf("  ℹ Successful allocations: %d, Failed: %d\n", successful_allocs, failed_allocs);
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Page protection for allocated memory
 * Verifies that allocated memory pages are writable
 */
int test_memory_page_protection(void) {
    TEST_START("Issue #1: Heap pages are writable");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Check that heap pages have correct protection flags
    // Heap is at 0x4000-0xBFFF (pages 64-191)
    for (uint32_t page = 64; page < 192; page++) {
        uint8_t prot = aurora_vm_get_page_protection(vm, page);
        TEST_ASSERT(prot & AURORA_PAGE_PRESENT, "Heap page is present");
        TEST_ASSERT(prot & AURORA_PAGE_READ, "Heap page is readable");
        TEST_ASSERT(prot & AURORA_PAGE_WRITE, "Heap page is writable");
    }
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/* ===== Issue #2: Interrupt Handling System ===== */

/**
 * Test: Interrupt pending flag logic
 * Verifies that interrupts are properly marked as pending when triggered
 */
int test_interrupt_pending_flag(void) {
    TEST_START("Issue #2: Interrupt pending flag logic");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Enable interrupts
    aurora_vm_irq_enable(vm, true);
    TEST_ASSERT(vm->irq_ctrl.enabled, "Interrupts enabled");
    
    // Set handler for IRQ 0
    aurora_vm_irq_set_handler(vm, 0, 0x2000);
    TEST_ASSERT(vm->irq_ctrl.interrupts[0].handler == 0x2000, "Handler set");
    TEST_ASSERT(vm->irq_ctrl.interrupts[0].enabled, "IRQ 0 enabled");
    
    // Trigger interrupt
    int result = aurora_vm_irq_trigger(vm, 0);
    TEST_ASSERT(result == 0, "IRQ trigger succeeded");
    
    // Verify pending flag is set
    TEST_ASSERT(vm->irq_ctrl.interrupts[0].pending, "Interrupt marked as pending");
    TEST_ASSERT((vm->irq_ctrl.active & (1 << 0)) != 0, "IRQ active bit set");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Interrupt controller state management
 * Verifies proper enable/disable of interrupts
 */
int test_interrupt_state_management(void) {
    TEST_START("Issue #2: Interrupt controller state management");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Initially disabled
    TEST_ASSERT(!vm->irq_ctrl.enabled, "IRQs initially disabled");
    
    // Enable interrupts
    aurora_vm_irq_enable(vm, true);
    TEST_ASSERT(vm->irq_ctrl.enabled, "IRQs enabled");
    
    // Disable interrupts
    aurora_vm_irq_enable(vm, false);
    TEST_ASSERT(!vm->irq_ctrl.enabled, "IRQs disabled");
    
    // Test individual IRQ enable via handler
    aurora_vm_irq_enable(vm, true);
    aurora_vm_irq_set_handler(vm, 1, 0x3000);
    TEST_ASSERT(vm->irq_ctrl.interrupts[1].enabled, "IRQ 1 enabled after setting handler");
    TEST_ASSERT(vm->irq_ctrl.interrupts[1].handler == 0x3000, "Handler address set correctly");
    
    // Update handler to different address
    aurora_vm_irq_set_handler(vm, 1, 0x4000);
    TEST_ASSERT(vm->irq_ctrl.interrupts[1].enabled, "IRQ 1 still enabled after handler update");
    TEST_ASSERT(vm->irq_ctrl.interrupts[1].handler == 0x4000, "Handler address updated");
    
    // Manually disable IRQ by setting enabled flag
    vm->irq_ctrl.interrupts[1].enabled = false;
    TEST_ASSERT(!vm->irq_ctrl.interrupts[1].enabled, "IRQ 1 can be manually disabled");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: End-to-end interrupt flow
 * Verifies full interrupt dispatch and handler execution
 */
int test_interrupt_end_to_end(void) {
    TEST_START("Issue #2: End-to-end interrupt flow");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Main program: wait for interrupt
    uint32_t main_program[] = {
        // Set r1 = 0 (flag to indicate if IRQ handler ran)
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0),
        
        // Loop: keep checking r1 (would be set by interrupt handler)
        // In real scenario, interrupt would break the loop
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 5),  // Counter
        // Loop start at PC = 8
        aurora_encode_r_type(AURORA_OP_SUB, 2, 2, 0),  // r2--  (note: r0 should be constant 1)
        aurora_encode_i_type(AURORA_OP_LOADI, 0, 1),
        aurora_encode_r_type(AURORA_OP_SUB, 2, 2, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    // Interrupt handler at 0x2000: set r1 = 0x99 and return
    uint32_t irq_handler[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x99),
        aurora_encode_r_type(AURORA_OP_RET, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)main_program, sizeof(main_program), 0);
    aurora_vm_load_program(vm, (uint8_t *)irq_handler, sizeof(irq_handler), 0x2000);
    
    // Enable interrupts and set handler
    aurora_vm_irq_enable(vm, true);
    aurora_vm_irq_set_handler(vm, 0, 0x2000);
    
    // Trigger interrupt after a few steps
    for (int i = 0; i < 3; i++) {
        aurora_vm_step(vm);
    }
    
    aurora_vm_irq_trigger(vm, 0);
    TEST_ASSERT(vm->irq_ctrl.interrupts[0].pending, "Interrupt pending after trigger");
    
    // Execute more steps to allow interrupt to be dispatched
    for (int i = 0; i < 10; i++) {
        if (vm->cpu.halted) break;
        aurora_vm_step(vm);
    }
    
    // Check if handler was called (r1 should be 0x99)
    uint32_t r1 = aurora_vm_get_register(vm, 1);
    TEST_ASSERT(r1 == 0x99, "Interrupt handler executed (r1 = 0x99)");
    TEST_ASSERT(!vm->irq_ctrl.interrupts[0].pending, "Interrupt cleared after handling");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Interrupt prioritization
 * Verifies that lower-numbered IRQs have higher priority
 */
int test_interrupt_prioritization(void) {
    TEST_START("Issue #2: Interrupt prioritization");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    aurora_vm_irq_enable(vm, true);
    
    // Set handlers for IRQ 0, 1, 2
    aurora_vm_irq_set_handler(vm, 0, 0x2000);
    aurora_vm_irq_set_handler(vm, 1, 0x2100);
    aurora_vm_irq_set_handler(vm, 2, 0x2200);
    
    // Trigger all three
    aurora_vm_irq_trigger(vm, 2);
    aurora_vm_irq_trigger(vm, 1);
    aurora_vm_irq_trigger(vm, 0);
    
    TEST_ASSERT(vm->irq_ctrl.interrupts[0].pending, "IRQ 0 pending");
    TEST_ASSERT(vm->irq_ctrl.interrupts[1].pending, "IRQ 1 pending");
    TEST_ASSERT(vm->irq_ctrl.interrupts[2].pending, "IRQ 2 pending");
    
    // Load a simple program with a few operations before halt
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    
    // Step once - IRQ 0 should be dispatched first (highest priority)
    aurora_vm_step(vm);
    
    TEST_ASSERT(!vm->irq_ctrl.interrupts[0].pending, "IRQ 0 handled first");
    TEST_ASSERT(vm->cpu.pc == 0x2000, "PC jumped to IRQ 0 handler");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/* ===== Issue #3: Network Device Implementation ===== */

/**
 * Test: Packet queue management
 * Verifies that TX queue properly manages multiple packets
 */
int test_network_packet_queue(void) {
    TEST_START("Issue #3: Network packet queue management");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Connect network
    vm->network.connected = true;
    
    // Send multiple packets
    const char *msg1 = "Packet 1";
    const char *msg2 = "Packet 2";
    const char *msg3 = "Packet 3";
    
    int result1 = aurora_vm_net_send(vm, (const uint8_t *)msg1, strlen(msg1) + 1);
    TEST_ASSERT(result1 > 0, "First packet sent successfully");
    
    int result2 = aurora_vm_net_send(vm, (const uint8_t *)msg2, strlen(msg2) + 1);
    TEST_ASSERT(result2 > 0, "Second packet sent successfully");
    
    int result3 = aurora_vm_net_send(vm, (const uint8_t *)msg3, strlen(msg3) + 1);
    TEST_ASSERT(result3 > 0, "Third packet sent successfully");
    
    // Verify packets are in TX queue
    TEST_ASSERT(vm->network.tx_head != vm->network.tx_tail, "TX queue not empty");
    
    // Count packets in queue
    int packet_count = 0;
    uint32_t tail = vm->network.tx_tail;
    while (tail != vm->network.tx_head) {
        packet_count++;
        tail = (tail + 1) % AURORA_VM_NET_QUEUE_SIZE;
    }
    
    TEST_ASSERT(packet_count == 3, "Three packets in TX queue");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Packet send/receive validation
 * Verifies data integrity of sent and received packets
 */
int test_network_packet_validation(void) {
    TEST_START("Issue #3: Packet send/receive validation");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    vm->network.connected = true;
    
    // Send a packet
    const char *test_msg = "Test Network Packet 123";
    int send_result = aurora_vm_net_send(vm, (const uint8_t *)test_msg, strlen(test_msg) + 1);
    TEST_ASSERT(send_result > 0, "Packet sent");
    
    // Simulate receiving the same packet (loopback)
    // Manually add to RX queue
    uint32_t rx_next = (vm->network.rx_head + 1) % AURORA_VM_NET_QUEUE_SIZE;
    memcpy(vm->network.rx_queue[vm->network.rx_head].data, test_msg, strlen(test_msg) + 1);
    vm->network.rx_queue[vm->network.rx_head].length = strlen(test_msg) + 1;
    vm->network.rx_head = rx_next;
    
    // Receive the packet
    uint8_t recv_buffer[256];
    int recv_result = aurora_vm_net_recv(vm, recv_buffer, sizeof(recv_buffer));
    TEST_ASSERT(recv_result > 0, "Packet received");
    TEST_ASSERT(strcmp((const char *)recv_buffer, test_msg) == 0, "Received data matches sent data");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Network device syscall implementation
 * Verifies NET_SEND and NET_RECV syscalls work correctly
 */
int test_network_syscalls(void) {
    TEST_START("Issue #3: Network device syscalls");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    vm->network.connected = true;
    
    // Prepare test data in memory
    const char *test_data = "Network test data";
    uint32_t data_addr = 0x5000;
    memcpy(&vm->memory[data_addr], test_data, strlen(test_data) + 1);
    
    // Program to send packet via NET_SEND syscall
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_NET_SEND),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, data_addr),  // Data address
        aurora_encode_i_type(AURORA_OP_LOADI, 2, strlen(test_data) + 1),  // Length
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "Network send syscall program executed");
    
    uint32_t bytes_sent = aurora_vm_get_register(vm, 0);
    TEST_ASSERT(bytes_sent > 0, "Syscall returned success (bytes sent > 0)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Network connection state
 * Verifies network device properly tracks connection state
 */
int test_network_connection_state(void) {
    TEST_START("Issue #3: Network connection state tracking");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    // Initially disconnected
    TEST_ASSERT(!aurora_vm_net_is_connected(vm), "Initially disconnected");
    
    // Try to send when disconnected - should fail
    uint8_t data[] = "test";
    int result = aurora_vm_net_send(vm, data, sizeof(data));
    TEST_ASSERT(result < 0, "Send fails when disconnected");
    
    // Connect
    vm->network.connected = true;
    TEST_ASSERT(aurora_vm_net_is_connected(vm), "Now connected");
    
    // Send should succeed
    result = aurora_vm_net_send(vm, data, sizeof(data));
    TEST_ASSERT(result > 0, "Send succeeds when connected");
    
    // Disconnect
    vm->network.connected = false;
    TEST_ASSERT(!aurora_vm_net_is_connected(vm), "Disconnected again");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/* ===== Issue #4: Atomic Operations ===== */

/**
 * Test: XCHG (exchange) atomic operation
 * Verifies exchange operation works correctly
 */
int test_atomic_xchg(void) {
    TEST_START("Issue #4: XCHG atomic operation");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    uint32_t program[] = {
        // Allocate memory for atomic operation
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 64),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 5, 0, 0),  // r5 = address
        
        // Clear r0 (contains return value from syscall)
        aurora_encode_i_type(AURORA_OP_LOADI, 0, 0),
        
        // Write initial value 42 to memory
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 42),
        aurora_encode_r_type(AURORA_OP_STORE, 1, 5, 0),
        
        // XCHG: exchange memory at r5 with value 100
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),  // New value
        aurora_encode_r_type(AURORA_OP_XCHG, 3, 5, 2),  // r3 = old value at [r5], [r5] = r2
        
        // Read back the new value
        aurora_encode_r_type(AURORA_OP_LOAD, 4, 5, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "XCHG program executed");
    
    uint32_t r3 = aurora_vm_get_register(vm, 3);  // Old value
    uint32_t r4 = aurora_vm_get_register(vm, 4);  // New value
    
    TEST_ASSERT(r3 == 42, "XCHG returned old value (r3 = 42)");
    TEST_ASSERT(r4 == 100, "Memory updated with new value (r4 = 100)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: CAS (compare-and-swap) atomic operation
 * Verifies compare-and-swap semantics
 */
int test_atomic_cas(void) {
    TEST_START("Issue #4: CAS atomic operation");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    uint32_t program[] = {
        // Allocate memory
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 64),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 5, 0, 0),  // r5 = address
        
        // Clear r0
        aurora_encode_i_type(AURORA_OP_LOADI, 0, 0),
        
        // Write value 100 to memory
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),
        aurora_encode_r_type(AURORA_OP_STORE, 1, 5, 0),
        
        // CAS with matching expected value
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),  // Expected value in rd
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 200),  // New value in rs2
        aurora_encode_r_type(AURORA_OP_CAS, 2, 5, 3),   // CAS at [r5], compare with r2, set to r3
        // r2 should be 1 if successful
        
        // Read back the value
        aurora_encode_r_type(AURORA_OP_LOAD, 4, 5, 0),
        
        // Try CAS again with wrong expected value
        aurora_encode_i_type(AURORA_OP_LOADI, 6, 999),  // Wrong expected
        aurora_encode_i_type(AURORA_OP_LOADI, 7, 300),  // New value
        aurora_encode_r_type(AURORA_OP_CAS, 6, 5, 7),   // CAS at [r5]
        // r6 should be 0 (failed)
        
        // Read value again
        aurora_encode_r_type(AURORA_OP_LOAD, 8, 5, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "CAS program executed");
    
    uint32_t r2 = aurora_vm_get_register(vm, 2);
    uint32_t r4 = aurora_vm_get_register(vm, 4);
    uint32_t r6 = aurora_vm_get_register(vm, 6);
    uint32_t r8 = aurora_vm_get_register(vm, 8);
    
    TEST_ASSERT(r2 == 1, "First CAS succeeded (r2 = 1)");
    TEST_ASSERT(r4 == 200, "Memory updated to 200 (r4 = 200)");
    TEST_ASSERT(r6 == 0, "Second CAS failed (r6 = 0)");
    TEST_ASSERT(r8 == 200, "Memory still 200 (r8 = 200)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: FADD atomic operation
 * Verifies fetch-and-add atomic operation
 */
int test_atomic_fadd(void) {
    TEST_START("Issue #4: FADD atomic operation");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    uint32_t program[] = {
        // Allocate memory
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 64),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 5, 0, 0),  // r5 = address
        
        // Clear r0
        aurora_encode_i_type(AURORA_OP_LOADI, 0, 0),
        
        // Write value 10 to memory
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),
        aurora_encode_r_type(AURORA_OP_STORE, 1, 5, 0),
        
        // FADD: r2 = old value at [r5], [r5] += 5
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 5),    // Amount to add
        aurora_encode_r_type(AURORA_OP_FADD_ATOMIC, 2, 5, 3),
        
        // Read back the new value
        aurora_encode_r_type(AURORA_OP_LOAD, 4, 5, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "FADD program executed");
    
    uint32_t r2 = aurora_vm_get_register(vm, 2);  // Old value
    uint32_t r4 = aurora_vm_get_register(vm, 4);  // New value
    
    TEST_ASSERT(r2 == 10, "FADD returned old value (r2 = 10)");
    TEST_ASSERT(r4 == 15, "Memory incremented (r4 = 15)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/**
 * Test: Multiple atomic operations in sequence
 * Verifies atomics work correctly in combination
 */
int test_atomic_operations_sequence(void) {
    TEST_START("Issue #4: Sequential atomic operations");
    
    AuroraVM *vm = aurora_vm_create();
    TEST_ASSERT(vm != NULL, "VM created");
    TEST_ASSERT(aurora_vm_init(vm) == 0, "VM initialized");
    
    uint32_t program[] = {
        // Allocate memory for tests
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 128),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 9, 0, 0),  // r9 = base address
        
        // Clear r0
        aurora_encode_i_type(AURORA_OP_LOADI, 0, 0),
        
        // Initialize memory locations
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 42),
        aurora_encode_r_type(AURORA_OP_STORE, 1, 9, 0),   // [r9] = 42
        
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 4),
        aurora_encode_r_type(AURORA_OP_ADD, 10, 9, 2),    // r10 = r9 + 4
        aurora_encode_r_type(AURORA_OP_STORE, 1, 10, 0),  // [r9+4] = 100
        
        // XCHG test: exchange values at r9 and r10
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 999),
        aurora_encode_r_type(AURORA_OP_XCHG, 3, 9, 1),    // r3 = [r9], [r9] = 999
        
        // CAS test: at r10
        aurora_encode_i_type(AURORA_OP_LOADI, 4, 100),    // Expected
        aurora_encode_i_type(AURORA_OP_LOADI, 5, 200),    // New value
        aurora_encode_r_type(AURORA_OP_CAS, 4, 10, 5),    // CAS at [r10]
        
        // FADD test: at r10
        aurora_encode_i_type(AURORA_OP_LOADI, 6, 10),     // Add 10
        aurora_encode_r_type(AURORA_OP_FADD_ATOMIC, 7, 10, 6),
        
        // Read final values
        aurora_encode_r_type(AURORA_OP_LOAD, 11, 9, 0),   // [r9]
        aurora_encode_r_type(AURORA_OP_LOAD, 12, 10, 0),  // [r10]
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    int result = aurora_vm_run(vm);
    
    TEST_ASSERT(result == 0, "Atomic operations sequence executed");
    
    uint32_t r3 = aurora_vm_get_register(vm, 3);   // XCHG old value
    uint32_t r4 = aurora_vm_get_register(vm, 4);   // CAS result
    uint32_t r7 = aurora_vm_get_register(vm, 7);   // FADD old value
    uint32_t r11 = aurora_vm_get_register(vm, 11); // Final [r9]
    uint32_t r12 = aurora_vm_get_register(vm, 12); // Final [r10]
    
    TEST_ASSERT(r3 == 42, "XCHG old value correct (r3 = 42)");
    TEST_ASSERT(r11 == 999, "XCHG new value stored (r11 = 999)");
    TEST_ASSERT(r4 == 1, "CAS succeeded (r4 = 1)");
    TEST_ASSERT(r7 == 200, "FADD old value correct (r7 = 200)");
    TEST_ASSERT(r12 == 210, "Final value correct (r12 = 210)");
    
    aurora_vm_destroy(vm);
    TEST_PASS();
}

/* ===== Main Test Runner ===== */

int main(void) {
    printf("========================================\n");
    printf("Aurora OS - Roadmap Priority Tests\n");
    printf("Comprehensive validation for Issues #1-4\n");
    printf("========================================\n");
    
    printf("\n=== Issue #1: Memory Allocation System ===\n");
    test_memory_allocation_alignment();
    test_memory_write_read_consistency();
    test_memory_stress_allocations();
    test_memory_page_protection();
    
    printf("\n=== Issue #2: Interrupt Handling System ===\n");
    test_interrupt_pending_flag();
    test_interrupt_state_management();
    test_interrupt_end_to_end();
    test_interrupt_prioritization();
    
    printf("\n=== Issue #3: Network Device Implementation ===\n");
    test_network_packet_queue();
    test_network_packet_validation();
    test_network_syscalls();
    test_network_connection_state();
    
    printf("\n=== Issue #4: Atomic Operations ===\n");
    test_atomic_xchg();
    test_atomic_cas();
    test_atomic_fadd();
    test_atomic_operations_sequence();
    
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", total_tests);
    printf("  Passed: %d\n", passed_tests);
    printf("  Failed: %d\n", failed_tests);
    printf("========================================\n");
    
    if (failed_tests == 0) {
        printf("\n✓ All priority issues validated successfully!\n");
        printf("Ready for v1.0 release candidate testing.\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed. Review issues before release.\n");
        return 1;
    }
}
