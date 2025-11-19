/**
 * Aurora OS - Scientific Computing Library Implementation
 * 
 * Mathematical functions and scientific computing utilities
 */

#include "math_lib.h"

// Floating-point utilities

double math_fabs(double x) {
    return x < 0.0 ? -x : x;
}

double math_floor(double x) {
    int32_t i = (int32_t)x;
    return (double)i - (x < (double)i ? 1.0 : 0.0);
}

double math_ceil(double x) {
    int32_t i = (int32_t)x;
    return (double)i + (x > (double)i ? 1.0 : 0.0);
}

double math_round(double x) {
    return math_floor(x + 0.5);
}

double math_fmod(double x, double y) {
    if (y == 0.0) return 0.0;
    double quotient = x / y;
    int32_t iquot = (int32_t)quotient;
    return x - (double)iquot * y;
}

// Power and root functions

double math_sqrt(double x) {
    if (x < 0.0) return 0.0;  // Return 0 for negative input
    if (x == 0.0) return 0.0;
    
    // Newton's method
    double guess = x / 2.0;
    for (int i = 0; i < 20; i++) {
        double new_guess = (guess + x / guess) / 2.0;
        if (math_fabs(new_guess - guess) < 1e-10) break;
        guess = new_guess;
    }
    return guess;
}

double math_cbrt(double x) {
    if (x == 0.0) return 0.0;
    
    int negative = (x < 0.0);
    if (negative) x = -x;
    
    // Newton's method for cube root
    double guess = x / 3.0;
    for (int i = 0; i < 20; i++) {
        double new_guess = (2.0 * guess + x / (guess * guess)) / 3.0;
        if (math_fabs(new_guess - guess) < 1e-10) break;
        guess = new_guess;
    }
    
    return negative ? -guess : guess;
}

double math_pow(double x, double y) {
    // Handle special cases
    if (y == 0.0) return 1.0;
    if (x == 0.0) return 0.0;
    if (y == 1.0) return x;
    
    // For integer exponents, use repeated multiplication
    int is_int = (y == math_floor(y));
    if (is_int) {
        int exp = (int)y;
        int negative = (exp < 0);
        if (negative) exp = -exp;
        
        double result = 1.0;
        for (int i = 0; i < exp; i++) {
            result *= x;
        }
        return negative ? 1.0 / result : result;
    }
    
    // For non-integer exponents, use exp(y * ln(x))
    if (x < 0.0) return 0.0;  // Simplified: don't handle complex results
    return math_exp(y * math_log(x));
}

double math_exp(double x) {
    // Taylor series for e^x
    // e^x = 1 + x + x^2/2! + x^3/3! + ...
    
    if (x > 20.0) return 1e10;   // Prevent overflow
    if (x < -20.0) return 0.0;
    
    double result = 1.0;
    double term = 1.0;
    
    for (int n = 1; n < 50; n++) {
        term *= x / (double)n;
        result += term;
        if (math_fabs(term) < 1e-15) break;
    }
    
    return result;
}

double math_log(double x) {
    // Natural logarithm using Newton's method
    // Solve: e^y = x for y
    
    if (x <= 0.0) return -1e10;  // Error case
    if (x == 1.0) return 0.0;
    
    // Initial guess
    double y = 0.0;
    if (x > 1.0) y = x - 1.0;
    else y = -(1.0 - x);
    
    // Newton's method: y_new = y + (x - e^y) / e^y
    for (int i = 0; i < 20; i++) {
        double ey = math_exp(y);
        double new_y = y + (x - ey) / ey;
        if (math_fabs(new_y - y) < 1e-10) break;
        y = new_y;
    }
    
    return y;
}

double math_log10(double x) {
    return math_log(x) / MATH_LN10;
}

double math_log2(double x) {
    return math_log(x) / MATH_LN2;
}

// Trigonometric functions

double math_sin(double x) {
    // Reduce angle to [-π, π]
    x = math_fmod(x, 2.0 * MATH_PI);
    if (x > MATH_PI) x -= 2.0 * MATH_PI;
    if (x < -MATH_PI) x += 2.0 * MATH_PI;
    
    // Taylor series: sin(x) = x - x^3/3! + x^5/5! - x^7/7! + ...
    double result = x;
    double term = x;
    
    for (int n = 1; n < 20; n++) {
        term *= -x * x / ((double)(2 * n) * (double)(2 * n + 1));
        result += term;
        if (math_fabs(term) < 1e-15) break;
    }
    
    return result;
}

double math_cos(double x) {
    // Reduce angle to [-π, π]
    x = math_fmod(x, 2.0 * MATH_PI);
    if (x > MATH_PI) x -= 2.0 * MATH_PI;
    if (x < -MATH_PI) x += 2.0 * MATH_PI;
    
    // Taylor series: cos(x) = 1 - x^2/2! + x^4/4! - x^6/6! + ...
    double result = 1.0;
    double term = 1.0;
    
    for (int n = 1; n < 20; n++) {
        term *= -x * x / ((double)(2 * n - 1) * (double)(2 * n));
        result += term;
        if (math_fabs(term) < 1e-15) break;
    }
    
    return result;
}

double math_tan(double x) {
    double cos_x = math_cos(x);
    if (math_fabs(cos_x) < 1e-10) return 1e10;  // Avoid division by zero
    return math_sin(x) / cos_x;
}

double math_asin(double x) {
    if (x < -1.0 || x > 1.0) return 0.0;  // Out of range
    
    // Taylor series for small values
    if (math_fabs(x) < 0.5) {
        double result = x;
        double term = x;
        for (int n = 1; n < 20; n++) {
            term *= x * x * (2.0 * (double)n - 1.0) / (2.0 * (double)n);
            result += term / (2.0 * (double)n + 1.0);
            if (math_fabs(term) < 1e-15) break;
        }
        return result;
    }
    
    // Use identity: asin(x) = atan(x / sqrt(1 - x^2))
    return math_atan(x / math_sqrt(1.0 - x * x));
}

double math_acos(double x) {
    if (x < -1.0 || x > 1.0) return 0.0;  // Out of range
    return MATH_PI / 2.0 - math_asin(x);
}

double math_atan(double x) {
    // For large values, use identity: atan(x) = π/2 - atan(1/x)
    if (math_fabs(x) > 1.0) {
        double result = MATH_PI / 2.0 - math_atan(1.0 / x);
        return x < 0.0 ? -result : result;
    }
    
    // Taylor series: atan(x) = x - x^3/3 + x^5/5 - x^7/7 + ...
    double result = x;
    double term = x;
    
    for (int n = 1; n < 50; n++) {
        term *= -x * x;
        result += term / (2.0 * (double)n + 1.0);
        if (math_fabs(term / (2.0 * (double)n + 1.0)) < 1e-15) break;
    }
    
    return result;
}

double math_atan2(double y, double x) {
    if (x > 0.0) return math_atan(y / x);
    if (x < 0.0 && y >= 0.0) return math_atan(y / x) + MATH_PI;
    if (x < 0.0 && y < 0.0) return math_atan(y / x) - MATH_PI;
    if (x == 0.0 && y > 0.0) return MATH_PI / 2.0;
    if (x == 0.0 && y < 0.0) return -MATH_PI / 2.0;
    return 0.0;  // x == 0 && y == 0
}

// Hyperbolic functions

double math_sinh(double x) {
    // sinh(x) = (e^x - e^(-x)) / 2
    double ex = math_exp(x);
    double enx = math_exp(-x);
    return (ex - enx) / 2.0;
}

double math_cosh(double x) {
    // cosh(x) = (e^x + e^(-x)) / 2
    double ex = math_exp(x);
    double enx = math_exp(-x);
    return (ex + enx) / 2.0;
}

double math_tanh(double x) {
    // tanh(x) = sinh(x) / cosh(x)
    double ex = math_exp(x);
    double enx = math_exp(-x);
    return (ex - enx) / (ex + enx);
}

// Vector operations

double math_dot3(const double v1[3], const double v2[3]) {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void math_cross3(double result[3], const double v1[3], const double v2[3]) {
    result[0] = v1[1] * v2[2] - v1[2] * v2[1];
    result[1] = v1[2] * v2[0] - v1[0] * v2[2];
    result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

double math_magnitude3(const double v[3]) {
    return math_sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void math_normalize3(double result[3], const double v[3]) {
    double mag = math_magnitude3(v);
    if (mag > 1e-10) {
        result[0] = v[0] / mag;
        result[1] = v[1] / mag;
        result[2] = v[2] / mag;
    } else {
        result[0] = result[1] = result[2] = 0.0;
    }
}

// Matrix operations

void math_matrix_mult3(double result[9], const double a[9], const double b[9]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i * 3 + j] = 0.0;
            for (int k = 0; k < 3; k++) {
                result[i * 3 + j] += a[i * 3 + k] * b[k * 3 + j];
            }
        }
    }
}

void math_matrix_transpose3(double result[9], const double matrix[9]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[j * 3 + i] = matrix[i * 3 + j];
        }
    }
}

void math_matrix_identity3(double result[9]) {
    for (int i = 0; i < 9; i++) {
        result[i] = (i % 4 == 0) ? 1.0 : 0.0;
    }
}

// Statistical functions

double math_mean(const double* values, uint32_t count) {
    if (count == 0) return 0.0;
    
    double sum = 0.0;
    for (uint32_t i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum / (double)count;
}

double math_stddev(const double* values, uint32_t count) {
    if (count < 2) return 0.0;
    
    double mean = math_mean(values, count);
    double sum_sq = 0.0;
    
    for (uint32_t i = 0; i < count; i++) {
        double diff = values[i] - mean;
        sum_sq += diff * diff;
    }
    
    return math_sqrt(sum_sq / (double)(count - 1));
}

double math_min(const double* values, uint32_t count) {
    if (count == 0) return 0.0;
    
    double min_val = values[0];
    for (uint32_t i = 1; i < count; i++) {
        if (values[i] < min_val) min_val = values[i];
    }
    return min_val;
}

double math_max(const double* values, uint32_t count) {
    if (count == 0) return 0.0;
    
    double max_val = values[0];
    for (uint32_t i = 1; i < count; i++) {
        if (values[i] > max_val) max_val = values[i];
    }
    return max_val;
}

// Utility functions

int math_isnan(double x) {
    return x != x;
}

int math_isinf(double x) {
    return (x == x) && (x + 1.0 == x);
}

int math_isfinite(double x) {
    return !math_isnan(x) && !math_isinf(x);
}
