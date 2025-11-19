/**
 * Aurora OS - Network Configuration Manager Implementation
 */

#include "network_config.h"
#include "network.h"
#include <stddef.h>

/* Global network configuration */
static net_config_t global_config = {
    .ip_address = 0,
    .subnet_mask = 0,
    .gateway = 0,
    .dns_count = 0,
    .config_mode = NET_CONFIG_DHCP,
    .enabled = 1
};

/* Helper function to copy string safely */
static void safe_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* Helper function to compare strings */
static int str_len(const char* str) {
    int len = 0;
    while (str[len] != '\0' && len < MAX_HOSTNAME_LEN) {
        len++;
    }
    return len;
}

/**
 * Initialize network configuration subsystem
 */
void net_config_init(void) {
    /* Set default hostname */
    safe_strcpy(global_config.hostname, "aurora-os", MAX_HOSTNAME_LEN);
    
    /* Initialize DNS servers to 0 */
    for (int i = 0; i < MAX_DNS_SERVERS; i++) {
        global_config.dns_servers[i] = 0;
    }
    
    /* Try to load saved configuration */
    net_config_load();
}

/**
 * Configure static IP address
 */
int net_config_set_static(uint32_t ip, uint32_t mask, uint32_t gateway) {
    if (ip == 0) {
        return -1;  /* Invalid IP */
    }
    
    global_config.ip_address = ip;
    global_config.subnet_mask = mask;
    global_config.gateway = gateway;
    global_config.config_mode = NET_CONFIG_STATIC;
    
    return 0;
}

/**
 * Configure DHCP mode
 */
int net_config_set_dhcp(void) {
    global_config.config_mode = NET_CONFIG_DHCP;
    
    /* In real implementation, would initiate DHCP discovery here */
    /* For now, we just set the mode */
    
    return 0;
}

/**
 * Add DNS server
 */
int net_config_add_dns(uint32_t dns_ip) {
    if (global_config.dns_count >= MAX_DNS_SERVERS) {
        return -1;  /* DNS server list full */
    }
    
    /* Check if DNS already exists */
    for (int i = 0; i < global_config.dns_count; i++) {
        if (global_config.dns_servers[i] == dns_ip) {
            return 0;  /* Already added */
        }
    }
    
    global_config.dns_servers[global_config.dns_count++] = dns_ip;
    return 0;
}

/**
 * Remove DNS server
 */
int net_config_remove_dns(uint32_t dns_ip) {
    int found = -1;
    
    /* Find the DNS server */
    for (int i = 0; i < global_config.dns_count; i++) {
        if (global_config.dns_servers[i] == dns_ip) {
            found = i;
            break;
        }
    }
    
    if (found < 0) {
        return -1;  /* Not found */
    }
    
    /* Shift remaining DNS servers */
    for (int i = found; i < global_config.dns_count - 1; i++) {
        global_config.dns_servers[i] = global_config.dns_servers[i + 1];
    }
    
    global_config.dns_count--;
    global_config.dns_servers[global_config.dns_count] = 0;
    
    return 0;
}

/**
 * Clear all DNS servers
 */
void net_config_clear_dns(void) {
    for (int i = 0; i < MAX_DNS_SERVERS; i++) {
        global_config.dns_servers[i] = 0;
    }
    global_config.dns_count = 0;
}

/**
 * Set hostname
 */
int net_config_set_hostname(const char* hostname) {
    if (hostname == NULL || str_len(hostname) == 0) {
        return -1;  /* Invalid hostname */
    }
    
    safe_strcpy(global_config.hostname, hostname, MAX_HOSTNAME_LEN);
    return 0;
}

/**
 * Get hostname
 */
const char* net_config_get_hostname(void) {
    return global_config.hostname;
}

/**
 * Get current configuration
 */
const net_config_t* net_config_get(void) {
    return &global_config;
}

/**
 * Apply configuration to network interface
 */
int net_config_apply(void) {
    /* In a real implementation, this would configure the actual network interface */
    /* For now, we just validate and return success */
    
    if (global_config.config_mode == NET_CONFIG_STATIC) {
        if (global_config.ip_address == 0) {
            return -1;  /* Invalid static configuration */
        }
    }
    
    return 0;
}

/**
 * Save configuration to persistent storage
 */
int net_config_save(void) {
    /* In real implementation, would write to file system */
    /* For now, this is a placeholder */
    return 0;
}

/**
 * Load configuration from persistent storage
 */
int net_config_load(void) {
    /* In real implementation, would read from file system */
    /* For now, this is a placeholder */
    return 0;
}
