/**
 * Aurora OS - Firewall with Optional Machine Learning
 * 
 * Provides packet filtering and optional ML-based threat detection
 */

#ifndef AURORA_FIREWALL_H
#define AURORA_FIREWALL_H

#include <stdint.h>

/* Firewall actions */
#define FW_ACTION_ALLOW    0
#define FW_ACTION_DENY     1
#define FW_ACTION_DROP     2

/* Firewall rule directions */
#define FW_DIR_INBOUND     0
#define FW_DIR_OUTBOUND    1
#define FW_DIR_BOTH        2

/* Maximum firewall rules */
#define FW_MAX_RULES       256

/* ML threat levels */
#define FW_THREAT_NONE     0
#define FW_THREAT_LOW      1
#define FW_THREAT_MEDIUM   2
#define FW_THREAT_HIGH     3
#define FW_THREAT_CRITICAL 4

/* Firewall rule structure */
typedef struct {
    uint32_t src_ip;
    uint32_t src_mask;
    uint32_t dst_ip;
    uint32_t dst_mask;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;  /* TCP, UDP, ICMP, etc. */
    uint8_t direction;
    uint8_t action;
    uint8_t enabled;
    uint32_t hit_count;
} firewall_rule_t;

/* ML detection statistics */
typedef struct {
    uint32_t packets_analyzed;
    uint32_t threats_detected;
    uint32_t false_positives;
    uint32_t last_threat_time;
    uint8_t threat_level;
} ml_stats_t;

/* Firewall statistics */
typedef struct {
    uint64_t packets_passed;
    uint64_t packets_blocked;
    uint64_t bytes_passed;
    uint64_t bytes_blocked;
    ml_stats_t ml_stats;
} firewall_stats_t;

/* Initialize firewall */
void firewall_init(void);

/* Rule management */
int firewall_add_rule(const firewall_rule_t* rule);
int firewall_remove_rule(int rule_id);
int firewall_enable_rule(int rule_id);
int firewall_disable_rule(int rule_id);
void firewall_clear_rules(void);
int firewall_get_rule_count(void);

/* Packet filtering */
int firewall_check_packet(uint32_t src_ip, uint32_t dst_ip, 
                         uint16_t src_port, uint16_t dst_port,
                         uint8_t protocol, uint8_t direction);

/* ML-based threat detection (optional) */
#ifdef ENABLE_ML_PLUGIN
void firewall_ml_init(void);
int firewall_ml_analyze_packet(const uint8_t* packet_data, uint32_t length);
uint8_t firewall_ml_get_threat_level(void);
void firewall_ml_update_model(void);
void firewall_ml_mark_false_positive(void);
#endif

/* Statistics */
const firewall_stats_t* firewall_get_stats(void);
void firewall_reset_stats(void);

/* Enable/disable firewall */
void firewall_enable(void);
void firewall_disable(void);
int firewall_is_enabled(void);

#endif /* AURORA_FIREWALL_H */
