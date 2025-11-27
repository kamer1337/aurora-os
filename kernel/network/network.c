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

/* Socket receive buffer */
#define SOCKET_RECV_BUFFER_SIZE 8192
typedef struct {
    uint8_t data[SOCKET_RECV_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} socket_recv_buffer_t;

static socket_recv_buffer_t socket_recv_buffers[MAX_SOCKETS];

/* Last received packet source IP - used for reply routing */
static uint32_t last_source_ip = 0;

/* TCP state definitions */
#define TCP_STATE_CLOSED      0
#define TCP_STATE_LISTEN      1
#define TCP_STATE_SYN_SENT    2
#define TCP_STATE_SYN_RCVD    3
#define TCP_STATE_ESTABLISHED 4
#define TCP_STATE_FIN_WAIT_1  5
#define TCP_STATE_FIN_WAIT_2  6
#define TCP_STATE_CLOSE_WAIT  7
#define TCP_STATE_CLOSING     8
#define TCP_STATE_LAST_ACK    9
#define TCP_STATE_TIME_WAIT   10

/* TCP flags */
#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20

/* TCP connection tracking */
typedef struct tcp_connection {
    socket_t* sock;
    uint32_t local_seq;
    uint32_t remote_seq;
    uint32_t ack_num;
    uint8_t state;
} tcp_connection_t;

#define MAX_TCP_CONNECTIONS 64
static tcp_connection_t tcp_connections[MAX_TCP_CONNECTIONS];

/* Helper to find TCP connection by socket */
static tcp_connection_t* find_tcp_connection(socket_t* sock) {
    if (!sock) return NULL;
    for (uint32_t i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].sock == sock) {
            return &tcp_connections[i];
        }
    }
    return NULL;
}

/* Helper to allocate TCP connection */
static tcp_connection_t* alloc_tcp_connection(socket_t* sock) {
    for (uint32_t i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].sock == NULL) {
            tcp_connections[i].sock = sock;
            tcp_connections[i].state = TCP_STATE_CLOSED;
            return &tcp_connections[i];
        }
    }
    return NULL;
}

/* Helper to free TCP connection */
static void free_tcp_connection(tcp_connection_t* conn) {
    if (conn) {
        conn->sock = NULL;
        conn->state = TCP_STATE_CLOSED;
        conn->local_seq = 0;
        conn->remote_seq = 0;
        conn->ack_num = 0;
    }
}

/* Simple pseudo-random number generator for initial sequence numbers */
static uint32_t tcp_random_seq(void) {
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return seed;
}

/* Add data to socket receive buffer */
static int socket_buffer_write(uint32_t socket_idx, uint8_t* data, uint32_t length) {
    if (socket_idx >= MAX_SOCKETS) return -1;
    socket_recv_buffer_t* buf = &socket_recv_buffers[socket_idx];
    
    for (uint32_t i = 0; i < length; i++) {
        if (buf->count >= SOCKET_RECV_BUFFER_SIZE) {
            return (int)i; /* Buffer full, return bytes written */
        }
        buf->data[buf->tail] = data[i];
        buf->tail = (buf->tail + 1) % SOCKET_RECV_BUFFER_SIZE;
        buf->count++;
    }
    return (int)length;
}

/* Read data from socket receive buffer */
static int socket_buffer_read(uint32_t socket_idx, uint8_t* data, uint32_t max_length) {
    if (socket_idx >= MAX_SOCKETS) return -1;
    socket_recv_buffer_t* buf = &socket_recv_buffers[socket_idx];
    
    uint32_t bytes_read = 0;
    while (bytes_read < max_length && buf->count > 0) {
        data[bytes_read++] = buf->data[buf->head];
        buf->head = (buf->head + 1) % SOCKET_RECV_BUFFER_SIZE;
        buf->count--;
    }
    return (int)bytes_read;
}

/* Get socket index in table */
static int get_socket_index(socket_t* sock) {
    if (!sock) return -1;
    for (uint32_t i = 0; i < MAX_SOCKETS; i++) {
        if (&socket_table[i] == sock) {
            return (int)i;
        }
    }
    return -1;
}

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
    
    /* Initialize socket receive buffers */
    for (uint32_t i = 0; i < MAX_SOCKETS; i++) {
        socket_recv_buffers[i].head = 0;
        socket_recv_buffers[i].tail = 0;
        socket_recv_buffers[i].count = 0;
    }
    
    /* Initialize TCP connections */
    for (uint32_t i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        tcp_connections[i].sock = NULL;
        tcp_connections[i].state = TCP_STATE_CLOSED;
        tcp_connections[i].local_seq = 0;
        tcp_connections[i].remote_seq = 0;
        tcp_connections[i].ack_num = 0;
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
    
    /* Save source IP for reply routing */
    last_source_ip = ip->src_ip;
    
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
    if (!iface || !icmp || length < sizeof(icmp_header_t)) {
        return;
    }
    
    /* Verify checksum */
    uint16_t orig_checksum = icmp->checksum;
    icmp->checksum = 0;
    uint16_t calc_checksum = ip_checksum(icmp, length);
    if (orig_checksum != calc_checksum) {
        return; /* Checksum failed */
    }
    
    /* Handle ICMP message types */
    switch (icmp->type) {
        case 8:  /* Echo request - send reply */
            {
                /* Build echo reply */
                icmp_header_t reply;
                reply.type = 0; /* Echo reply */
                reply.code = 0;
                reply.checksum = 0;
                reply.id = icmp->id;
                reply.sequence = icmp->sequence;
                
                /* Calculate checksum */
                reply.checksum = ip_checksum(&reply, sizeof(icmp_header_t));
                
                /* Send reply to the source IP (saved from IP header processing) */
                if (last_source_ip != 0) {
                    ip_send_packet(iface, last_source_ip, PROTO_ICMP, 
                                   (uint8_t*)&reply, sizeof(icmp_header_t));
                }
            }
            break;
        case 0:  /* Echo reply - notify waiting process */
            /* In a full implementation, would signal waiting ping process */
            break;
        case 3:  /* Destination unreachable */
            /* Handle error notification */
            break;
        case 11: /* Time exceeded */
            /* Handle TTL exceeded notification */
            break;
        default:
            break;
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
    if (!iface || !udp || length < sizeof(udp_header_t)) {
        return;
    }
    
    /* Find socket bound to this port */
    socket_t* sock = NULL;
    int sock_idx = -1;
    for (uint32_t i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].id != 0 && 
            socket_table[i].protocol == PROTO_UDP &&
            socket_table[i].local_port == udp->dest_port) {
            sock = &socket_table[i];
            sock_idx = (int)i;
            break;
        }
    }
    
    if (!sock || sock_idx < 0) {
        return; /* No socket listening on this port */
    }
    
    /* Extract payload data */
    uint8_t* payload = (uint8_t*)udp + sizeof(udp_header_t);
    uint32_t payload_len = length - sizeof(udp_header_t);
    
    if (payload_len > 0) {
        /* Store data in socket receive buffer */
        socket_buffer_write((uint32_t)sock_idx, payload, payload_len);
    }
}

/**
 * Initialize TCP
 */
void tcp_init(void) {
    /* Initialize TCP connections */
    for (uint32_t i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        tcp_connections[i].sock = NULL;
        tcp_connections[i].state = TCP_STATE_CLOSED;
        tcp_connections[i].local_seq = 0;
        tcp_connections[i].remote_seq = 0;
        tcp_connections[i].ack_num = 0;
    }
}

/**
 * TCP pseudo-header for checksum calculation (RFC 793)
 */
typedef struct {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
} __attribute__((packed)) tcp_pseudo_header_t;

/**
 * Calculate TCP checksum including pseudo-header
 */
static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dest_ip, 
                             uint8_t* tcp_segment, uint32_t tcp_length) {
    uint32_t sum = 0;
    
    /* Add pseudo-header */
    tcp_pseudo_header_t pseudo;
    pseudo.src_ip = src_ip;
    pseudo.dest_ip = dest_ip;
    pseudo.reserved = 0;
    pseudo.protocol = PROTO_TCP;
    pseudo.tcp_length = ((tcp_length >> 8) & 0xFF) | ((tcp_length << 8) & 0xFF00); /* Network byte order */
    
    uint16_t* ptr = (uint16_t*)&pseudo;
    for (uint32_t i = 0; i < sizeof(tcp_pseudo_header_t) / 2; i++) {
        sum += ptr[i];
    }
    
    /* Add TCP segment */
    ptr = (uint16_t*)tcp_segment;
    uint32_t remaining = tcp_length;
    while (remaining > 1) {
        sum += *ptr++;
        remaining -= 2;
    }
    
    /* Add remaining byte if odd length */
    if (remaining == 1) {
        sum += *(uint8_t*)ptr;
    }
    
    /* Fold 32-bit sum to 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~((uint16_t)sum);
}

/**
 * Send TCP segment
 */
static int tcp_send_segment(net_interface_t* iface, socket_t* sock, uint8_t flags,
                            uint32_t seq, uint32_t ack, uint8_t* data, uint32_t data_len) {
    if (!iface || !sock) {
        return -1;
    }
    
    /* Calculate total packet size */
    uint32_t packet_len = sizeof(tcp_header_t) + data_len;
    uint8_t* packet = kmalloc(packet_len);
    if (!packet) {
        return -1;
    }
    
    /* Build TCP header */
    tcp_header_t* tcp = (tcp_header_t*)packet;
    tcp->src_port = sock->local_port;
    tcp->dest_port = sock->remote_port;
    tcp->seq_num = seq;
    tcp->ack_num = ack;
    tcp->offset_reserved = (5 << 4); /* 5 words = 20 bytes (no options) */
    tcp->flags = flags;
    tcp->window = 8192; /* Receive window size */
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;
    
    /* Copy payload data if any */
    if (data && data_len > 0) {
        uint8_t* payload = packet + sizeof(tcp_header_t);
        for (uint32_t i = 0; i < data_len; i++) {
            payload[i] = data[i];
        }
    }
    
    /* Calculate TCP checksum with pseudo-header */
    tcp->checksum = tcp_checksum(iface->ip_addr, sock->remote_ip, packet, packet_len);
    
    /* Send via IP layer */
    int result = ip_send_packet(iface, sock->remote_ip, PROTO_TCP, packet, packet_len);
    
    kfree(packet);
    return result;
}

/**
 * TCP connect - initiate 3-way handshake
 * Note: This is a non-blocking implementation that starts the handshake.
 * The connection state is updated when SYN-ACK is received in tcp_receive.
 */
int tcp_connect(socket_t* sock) {
    if (!sock || !default_interface) {
        return -1;
    }
    
    /* Allocate TCP connection tracking */
    tcp_connection_t* conn = find_tcp_connection(sock);
    if (!conn) {
        conn = alloc_tcp_connection(sock);
        if (!conn) {
            return -1; /* No free connection slots */
        }
    }
    
    /* Generate initial sequence number */
    conn->local_seq = tcp_random_seq();
    conn->state = TCP_STATE_SYN_SENT;
    
    /* Send SYN */
    int result = tcp_send_segment(default_interface, sock, TCP_FLAG_SYN,
                                   conn->local_seq, 0, NULL, 0);
    
    if (result < 0) {
        conn->state = TCP_STATE_CLOSED;
        return -1;
    }
    
    /* Increment sequence number for SYN */
    conn->local_seq++;
    
    /* Connection is now in SYN_SENT state, waiting for SYN-ACK */
    /* The state will be updated to ESTABLISHED when tcp_receive gets SYN-ACK */
    /* For now, return success indicating handshake has started */
    return 0;
}

/**
 * Send TCP packet with data
 */
int tcp_send(socket_t* sock, uint8_t* data, uint32_t length) {
    if (!sock || !data || length == 0 || !default_interface) {
        return -1;
    }
    
    /* Find TCP connection */
    tcp_connection_t* conn = find_tcp_connection(sock);
    if (!conn || conn->state != TCP_STATE_ESTABLISHED) {
        return -1; /* Not connected */
    }
    
    /* Send data with ACK flag */
    int result = tcp_send_segment(default_interface, sock, TCP_FLAG_ACK | TCP_FLAG_PSH,
                                   conn->local_seq, conn->ack_num, data, length);
    
    if (result >= 0) {
        conn->local_seq += length;
    }
    
    return result;
}

/**
 * Receive TCP packet - handle TCP state machine
 */
void tcp_receive(net_interface_t* iface, tcp_header_t* tcp, uint32_t length) {
    if (!iface || !tcp || length < sizeof(tcp_header_t)) {
        return;
    }
    
    /* Find socket by port */
    socket_t* sock = NULL;
    for (uint32_t i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].id != 0 && 
            socket_table[i].protocol == PROTO_TCP &&
            socket_table[i].local_port == tcp->dest_port) {
            sock = &socket_table[i];
            break;
        }
    }
    
    if (!sock) {
        return; /* No socket for this port */
    }
    
    /* Find TCP connection */
    tcp_connection_t* conn = find_tcp_connection(sock);
    if (!conn) {
        /* Handle new incoming connection (SYN) */
        if (tcp->flags & TCP_FLAG_SYN) {
            conn = alloc_tcp_connection(sock);
            if (!conn) {
                return; /* No free slots */
            }
            conn->remote_seq = tcp->seq_num + 1;
            conn->local_seq = tcp_random_seq();
            conn->state = TCP_STATE_SYN_RCVD;
            
            /* Send SYN-ACK */
            tcp_send_segment(iface, sock, TCP_FLAG_SYN | TCP_FLAG_ACK,
                            conn->local_seq, conn->remote_seq, NULL, 0);
            conn->local_seq++;
        }
        return;
    }
    
    /* Handle TCP state machine */
    switch (conn->state) {
        case TCP_STATE_SYN_SENT:
            /* Waiting for SYN-ACK */
            if ((tcp->flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
                conn->remote_seq = tcp->seq_num + 1;
                conn->ack_num = conn->remote_seq;
                conn->state = TCP_STATE_ESTABLISHED;
                sock->state = TCP_STATE_ESTABLISHED;
                
                /* Send ACK */
                tcp_send_segment(iface, sock, TCP_FLAG_ACK,
                                conn->local_seq, conn->ack_num, NULL, 0);
            }
            break;
            
        case TCP_STATE_SYN_RCVD:
            /* Waiting for ACK to complete handshake */
            if (tcp->flags & TCP_FLAG_ACK) {
                conn->state = TCP_STATE_ESTABLISHED;
                sock->state = TCP_STATE_ESTABLISHED;
            }
            break;
            
        case TCP_STATE_ESTABLISHED:
            /* Handle data and connection close */
            if (tcp->flags & TCP_FLAG_FIN) {
                /* Remote side wants to close */
                conn->ack_num = tcp->seq_num + 1;
                conn->state = TCP_STATE_CLOSE_WAIT;
                
                /* Send ACK */
                tcp_send_segment(iface, sock, TCP_FLAG_ACK,
                                conn->local_seq, conn->ack_num, NULL, 0);
            } else if (tcp->flags & TCP_FLAG_ACK) {
                /* Data segment */
                uint8_t header_len = (tcp->offset_reserved >> 4) * 4;
                uint32_t data_len = length - header_len;
                
                if (data_len > 0) {
                    /* Extract payload and store in receive buffer */
                    uint8_t* payload = (uint8_t*)tcp + header_len;
                    int sock_idx = get_socket_index(sock);
                    if (sock_idx >= 0) {
                        socket_buffer_write((uint32_t)sock_idx, payload, data_len);
                    }
                    
                    /* Update sequence tracking and send ACK */
                    conn->ack_num = tcp->seq_num + data_len;
                    tcp_send_segment(iface, sock, TCP_FLAG_ACK,
                                    conn->local_seq, conn->ack_num, NULL, 0);
                }
            }
            break;
            
        case TCP_STATE_FIN_WAIT_1:
            if (tcp->flags & TCP_FLAG_ACK) {
                conn->state = TCP_STATE_FIN_WAIT_2;
            }
            if (tcp->flags & TCP_FLAG_FIN) {
                conn->ack_num = tcp->seq_num + 1;
                tcp_send_segment(iface, sock, TCP_FLAG_ACK,
                                conn->local_seq, conn->ack_num, NULL, 0);
                conn->state = TCP_STATE_TIME_WAIT;
            }
            break;
            
        case TCP_STATE_FIN_WAIT_2:
            if (tcp->flags & TCP_FLAG_FIN) {
                conn->ack_num = tcp->seq_num + 1;
                tcp_send_segment(iface, sock, TCP_FLAG_ACK,
                                conn->local_seq, conn->ack_num, NULL, 0);
                conn->state = TCP_STATE_TIME_WAIT;
            }
            break;
            
        case TCP_STATE_CLOSE_WAIT:
            /* Application should close the connection */
            break;
            
        case TCP_STATE_LAST_ACK:
            if (tcp->flags & TCP_FLAG_ACK) {
                conn->state = TCP_STATE_CLOSED;
                free_tcp_connection(conn);
            }
            break;
            
        case TCP_STATE_TIME_WAIT:
            /* Wait and then close */
            conn->state = TCP_STATE_CLOSED;
            free_tcp_connection(conn);
            break;
            
        default:
            break;
    }
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
    if (!sock || !data || max_length == 0) {
        return -1;
    }
    
    /* Get socket index */
    int sock_idx = get_socket_index(sock);
    if (sock_idx < 0) {
        return -1;
    }
    
    /* Read from receive buffer */
    return socket_buffer_read((uint32_t)sock_idx, data, max_length);
}

/**
 * Close socket
 */
void socket_close(socket_t* sock) {
    if (!sock) {
        return;
    }
    
    /* For TCP, send FIN and clean up connection */
    if (sock->protocol == PROTO_TCP) {
        tcp_connection_t* conn = find_tcp_connection(sock);
        if (conn && conn->state == TCP_STATE_ESTABLISHED) {
            /* Send FIN */
            if (default_interface) {
                tcp_send_segment(default_interface, sock, TCP_FLAG_FIN | TCP_FLAG_ACK,
                                conn->local_seq, conn->ack_num, NULL, 0);
                conn->state = TCP_STATE_FIN_WAIT_1;
            }
        }
        /* Free TCP connection tracking */
        if (conn) {
            free_tcp_connection(conn);
        }
    }
    
    /* Clear socket receive buffer */
    int sock_idx = get_socket_index(sock);
    if (sock_idx >= 0) {
        socket_recv_buffers[sock_idx].head = 0;
        socket_recv_buffers[sock_idx].tail = 0;
        socket_recv_buffers[sock_idx].count = 0;
    }
    
    /* Mark socket as free */
    sock->id = 0;
    sock->state = 0;
}
