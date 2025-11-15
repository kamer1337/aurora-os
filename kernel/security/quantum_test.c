/**
 * Aurora OS - Quantum Encryption Test/Demo
 * 
 * Demonstrates the quantum encryption capabilities
 */

#include "quantum_crypto.h"
#include "../drivers/vga.h"
#include "../drivers/serial.h"

/**
 * Test quantum random number generation
 */
void test_quantum_random(void) {
    vga_write("\n=== Quantum Random Number Generation Test ===\n");
    
    /* Test random bytes */
    uint8_t random_buffer[32];
    int result = quantum_random_bytes(random_buffer, sizeof(random_buffer));
    
    if (result == QCRYPTO_SUCCESS) {
        vga_write("Generated 32 quantum random bytes\n");
        serial_write(SERIAL_COM1, "Quantum random bytes: ");
        
        char hex[3];
        for (int i = 0; i < 8; i++) {
            hex[0] = "0123456789ABCDEF"[random_buffer[i] >> 4];
            hex[1] = "0123456789ABCDEF"[random_buffer[i] & 0x0F];
            hex[2] = '\0';
            serial_write(SERIAL_COM1, hex);
            serial_write(SERIAL_COM1, " ");
        }
        serial_write(SERIAL_COM1, "...\n");
    } else {
        vga_write("ERROR: Quantum random generation failed\n");
    }
    
    /* Test random uint32 */
    uint32_t rand32 = quantum_random_uint32();
    vga_write("Generated 32-bit quantum random number\n");
    
    /* Test random uint64 */
    uint64_t rand64 = quantum_random_uint64();
    vga_write("Generated 64-bit quantum random number\n");
}

/**
 * Test quantum key generation
 */
void test_quantum_key_generation(void) {
    vga_write("\n=== Quantum Key Generation Test ===\n");
    
    quantum_key_t key;
    int result = quantum_key_generate(&key, QCRYPTO_KEY_SIZE_256);
    
    if (result == QCRYPTO_SUCCESS && key.is_valid) {
        vga_write("Generated 256-bit quantum key successfully\n");
        serial_write(SERIAL_COM1, "Quantum key generated (256 bits)\n");
        
        /* Test key derivation */
        quantum_key_t derived_key;
        uint8_t salt[] = "AuroraOS_Quantum_Salt";
        result = quantum_key_derive(&derived_key, &key, salt, sizeof(salt));
        
        if (result == QCRYPTO_SUCCESS && derived_key.is_valid) {
            vga_write("Derived new quantum key from master key\n");
        }
        
        /* Clean up keys */
        quantum_key_destroy(&key);
        quantum_key_destroy(&derived_key);
        vga_write("Keys securely destroyed\n");
    } else {
        vga_write("ERROR: Quantum key generation failed\n");
    }
}

/**
 * Test quantum encryption/decryption
 */
void test_quantum_encryption(void) {
    vga_write("\n=== Quantum Encryption/Decryption Test ===\n");
    
    /* Generate encryption key */
    quantum_key_t key;
    int result = quantum_key_generate(&key, QCRYPTO_KEY_SIZE_256);
    
    if (result != QCRYPTO_SUCCESS) {
        vga_write("ERROR: Key generation failed\n");
        return;
    }
    
    /* Create encryption context */
    quantum_crypto_ctx_t ctx;
    result = quantum_crypto_ctx_create(&ctx, &key);
    
    if (result != QCRYPTO_SUCCESS) {
        vga_write("ERROR: Context creation failed\n");
        quantum_key_destroy(&key);
        return;
    }
    
    /* Test data */
    const char* plaintext = "Aurora OS - Quantum Encrypted Data!";
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    
    /* Encrypt */
    result = quantum_encrypt_block(&ctx, (const uint8_t*)plaintext, 
                                   ciphertext, 36);
    
    if (result == QCRYPTO_SUCCESS) {
        vga_write("Data encrypted with quantum encryption\n");
        serial_write(SERIAL_COM1, "Encrypted: ");
        
        char hex[3];
        for (int i = 0; i < 16; i++) {
            hex[0] = "0123456789ABCDEF"[ciphertext[i] >> 4];
            hex[1] = "0123456789ABCDEF"[ciphertext[i] & 0x0F];
            hex[2] = '\0';
            serial_write(SERIAL_COM1, hex);
            serial_write(SERIAL_COM1, " ");
        }
        serial_write(SERIAL_COM1, "...\n");
    } else {
        vga_write("ERROR: Encryption failed\n");
        goto cleanup;
    }
    
    /* Decrypt */
    result = quantum_decrypt_block(&ctx, ciphertext, decrypted, 36);
    
    if (result == QCRYPTO_SUCCESS) {
        decrypted[36] = '\0';
        vga_write("Data decrypted successfully\n");
        serial_write(SERIAL_COM1, "Decrypted: ");
        serial_write(SERIAL_COM1, (const char*)decrypted);
        serial_write(SERIAL_COM1, "\n");
        
        /* Verify decryption */
        int match = 1;
        for (int i = 0; i < 36; i++) {
            if (decrypted[i] != plaintext[i]) {
                match = 0;
                break;
            }
        }
        
        if (match) {
            vga_write("Decryption verification: PASSED\n");
        } else {
            vga_write("Decryption verification: FAILED\n");
        }
    } else {
        vga_write("ERROR: Decryption failed\n");
    }
    
cleanup:
    quantum_crypto_ctx_destroy(&ctx);
    quantum_key_destroy(&key);
}

/**
 * Test quantum hash function
 */
void test_quantum_hash(void) {
    vga_write("\n=== Quantum Hash Function Test ===\n");
    
    const char* data = "Aurora OS Quantum Cryptography";
    uint8_t hash[16];
    
    int result = quantum_hash((const uint8_t*)data, 30, hash, sizeof(hash));
    
    if (result == QCRYPTO_SUCCESS) {
        vga_write("Generated quantum-resistant hash\n");
        serial_write(SERIAL_COM1, "Hash: ");
        
        char hex[3];
        for (int i = 0; i < sizeof(hash); i++) {
            hex[0] = "0123456789ABCDEF"[hash[i] >> 4];
            hex[1] = "0123456789ABCDEF"[hash[i] & 0x0F];
            hex[2] = '\0';
            serial_write(SERIAL_COM1, hex);
        }
        serial_write(SERIAL_COM1, "\n");
        
        /* Test integrity verification */
        result = quantum_verify_integrity((const uint8_t*)data, 30, hash);
        if (result == QCRYPTO_SUCCESS) {
            vga_write("Integrity verification: PASSED\n");
        } else {
            vga_write("Integrity verification: FAILED\n");
        }
    } else {
        vga_write("ERROR: Hash generation failed\n");
    }
}

/**
 * Run all quantum encryption tests
 */
void run_quantum_crypto_tests(void) {
    vga_write("\n");
    vga_write("========================================\n");
    vga_write("  Aurora OS Quantum Encryption Tests   \n");
    vga_write("========================================\n");
    
    test_quantum_random();
    test_quantum_key_generation();
    test_quantum_encryption();
    test_quantum_hash();
    
    vga_write("\n========================================\n");
    vga_write("  All Quantum Encryption Tests Complete\n");
    vga_write("========================================\n\n");
}
