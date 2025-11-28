/**
 * Aurora OS - DNS Resolver Header
 * 
 * Domain Name System resolution with caching
 */

#ifndef AURORA_DNS_H
#define AURORA_DNS_H

#include <stdint.h>

/* DNS Configuration */
#define DNS_PORT                53
#define DNS_MAX_NAME_LENGTH     256
#define DNS_MAX_CACHE_ENTRIES   256
#define DNS_CACHE_TTL_DEFAULT   300     /* 5 minutes default TTL */
#define DNS_TIMEOUT_MS          5000    /* 5 second timeout */
#define DNS_MAX_RETRIES         3

/* DNS Record Types */
#define DNS_TYPE_A              1       /* IPv4 address */
#define DNS_TYPE_NS             2       /* Name server */
#define DNS_TYPE_CNAME          5       /* Canonical name */
#define DNS_TYPE_SOA            6       /* Start of authority */
#define DNS_TYPE_PTR            12      /* Pointer record */
#define DNS_TYPE_MX             15      /* Mail exchange */
#define DNS_TYPE_TXT            16      /* Text record */
#define DNS_TYPE_AAAA           28      /* IPv6 address */
#define DNS_TYPE_SRV            33      /* Service record */

/* DNS Classes */
#define DNS_CLASS_IN            1       /* Internet */

/* DNS Response Codes */
#define DNS_RCODE_OK            0       /* No error */
#define DNS_RCODE_FORMAT_ERR    1       /* Format error */
#define DNS_RCODE_SERVER_FAIL   2       /* Server failure */
#define DNS_RCODE_NAME_ERR      3       /* Name error (NXDOMAIN) */
#define DNS_RCODE_NOT_IMPL      4       /* Not implemented */
#define DNS_RCODE_REFUSED       5       /* Query refused */

/* DNS Header Structure */
typedef struct {
    uint16_t id;            /* Transaction ID */
    uint16_t flags;         /* Flags and codes */
    uint16_t qd_count;      /* Question count */
    uint16_t an_count;      /* Answer count */
    uint16_t ns_count;      /* Authority count */
    uint16_t ar_count;      /* Additional count */
} __attribute__((packed)) dns_header_t;

/* DNS Question Structure */
typedef struct {
    /* QNAME is variable length, encoded before this */
    uint16_t qtype;         /* Question type */
    uint16_t qclass;        /* Question class */
} __attribute__((packed)) dns_question_t;

/* DNS Resource Record Structure */
typedef struct {
    /* NAME is variable length, encoded before this */
    uint16_t type;          /* Record type */
    uint16_t class;         /* Record class */
    uint32_t ttl;           /* Time to live */
    uint16_t rdlength;      /* Resource data length */
    /* RDATA follows */
} __attribute__((packed)) dns_rr_t;

/* DNS Cache Entry */
typedef struct dns_cache_entry {
    char hostname[DNS_MAX_NAME_LENGTH];
    uint32_t ip_addr;       /* IPv4 address */
    uint32_t ttl;           /* Remaining TTL */
    uint64_t timestamp;     /* When entry was created */
    uint8_t valid;
    struct dns_cache_entry* next;
} dns_cache_entry_t;

/* DNS Resolver State */
typedef struct {
    uint32_t primary_dns;
    uint32_t secondary_dns;
    dns_cache_entry_t* cache;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t queries_sent;
    uint32_t responses_received;
    uint32_t errors;
    uint16_t next_transaction_id;
    uint8_t initialized;
} dns_resolver_t;

/* DNS Query Result */
typedef struct {
    uint32_t ip_addr;
    uint32_t ttl;
    uint8_t success;
    uint8_t from_cache;
    uint8_t rcode;
} dns_result_t;

/**
 * Initialize DNS resolver
 */
void dns_init(void);

/**
 * Set primary DNS server
 * @param dns_ip DNS server IP address
 */
void dns_set_primary(uint32_t dns_ip);

/**
 * Set secondary DNS server
 * @param dns_ip DNS server IP address
 */
void dns_set_secondary(uint32_t dns_ip);

/**
 * Resolve hostname to IP address
 * @param hostname Domain name to resolve
 * @param result Result structure
 * @return 0 on success, -1 on error
 */
int dns_resolve(const char* hostname, dns_result_t* result);

/**
 * Resolve hostname (simple version, returns IP)
 * @param hostname Domain name to resolve
 * @return IP address or 0 on error
 */
uint32_t dns_lookup(const char* hostname);

/**
 * Add entry to DNS cache
 * @param hostname Hostname
 * @param ip_addr IP address
 * @param ttl Time to live in seconds
 */
void dns_cache_add(const char* hostname, uint32_t ip_addr, uint32_t ttl);

/**
 * Lookup entry in DNS cache
 * @param hostname Hostname to lookup
 * @return IP address or 0 if not found
 */
uint32_t dns_cache_lookup(const char* hostname);

/**
 * Clear DNS cache
 */
void dns_cache_clear(void);

/**
 * Flush expired cache entries
 */
void dns_cache_flush_expired(void);

/**
 * Get DNS resolver statistics
 * @param hits Output: cache hits
 * @param misses Output: cache misses
 * @param queries Output: queries sent
 */
void dns_get_stats(uint32_t* hits, uint32_t* misses, uint32_t* queries);

/**
 * Parse IP address from string (e.g., "192.168.1.1")
 * @param str IP address string
 * @return IP address in network byte order, 0 on error
 */
uint32_t dns_parse_ip(const char* str);

/**
 * Format IP address to string
 * @param ip IP address
 * @param buffer Output buffer
 * @param size Buffer size
 */
void dns_format_ip(uint32_t ip, char* buffer, uint32_t size);

/**
 * Reverse DNS lookup (PTR record)
 * @param ip IP address to lookup
 * @param hostname Output buffer for hostname
 * @param hostname_size Buffer size
 * @return 0 on success, -1 on error
 */
int dns_reverse_lookup(uint32_t ip, char* hostname, uint32_t hostname_size);

#endif /* AURORA_DNS_H */
