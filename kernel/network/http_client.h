/**
 * Aurora OS - HTTP Client Header
 * 
 * HTTP/1.1 client implementation with HTTPS support
 */

#ifndef AURORA_HTTP_CLIENT_H
#define AURORA_HTTP_CLIENT_H

#include <stdint.h>
#include "network.h"

/* HTTP Configuration */
#define HTTP_MAX_URL_LENGTH         2048
#define HTTP_MAX_HEADER_SIZE        8192
#define HTTP_MAX_BODY_SIZE          (10 * 1024 * 1024)  /* 10 MB */
#define HTTP_MAX_HEADERS            64
#define HTTP_DEFAULT_TIMEOUT_MS     30000
#define HTTP_MAX_REDIRECTS          10
#define HTTP_BUFFER_SIZE            4096

/* HTTP Methods */
#define HTTP_METHOD_GET             0
#define HTTP_METHOD_POST            1
#define HTTP_METHOD_PUT             2
#define HTTP_METHOD_DELETE          3
#define HTTP_METHOD_HEAD            4
#define HTTP_METHOD_OPTIONS         5
#define HTTP_METHOD_PATCH           6
#define HTTP_METHOD_TRACE           7
#define HTTP_METHOD_CONNECT         8

/* HTTP Status Codes */
#define HTTP_STATUS_CONTINUE                100
#define HTTP_STATUS_SWITCHING_PROTOCOLS     101
#define HTTP_STATUS_OK                      200
#define HTTP_STATUS_CREATED                 201
#define HTTP_STATUS_ACCEPTED                202
#define HTTP_STATUS_NO_CONTENT              204
#define HTTP_STATUS_PARTIAL_CONTENT         206
#define HTTP_STATUS_MOVED_PERMANENTLY       301
#define HTTP_STATUS_FOUND                   302
#define HTTP_STATUS_SEE_OTHER               303
#define HTTP_STATUS_NOT_MODIFIED            304
#define HTTP_STATUS_TEMPORARY_REDIRECT      307
#define HTTP_STATUS_PERMANENT_REDIRECT      308
#define HTTP_STATUS_BAD_REQUEST             400
#define HTTP_STATUS_UNAUTHORIZED            401
#define HTTP_STATUS_FORBIDDEN               403
#define HTTP_STATUS_NOT_FOUND               404
#define HTTP_STATUS_METHOD_NOT_ALLOWED      405
#define HTTP_STATUS_REQUEST_TIMEOUT         408
#define HTTP_STATUS_CONFLICT                409
#define HTTP_STATUS_GONE                    410
#define HTTP_STATUS_LENGTH_REQUIRED         411
#define HTTP_STATUS_PAYLOAD_TOO_LARGE       413
#define HTTP_STATUS_URI_TOO_LONG            414
#define HTTP_STATUS_UNSUPPORTED_MEDIA       415
#define HTTP_STATUS_TOO_MANY_REQUESTS       429
#define HTTP_STATUS_INTERNAL_ERROR          500
#define HTTP_STATUS_NOT_IMPLEMENTED         501
#define HTTP_STATUS_BAD_GATEWAY             502
#define HTTP_STATUS_SERVICE_UNAVAILABLE     503
#define HTTP_STATUS_GATEWAY_TIMEOUT         504

/* HTTP Header */
typedef struct {
    char name[128];
    char value[512];
} http_header_t;

/* HTTP Request Configuration */
typedef struct {
    uint8_t method;
    char url[HTTP_MAX_URL_LENGTH];
    char host[256];
    uint16_t port;
    char path[1024];
    char query[1024];
    uint8_t use_ssl;
    
    /* Headers */
    http_header_t headers[HTTP_MAX_HEADERS];
    uint32_t header_count;
    
    /* Body */
    uint8_t* body;
    uint32_t body_length;
    char content_type[128];
    
    /* Timeouts */
    uint32_t connect_timeout_ms;
    uint32_t read_timeout_ms;
    
    /* Options */
    uint8_t follow_redirects;
    uint8_t verify_ssl;
    char user_agent[256];
    char auth_header[512];
    
    /* Proxy */
    char proxy_host[256];
    uint16_t proxy_port;
    uint8_t use_proxy;
} http_request_config_t;

/* HTTP Response */
typedef struct {
    uint16_t status_code;
    char status_text[64];
    char http_version[16];
    
    /* Headers */
    http_header_t headers[HTTP_MAX_HEADERS];
    uint32_t header_count;
    
    /* Body */
    uint8_t* body;
    uint32_t body_length;
    uint32_t content_length;
    char content_type[128];
    char content_encoding[64];
    char transfer_encoding[64];
    
    /* Connection info */
    uint8_t connection_keep_alive;
    uint8_t chunked;
    
    /* Timing */
    uint32_t dns_time_ms;
    uint32_t connect_time_ms;
    uint32_t ssl_time_ms;
    uint32_t send_time_ms;
    uint32_t receive_time_ms;
    uint32_t total_time_ms;
    
    /* Error */
    int error_code;
    char error_message[256];
} http_response_t;

/* HTTP Client Connection */
typedef struct http_connection {
    socket_t* socket;
    void* ssl;          /* SSL session if HTTPS */
    char host[256];
    uint16_t port;
    uint8_t is_ssl;
    uint8_t keep_alive;
    uint64_t last_used;
} http_connection_t;

/* HTTP Client State */
typedef struct {
    /* Connection pool */
    http_connection_t connections[16];
    uint32_t connection_count;
    
    /* Default settings */
    char user_agent[256];
    uint32_t default_timeout_ms;
    uint8_t follow_redirects;
    uint8_t verify_ssl;
    
    /* Statistics */
    uint32_t requests_sent;
    uint32_t responses_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t errors;
    
    uint8_t initialized;
} http_client_t;

/**
 * Initialize HTTP client
 */
void http_client_init(void);

/**
 * Cleanup HTTP client
 */
void http_client_cleanup(void);

/**
 * Create HTTP request configuration
 * @param method HTTP method
 * @param url URL to request
 * @return Request configuration or NULL on error
 */
http_request_config_t* http_create_request(uint8_t method, const char* url);

/**
 * Add header to request
 * @param request Request configuration
 * @param name Header name
 * @param value Header value
 * @return 0 on success, -1 on error
 */
int http_add_header(http_request_config_t* request, const char* name, const char* value);

/**
 * Set request body
 * @param request Request configuration
 * @param body Body data
 * @param length Body length
 * @param content_type Content type
 * @return 0 on success, -1 on error
 */
int http_set_body(http_request_config_t* request, const uint8_t* body, 
                  uint32_t length, const char* content_type);

/**
 * Set Basic authentication
 * @param request Request configuration
 * @param username Username
 * @param password Password
 * @return 0 on success, -1 on error
 */
int http_set_basic_auth(http_request_config_t* request, const char* username, const char* password);

/**
 * Set Bearer token authentication
 * @param request Request configuration
 * @param token Bearer token
 * @return 0 on success, -1 on error
 */
int http_set_bearer_auth(http_request_config_t* request, const char* token);

/**
 * Send HTTP request
 * @param request Request configuration
 * @return Response or NULL on error
 */
http_response_t* http_send(http_request_config_t* request);

/**
 * Free HTTP request
 * @param request Request to free
 */
void http_free_request(http_request_config_t* request);

/**
 * Free HTTP response
 * @param response Response to free
 */
void http_free_response(http_response_t* response);

/**
 * Get response header value
 * @param response Response
 * @param name Header name
 * @return Header value or NULL if not found
 */
const char* http_get_response_header(http_response_t* response, const char* name);

/**
 * Simple GET request
 * @param url URL to fetch
 * @return Response or NULL on error
 */
http_response_t* http_get(const char* url);

/**
 * Simple POST request
 * @param url URL to post to
 * @param body Body data
 * @param body_length Body length
 * @param content_type Content type
 * @return Response or NULL on error
 */
http_response_t* http_post(const char* url, const uint8_t* body, 
                           uint32_t body_length, const char* content_type);

/**
 * Download file to path
 * @param url URL to download
 * @param path Local path to save
 * @param progress_callback Optional callback for progress (can be NULL)
 * @return 0 on success, -1 on error
 */
int http_download_file(const char* url, const char* path,
                       void (*progress_callback)(uint64_t downloaded, uint64_t total));

/**
 * URL encode string
 * @param input Input string
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return Encoded string length or -1 on error
 */
int http_url_encode(const char* input, char* output, uint32_t output_size);

/**
 * URL decode string
 * @param input Input string
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return Decoded string length or -1 on error
 */
int http_url_decode(const char* input, char* output, uint32_t output_size);

/**
 * Parse URL into components
 * @param url URL string
 * @param protocol Output: protocol (http/https)
 * @param host Output: hostname
 * @param port Output: port number
 * @param path Output: path
 * @return 0 on success, -1 on error
 */
int http_parse_url(const char* url, char* protocol, char* host, 
                   uint16_t* port, char* path);

/**
 * Get HTTP client statistics
 * @param requests Output: requests sent
 * @param responses Output: responses received
 * @param bytes_sent Output: bytes sent
 * @param bytes_received Output: bytes received
 */
void http_get_stats(uint32_t* requests, uint32_t* responses,
                    uint32_t* bytes_sent, uint32_t* bytes_received);

/**
 * Set default user agent
 * @param user_agent User agent string
 */
void http_set_user_agent(const char* user_agent);

/**
 * Set default timeout
 * @param timeout_ms Timeout in milliseconds
 */
void http_set_timeout(uint32_t timeout_ms);

/**
 * Set default SSL verification
 * @param verify Enable/disable SSL verification
 */
void http_set_ssl_verify(uint8_t verify);

#endif /* AURORA_HTTP_CLIENT_H */
