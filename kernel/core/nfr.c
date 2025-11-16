/**
 * Aurora OS - Non-Functional Requirements Module Implementation
 * 
 * Provides monitoring and metrics for non-functional requirements
 */

#include "nfr.h"
#include "kernel.h"
#include <stddef.h>

/* Include necessary headers */
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../memory/memory.h"
#include "../process/process.h"

/* External helper functions we'll implement */
static uint32_t get_total_memory(void);
static uint32_t get_used_memory(void);
static uint32_t get_process_count(void);

/* Global NFR metrics */
static nfr_metrics_t g_nfr_metrics;
static bool g_nfr_initialized = false;

/* Stub implementations for system information functions */
static uint32_t get_total_memory(void) {
    /* Return a reasonable default (16 MB) */
    return 16 * 1024 * 1024;
}

static uint32_t get_used_memory(void) {
    /* Estimate based on basic kernel usage */
    /* In a real implementation, this would track actual allocations */
    static uint32_t used = 1024 * 1024; /* Start with 1MB used */
    used += 1024; /* Increment slightly each call to simulate usage */
    return used;
}

static uint32_t get_process_count(void) {
    /* Return a reasonable default */
    /* In a real implementation, this would count actual processes */
    return 3; /* Kernel, init, and one user process */
}

/**
 * Initialize the NFR monitoring module
 */
void nfr_init(void) {
    if (g_nfr_initialized) {
        return;
    }
    
    /* Zero out all metrics */
    for (uint32_t i = 0; i < sizeof(nfr_metrics_t); i++) {
        ((uint8_t*)&g_nfr_metrics)[i] = 0;
    }
    
    /* Initialize start time */
    g_nfr_metrics.performance.start_time_ms = timer_get_ticks();
    
    /* Set initial values for min/max */
    g_nfr_metrics.performance.min_latency_us = 0xFFFFFFFF;
    g_nfr_metrics.performance.max_latency_us = 0;
    
    /* Set maximum capacities */
    g_nfr_metrics.scalability.max_processes = 256;
    g_nfr_metrics.scalability.max_threads = 1024;
    
    /* Initialize scores */
    g_nfr_metrics.reliability.reliability_score = 100.0f;
    g_nfr_metrics.usability.usability_score = 100.0f;
    g_nfr_metrics.usability.ui_responsiveness = 100;
    
    g_nfr_initialized = true;
}

/**
 * Get current NFR metrics
 */
nfr_metrics_t* nfr_get_metrics(void) {
    return &g_nfr_metrics;
}

/**
 * Update performance metrics
 */
void nfr_update_performance(uint32_t operation_latency_us) {
    performance_metrics_t* perf = &g_nfr_metrics.performance;
    
    /* Update operation count */
    perf->total_operations++;
    
    /* Update latency statistics */
    if (operation_latency_us < perf->min_latency_us) {
        perf->min_latency_us = operation_latency_us;
    }
    if (operation_latency_us > perf->max_latency_us) {
        perf->max_latency_us = operation_latency_us;
    }
    
    /* Update average latency (running average) */
    if (perf->total_operations == 1) {
        perf->avg_latency_us = operation_latency_us;
    } else {
        /* Use weighted average to avoid overflow */
        uint32_t ops = perf->total_operations;
        if (ops > 0xFFFF) {
            /* Use weighted average for large operation counts */
            perf->avg_latency_us = (perf->avg_latency_us * 15 + operation_latency_us) / 16;
        } else {
            uint32_t total = perf->avg_latency_us * (ops - 1);
            total += operation_latency_us;
            perf->avg_latency_us = total / ops;
        }
    }
    
    /* Calculate throughput (operations per second) */
    uint32_t elapsed_ms = timer_get_ticks() - perf->start_time_ms;
    if (elapsed_ms > 0) {
        perf->throughput_ops_sec = (perf->total_operations * 1000) / elapsed_ms;
    }
    
    /* Estimate CPU usage based on operation frequency and latency */
    /* This is a simplified model: higher ops and lower latency = better efficiency */
    if (perf->avg_latency_us > 0 && perf->throughput_ops_sec > 0) {
        uint32_t load = (perf->avg_latency_us * perf->throughput_ops_sec) / 10000;
        perf->cpu_usage_percent = (load > 100) ? 100 : (uint8_t)load;
    }
}

/**
 * Update scalability metrics
 */
void nfr_update_scalability(void) {
    scalability_metrics_t* scale = &g_nfr_metrics.scalability;
    
    /* Get memory information */
    scale->total_memory_kb = get_total_memory() / 1024;
    scale->used_memory_kb = get_used_memory() / 1024;
    scale->free_memory_kb = scale->total_memory_kb - scale->used_memory_kb;
    
    /* Calculate memory usage percentage */
    if (scale->total_memory_kb > 0) {
        scale->memory_usage_percent = (uint8_t)((scale->used_memory_kb * 100) / scale->total_memory_kb);
    }
    
    /* Get process/thread counts */
    scale->active_processes = get_process_count();
    scale->active_threads = scale->active_processes * 2; /* Estimate: avg 2 threads per process */
    
    /* Calculate system load based on memory and process usage */
    uint32_t memory_load = scale->memory_usage_percent;
    uint32_t process_load = (scale->active_processes * 100) / scale->max_processes;
    scale->system_load_percent = (uint8_t)((memory_load + process_load) / 2);
    
    if (scale->system_load_percent > 100) {
        scale->system_load_percent = 100;
    }
}

/**
 * Record an error for reliability tracking
 */
void nfr_record_error(bool is_critical, bool was_recovered) {
    reliability_metrics_t* rel = &g_nfr_metrics.reliability;
    
    rel->total_errors++;
    
    if (is_critical) {
        rel->critical_errors++;
        if (!was_recovered) {
            rel->system_crashes++;
        }
    } else {
        rel->recoverable_errors++;
    }
    
    if (was_recovered) {
        rel->successful_recoveries++;
    } else {
        rel->failed_recoveries++;
    }
    
    /* Calculate reliability score */
    /* Score decreases with errors, especially critical ones */
    if (rel->total_errors > 0) {
        float error_impact = (float)rel->critical_errors * 5.0f + (float)rel->recoverable_errors;
        float recovery_bonus = (float)rel->successful_recoveries * 0.5f;
        float score = 100.0f - (error_impact / 10.0f) + recovery_bonus;
        
        if (score < 0.0f) score = 0.0f;
        if (score > 100.0f) score = 100.0f;
        
        rel->reliability_score = score;
    }
}

/**
 * Record a user interaction for usability tracking
 */
void nfr_record_interaction(uint32_t response_time_ms, bool was_successful) {
    usability_metrics_t* use = &g_nfr_metrics.usability;
    
    use->user_interactions++;
    
    if (was_successful) {
        use->successful_interactions++;
    } else {
        use->failed_interactions++;
    }
    
    /* Update average response time */
    if (use->user_interactions == 1) {
        use->avg_response_time_ms = response_time_ms;
    } else {
        /* Use weighted average to avoid overflow */
        uint32_t interactions = use->user_interactions;
        if (interactions > 0xFFFF) {
            use->avg_response_time_ms = (use->avg_response_time_ms * 15 + response_time_ms) / 16;
        } else {
            uint32_t total = use->avg_response_time_ms * (interactions - 1);
            total += response_time_ms;
            use->avg_response_time_ms = total / interactions;
        }
    }
    
    /* Calculate usability score based on success rate and response time */
    if (use->user_interactions > 0) {
        float success_rate = ((float)use->successful_interactions * 100.0f) / (float)use->user_interactions;
        
        /* Penalize slow response times (target: < 100ms excellent, > 1000ms poor) */
        float response_score = 100.0f;
        if (use->avg_response_time_ms > 100) {
            response_score = 100.0f - ((float)(use->avg_response_time_ms - 100) / 10.0f);
            if (response_score < 0.0f) response_score = 0.0f;
        }
        
        use->usability_score = (success_rate + response_score) / 2.0f;
        use->ui_responsiveness = (uint8_t)response_score;
    }
}

/**
 * Update all metrics
 */
void nfr_update_all(void) {
    if (!g_nfr_initialized) return;
    
    /* Update uptime */
    uint32_t current_time = timer_get_ticks();
    g_nfr_metrics.reliability.uptime_seconds = 
        (current_time - g_nfr_metrics.performance.start_time_ms) / 1000;
    
    /* Update scalability metrics */
    nfr_update_scalability();
    
    /* Update context switch count (increment periodically) */
    g_nfr_metrics.performance.context_switches++;
}

/**
 * Simple integer to string conversion
 */
static void print_uint(uint32_t value) {
    char buffer[12];
    int pos = 0;
    
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    for (int i = pos - 1; i >= 0; i--) {
        vga_putchar(buffer[i]);
    }
}

/**
 * Print a float value (simplified)
 */
static void print_float(float value) {
    uint32_t integer_part = (uint32_t)value;
    uint32_t decimal_part = (uint32_t)((value - integer_part) * 100);
    
    print_uint(integer_part);
    vga_putchar('.');
    if (decimal_part < 10) vga_putchar('0');
    print_uint(decimal_part);
}

/**
 * Print NFR metrics report
 */
void nfr_print_report(void) {
    if (!g_nfr_initialized) {
        vga_write("NFR module not initialized\n");
        return;
    }
    
    nfr_metrics_t* m = &g_nfr_metrics;
    
    vga_write("\n=== Aurora OS Non-Functional Requirements Report ===\n\n");
    
    /* Performance Metrics */
    vga_write("PERFORMANCE:\n");
    vga_write("  Total Operations: ");
    print_uint((uint32_t)m->performance.total_operations);
    vga_write("\n  Avg Latency: ");
    print_uint(m->performance.avg_latency_us);
    vga_write(" us\n");
    vga_write("  Min Latency: ");
    print_uint(m->performance.min_latency_us);
    vga_write(" us\n");
    vga_write("  Max Latency: ");
    print_uint(m->performance.max_latency_us);
    vga_write(" us\n");
    vga_write("  Throughput: ");
    print_uint(m->performance.throughput_ops_sec);
    vga_write(" ops/sec\n");
    vga_write("  CPU Usage: ");
    print_uint(m->performance.cpu_usage_percent);
    vga_write("%\n");
    vga_write("  Score: ");
    print_uint(nfr_get_performance_score());
    vga_write("/100\n\n");
    
    /* Scalability Metrics */
    vga_write("SCALABILITY:\n");
    vga_write("  Memory Usage: ");
    print_uint(m->scalability.used_memory_kb);
    vga_write("/");
    print_uint(m->scalability.total_memory_kb);
    vga_write(" KB (");
    print_uint(m->scalability.memory_usage_percent);
    vga_write("%)\n");
    vga_write("  Active Processes: ");
    print_uint(m->scalability.active_processes);
    vga_write("/");
    print_uint(m->scalability.max_processes);
    vga_write("\n");
    vga_write("  System Load: ");
    print_uint(m->scalability.system_load_percent);
    vga_write("%\n");
    vga_write("  Score: ");
    print_uint(nfr_get_scalability_score());
    vga_write("/100\n\n");
    
    /* Reliability Metrics */
    vga_write("RELIABILITY:\n");
    vga_write("  Uptime: ");
    print_uint((uint32_t)m->reliability.uptime_seconds);
    vga_write(" seconds\n");
    vga_write("  Total Errors: ");
    print_uint(m->reliability.total_errors);
    vga_write(" (Critical: ");
    print_uint(m->reliability.critical_errors);
    vga_write(")\n");
    vga_write("  Successful Recoveries: ");
    print_uint(m->reliability.successful_recoveries);
    vga_write("/");
    print_uint(m->reliability.successful_recoveries + m->reliability.failed_recoveries);
    vga_write("\n");
    vga_write("  Score: ");
    print_float(m->reliability.reliability_score);
    vga_write("/100\n\n");
    
    /* Usability Metrics */
    vga_write("USABILITY:\n");
    vga_write("  User Interactions: ");
    print_uint(m->usability.user_interactions);
    vga_write(" (Success: ");
    print_uint(m->usability.successful_interactions);
    vga_write(")\n");
    vga_write("  Avg Response Time: ");
    print_uint(m->usability.avg_response_time_ms);
    vga_write(" ms\n");
    vga_write("  UI Responsiveness: ");
    print_uint(m->usability.ui_responsiveness);
    vga_write("%\n");
    vga_write("  Score: ");
    print_float(m->usability.usability_score);
    vga_write("/100\n\n");
    
    /* Overall Score */
    vga_write("OVERALL SYSTEM HEALTH: ");
    print_uint(nfr_get_overall_score());
    vga_write("/100\n");
    vga_write("=====================================================\n");
}

/**
 * Get performance score (0-100)
 */
uint8_t nfr_get_performance_score(void) {
    performance_metrics_t* perf = &g_nfr_metrics.performance;
    
    /* Score based on low latency, high throughput, and reasonable CPU usage */
    uint32_t latency_score = 100;
    if (perf->avg_latency_us > 1000) {
        latency_score = 100 - ((perf->avg_latency_us - 1000) / 100);
        if (latency_score > 100) latency_score = 0;
    }
    
    uint32_t throughput_score = (perf->throughput_ops_sec > 1000) ? 100 : 
                                (perf->throughput_ops_sec / 10);
    
    uint32_t cpu_score = (perf->cpu_usage_percent < 80) ? 100 : 
                         (100 - perf->cpu_usage_percent);
    
    uint32_t overall = (latency_score + throughput_score + cpu_score) / 3;
    return (overall > 100) ? 100 : (uint8_t)overall;
}

/**
 * Get scalability score (0-100)
 */
uint8_t nfr_get_scalability_score(void) {
    scalability_metrics_t* scale = &g_nfr_metrics.scalability;
    
    /* Score based on available resources and efficient usage */
    uint32_t memory_score = 100 - scale->memory_usage_percent;
    uint32_t process_score = 100 - ((scale->active_processes * 100) / scale->max_processes);
    uint32_t load_score = 100 - scale->system_load_percent;
    
    uint32_t overall = (memory_score + process_score + load_score) / 3;
    return (overall > 100) ? 100 : (uint8_t)overall;
}

/**
 * Get reliability score (0-100)
 */
uint8_t nfr_get_reliability_score(void) {
    return (uint8_t)g_nfr_metrics.reliability.reliability_score;
}

/**
 * Get usability score (0-100)
 */
uint8_t nfr_get_usability_score(void) {
    return (uint8_t)g_nfr_metrics.usability.usability_score;
}

/**
 * Get overall system health score (0-100)
 */
uint8_t nfr_get_overall_score(void) {
    uint32_t perf = nfr_get_performance_score();
    uint32_t scale = nfr_get_scalability_score();
    uint32_t rel = nfr_get_reliability_score();
    uint32_t use = nfr_get_usability_score();
    
    uint32_t overall = (perf + scale + rel + use) / 4;
    return (overall > 100) ? 100 : (uint8_t)overall;
}

/**
 * Reset all metrics
 */
void nfr_reset_metrics(void) {
    g_nfr_initialized = false;
    nfr_init();
}
