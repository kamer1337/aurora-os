/**
 * Aurora OS - DHCP Client Implementation
 * 
 * Dynamic Host Configuration Protocol client implementation
 */

#include "dhcp.h"
#include "network.h"
#include "../memory/memory.h"
#include <stddef.h>

/* String utilities */
static size_t dhcp_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void dhcp_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void dhcp_memset(void* ptr, int value, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
}

static void dhcp_memcpy(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

/* Network byte order helpers */
static uint16_t dhcp_htons(uint16_t hostshort) {
    return ((hostshort >> 8) & 0xFF) | ((hostshort & 0xFF) << 8);
}

static uint32_t dhcp_htonl(uint32_t hostlong) {
    return ((hostlong >> 24) & 0xFF) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong << 8) & 0xFF0000) |
           ((hostlong << 24) & 0xFF000000);
}

static uint32_t dhcp_ntohl(uint32_t netlong) {
    return dhcp_htonl(netlong);
}

/* Global DHCP client state */
static dhcp_client_t client;

/* Client hostname */
static char client_hostname[64] = "aurora-os";

/* Fake time for stub implementation */
static uint64_t get_current_time_sec(void) {
    static uint64_t fake_time = 1700000000ULL;
    return fake_time++;
}

/* Generate random transaction ID */
static uint32_t generate_xid(void) {
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return seed;
}

/* Add option to DHCP message */
static int add_option(uint8_t* options, int offset, uint8_t code, uint8_t len, const void* data) {
    if (offset + 2 + len > 312) {
        return offset; /* Not enough space */
    }
    
    options[offset++] = code;
    options[offset++] = len;
    dhcp_memcpy(&options[offset], data, len);
    
    return offset + len;
}

/* Add single byte option */
static int add_option_byte(uint8_t* options, int offset, uint8_t code, uint8_t value) {
    return add_option(options, offset, code, 1, &value);
}

/* Add 4-byte option */
static int add_option_u32(uint8_t* options, int offset, uint8_t code, uint32_t value) {
    uint32_t net_value = dhcp_htonl(value);
    return add_option(options, offset, code, 4, &net_value);
}

/* Find option in DHCP message */
static uint8_t* find_option(uint8_t* options, uint32_t options_len, uint8_t code) {
    uint32_t i = 0;
    
    /* Skip magic cookie */
    if (options_len < 4) return NULL;
    i = 4;
    
    while (i < options_len) {
        uint8_t opt_code = options[i];
        
        if (opt_code == DHCP_OPT_END) {
            break;
        }
        
        if (opt_code == DHCP_OPT_PAD) {
            i++;
            continue;
        }
        
        if (i + 1 >= options_len) {
            break;
        }
        
        uint8_t opt_len = options[i + 1];
        
        if (opt_code == code) {
            return &options[i];
        }
        
        i += 2 + opt_len;
    }
    
    return NULL;
}

/* Get option value as uint32 */
static uint32_t get_option_u32(uint8_t* option) {
    if (!option || option[1] < 4) return 0;
    uint32_t* value = (uint32_t*)&option[2];
    return dhcp_ntohl(*value);
}

/* Get option value as single byte */
static uint8_t get_option_byte(uint8_t* option) {
    if (!option || option[1] < 1) return 0;
    return option[2];
}

/* Build DHCP message */
static int build_message(dhcp_message_t* msg, uint8_t msg_type) {
    dhcp_memset(msg, 0, sizeof(dhcp_message_t));
    
    msg->op = BOOTP_REQUEST;
    msg->htype = HTYPE_ETHERNET;
    msg->hlen = 6;
    msg->hops = 0;
    msg->xid = dhcp_htonl(client.xid);
    msg->secs = 0;
    msg->flags = dhcp_htons(0x8000); /* Broadcast */
    
    /* Copy MAC address */
    dhcp_memcpy(msg->chaddr, client.mac_addr, 6);
    
    /* Set current IP if bound */
    if (client.state == DHCP_STATE_BOUND || 
        client.state == DHCP_STATE_RENEWING ||
        client.state == DHCP_STATE_REBINDING) {
        msg->ciaddr = client.lease.ip_address;
    }
    
    /* Add magic cookie */
    uint32_t magic = dhcp_htonl(DHCP_MAGIC_COOKIE);
    dhcp_memcpy(msg->options, &magic, 4);
    
    int opt_offset = 4;
    
    /* Message type */
    opt_offset = add_option_byte(msg->options, opt_offset, DHCP_OPT_MESSAGE_TYPE, msg_type);
    
    /* Client ID */
    uint8_t client_id[7];
    client_id[0] = HTYPE_ETHERNET;
    dhcp_memcpy(&client_id[1], client.mac_addr, 6);
    opt_offset = add_option(msg->options, opt_offset, DHCP_OPT_CLIENT_ID, 7, client_id);
    
    /* Hostname */
    if (client_hostname[0]) {
        opt_offset = add_option(msg->options, opt_offset, DHCP_OPT_HOSTNAME,
                                (uint8_t)dhcp_strlen(client_hostname), client_hostname);
    }
    
    /* Parameter request list */
    uint8_t param_request[] = {
        DHCP_OPT_SUBNET_MASK,
        DHCP_OPT_ROUTER,
        DHCP_OPT_DNS_SERVER,
        DHCP_OPT_DOMAIN_NAME,
        DHCP_OPT_BROADCAST_ADDR,
        DHCP_OPT_LEASE_TIME,
        DHCP_OPT_RENEWAL_TIME,
        DHCP_OPT_REBINDING_TIME
    };
    opt_offset = add_option(msg->options, opt_offset, DHCP_OPT_PARAM_REQUEST,
                            sizeof(param_request), param_request);
    
    /* Add requested IP if we have one */
    if (msg_type == DHCP_REQUEST && client.lease.ip_address != 0) {
        opt_offset = add_option_u32(msg->options, opt_offset, DHCP_OPT_REQUESTED_IP,
                                    dhcp_ntohl(client.lease.ip_address));
    }
    
    /* Add server ID if we have one */
    if (msg_type == DHCP_REQUEST && client.lease.dhcp_server != 0) {
        opt_offset = add_option_u32(msg->options, opt_offset, DHCP_OPT_SERVER_ID,
                                    dhcp_ntohl(client.lease.dhcp_server));
    }
    
    /* End option */
    msg->options[opt_offset++] = DHCP_OPT_END;
    
    return sizeof(dhcp_message_t) - sizeof(msg->options) + opt_offset;
}

/* Send DHCP message */
static int send_message(dhcp_message_t* msg, int msg_len) {
    /* Create UDP socket */
    socket_t* sock = socket_create(PROTO_UDP);
    if (!sock) {
        return -1;
    }
    
    /* Bind to DHCP client port */
    socket_bind(sock, DHCP_CLIENT_PORT);
    
    /* Broadcast address */
    uint32_t broadcast = 0xFFFFFFFF;
    
    /* Connect to broadcast address */
    if (socket_connect(sock, broadcast, DHCP_SERVER_PORT) < 0) {
        socket_close(sock);
        return -1;
    }
    
    /* Send message */
    if (socket_send(sock, (uint8_t*)msg, (uint32_t)msg_len) < 0) {
        socket_close(sock);
        return -1;
    }
    
    socket_close(sock);
    return 0;
}

/* Receive DHCP message */
static int receive_message(dhcp_message_t* msg, uint32_t timeout_ms) {
    /* Create UDP socket */
    socket_t* sock = socket_create(PROTO_UDP);
    if (!sock) {
        return -1;
    }
    
    /* Bind to DHCP client port */
    socket_bind(sock, DHCP_CLIENT_PORT);
    
    /* Receive message */
    int recv_len = socket_receive(sock, (uint8_t*)msg, sizeof(dhcp_message_t));
    
    socket_close(sock);
    
    if (recv_len < (int)sizeof(dhcp_message_t) - 312) {
        return -1;
    }
    
    /* Verify transaction ID */
    if (dhcp_ntohl(msg->xid) != client.xid) {
        return -1;
    }
    
    /* Verify it's a reply */
    if (msg->op != BOOTP_REPLY) {
        return -1;
    }
    
    (void)timeout_ms;
    
    return recv_len;
}

void dhcp_init(const uint8_t* mac_addr) {
    dhcp_memset(&client, 0, sizeof(dhcp_client_t));
    
    if (mac_addr) {
        dhcp_memcpy(client.mac_addr, mac_addr, 6);
    }
    
    client.state = DHCP_STATE_INIT;
    client.xid = generate_xid();
    client.initialized = 1;
}

int dhcp_discover(void) {
    if (!client.initialized) {
        return -1;
    }
    
    /* Generate new transaction ID */
    client.xid = generate_xid();
    client.state = DHCP_STATE_SELECTING;
    client.state_start_time = get_current_time_sec();
    client.retry_count = 0;
    
    /* Build DHCP DISCOVER message */
    dhcp_message_t msg;
    int msg_len = build_message(&msg, DHCP_DISCOVER);
    
    /* Send message */
    if (send_message(&msg, msg_len) < 0) {
        return -1;
    }
    
    /* Wait for OFFER */
    dhcp_message_t reply;
    int recv_len = receive_message(&reply, DHCP_TIMEOUT_MS);
    
    if (recv_len > 0) {
        dhcp_process_message(&reply, (uint32_t)recv_len);
    }
    
    return (client.state == DHCP_STATE_REQUESTING) ? 0 : -1;
}

int dhcp_request(uint32_t requested_ip) {
    if (!client.initialized) {
        return -1;
    }
    
    /* Store requested IP */
    client.lease.ip_address = requested_ip;
    client.state = DHCP_STATE_REQUESTING;
    
    /* Build DHCP REQUEST message */
    dhcp_message_t msg;
    int msg_len = build_message(&msg, DHCP_REQUEST);
    
    /* Send message */
    if (send_message(&msg, msg_len) < 0) {
        return -1;
    }
    
    /* Wait for ACK/NAK */
    dhcp_message_t reply;
    int recv_len = receive_message(&reply, DHCP_TIMEOUT_MS);
    
    if (recv_len > 0) {
        dhcp_process_message(&reply, (uint32_t)recv_len);
    }
    
    return (client.state == DHCP_STATE_BOUND) ? 0 : -1;
}

int dhcp_release(void) {
    if (!client.initialized || client.state != DHCP_STATE_BOUND) {
        return -1;
    }
    
    /* Build DHCP RELEASE message */
    dhcp_message_t msg;
    int msg_len = build_message(&msg, DHCP_RELEASE);
    
    /* Add server ID */
    /* Message already includes it */
    
    /* Send message */
    send_message(&msg, msg_len);
    
    /* Clear lease */
    dhcp_memset(&client.lease, 0, sizeof(dhcp_lease_t));
    client.state = DHCP_STATE_RELEASED;
    
    return 0;
}

int dhcp_renew(void) {
    if (!client.initialized || client.state != DHCP_STATE_BOUND) {
        return -1;
    }
    
    client.state = DHCP_STATE_RENEWING;
    
    /* Build DHCP REQUEST message (unicast to server) */
    dhcp_message_t msg;
    int msg_len = build_message(&msg, DHCP_REQUEST);
    
    /* Send to DHCP server */
    socket_t* sock = socket_create(PROTO_UDP);
    if (!sock) {
        return -1;
    }
    
    socket_bind(sock, DHCP_CLIENT_PORT);
    
    if (socket_connect(sock, client.lease.dhcp_server, DHCP_SERVER_PORT) < 0) {
        socket_close(sock);
        return -1;
    }
    
    if (socket_send(sock, (uint8_t*)&msg, (uint32_t)msg_len) < 0) {
        socket_close(sock);
        return -1;
    }
    
    /* Receive response */
    dhcp_message_t reply;
    int recv_len = socket_receive(sock, (uint8_t*)&reply, sizeof(dhcp_message_t));
    
    socket_close(sock);
    
    if (recv_len > 0) {
        dhcp_process_message(&reply, (uint32_t)recv_len);
    }
    
    return (client.state == DHCP_STATE_BOUND) ? 0 : -1;
}

int dhcp_decline(uint32_t ip_addr) {
    if (!client.initialized) {
        return -1;
    }
    
    client.lease.ip_address = ip_addr;
    
    /* Build DHCP DECLINE message */
    dhcp_message_t msg;
    int msg_len = build_message(&msg, DHCP_DECLINE);
    
    /* Send message */
    send_message(&msg, msg_len);
    
    /* Reset state */
    dhcp_memset(&client.lease, 0, sizeof(dhcp_lease_t));
    client.state = DHCP_STATE_INIT;
    
    return 0;
}

void dhcp_process_message(dhcp_message_t* message, uint32_t length) {
    if (!message || length < sizeof(dhcp_message_t) - 312) {
        return;
    }
    
    /* Verify magic cookie */
    uint32_t* magic = (uint32_t*)message->options;
    if (dhcp_ntohl(*magic) != DHCP_MAGIC_COOKIE) {
        return;
    }
    
    /* Get message type */
    uint8_t* msg_type_opt = find_option(message->options, length - 
                                         (sizeof(dhcp_message_t) - sizeof(message->options)),
                                         DHCP_OPT_MESSAGE_TYPE);
    if (!msg_type_opt) {
        return;
    }
    
    uint8_t msg_type = get_option_byte(msg_type_opt);
    
    switch (msg_type) {
        case DHCP_OFFER:
            if (client.state == DHCP_STATE_SELECTING) {
                /* Store offered IP */
                client.lease.ip_address = message->yiaddr;
                
                /* Get server ID */
                uint8_t* server_opt = find_option(message->options, 
                                                   sizeof(message->options),
                                                   DHCP_OPT_SERVER_ID);
                if (server_opt) {
                    client.lease.dhcp_server = *(uint32_t*)&server_opt[2];
                }
                
                /* Move to requesting state */
                client.state = DHCP_STATE_REQUESTING;
                
                /* Request the offered IP */
                dhcp_request(message->yiaddr);
            }
            break;
            
        case DHCP_ACK:
            if (client.state == DHCP_STATE_REQUESTING ||
                client.state == DHCP_STATE_RENEWING ||
                client.state == DHCP_STATE_REBINDING) {
                /* Process lease */
                client.lease.ip_address = message->yiaddr;
                client.lease.valid = 1;
                client.lease.lease_start = get_current_time_sec();
                
                /* Parse options */
                uint8_t* opt;
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_SUBNET_MASK);
                if (opt) {
                    client.lease.subnet_mask = *(uint32_t*)&opt[2];
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_ROUTER);
                if (opt) {
                    client.lease.gateway = *(uint32_t*)&opt[2];
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_DNS_SERVER);
                if (opt) {
                    uint8_t dns_len = opt[1];
                    uint8_t dns_count = dns_len / 4;
                    if (dns_count > 4) dns_count = 4;
                    client.lease.dns_count = dns_count;
                    for (uint8_t i = 0; i < dns_count; i++) {
                        client.lease.dns_servers[i] = *(uint32_t*)&opt[2 + i * 4];
                    }
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_LEASE_TIME);
                if (opt) {
                    client.lease.lease_time = get_option_u32(opt);
                } else {
                    client.lease.lease_time = 86400; /* Default 24 hours */
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_RENEWAL_TIME);
                if (opt) {
                    client.lease.renewal_time = get_option_u32(opt);
                } else {
                    client.lease.renewal_time = client.lease.lease_time / 2;
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_REBINDING_TIME);
                if (opt) {
                    client.lease.rebinding_time = get_option_u32(opt);
                } else {
                    client.lease.rebinding_time = (client.lease.lease_time * 7) / 8;
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_DOMAIN_NAME);
                if (opt && opt[1] < sizeof(client.lease.domain)) {
                    dhcp_memcpy(client.lease.domain, &opt[2], opt[1]);
                    client.lease.domain[opt[1]] = '\0';
                }
                
                opt = find_option(message->options, sizeof(message->options), DHCP_OPT_SERVER_ID);
                if (opt) {
                    client.lease.dhcp_server = *(uint32_t*)&opt[2];
                }
                
                client.state = DHCP_STATE_BOUND;
            }
            break;
            
        case DHCP_NAK:
            /* Lease rejected - restart discovery */
            dhcp_memset(&client.lease, 0, sizeof(dhcp_lease_t));
            client.state = DHCP_STATE_INIT;
            break;
            
        default:
            break;
    }
}

dhcp_state_t dhcp_get_state(void) {
    return client.state;
}

const dhcp_lease_t* dhcp_get_lease(void) {
    if (client.lease.valid) {
        return &client.lease;
    }
    return NULL;
}

int dhcp_needs_renewal(void) {
    if (!client.lease.valid || client.state != DHCP_STATE_BOUND) {
        return 0;
    }
    
    uint64_t elapsed = get_current_time_sec() - client.lease.lease_start;
    return (elapsed >= client.lease.renewal_time);
}

int dhcp_is_expired(void) {
    if (!client.lease.valid) {
        return 1;
    }
    
    uint64_t elapsed = get_current_time_sec() - client.lease.lease_start;
    return (elapsed >= client.lease.lease_time);
}

void dhcp_set_hostname(const char* hostname) {
    if (hostname) {
        dhcp_strcpy(client_hostname, hostname, sizeof(client_hostname));
    }
}

uint32_t dhcp_get_ip(void) {
    return client.lease.valid ? client.lease.ip_address : 0;
}

uint32_t dhcp_get_netmask(void) {
    return client.lease.valid ? client.lease.subnet_mask : 0;
}

uint32_t dhcp_get_gateway(void) {
    return client.lease.valid ? client.lease.gateway : 0;
}

uint32_t dhcp_get_dns(uint8_t index) {
    if (!client.lease.valid || index >= client.lease.dns_count) {
        return 0;
    }
    return client.lease.dns_servers[index];
}

int dhcp_configure(void) {
    if (!client.initialized) {
        return -1;
    }
    
    /* Start discovery */
    for (uint32_t retry = 0; retry < DHCP_MAX_RETRIES; retry++) {
        if (dhcp_discover() == 0) {
            return 0;
        }
        client.retry_count++;
    }
    
    return -1;
}

void dhcp_tick(void) {
    if (!client.initialized || !client.lease.valid) {
        return;
    }
    
    switch (client.state) {
        case DHCP_STATE_BOUND:
            if (dhcp_is_expired()) {
                /* Lease expired - restart */
                client.lease.valid = 0;
                client.state = DHCP_STATE_INIT;
            } else if (dhcp_needs_renewal()) {
                /* Try to renew */
                dhcp_renew();
            }
            break;
            
        case DHCP_STATE_RENEWING:
            if (dhcp_is_expired()) {
                /* Move to rebinding */
                client.state = DHCP_STATE_REBINDING;
            }
            break;
            
        case DHCP_STATE_REBINDING:
            if (dhcp_is_expired()) {
                /* Lease expired - restart */
                client.lease.valid = 0;
                client.state = DHCP_STATE_INIT;
            }
            break;
            
        default:
            break;
    }
}
