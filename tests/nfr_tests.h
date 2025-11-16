/**
 * Aurora OS - Non-Functional Requirements Module Tests Header
 */

#ifndef AURORA_NFR_TESTS_H
#define AURORA_NFR_TESTS_H

/**
 * Run all NFR tests
 */
void run_nfr_tests(void);

/**
 * Get test results
 * @param passed Pointer to store number of passed tests
 * @param failed Pointer to store number of failed tests
 */
void get_nfr_test_results(int* passed, int* failed);

#endif /* AURORA_NFR_TESTS_H */
