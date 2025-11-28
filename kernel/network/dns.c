/**
 * Aurora OS - DNS Resolver Implementation
 * 
 * Domain Name System resolution with caching
 */

#include "dns.h"
#include "network.h"
#include "../memory/memory.h"
#include <stddef.h>

/* String utilities */
static size_t dns_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void dns_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int dns_strcmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void dns_memset(void* ptr, int value, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
}

static void dns_memcpy(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static char dns_tolower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int dns_strcasecmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && *s2) {
        char c1 = dns_tolower(*s1);
        char c2 = dns_tolower(*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return dns_tolower(*s1) - dns_tolower(*s2);
}

/* Global DNS resolver state */
static dns_resolver_t resolver;

/* DNS cache storage */
#define DNS_CACHE_STORAGE_SIZE 256
static dns_cache_entry_t cache_storage[DNS_CACHE_STORAGE_SIZE];
static uint32_t cache_storage_used = 0;

/* 
 * Stub time function for cache TTL validation.
 * In a real implementation, this would integrate with the kernel's
 * system time (e.g., from RTC or timer subsystem).
 * Currently uses monotonically increasing fake time for basic functionality.
 */
static uint64_t get_current_timestamp(void) {
    static uint64_t fake_time = 1700000000ULL;
    return fake_time++;
}

/* Network byte order helpers */
static uint16_t htons(uint16_t hostshort) {
    return ((hostshort >> 8) & 0xFF) | ((hostshort & 0xFF) << 8);
}

static uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

static uint32_t htonl(uint32_t hostlong) {
    return ((hostlong >> 24) & 0xFF) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong << 8) & 0xFF0000) |
           ((hostlong << 24) & 0xFF000000);
}

static uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}

/* Allocate cache entry */
static dns_cache_entry_t* alloc_cache_entry(void) {
    if (cache_storage_used >= DNS_CACHE_STORAGE_SIZE) {
        /* Cache full - evict oldest entry */
        uint64_t oldest_time = 0xFFFFFFFFFFFFFFFFULL;
        uint32_t oldest_idx = 0;
        
        for (uint32_t i = 0; i < DNS_CACHE_STORAGE_SIZE; i++) {
            if (cache_storage[i].valid && cache_storage[i].timestamp < oldest_time) {
                oldest_time = cache_storage[i].timestamp;
                oldest_idx = i;
            }
        }
        
        dns_memset(&cache_storage[oldest_idx], 0, sizeof(dns_cache_entry_t));
        return &cache_storage[oldest_idx];
    }
    
    dns_cache_entry_t* entry = &cache_storage[cache_storage_used++];
    dns_memset(entry, 0, sizeof(dns_cache_entry_t));
    return entry;
}

/* Encode hostname to DNS format (length-prefixed labels) */
static int encode_hostname(const char* hostname, uint8_t* buffer, uint32_t buffer_size) {
    if (!hostname || !buffer || buffer_size < 2) {
        return -1;
    }
    
    uint32_t pos = 0;
    const char* ptr = hostname;
    
    while (*ptr && pos < buffer_size - 2) {
        /* Find length of current label */
        const char* label_start = ptr;
        uint32_t label_len = 0;
        
        while (*ptr && *ptr != '.') {
            ptr++;
            label_len++;
        }
        
        if (label_len > 63 || label_len == 0) {
            return -1; /* Invalid label */
        }
        
        /* Write length byte */
        buffer[pos++] = (uint8_t)label_len;
        
        /* Write label */
        for (uint32_t i = 0; i < label_len && pos < buffer_size - 1; i++) {
            buffer[pos++] = (uint8_t)label_start[i];
        }
        
        /* Skip dot */
        if (*ptr == '.') {
            ptr++;
        }
    }
    
    /* Null terminator */
    buffer[pos++] = 0;
    
    return (int)pos;
}

/* Decode hostname from DNS format */
static int decode_hostname(const uint8_t* data, uint32_t data_len, uint32_t offset,
                           char* hostname, uint32_t hostname_size) {
    if (!data || !hostname || hostname_size < 2) {
        return -1;
    }
    
    uint32_t pos = offset;
    uint32_t out_pos = 0;
    int jumps = 0;
    int return_pos = -1;
    
    while (pos < data_len && out_pos < hostname_size - 1) {
        uint8_t len = data[pos];
        
        if (len == 0) {
            break;
        }
        
        /* Check for compression pointer */
        if ((len & 0xC0) == 0xC0) {
            if (pos + 1 >= data_len) {
                return -1;
            }
            
            if (return_pos < 0) {
                return_pos = (int)(pos + 2);
            }
            
            uint16_t pointer = ((len & 0x3F) << 8) | data[pos + 1];
            pos = pointer;
            jumps++;
            
            if (jumps > 10) {
                return -1; /* Prevent infinite loops */
            }
            continue;
        }
        
        pos++;
        
        if (out_pos > 0 && out_pos < hostname_size - 1) {
            hostname[out_pos++] = '.';
        }
        
        for (uint8_t i = 0; i < len && pos < data_len && out_pos < hostname_size - 1; i++) {
            hostname[out_pos++] = (char)data[pos++];
        }
    }
    
    hostname[out_pos] = '\0';
    
    if (return_pos >= 0) {
        return return_pos;
    }
    return (int)(pos + 1);
}

void dns_init(void) {
    if (resolver.initialized) return;
    
    dns_memset(&resolver, 0, sizeof(dns_resolver_t));
    dns_memset(cache_storage, 0, sizeof(cache_storage));
    
    /* Set default DNS servers (Google Public DNS) */
    resolver.primary_dns = dns_parse_ip("8.8.8.8");
    resolver.secondary_dns = dns_parse_ip("8.8.4.4");
    
    resolver.next_transaction_id = 1;
    resolver.initialized = 1;
    
    /* Pre-populate cache with some common entries for testing */
    dns_cache_add("localhost", dns_parse_ip("127.0.0.1"), 86400);
    dns_cache_add("aurora.os", dns_parse_ip("192.168.1.100"), 3600);
}

void dns_set_primary(uint32_t dns_ip) {
    resolver.primary_dns = dns_ip;
}

void dns_set_secondary(uint32_t dns_ip) {
    resolver.secondary_dns = dns_ip;
}

int dns_resolve(const char* hostname, dns_result_t* result) {
    if (!hostname || !result) {
        return -1;
    }
    
    if (!resolver.initialized) {
        dns_init();
    }
    
    dns_memset(result, 0, sizeof(dns_result_t));
    
    /* Check if it's already an IP address */
    uint32_t ip = dns_parse_ip(hostname);
    if (ip != 0) {
        result->ip_addr = ip;
        result->ttl = 0;
        result->success = 1;
        result->from_cache = 0;
        return 0;
    }
    
    /* Check cache first */
    uint32_t cached_ip = dns_cache_lookup(hostname);
    if (cached_ip != 0) {
        result->ip_addr = cached_ip;
        result->ttl = DNS_CACHE_TTL_DEFAULT;
        result->success = 1;
        result->from_cache = 1;
        resolver.cache_hits++;
        return 0;
    }
    
    resolver.cache_misses++;
    
    /* Build DNS query packet */
    uint8_t query[512];
    dns_memset(query, 0, sizeof(query));
    
    dns_header_t* header = (dns_header_t*)query;
    header->id = htons(resolver.next_transaction_id++);
    header->flags = htons(0x0100); /* Standard query, recursion desired */
    header->qd_count = htons(1);
    
    /* Encode hostname */
    int name_len = encode_hostname(hostname, query + sizeof(dns_header_t), 
                                    sizeof(query) - sizeof(dns_header_t) - sizeof(dns_question_t));
    if (name_len < 0) {
        return -1;
    }
    
    /* Add question */
    dns_question_t* question = (dns_question_t*)(query + sizeof(dns_header_t) + name_len);
    question->qtype = htons(DNS_TYPE_A);
    question->qclass = htons(DNS_CLASS_IN);
    
    uint32_t query_len = sizeof(dns_header_t) + (uint32_t)name_len + sizeof(dns_question_t);
    
    /* Create UDP socket */
    socket_t* sock = socket_create(PROTO_UDP);
    if (!sock) {
        return -1;
    }
    
    /* Bind to ephemeral port */
    socket_bind(sock, 0);
    
    /* Connect to DNS server */
    if (socket_connect(sock, resolver.primary_dns, DNS_PORT) < 0) {
        socket_close(sock);
        return -1;
    }
    
    /* Send query */
    resolver.queries_sent++;
    if (socket_send(sock, query, query_len) < 0) {
        socket_close(sock);
        resolver.errors++;
        return -1;
    }
    
    /* Receive response */
    uint8_t response[512];
    int recv_len = socket_receive(sock, response, sizeof(response));
    socket_close(sock);
    
    if (recv_len < (int)sizeof(dns_header_t)) {
        /* Try secondary DNS */
        sock = socket_create(PROTO_UDP);
        if (!sock) {
            resolver.errors++;
            return -1;
        }
        
        socket_bind(sock, 0);
        if (socket_connect(sock, resolver.secondary_dns, DNS_PORT) < 0) {
            socket_close(sock);
            resolver.errors++;
            return -1;
        }
        
        resolver.queries_sent++;
        if (socket_send(sock, query, query_len) < 0) {
            socket_close(sock);
            resolver.errors++;
            return -1;
        }
        
        recv_len = socket_receive(sock, response, sizeof(response));
        socket_close(sock);
        
        if (recv_len < (int)sizeof(dns_header_t)) {
            resolver.errors++;
            return -1;
        }
    }
    
    resolver.responses_received++;
    
    /* Parse response */
    dns_header_t* resp_header = (dns_header_t*)response;
    
    /* Check response code */
    uint16_t flags = ntohs(resp_header->flags);
    uint8_t rcode = flags & 0x0F;
    result->rcode = rcode;
    
    if (rcode != DNS_RCODE_OK) {
        return -1;
    }
    
    /* Skip question section */
    uint32_t pos = sizeof(dns_header_t);
    uint16_t qd_count = ntohs(resp_header->qd_count);
    
    for (uint16_t i = 0; i < qd_count && pos < (uint32_t)recv_len; i++) {
        /* Skip name */
        while (pos < (uint32_t)recv_len && response[pos] != 0) {
            if ((response[pos] & 0xC0) == 0xC0) {
                pos += 2;
                break;
            }
            pos += response[pos] + 1;
        }
        if (response[pos] == 0) pos++;
        pos += 4; /* Skip QTYPE and QCLASS */
    }
    
    /* Parse answer section */
    uint16_t an_count = ntohs(resp_header->an_count);
    
    for (uint16_t i = 0; i < an_count && pos < (uint32_t)recv_len; i++) {
        /* Skip name (possibly compressed) */
        if ((response[pos] & 0xC0) == 0xC0) {
            pos += 2;
        } else {
            while (pos < (uint32_t)recv_len && response[pos] != 0) {
                pos += response[pos] + 1;
            }
            if (response[pos] == 0) pos++;
        }
        
        if (pos + sizeof(dns_rr_t) > (uint32_t)recv_len) {
            break;
        }
        
        dns_rr_t* rr = (dns_rr_t*)&response[pos];
        uint16_t type = ntohs(rr->type);
        uint32_t ttl = ntohl(rr->ttl);
        uint16_t rdlength = ntohs(rr->rdlength);
        
        pos += 10; /* Skip RR header */
        
        if (type == DNS_TYPE_A && rdlength == 4) {
            /* Found A record */
            result->ip_addr = *(uint32_t*)&response[pos];
            result->ttl = ttl;
            result->success = 1;
            result->from_cache = 0;
            
            /* Add to cache */
            dns_cache_add(hostname, result->ip_addr, ttl);
            
            return 0;
        }
        
        pos += rdlength;
    }
    
    return -1;
}

uint32_t dns_lookup(const char* hostname) {
    dns_result_t result;
    if (dns_resolve(hostname, &result) == 0 && result.success) {
        return result.ip_addr;
    }
    return 0;
}

void dns_cache_add(const char* hostname, uint32_t ip_addr, uint32_t ttl) {
    if (!hostname || ip_addr == 0) {
        return;
    }
    
    /* Check if entry already exists */
    for (uint32_t i = 0; i < cache_storage_used; i++) {
        if (cache_storage[i].valid && 
            dns_strcasecmp(cache_storage[i].hostname, hostname) == 0) {
            /* Update existing entry */
            cache_storage[i].ip_addr = ip_addr;
            cache_storage[i].ttl = ttl;
            cache_storage[i].timestamp = get_current_timestamp();
            return;
        }
    }
    
    /* Allocate new entry */
    dns_cache_entry_t* entry = alloc_cache_entry();
    if (!entry) {
        return;
    }
    
    dns_strcpy(entry->hostname, hostname, DNS_MAX_NAME_LENGTH);
    entry->ip_addr = ip_addr;
    entry->ttl = ttl;
    entry->timestamp = get_current_timestamp();
    entry->valid = 1;
    
    /* Add to cache list */
    entry->next = resolver.cache;
    resolver.cache = entry;
}

uint32_t dns_cache_lookup(const char* hostname) {
    if (!hostname) {
        return 0;
    }
    
    uint64_t current_time = get_current_timestamp();
    
    for (uint32_t i = 0; i < cache_storage_used; i++) {
        if (cache_storage[i].valid &&
            dns_strcasecmp(cache_storage[i].hostname, hostname) == 0) {
            /* Check if entry has expired */
            if (current_time - cache_storage[i].timestamp > cache_storage[i].ttl) {
                cache_storage[i].valid = 0;
                return 0;
            }
            return cache_storage[i].ip_addr;
        }
    }
    
    return 0;
}

void dns_cache_clear(void) {
    dns_memset(cache_storage, 0, sizeof(cache_storage));
    cache_storage_used = 0;
    resolver.cache = NULL;
}

void dns_cache_flush_expired(void) {
    uint64_t current_time = get_current_timestamp();
    
    for (uint32_t i = 0; i < cache_storage_used; i++) {
        if (cache_storage[i].valid) {
            if (current_time - cache_storage[i].timestamp > cache_storage[i].ttl) {
                cache_storage[i].valid = 0;
            }
        }
    }
}

void dns_get_stats(uint32_t* hits, uint32_t* misses, uint32_t* queries) {
    if (hits) *hits = resolver.cache_hits;
    if (misses) *misses = resolver.cache_misses;
    if (queries) *queries = resolver.queries_sent;
}

uint32_t dns_parse_ip(const char* str) {
    if (!str) return 0;
    
    uint32_t ip = 0;
    uint32_t octet = 0;
    int octet_count = 0;
    int digit_count = 0;
    
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            octet = octet * 10 + (*str - '0');
            digit_count++;
            if (octet > 255 || digit_count > 3) {
                return 0; /* Invalid */
            }
        } else if (*str == '.') {
            if (digit_count == 0) return 0;
            ip = (ip << 8) | (octet & 0xFF);
            octet = 0;
            octet_count++;
            digit_count = 0;
            if (octet_count > 3) return 0;
        } else {
            return 0; /* Invalid character */
        }
        str++;
    }
    
    if (digit_count == 0 || octet_count != 3) {
        return 0; /* Need exactly 4 octets */
    }
    
    ip = (ip << 8) | (octet & 0xFF);
    
    return htonl(ip); /* Return in network byte order */
}

void dns_format_ip(uint32_t ip, char* buffer, uint32_t size) {
    if (!buffer || size < 16) return;
    
    ip = ntohl(ip);
    
    uint8_t octets[4];
    octets[0] = (ip >> 24) & 0xFF;
    octets[1] = (ip >> 16) & 0xFF;
    octets[2] = (ip >> 8) & 0xFF;
    octets[3] = ip & 0xFF;
    
    uint32_t pos = 0;
    
    for (int i = 0; i < 4; i++) {
        uint8_t val = octets[i];
        
        if (val >= 100) {
            buffer[pos++] = '0' + (val / 100);
            buffer[pos++] = '0' + ((val / 10) % 10);
            buffer[pos++] = '0' + (val % 10);
        } else if (val >= 10) {
            buffer[pos++] = '0' + (val / 10);
            buffer[pos++] = '0' + (val % 10);
        } else {
            buffer[pos++] = '0' + val;
        }
        
        if (i < 3) {
            buffer[pos++] = '.';
        }
    }
    
    buffer[pos] = '\0';
}

int dns_reverse_lookup(uint32_t ip, char* hostname, uint32_t hostname_size) {
    if (!hostname || hostname_size < 32) {
        return -1;
    }
    
    /* Build reverse DNS name (PTR record) */
    char ptr_name[64];
    ip = ntohl(ip);
    
    uint8_t octets[4];
    octets[0] = ip & 0xFF;
    octets[1] = (ip >> 8) & 0xFF;
    octets[2] = (ip >> 16) & 0xFF;
    octets[3] = (ip >> 24) & 0xFF;
    
    /* Format: d.c.b.a.in-addr.arpa */
    uint32_t pos = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t val = octets[i];
        if (val >= 100) {
            ptr_name[pos++] = '0' + (val / 100);
            ptr_name[pos++] = '0' + ((val / 10) % 10);
            ptr_name[pos++] = '0' + (val % 10);
        } else if (val >= 10) {
            ptr_name[pos++] = '0' + (val / 10);
            ptr_name[pos++] = '0' + (val % 10);
        } else {
            ptr_name[pos++] = '0' + val;
        }
        ptr_name[pos++] = '.';
    }
    dns_strcpy(&ptr_name[pos], "in-addr.arpa", sizeof(ptr_name) - pos);
    
    /* Build DNS query packet for PTR record */
    uint8_t query[512];
    dns_memset(query, 0, sizeof(query));
    
    dns_header_t* header = (dns_header_t*)query;
    header->id = htons(resolver.next_transaction_id++);
    header->flags = htons(0x0100);
    header->qd_count = htons(1);
    
    int name_len = encode_hostname(ptr_name, query + sizeof(dns_header_t),
                                    sizeof(query) - sizeof(dns_header_t) - sizeof(dns_question_t));
    if (name_len < 0) {
        return -1;
    }
    
    dns_question_t* question = (dns_question_t*)(query + sizeof(dns_header_t) + name_len);
    question->qtype = htons(DNS_TYPE_PTR);
    question->qclass = htons(DNS_CLASS_IN);
    
    uint32_t query_len = sizeof(dns_header_t) + (uint32_t)name_len + sizeof(dns_question_t);
    
    /* Send DNS query to primary DNS server */
    uint8_t response[512];
    int recv_len = -1;
    
    socket_t* sock = socket_create(PROTO_UDP);
    if (!sock) {
        dns_strcpy(hostname, "unknown", hostname_size);
        return -1;
    }
    
    socket_bind(sock, 0);
    if (socket_connect(sock, resolver.primary_dns, DNS_PORT) >= 0) {
        resolver.queries_sent++;
        if (socket_send(sock, query, query_len) >= 0) {
            recv_len = socket_receive(sock, response, sizeof(response));
        }
    }
    socket_close(sock);
    
    /* Try secondary DNS if primary failed */
    if (recv_len < (int)sizeof(dns_header_t) && resolver.secondary_dns != 0) {
        sock = socket_create(PROTO_UDP);
        if (sock) {
            socket_bind(sock, 0);
            if (socket_connect(sock, resolver.secondary_dns, DNS_PORT) >= 0) {
                resolver.queries_sent++;
                if (socket_send(sock, query, query_len) >= 0) {
                    recv_len = socket_receive(sock, response, sizeof(response));
                }
            }
            socket_close(sock);
        }
    }
    
    if (recv_len < (int)sizeof(dns_header_t)) {
        dns_strcpy(hostname, "unknown", hostname_size);
        resolver.errors++;
        return -1;
    }
    
    /* Safe to use recv_len as uint32_t since we verified it's positive */
    uint32_t response_len = (uint32_t)recv_len;
    
    resolver.responses_received++;
    
    /* Parse response header */
    dns_header_t* resp_header = (dns_header_t*)response;
    uint16_t flags = ntohs(resp_header->flags);
    uint8_t rcode = flags & 0x0F;
    
    if (rcode != DNS_RCODE_OK) {
        dns_strcpy(hostname, "unknown", hostname_size);
        return -1;
    }
    
    /* Skip question section */
    uint32_t resp_pos = sizeof(dns_header_t);
    uint16_t qd_count = ntohs(resp_header->qd_count);
    
    for (uint16_t i = 0; i < qd_count && resp_pos < response_len; i++) {
        /* Skip name */
        while (resp_pos < response_len && response[resp_pos] != 0) {
            if ((response[resp_pos] & 0xC0) == 0xC0) {
                resp_pos += 2;
                break;
            }
            resp_pos += response[resp_pos] + 1;
        }
        if (response[resp_pos] == 0) resp_pos++;
        resp_pos += 4; /* Skip QTYPE and QCLASS */
    }
    
    /* Parse answer section for PTR record */
    uint16_t an_count = ntohs(resp_header->an_count);
    
    for (uint16_t i = 0; i < an_count && resp_pos < response_len; i++) {
        /* Skip name (possibly compressed) */
        if ((response[resp_pos] & 0xC0) == 0xC0) {
            resp_pos += 2;
        } else {
            while (resp_pos < response_len && response[resp_pos] != 0) {
                resp_pos += response[resp_pos] + 1;
            }
            if (response[resp_pos] == 0) resp_pos++;
        }
        
        if (resp_pos + sizeof(dns_rr_t) > response_len) {
            break;
        }
        
        dns_rr_t* rr = (dns_rr_t*)&response[resp_pos];
        uint16_t type = ntohs(rr->type);
        uint16_t rdlength = ntohs(rr->rdlength);
        
        resp_pos += 10; /* Skip RR header */
        
        if (type == DNS_TYPE_PTR && rdlength > 0) {
            /* Found PTR record - decode the hostname */
            int decoded = decode_hostname(response, response_len, resp_pos, hostname, hostname_size);
            if (decoded > 0) {
                return 0;  /* Success */
            }
        }
        
        resp_pos += rdlength;
    }
    
    /* No PTR record found */
    dns_strcpy(hostname, "unknown", hostname_size);
    return 0;
}
