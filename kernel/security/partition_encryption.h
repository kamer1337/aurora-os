/**
 * Aurora OS - Partition Encryption with Crystal-Kyber
 * 
 * Provides post-quantum encryption for disk partitions
 * Based on CRYSTALS-Kyber (NIST PQC standard)
 */

#ifndef AURORA_PARTITION_ENCRYPTION_H
#define AURORA_PARTITION_ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

/* Kyber security levels */
#define KYBER_512   1  /* Security level 1 */
#define KYBER_768   2  /* Security level 3 */
#define KYBER_1024  3  /* Security level 5 */

/* Key sizes */
#define KYBER_PUBLIC_KEY_SIZE    1568
#define KYBER_SECRET_KEY_SIZE    3168
#define KYBER_CIPHERTEXT_SIZE    1568
#define KYBER_SHARED_SECRET_SIZE 32

/* Encryption key structure */
typedef struct {
    uint8_t public_key[KYBER_PUBLIC_KEY_SIZE];
    uint8_t secret_key[KYBER_SECRET_KEY_SIZE];
    uint8_t shared_secret[KYBER_SHARED_SECRET_SIZE];
    uint8_t security_level;
    uint8_t is_valid;
} kyber_key_t;

/* Encrypted partition metadata */
typedef struct {
    uint8_t disk_id;
    uint8_t partition_id;
    uint8_t encrypted;
    uint32_t encryption_version;
    kyber_key_t encryption_key;
    uint8_t salt[32];
    uint64_t encrypted_sectors;
} encrypted_partition_t;

/* Initialize encryption subsystem */
void partition_encryption_init(void);

/* Key management */
int partition_encryption_generate_keypair(kyber_key_t* key, uint8_t security_level);
int partition_encryption_derive_key(const char* password, uint8_t* key_out, size_t key_len);

/* Partition encryption operations */
int partition_encrypt(uint8_t disk_id, uint8_t partition_id, const char* password);
int partition_decrypt(uint8_t disk_id, uint8_t partition_id, const char* password);
int partition_is_encrypted(uint8_t disk_id, uint8_t partition_id);

/* Mount/unmount encrypted partitions */
int partition_mount_encrypted(uint8_t disk_id, uint8_t partition_id, const char* password);
int partition_unmount_encrypted(uint8_t disk_id, uint8_t partition_id);

/* Sector-level encryption/decryption */
int partition_encrypt_sector(encrypted_partition_t* part, uint32_t sector_num, 
                             const uint8_t* data_in, uint8_t* data_out);
int partition_decrypt_sector(encrypted_partition_t* part, uint32_t sector_num,
                             const uint8_t* data_in, uint8_t* data_out);

/* Change encryption password */
int partition_change_password(uint8_t disk_id, uint8_t partition_id,
                              const char* old_password, const char* new_password);

/* Get encryption status */
const encrypted_partition_t* partition_get_encryption_info(uint8_t disk_id, uint8_t partition_id);

#endif /* AURORA_PARTITION_ENCRYPTION_H */
