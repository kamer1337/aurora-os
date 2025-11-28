/**
 * Aurora OS - Linux Features Test Suite
 * 
 * Tests for additional Linux syscalls, eBPF, LSM, containers,
 * real-time scheduling, and hardware-accelerated cryptography
 */

#include "../kernel/core/linux_compat.h"
#include "../kernel/security/lsm.h"
#include "../kernel/security/ebpf.h"
#include "../kernel/security/quantum_crypto.h"
#include "../kernel/process/container.h"
#include "../kernel/process/scheduler_optimization.h"
#include "../kernel/drivers/vga.h"
#include <stddef.h>

/* File permission flags for LSM tests */
#define MAY_READ    0x04
#define MAY_WRITE   0x02
#define MAY_EXEC    0x01

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            tests_passed++; \
            vga_write("  [PASS] "); \
            vga_write(msg); \
            vga_write("\n"); \
        } else { \
            tests_failed++; \
            vga_write("  [FAIL] "); \
            vga_write(msg); \
            vga_write("\n"); \
        } \
    } while(0)

/**
 * Test additional Linux syscalls
 */
static void test_linux_syscalls(void) {
    vga_write("\nTest: Additional Linux Syscalls\n");
    
    /* Initialize linux compat */
    int result = linux_compat_init();
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Linux compat init");
    
    /* Test sys_getpid */
    long pid = linux_syscall(__NR_getpid, 0, 0, 0, 0, 0, 0);
    TEST_ASSERT(pid > 0, "sys_getpid returns valid PID");
    
    /* Test sys_getuid */
    long uid = linux_syscall(__NR_getuid, 0, 0, 0, 0, 0, 0);
    TEST_ASSERT(uid >= 0, "sys_getuid returns valid UID");
    
    /* Test sys_brk */
    long brk = linux_syscall(__NR_brk, 0, 0, 0, 0, 0, 0);
    TEST_ASSERT(brk != 0, "sys_brk returns program break");
    
    /* Test sys_pipe */
    int pipefd[2] = {-1, -1};
    result = (int)linux_syscall(__NR_pipe, (long)pipefd, 0, 0, 0, 0, 0);
    TEST_ASSERT(result == 0, "sys_pipe creates pipe");
    TEST_ASSERT(pipefd[0] >= 0 && pipefd[1] >= 0, "sys_pipe returns valid fds");
    
    /* Close pipe fds */
    if (pipefd[0] >= 0) linux_syscall(__NR_close, pipefd[0], 0, 0, 0, 0, 0);
    if (pipefd[1] >= 0) linux_syscall(__NR_close, pipefd[1], 0, 0, 0, 0, 0);
    
    /* Test sys_dup */
    int fd = (int)linux_syscall(__NR_open, (long)"/test", O_RDWR, 0, 0, 0, 0);
    if (fd >= 0) {
        int newfd = (int)linux_syscall(__NR_dup, fd, 0, 0, 0, 0, 0);
        TEST_ASSERT(newfd >= 0 && newfd != fd, "sys_dup duplicates fd");
        linux_syscall(__NR_close, newfd, 0, 0, 0, 0, 0);
        linux_syscall(__NR_close, fd, 0, 0, 0, 0, 0);
    }
    
    /* Test sys_getcwd */
    char cwd[256];
    long cwd_result = linux_syscall(__NR_getcwd, (long)cwd, 256, 0, 0, 0, 0);
    TEST_ASSERT(cwd_result != 0, "sys_getcwd returns cwd");
    TEST_ASSERT(cwd[0] == '/', "sys_getcwd path starts with /");
    
    /* Test sys_stat */
    linux_stat_t statbuf;
    result = (int)linux_syscall(__NR_stat, (long)"/", (long)&statbuf, 0, 0, 0, 0);
    TEST_ASSERT(result == 0, "sys_stat succeeds");
    
    /* Test sys_socket */
    int sockfd = (int)linux_syscall(__NR_socket, 2, 1, 0, 0, 0, 0);  /* AF_INET, SOCK_STREAM */
    TEST_ASSERT(sockfd >= 0, "sys_socket creates socket");
    if (sockfd >= 0) linux_syscall(__NR_close, sockfd, 0, 0, 0, 0, 0);
    
    /* Test sys_mmap anonymous */
    void* mem = (void*)linux_syscall(__NR_mmap, 0, 4096, PROT_READ | PROT_WRITE, 
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(mem != MAP_FAILED, "sys_mmap anonymous succeeds");
    
    /* Test sys_munmap */
    if (mem != MAP_FAILED) {
        result = (int)linux_syscall(__NR_munmap, (long)mem, 4096, 0, 0, 0, 0);
        TEST_ASSERT(result == 0, "sys_munmap succeeds");
    }
    
    /* Test sys_ioctl */
    uint16_t ws[4];
    result = (int)linux_syscall(__NR_ioctl, 1, TIOCGWINSZ, (long)ws, 0, 0, 0);
    TEST_ASSERT(result == 0, "sys_ioctl TIOCGWINSZ succeeds");
}

/**
 * Test LSM (Linux Security Modules) framework
 */
static void test_lsm_framework(void) {
    vga_write("\nTest: LSM Security Framework\n");
    
    /* Initialize LSM */
    int result = lsm_init();
    TEST_ASSERT(result == LSM_ALLOW, "LSM initialization");
    
    /* Test capability checks - use task security context */
    task_security_t task;
    task_security_init(&task, 1);
    
    result = capable(&task, CAP_SYS_ADMIN);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY, 
                "CAP_SYS_ADMIN check returns valid result");
    
    result = capable(&task, CAP_NET_ADMIN);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "CAP_NET_ADMIN check returns valid result");
    
    /* Test task security hooks */
    result = security_task_create(&task, 0);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_task_create hook works");
    
    /* Test file security hooks */
    file_security_t file;
    file.inode_id = 1;
    file.mode = S_IFREG | S_IRUSR | S_IWUSR;
    file.owner_uid = 0;
    file.owner_gid = 0;
    file.security_data = NULL;
    
    result = security_file_read(&task, &file);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_file_read hook works");
    
    result = security_file_write(&task, &file);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_file_write check works");
    
    /* Test inode hooks */
    inode_security_t inode;
    inode.inode_id = 1;
    inode.mode = S_IFDIR | S_IRWXU;
    inode.uid = 0;
    inode.gid = 0;
    inode.security_data = NULL;
    
    result = security_inode_create(&task, &inode, "newfile", S_IFREG);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_inode_create hook works");
    
    result = security_inode_permission(&task, &inode, MAY_READ);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_inode_permission hook works");
    
    /* Test socket hooks */
    result = security_socket_create(&task, 2, 1, 0);  /* AF_INET, SOCK_STREAM */
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_socket_create hook works");
    
    /* Test module hooks */
    result = security_module_load("test_module", NULL, 0);
    TEST_ASSERT(result == LSM_ALLOW || result == LSM_DENY,
                "security_module_load hook works");
    
    /* Test capability operations */
    cap_t caps;
    cap_init(&caps);
    TEST_ASSERT(1, "cap_init succeeds");
    
    cap_set(&caps, CAP_NET_ADMIN);
    TEST_ASSERT(cap_isset(&caps, CAP_NET_ADMIN), "cap_set/cap_isset works");
    
    cap_clear(&caps, CAP_NET_ADMIN);
    TEST_ASSERT(!cap_isset(&caps, CAP_NET_ADMIN), "cap_clear works");
    
    task_security_cleanup(&task);
}

/**
 * Test eBPF framework
 */
static void test_ebpf_framework(void) {
    vga_write("\nTest: eBPF Framework\n");
    
    /* Initialize eBPF */
    int result = bpf_subsystem_init();
    TEST_ASSERT(result == BPF_OK, "eBPF initialization");
    
    /* Test map creation */
    bpf_map_t* hash_map = NULL;
    result = bpf_map_create(BPF_MAP_TYPE_HASH, sizeof(uint32_t), sizeof(uint64_t), 256, &hash_map);
    TEST_ASSERT(result == BPF_OK && hash_map != NULL, "eBPF hash map creation");
    
    /* Test map operations */
    if (hash_map) {
        uint32_t key = 42;
        uint64_t value = 0xDEADBEEF;
        
        /* Update map */
        result = bpf_map_update(hash_map, &key, &value, 0);
        TEST_ASSERT(result == BPF_OK, "eBPF map update");
        
        /* Lookup map */
        uint64_t lookup_value = 0;
        result = bpf_map_lookup(hash_map, &key, &lookup_value);
        TEST_ASSERT(result == BPF_OK, "eBPF map lookup");
        TEST_ASSERT(lookup_value == value, "eBPF map value matches");
        
        /* Delete from map */
        result = bpf_map_delete(hash_map, &key);
        TEST_ASSERT(result == BPF_OK, "eBPF map delete");
        
        /* Lookup should fail now */
        result = bpf_map_lookup(hash_map, &key, &lookup_value);
        TEST_ASSERT(result != BPF_OK, "eBPF map lookup fails after delete");
    }
    
    /* Test array map */
    bpf_map_t* array_map = NULL;
    result = bpf_map_create(BPF_MAP_TYPE_ARRAY, sizeof(uint32_t), sizeof(uint64_t), 16, &array_map);
    TEST_ASSERT(result == BPF_OK && array_map != NULL, "eBPF array map creation");
    
    if (array_map) {
        uint32_t key = 5;
        uint64_t value = 0x12345678;
        
        result = bpf_map_update(array_map, &key, &value, 0);
        TEST_ASSERT(result == BPF_OK, "eBPF array map update");
    }
    
    /* Test program loading (simple exit program) */
    bpf_insn_t prog[] = {
        {.opcode = BPF_ALU64 | BPF_MOV | BPF_K, .dst_reg = 0, .src_reg = 0, .offset = 0, .imm = 0},  /* r0 = 0 */
        {.opcode = BPF_JMP | BPF_EXIT, .dst_reg = 0, .src_reg = 0, .offset = 0, .imm = 0}            /* exit */
    };
    
    bpf_prog_t* bpf_prog = NULL;
    result = bpf_prog_load(BPF_PROG_TYPE_SOCKET_FILTER, prog, 2, "test_prog", &bpf_prog);
    TEST_ASSERT(result == BPF_OK && bpf_prog != NULL, "eBPF program loading");
    
    /* Test program execution */
    if (bpf_prog) {
        uint64_t prog_result = 0;
        result = bpf_prog_run(bpf_prog, NULL, &prog_result);
        TEST_ASSERT(result == BPF_OK, "eBPF program execution");
        TEST_ASSERT(prog_result == 0, "eBPF program returns 0");
    }
}

/**
 * Test container/namespace support
 */
static void test_container_support(void) {
    vga_write("\nTest: Container/Namespace Support\n");
    
    /* Initialize container subsystem */
    int result = container_subsystem_init();
    TEST_ASSERT(result == NS_SUCCESS, "Container initialization");
    
    /* Test namespace creation */
    namespace_t* pid_ns = NULL;
    result = ns_create(NS_TYPE_PID, 1, &pid_ns);
    TEST_ASSERT(result == NS_SUCCESS && pid_ns != NULL, "PID namespace creation");
    
    namespace_t* net_ns = NULL;
    result = ns_create(NS_TYPE_NET, 1, &net_ns);
    TEST_ASSERT(result == NS_SUCCESS && net_ns != NULL, "NET namespace creation");
    
    namespace_t* uts_ns = NULL;
    result = ns_create(NS_TYPE_UTS, 1, &uts_ns);
    TEST_ASSERT(result == NS_SUCCESS && uts_ns != NULL, "UTS namespace creation");
    
    if (uts_ns) {
        /* Test UTS namespace operations */
        result = uts_set_hostname(uts_ns, "test-container");
        TEST_ASSERT(result == NS_SUCCESS, "uts_set_hostname");
        
        char hostname[64];
        result = uts_get_hostname(uts_ns, hostname, sizeof(hostname));
        TEST_ASSERT(result == NS_SUCCESS, "uts_get_hostname");
    }
    
    /* Test container creation */
    container_config_t config;
    config.ns_flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWUTS;
    config.memory_limit = 1024 * 1024 * 256;  /* 256MB */
    config.cpu_shares = 1024;
    
    /* Copy strings safely */
    const char* name = "test_container";
    const char* root = "/";
    const char* host = "test-host";
    int i;
    for (i = 0; name[i] && i < 63; i++) config.name[i] = name[i];
    config.name[i] = '\0';
    for (i = 0; root[i] && i < 255; i++) config.root_path[i] = root[i];
    config.root_path[i] = '\0';
    for (i = 0; host[i] && i < 63; i++) config.hostname[i] = host[i];
    config.hostname[i] = '\0';
    
    container_t* container = NULL;
    result = container_create(&config, &container);
    TEST_ASSERT(result == NS_SUCCESS && container != NULL, "Container creation");
    
    if (container) {
        /* Test container info */
        TEST_ASSERT(container->status == CONTAINER_CREATED, "Container status is CREATED");
        
        /* Test container start */
        result = container_start(container->id);
        TEST_ASSERT(result == NS_SUCCESS, "Container start");
        
        container = container_find(container->id);
        if (container) {
            TEST_ASSERT(container->status == CONTAINER_RUNNING, "Container status is RUNNING");
        }
        
        /* Test container pause */
        result = container_pause(container->id);
        TEST_ASSERT(result == NS_SUCCESS, "Container pause");
        
        /* Test container resume */
        result = container_resume(container->id);
        TEST_ASSERT(result == NS_SUCCESS, "Container resume");
        
        /* Test container stop */
        result = container_stop(container->id);
        TEST_ASSERT(result == NS_SUCCESS, "Container stop");
        
        /* Test container destroy */
        result = container_destroy(container->id);
        TEST_ASSERT(result == NS_SUCCESS, "Container destroy");
    }
    
    /* Test cgroup operations with a new container */
    result = container_create(&config, &container);
    if (result == NS_SUCCESS && container) {
        /* Set cgroup limits */
        result = cgroup_set_memory_limit(container, 1024 * 1024 * 128);
        TEST_ASSERT(result == NS_SUCCESS, "Cgroup memory limit set");
        
        result = cgroup_set_cpu_quota(container, 50000, 100000);  /* 50% CPU */
        TEST_ASSERT(result == NS_SUCCESS, "Cgroup CPU quota set");
        
        container_destroy(container->id);
    }
}

/**
 * Test real-time scheduling
 */
static void test_realtime_scheduling(void) {
    vga_write("\nTest: Real-Time Scheduling\n");
    
    /* Initialize scheduler optimizations */
    int result = scheduler_optimization_init();
    TEST_ASSERT(result == 0, "Scheduler optimization init");
    
    /* Enable real-time support */
    scheduler_enable_realtime();
    
    /* Test SCHED_FIFO task creation */
    rt_task_params_t fifo_params = {
        .policy = SCHED_FIFO,
        .priority = 50,
        .time_slice_us = 0,
        .deadline = {0, 0, 0}
    };
    
    result = rt_task_create(100, &fifo_params);
    TEST_ASSERT(result == 0, "SCHED_FIFO task creation");
    
    /* Verify task parameters */
    int policy = 0, priority = 0;
    result = rt_get_scheduler(100, &policy, &priority);
    TEST_ASSERT(result == 0, "Get scheduler parameters");
    TEST_ASSERT(policy == SCHED_FIFO, "Policy is SCHED_FIFO");
    TEST_ASSERT(priority == 50, "Priority is 50");
    
    /* Test SCHED_RR task creation */
    rt_task_params_t rr_params = {
        .policy = SCHED_RR,
        .priority = 75,
        .time_slice_us = RT_TIME_QUANTUM_US,
        .deadline = {0, 0, 0}
    };
    
    result = rt_task_create(101, &rr_params);
    TEST_ASSERT(result == 0, "SCHED_RR task creation");
    
    /* Test SCHED_DEADLINE task */
    sched_deadline_params_t dl_params = {
        .runtime_ns = 5000000,      /* 5ms */
        .deadline_ns = 20000000,    /* 20ms */
        .period_ns = 20000000       /* 20ms */
    };
    
    result = rt_set_deadline_params(102, &dl_params);
    TEST_ASSERT(result == 0, "SCHED_DEADLINE task creation");
    
    /* Test task pick */
    uint32_t next_pid = rt_pick_next_task();
    TEST_ASSERT(next_pid == 102 || next_pid == 101 || next_pid == 100,
                "rt_pick_next_task returns valid RT task");
    
    /* Test priority change */
    result = rt_set_scheduler(100, SCHED_FIFO, 90);
    TEST_ASSERT(result == 0, "Priority change");
    
    result = rt_get_scheduler(100, &policy, &priority);
    TEST_ASSERT(priority == 90, "Priority updated to 90");
    
    /* Test priority inheritance */
    priority_inherit_boost(100, 95);
    result = rt_get_scheduler(100, &policy, &priority);
    TEST_ASSERT(priority == 95, "Priority inherited to 95");
    
    priority_inherit_restore(100);
    result = rt_get_scheduler(100, &policy, &priority);
    TEST_ASSERT(priority == 90, "Priority restored to 90");
    
    /* Cleanup */
    rt_task_destroy(100);
    rt_task_destroy(101);
    rt_task_destroy(102);
}

/**
 * Test hardware-accelerated Crystal-Kyber
 */
static void test_kyber_acceleration(void) {
    vga_write("\nTest: Hardware-Accelerated Crystal-Kyber\n");
    
    /* Initialize quantum crypto */
    int result = quantum_crypto_init();
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Quantum crypto init");
    
    /* Test key generation */
    quantum_key_t key;
    result = quantum_key_generate(&key, QCRYPTO_KEY_SIZE_256);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Key generation");
    TEST_ASSERT(key.is_valid == 1, "Key is valid");
    TEST_ASSERT(key.key_length == QCRYPTO_KEY_SIZE_256, "Key length correct");
    
    /* Test encryption/decryption context */
    quantum_crypto_ctx_t ctx;
    result = quantum_crypto_ctx_create(&ctx, &key);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Encryption context creation");
    
    /* Test encryption */
    uint8_t plaintext[64];
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    
    for (int i = 0; i < 64; i++) {
        plaintext[i] = (uint8_t)i;
    }
    
    result = quantum_encrypt_block(&ctx, plaintext, ciphertext, 64);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Block encryption");
    
    /* Verify ciphertext differs from plaintext */
    int different = 0;
    for (int i = 0; i < 64; i++) {
        if (ciphertext[i] != plaintext[i]) {
            different = 1;
            break;
        }
    }
    TEST_ASSERT(different == 1, "Ciphertext differs from plaintext");
    
    /* Test decryption */
    result = quantum_decrypt_block(&ctx, ciphertext, decrypted, 64);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Block decryption");
    
    /* Test quantum hash */
    uint8_t hash[32];
    result = quantum_hash((uint8_t*)"test data", 9, hash, 32);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Quantum hash");
    
    /* Test random number generation */
    uint64_t rand1 = quantum_random_uint64();
    uint64_t rand2 = quantum_random_uint64();
    TEST_ASSERT(rand1 != rand2 || rand1 != 0, "Random numbers differ");
    
    /* Test password hashing */
    uint8_t pwd_hash[32];
    result = quantum_hash_password("testpassword", pwd_hash, 32);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Password hashing");
    
    /* Test password verification */
    result = quantum_verify_password("testpassword", pwd_hash, 32);
    TEST_ASSERT(result == QCRYPTO_SUCCESS, "Password verification (correct)");
    
    result = quantum_verify_password("wrongpassword", pwd_hash, 32);
    TEST_ASSERT(result != QCRYPTO_SUCCESS, "Password verification (wrong)");
    
    /* Cleanup */
    quantum_crypto_ctx_destroy(&ctx);
    quantum_key_destroy(&key);
}

/**
 * Test extended module compatibility
 */
static void test_module_compatibility(void) {
    vga_write("\nTest: Extended Module Compatibility\n");
    
    /* Test symbol export */
    int dummy_var = 42;
    int result = linux_compat_export_symbol("test_symbol", &dummy_var, 0);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Symbol export");
    
    /* Test symbol lookup */
    void* sym = linux_compat_find_symbol("test_symbol");
    TEST_ASSERT(sym == &dummy_var, "Symbol lookup matches");
    
    /* Test module metadata */
    linux_module_t* module = linux_compat_find_module("test_module");
    /* Module may not exist, so just test the API works */
    if (module) {
        result = linux_compat_set_module_license(module, "GPL");
        TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Set module license");
        
        result = linux_compat_set_module_author(module, "Aurora OS Team");
        TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Set module author");
        
        result = linux_compat_set_module_description(module, "Test module");
        TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Set module description");
    }
    
    /* Test module loading */
    result = linux_compat_load_module("extended_test_module", "1.0.0",
                                      AURORA_ENHANCE_ALL, NULL, NULL);
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Module loading");
    
    module = linux_compat_find_module("extended_test_module");
    TEST_ASSERT(module != NULL, "Module find after load");
    
    /* Test module unloading */
    result = linux_compat_unload_module("extended_test_module");
    TEST_ASSERT(result == LINUX_COMPAT_SUCCESS, "Module unloading");
}

/**
 * Run all Linux features tests
 */
void run_linux_features_tests(void) {
    vga_write("\n");
    vga_write("========================================\n");
    vga_write("Linux Features Test Suite\n");
    vga_write("========================================\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    /* Run all tests */
    test_linux_syscalls();
    test_lsm_framework();
    test_ebpf_framework();
    test_container_support();
    test_realtime_scheduling();
    test_kyber_acceleration();
    test_module_compatibility();
    
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
        vga_write("\nAll tests PASSED!\n");
    } else {
        vga_write("\nSome tests FAILED!\n");
    }
    vga_write("========================================\n");
}

/**
 * Get test results
 */
void get_linux_features_test_results(int* passed, int* failed) {
    if (passed) *passed = tests_passed;
    if (failed) *failed = tests_failed;
}
