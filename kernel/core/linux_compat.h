/**
 * Aurora OS - Linux Kernel Compatibility Layer
 * 
 * Provides Linux kernel compatibility with Aurora enhancements
 * including Crystal-Kyber encryption and performance optimizations
 */

#ifndef AURORA_LINUX_COMPAT_H
#define AURORA_LINUX_COMPAT_H

#include <stdint.h>
#include <stddef.h>

/* Linux kernel compatibility version */
#define AURORA_LINUX_COMPAT_VERSION "1.0.0"
#define AURORA_LINUX_KERNEL_VERSION "6.6-aurora"

/* Linux kernel compatibility status codes */
#define LINUX_COMPAT_SUCCESS 0
#define LINUX_COMPAT_ERROR -1
#define LINUX_COMPAT_NOT_INITIALIZED -2
#define LINUX_COMPAT_INVALID_PARAM -3

/* Aurora enhancement flags */
#define AURORA_ENHANCE_CRYPTO     (1 << 0)  /* Crystal-Kyber encryption */
#define AURORA_ENHANCE_PERF       (1 << 1)  /* Performance optimizations */
#define AURORA_ENHANCE_SECURITY   (1 << 2)  /* Enhanced security features */
#define AURORA_ENHANCE_QUANTUM    (1 << 3)  /* Quantum computing support */
#define AURORA_ENHANCE_ALL        (0xFF)    /* All enhancements */

/**
 * Linux kernel module structure
 */
typedef struct {
    const char* name;
    const char* version;
    uint32_t enhancement_flags;
    void (*init_func)(void);
    void (*cleanup_func)(void);
    uint8_t is_loaded;
    uint32_t load_address;
    size_t module_size;
} linux_module_t;

/**
 * Aurora enhancement context
 */
typedef struct {
    uint8_t crypto_enabled;
    uint8_t perf_optimization_enabled;
    uint8_t quantum_enabled;
    uint32_t enhancement_flags;
    uint64_t optimizations_applied;
    uint64_t crypto_operations;
} aurora_enhancement_ctx_t;

/**
 * Linux kernel compatibility context
 */
typedef struct {
    uint8_t initialized;
    uint32_t module_count;
    linux_module_t modules[32];
    aurora_enhancement_ctx_t enhancement_ctx;
    uint64_t uptime_ticks;
} linux_compat_ctx_t;

/* Initialization and cleanup */
int linux_compat_init(void);
void linux_compat_cleanup(void);

/* Module management */
int linux_compat_load_module(const char* name, const char* version,
                             uint32_t enhancement_flags,
                             void (*init_func)(void),
                             void (*cleanup_func)(void));
int linux_compat_unload_module(const char* name);
linux_module_t* linux_compat_find_module(const char* name);
void linux_compat_list_modules(void);

/* Aurora enhancements */
int aurora_enable_crypto_enhancement(void);
int aurora_enable_perf_optimization(void);
int aurora_enable_quantum_support(void);
int aurora_apply_enhancement(uint32_t flags);

/* Crystal-Kyber integration */
int linux_compat_kyber_encrypt(const uint8_t* plaintext, size_t length,
                               uint8_t* ciphertext, size_t* out_length);
int linux_compat_kyber_decrypt(const uint8_t* ciphertext, size_t length,
                               uint8_t* plaintext, size_t* out_length);

/* Performance optimizations */
void linux_compat_optimize_memory(void);
void linux_compat_optimize_io(void);
void linux_compat_optimize_scheduling(void);

/* Status and statistics */
const linux_compat_ctx_t* linux_compat_get_context(void);
void linux_compat_print_stats(void);

/* Linux kernel syscall compatibility */
long linux_syscall(long syscall_num, long arg1, long arg2, long arg3, 
                   long arg4, long arg5, long arg6);

#endif /* AURORA_LINUX_COMPAT_H */
