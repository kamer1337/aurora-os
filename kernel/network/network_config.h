/**
 * Aurora OS - Network Configuration Manager
 * 
 * Provides network settings configuration and management
 */

#ifndef AURORA_NETWORK_CONFIG_H
#define AURORA_NETWORK_CONFIG_H

#include <stdint.h>

/* Network configuration modes */
#define NET_CONFIG_STATIC    0
#define NET_CONFIG_DHCP      1

/* DNS configuration */
#define MAX_DNS_SERVERS      4
#define MAX_HOSTNAME_LEN     64

/* Network interface configuration */
typedef struct {
    uint32_t ip_address;
    uint32_t subnet_mask;
    uint32_t gateway;
    uint32_t dns_servers[MAX_DNS_SERVERS];
    uint8_t dns_count;
    uint8_t config_mode;  /* STATIC or DHCP */
    char hostname[MAX_HOSTNAME_LEN];
    uint8_t enabled;
} net_config_t;

/* Initialize network configuration subsystem */
void net_config_init(void);

/* Configure network interface */
int net_config_set_static(uint32_t ip, uint32_t mask, uint32_t gateway);
int net_config_set_dhcp(void);

/* DNS configuration */
int net_config_add_dns(uint32_t dns_ip);
int net_config_remove_dns(uint32_t dns_ip);
void net_config_clear_dns(void);

/* Hostname configuration */
int net_config_set_hostname(const char* hostname);
const char* net_config_get_hostname(void);

/* Get current configuration */
const net_config_t* net_config_get(void);

/* Apply configuration to interface */
int net_config_apply(void);

/* Save/Load configuration */
int net_config_save(void);
int net_config_load(void);

#endif /* AURORA_NETWORK_CONFIG_H */
