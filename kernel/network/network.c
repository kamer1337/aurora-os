/**
 * Aurora OS - Network Stack Implementation
 * 
 * Core networking functionality
 */

#include "network.h"
#include "../memory/memory.h"
#include <stddef.h>

/* ARP cache */
#define ARP_CACHE_SIZE 32
static struct {
    uint32_t ip_addr;
    uint8_t mac_addr[MAC_ADDR_LEN];
    int valid;
} arp_cache[ARP_CACHE_SIZE];

/* Socket table */
#define MAX_SOCKETS 64
static socket_t socket_table[MAX_SOCKETS];
static uint32_t next_socket_id = 1;

/* Network interface (simplified - single interface) */
static net_interface_t* default_interface = NULL;

/**
 * Initialize network subsystem
 */
void network_init(void) {
    arp_init();
    ip_init();
    icmp_init();
    udp_init();
    tcp_init();
    
    /* Initialize socket table */
    for (uint32_t i = 0; i < MAX_SOCKETS; i++) {
        socket_table[i].id = 0;
        socket_table[i].state = 0;
    }
}

/**
 * Initialize ARP subsystem
 */
void arp_init(void) {
    for (uint32_t i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].ip_addr = 0;
        arp_cache[i].valid = 0;
    }
}

/**
 * Lookup MAC address for IP in ARP cache
 */
uint8_t* arp_lookup(uint32_t ip_addr) {
    for (uint32_t i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip_addr == ip_addr) {
            return arp_cache[i].mac_addr;
        }
    }
    return NULL;
}

/**
 * Add entry to ARP cache
 */
static void arp_cache_add(uint32_t ip_addr, uint8_t* mac_addr) {
    /* Find free slot or oldest entry */
    int slot = -1;
    for (uint32_t i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        slot = 0; /* Replace first entry */
    }
    
    arp_cache[slot].ip_addr = ip_addr;
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        arp_cache[slot].mac_addr[i] = mac_addr[i];
    }
    arp_cache[slot].valid = 1;
}

/**
 * Send ARP request
 */
int arp_send_request(net_interface_t* iface, uint32_t target_ip) {
    if (!iface) {
        return -1;
    }
    
    arp_header_t arp;
    arp.hw_type = 0x0001;  /* Ethernet */
    arp.proto_type = 0x0800; /* IP */
    arp.hw_addr_len = MAC_ADDR_LEN;
    arp.proto_addr_len = IP_ADDR_LEN;
    arp.operation = 0x0001; /* Request */
    
    /* Set sender info */
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        arp.sender_mac[i] = iface->mac_addr[i];
    }
    arp.sender_ip = iface->ip_addr;
    
    /* Set target info */
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        arp.target_mac[i] = 0;
    }
    arp.target_ip = target_ip;
    
    /* Broadcast MAC */
    uint8_t broadcast[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    return eth_send_packet(iface, broadcast, ETH_TYPE_ARP, (uint8_t*)&arp, sizeof(arp));
}

/**
 * Receive ARP packet
 */
void arp_receive(net_interface_t* iface, arp_header_t* arp) {
    if (!iface || !arp) {
        return;
    }
    
    /* Add sender to cache */
    arp_cache_add(arp->sender_ip, arp->sender_mac);
    
    /* If this is a request for us, send reply */
    if (arp->operation == 0x0001 && arp->target_ip == iface->ip_addr) {
        arp_header_t reply;
        reply.hw_type = 0x0001;
        reply.proto_type = 0x0800;
        reply.hw_addr_len = MAC_ADDR_LEN;
        reply.proto_addr_len = IP_ADDR_LEN;
        reply.operation = 0x0002; /* Reply */
        
        for (int i = 0; i < MAC_ADDR_LEN; i++) {
            reply.sender_mac[i] = iface->mac_addr[i];
            reply.target_mac[i] = arp->sender_mac[i];
        }
        reply.sender_ip = iface->ip_addr;
        reply.target_ip = arp->sender_ip;
        
        eth_send_packet(iface, arp->sender_mac, ETH_TYPE_ARP, (uint8_t*)&reply, sizeof(reply));
    }
}

/**
 * Initialize IP subsystem
 */
void ip_init(void) {
    /* Nothing to initialize for now */
}

/**
 * Calculate IP checksum
 */
uint16_t ip_checksum(void* data, uint32_t length) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;
    
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/**
 * Send IP packet
 */
int ip_send_packet(net_interface_t* iface, uint32_t dest_ip, uint8_t protocol,
                   uint8_t* payload, uint32_t payload_len) {
    if (!iface || !payload) {
        return -1;
    }
    
    /* Lookup destination MAC */
    uint8_t* dest_mac = arp_lookup(dest_ip);
    if (!dest_mac) {
        /* Send ARP request and queue packet */
        arp_send_request(iface, dest_ip);
        return -1; /* Would retry later in real implementation */
    }
    
    /* Build IP header */
    ip_header_t ip;
    ip.version_ihl = 0x45; /* IPv4, 5 words */
    ip.tos = 0;
    ip.total_length = sizeof(ip_header_t) + payload_len;
    ip.id = 0;
    ip.flags_offset = 0;
    ip.ttl = 64;
    ip.protocol = protocol;
    ip.checksum = 0;
    ip.src_ip = iface->ip_addr;
    ip.dest_ip = dest_ip;
    
    /* Calculate checksum */
    ip.checksum = ip_checksum(&ip, sizeof(ip_header_t));
    
    /* Create packet with IP header + payload */
    uint8_t* packet = kmalloc(sizeof(ip_header_t) + payload_len);
    if (!packet) {
        return -1;
    }
    
    uint8_t* ptr = packet;
    for (uint32_t i = 0; i < sizeof(ip_header_t); i++) {
        ptr[i] = ((uint8_t*)&ip)[i];
    }
    for (uint32_t i = 0; i < payload_len; i++) {
        ptr[sizeof(ip_header_t) + i] = payload[i];
    }
    
    int result = eth_send_packet(iface, dest_mac, ETH_TYPE_IP, packet, 
                                  sizeof(ip_header_t) + payload_len);
    
    kfree(packet);
    return result;
}

/**
 * Receive IP packet
 */
void ip_receive_packet(net_interface_t* iface, ip_header_t* ip, uint32_t length) {
    if (!iface || !ip) {
        return;
    }
    
    /* Verify checksum */
    uint16_t orig_checksum = ip->checksum;
    ip->checksum = 0;
    uint16_t calc_checksum = ip_checksum(ip, sizeof(ip_header_t));
    if (orig_checksum != calc_checksum) {
        return; /* Checksum failed */
    }
    
    /* Get payload */
    uint8_t* payload = (uint8_t*)ip + sizeof(ip_header_t);
    uint32_t payload_len = length - sizeof(ip_header_t);
    
    /* Dispatch to protocol handler */
    switch (ip->protocol) {
        case PROTO_ICMP:
            icmp_receive(iface, (icmp_header_t*)payload, payload_len);
            break;
        case PROTO_UDP:
            udp_receive(iface, (udp_header_t*)payload, payload_len);
            break;
        case PROTO_TCP:
            tcp_receive(iface, (tcp_header_t*)payload, payload_len);
            break;
        default:
            break;
    }
}

/**
 * Send Ethernet packet
 */
int eth_send_packet(net_interface_t* iface, uint8_t* dest_mac, uint16_t type,
                    uint8_t* payload, uint32_t payload_len) {
    if (!iface || !dest_mac || !payload) {
        return -1;
    }
    
    /* Build Ethernet frame */
    uint8_t* frame = kmalloc(sizeof(eth_header_t) + payload_len);
    if (!frame) {
        return -1;
    }
    
    eth_header_t* eth = (eth_header_t*)frame;
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        eth->dest_mac[i] = dest_mac[i];
        eth->src_mac[i] = iface->mac_addr[i];
    }
    eth->type = type;
    
    /* Copy payload */
    uint8_t* ptr = frame + sizeof(eth_header_t);
    for (uint32_t i = 0; i < payload_len; i++) {
        ptr[i] = payload[i];
    }
    
    /* Send frame */
    int result = -1;
    if (iface->send) {
        result = iface->send(frame, sizeof(eth_header_t) + payload_len);
    }
    
    kfree(frame);
    return result;
}

/**
 * Receive Ethernet packet
 */
void eth_receive_packet(net_interface_t* iface, uint8_t* data, uint32_t length) {
    if (!iface || !data || length < sizeof(eth_header_t)) {
        return;
    }
    
    eth_header_t* eth = (eth_header_t*)data;
    uint8_t* payload = data + sizeof(eth_header_t);
    uint32_t payload_len = length - sizeof(eth_header_t);
    
    switch (eth->type) {
        case ETH_TYPE_ARP:
            arp_receive(iface, (arp_header_t*)payload);
            break;
        case ETH_TYPE_IP:
            ip_receive_packet(iface, (ip_header_t*)payload, payload_len);
            break;
        default:
            break;
    }
}

/**
 * Initialize ICMP
 */
void icmp_init(void) {
    /* Nothing to initialize */
}

/**
 * Send ICMP echo request
 */
int icmp_send_echo(net_interface_t* iface, uint32_t dest_ip, uint16_t id, uint16_t seq) {
    icmp_header_t icmp;
    icmp.type = 8; /* Echo request */
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.id = id;
    icmp.sequence = seq;
    
    icmp.checksum = ip_checksum(&icmp, sizeof(icmp_header_t));
    
    return ip_send_packet(iface, dest_ip, PROTO_ICMP, (uint8_t*)&icmp, sizeof(icmp));
}

/**
 * Receive ICMP packet
 */
void icmp_receive(net_interface_t* iface, icmp_header_t* icmp, uint32_t length) {
    (void)iface;
    (void)length;
    
    if (!icmp) {
        return;
    }
    
    /* Handle echo request - send reply */
    if (icmp->type == 8) {
        icmp->type = 0; /* Echo reply */
        icmp->checksum = 0;
        icmp->checksum = ip_checksum(icmp, sizeof(icmp_header_t));
        /* Would send reply here */
    }
}

/**
 * Initialize UDP
 */
void udp_init(void) {
    /* Nothing to initialize */
}

/**
 * Send UDP packet
 */
int udp_send(socket_t* sock, uint8_t* data, uint32_t length) {
    if (!sock || !data || !default_interface) {
        return -1;
    }
    
    udp_header_t udp;
    udp.src_port = sock->local_port;
    udp.dest_port = sock->remote_port;
    udp.length = sizeof(udp_header_t) + length;
    udp.checksum = 0; /* Optional for IPv4 */
    
    /* Create UDP packet */
    uint8_t* packet = kmalloc(sizeof(udp_header_t) + length);
    if (!packet) {
        return -1;
    }
    
    uint8_t* ptr = packet;
    for (uint32_t i = 0; i < sizeof(udp_header_t); i++) {
        ptr[i] = ((uint8_t*)&udp)[i];
    }
    for (uint32_t i = 0; i < length; i++) {
        ptr[sizeof(udp_header_t) + i] = data[i];
    }
    
    int result = ip_send_packet(default_interface, sock->remote_ip, PROTO_UDP,
                                 packet, sizeof(udp_header_t) + length);
    
    kfree(packet);
    return result;
}

/**
 * Receive UDP packet
 */
void udp_receive(net_interface_t* iface, udp_header_t* udp, uint32_t length) {
    (void)iface;
    (void)udp;
    (void)length;
    
    /* Would deliver to socket here */
}

/**
 * Initialize TCP
 */
void tcp_init(void) {
    /* Nothing to initialize */
}

/**
 * TCP connect
 */
int tcp_connect(socket_t* sock) {
    (void)sock;
    /* Would implement TCP handshake here */
    return -1;
}

/**
 * Send TCP packet
 */
int tcp_send(socket_t* sock, uint8_t* data, uint32_t length) {
    (void)sock;
    (void)data;
    (void)length;
    /* Would implement TCP send here */
    return -1;
}

/**
 * Receive TCP packet
 */
void tcp_receive(net_interface_t* iface, tcp_header_t* tcp, uint32_t length) {
    (void)iface;
    (void)tcp;
    (void)length;
    /* Would handle TCP state machine here */
}

/**
 * Create socket
 */
socket_t* socket_create(uint8_t protocol) {
    /* Find free socket */
    for (uint32_t i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].id == 0) {
            socket_table[i].id = next_socket_id++;
            socket_table[i].protocol = protocol;
            socket_table[i].local_port = 0;
            socket_table[i].remote_port = 0;
            socket_table[i].local_ip = 0;
            socket_table[i].remote_ip = 0;
            socket_table[i].state = 0;
            return &socket_table[i];
        }
    }
    return NULL;
}

/**
 * Bind socket to port
 */
int socket_bind(socket_t* sock, uint16_t port) {
    if (!sock) {
        return -1;
    }
    sock->local_port = port;
    return 0;
}

/**
 * Connect socket
 */
int socket_connect(socket_t* sock, uint32_t remote_ip, uint16_t remote_port) {
    if (!sock) {
        return -1;
    }
    
    sock->remote_ip = remote_ip;
    sock->remote_port = remote_port;
    
    if (sock->protocol == PROTO_TCP) {
        return tcp_connect(sock);
    }
    
    return 0;
}

/**
 * Send data on socket
 */
int socket_send(socket_t* sock, uint8_t* data, uint32_t length) {
    if (!sock || !data) {
        return -1;
    }
    
    if (sock->protocol == PROTO_UDP) {
        return udp_send(sock, data, length);
    } else if (sock->protocol == PROTO_TCP) {
        return tcp_send(sock, data, length);
    }
    
    return -1;
}

/**
 * Receive data from socket
 */
int socket_receive(socket_t* sock, uint8_t* data, uint32_t max_length) {
    (void)sock;
    (void)data;
    (void)max_length;
    /* Would implement socket receive queue here */
    return -1;
}

/**
 * Close socket
 */
void socket_close(socket_t* sock) {
    if (!sock) {
        return;
    }
    
    sock->id = 0;
    sock->state = 0;
}
