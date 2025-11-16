/**
 * Aurora OS - Display Manager Test Header
 */

#ifndef DISPLAY_MANAGER_TEST_H
#define DISPLAY_MANAGER_TEST_H

/**
 * Run all display manager tests
 */
void run_display_manager_tests(void);

/**
 * Get test results
 * @param passed Output parameter for number of passed tests
 * @param failed Output parameter for number of failed tests
 */
void get_display_manager_test_results(int* passed, int* failed);

#endif // DISPLAY_MANAGER_TEST_H
