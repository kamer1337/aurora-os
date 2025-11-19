/**
 * Aurora OS - Scientific Computing Library Tests
 * 
 * Test suite for mathematical functions
 */

#include <stdio.h>
#include <stdint.h>
#include "../kernel/core/math_lib.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  ✗ %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

#define EPSILON 0.001

static int approx_equal(double a, double b, double epsilon) {
    double diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < epsilon;
}

void test_basic_functions(void) {
    printf("\n=== Testing Basic Functions ===\n");
    
    TEST_ASSERT(math_fabs(-5.0) == 5.0, "fabs(-5.0) == 5.0");
    TEST_ASSERT(math_fabs(5.0) == 5.0, "fabs(5.0) == 5.0");
    
    TEST_ASSERT(math_floor(3.7) == 3.0, "floor(3.7) == 3.0");
    TEST_ASSERT(math_floor(-3.7) == -4.0, "floor(-3.7) == -4.0");
    
    TEST_ASSERT(math_ceil(3.2) == 4.0, "ceil(3.2) == 4.0");
    TEST_ASSERT(math_ceil(-3.2) == -3.0, "ceil(-3.2) == -3.0");
    
    TEST_ASSERT(math_round(3.5) == 4.0, "round(3.5) == 4.0");
    TEST_ASSERT(math_round(3.4) == 3.0, "round(3.4) == 3.0");
}

void test_power_functions(void) {
    printf("\n=== Testing Power and Root Functions ===\n");
    
    TEST_ASSERT(approx_equal(math_sqrt(4.0), 2.0, EPSILON), "sqrt(4.0) ≈ 2.0");
    TEST_ASSERT(approx_equal(math_sqrt(9.0), 3.0, EPSILON), "sqrt(9.0) ≈ 3.0");
    TEST_ASSERT(approx_equal(math_sqrt(2.0), 1.414, 0.01), "sqrt(2.0) ≈ 1.414");
    
    TEST_ASSERT(approx_equal(math_cbrt(8.0), 2.0, EPSILON), "cbrt(8.0) ≈ 2.0");
    TEST_ASSERT(approx_equal(math_cbrt(27.0), 3.0, EPSILON), "cbrt(27.0) ≈ 3.0");
    
    TEST_ASSERT(approx_equal(math_pow(2.0, 3.0), 8.0, EPSILON), "pow(2.0, 3.0) ≈ 8.0");
    TEST_ASSERT(approx_equal(math_pow(5.0, 2.0), 25.0, EPSILON), "pow(5.0, 2.0) ≈ 25.0");
    
    TEST_ASSERT(approx_equal(math_exp(0.0), 1.0, EPSILON), "exp(0.0) ≈ 1.0");
    TEST_ASSERT(approx_equal(math_exp(1.0), 2.718, 0.01), "exp(1.0) ≈ 2.718 (e)");
    
    TEST_ASSERT(approx_equal(math_log(1.0), 0.0, EPSILON), "log(1.0) ≈ 0.0");
    TEST_ASSERT(approx_equal(math_log(MATH_E), 1.0, 0.01), "log(e) ≈ 1.0");
}

void test_trigonometric_functions(void) {
    printf("\n=== Testing Trigonometric Functions ===\n");
    
    TEST_ASSERT(approx_equal(math_sin(0.0), 0.0, EPSILON), "sin(0.0) ≈ 0.0");
    TEST_ASSERT(approx_equal(math_sin(MATH_PI / 2.0), 1.0, EPSILON), "sin(π/2) ≈ 1.0");
    TEST_ASSERT(approx_equal(math_sin(MATH_PI), 0.0, EPSILON), "sin(π) ≈ 0.0");
    
    TEST_ASSERT(approx_equal(math_cos(0.0), 1.0, EPSILON), "cos(0.0) ≈ 1.0");
    TEST_ASSERT(approx_equal(math_cos(MATH_PI / 2.0), 0.0, EPSILON), "cos(π/2) ≈ 0.0");
    TEST_ASSERT(approx_equal(math_cos(MATH_PI), -1.0, EPSILON), "cos(π) ≈ -1.0");
    
    TEST_ASSERT(approx_equal(math_tan(0.0), 0.0, EPSILON), "tan(0.0) ≈ 0.0");
    TEST_ASSERT(approx_equal(math_tan(MATH_PI / 4.0), 1.0, 0.01), "tan(π/4) ≈ 1.0");
}

void test_hyperbolic_functions(void) {
    printf("\n=== Testing Hyperbolic Functions ===\n");
    
    TEST_ASSERT(approx_equal(math_sinh(0.0), 0.0, EPSILON), "sinh(0.0) ≈ 0.0");
    TEST_ASSERT(approx_equal(math_cosh(0.0), 1.0, EPSILON), "cosh(0.0) ≈ 1.0");
    TEST_ASSERT(approx_equal(math_tanh(0.0), 0.0, EPSILON), "tanh(0.0) ≈ 0.0");
}

void test_vector_operations(void) {
    printf("\n=== Testing Vector Operations ===\n");
    
    double v1[3] = {1.0, 0.0, 0.0};
    double v2[3] = {0.0, 1.0, 0.0};
    double result[3];
    
    TEST_ASSERT(approx_equal(math_dot3(v1, v2), 0.0, EPSILON), "dot([1,0,0], [0,1,0]) ≈ 0.0");
    
    math_cross3(result, v1, v2);
    TEST_ASSERT(approx_equal(result[0], 0.0, EPSILON) &&
                approx_equal(result[1], 0.0, EPSILON) &&
                approx_equal(result[2], 1.0, EPSILON),
                "cross([1,0,0], [0,1,0]) ≈ [0,0,1]");
    
    double v3[3] = {3.0, 4.0, 0.0};
    TEST_ASSERT(approx_equal(math_magnitude3(v3), 5.0, EPSILON), "magnitude([3,4,0]) ≈ 5.0");
    
    math_normalize3(result, v3);
    TEST_ASSERT(approx_equal(math_magnitude3(result), 1.0, EPSILON), 
                "normalized vector has magnitude 1.0");
}

void test_matrix_operations(void) {
    printf("\n=== Testing Matrix Operations ===\n");
    
    double identity[9];
    math_matrix_identity3(identity);
    TEST_ASSERT(identity[0] == 1.0 && identity[4] == 1.0 && identity[8] == 1.0,
                "Identity matrix diagonal is 1.0");
    TEST_ASSERT(identity[1] == 0.0 && identity[2] == 0.0 && identity[3] == 0.0,
                "Identity matrix off-diagonal is 0.0");
    
    double mat[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double trans[9];
    math_matrix_transpose3(trans, mat);
    TEST_ASSERT(trans[0] == 1 && trans[1] == 4 && trans[2] == 7,
                "Transpose correct: first row");
}

void test_statistical_functions(void) {
    printf("\n=== Testing Statistical Functions ===\n");
    
    double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    
    TEST_ASSERT(approx_equal(math_mean(values, 5), 3.0, EPSILON), "mean([1,2,3,4,5]) ≈ 3.0");
    TEST_ASSERT(approx_equal(math_min(values, 5), 1.0, EPSILON), "min([1,2,3,4,5]) ≈ 1.0");
    TEST_ASSERT(approx_equal(math_max(values, 5), 5.0, EPSILON), "max([1,2,3,4,5]) ≈ 5.0");
    
    double stddev = math_stddev(values, 5);
    TEST_ASSERT(approx_equal(stddev, 1.58, 0.1), "stddev([1,2,3,4,5]) ≈ 1.58");
}

int main(void) {
    printf("========================================\n");
    printf("Aurora OS - Scientific Computing Library Tests\n");
    printf("========================================\n");
    
    test_basic_functions();
    test_power_functions();
    test_trigonometric_functions();
    test_hyperbolic_functions();
    test_vector_operations();
    test_matrix_operations();
    test_statistical_functions();
    
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
