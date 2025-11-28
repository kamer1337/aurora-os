/**
 * Aurora OS - DHCP Client Header
 * 
 * Dynamic Host Configuration Protocol client implementation
 */

#ifndef AURORA_DHCP_H
#define AURORA_DHCP_H

#include <stdint.h>

/* DHCP Configuration */
#define DHCP_SERVER_PORT            67
#define DHCP_CLIENT_PORT            68
#define DHCP_MAGIC_COOKIE           0x63825363
#define DHCP_MAX_MESSAGE_SIZE       576
#define DHCP_TIMEOUT_MS             5000
#define DHCP_MAX_RETRIES            4

/* DHCP Message Types */
#define DHCP_DISCOVER               1
#define DHCP_OFFER                  2
#define DHCP_REQUEST                3
#define DHCP_DECLINE                4
#define DHCP_ACK                    5
#define DHCP_NAK                    6
#define DHCP_RELEASE                7
#define DHCP_INFORM                 8

/* DHCP Option Codes */
#define DHCP_OPT_PAD                0
#define DHCP_OPT_SUBNET_MASK        1
#define DHCP_OPT_ROUTER             3
#define DHCP_OPT_DNS_SERVER         6
#define DHCP_OPT_HOSTNAME           12
#define DHCP_OPT_DOMAIN_NAME        15
#define DHCP_OPT_BROADCAST_ADDR     28
#define DHCP_OPT_REQUESTED_IP       50
#define DHCP_OPT_LEASE_TIME         51
#define DHCP_OPT_MESSAGE_TYPE       53
#define DHCP_OPT_SERVER_ID          54
#define DHCP_OPT_PARAM_REQUEST      55
#define DHCP_OPT_RENEWAL_TIME       58
#define DHCP_OPT_REBINDING_TIME     59
#define DHCP_OPT_CLIENT_ID          61
#define DHCP_OPT_END                255

/* BOOTP Operation Codes */
#define BOOTP_REQUEST               1
#define BOOTP_REPLY                 2

/* Hardware Types */
#define HTYPE_ETHERNET              1

/* DHCP Message Structure */
typedef struct {
    uint8_t op;             /* Operation: 1=request, 2=reply */
    uint8_t htype;          /* Hardware type (1=Ethernet) */
    uint8_t hlen;           /* Hardware address length */
    uint8_t hops;           /* Hop count */
    uint32_t xid;           /* Transaction ID */
    uint16_t secs;          /* Seconds since boot */
    uint16_t flags;         /* Flags */
    uint32_t ciaddr;        /* Client IP address */
    uint32_t yiaddr;        /* Your (client) IP address */
    uint32_t siaddr;        /* Server IP address */
    uint32_t giaddr;        /* Gateway IP address */
    uint8_t chaddr[16];     /* Client hardware address */
    uint8_t sname[64];      /* Server host name */
    uint8_t file[128];      /* Boot file name */
    uint8_t options[312];   /* Options (variable length) */
} __attribute__((packed)) dhcp_message_t;

/* DHCP Lease Information */
typedef struct {
    uint32_t ip_address;
    uint32_t subnet_mask;
    uint32_t gateway;
    uint32_t dns_servers[4];
    uint8_t dns_count;
    uint32_t dhcp_server;
    uint32_t lease_time;
    uint32_t renewal_time;
    uint32_t rebinding_time;
    uint64_t lease_start;
    uint8_t valid;
    char hostname[64];
    char domain[64];
} dhcp_lease_t;

/* DHCP Client State */
typedef enum {
    DHCP_STATE_INIT,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING,
    DHCP_STATE_RELEASED
} dhcp_state_t;

/* DHCP Client Structure */
typedef struct {
    dhcp_state_t state;
    dhcp_lease_t lease;
    uint32_t xid;           /* Current transaction ID */
    uint8_t mac_addr[6];    /* Client MAC address */
    uint32_t retry_count;
    uint64_t state_start_time;
    uint8_t initialized;
} dhcp_client_t;

/**
 * Initialize DHCP client
 * @param mac_addr Client MAC address
 */
void dhcp_init(const uint8_t* mac_addr);

/**
 * Start DHCP discovery process
 * @return 0 on success, -1 on error
 */
int dhcp_discover(void);

/**
 * Request specific IP address
 * @param requested_ip IP address to request
 * @return 0 on success, -1 on error
 */
int dhcp_request(uint32_t requested_ip);

/**
 * Release current lease
 * @return 0 on success, -1 on error
 */
int dhcp_release(void);

/**
 * Renew current lease
 * @return 0 on success, -1 on error
 */
int dhcp_renew(void);

/**
 * Decline offered address
 * @param ip_addr IP address to decline
 * @return 0 on success, -1 on error
 */
int dhcp_decline(uint32_t ip_addr);

/**
 * Process incoming DHCP message
 * @param message DHCP message to process
 * @param length Message length
 */
void dhcp_process_message(dhcp_message_t* message, uint32_t length);

/**
 * Get current DHCP state
 * @return Current state
 */
dhcp_state_t dhcp_get_state(void);

/**
 * Get current lease information
 * @return Pointer to lease structure or NULL if no valid lease
 */
const dhcp_lease_t* dhcp_get_lease(void);

/**
 * Check if lease needs renewal
 * @return 1 if renewal needed, 0 otherwise
 */
int dhcp_needs_renewal(void);

/**
 * Check if lease has expired
 * @return 1 if expired, 0 otherwise
 */
int dhcp_is_expired(void);

/**
 * Set hostname for DHCP requests
 * @param hostname Hostname string
 */
void dhcp_set_hostname(const char* hostname);

/**
 * Get assigned IP address
 * @return IP address or 0 if not assigned
 */
uint32_t dhcp_get_ip(void);

/**
 * Get assigned subnet mask
 * @return Subnet mask or 0 if not assigned
 */
uint32_t dhcp_get_netmask(void);

/**
 * Get assigned gateway
 * @return Gateway IP or 0 if not assigned
 */
uint32_t dhcp_get_gateway(void);

/**
 * Get assigned DNS server
 * @param index DNS server index (0-3)
 * @return DNS server IP or 0 if not available
 */
uint32_t dhcp_get_dns(uint8_t index);

/**
 * Perform full DHCP configuration (blocking)
 * @return 0 on success, -1 on error
 */
int dhcp_configure(void);

/**
 * DHCP timer tick - call periodically for lease management
 */
void dhcp_tick(void);

#endif /* AURORA_DHCP_H */
