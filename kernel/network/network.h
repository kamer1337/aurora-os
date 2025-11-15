/**
 * Aurora OS - Network Stack Header
 * 
 * Core networking functionality
 */

#ifndef AURORA_NETWORK_H
#define AURORA_NETWORK_H

#include <stdint.h>

/* Network protocols */
#define PROTO_ICMP  1
#define PROTO_TCP   6
#define PROTO_UDP   17

/* Ethernet frame types */
#define ETH_TYPE_IP   0x0800
#define ETH_TYPE_ARP  0x0806

/* MAC address length */
#define MAC_ADDR_LEN 6

/* IP address length */
#define IP_ADDR_LEN 4

/* Maximum packet size */
#define MAX_PACKET_SIZE 1518

/* Ethernet header */
typedef struct {
    uint8_t dest_mac[MAC_ADDR_LEN];
    uint8_t src_mac[MAC_ADDR_LEN];
    uint16_t type;
} __attribute__((packed)) eth_header_t;

/* IP header */
typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

/* ARP header */
typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint16_t operation;
    uint8_t sender_mac[MAC_ADDR_LEN];
    uint32_t sender_ip;
    uint8_t target_mac[MAC_ADDR_LEN];
    uint32_t target_ip;
} __attribute__((packed)) arp_header_t;

/* ICMP header */
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

/* UDP header */
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

/* TCP header */
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t offset_reserved;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

/* Network packet buffer */
typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    uint32_t length;
    uint32_t offset;
} net_packet_t;

/* Socket structure */
typedef struct {
    uint32_t id;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t local_ip;
    uint32_t remote_ip;
    uint8_t protocol;
    uint8_t state;
} socket_t;

/* Network interface */
typedef struct {
    uint8_t mac_addr[MAC_ADDR_LEN];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    int (*send)(uint8_t* data, uint32_t length);
    void (*receive)(uint8_t* data, uint32_t length);
} net_interface_t;

/* Network subsystem initialization */
void network_init(void);

/* Ethernet functions */
int eth_send_packet(net_interface_t* iface, uint8_t* dest_mac, uint16_t type, 
                    uint8_t* payload, uint32_t payload_len);
void eth_receive_packet(net_interface_t* iface, uint8_t* data, uint32_t length);

/* ARP functions */
void arp_init(void);
int arp_send_request(net_interface_t* iface, uint32_t target_ip);
void arp_receive(net_interface_t* iface, arp_header_t* arp);
uint8_t* arp_lookup(uint32_t ip_addr);

/* IP functions */
void ip_init(void);
int ip_send_packet(net_interface_t* iface, uint32_t dest_ip, uint8_t protocol,
                   uint8_t* payload, uint32_t payload_len);
void ip_receive_packet(net_interface_t* iface, ip_header_t* ip, uint32_t length);
uint16_t ip_checksum(void* data, uint32_t length);

/* ICMP functions */
void icmp_init(void);
int icmp_send_echo(net_interface_t* iface, uint32_t dest_ip, uint16_t id, uint16_t seq);
void icmp_receive(net_interface_t* iface, icmp_header_t* icmp, uint32_t length);

/* UDP functions */
void udp_init(void);
int udp_send(socket_t* sock, uint8_t* data, uint32_t length);
void udp_receive(net_interface_t* iface, udp_header_t* udp, uint32_t length);

/* TCP functions */
void tcp_init(void);
int tcp_connect(socket_t* sock);
int tcp_send(socket_t* sock, uint8_t* data, uint32_t length);
void tcp_receive(net_interface_t* iface, tcp_header_t* tcp, uint32_t length);

/* Socket interface */
socket_t* socket_create(uint8_t protocol);
int socket_bind(socket_t* sock, uint16_t port);
int socket_connect(socket_t* sock, uint32_t remote_ip, uint16_t remote_port);
int socket_send(socket_t* sock, uint8_t* data, uint32_t length);
int socket_receive(socket_t* sock, uint8_t* data, uint32_t max_length);
void socket_close(socket_t* sock);

#endif /* AURORA_NETWORK_H */
