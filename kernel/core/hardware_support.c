/**
 * Aurora OS - Hardware Support Expansion
 * 
 * This module provides implementations for various hardware platform support
 * including Intel, AMD processors and multi-core scaling using CPUID instruction.
 */

#include "hardware_support.h"

/* CPU vendor identification strings */
#define CPUID_VENDOR_INTEL "GenuineIntel"
#define CPUID_VENDOR_AMD   "AuthenticAMD"

/* CPUID feature flags (EDX from leaf 1) */
#define CPUID_FEAT_EDX_FPU      (1 << 0)
#define CPUID_FEAT_EDX_VME      (1 << 1)
#define CPUID_FEAT_EDX_PSE      (1 << 3)
#define CPUID_FEAT_EDX_TSC      (1 << 4)
#define CPUID_FEAT_EDX_MSR      (1 << 5)
#define CPUID_FEAT_EDX_PAE      (1 << 6)
#define CPUID_FEAT_EDX_APIC     (1 << 9)
#define CPUID_FEAT_EDX_SSE      (1 << 25)
#define CPUID_FEAT_EDX_SSE2     (1 << 26)
#define CPUID_FEAT_EDX_HTT      (1 << 28)

/* CPUID feature flags (ECX from leaf 1) */
#define CPUID_FEAT_ECX_SSE3     (1 << 0)
#define CPUID_FEAT_ECX_SSSE3    (1 << 9)
#define CPUID_FEAT_ECX_SSE41    (1 << 19)
#define CPUID_FEAT_ECX_SSE42    (1 << 20)
#define CPUID_FEAT_ECX_AVX      (1 << 28)

/* CPU feature capabilities detected */
typedef struct {
    uint32_t sse_support;       /* SSE instruction support level (0-4) */
    uint32_t avx_support;       /* AVX support (0 or 1) */
    uint32_t fpu_present;       /* x87 FPU present */
    uint32_t apic_present;      /* Local APIC present */
    uint32_t htt_capable;       /* Hyper-threading capable */
    uint32_t vmx_capable;       /* Intel VT-x capable */
    uint32_t svm_capable;       /* AMD-V capable */
} cpu_features_t;

/* Per-CPU data structure for multi-core support */
typedef struct {
    uint32_t cpu_id;
    uint32_t apic_id;
    uint32_t active;
    uint32_t idle;
} per_cpu_data_t;

/* Maximum supported cores */
#define MAX_CPUS 16

/* Hardware support state */
static hardware_support_state_t hw_state = {
    .intel_optimization = 0,
    .amd_support = 0,
    .multicore_scaling = 0,
    .detected_cores = 1
};

/* Detected CPU features */
static cpu_features_t cpu_features = {0};

/* Per-CPU data array */
static per_cpu_data_t per_cpu_data[MAX_CPUS] = {{0}};

/* CPU vendor string */
static char cpu_vendor[13] = {0};

/**
 * Execute CPUID instruction
 * @param leaf CPUID leaf number
 * @param eax Output EAX register
 * @param ebx Output EBX register
 * @param ecx Output ECX register
 * @param edx Output EDX register
 */
static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile (
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (0)
    );
#else
    /* Non-x86 architecture - return zeros */
    *eax = *ebx = *ecx = *edx = 0;
#endif
}

/**
 * Detect CPU vendor and populate vendor string
 */
static void detect_cpu_vendor(void) {
    uint32_t eax, ebx, ecx, edx;
    
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    /* Build vendor string from EBX, EDX, ECX */
    *((uint32_t *)&cpu_vendor[0]) = ebx;
    *((uint32_t *)&cpu_vendor[4]) = edx;
    *((uint32_t *)&cpu_vendor[8]) = ecx;
    cpu_vendor[12] = '\0';
}

/**
 * Detect CPU feature flags
 */
static void detect_cpu_features(void) {
    uint32_t eax, ebx, ecx, edx;
    
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    /* Check EDX features */
    cpu_features.fpu_present = (edx & CPUID_FEAT_EDX_FPU) ? 1 : 0;
    cpu_features.apic_present = (edx & CPUID_FEAT_EDX_APIC) ? 1 : 0;
    cpu_features.htt_capable = (edx & CPUID_FEAT_EDX_HTT) ? 1 : 0;
    
    /* SSE support level */
    cpu_features.sse_support = 0;
    if (edx & CPUID_FEAT_EDX_SSE) cpu_features.sse_support = 1;
    if (edx & CPUID_FEAT_EDX_SSE2) cpu_features.sse_support = 2;
    if (ecx & CPUID_FEAT_ECX_SSE3) cpu_features.sse_support = 3;
    if (ecx & CPUID_FEAT_ECX_SSE42) cpu_features.sse_support = 4;
    
    /* AVX support */
    cpu_features.avx_support = (ecx & CPUID_FEAT_ECX_AVX) ? 1 : 0;
}

/**
 * Detect number of logical processors
 * @return Number of logical processors
 */
static uint32_t detect_logical_processors(void) {
    uint32_t eax, ebx, ecx, edx;
    
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    /* If HTT bit is set, bits 23:16 of EBX contain logical processor count */
    if (edx & CPUID_FEAT_EDX_HTT) {
        return (ebx >> 16) & 0xFF;
    }
    
    return 1;
}

/**
 * Initialize hardware support
 * Auto-detects CPU vendor and capabilities
 * @return 0 on success, -1 on failure
 */
int hardware_support_init(void) {
    /* Detect CPU vendor */
    detect_cpu_vendor();
    
    /* Detect CPU features */
    detect_cpu_features();
    
    /* Detect number of logical processors */
    hw_state.detected_cores = detect_logical_processors();
    if (hw_state.detected_cores == 0) {
        hw_state.detected_cores = 1;
    }
    
    /* Check if Intel or AMD and initialize optimizations */
    int is_intel = 0;
    int is_amd = 0;
    
    /* Compare vendor string */
    const char *intel_str = CPUID_VENDOR_INTEL;
    const char *amd_str = CPUID_VENDOR_AMD;
    
    is_intel = 1;
    is_amd = 1;
    for (int i = 0; i < 12; i++) {
        if (cpu_vendor[i] != intel_str[i]) is_intel = 0;
        if (cpu_vendor[i] != amd_str[i]) is_amd = 0;
    }
    
    if (is_intel) {
        intel_processor_optimization_init();
    } else if (is_amd) {
        amd_processor_support_init();
    }
    
    /* Initialize multi-core support if more than one core detected */
    if (hw_state.detected_cores > 1) {
        multicore_scaling_init(hw_state.detected_cores);
    }
    
    return 0;
}

/**
 * Enable Intel processor optimization (Core i3/i5/i7)
 * Detects Intel-specific features via CPUID
 */
int intel_processor_optimization_init(void) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Check for Intel VMX (VT-x) support via CPUID leaf 1 */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    cpu_features.vmx_capable = (ecx & (1 << 5)) ? 1 : 0;
    
    /* Check for extended CPUID support */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    
    hw_state.intel_optimization = 1;
    return 0;
}

/**
 * Enable AMD processor support (Ryzen series)
 * Detects AMD-specific features via CPUID
 */
int amd_processor_support_init(void) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Check for AMD SVM (AMD-V) support via extended CPUID */
    cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
    cpu_features.svm_capable = (ecx & (1 << 2)) ? 1 : 0;
    
    hw_state.amd_support = 1;
    return 0;
}

/**
 * Enable multi-core CPU scaling (2-16 cores)
 * Sets up per-CPU data structures and load balancing
 * @param num_cores Number of CPU cores to initialize
 */
int multicore_scaling_init(uint32_t num_cores) {
    if (num_cores > MAX_CPUS) {
        num_cores = MAX_CPUS;
    }
    
    /* Initialize per-CPU data structures */
    for (uint32_t i = 0; i < num_cores; i++) {
        per_cpu_data[i].cpu_id = i;
        per_cpu_data[i].apic_id = i;  /* Simplified; real code would read from APIC */
        per_cpu_data[i].active = (i == 0) ? 1 : 0;  /* Only BSP active initially */
        per_cpu_data[i].idle = 0;
    }
    
    hw_state.multicore_scaling = 1;
    hw_state.detected_cores = num_cores;
    
    return 0;
}

/**
 * Get number of detected CPU cores
 * @return Number of detected cores
 */
uint32_t hardware_get_core_count(void) {
    return hw_state.detected_cores;
}
