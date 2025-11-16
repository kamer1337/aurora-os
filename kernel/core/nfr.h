/**
 * Aurora OS - Non-Functional Requirements Module
 * 
 * Provides monitoring and metrics for non-functional requirements:
 * - Performance (latency, throughput, CPU usage)
 * - Scalability (resource usage, load monitoring)
 * - Reliability (uptime, error tracking, recovery)
 * - Usability (interaction metrics, responsiveness)
 */

#ifndef AURORA_NFR_H
#define AURORA_NFR_H

#include <stdint.h>
#include <stdbool.h>

/* Performance metrics structure */
typedef struct {
    uint32_t start_time_ms;         /* System start time (32-bit for simplicity) */
    uint32_t total_operations;      /* Total operations performed (32-bit) */
    uint32_t avg_latency_us;        /* Average operation latency (microseconds) */
    uint32_t min_latency_us;        /* Minimum latency */
    uint32_t max_latency_us;        /* Maximum latency */
    uint32_t throughput_ops_sec;    /* Operations per second */
    uint8_t cpu_usage_percent;      /* CPU usage percentage */
    uint32_t context_switches;      /* Number of context switches */
} performance_metrics_t;

/* Scalability metrics structure */
typedef struct {
    uint32_t total_memory_kb;       /* Total system memory */
    uint32_t used_memory_kb;        /* Used memory */
    uint32_t free_memory_kb;        /* Free memory */
    uint8_t memory_usage_percent;   /* Memory usage percentage */
    uint32_t active_processes;      /* Number of active processes */
    uint32_t active_threads;        /* Number of active threads */
    uint32_t max_processes;         /* Maximum supported processes */
    uint32_t max_threads;           /* Maximum supported threads */
    uint8_t system_load_percent;    /* Overall system load */
} scalability_metrics_t;

/* Reliability metrics structure */
typedef struct {
    uint32_t uptime_seconds;        /* System uptime in seconds (32-bit for simplicity) */
    uint32_t total_errors;          /* Total errors encountered */
    uint32_t critical_errors;       /* Critical errors */
    uint32_t recoverable_errors;    /* Recoverable errors */
    uint32_t system_crashes;        /* Number of crashes */
    uint32_t successful_recoveries; /* Successful recovery operations */
    uint32_t failed_recoveries;     /* Failed recovery operations */
    float reliability_score;        /* Reliability score (0.0-100.0) */
} reliability_metrics_t;

/* Usability metrics structure */
typedef struct {
    uint32_t user_interactions;     /* Total user interactions */
    uint32_t avg_response_time_ms;  /* Average UI response time */
    uint32_t failed_interactions;   /* Failed user interactions */
    uint32_t successful_interactions; /* Successful interactions */
    float usability_score;          /* Usability score (0.0-100.0) */
    uint8_t ui_responsiveness;      /* UI responsiveness (0-100) */
} usability_metrics_t;

/* Combined NFR metrics */
typedef struct {
    performance_metrics_t performance;
    scalability_metrics_t scalability;
    reliability_metrics_t reliability;
    usability_metrics_t usability;
} nfr_metrics_t;

/**
 * Initialize the NFR monitoring module
 */
void nfr_init(void);

/**
 * Get current NFR metrics
 * @return Pointer to current metrics structure
 */
nfr_metrics_t* nfr_get_metrics(void);

/**
 * Update performance metrics
 * @param operation_latency_us Latency of last operation in microseconds
 */
void nfr_update_performance(uint32_t operation_latency_us);

/**
 * Update scalability metrics
 */
void nfr_update_scalability(void);

/**
 * Record an error for reliability tracking
 * @param is_critical Whether the error is critical
 * @param was_recovered Whether the error was successfully recovered
 */
void nfr_record_error(bool is_critical, bool was_recovered);

/**
 * Record a user interaction for usability tracking
 * @param response_time_ms Response time in milliseconds
 * @param was_successful Whether the interaction was successful
 */
void nfr_record_interaction(uint32_t response_time_ms, bool was_successful);

/**
 * Update all metrics (should be called periodically)
 */
void nfr_update_all(void);

/**
 * Print NFR metrics report
 */
void nfr_print_report(void);

/**
 * Get performance score (0-100)
 */
uint8_t nfr_get_performance_score(void);

/**
 * Get scalability score (0-100)
 */
uint8_t nfr_get_scalability_score(void);

/**
 * Get reliability score (0-100)
 */
uint8_t nfr_get_reliability_score(void);

/**
 * Get usability score (0-100)
 */
uint8_t nfr_get_usability_score(void);

/**
 * Get overall system health score (0-100)
 */
uint8_t nfr_get_overall_score(void);

/**
 * Reset all metrics (useful for testing)
 */
void nfr_reset_metrics(void);

#endif /* AURORA_NFR_H */
