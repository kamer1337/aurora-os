/**
 * @file network_bridge.c
 * @brief Network Bridge between VMs and Aurora OS
 *
 * Implements virtual network bridge for VM network connectivity
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * NETWORK BRIDGE DEFINITIONS
 * ============================================================================ */

#define NET_BRIDGE_MTU          1500
#define NET_BRIDGE_MAX_PORTS    16
#define NET_BRIDGE_MAX_MACS     256
#define NET_BRIDGE_QUEUE_SIZE   64

/* Ethernet frame header */
typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} eth_header_t;

/* MAC address entry */
typedef struct {
    uint8_t mac[6];
    uint32_t port;
    uint32_t age;
    bool valid;
} mac_entry_t;

/* Network packet */
typedef struct {
    uint8_t data[NET_BRIDGE_MTU + sizeof(eth_header_t)];
    uint32_t length;
    uint32_t src_port;
    bool valid;
} net_packet_t;

/* Bridge port types */
typedef enum {
    PORT_TYPE_NONE = 0,
    PORT_TYPE_VM,           /* Virtual machine port */
    PORT_TYPE_HOST,         /* Host network interface */
    PORT_TYPE_TAP,          /* TAP device */
    PORT_TYPE_VETH          /* Virtual ethernet pair */
} port_type_t;

/* Bridge port */
typedef struct {
    bool active;
    port_type_t type;
    uint8_t mac[6];
    char name[32];
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
    /* Packet queues */
    net_packet_t rx_queue[NET_BRIDGE_QUEUE_SIZE];
    net_packet_t tx_queue[NET_BRIDGE_QUEUE_SIZE];
    uint32_t rx_head, rx_tail;
    uint32_t tx_head, tx_tail;
} bridge_port_t;

/* Network bridge */
typedef struct {
    bool active;
    char name[32];
    uint8_t mac[6];             /* Bridge MAC address */
    bridge_port_t ports[NET_BRIDGE_MAX_PORTS];
    uint32_t port_count;
    mac_entry_t mac_table[NET_BRIDGE_MAX_MACS];
    uint32_t mac_count;
    /* Statistics */
    uint64_t total_rx;
    uint64_t total_tx;
    uint64_t forwarded;
    uint64_t flooded;
    /* Configuration */
    bool stp_enabled;           /* Spanning Tree Protocol */
    bool learning_enabled;      /* MAC learning */
    uint32_t ageing_time;       /* MAC ageing time in seconds */
} network_bridge_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

#define MAX_BRIDGES 4

static network_bridge_t g_bridges[MAX_BRIDGES];
static bool g_bridge_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Compare MAC addresses
 */
static bool mac_equal(const uint8_t* a, const uint8_t* b) {
    return platform_memcmp(a, b, 6) == 0;
}

/**
 * Copy MAC address
 */
static void mac_copy(uint8_t* dst, const uint8_t* src) {
    platform_memcpy(dst, src, 6);
}

/**
 * Check if MAC is broadcast
 */
static bool mac_is_broadcast(const uint8_t* mac) {
    return mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
           mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF;
}

/**
 * Check if MAC is multicast
 */
static bool mac_is_multicast(const uint8_t* mac) {
    return (mac[0] & 0x01) != 0;
}

/**
 * Find MAC entry in table
 */
static int find_mac_entry(network_bridge_t* bridge, const uint8_t* mac) {
    for (uint32_t i = 0; i < bridge->mac_count; i++) {
        if (bridge->mac_table[i].valid && mac_equal(bridge->mac_table[i].mac, mac)) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * Add MAC entry to table
 */
static int add_mac_entry(network_bridge_t* bridge, const uint8_t* mac, uint32_t port) {
    /* Check if already exists */
    int idx = find_mac_entry(bridge, mac);
    if (idx >= 0) {
        bridge->mac_table[idx].port = port;
        bridge->mac_table[idx].age = 0;
        return idx;
    }
    
    /* Find free slot */
    for (uint32_t i = 0; i < NET_BRIDGE_MAX_MACS; i++) {
        if (!bridge->mac_table[i].valid) {
            mac_copy(bridge->mac_table[i].mac, mac);
            bridge->mac_table[i].port = port;
            bridge->mac_table[i].age = 0;
            bridge->mac_table[i].valid = true;
            bridge->mac_count++;
            return (int)i;
        }
    }
    
    return -1; /* Table full */
}

/**
 * Queue packet for transmission
 */
static int queue_packet(bridge_port_t* port, const uint8_t* data, uint32_t len, bool is_rx) {
    net_packet_t* queue = is_rx ? port->rx_queue : port->tx_queue;
    uint32_t* head = is_rx ? &port->rx_head : &port->tx_head;
    uint32_t* tail = is_rx ? &port->rx_tail : &port->tx_tail;
    
    uint32_t next_tail = (*tail + 1) % NET_BRIDGE_QUEUE_SIZE;
    if (next_tail == *head) {
        /* Queue full */
        if (is_rx) port->rx_dropped++;
        else port->tx_dropped++;
        return -1;
    }
    
    if (len > sizeof(queue[*tail].data)) {
        return -1;
    }
    
    platform_memcpy(queue[*tail].data, data, len);
    queue[*tail].length = len;
    queue[*tail].valid = true;
    *tail = next_tail;
    
    return 0;
}

/**
 * Dequeue packet
 */
static int dequeue_packet(bridge_port_t* port, uint8_t* data, uint32_t* len, bool is_rx) {
    net_packet_t* queue = is_rx ? port->rx_queue : port->tx_queue;
    uint32_t* head = is_rx ? &port->rx_head : &port->rx_head;
    uint32_t* tail = is_rx ? &port->rx_tail : &port->tx_tail;
    
    if (*head == *tail) {
        return -1; /* Queue empty */
    }
    
    if (queue[*head].valid) {
        platform_memcpy(data, queue[*head].data, queue[*head].length);
        *len = queue[*head].length;
        queue[*head].valid = false;
        *head = (*head + 1) % NET_BRIDGE_QUEUE_SIZE;
        return 0;
    }
    
    return -1;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Initialize network bridge subsystem
 */
int network_bridge_init(void) {
    if (g_bridge_initialized) {
        return 0;
    }
    
    platform_memset(g_bridges, 0, sizeof(g_bridges));
    g_bridge_initialized = true;
    
    return 0;
}

/**
 * Create a new network bridge
 */
int network_bridge_create(const char* name) {
    if (!g_bridge_initialized) {
        network_bridge_init();
    }
    
    /* Find free slot */
    int bridge_id = -1;
    for (int i = 0; i < MAX_BRIDGES; i++) {
        if (!g_bridges[i].active) {
            bridge_id = i;
            break;
        }
    }
    
    if (bridge_id < 0) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    platform_memset(bridge, 0, sizeof(network_bridge_t));
    
    /* Set bridge properties */
    bridge->active = true;
    if (name) {
        platform_strncpy(bridge->name, name, sizeof(bridge->name));
    } else {
        bridge->name[0] = 'b';
        bridge->name[1] = 'r';
        bridge->name[2] = '0' + bridge_id;
        bridge->name[3] = '\0';
    }
    
    /* Generate bridge MAC */
    bridge->mac[0] = 0x52;  /* Local administered */
    bridge->mac[1] = 0x54;
    bridge->mac[2] = 0x00;
    bridge->mac[3] = 0x12;
    bridge->mac[4] = 0x34;
    bridge->mac[5] = 0x50 + bridge_id;
    
    /* Default configuration */
    bridge->learning_enabled = true;
    bridge->ageing_time = 300;
    bridge->stp_enabled = false;
    
    return bridge_id;
}

/**
 * Destroy network bridge
 */
int network_bridge_destroy(int bridge_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active) {
        return -1;
    }
    
    bridge->active = false;
    return 0;
}

/**
 * Add port to bridge
 */
int network_bridge_add_port(int bridge_id, const char* name, port_type_t type, const uint8_t* mac) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active) {
        return -1;
    }
    
    /* Find free port */
    int port_id = -1;
    for (int i = 0; i < NET_BRIDGE_MAX_PORTS; i++) {
        if (!bridge->ports[i].active) {
            port_id = i;
            break;
        }
    }
    
    if (port_id < 0) {
        return -1;
    }
    
    bridge_port_t* port = &bridge->ports[port_id];
    platform_memset(port, 0, sizeof(bridge_port_t));
    
    port->active = true;
    port->type = type;
    
    if (name) {
        platform_strncpy(port->name, name, sizeof(port->name));
    }
    
    if (mac) {
        mac_copy(port->mac, mac);
    } else {
        /* Generate port MAC */
        port->mac[0] = 0x52;
        port->mac[1] = 0x54;
        port->mac[2] = 0x00;
        port->mac[3] = 0x12;
        port->mac[4] = 0x34;
        port->mac[5] = 0x60 + port_id;
    }
    
    bridge->port_count++;
    
    return port_id;
}

/**
 * Remove port from bridge
 */
int network_bridge_remove_port(int bridge_id, int port_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active || port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS) {
        return -1;
    }
    
    bridge_port_t* port = &bridge->ports[port_id];
    if (!port->active) {
        return -1;
    }
    
    /* Remove MAC entries for this port */
    for (uint32_t i = 0; i < NET_BRIDGE_MAX_MACS; i++) {
        if (bridge->mac_table[i].valid && bridge->mac_table[i].port == (uint32_t)port_id) {
            bridge->mac_table[i].valid = false;
            bridge->mac_count--;
        }
    }
    
    port->active = false;
    bridge->port_count--;
    
    return 0;
}

/**
 * Forward packet through bridge
 */
int network_bridge_forward(int bridge_id, int src_port, const uint8_t* packet, uint32_t length) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active || length < sizeof(eth_header_t)) {
        return -1;
    }
    
    const eth_header_t* eth = (const eth_header_t*)packet;
    
    /* Learn source MAC */
    if (bridge->learning_enabled && src_port >= 0 && !mac_is_multicast(eth->src_mac)) {
        add_mac_entry(bridge, eth->src_mac, src_port);
    }
    
    /* Update statistics */
    bridge->total_rx++;
    if (src_port >= 0 && src_port < NET_BRIDGE_MAX_PORTS) {
        bridge->ports[src_port].rx_packets++;
        bridge->ports[src_port].rx_bytes += length;
    }
    
    /* Determine destination */
    if (mac_is_broadcast(eth->dst_mac) || mac_is_multicast(eth->dst_mac)) {
        /* Flood to all ports except source */
        for (int i = 0; i < NET_BRIDGE_MAX_PORTS; i++) {
            if (i != src_port && bridge->ports[i].active) {
                queue_packet(&bridge->ports[i], packet, length, false);
                bridge->ports[i].tx_packets++;
                bridge->ports[i].tx_bytes += length;
            }
        }
        bridge->flooded++;
        if (bridge->port_count > 1) {
            bridge->total_tx += bridge->port_count - 1;
        }
    } else {
        /* Lookup destination MAC */
        int mac_idx = find_mac_entry(bridge, eth->dst_mac);
        
        if (mac_idx >= 0) {
            /* Forward to specific port */
            uint32_t dst_port = bridge->mac_table[mac_idx].port;
            if (dst_port != (uint32_t)src_port && bridge->ports[dst_port].active) {
                queue_packet(&bridge->ports[dst_port], packet, length, false);
                bridge->ports[dst_port].tx_packets++;
                bridge->ports[dst_port].tx_bytes += length;
                bridge->forwarded++;
                bridge->total_tx++;
            }
        } else {
            /* Unknown destination - flood */
            for (int i = 0; i < NET_BRIDGE_MAX_PORTS; i++) {
                if (i != src_port && bridge->ports[i].active) {
                    queue_packet(&bridge->ports[i], packet, length, false);
                    bridge->ports[i].tx_packets++;
                    bridge->ports[i].tx_bytes += length;
                }
            }
            bridge->flooded++;
            if (bridge->port_count > 1) {
                bridge->total_tx += bridge->port_count - 1;
            }
        }
    }
    
    return 0;
}

/**
 * Send packet from VM to bridge
 */
int network_bridge_send(int bridge_id, int port_id, const uint8_t* packet, uint32_t length) {
    return network_bridge_forward(bridge_id, port_id, packet, length);
}

/**
 * Receive packet from bridge to VM
 */
int network_bridge_receive(int bridge_id, int port_id, uint8_t* packet, uint32_t* length) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active || port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS) {
        return -1;
    }
    
    bridge_port_t* port = &bridge->ports[port_id];
    if (!port->active) {
        return -1;
    }
    
    return dequeue_packet(port, packet, length, false);
}

/**
 * Get bridge statistics
 */
int network_bridge_get_stats(int bridge_id, uint64_t* rx, uint64_t* tx, 
                              uint64_t* forwarded, uint64_t* flooded) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active) {
        return -1;
    }
    
    if (rx) *rx = bridge->total_rx;
    if (tx) *tx = bridge->total_tx;
    if (forwarded) *forwarded = bridge->forwarded;
    if (flooded) *flooded = bridge->flooded;
    
    return 0;
}

/**
 * Get port statistics
 */
int network_bridge_get_port_stats(int bridge_id, int port_id,
                                   uint32_t* rx_packets, uint32_t* tx_packets,
                                   uint32_t* rx_bytes, uint32_t* tx_bytes) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return -1;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active || port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS) {
        return -1;
    }
    
    bridge_port_t* port = &bridge->ports[port_id];
    if (!port->active) {
        return -1;
    }
    
    if (rx_packets) *rx_packets = port->rx_packets;
    if (tx_packets) *tx_packets = port->tx_packets;
    if (rx_bytes) *rx_bytes = port->rx_bytes;
    if (tx_bytes) *tx_bytes = port->tx_bytes;
    
    return 0;
}

/**
 * Age MAC table entries
 */
void network_bridge_age_macs(int bridge_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES) {
        return;
    }
    
    network_bridge_t* bridge = &g_bridges[bridge_id];
    if (!bridge->active) {
        return;
    }
    
    for (uint32_t i = 0; i < NET_BRIDGE_MAX_MACS; i++) {
        if (bridge->mac_table[i].valid) {
            bridge->mac_table[i].age++;
            if (bridge->mac_table[i].age > bridge->ageing_time) {
                bridge->mac_table[i].valid = false;
                bridge->mac_count--;
            }
        }
    }
}

/**
 * Get network bridge version
 */
const char* network_bridge_get_version(void) {
    return "1.0.0-aurora-bridge";
}

/* ============================================================================
 * VLAN SUPPORT
 * ============================================================================ */

#define MAX_VLANS 4096
#define VLAN_TAG_SIZE 4

/* 802.1Q VLAN tag */
typedef struct __attribute__((packed)) {
    uint16_t tpid;      /* Tag Protocol ID (0x8100) */
    uint16_t tci;       /* Tag Control Information (PCP, DEI, VID) */
} vlan_tag_t;

/* VLAN configuration per port */
typedef struct {
    bool enabled;
    uint16_t pvid;              /* Port VLAN ID */
    bool untagged;              /* Untagged port */
    uint64_t vlan_bitmap[64];   /* Bitmap of allowed VLANs (4096 bits) */
} port_vlan_config_t;

static port_vlan_config_t g_port_vlan_config[MAX_BRIDGES][NET_BRIDGE_MAX_PORTS];

/**
 * Set VLAN ID bit in bitmap
 */
static void vlan_bitmap_set(uint64_t* bitmap, uint16_t vlan_id) {
    if (vlan_id < MAX_VLANS) {
        bitmap[vlan_id / 64] |= (1ULL << (vlan_id % 64));
    }
}

/**
 * Clear VLAN ID bit in bitmap
 */
static void vlan_bitmap_clear(uint64_t* bitmap, uint16_t vlan_id) {
    if (vlan_id < MAX_VLANS) {
        bitmap[vlan_id / 64] &= ~(1ULL << (vlan_id % 64));
    }
}

/**
 * Check if VLAN ID is set in bitmap
 */
static bool vlan_bitmap_isset(const uint64_t* bitmap, uint16_t vlan_id) {
    if (vlan_id >= MAX_VLANS) return false;
    return (bitmap[vlan_id / 64] & (1ULL << (vlan_id % 64))) != 0;
}

/**
 * Enable VLAN on bridge port
 */
int network_bridge_enable_vlan(int bridge_id, int port_id, uint16_t pvid, bool untagged) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES ||
        port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS) {
        return -1;
    }
    
    if (!g_bridges[bridge_id].active || !g_bridges[bridge_id].ports[port_id].active) {
        return -1;
    }
    
    port_vlan_config_t* cfg = &g_port_vlan_config[bridge_id][port_id];
    cfg->enabled = true;
    cfg->pvid = pvid;
    cfg->untagged = untagged;
    
    /* Allow the PVID by default */
    vlan_bitmap_set(cfg->vlan_bitmap, pvid);
    
    return 0;
}

/**
 * Disable VLAN on bridge port
 */
int network_bridge_disable_vlan(int bridge_id, int port_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES ||
        port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS) {
        return -1;
    }
    
    port_vlan_config_t* cfg = &g_port_vlan_config[bridge_id][port_id];
    cfg->enabled = false;
    
    return 0;
}

/**
 * Add VLAN to port
 */
int network_bridge_add_vlan(int bridge_id, int port_id, uint16_t vlan_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES ||
        port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS ||
        vlan_id >= MAX_VLANS) {
        return -1;
    }
    
    port_vlan_config_t* cfg = &g_port_vlan_config[bridge_id][port_id];
    if (!cfg->enabled) {
        return -1;
    }
    
    vlan_bitmap_set(cfg->vlan_bitmap, vlan_id);
    return 0;
}

/**
 * Remove VLAN from port
 */
int network_bridge_remove_vlan(int bridge_id, int port_id, uint16_t vlan_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES ||
        port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS ||
        vlan_id >= MAX_VLANS) {
        return -1;
    }
    
    port_vlan_config_t* cfg = &g_port_vlan_config[bridge_id][port_id];
    if (!cfg->enabled) {
        return -1;
    }
    
    vlan_bitmap_clear(cfg->vlan_bitmap, vlan_id);
    return 0;
}

/**
 * Check if VLAN is allowed on port
 */
bool network_bridge_is_vlan_allowed(int bridge_id, int port_id, uint16_t vlan_id) {
    if (bridge_id < 0 || bridge_id >= MAX_BRIDGES ||
        port_id < 0 || port_id >= NET_BRIDGE_MAX_PORTS) {
        return false;
    }
    
    port_vlan_config_t* cfg = &g_port_vlan_config[bridge_id][port_id];
    if (!cfg->enabled) {
        return true;  /* No VLAN filtering */
    }
    
    return vlan_bitmap_isset(cfg->vlan_bitmap, vlan_id);
}

/**
 * Get VLAN ID from packet
 */
uint16_t network_bridge_get_vlan_id(const uint8_t* packet, uint32_t length) {
    if (length < sizeof(eth_header_t) + VLAN_TAG_SIZE) {
        return 0;
    }
    
    const eth_header_t* eth = (const eth_header_t*)packet;
    
    /* Check for VLAN tag (0x8100) */
    if (eth->ethertype == 0x0081) {  /* Network byte order */
        const vlan_tag_t* vlan = (const vlan_tag_t*)(packet + sizeof(eth_header_t));
        return (vlan->tci >> 4) & 0x0FFF;  /* Extract VID */
    }
    
    return 0;  /* No VLAN tag */
}

/* ============================================================================
 * NAT SUPPORT
 * ============================================================================ */

#define NAT_TABLE_SIZE 256

/* NAT entry */
typedef struct {
    bool active;
    uint32_t internal_ip;
    uint16_t internal_port;
    uint32_t external_ip;
    uint16_t external_port;
    uint8_t protocol;   /* 6=TCP, 17=UDP */
    uint32_t timeout;
    uint64_t packets;
    uint64_t bytes;
} nat_entry_t;

/* NAT table */
typedef struct {
    bool enabled;
    uint32_t external_ip;       /* WAN IP address */
    uint32_t internal_network;  /* LAN network */
    uint32_t internal_mask;     /* LAN netmask */
    nat_entry_t entries[NAT_TABLE_SIZE];
    uint32_t entry_count;
    uint16_t next_port;         /* Next available NAT port */
} nat_table_t;

static nat_table_t g_nat_table = {0};

/**
 * Initialize NAT
 */
int network_nat_init(uint32_t external_ip, uint32_t internal_network, uint32_t internal_mask) {
    platform_memset(&g_nat_table, 0, sizeof(nat_table_t));
    
    g_nat_table.external_ip = external_ip;
    g_nat_table.internal_network = internal_network;
    g_nat_table.internal_mask = internal_mask;
    g_nat_table.next_port = 10000;
    g_nat_table.enabled = true;
    
    return 0;
}

/**
 * Find NAT entry by internal endpoint
 */
static int nat_find_by_internal(uint32_t ip, uint16_t port, uint8_t protocol) {
    for (uint32_t i = 0; i < NAT_TABLE_SIZE; i++) {
        if (g_nat_table.entries[i].active &&
            g_nat_table.entries[i].internal_ip == ip &&
            g_nat_table.entries[i].internal_port == port &&
            g_nat_table.entries[i].protocol == protocol) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * Find NAT entry by external port
 */
static int nat_find_by_external(uint16_t port, uint8_t protocol) {
    for (uint32_t i = 0; i < NAT_TABLE_SIZE; i++) {
        if (g_nat_table.entries[i].active &&
            g_nat_table.entries[i].external_port == port &&
            g_nat_table.entries[i].protocol == protocol) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * Create NAT entry
 */
static int nat_create_entry(uint32_t internal_ip, uint16_t internal_port, uint8_t protocol) {
    /* Find free slot */
    int idx = -1;
    for (uint32_t i = 0; i < NAT_TABLE_SIZE; i++) {
        if (!g_nat_table.entries[i].active) {
            idx = (int)i;
            break;
        }
    }
    
    if (idx < 0) {
        return -1;  /* Table full */
    }
    
    nat_entry_t* entry = &g_nat_table.entries[idx];
    entry->active = true;
    entry->internal_ip = internal_ip;
    entry->internal_port = internal_port;
    entry->external_ip = g_nat_table.external_ip;
    entry->external_port = g_nat_table.next_port++;
    entry->protocol = protocol;
    entry->timeout = 300;  /* 5 minute default timeout */
    entry->packets = 0;
    entry->bytes = 0;
    
    g_nat_table.entry_count++;
    
    /* Wrap around port number */
    if (g_nat_table.next_port > 65000) {
        g_nat_table.next_port = 10000;
    }
    
    return idx;
}

/**
 * Perform outbound NAT translation
 */
int network_nat_translate_outbound(uint8_t* packet, uint32_t* length,
                                    uint32_t src_ip, uint16_t src_port,
                                    uint8_t protocol) {
    if (!g_nat_table.enabled || !packet || !length) {
        return -1;
    }
    
    /* Check if source is from internal network */
    if ((src_ip & g_nat_table.internal_mask) != g_nat_table.internal_network) {
        return 0;  /* Not internal, no NAT needed */
    }
    
    /* Find or create NAT entry */
    int idx = nat_find_by_internal(src_ip, src_port, protocol);
    if (idx < 0) {
        idx = nat_create_entry(src_ip, src_port, protocol);
        if (idx < 0) {
            return -1;  /* Failed to create entry */
        }
    }
    
    nat_entry_t* entry = &g_nat_table.entries[idx];
    entry->packets++;
    entry->bytes += *length;
    entry->timeout = 300;  /* Reset timeout */
    
    /* NAT translation would modify IP header here */
    /* For simplicity, we just return the external port */
    
    return (int)entry->external_port;
}

/**
 * Perform inbound NAT translation
 */
int network_nat_translate_inbound(uint8_t* packet, uint32_t* length,
                                   uint16_t dst_port, uint8_t protocol,
                                   uint32_t* internal_ip, uint16_t* internal_port) {
    if (!g_nat_table.enabled || !packet || !length) {
        return -1;
    }
    
    int idx = nat_find_by_external(dst_port, protocol);
    if (idx < 0) {
        return -1;  /* No matching NAT entry */
    }
    
    nat_entry_t* entry = &g_nat_table.entries[idx];
    entry->packets++;
    entry->bytes += *length;
    entry->timeout = 300;
    
    if (internal_ip) *internal_ip = entry->internal_ip;
    if (internal_port) *internal_port = entry->internal_port;
    
    return 0;
}

/**
 * Age NAT entries
 */
void network_nat_age_entries(void) {
    for (uint32_t i = 0; i < NAT_TABLE_SIZE; i++) {
        if (g_nat_table.entries[i].active) {
            if (g_nat_table.entries[i].timeout > 0) {
                g_nat_table.entries[i].timeout--;
            } else {
                g_nat_table.entries[i].active = false;
                g_nat_table.entry_count--;
            }
        }
    }
}

/**
 * Get NAT statistics
 */
int network_nat_get_stats(uint32_t* entry_count, uint64_t* total_packets, uint64_t* total_bytes) {
    if (!g_nat_table.enabled) {
        return -1;
    }
    
    uint64_t packets = 0, bytes = 0;
    
    for (uint32_t i = 0; i < NAT_TABLE_SIZE; i++) {
        if (g_nat_table.entries[i].active) {
            packets += g_nat_table.entries[i].packets;
            bytes += g_nat_table.entries[i].bytes;
        }
    }
    
    if (entry_count) *entry_count = g_nat_table.entry_count;
    if (total_packets) *total_packets = packets;
    if (total_bytes) *total_bytes = bytes;
    
    return 0;
}

/**
 * Disable NAT
 */
void network_nat_shutdown(void) {
    g_nat_table.enabled = false;
    platform_memset(&g_nat_table, 0, sizeof(nat_table_t));
}

/* ============================================================================
 * DHCP SERVER FOR VMs
 * ============================================================================ */

#define DHCP_POOL_SIZE 64

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    uint32_t lease_time;
    bool assigned;
} dhcp_lease_t;

typedef struct {
    bool enabled;
    uint32_t server_ip;
    uint32_t pool_start;
    uint32_t pool_end;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns;
    uint32_t lease_duration;
    dhcp_lease_t leases[DHCP_POOL_SIZE];
} dhcp_server_t;

static dhcp_server_t g_dhcp_server = {0};

/**
 * Initialize DHCP server for VMs
 */
int network_dhcp_init(uint32_t server_ip, uint32_t pool_start, uint32_t pool_end,
                       uint32_t netmask, uint32_t gateway, uint32_t dns) {
    platform_memset(&g_dhcp_server, 0, sizeof(dhcp_server_t));
    
    g_dhcp_server.enabled = true;
    g_dhcp_server.server_ip = server_ip;
    g_dhcp_server.pool_start = pool_start;
    g_dhcp_server.pool_end = pool_end;
    g_dhcp_server.netmask = netmask;
    g_dhcp_server.gateway = gateway;
    g_dhcp_server.dns = dns;
    g_dhcp_server.lease_duration = 86400;  /* 24 hours */
    
    return 0;
}

/**
 * Find existing lease for MAC
 */
static dhcp_lease_t* dhcp_find_lease_by_mac(const uint8_t* mac) {
    for (int i = 0; i < DHCP_POOL_SIZE; i++) {
        if (g_dhcp_server.leases[i].assigned && 
            mac_equal(g_dhcp_server.leases[i].mac, mac)) {
            return &g_dhcp_server.leases[i];
        }
    }
    return NULL;
}

/**
 * Allocate IP from DHCP pool
 */
int network_dhcp_allocate(const uint8_t* mac, uint32_t* ip) {
    if (!g_dhcp_server.enabled || !mac || !ip) {
        return -1;
    }
    
    /* Check for existing lease */
    dhcp_lease_t* lease = dhcp_find_lease_by_mac(mac);
    if (lease) {
        lease->lease_time = g_dhcp_server.lease_duration;
        *ip = lease->ip;
        return 0;
    }
    
    /* Find free IP in pool */
    for (uint32_t addr = g_dhcp_server.pool_start; addr <= g_dhcp_server.pool_end; addr++) {
        bool in_use = false;
        for (int i = 0; i < DHCP_POOL_SIZE; i++) {
            if (g_dhcp_server.leases[i].assigned && g_dhcp_server.leases[i].ip == addr) {
                in_use = true;
                break;
            }
        }
        
        if (!in_use) {
            /* Find free lease slot */
            for (int i = 0; i < DHCP_POOL_SIZE; i++) {
                if (!g_dhcp_server.leases[i].assigned) {
                    g_dhcp_server.leases[i].assigned = true;
                    g_dhcp_server.leases[i].ip = addr;
                    mac_copy(g_dhcp_server.leases[i].mac, mac);
                    g_dhcp_server.leases[i].lease_time = g_dhcp_server.lease_duration;
                    *ip = addr;
                    return 0;
                }
            }
        }
    }
    
    return -1;  /* Pool exhausted */
}

/**
 * Release DHCP lease
 */
int network_dhcp_release(const uint8_t* mac) {
    if (!g_dhcp_server.enabled || !mac) {
        return -1;
    }
    
    dhcp_lease_t* lease = dhcp_find_lease_by_mac(mac);
    if (lease) {
        lease->assigned = false;
        return 0;
    }
    
    return -1;
}

/**
 * Get DHCP configuration for VM
 */
int network_dhcp_get_config(uint32_t* netmask, uint32_t* gateway, uint32_t* dns) {
    if (!g_dhcp_server.enabled) {
        return -1;
    }
    
    if (netmask) *netmask = g_dhcp_server.netmask;
    if (gateway) *gateway = g_dhcp_server.gateway;
    if (dns) *dns = g_dhcp_server.dns;
    
    return 0;
}

/**
 * Age DHCP leases
 */
void network_dhcp_age_leases(void) {
    for (int i = 0; i < DHCP_POOL_SIZE; i++) {
        if (g_dhcp_server.leases[i].assigned) {
            if (g_dhcp_server.leases[i].lease_time > 0) {
                g_dhcp_server.leases[i].lease_time--;
            } else {
                g_dhcp_server.leases[i].assigned = false;
            }
        }
    }
}

/**
 * Shutdown DHCP server
 */
void network_dhcp_shutdown(void) {
    g_dhcp_server.enabled = false;
}
