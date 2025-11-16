/**
 * @file example_aurora_vm.c
 * @brief Aurora VM Test Suite - Comprehensive tests with 7 categories
 */

#include "../include/platform/aurora_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test statistics */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper macros */
#define TEST(name) \
    do { \
        printf("\n[TEST] %s\n", name); \
        tests_run++; \
    } while(0)

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define PASS() \
    do { \
        printf("  PASSED\n"); \
        tests_passed++; \
    } while(0)

/* ===== Test Category 1: Basic Arithmetic and Logic ===== */

void test_arithmetic_add(void) {
    TEST("Arithmetic: ADD operation");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: add 42 + 58 = 100 */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 42),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 58),
        aurora_encode_r_type(AURORA_OP_ADD, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 100);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_arithmetic_sub(void) {
    TEST("Arithmetic: SUB operation");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: 100 - 42 = 58 */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 42),
        aurora_encode_r_type(AURORA_OP_SUB, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 58);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_arithmetic_mul(void) {
    TEST("Arithmetic: MUL operation");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: 7 * 8 = 56 */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 7),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 8),
        aurora_encode_r_type(AURORA_OP_MUL, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 56);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_arithmetic_div(void) {
    TEST("Arithmetic: DIV operation");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: 100 / 5 = 20 */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 5),
        aurora_encode_r_type(AURORA_OP_DIV, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 20);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_logic_operations(void) {
    TEST("Logic: AND, OR, XOR, NOT operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: test bitwise operations */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0xF0),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0x0F),
        aurora_encode_r_type(AURORA_OP_AND, 3, 1, 2),  /* 0xF0 & 0x0F = 0x00 */
        aurora_encode_r_type(AURORA_OP_OR, 4, 1, 2),   /* 0xF0 | 0x0F = 0xFF */
        aurora_encode_r_type(AURORA_OP_XOR, 5, 1, 2),  /* 0xF0 ^ 0x0F = 0xFF */
        aurora_encode_r_type(AURORA_OP_NOT, 6, 1, 0),  /* ~0xF0 = 0xFFFFFF0F */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 0x00);
    ASSERT(aurora_vm_get_register(vm, 4) == 0xFF);
    ASSERT(aurora_vm_get_register(vm, 5) == 0xFF);
    ASSERT(aurora_vm_get_register(vm, 6) == 0xFFFFFF0F);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_shift_operations(void) {
    TEST("Logic: SHL and SHR operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: test shift operations */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x0F),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 4),
        aurora_encode_r_type(AURORA_OP_SHL, 3, 1, 2),  /* 0x0F << 4 = 0xF0 */
        aurora_encode_r_type(AURORA_OP_SHR, 4, 3, 2),  /* 0xF0 >> 4 = 0x0F */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 0xF0);
    ASSERT(aurora_vm_get_register(vm, 4) == 0x0F);
    
    aurora_vm_destroy(vm);
    PASS();
}

/* ===== Test Category 2: Memory Operations ===== */

void test_memory_load_store(void) {
    TEST("Memory: LOAD and STORE operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: store and load a value */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x4000),  /* Address in heap (16KB) */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 12345),   /* Value to store */
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 0),       /* Offset */
        aurora_encode_r_type(AURORA_OP_STORE, 2, 1, 3),    /* STORE [r1 + r3], r2 */
        aurora_encode_r_type(AURORA_OP_LOAD, 4, 1, 3),     /* LOAD r4, [r1 + r3] */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 4) == 12345);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_memory_byte_operations(void) {
    TEST("Memory: LOADB and STOREB operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: store and load bytes */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x4000),  /* Address in heap (16KB) */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0xAB),    /* Byte value */
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 0),       /* Offset */
        aurora_encode_r_type(AURORA_OP_STOREB, 2, 1, 3),   /* STOREB [r1 + r3], r2 */
        aurora_encode_r_type(AURORA_OP_LOADB, 4, 1, 3),    /* LOADB r4, [r1 + r3] */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 4) == 0xAB);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_memory_page_protection(void) {
    TEST("Memory: Page protection");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Test read/write protection */
    uint8_t prot = aurora_vm_get_page_protection(vm, 100);
    ASSERT(prot & AURORA_PAGE_READ);
    ASSERT(prot & AURORA_PAGE_WRITE);
    
    /* Test executable pages */
    prot = aurora_vm_get_page_protection(vm, 0);
    ASSERT(prot & AURORA_PAGE_EXEC);
    
    aurora_vm_destroy(vm);
    PASS();
}

/* ===== Test Category 3: Control Flow ===== */

void test_control_jump(void) {
    TEST("Control Flow: JMP operation");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: jump over an instruction */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),      /* Load 10 */
        aurora_encode_j_type(AURORA_OP_JMP, 12),           /* Jump to offset 12 (3rd instruction) */
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 99),      /* This should be skipped */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 1) == 10);  /* Should remain 10 */
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_control_conditional_jump(void) {
    TEST("Control Flow: JZ and JNZ operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: conditional jumps */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0),       /* Load 0 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0),       /* Load 0 */
        aurora_encode_r_type(AURORA_OP_CMP, 0, 1, 2),      /* Compare (sets zero flag) */
        aurora_encode_j_type(AURORA_OP_JZ, 20),            /* Jump if zero to instruction 5 */
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 99),      /* Skip this */
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 42),      /* Load 42 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 42);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_control_call_return(void) {
    TEST("Control Flow: CALL and RET operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: function call and return */
    uint32_t program[] = {
        /* Main: */
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),      /* 0: Load 10 */
        aurora_encode_j_type(AURORA_OP_CALL, 12),          /* 4: Call function at offset 12 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),     /* 8: Halt */
        /* Function: */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 5),       /* 12: Load 5 */
        aurora_encode_r_type(AURORA_OP_ADD, 1, 1, 2),      /* 16: Add to r1 */
        aurora_encode_r_type(AURORA_OP_RET, 0, 0, 0),      /* 20: Return */
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 1) == 15);  /* 10 + 5 */
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_comparison_operations(void) {
    TEST("Control Flow: Comparison operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: test comparisons */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 20),
        aurora_encode_r_type(AURORA_OP_SLT, 3, 1, 2),      /* r3 = (10 < 20) = 1 */
        aurora_encode_r_type(AURORA_OP_SLE, 4, 1, 2),      /* r4 = (10 <= 20) = 1 */
        aurora_encode_r_type(AURORA_OP_SEQ, 5, 1, 2),      /* r5 = (10 == 20) = 0 */
        aurora_encode_r_type(AURORA_OP_SNE, 6, 1, 2),      /* r6 = (10 != 20) = 1 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 3) == 1);
    ASSERT(aurora_vm_get_register(vm, 4) == 1);
    ASSERT(aurora_vm_get_register(vm, 5) == 0);
    ASSERT(aurora_vm_get_register(vm, 6) == 1);
    
    aurora_vm_destroy(vm);
    PASS();
}

/* ===== Test Category 4: System Calls ===== */

void test_syscall_alloc_free(void) {
    TEST("Syscalls: ALLOC and FREE");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: allocate and free memory */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 1024),    /* Allocate 1KB */
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 2, 0, 0),     /* Save address */
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_FREE),
        aurora_encode_r_type(AURORA_OP_MOVE, 1, 2, 0),     /* Address to free */
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 2) != 0);  /* Got an address */
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_syscall_get_time(void) {
    TEST("Syscalls: GET_TIME");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: get time */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_GET_TIME),
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_MOVE, 1, 0, 0),     /* Save result to r1 */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    /* Time should be > 0 (instructions were executed) */
    ASSERT(aurora_vm_get_register(vm, 1) > 0);
    
    aurora_vm_destroy(vm);
    PASS();
}

/* Removed: Device syscall tests - no memory-mapped I/O in simplified model
void test_syscall_pixel(void) {
    TEST("Syscalls: PIXEL (draw pixel)");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    // Program: draw a pixel
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_PIXEL),
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),     // X = 100
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),     // Y = 100
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 255),     // Red color (0xFF0000FF) - using small imm
        aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_display_get_pixel(vm, 100, 100) == 255);
    
    aurora_vm_destroy(vm);
    PASS();
}
*/

/* ===== Test Category 5: Device I/O ===== */
/* Note: Device tests removed - simplified model has no memory-mapped I/O */

/* Removed: Display device tests
void test_device_display(void) {
    TEST("Devices: Display operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    // Test setting and getting pixels
    aurora_vm_display_set_pixel(vm, 50, 50, 0xFF00FF00);  // Green
    ASSERT(aurora_vm_display_get_pixel(vm, 50, 50) == 0xFF00FF00);
    
    aurora_vm_display_set_pixel(vm, 0, 0, 0xFFFFFFFF);    // White
    ASSERT(aurora_vm_display_get_pixel(vm, 0, 0) == 0xFFFFFFFF);
    
    aurora_vm_destroy(vm);
    PASS();
}
*/

/* Removed: Keyboard device tests
void test_device_keyboard(void) {
    TEST("Devices: Keyboard operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    // Test key presses
    aurora_vm_keyboard_set_key(vm, 'A', true);
    ASSERT(aurora_vm_keyboard_is_key_pressed(vm, 'A') == true);
    
    aurora_vm_keyboard_set_key(vm, 'A', false);
    ASSERT(aurora_vm_keyboard_is_key_pressed(vm, 'A') == false);
    
    aurora_vm_destroy(vm);
    PASS();
}
*/

/* Removed: Mouse device tests
void test_device_mouse(void) {
    TEST("Devices: Mouse operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    // Test mouse position
    aurora_vm_mouse_set_position(vm, 150, 200);
    int32_t x, y;
    aurora_vm_mouse_get_position(vm, &x, &y);
    ASSERT(x == 150);
    ASSERT(y == 200);
    
    // Test mouse buttons
    aurora_vm_mouse_set_buttons(vm, 0x01);  // Left button
    ASSERT(aurora_vm_mouse_get_buttons(vm) == 0x01);
    
    aurora_vm_destroy(vm);
    PASS();
}
*/

void test_device_timer(void) {
    TEST("Devices: Timer operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Test timer */
    uint64_t initial = aurora_vm_timer_get_ticks(vm);
    aurora_vm_timer_advance(vm, 1000);
    uint64_t after = aurora_vm_timer_get_ticks(vm);
    ASSERT(after == initial + 1000);
    
    aurora_vm_destroy(vm);
    PASS();
}

/* Removed: Storage device tests - no memory-mapped I/O in simplified model
void test_device_storage(void) {
    TEST("Devices: Storage operations");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    // Test storage read/write
    uint8_t data[] = {1, 2, 3, 4, 5};
    ASSERT(aurora_vm_storage_write(vm, 0, data, sizeof(data)) == sizeof(data));
    
    uint8_t read_data[5];
    ASSERT(aurora_vm_storage_read(vm, 0, read_data, sizeof(read_data)) == sizeof(read_data));
    ASSERT(memcmp(data, read_data, sizeof(data)) == 0);
    
    aurora_vm_destroy(vm);
    PASS();
}
*/

/* ===== Test Category 6: Debugger ===== */

void test_debugger_breakpoints(void) {
    TEST("Debugger: Breakpoints");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Set breakpoint */
    aurora_vm_debugger_enable(vm, true);
    ASSERT(aurora_vm_debugger_add_breakpoint(vm, 8) == 0);
    
    /* Program that will hit breakpoint */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),      /* 0 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 20),      /* 4 */
        aurora_encode_r_type(AURORA_OP_ADD, 3, 1, 2),      /* 8 - breakpoint here */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),     /* 12 */
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    
    /* Step until breakpoint */
    int result;
    do {
        result = aurora_vm_step(vm);
    } while (result == 0);
    
    ASSERT(result == 2);  /* Hit breakpoint */
    
    /* Remove breakpoint and continue */
    aurora_vm_debugger_remove_breakpoint(vm, 8);
    aurora_vm_run(vm);
    ASSERT(aurora_vm_get_register(vm, 3) == 30);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_debugger_single_step(void) {
    TEST("Debugger: Single-step mode");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    aurora_vm_debugger_enable(vm, true);
    aurora_vm_debugger_set_single_step(vm, true);
    
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 20),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    
    /* Each step should pause */
    ASSERT(aurora_vm_step(vm) == 2);  /* Paused after first instruction */
    ASSERT(aurora_vm_get_register(vm, 1) == 10);
    
    ASSERT(aurora_vm_step(vm) == 2);  /* Paused after second instruction */
    ASSERT(aurora_vm_get_register(vm, 2) == 20);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_debugger_counters(void) {
    TEST("Debugger: Instruction and cycle counters");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 20),
        aurora_encode_r_type(AURORA_OP_ADD, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    
    /* Should have executed 4 instructions */
    ASSERT(aurora_vm_debugger_get_instruction_count(vm) == 4);
    ASSERT(aurora_vm_debugger_get_cycle_count(vm) >= 4);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_debugger_disassembly(void) {
    TEST("Debugger: Disassembly");
    
    char buffer[128];
    
    /* Test disassembly of various instructions */
    uint32_t add_inst = aurora_encode_r_type(AURORA_OP_ADD, 3, 1, 2);
    aurora_vm_disassemble(add_inst, buffer, sizeof(buffer));
    ASSERT(strstr(buffer, "ADD") != NULL);
    
    uint32_t loadi_inst = aurora_encode_i_type(AURORA_OP_LOADI, 1, 42);
    aurora_vm_disassemble(loadi_inst, buffer, sizeof(buffer));
    ASSERT(strstr(buffer, "LOADI") != NULL);
    
    uint32_t jmp_inst = aurora_encode_j_type(AURORA_OP_JMP, 0x100);
    aurora_vm_disassemble(jmp_inst, buffer, sizeof(buffer));
    ASSERT(strstr(buffer, "JMP") != NULL);
    
    PASS();
}

/* ===== Test Category 7: Performance and Edge Cases ===== */

void test_performance_loop(void) {
    TEST("Performance: Loop execution");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: count from 0 to 100 */
    uint32_t program[] = {
        /* 0: */ aurora_encode_i_type(AURORA_OP_LOADI, 1, 0),     /* counter = 0 */
        /* 4: */ aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),   /* limit = 100 */
        /* 8: */ aurora_encode_i_type(AURORA_OP_LOADI, 3, 1),     /* increment = 1 */
        /* 12: loop: */
        /* 12: */ aurora_encode_r_type(AURORA_OP_ADD, 1, 1, 3),   /* counter++ */
        /* 16: */ aurora_encode_r_type(AURORA_OP_CMP, 0, 1, 2),   /* compare counter with limit */
        /* 20: */ aurora_encode_j_type(AURORA_OP_JNZ, 12),        /* if not equal, loop */
        /* 24: */ aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 1) == 100);
    
    /* Check performance - should execute in reasonable number of cycles */
    uint64_t cycles = aurora_vm_debugger_get_cycle_count(vm);
    ASSERT(cycles > 0);
    printf("  Executed %llu cycles for 100 iterations\n", (unsigned long long)cycles);
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_edge_case_division_by_zero(void) {
    TEST("Edge Cases: Division by zero");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: divide by zero */
    uint32_t program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 100),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0),
        aurora_encode_r_type(AURORA_OP_DIV, 3, 1, 2),      /* Divide by zero */
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    int result = aurora_vm_run(vm);
    ASSERT(result == -1);  /* Should fail */
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_edge_case_memory_bounds(void) {
    TEST("Edge Cases: Memory access out of bounds");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Try to access invalid memory */
    uint8_t buffer[4];
    int result = aurora_vm_read_memory(vm, AURORA_VM_MEMORY_SIZE, 4, buffer);
    ASSERT(result == -1);  /* Should fail */
    
    aurora_vm_destroy(vm);
    PASS();
}

void test_complex_fibonacci(void) {
    TEST("Performance: Fibonacci calculation");
    
    AuroraVM *vm = aurora_vm_create();
    ASSERT(vm != NULL);
    ASSERT(aurora_vm_init(vm) == 0);
    
    /* Program: calculate 10th Fibonacci number (55) */
    uint32_t program[] = {
        /* Initialize */
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0),       /* fib(n-2) = 0 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 1),       /* fib(n-1) = 1 */
        aurora_encode_i_type(AURORA_OP_LOADI, 4, 10),      /* counter = 10 */
        aurora_encode_i_type(AURORA_OP_LOADI, 5, 1),       /* decrement = 1 */
        aurora_encode_i_type(AURORA_OP_LOADI, 6, 0),       /* zero for comparison */
        /* Loop */
        /* 20: */ aurora_encode_r_type(AURORA_OP_ADD, 3, 1, 2),   /* fib(n) = fib(n-2) + fib(n-1) */
        /* 24: */ aurora_encode_r_type(AURORA_OP_MOVE, 1, 2, 0),  /* fib(n-2) = fib(n-1) */
        /* 28: */ aurora_encode_r_type(AURORA_OP_MOVE, 2, 3, 0),  /* fib(n-1) = fib(n) */
        /* 32: */ aurora_encode_r_type(AURORA_OP_SUB, 4, 4, 5),   /* counter-- */
        /* 36: */ aurora_encode_r_type(AURORA_OP_CMP, 0, 4, 6),   /* compare counter with 0 */
        /* 40: */ aurora_encode_j_type(AURORA_OP_JNZ, 20),        /* if not zero, loop */
        /* 44: */ aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    ASSERT(aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0) == 0);
    ASSERT(aurora_vm_run(vm) == 0);
    ASSERT(aurora_vm_get_register(vm, 1) == 55);  /* 10th Fibonacci number is in r1 */
    
    printf("  Calculated Fibonacci(10) = %u (r1), r2=%u, r3=%u\n", 
           aurora_vm_get_register(vm, 1),
           aurora_vm_get_register(vm, 2),
           aurora_vm_get_register(vm, 3));
    printf("  Instructions: %llu, Cycles: %llu\n",
           (unsigned long long)aurora_vm_debugger_get_instruction_count(vm),
           (unsigned long long)aurora_vm_debugger_get_cycle_count(vm));
    
    aurora_vm_destroy(vm);
    PASS();
}

/* ===== Main Test Runner ===== */

int main(void) {
    printf("========================================\n");
    printf("Aurora VM Test Suite\n");
    printf("========================================\n");
    
    /* Category 1: Basic Arithmetic and Logic */
    printf("\n=== Category 1: Arithmetic and Logic ===\n");
    test_arithmetic_add();
    test_arithmetic_sub();
    test_arithmetic_mul();
    test_arithmetic_div();
    test_logic_operations();
    test_shift_operations();
    
    /* Category 2: Memory Operations */
    printf("\n=== Category 2: Memory Operations ===\n");
    test_memory_load_store();
    test_memory_byte_operations();
    test_memory_page_protection();
    
    /* Category 3: Control Flow */
    printf("\n=== Category 3: Control Flow ===\n");
    test_control_jump();
    test_control_conditional_jump();
    test_control_call_return();
    test_comparison_operations();
    
    /* Category 4: System Calls */
    printf("\n=== Category 4: System Calls ===\n");
    test_syscall_alloc_free();
    test_syscall_get_time();
    test_syscall_pixel();
    
    /* Category 5: Device I/O */
    printf("\n=== Category 5: Device I/O ===\n");
    test_device_display();
    test_device_keyboard();
    test_device_mouse();
    test_device_timer();
    test_device_storage();
    
    /* Category 6: Debugger */
    printf("\n=== Category 6: Debugger ===\n");
    test_debugger_breakpoints();
    test_debugger_single_step();
    test_debugger_counters();
    test_debugger_disassembly();
    
    /* Category 7: Performance and Edge Cases */
    printf("\n=== Category 7: Performance & Edge Cases ===\n");
    test_performance_loop();
    test_edge_case_division_by_zero();
    test_edge_case_memory_bounds();
    test_complex_fibonacci();
    
    /* Summary */
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("========================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
