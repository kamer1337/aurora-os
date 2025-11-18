/**
 * Aurora OS - Network Stack Throughput Improvement
 * Medium-Term Goal (Q2 2026): Network stack throughput improvement
 * 
 * This module implements optimizations for network stack performance
 * and throughput enhancement.
 */

#include "network_optimization.h"

// Network optimization state
static network_opt_state_t net_opt = {
    .zero_copy = 0,
    .tcp_offload = 0,
    .jumbo_frames = 0,
    .interrupt_coalescing = 0,
    .receive_scaling = 0
};

/**
 * Initialize network stack optimizations
 * @return 0 on success, -1 on failure
 */
int network_optimization_init(void) {
    // Enable zero-copy networking
    // Avoids copying packet data between buffers for better performance
    // Direct DMA from NIC to application memory space
    net_opt.zero_copy = 1;
    
    // Enable TCP offload engine (TOE) support
    // Offload TCP/IP processing to network card when supported
    net_opt.tcp_offload = 1;
    
    // Enable interrupt coalescing
    // Batch multiple interrupts to reduce CPU overhead
    net_opt.interrupt_coalescing = 1;
    
    // Jumbo frames and receive scaling will be enabled based on hardware
    net_opt.jumbo_frames = 0;  // Requires MTU > 1500
    net_opt.receive_scaling = 0;  // Requires multi-queue NIC
    
    return 0;
}

/**
 * Enable zero-copy networking
 */
void network_enable_zero_copy(void) {
    net_opt.zero_copy = 1;
}

/**
 * Enable TCP offload engine
 */
void network_enable_tcp_offload(void) {
    net_opt.tcp_offload = 1;
}

/**
 * Enable jumbo frames
 */
void network_enable_jumbo_frames(void) {
    // Enable jumbo frames (MTU > 1500 bytes, typically 9000)
    // Reduces per-packet overhead for high-speed networks
    // Requires NIC and switch support
    net_opt.jumbo_frames = 1;
}

/**
 * Enable interrupt coalescing
 */
void network_enable_interrupt_coalescing(void) {
    net_opt.interrupt_coalescing = 1;
}
