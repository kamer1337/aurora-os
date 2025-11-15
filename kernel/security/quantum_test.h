/**
 * Aurora OS - Quantum Encryption Test Header
 */

#ifndef AURORA_QUANTUM_TEST_H
#define AURORA_QUANTUM_TEST_H

/* Run all quantum cryptography tests */
void run_quantum_crypto_tests(void);

/* Individual test functions */
void test_quantum_random(void);
void test_quantum_key_generation(void);
void test_quantum_encryption(void);
void test_quantum_hash(void);

#endif /* AURORA_QUANTUM_TEST_H */
