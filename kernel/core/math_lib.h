/**
 * Aurora OS - Scientific Computing Library Header
 * 
 * Mathematical functions and scientific computing utilities
 */

#ifndef MATH_LIB_H
#define MATH_LIB_H

#include <stdint.h>

// Mathematical constants
#define MATH_PI 3.14159265358979323846
#define MATH_E  2.71828182845904523536
#define MATH_SQRT2 1.41421356237309504880
#define MATH_LN2 0.69314718055994530942
#define MATH_LN10 2.30258509299404568402

// Floating-point utilities
/**
 * Absolute value of a double
 * @param x Input value
 * @return Absolute value
 */
double math_fabs(double x);

/**
 * Floor function - largest integer <= x
 * @param x Input value
 * @return Floor of x
 */
double math_floor(double x);

/**
 * Ceiling function - smallest integer >= x
 * @param x Input value
 * @return Ceiling of x
 */
double math_ceil(double x);

/**
 * Round to nearest integer
 * @param x Input value
 * @return Rounded value
 */
double math_round(double x);

/**
 * Modulo operation for floating-point
 * @param x Dividend
 * @param y Divisor
 * @return x mod y
 */
double math_fmod(double x, double y);

// Power and root functions
/**
 * Square root using Newton's method
 * @param x Input value (must be >= 0)
 * @return Square root of x
 */
double math_sqrt(double x);

/**
 * Cube root
 * @param x Input value
 * @return Cube root of x
 */
double math_cbrt(double x);

/**
 * Power function - x raised to y
 * @param x Base
 * @param y Exponent
 * @return x^y
 */
double math_pow(double x, double y);

/**
 * Exponential function - e^x
 * @param x Exponent
 * @return e^x
 */
double math_exp(double x);

/**
 * Natural logarithm - ln(x)
 * @param x Input value (must be > 0)
 * @return ln(x)
 */
double math_log(double x);

/**
 * Base-10 logarithm
 * @param x Input value (must be > 0)
 * @return log10(x)
 */
double math_log10(double x);

/**
 * Base-2 logarithm
 * @param x Input value (must be > 0)
 * @return log2(x)
 */
double math_log2(double x);

// Trigonometric functions
/**
 * Sine function
 * @param x Angle in radians
 * @return sin(x)
 */
double math_sin(double x);

/**
 * Cosine function
 * @param x Angle in radians
 * @return cos(x)
 */
double math_cos(double x);

/**
 * Tangent function
 * @param x Angle in radians
 * @return tan(x)
 */
double math_tan(double x);

/**
 * Arcsine function
 * @param x Input value [-1, 1]
 * @return arcsin(x) in radians
 */
double math_asin(double x);

/**
 * Arccosine function
 * @param x Input value [-1, 1]
 * @return arccos(x) in radians
 */
double math_acos(double x);

/**
 * Arctangent function
 * @param x Input value
 * @return arctan(x) in radians
 */
double math_atan(double x);

/**
 * Two-argument arctangent
 * @param y Y coordinate
 * @param x X coordinate
 * @return arctan(y/x) in radians, correctly handling signs
 */
double math_atan2(double y, double x);

// Hyperbolic functions
/**
 * Hyperbolic sine
 * @param x Input value
 * @return sinh(x)
 */
double math_sinh(double x);

/**
 * Hyperbolic cosine
 * @param x Input value
 * @return cosh(x)
 */
double math_cosh(double x);

/**
 * Hyperbolic tangent
 * @param x Input value
 * @return tanh(x)
 */
double math_tanh(double x);

// Vector operations
/**
 * Dot product of two 3D vectors
 * @param v1 First vector [x, y, z]
 * @param v2 Second vector [x, y, z]
 * @return Dot product
 */
double math_dot3(const double v1[3], const double v2[3]);

/**
 * Cross product of two 3D vectors
 * @param result Result vector [x, y, z]
 * @param v1 First vector [x, y, z]
 * @param v2 Second vector [x, y, z]
 */
void math_cross3(double result[3], const double v1[3], const double v2[3]);

/**
 * Magnitude of a 3D vector
 * @param v Vector [x, y, z]
 * @return Magnitude
 */
double math_magnitude3(const double v[3]);

/**
 * Normalize a 3D vector
 * @param result Normalized vector [x, y, z]
 * @param v Input vector [x, y, z]
 */
void math_normalize3(double result[3], const double v[3]);

// Matrix operations (3x3)
/**
 * Multiply two 3x3 matrices
 * @param result Result matrix (3x3, row-major)
 * @param a First matrix (3x3, row-major)
 * @param b Second matrix (3x3, row-major)
 */
void math_matrix_mult3(double result[9], const double a[9], const double b[9]);

/**
 * Transpose a 3x3 matrix
 * @param result Transposed matrix (3x3, row-major)
 * @param matrix Input matrix (3x3, row-major)
 */
void math_matrix_transpose3(double result[9], const double matrix[9]);

/**
 * Identity 3x3 matrix
 * @param result Identity matrix (3x3, row-major)
 */
void math_matrix_identity3(double result[9]);

// Statistical functions
/**
 * Mean (average) of an array
 * @param values Array of values
 * @param count Number of values
 * @return Mean value
 */
double math_mean(const double* values, uint32_t count);

/**
 * Standard deviation of an array
 * @param values Array of values
 * @param count Number of values
 * @return Standard deviation
 */
double math_stddev(const double* values, uint32_t count);

/**
 * Minimum value in an array
 * @param values Array of values
 * @param count Number of values
 * @return Minimum value
 */
double math_min(const double* values, uint32_t count);

/**
 * Maximum value in an array
 * @param values Array of values
 * @param count Number of values
 * @return Maximum value
 */
double math_max(const double* values, uint32_t count);

// Utility functions
/**
 * Check if a number is NaN
 * @param x Value to check
 * @return 1 if NaN, 0 otherwise
 */
int math_isnan(double x);

/**
 * Check if a number is infinite
 * @param x Value to check
 * @return 1 if infinite, 0 otherwise
 */
int math_isinf(double x);

/**
 * Check if a number is finite
 * @param x Value to check
 * @return 1 if finite, 0 otherwise
 */
int math_isfinite(double x);

#endif // MATH_LIB_H
