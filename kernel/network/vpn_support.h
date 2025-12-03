/**
 * Aurora OS - VPN Support Header
 * 
 * Provides Virtual Private Network functionality including
 * IPsec, OpenVPN, WireGuard, and L2TP/PPTP protocols
 */

#ifndef VPN_SUPPORT_H
#define VPN_SUPPORT_H

#include <stdint.h>

/* VPN protocol types */
typedef enum {
    VPN_PROTOCOL_IPSEC = 0,      /* IPsec (ESP/AH) */
    VPN_PROTOCOL_OPENVPN = 1,    /* OpenVPN (SSL/TLS) */
    VPN_PROTOCOL_WIREGUARD = 2,  /* WireGuard (modern, fast) */
    VPN_PROTOCOL_L2TP = 3,       /* L2TP/IPsec */
    VPN_PROTOCOL_PPTP = 4,       /* PPTP (legacy) */
    VPN_PROTOCOL_SSTP = 5,       /* SSTP (Microsoft) */
    VPN_PROTOCOL_IKEV2 = 6       /* IKEv2/IPsec */
} vpn_protocol_t;

/* VPN connection status */
typedef enum {
    VPN_STATUS_DISCONNECTED = 0,
    VPN_STATUS_CONNECTING = 1,
    VPN_STATUS_CONNECTED = 2,
    VPN_STATUS_DISCONNECTING = 3,
    VPN_STATUS_ERROR = 4,
    VPN_STATUS_RECONNECTING = 5
} vpn_status_t;

/* IPsec encryption algorithms */
typedef enum {
    IPSEC_ENCRYPT_DES = 0,
    IPSEC_ENCRYPT_3DES = 1,
    IPSEC_ENCRYPT_AES128 = 2,
    IPSEC_ENCRYPT_AES192 = 3,
    IPSEC_ENCRYPT_AES256 = 4,
    IPSEC_ENCRYPT_CHACHA20 = 5
} ipsec_encrypt_t;

/* IPsec authentication algorithms */
typedef enum {
    IPSEC_AUTH_MD5 = 0,
    IPSEC_AUTH_SHA1 = 1,
    IPSEC_AUTH_SHA256 = 2,
    IPSEC_AUTH_SHA384 = 3,
    IPSEC_AUTH_SHA512 = 4
} ipsec_auth_t;

/* VPN configuration structure */
typedef struct {
    vpn_protocol_t protocol;
    char server[256];            /* VPN server hostname or IP */
    uint16_t port;               /* Server port */
    char username[128];          /* Authentication username */
    char password[128];          /* Authentication password */
    char psk[256];               /* Pre-shared key (for IPsec) */
    char certificate_path[512];  /* Path to client certificate */
    char ca_cert_path[512];      /* Path to CA certificate */
    char private_key_path[512];  /* Path to private key */
    int use_compression;         /* Enable compression */
    int split_tunnel;            /* Split tunneling mode */
    int mtu;                     /* MTU size */
    uint32_t keepalive_interval; /* Keepalive interval (seconds) */
} vpn_config_t;

/* IPsec configuration */
typedef struct {
    ipsec_encrypt_t encryption;
    ipsec_auth_t authentication;
    int use_esp;                 /* Use ESP (Encapsulating Security Payload) */
    int use_ah;                  /* Use AH (Authentication Header) */
    uint32_t lifetime_seconds;   /* SA lifetime */
    int pfs_group;               /* Perfect Forward Secrecy DH group */
    int nat_traversal;           /* NAT-T support */
} ipsec_config_t;

/* WireGuard configuration */
typedef struct {
    uint8_t private_key[32];     /* Curve25519 private key */
    uint8_t public_key[32];      /* Curve25519 public key */
    uint8_t peer_public_key[32]; /* Peer's public key */
    uint8_t preshared_key[32];   /* Optional pre-shared key */
    char endpoint[256];          /* Peer endpoint */
    uint16_t listen_port;        /* Local listen port */
    uint32_t keepalive;          /* Persistent keepalive */
} wireguard_config_t;

/* VPN connection structure */
typedef struct {
    uint32_t id;                 /* Connection ID */
    vpn_config_t config;         /* Configuration */
    vpn_status_t status;         /* Current status */
    char interface_name[16];     /* Virtual interface name (tun0, wg0, etc.) */
    uint32_t tun_fd;            /* TUN/TAP device file descriptor */
    uint8_t local_ip[4];        /* Assigned local IP */
    uint8_t remote_ip[4];       /* Remote/peer IP */
    uint8_t gateway[4];         /* VPN gateway IP */
    uint64_t bytes_sent;        /* Statistics */
    uint64_t bytes_received;
    uint64_t packets_sent;
    uint64_t packets_received;
    uint32_t last_handshake;    /* Last handshake timestamp */
    int dns_pushed;             /* DNS servers pushed */
    uint8_t dns_servers[4][4];  /* Up to 4 DNS servers */
} vpn_connection_t;

/* VPN tunnel statistics */
typedef struct {
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t total_packets_sent;
    uint64_t total_packets_received;
    uint32_t active_connections;
    uint32_t total_connections;
    uint32_t failed_connections;
    uint32_t dropped_packets;
} vpn_stats_t;

/* Core VPN functions */
int vpn_init(void);
int vpn_shutdown(void);

/* Connection management */
int vpn_connect(const vpn_config_t* config, vpn_connection_t** conn);
int vpn_disconnect(vpn_connection_t* conn);
int vpn_reconnect(vpn_connection_t* conn);
vpn_status_t vpn_get_status(vpn_connection_t* conn);

/* Protocol-specific initialization */
int vpn_ipsec_init(vpn_connection_t* conn, const ipsec_config_t* config);
int vpn_openvpn_init(vpn_connection_t* conn);
int vpn_wireguard_init(vpn_connection_t* conn, const wireguard_config_t* config);
int vpn_l2tp_init(vpn_connection_t* conn);

/* TUN/TAP interface management */
int vpn_create_tun_interface(const char* name, int* fd);
int vpn_destroy_tun_interface(int fd);
int vpn_configure_interface(int fd, const uint8_t* ip, const uint8_t* netmask);

/* Routing */
int vpn_add_route(const uint8_t* network, const uint8_t* netmask, const uint8_t* gateway);
int vpn_delete_route(const uint8_t* network, const uint8_t* netmask);
int vpn_set_default_route(vpn_connection_t* conn);
int vpn_restore_default_route(void);

/* Split tunneling */
int vpn_enable_split_tunnel(vpn_connection_t* conn);
int vpn_add_split_tunnel_route(vpn_connection_t* conn, const uint8_t* network, const uint8_t* netmask);

/* DNS management */
int vpn_push_dns_servers(vpn_connection_t* conn, const uint8_t dns[][4], uint32_t count);
int vpn_restore_dns_servers(vpn_connection_t* conn);

/* Key management */
int vpn_generate_keys(vpn_protocol_t protocol, uint8_t* private_key, uint8_t* public_key);
int vpn_load_certificate(const char* path, uint8_t** cert, uint32_t* cert_len);

/* Statistics */
int vpn_get_connection_stats(vpn_connection_t* conn, vpn_stats_t* stats);
int vpn_reset_stats(vpn_connection_t* conn);

/* Configuration helpers */
int vpn_load_config_file(const char* path, vpn_config_t* config);
int vpn_save_config_file(const char* path, const vpn_config_t* config);

#endif /* VPN_SUPPORT_H */
