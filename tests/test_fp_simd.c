/**
 * @file test_fp_simd.c
 * @brief Tests for floating-point and SIMD operations
 */

#include "../include/platform/aurora_vm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int tests_passed = 0;
static int tests_failed = 0;

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

/* Helper to compare floats with tolerance */
static int float_equal(float a, float b, float epsilon) {
    return fabs(a - b) < epsilon;
}

void test_floating_point_operations(void) {
    TEST_START("Floating-Point Operations");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Test FCVT and FADD */
    uint32_t program[] = {
        /* Convert 5 to float in r1 */
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 5),
        aurora_encode_r_type(AURORA_OP_FCVT, 1, 1, 0),
        
        /* Convert 3 to float in r2 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 3),
        aurora_encode_r_type(AURORA_OP_FCVT, 2, 2, 0),
        
        /* Add: r3 = r1 + r2 (5.0 + 3.0 = 8.0) */
        aurora_encode_r_type(AURORA_OP_FADD, 3, 1, 2),
        
        /* Convert result back to int in r4 */
        aurora_encode_r_type(AURORA_OP_ICVT, 4, 3, 0),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    aurora_vm_run(vm);
    
    uint32_t result = aurora_vm_get_register(vm, 4);
    TEST_ASSERT(result == 8, "FADD: 5.0 + 3.0 = 8.0");
    
    /* Test FSUB */
    aurora_vm_reset(vm);
    uint32_t sub_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 10),
        aurora_encode_r_type(AURORA_OP_FCVT, 1, 1, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 3),
        aurora_encode_r_type(AURORA_OP_FCVT, 2, 2, 0),
        aurora_encode_r_type(AURORA_OP_FSUB, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_ICVT, 4, 3, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)sub_program, sizeof(sub_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 4);
    TEST_ASSERT(result == 7, "FSUB: 10.0 - 3.0 = 7.0");
    
    /* Test FMUL */
    aurora_vm_reset(vm);
    uint32_t mul_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 4),
        aurora_encode_r_type(AURORA_OP_FCVT, 1, 1, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 5),
        aurora_encode_r_type(AURORA_OP_FCVT, 2, 2, 0),
        aurora_encode_r_type(AURORA_OP_FMUL, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_ICVT, 4, 3, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)mul_program, sizeof(mul_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 4);
    TEST_ASSERT(result == 20, "FMUL: 4.0 * 5.0 = 20.0");
    
    /* Test FDIV */
    aurora_vm_reset(vm);
    uint32_t div_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 20),
        aurora_encode_r_type(AURORA_OP_FCVT, 1, 1, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 4),
        aurora_encode_r_type(AURORA_OP_FCVT, 2, 2, 0),
        aurora_encode_r_type(AURORA_OP_FDIV, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_ICVT, 4, 3, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)div_program, sizeof(div_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 4);
    TEST_ASSERT(result == 5, "FDIV: 20.0 / 4.0 = 5.0");
    
    /* Test FCMP */
    aurora_vm_reset(vm);
    uint32_t cmp_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 5),
        aurora_encode_r_type(AURORA_OP_FCVT, 1, 1, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 5),
        aurora_encode_r_type(AURORA_OP_FCVT, 2, 2, 0),
        aurora_encode_r_type(AURORA_OP_FCMP, 0, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)cmp_program, sizeof(cmp_program), 0);
    aurora_vm_run(vm);
    
    TEST_ASSERT(vm->cpu.flags & AURORA_FLAG_ZERO, "FCMP: 5.0 == 5.0 sets zero flag");
    
    /* Test FMOV */
    aurora_vm_reset(vm);
    uint32_t mov_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 42),
        aurora_encode_r_type(AURORA_OP_FCVT, 1, 1, 0),
        aurora_encode_r_type(AURORA_OP_FMOV, 3, 1, 0),
        aurora_encode_r_type(AURORA_OP_ICVT, 4, 3, 0),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)mov_program, sizeof(mov_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 4);
    TEST_ASSERT(result == 42, "FMOV: Copy float value");
    
    aurora_vm_destroy(vm);
}

void test_simd_operations(void) {
    TEST_START("SIMD/Vector Operations");
    
    AuroraVM *vm = aurora_vm_create();
    aurora_vm_init(vm);
    
    /* Test VADD - add 4 bytes packed in registers */
    uint32_t program[] = {
        /* r1 = 0x04030201 (bytes: 1,2,3,4) */
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x0201),
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0x0403),
        aurora_encode_r_type(AURORA_OP_SHL, 2, 2, 0),
        aurora_encode_i_type(AURORA_OP_LOADI, 3, 16),
        aurora_encode_r_type(AURORA_OP_SHL, 2, 2, 3),
        aurora_encode_r_type(AURORA_OP_OR, 1, 1, 2),
        
        /* r2 = 0x04030201 (bytes: 1,2,3,4) */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0x0201),
        aurora_encode_i_type(AURORA_OP_LOADI, 4, 0x0403),
        aurora_encode_r_type(AURORA_OP_SHL, 4, 4, 0),
        aurora_encode_r_type(AURORA_OP_SHL, 4, 4, 3),
        aurora_encode_r_type(AURORA_OP_OR, 2, 2, 4),
        
        /* VADD r3 = r1 + r2 (should be 0x08060402) */
        aurora_encode_r_type(AURORA_OP_VADD, 3, 1, 2),
        
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
    aurora_vm_run(vm);
    
    uint32_t result = aurora_vm_get_register(vm, 3);
    /* Each byte should be doubled: 2,4,6,8 */
    TEST_ASSERT((result & 0xFF) == 2, "VADD: byte 0 correct");
    TEST_ASSERT(((result >> 8) & 0xFF) == 4, "VADD: byte 1 correct");
    TEST_ASSERT(((result >> 16) & 0xFF) == 6, "VADD: byte 2 correct");
    TEST_ASSERT(((result >> 24) & 0xFF) == 8, "VADD: byte 3 correct");
    
    /* Test VSUB */
    aurora_vm_reset(vm);
    uint32_t sub_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x0A05),  /* 5, 10 in low bytes */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0x0103),  /* 3, 1 in low bytes */
        aurora_encode_r_type(AURORA_OP_VSUB, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)sub_program, sizeof(sub_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 3);
    TEST_ASSERT((result & 0xFF) == 2, "VSUB: byte 0 = 5-3 = 2");
    TEST_ASSERT(((result >> 8) & 0xFF) == 9, "VSUB: byte 1 = 10-1 = 9");
    
    /* Test VMUL */
    aurora_vm_reset(vm);
    uint32_t mul_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x0302),  /* 2, 3 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0x0504),  /* 4, 5 */
        aurora_encode_r_type(AURORA_OP_VMUL, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)mul_program, sizeof(mul_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 3);
    TEST_ASSERT((result & 0xFF) == 8, "VMUL: byte 0 = 2*4 = 8");
    TEST_ASSERT(((result >> 8) & 0xFF) == 15, "VMUL: byte 1 = 3*5 = 15");
    
    /* Test VDOT */
    aurora_vm_reset(vm);
    uint32_t dot_program[] = {
        aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x0201),  /* 1, 2, 0, 0 */
        aurora_encode_i_type(AURORA_OP_LOADI, 2, 0x0403),  /* 3, 4, 0, 0 */
        aurora_encode_r_type(AURORA_OP_VDOT, 3, 1, 2),
        aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
    };
    
    aurora_vm_load_program(vm, (uint8_t *)dot_program, sizeof(dot_program), 0);
    aurora_vm_run(vm);
    
    result = aurora_vm_get_register(vm, 3);
    /* 1*3 + 2*4 + 0*0 + 0*0 = 3 + 8 = 11 */
    TEST_ASSERT(result == 11, "VDOT: dot product = 11");
    
    aurora_vm_destroy(vm);
}

int main(void) {
    printf("========================================\n");
    printf("Aurora VM - Floating-Point & SIMD Tests\n");
    printf("========================================\n");
    
    test_floating_point_operations();
    test_simd_operations();
    
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
