/**
 * Aurora OS - Enhanced Linux Kernel with Crystal-Kyber Optimization
 * 
 * This module provides a full Linux kernel compatibility layer with
 * Aurora-specific enhancements including:
 * - Crystal-Kyber post-quantum encryption
 * - Performance optimizations
 * - Security enhancements
 * - Quantum computing support
 */

#ifndef AURORA_LINUX_KERNEL_H
#define AURORA_LINUX_KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include "linux_compat.h"

/* Aurora Linux Kernel version */
#define AURORA_LINUX_KERNEL_MAJOR 6
#define AURORA_LINUX_KERNEL_MINOR 6
#define AURORA_LINUX_KERNEL_PATCH 0
#define AURORA_LINUX_KERNEL_CODENAME "Aurora Enhanced"

/* Kernel feature flags */
#define KERNEL_FEATURE_SMP           (1 << 0)
#define KERNEL_FEATURE_PREEMPT       (1 << 1)
#define KERNEL_FEATURE_MODULES       (1 << 2)
#define KERNEL_FEATURE_NETWORKING    (1 << 3)
#define KERNEL_FEATURE_USB           (1 << 4)
#define KERNEL_FEATURE_FILESYSTEM    (1 << 5)
#define KERNEL_FEATURE_CRYPTO        (1 << 6)
#define KERNEL_FEATURE_VIRTUALIZATION (1 << 7)

/* Aurora optimization levels */
typedef enum {
    AURORA_OPT_NONE = 0,
    AURORA_OPT_MINIMAL = 1,
    AURORA_OPT_BALANCED = 2,
    AURORA_OPT_AGGRESSIVE = 3,
    AURORA_OPT_MAXIMUM = 4
} aurora_optimization_level_t;

/* Crystal-Kyber encryption modes */
typedef enum {
    KYBER_MODE_DISABLED = 0,
    KYBER_MODE_512 = 1,   /* Security level 1 */
    KYBER_MODE_768 = 2,   /* Security level 3 */
    KYBER_MODE_1024 = 3   /* Security level 5 */
} kyber_encryption_mode_t;

/**
 * Aurora Linux Kernel configuration
 */
typedef struct {
    uint32_t kernel_features;
    aurora_optimization_level_t optimization_level;
    kyber_encryption_mode_t kyber_mode;
    uint8_t enable_quantum_rng;
    uint8_t enable_secure_boot;
    uint8_t enable_memory_encryption;
    uint8_t enable_network_encryption;
    uint32_t max_modules;
    uint32_t memory_limit_mb;
} aurora_linux_kernel_config_t;

/**
 * Kernel statistics
 */
typedef struct {
    uint64_t boot_time;
    uint64_t uptime_seconds;
    uint64_t context_switches;
    uint64_t interrupts;
    uint64_t syscalls;
    uint64_t page_faults;
    uint64_t crypto_operations;
    uint64_t network_packets_tx;
    uint64_t network_packets_rx;
    uint32_t processes_created;
    uint32_t modules_loaded;
} aurora_kernel_stats_t;

/* Kernel initialization and shutdown */
int aurora_linux_kernel_init(const aurora_linux_kernel_config_t* config);
void aurora_linux_kernel_shutdown(void);
int aurora_linux_kernel_reboot(void);

/* Module management */
int aurora_kernel_register_module(const char* name, const char* description,
                                 void (*init)(void), void (*exit)(void));
int aurora_kernel_unregister_module(const char* name);

/* Crystal-Kyber encryption interface */
int aurora_kernel_kyber_init(kyber_encryption_mode_t mode);
int aurora_kernel_kyber_encrypt_memory(void* addr, size_t size);
int aurora_kernel_kyber_decrypt_memory(void* addr, size_t size);
int aurora_kernel_kyber_encrypt_disk(uint8_t disk_id, uint8_t partition_id);
int aurora_kernel_kyber_decrypt_disk(uint8_t disk_id, uint8_t partition_id);

/* Performance optimizations */
void aurora_kernel_optimize_memory_allocator(void);
void aurora_kernel_optimize_page_cache(void);
void aurora_kernel_optimize_scheduler(void);
void aurora_kernel_optimize_io_scheduler(void);
void aurora_kernel_optimize_network_stack(void);
void aurora_kernel_apply_all_optimizations(aurora_optimization_level_t level);

/* Security enhancements */
int aurora_kernel_enable_secure_boot(void);
int aurora_kernel_enable_memory_protection(void);
int aurora_kernel_enable_kernel_hardening(void);
int aurora_kernel_enable_exploit_mitigation(void);

/* Quantum computing support */
int aurora_kernel_init_quantum_rng(void);
uint64_t aurora_kernel_quantum_random(void);
int aurora_kernel_quantum_encrypt(const void* data, size_t size, void* out);
int aurora_kernel_quantum_decrypt(const void* data, size_t size, void* out);

/* System information */
const char* aurora_kernel_get_version_string(void);
uint32_t aurora_kernel_get_version_code(void);
const aurora_linux_kernel_config_t* aurora_kernel_get_config(void);
const aurora_kernel_stats_t* aurora_kernel_get_stats(void);

/* Power management */
int aurora_kernel_suspend(void);
int aurora_kernel_resume(void);
int aurora_kernel_hibernate(void);
void aurora_kernel_power_off(void);

/* Debugging and diagnostics */
void aurora_kernel_print_info(void);
void aurora_kernel_print_stats(void);
void aurora_kernel_print_modules(void);
void aurora_kernel_print_memory_map(void);

#endif /* AURORA_LINUX_KERNEL_H */
