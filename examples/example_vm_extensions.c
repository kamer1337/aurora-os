/**
 * @file example_vm_extensions.c
 * @brief Aurora VM - Test new extensions (JIT, interrupts, threads, network, atomic ops)
 * 
 * This example demonstrates all the new features added to Aurora VM:
 * - Instruction set extensions (floating-point, SIMD, atomic operations)
 * - Memory-mapped device I/O
 * - Interrupt support
 * - Multi-threading/SMP support
 * - Network device emulation
 * - GDB remote debugging protocol
 * - JIT compilation infrastructure
 */

#include "../include/platform/aurora_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper macros */
#define TEST_START(name) \
    printf("\n[TEST] %s\n", name);

#define TEST_ASSERT(condition, message) \
    if (condition) { \
        printf("  ✓ %s\n", message); \
        tests_passed++; \
    } else { \
        printf("  ✗ %s\n", message); \
        tests_failed++; \
    }

#define TEST_SECTION(name) \
    printf("\n=== %s ===\n", name);

/* Test 1: Atomic Operations */
void test_atomic_operations(void) {
    TEST_START("Atomic Operations (XCHG, CAS, FADD_ATOMIC)");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Test XCHG (atomic exchange) */
    uint32_t xchg_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x4000),  /* r1 = heap address */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 42),      /* r2 = 42 */
        aurora_encode_r_type(AURORA_OP_STORE, 2, 1, 0),    /* mem[r1] = 42 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),     /* r2 = 100 */
        aurora_encode_r_type(AURORA_OP_XCHG, 3, 1, 2),     /* r3 = mem[r1], mem[r1] = r2 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)xchg_program, sizeof(xchg_program), 0);
    aurora_vm_run(vm);
    
    TEST_ASSERT(aurora_vm_get_register(vm, 3) == 42, "XCHG returned old value (42)");
    
    uint32_t mem_value = 0;
    aurora_vm_read_memory(vm, 0x4000, 4, &mem_value);
    TEST_ASSERT(mem_value == 100, "XCHG stored new value (100)");
    
    /* Test CAS (compare-and-swap) */
    aurora_vm_reset(vm);
    uint32_t cas_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x4000),  /* r1 = heap address */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 50),      /* r2 = 50 */
        aurora_encode_r_type(AURORA_OP_STORE, 2, 1, 0),    /* mem[r1] = 50 */
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 50),      /* r3 = 50 (expected) */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 75),      /* r2 = 75 (new value) */
        aurora_encode_r_type(AURORA_OP_CAS, 3, 1, 2),      /* if mem[r1]==r3 then mem[r1]=r2 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)cas_program, sizeof(cas_program), 0);
    aurora_vm_run(vm);
    
    TEST_ASSERT(aurora_vm_get_register(vm, 3) == 1, "CAS succeeded (returned 1)");
    
    /* Test FADD_ATOMIC (fetch-and-add) */
    aurora_vm_reset(vm);
    uint32_t fadd_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x4000),  /* r1 = heap address */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 10),      /* r2 = 10 */
        aurora_encode_r_type(AURORA_OP_STORE, 2, 1, 0),    /* mem[r1] = 10 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 5),       /* r2 = 5 */
        aurora_encode_r_type(AURORA_OP_FADD_ATOMIC, 3, 1, 2), /* r3 = mem[r1], mem[r1] += r2 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)fadd_program, sizeof(fadd_program), 0);
    aurora_vm_run(vm);
    
    TEST_ASSERT(aurora_vm_get_register(vm, 3) == 10, "FADD_ATOMIC returned old value (10)");
    
    aurora_vm_read_memory(vm, 0x4000, 4, &mem_value);
    TEST_ASSERT(mem_value == 15, "FADD_ATOMIC added value (15)");
    
    aurora_vm_destroy(vm);
}

/* Test 2: Interrupt Handling */
void test_interrupts(void) {
    TEST_START("Interrupt Handling");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Set up interrupt handler at address 0x100 */
    uint32_t handler_program[] = {
        /* Handler increments r5 and returns */
        aurora_encode_i_type(AURORA_OP_LOADI, 5, 1),       /* r5 = 1 */
        aurora_encode_r_type(AURORA_OP_ADD, 5, 5, 5),      /* r5 += r5 (r5 = 2) */
        aurora_encode_r_type(AURORA_OP_RET, 0, 0, 0),      /* Return from interrupt */
    };
    aurora_vm_load_program(vm, (uint8_t *)handler_program, sizeof(handler_program), 0x100);
    
    /* Set up interrupt handler */
    aurora_vm_irq_set_handler(vm, AURORA_VM_IRQ_TIMER, 0x100);
    aurora_vm_irq_enable(vm, true);
    
    TEST_ASSERT(vm->irq_ctrl.enabled == true, "Interrupts enabled");
    TEST_ASSERT(vm->irq_ctrl.interrupts[AURORA_VM_IRQ_TIMER].handler == 0x100, "Handler registered");
    
    /* Main program */
    uint32_t main_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 4, 0),       /* r4 = 0 */
        aurora_encode_i_type(AURORA_OP_LOADI, 5, 0),       /* r5 = 0 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    aurora_vm_load_program(vm, (uint8_t *)main_program, sizeof(main_program), 0);
    
    /* Run and trigger interrupt */
    aurora_vm_step(vm);  /* Execute first instruction */
    aurora_vm_irq_trigger(vm, AURORA_VM_IRQ_TIMER);  /* Trigger timer interrupt */
    
    /* Interrupt should be pending after trigger, then cleared on next step */
    TEST_ASSERT(vm->irq_ctrl.interrupts[AURORA_VM_IRQ_TIMER].pending == true, "Interrupt pending after trigger");
    aurora_vm_step(vm);  /* This step will dispatch the interrupt */
    TEST_ASSERT(vm->irq_ctrl.interrupts[AURORA_VM_IRQ_TIMER].pending == false, "Interrupt handled after step");
    
    aurora_vm_destroy(vm);
}

/* Test 3: Network Device */
void test_network_device(void) {
    TEST_START("Network Device Emulation");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Connect network */
    vm->network.connected = true;
    TEST_ASSERT(aurora_vm_net_is_connected(vm), "Network connected");
    
    /* Send a packet */
    uint8_t send_data[] = "Hello, Network!";
    int sent = aurora_vm_net_send(vm, send_data, sizeof(send_data));
    TEST_ASSERT(sent == sizeof(send_data), "Packet sent successfully");
    TEST_ASSERT(vm->network.tx_head == 1, "TX queue updated");
    
    /* Simulate receiving a packet by adding to RX queue */
    uint8_t recv_data[] = "Response from server";
    memcpy(vm->network.rx_queue[0].data, recv_data, sizeof(recv_data));
    vm->network.rx_queue[0].length = sizeof(recv_data);
    vm->network.rx_head = 1;
    
    /* Receive the packet */
    uint8_t buffer[64] = {0};
    int received = aurora_vm_net_recv(vm, buffer, sizeof(buffer));
    TEST_ASSERT(received == sizeof(recv_data), "Packet received successfully");
    TEST_ASSERT(strcmp((char *)buffer, (char *)recv_data) == 0, "Received correct data");
    
    aurora_vm_destroy(vm);
}

/* Test 4: Multi-threading */
void test_multithreading(void) {
    TEST_START("Multi-threading Support");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    TEST_ASSERT(vm->scheduler.count == 1, "Main thread initialized");
    TEST_ASSERT(vm->scheduler.current == 0, "Current thread is main");
    
    /* Create a new thread */
    int tid = aurora_vm_thread_create(vm, 0x1000, 42);
    TEST_ASSERT(tid == 1, "Thread created with ID 1");
    TEST_ASSERT(vm->scheduler.count == 2, "Thread count is 2");
    TEST_ASSERT(vm->scheduler.threads[1].active == true, "New thread is active");
    TEST_ASSERT(vm->scheduler.threads[1].registers[1] == 42, "Thread argument passed");
    
    /* Test thread switching */
    uint32_t current_before = aurora_vm_thread_current(vm);
    aurora_vm_thread_yield(vm);
    uint32_t current_after = aurora_vm_thread_current(vm);
    TEST_ASSERT(current_after != current_before, "Thread switched on yield");
    
    /* Create more threads */
    aurora_vm_thread_create(vm, 0x2000, 100);
    aurora_vm_thread_create(vm, 0x3000, 200);
    TEST_ASSERT(vm->scheduler.count == 4, "Multiple threads created");
    
    aurora_vm_destroy(vm);
}

/* Test 5: JIT Compilation */
void test_jit_compilation(void) {
    TEST_START("JIT Compilation Infrastructure");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    TEST_ASSERT(vm->jit.enabled == AURORA_VM_JIT_ENABLED, "JIT enabled by default");
    
    /* Enable JIT (allocates cache) */
    aurora_vm_jit_enable(vm, true);
    TEST_ASSERT(vm->jit.cache != NULL, "JIT cache allocated");
    TEST_ASSERT(vm->jit.cache_size == AURORA_VM_JIT_CACHE_SIZE, "JIT cache size correct");
    
    /* Compile a basic block */
    int result = aurora_vm_jit_compile_block(vm, 0x0);
    TEST_ASSERT(result == 0, "Basic block compiled");
    TEST_ASSERT(vm->jit.num_blocks == 1, "Block added to JIT");
    
    /* Compile another block */
    aurora_vm_jit_compile_block(vm, 0x100);
    TEST_ASSERT(vm->jit.num_blocks == 2, "Multiple blocks tracked");
    
    /* Clear JIT cache */
    aurora_vm_jit_clear_cache(vm);
    TEST_ASSERT(vm->jit.num_blocks == 0, "JIT cache cleared");
    TEST_ASSERT(vm->jit.cache_used == 0, "Cache usage reset");
    
    aurora_vm_destroy(vm);
}

/* Test 6: GDB Server */
void test_gdb_server(void) {
    TEST_START("GDB Remote Debugging Protocol");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    TEST_ASSERT(vm->gdb.enabled == false, "GDB server disabled initially");
    
    /* Start GDB server */
    int result = aurora_vm_gdb_start(vm, AURORA_VM_GDB_PORT);
    TEST_ASSERT(result == 0, "GDB server started");
    TEST_ASSERT(vm->gdb.enabled == true, "GDB server enabled");
    
    /* Handle GDB events */
    result = aurora_vm_gdb_handle(vm);
    TEST_ASSERT(result == 0, "GDB handle executed");
    
    /* Test break request */
    vm->gdb.break_requested = true;
    result = aurora_vm_gdb_handle(vm);
    TEST_ASSERT(result == 1, "Break request handled");
    TEST_ASSERT(vm->cpu.halted == true, "VM halted on break");
    
    /* Stop GDB server */
    aurora_vm_gdb_stop(vm);
    TEST_ASSERT(vm->gdb.enabled == false, "GDB server stopped");
    
    aurora_vm_destroy(vm);
}

/* Test 7: Memory-Mapped I/O */
void test_mmio(void) {
    TEST_START("Memory-Mapped Device I/O");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Verify MMIO regions are defined */
    TEST_ASSERT(AURORA_VM_MMIO_BASE == 0xC000, "MMIO base address defined");
    TEST_ASSERT(AURORA_VM_MMIO_SIZE == 0x2000, "MMIO region size defined");
    
    /* Verify device MMIO addresses */
    TEST_ASSERT(AURORA_VM_MMIO_DISPLAY == 0xC000, "Display MMIO address");
    TEST_ASSERT(AURORA_VM_MMIO_KEYBOARD == 0xC400, "Keyboard MMIO address");
    TEST_ASSERT(AURORA_VM_MMIO_MOUSE == 0xC800, "Mouse MMIO address");
    TEST_ASSERT(AURORA_VM_MMIO_TIMER == 0xCC00, "Timer MMIO address");
    TEST_ASSERT(AURORA_VM_MMIO_NETWORK == 0xD000, "Network MMIO address");
    TEST_ASSERT(AURORA_VM_MMIO_IRQ_CTRL == 0xD400, "IRQ controller MMIO address");
    
    printf("  ℹ MMIO regions properly defined for future device access\n");
    
    aurora_vm_destroy(vm);
}

/* Test 8: Instruction Set Extensions */
void test_instruction_extensions(void) {
    TEST_START("Instruction Set Extensions");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Test floating-point opcodes (stubbed) */
    uint32_t fp_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 20),
        aurora_encode_r_type(AURORA_OP_FADD, 3, 1, 2),  /* Float add (stubbed) */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)fp_program, sizeof(fp_program), 0);
    int result = aurora_vm_run(vm);
    TEST_ASSERT(result == 0, "Floating-point instruction executed (stubbed)");
    
    /* Test SIMD opcodes (stubbed) */
    aurora_vm_reset(vm);
    uint32_t simd_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 5),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 10),
        aurora_encode_r_type(AURORA_OP_VADD, 3, 1, 2),  /* Vector add (stubbed) */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)simd_program, sizeof(simd_program), 0);
    result = aurora_vm_run(vm);
    TEST_ASSERT(result == 0, "SIMD instruction executed (stubbed)");
    
    printf("  ℹ Floating-point and SIMD operations are stubbed for future implementation\n");
    
    aurora_vm_destroy(vm);
}

/* Main test runner */
int main(void) {
    printf("========================================\n");
    printf("Aurora VM - New Extensions Test Suite\n");
    printf("========================================\n");
    
    TEST_SECTION("Category 1: Instruction Set Extensions");
    test_atomic_operations();
    test_instruction_extensions();
    
    TEST_SECTION("Category 2: Interrupt Support");
    test_interrupts();
    
    TEST_SECTION("Category 3: Network Device");
    test_network_device();
    
    TEST_SECTION("Category 4: Multi-threading");
    test_multithreading();
    
    TEST_SECTION("Category 5: JIT Compilation");
    test_jit_compilation();
    
    TEST_SECTION("Category 6: GDB Server");
    test_gdb_server();
    
    TEST_SECTION("Category 7: Memory-Mapped I/O");
    test_mmio();
    
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
