/**
 * Aurora OS - Quantum Cryptography Module
 * 
 * This module provides quantum encryption capabilities for the kernel.
 * Includes quantum random number generation and post-quantum cryptographic
 * algorithms suitable for kernel-level security operations.
 */

#ifndef AURORA_QUANTUM_CRYPTO_H
#define AURORA_QUANTUM_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

/* Quantum encryption status codes */
#define QCRYPTO_SUCCESS 0
#define QCRYPTO_ERROR -1
#define QCRYPTO_INVALID_PARAM -2
#define QCRYPTO_NOT_INITIALIZED -3

/* Quantum key sizes */
#define QCRYPTO_KEY_SIZE_128 16
#define QCRYPTO_KEY_SIZE_256 32
#define QCRYPTO_KEY_SIZE_512 64

/* Quantum encryption block size */
#define QCRYPTO_BLOCK_SIZE 16

/**
 * Quantum Key Structure
 * Represents a quantum-generated cryptographic key
 */
typedef struct {
    uint8_t key_data[QCRYPTO_KEY_SIZE_256];
    size_t key_length;
    uint32_t generation_time;
    uint8_t is_valid;
} quantum_key_t;

/**
 * Quantum Encryption Context
 * Maintains state for encryption/decryption operations
 */
typedef struct {
    quantum_key_t current_key;
    uint32_t operations_count;
    uint8_t is_initialized;
} quantum_crypto_ctx_t;

/* Initialization and cleanup */
int quantum_crypto_init(void);
void quantum_crypto_cleanup(void);

/* Quantum Random Number Generation */
int quantum_random_bytes(uint8_t* buffer, size_t length);
uint32_t quantum_random_uint32(void);
uint64_t quantum_random_uint64(void);

/* Quantum Key Generation and Management */
int quantum_key_generate(quantum_key_t* key, size_t key_size);
int quantum_key_derive(quantum_key_t* dest, const quantum_key_t* source, const uint8_t* salt, size_t salt_len);
void quantum_key_destroy(quantum_key_t* key);

/* Quantum Encryption/Decryption Operations */
int quantum_encrypt_block(quantum_crypto_ctx_t* ctx, const uint8_t* plaintext, 
                          uint8_t* ciphertext, size_t length);
int quantum_decrypt_block(quantum_crypto_ctx_t* ctx, const uint8_t* ciphertext,
                          uint8_t* plaintext, size_t length);

/* Context Management */
int quantum_crypto_ctx_create(quantum_crypto_ctx_t* ctx, const quantum_key_t* key);
void quantum_crypto_ctx_destroy(quantum_crypto_ctx_t* ctx);

/* Utility Functions */
int quantum_hash(const uint8_t* data, size_t length, uint8_t* hash, size_t hash_size);
int quantum_verify_integrity(const uint8_t* data, size_t length, const uint8_t* expected_hash);

#endif /* AURORA_QUANTUM_CRYPTO_H */
