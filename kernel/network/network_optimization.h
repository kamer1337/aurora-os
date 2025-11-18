/**
 * Aurora OS - Network Stack Throughput Improvement Header
 */

#ifndef NETWORK_OPTIMIZATION_H
#define NETWORK_OPTIMIZATION_H

typedef struct {
    int zero_copy;
    int tcp_offload;
    int jumbo_frames;
    int interrupt_coalescing;
    int receive_scaling;
} network_opt_state_t;

// Function prototypes
int network_optimization_init(void);
void network_enable_zero_copy(void);
void network_enable_tcp_offload(void);
void network_enable_jumbo_frames(void);
void network_enable_interrupt_coalescing(void);

#endif // NETWORK_OPTIMIZATION_H
