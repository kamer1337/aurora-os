/**
 * Aurora OS - Firewall Implementation
 */

#include "firewall.h"
#include <stddef.h>

/* Global firewall state */
static firewall_rule_t rules[FW_MAX_RULES];
static int rule_count = 0;
static int firewall_enabled = 1;
static firewall_stats_t stats = {0};

/* Helper function to match IP with mask */
static int ip_matches(uint32_t packet_ip, uint32_t rule_ip, uint32_t mask) {
    if (mask == 0) {
        return 1;  /* Match all */
    }
    return (packet_ip & mask) == (rule_ip & mask);
}

/* Helper function to match port (0 means any port) */
static int port_matches(uint16_t packet_port, uint16_t rule_port) {
    if (rule_port == 0) {
        return 1;  /* Match all */
    }
    return packet_port == rule_port;
}

/**
 * Initialize firewall
 */
void firewall_init(void) {
    rule_count = 0;
    firewall_enabled = 1;
    
    /* Clear statistics */
    firewall_reset_stats();
    
    /* Initialize ML if enabled */
#ifdef ENABLE_ML_PLUGIN
    firewall_ml_init();
#endif
}

/**
 * Add firewall rule
 */
int firewall_add_rule(const firewall_rule_t* rule) {
    if (rule == NULL || rule_count >= FW_MAX_RULES) {
        return -1;
    }
    
    /* Copy rule */
    rules[rule_count] = *rule;
    rules[rule_count].enabled = 1;
    rules[rule_count].hit_count = 0;
    
    rule_count++;
    return rule_count - 1;  /* Return rule ID */
}

/**
 * Remove firewall rule
 */
int firewall_remove_rule(int rule_id) {
    if (rule_id < 0 || rule_id >= rule_count) {
        return -1;
    }
    
    /* Shift rules down */
    for (int i = rule_id; i < rule_count - 1; i++) {
        rules[i] = rules[i + 1];
    }
    
    rule_count--;
    return 0;
}

/**
 * Enable firewall rule
 */
int firewall_enable_rule(int rule_id) {
    if (rule_id < 0 || rule_id >= rule_count) {
        return -1;
    }
    
    rules[rule_id].enabled = 1;
    return 0;
}

/**
 * Disable firewall rule
 */
int firewall_disable_rule(int rule_id) {
    if (rule_id < 0 || rule_id >= rule_count) {
        return -1;
    }
    
    rules[rule_id].enabled = 0;
    return 0;
}

/**
 * Clear all rules
 */
void firewall_clear_rules(void) {
    rule_count = 0;
}

/**
 * Get rule count
 */
int firewall_get_rule_count(void) {
    return rule_count;
}

/**
 * Check packet against firewall rules
 */
int firewall_check_packet(uint32_t src_ip, uint32_t dst_ip,
                         uint16_t src_port, uint16_t dst_port,
                         uint8_t protocol, uint8_t direction) {
    if (!firewall_enabled) {
        stats.packets_passed++;
        return FW_ACTION_ALLOW;
    }
    
    /* Check each rule */
    for (int i = 0; i < rule_count; i++) {
        firewall_rule_t* rule = &rules[i];
        
        if (!rule->enabled) {
            continue;
        }
        
        /* Check direction */
        if (rule->direction != FW_DIR_BOTH && rule->direction != direction) {
            continue;
        }
        
        /* Check protocol (0 means any) */
        if (rule->protocol != 0 && rule->protocol != protocol) {
            continue;
        }
        
        /* Check source IP */
        if (!ip_matches(src_ip, rule->src_ip, rule->src_mask)) {
            continue;
        }
        
        /* Check destination IP */
        if (!ip_matches(dst_ip, rule->dst_ip, rule->dst_mask)) {
            continue;
        }
        
        /* Check source port */
        if (!port_matches(src_port, rule->src_port)) {
            continue;
        }
        
        /* Check destination port */
        if (!port_matches(dst_port, rule->dst_port)) {
            continue;
        }
        
        /* Rule matched */
        rule->hit_count++;
        
        if (rule->action == FW_ACTION_ALLOW) {
            stats.packets_passed++;
            return FW_ACTION_ALLOW;
        } else {
            stats.packets_blocked++;
            return rule->action;
        }
    }
    
    /* Default action: allow */
    stats.packets_passed++;
    return FW_ACTION_ALLOW;
}

/**
 * Get firewall statistics
 */
const firewall_stats_t* firewall_get_stats(void) {
    return &stats;
}

/**
 * Reset statistics
 */
void firewall_reset_stats(void) {
    stats.packets_passed = 0;
    stats.packets_blocked = 0;
    stats.bytes_passed = 0;
    stats.bytes_blocked = 0;
    
#ifdef ENABLE_ML_PLUGIN
    stats.ml_stats.packets_analyzed = 0;
    stats.ml_stats.threats_detected = 0;
    stats.ml_stats.false_positives = 0;
    stats.ml_stats.last_threat_time = 0;
    stats.ml_stats.threat_level = FW_THREAT_NONE;
#endif
}

/**
 * Enable firewall
 */
void firewall_enable(void) {
    firewall_enabled = 1;
}

/**
 * Disable firewall
 */
void firewall_disable(void) {
    firewall_enabled = 0;
}

/**
 * Check if firewall is enabled
 */
int firewall_is_enabled(void) {
    return firewall_enabled;
}

#ifdef ENABLE_ML_PLUGIN
/**
 * Initialize ML-based threat detection
 */
void firewall_ml_init(void) {
    stats.ml_stats.packets_analyzed = 0;
    stats.ml_stats.threats_detected = 0;
    stats.ml_stats.false_positives = 0;
    stats.ml_stats.threat_level = FW_THREAT_NONE;
}

/**
 * Analyze packet with ML
 * 
 * This is a simple heuristic-based implementation.
 * A real ML implementation would use trained models.
 */
int firewall_ml_analyze_packet(const uint8_t* packet_data, uint32_t length) {
    if (packet_data == NULL || length == 0) {
        return FW_THREAT_NONE;
    }
    
    stats.ml_stats.packets_analyzed++;
    
    /* Simple heuristic checks */
    int threat_score = 0;
    
    /* Check for suspicious patterns */
    /* 1. Very large packets might be flood attacks */
    if (length > 1400) {
        threat_score += 1;
    }
    
    /* 2. Check for common attack signatures (simplified) */
    for (uint32_t i = 0; i < length - 3; i++) {
        /* Check for repeated patterns that might indicate attacks */
        if (packet_data[i] == packet_data[i+1] && 
            packet_data[i] == packet_data[i+2] &&
            packet_data[i] == packet_data[i+3]) {
            threat_score += 1;
            break;
        }
    }
    
    /* Determine threat level based on score */
    uint8_t threat_level = FW_THREAT_NONE;
    if (threat_score >= 2) {
        threat_level = FW_THREAT_HIGH;
        stats.ml_stats.threats_detected++;
    } else if (threat_score >= 1) {
        threat_level = FW_THREAT_LOW;
    }
    
    /* Update global threat level */
    if (threat_level > stats.ml_stats.threat_level) {
        stats.ml_stats.threat_level = threat_level;
    }
    
    return threat_level;
}

/**
 * Get current threat level
 */
uint8_t firewall_ml_get_threat_level(void) {
    return stats.ml_stats.threat_level;
}

/**
 * Update ML model based on collected data
 * 
 * This implementation uses a simple adaptive threshold mechanism.
 * In a real implementation, this would use proper ML training algorithms.
 */
void firewall_ml_update_model(void) {
    /* Calculate detection accuracy based on collected statistics */
    uint64_t total_analyzed = stats.ml_stats.packets_analyzed;
    uint64_t threats_found = stats.ml_stats.threats_detected;
    uint64_t false_positives = stats.ml_stats.false_positives;
    
    if (total_analyzed == 0) {
        return;  /* No data to train on */
    }
    
    /* Calculate false positive rate */
    uint64_t fp_rate = 0;
    if (threats_found > 0) {
        fp_rate = (false_positives * 100) / threats_found;
    }
    
    /* Adaptive threshold adjustment:
     * If false positive rate is too high, reduce sensitivity
     * This is a simple feedback mechanism for the heuristic model
     */
    static uint8_t sensitivity_level = 5;  /* 1-10 scale */
    
    if (fp_rate > 20) {
        /* Too many false positives, reduce sensitivity */
        if (sensitivity_level > 1) {
            sensitivity_level--;
        }
    } else if (fp_rate < 5 && threats_found > 10) {
        /* Good accuracy with significant data, can increase sensitivity */
        if (sensitivity_level < 10) {
            sensitivity_level++;
        }
    }
    
    /* Decay old threat level over time to reflect current state */
    if (stats.ml_stats.threat_level > FW_THREAT_NONE) {
        /* Gradually reduce threat level if no recent threats */
        if (stats.ml_stats.threats_detected == 0 || 
            (total_analyzed > 1000 && threats_found * 100 / total_analyzed < 1)) {
            stats.ml_stats.threat_level = (stats.ml_stats.threat_level > FW_THREAT_NONE) ?
                                          stats.ml_stats.threat_level - 1 : FW_THREAT_NONE;
        }
    }
    
    /* Reset counters for next training cycle */
    stats.ml_stats.packets_analyzed = 0;
    stats.ml_stats.threats_detected = 0;
    stats.ml_stats.false_positives = 0;
}

/**
 * Mark last detection as false positive (for model training)
 */
void firewall_ml_mark_false_positive(void) {
    if (stats.ml_stats.false_positives < 0xFFFFFFFF) {
        stats.ml_stats.false_positives++;
    }
}
#endif
