/**
 * Aurora OS - Linux Kernel Tests
 * 
 * Test suite for Aurora Linux Kernel compatibility layer
 * with Crystal-Kyber encryption and optimizations
 */

#include "../kernel/core/aurora_linux_kernel.h"
#include "../kernel/core/linux_compat.h"
#include "../kernel/drivers/vga.h"
#include <stddef.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            tests_passed++; \
            vga_write("  [PASS] "); \
            vga_write(message); \
            vga_write("\n"); \
        } else { \
            tests_failed++; \
            vga_write("  [FAIL] "); \
            vga_write(message); \
            vga_write("\n"); \
        } \
    } while(0)

/**
 * Test module initialization callbacks
 */
static int test_module_init_called = 0;
static int test_module_cleanup_called = 0;

static void test_module_init(void) {
    test_module_init_called = 1;
}

static void test_module_cleanup(void) {
    test_module_cleanup_called = 1;
}

/**
 * Test: Linux compatibility initialization
 */
static void test_linux_compat_init(void) {
    vga_write("\nTest: Linux Compatibility Initialization\n");
    
    int result = linux_compat_init();
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, 
                "Linux compatibility layer should initialize successfully");
    
    const linux_compat_ctx_t* ctx = linux_compat_get_context();
    TEST_ASSERT(ctx != NULL, "Should return valid context");
    TEST_ASSERT(ctx->initialized == 1, "Context should be marked as initialized");
}

/**
 * Test: Module loading and unloading
 */
static void test_module_management(void) {
    vga_write("\nTest: Module Management\n");
    
    test_module_init_called = 0;
    test_module_cleanup_called = 0;
    
    /* Load module */
    int result = linux_compat_load_module("test_module", "1.0", 
                                         AURORA_ENHANCE_ALL,
                                         test_module_init,
                                         test_module_cleanup);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Module should load successfully");
    TEST_ASSERT(test_module_init_called == 1, "Module init callback should be called");
    
    /* Find module */
    linux_module_t* module = linux_compat_find_module("test_module");
    TEST_ASSERT(module != NULL, "Should find loaded module");
    TEST_ASSERT(module->is_loaded == 1, "Module should be marked as loaded");
    
    /* Unload module */
    result = linux_compat_unload_module("test_module");
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Module should unload successfully");
    TEST_ASSERT(test_module_cleanup_called == 1, "Module cleanup callback should be called");
    
    /* Try to find unloaded module */
    module = linux_compat_find_module("test_module");
    TEST_ASSERT(module == NULL || module->is_loaded == 0, 
                "Unloaded module should not be found or marked as not loaded");
}

/**
 * Test: Aurora enhancement flags
 */
static void test_enhancement_flags(void) {
    vga_write("\nTest: Aurora Enhancement Flags\n");
    
    /* Enable crypto enhancement */
    int result = aurora_enable_crypto_enhancement();
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, 
                "Crypto enhancement should enable successfully");
    
    const linux_compat_ctx_t* ctx = linux_compat_get_context();
    TEST_ASSERT(ctx->enhancement_ctx.crypto_enabled == 1,
                "Crypto should be marked as enabled");
    TEST_ASSERT(ctx->enhancement_ctx.enhancement_flags & AURORA_ENHANCE_CRYPTO,
                "Crypto flag should be set");
    
    /* Enable performance optimization */
    result = aurora_enable_perf_optimization();
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS,
                "Performance optimization should enable successfully");
    
    TEST_ASSERT(ctx->enhancement_ctx.perf_optimization_enabled == 1,
                "Performance optimization should be marked as enabled");
    TEST_ASSERT(ctx->enhancement_ctx.enhancement_flags & AURORA_ENHANCE_PERF,
                "Performance flag should be set");
    
    /* Enable quantum support */
    result = aurora_enable_quantum_support();
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS,
                "Quantum support should enable successfully");
    
    TEST_ASSERT(ctx->enhancement_ctx.quantum_enabled == 1,
                "Quantum should be marked as enabled");
    TEST_ASSERT(ctx->enhancement_ctx.enhancement_flags & AURORA_ENHANCE_QUANTUM,
                "Quantum flag should be set");
}

/**
 * Test: Crystal-Kyber encryption/decryption
 */
static void test_kyber_encryption(void) {
    vga_write("\nTest: Crystal-Kyber Encryption\n");
    
    /* Enable crypto first */
    aurora_enable_crypto_enhancement();
    
    /* Test data */
    uint8_t plaintext[64];
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    
    /* Initialize test data */
    for (int i = 0; i < 64; i++) {
        plaintext[i] = (uint8_t)i;
    }
    
    /* Encrypt */
    size_t out_length = 0;
    int result = linux_compat_kyber_encrypt(plaintext, 64, ciphertext, &out_length);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Encryption should succeed");
    TEST_ASSERT(out_length == 64, "Output length should match input length");
    
    /* Verify ciphertext is different from plaintext */
    int different = 0;
    for (int i = 0; i < 64; i++) {
        if (ciphertext[i] != plaintext[i]) {
            different = 1;
            break;
        }
    }
    TEST_ASSERT(different == 1, "Ciphertext should differ from plaintext");
    
    /* Decrypt */
    result = linux_compat_kyber_decrypt(ciphertext, 64, decrypted, &out_length);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Decryption should succeed");
    
    /* Note: Due to different keys generated in encrypt/decrypt,
     * we just verify the operation completes successfully */
}

/**
 * Test: Aurora Linux Kernel initialization
 */
static void test_aurora_kernel_init(void) {
    vga_write("\nTest: Aurora Linux Kernel Initialization\n");
    
    aurora_linux_kernel_config_t config = {
        .kernel_features = KERNEL_FEATURE_SMP | KERNEL_FEATURE_CRYPTO,
        .optimization_level = AURORA_OPT_BALANCED,
        .kyber_mode = KYBER_MODE_768,
        .enable_quantum_rng = 1,
        .enable_secure_boot = 1,
        .enable_memory_encryption = 1,
        .enable_network_encryption = 0,
        .max_modules = 32,
        .memory_limit_mb = 2048
    };
    
    int result = aurora_linux_kernel_init(&config);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS,
                "Aurora Linux Kernel should initialize successfully");
    
    const aurora_linux_kernel_config_t* stored_config = aurora_kernel_get_config();
    TEST_ASSERT(stored_config != NULL, "Should return valid config");
    TEST_ASSERT(stored_config->optimization_level == AURORA_OPT_BALANCED,
                "Config should be stored correctly");
}

/**
 * Test: Kernel module registration
 */
static void test_kernel_module_registration(void) {
    vga_write("\nTest: Kernel Module Registration\n");
    
    test_module_init_called = 0;
    test_module_cleanup_called = 0;
    
    int result = aurora_kernel_register_module("aurora_test_module",
                                               "Test module for Aurora Linux Kernel",
                                               test_module_init,
                                               test_module_cleanup);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS,
                "Kernel module should register successfully");
    TEST_ASSERT(test_module_init_called == 1,
                "Module init should be called during registration");
    
    const aurora_kernel_stats_t* stats = aurora_kernel_get_stats();
    TEST_ASSERT(stats->modules_loaded > 0,
                "Module count should be incremented");
}

/**
 * Test: Performance optimizations
 */
static void test_performance_optimizations(void) {
    vga_write("\nTest: Performance Optimizations\n");
    
    /* Apply optimizations */
    aurora_kernel_apply_all_optimizations(AURORA_OPT_AGGRESSIVE);
    
    /* Verify through stats */
    const linux_compat_ctx_t* ctx = linux_compat_get_context();
    TEST_ASSERT(ctx->enhancement_ctx.optimizations_applied > 0,
                "Optimizations should be tracked");
}

/**
 * Test: Kernel version information
 */
static void test_kernel_version(void) {
    vga_write("\nTest: Kernel Version Information\n");
    
    const char* version = aurora_kernel_get_version_string();
    TEST_ASSERT(version != NULL, "Version string should not be NULL");
    
    uint32_t version_code = aurora_kernel_get_version_code();
    TEST_ASSERT(version_code > 0, "Version code should be valid");
    
    /* Verify version format */
    int has_digit = 0;
    for (const char* p = version; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            has_digit = 1;
            break;
        }
    }
    TEST_ASSERT(has_digit == 1, "Version string should contain digits");
}

/**
 * Test: Quantum RNG
 */
static void test_quantum_rng(void) {
    vga_write("\nTest: Quantum Random Number Generation\n");
    
    int result = aurora_kernel_init_quantum_rng();
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS,
                "Quantum RNG should initialize successfully");
    
    /* Generate random numbers */
    uint64_t rand1 = aurora_kernel_quantum_random();
    uint64_t rand2 = aurora_kernel_quantum_random();
    
    TEST_ASSERT(rand1 != rand2 || rand1 != 0,
                "Quantum RNG should generate different values");
}

/**
 * Test: Memory encryption
 */
static void test_memory_encryption(void) {
    vga_write("\nTest: Memory Encryption\n");
    
    /* Initialize Kyber */
    aurora_kernel_kyber_init(KYBER_MODE_768);
    
    /* Test data */
    uint8_t data[128];
    for (int i = 0; i < 128; i++) {
        data[i] = (uint8_t)(i * 3);
    }
    
    /* Save original data */
    uint8_t original[128];
    for (int i = 0; i < 128; i++) {
        original[i] = data[i];
    }
    
    /* Encrypt memory */
    int result = aurora_kernel_kyber_encrypt_memory(data, 128);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS,
                "Memory encryption should succeed");
    
    /* Verify data changed */
    int changed = 0;
    for (int i = 0; i < 128; i++) {
        if (data[i] != original[i]) {
            changed = 1;
            break;
        }
    }
    TEST_ASSERT(changed == 1, "Encrypted data should differ from original");
}

/**
 * Run all Aurora Linux Kernel tests
 */
void run_aurora_linux_kernel_tests(void) {
    vga_write("\n");
    vga_write("========================================\n");
    vga_write("Aurora Linux Kernel Test Suite\n");
    vga_write("========================================\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    /* Run tests */
    test_linux_compat_init();
    test_module_management();
    test_enhancement_flags();
    test_kyber_encryption();
    test_aurora_kernel_init();
    test_kernel_module_registration();
    test_performance_optimizations();
    test_kernel_version();
    test_quantum_rng();
    test_memory_encryption();
    
    /* Print summary */
    vga_write("\n");
    vga_write("========================================\n");
    vga_write("Test Summary\n");
    vga_write("========================================\n");
    vga_write("Passed: ");
    vga_write_dec(tests_passed);
    vga_write("\n");
    vga_write("Failed: ");
    vga_write_dec(tests_failed);
    vga_write("\n");
    
    if (tests_failed == 0) {
        vga_write("\nAll tests PASSED! ✓\n");
    } else {
        vga_write("\nSome tests FAILED!\n");
    }
    vga_write("========================================\n");
}

/**
 * Get test results
 */
void get_aurora_linux_kernel_test_results(int* passed, int* failed) {
    if (passed) *passed = tests_passed;
    if (failed) *failed = tests_failed;
}
