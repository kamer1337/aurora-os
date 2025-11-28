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

/* Crystal-Kyber parameters */
#define KYBER_N 256
#define KYBER_Q 3329
#define KYBER_K 3
#define KYBER_ETA1 2
#define KYBER_ETA2 2

/* SIMD acceleration flags */
#define SIMD_NONE    0
#define SIMD_SSE2    (1 << 0)
#define SIMD_AVX     (1 << 1)
#define SIMD_AVX2    (1 << 2)
#define SIMD_AVX512  (1 << 3)

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
 * Kyber polynomial (NTT domain)
 */
typedef struct {
    int16_t coeffs[KYBER_N];
} kyber_poly_t;

/**
 * Kyber public key
 */
typedef struct {
    kyber_poly_t pk_poly[KYBER_K];
    uint8_t seed[32];
} kyber_pubkey_t;

/**
 * Kyber secret key
 */
typedef struct {
    kyber_poly_t sk_poly[KYBER_K];
} kyber_seckey_t;

/**
 * Kyber ciphertext
 */
typedef struct {
    kyber_poly_t ct_poly[KYBER_K];
    kyber_poly_t v;
} kyber_ciphertext_t;

/**
 * Hardware acceleration context
 */
typedef struct {
    uint32_t simd_flags;
    uint8_t hw_accel_enabled;
    uint64_t simd_operations;
} hw_accel_ctx_t;

/**
 * Quantum Encryption Context
 * Maintains state for encryption/decryption operations
 */
typedef struct {
    quantum_key_t current_key;
    uint32_t operations_count;
    uint8_t is_initialized;
    hw_accel_ctx_t hw_ctx;
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

/* Password Hashing Functions */
int quantum_hash_password(const char* password, uint8_t* hash_out, size_t hash_size);
int quantum_verify_password(const char* password, const uint8_t* stored_hash, size_t hash_size);

/* Hardware acceleration functions */
int hw_accel_init(hw_accel_ctx_t* ctx);
int hw_accel_detect_simd(void);
void hw_accel_cleanup(hw_accel_ctx_t* ctx);

/* SIMD-accelerated Crystal-Kyber operations */
int kyber_keygen(kyber_pubkey_t* pk, kyber_seckey_t* sk);
int kyber_encaps(const kyber_pubkey_t* pk, uint8_t* shared_secret, kyber_ciphertext_t* ct);
int kyber_decaps(const kyber_seckey_t* sk, const kyber_ciphertext_t* ct, uint8_t* shared_secret);

/* SIMD-accelerated polynomial operations */
void poly_ntt_simd(kyber_poly_t* p, uint32_t simd_flags);
void poly_invntt_simd(kyber_poly_t* p, uint32_t simd_flags);
void poly_add_simd(kyber_poly_t* r, const kyber_poly_t* a, const kyber_poly_t* b, uint32_t simd_flags);
void poly_sub_simd(kyber_poly_t* r, const kyber_poly_t* a, const kyber_poly_t* b, uint32_t simd_flags);
void poly_mul_simd(kyber_poly_t* r, const kyber_poly_t* a, const kyber_poly_t* b, uint32_t simd_flags);
void poly_reduce_simd(kyber_poly_t* p, uint32_t simd_flags);

/* Vectorized modular arithmetic */
int16_t barrett_reduce(int16_t a);
int16_t montgomery_reduce(int32_t a);

#endif /* AURORA_QUANTUM_CRYPTO_H */
