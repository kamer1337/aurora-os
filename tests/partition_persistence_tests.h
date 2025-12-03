/**
 * Aurora OS - Partition Persistence Test Suite Header
 */

#ifndef PARTITION_PERSISTENCE_TESTS_H
#define PARTITION_PERSISTENCE_TESTS_H

/* Test functions */
void test_partition_persistence(void);
void test_automatic_persistence(void);
void test_partition_table_type_persistence(void);
void test_empty_table_persistence(void);
void test_corruption_detection(void);
void test_maximum_partitions_persistence(void);

/* Run all tests */
void run_partition_persistence_tests(void);

/* Get test results */
void get_partition_persistence_test_results(int* passed, int* failed);

#endif /* PARTITION_PERSISTENCE_TESTS_H */
