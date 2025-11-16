/**
 * Aurora OS - WiFi Driver Header
 * 
 * 802.11 wireless networking driver
 */

#ifndef AURORA_WIFI_H
#define AURORA_WIFI_H

#include <stdint.h>

/* WiFi standards */
#define WIFI_STANDARD_80211A   0x01  /* 5 GHz, up to 54 Mbps */
#define WIFI_STANDARD_80211B   0x02  /* 2.4 GHz, up to 11 Mbps */
#define WIFI_STANDARD_80211G   0x04  /* 2.4 GHz, up to 54 Mbps */
#define WIFI_STANDARD_80211N   0x08  /* 2.4/5 GHz, up to 600 Mbps */
#define WIFI_STANDARD_80211AC  0x10  /* 5 GHz, up to 6.77 Gbps */
#define WIFI_STANDARD_80211AX  0x20  /* 2.4/5/6 GHz, WiFi 6 */

/* WiFi security modes */
#define WIFI_SECURITY_NONE     0
#define WIFI_SECURITY_WEP      1
#define WIFI_SECURITY_WPA      2
#define WIFI_SECURITY_WPA2     3
#define WIFI_SECURITY_WPA3     4

/* WiFi connection states */
typedef enum {
    WIFI_STATE_DISABLED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_SCANNING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_ERROR
} wifi_state_t;

/* WiFi network structure */
typedef struct {
    uint8_t ssid[32];
    uint8_t ssid_len;
    uint8_t bssid[6];
    uint8_t channel;
    int8_t rssi;           /* Received Signal Strength Indicator */
    uint8_t security_mode;
    uint16_t capabilities;
} wifi_network_t;

/* WiFi configuration */
typedef struct {
    uint8_t ssid[32];
    uint8_t ssid_len;
    uint8_t password[64];
    uint8_t password_len;
    uint8_t security_mode;
    uint8_t auto_connect;
} wifi_config_t;

/* WiFi statistics */
typedef struct {
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t errors;
    uint32_t retries;
} wifi_stats_t;

/* WiFi device structure */
typedef struct {
    uint8_t mac_addr[6];
    uint8_t standards_supported;
    wifi_state_t state;
    wifi_config_t config;
    wifi_stats_t stats;
    int8_t current_rssi;
    uint8_t current_channel;
} wifi_device_t;

/* WiFi driver initialization */
void wifi_init(void);

/* WiFi control functions */
int wifi_enable(void);
int wifi_disable(void);
wifi_state_t wifi_get_state(void);

/* Network scanning */
int wifi_scan_networks(wifi_network_t* networks, uint32_t max_networks, uint32_t* count);
int wifi_get_network_info(const uint8_t* ssid, uint8_t ssid_len, wifi_network_t* info);

/* Connection management */
int wifi_connect(const uint8_t* ssid, uint8_t ssid_len, const uint8_t* password, uint8_t password_len, uint8_t security_mode);
int wifi_disconnect(void);
int wifi_reconnect(void);

/* Configuration */
int wifi_set_config(wifi_config_t* config);
int wifi_get_config(wifi_config_t* config);
int wifi_get_mac_address(uint8_t* mac_addr);

/* Statistics */
int wifi_get_stats(wifi_stats_t* stats);
int wifi_get_signal_strength(int8_t* rssi);

/* Power management */
int wifi_set_power_save(int enabled);
int wifi_get_power_save(int* enabled);

/* Advanced features */
int wifi_set_channel(uint8_t channel);
int wifi_get_channel(uint8_t* channel);
int wifi_set_tx_power(int8_t power_dbm);
int wifi_get_tx_power(int8_t* power_dbm);

/* Packet transmission */
int wifi_send_packet(uint8_t* data, uint32_t length);
int wifi_receive_packet(uint8_t* buffer, uint32_t max_length, uint32_t* actual_length);

#endif /* AURORA_WIFI_H */
