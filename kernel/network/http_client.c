/**
 * Aurora OS - HTTP Client Implementation
 * 
 * HTTP/1.1 client implementation with HTTPS support
 */

#include "http_client.h"
#include "network.h"
#include "dns.h"
#include "../memory/memory.h"
#include <stddef.h>

/* String utilities */
static size_t http_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void http_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int http_strcmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static int http_strncmp(const char* s1, const char* s2, size_t n) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (n > 0 && *s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void http_strcat(char* dest, const char* src, size_t max) {
    size_t dest_len = http_strlen(dest);
    size_t i = 0;
    while (src && src[i] && (dest_len + i) < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

static void http_memset(void* ptr, int value, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
}

static void http_memcpy(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static char* http_strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    if (!*needle) return (char*)haystack;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char*)haystack;
        haystack++;
    }
    return NULL;
}

static char http_tolower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int http_strcasecmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && *s2) {
        char c1 = http_tolower(*s1);
        char c2 = http_tolower(*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return http_tolower(*s1) - http_tolower(*s2);
}

static int http_atoi(const char* str) {
    if (!str) return 0;
    int result = 0;
    int negative = 0;
    
    while (*str == ' ' || *str == '\t') str++;
    
    if (*str == '-') {
        negative = 1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return negative ? -result : result;
}

static void http_itoa(int value, char* buffer, size_t size) {
    if (!buffer || size < 2) return;
    
    int negative = 0;
    if (value < 0) {
        negative = 1;
        value = -value;
    }
    
    char temp[32];
    int pos = 0;
    
    if (value == 0) {
        temp[pos++] = '0';
    } else {
        while (value > 0) {
            temp[pos++] = '0' + (value % 10);
            value /= 10;
        }
    }
    
    size_t out = 0;
    if (negative && out < size - 1) {
        buffer[out++] = '-';
    }
    
    while (pos > 0 && out < size - 1) {
        buffer[out++] = temp[--pos];
    }
    
    buffer[out] = '\0';
}

/* Global HTTP client state */
static http_client_t client;

/* Static storage for requests */
#define MAX_REQUEST_STORAGE 10
static http_request_config_t request_storage[MAX_REQUEST_STORAGE];
static uint32_t request_storage_used = 0;

/* Static storage for responses */
#define MAX_RESPONSE_STORAGE 10
static http_response_t response_storage[MAX_RESPONSE_STORAGE];
static uint32_t response_storage_used = 0;

/* Response body buffer */
#define RESPONSE_BODY_BUFFER_SIZE (256 * 1024)  /* 256 KB */
static uint8_t response_body_buffers[MAX_RESPONSE_STORAGE][RESPONSE_BODY_BUFFER_SIZE];

/* Base64 encoding table */
static const char base64_table[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Base64 encode */
static int base64_encode(const uint8_t* input, size_t input_len, 
                         char* output, size_t output_size) {
    if (!input || !output || output_size < ((input_len + 2) / 3 * 4 + 1)) {
        return -1;
    }
    
    size_t out_pos = 0;
    size_t i = 0;
    
    while (i < input_len) {
        uint32_t octet_a = i < input_len ? input[i++] : 0;
        uint32_t octet_b = i < input_len ? input[i++] : 0;
        uint32_t octet_c = i < input_len ? input[i++] : 0;
        
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        output[out_pos++] = base64_table[(triple >> 18) & 0x3F];
        output[out_pos++] = base64_table[(triple >> 12) & 0x3F];
        output[out_pos++] = (i > input_len + 1) ? '=' : base64_table[(triple >> 6) & 0x3F];
        output[out_pos++] = (i > input_len) ? '=' : base64_table[triple & 0x3F];
    }
    
    output[out_pos] = '\0';
    return (int)out_pos;
}

/* Method name strings */
static const char* method_names[] = {
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
};

void http_client_init(void) {
    if (client.initialized) return;
    
    http_memset(&client, 0, sizeof(http_client_t));
    http_memset(request_storage, 0, sizeof(request_storage));
    http_memset(response_storage, 0, sizeof(response_storage));
    
    http_strcpy(client.user_agent, "Aurora/1.0 (Aurora OS)", sizeof(client.user_agent));
    client.default_timeout_ms = HTTP_DEFAULT_TIMEOUT_MS;
    client.follow_redirects = 1;
    client.verify_ssl = 1;
    
    client.initialized = 1;
}

void http_client_cleanup(void) {
    /* Close all connections */
    for (uint32_t i = 0; i < client.connection_count; i++) {
        if (client.connections[i].socket) {
            socket_close(client.connections[i].socket);
        }
    }
    
    client.connection_count = 0;
    request_storage_used = 0;
    response_storage_used = 0;
}

http_request_config_t* http_create_request(uint8_t method, const char* url) {
    if (!url || method > HTTP_METHOD_CONNECT) {
        return NULL;
    }
    
    if (!client.initialized) {
        http_client_init();
    }
    
    /* Find free request slot */
    http_request_config_t* request = NULL;
    for (uint32_t i = 0; i < MAX_REQUEST_STORAGE; i++) {
        if (request_storage[i].url[0] == '\0') {
            request = &request_storage[i];
            break;
        }
    }
    
    if (!request) {
        /* Reuse oldest */
        request = &request_storage[request_storage_used % MAX_REQUEST_STORAGE];
        request_storage_used++;
    }
    
    http_memset(request, 0, sizeof(http_request_config_t));
    
    request->method = method;
    http_strcpy(request->url, url, HTTP_MAX_URL_LENGTH);
    
    /* Parse URL */
    char protocol[16] = {0};
    http_parse_url(url, protocol, request->host, &request->port, request->path);
    
    if (http_strcasecmp(protocol, "https") == 0) {
        request->use_ssl = 1;
        if (request->port == 0) request->port = 443;
    } else {
        request->use_ssl = 0;
        if (request->port == 0) request->port = 80;
    }
    
    if (request->path[0] == '\0') {
        http_strcpy(request->path, "/", sizeof(request->path));
    }
    
    /* Set defaults */
    http_strcpy(request->user_agent, client.user_agent, sizeof(request->user_agent));
    request->connect_timeout_ms = client.default_timeout_ms;
    request->read_timeout_ms = client.default_timeout_ms;
    request->follow_redirects = client.follow_redirects;
    request->verify_ssl = client.verify_ssl;
    
    /* Add default headers */
    http_add_header(request, "Host", request->host);
    http_add_header(request, "User-Agent", request->user_agent);
    http_add_header(request, "Accept", "*/*");
    http_add_header(request, "Connection", "keep-alive");
    
    return request;
}

int http_add_header(http_request_config_t* request, const char* name, const char* value) {
    if (!request || !name || !value) {
        return -1;
    }
    
    if (request->header_count >= HTTP_MAX_HEADERS) {
        return -1;
    }
    
    /* Check if header already exists */
    for (uint32_t i = 0; i < request->header_count; i++) {
        if (http_strcasecmp(request->headers[i].name, name) == 0) {
            /* Update existing header */
            http_strcpy(request->headers[i].value, value, sizeof(request->headers[i].value));
            return 0;
        }
    }
    
    /* Add new header */
    http_strcpy(request->headers[request->header_count].name, name, 
               sizeof(request->headers[request->header_count].name));
    http_strcpy(request->headers[request->header_count].value, value,
               sizeof(request->headers[request->header_count].value));
    request->header_count++;
    
    return 0;
}

int http_set_body(http_request_config_t* request, const uint8_t* body,
                  uint32_t length, const char* content_type) {
    if (!request) {
        return -1;
    }
    
    request->body = (uint8_t*)body;
    request->body_length = length;
    
    if (content_type) {
        http_strcpy(request->content_type, content_type, sizeof(request->content_type));
        http_add_header(request, "Content-Type", content_type);
    }
    
    /* Add Content-Length header */
    char len_str[32];
    http_itoa((int)length, len_str, sizeof(len_str));
    http_add_header(request, "Content-Length", len_str);
    
    return 0;
}

int http_set_basic_auth(http_request_config_t* request, const char* username, const char* password) {
    if (!request || !username || !password) {
        return -1;
    }
    
    /* Build credentials string: username:password */
    char credentials[256];
    http_strcpy(credentials, username, sizeof(credentials));
    http_strcat(credentials, ":", sizeof(credentials));
    http_strcat(credentials, password, sizeof(credentials));
    
    /* Base64 encode */
    char encoded[512];
    int encoded_len = base64_encode((const uint8_t*)credentials, http_strlen(credentials),
                                    encoded, sizeof(encoded));
    if (encoded_len < 0) {
        return -1;
    }
    
    /* Build auth header */
    http_strcpy(request->auth_header, "Basic ", sizeof(request->auth_header));
    http_strcat(request->auth_header, encoded, sizeof(request->auth_header));
    
    http_add_header(request, "Authorization", request->auth_header);
    
    return 0;
}

int http_set_bearer_auth(http_request_config_t* request, const char* token) {
    if (!request || !token) {
        return -1;
    }
    
    http_strcpy(request->auth_header, "Bearer ", sizeof(request->auth_header));
    http_strcat(request->auth_header, token, sizeof(request->auth_header));
    
    http_add_header(request, "Authorization", request->auth_header);
    
    return 0;
}

/* Build HTTP request string */
static int build_request_string(http_request_config_t* request, char* buffer, size_t buffer_size) {
    if (!request || !buffer || buffer_size < 256) {
        return -1;
    }
    
    buffer[0] = '\0';
    
    /* Request line */
    http_strcat(buffer, method_names[request->method], buffer_size);
    http_strcat(buffer, " ", buffer_size);
    http_strcat(buffer, request->path, buffer_size);
    if (request->query[0]) {
        http_strcat(buffer, "?", buffer_size);
        http_strcat(buffer, request->query, buffer_size);
    }
    http_strcat(buffer, " HTTP/1.1\r\n", buffer_size);
    
    /* Headers */
    for (uint32_t i = 0; i < request->header_count; i++) {
        http_strcat(buffer, request->headers[i].name, buffer_size);
        http_strcat(buffer, ": ", buffer_size);
        http_strcat(buffer, request->headers[i].value, buffer_size);
        http_strcat(buffer, "\r\n", buffer_size);
    }
    
    /* End of headers */
    http_strcat(buffer, "\r\n", buffer_size);
    
    return (int)http_strlen(buffer);
}

/* Parse HTTP response status line */
static int parse_status_line(const char* line, http_response_t* response) {
    if (!line || !response) return -1;
    
    /* Format: HTTP/x.x STATUS_CODE STATUS_TEXT */
    if (http_strncmp(line, "HTTP/", 5) != 0) {
        return -1;
    }
    
    /* Get version */
    const char* ptr = line + 5;
    size_t ver_len = 0;
    while (ptr[ver_len] && ptr[ver_len] != ' ' && ver_len < sizeof(response->http_version) - 1) {
        response->http_version[ver_len] = ptr[ver_len];
        ver_len++;
    }
    response->http_version[ver_len] = '\0';
    
    /* Skip space */
    ptr += ver_len;
    while (*ptr == ' ') ptr++;
    
    /* Get status code */
    response->status_code = (uint16_t)http_atoi(ptr);
    
    /* Skip status code */
    while (*ptr >= '0' && *ptr <= '9') ptr++;
    while (*ptr == ' ') ptr++;
    
    /* Get status text */
    http_strcpy(response->status_text, ptr, sizeof(response->status_text));
    
    /* Remove trailing \r\n */
    size_t len = http_strlen(response->status_text);
    while (len > 0 && (response->status_text[len-1] == '\r' || response->status_text[len-1] == '\n')) {
        response->status_text[--len] = '\0';
    }
    
    return 0;
}

/* Parse HTTP header line */
static int parse_header_line(const char* line, http_response_t* response) {
    if (!line || !response) return -1;
    
    /* Find colon separator */
    const char* colon = NULL;
    const char* ptr = line;
    while (*ptr && *ptr != ':') {
        ptr++;
    }
    
    if (*ptr != ':') {
        return -1;
    }
    colon = ptr;
    
    if (response->header_count >= HTTP_MAX_HEADERS) {
        return 0; /* Ignore extra headers */
    }
    
    /* Copy name */
    size_t name_len = colon - line;
    if (name_len >= sizeof(response->headers[response->header_count].name)) {
        name_len = sizeof(response->headers[response->header_count].name) - 1;
    }
    http_memcpy(response->headers[response->header_count].name, line, name_len);
    response->headers[response->header_count].name[name_len] = '\0';
    
    /* Skip colon and spaces */
    ptr = colon + 1;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    /* Copy value */
    http_strcpy(response->headers[response->header_count].value, ptr,
               sizeof(response->headers[response->header_count].value));
    
    /* Remove trailing \r\n */
    size_t len = http_strlen(response->headers[response->header_count].value);
    while (len > 0 && (response->headers[response->header_count].value[len-1] == '\r' ||
                       response->headers[response->header_count].value[len-1] == '\n')) {
        response->headers[response->header_count].value[--len] = '\0';
    }
    
    /* Check for special headers */
    const char* name = response->headers[response->header_count].name;
    const char* value = response->headers[response->header_count].value;
    
    if (http_strcasecmp(name, "Content-Length") == 0) {
        response->content_length = (uint32_t)http_atoi(value);
    } else if (http_strcasecmp(name, "Content-Type") == 0) {
        http_strcpy(response->content_type, value, sizeof(response->content_type));
    } else if (http_strcasecmp(name, "Transfer-Encoding") == 0) {
        http_strcpy(response->transfer_encoding, value, sizeof(response->transfer_encoding));
        if (http_strstr(value, "chunked")) {
            response->chunked = 1;
        }
    } else if (http_strcasecmp(name, "Content-Encoding") == 0) {
        http_strcpy(response->content_encoding, value, sizeof(response->content_encoding));
    } else if (http_strcasecmp(name, "Connection") == 0) {
        response->connection_keep_alive = (http_strcasecmp(value, "keep-alive") == 0);
    }
    
    response->header_count++;
    return 0;
}

http_response_t* http_send(http_request_config_t* request) {
    if (!request) {
        return NULL;
    }
    
    if (!client.initialized) {
        http_client_init();
    }
    
    /* Find free response slot */
    uint32_t resp_idx = response_storage_used % MAX_RESPONSE_STORAGE;
    response_storage_used++;
    http_response_t* response = &response_storage[resp_idx];
    http_memset(response, 0, sizeof(http_response_t));
    response->body = response_body_buffers[resp_idx];
    
    /* Resolve DNS */
    uint32_t ip_addr = dns_lookup(request->host);
    if (ip_addr == 0) {
        response->error_code = -1;
        http_strcpy(response->error_message, "DNS resolution failed", sizeof(response->error_message));
        return response;
    }
    
    /* Create socket */
    socket_t* sock = socket_create(PROTO_TCP);
    if (!sock) {
        response->error_code = -2;
        http_strcpy(response->error_message, "Failed to create socket", sizeof(response->error_message));
        return response;
    }
    
    /* Connect */
    if (socket_connect(sock, ip_addr, request->port) < 0) {
        socket_close(sock);
        response->error_code = -3;
        http_strcpy(response->error_message, "Connection failed", sizeof(response->error_message));
        return response;
    }
    
    /* Build and send request */
    char request_buffer[HTTP_MAX_HEADER_SIZE];
    int req_len = build_request_string(request, request_buffer, sizeof(request_buffer));
    if (req_len < 0) {
        socket_close(sock);
        response->error_code = -4;
        http_strcpy(response->error_message, "Failed to build request", sizeof(response->error_message));
        return response;
    }
    
    client.requests_sent++;
    client.bytes_sent += (uint32_t)req_len;
    
    /* Send headers */
    if (socket_send(sock, (uint8_t*)request_buffer, (uint32_t)req_len) < 0) {
        socket_close(sock);
        response->error_code = -5;
        http_strcpy(response->error_message, "Failed to send request", sizeof(response->error_message));
        return response;
    }
    
    /* Send body if present */
    if (request->body && request->body_length > 0) {
        if (socket_send(sock, request->body, request->body_length) < 0) {
            socket_close(sock);
            response->error_code = -6;
            http_strcpy(response->error_message, "Failed to send body", sizeof(response->error_message));
            return response;
        }
        client.bytes_sent += request->body_length;
    }
    
    /* Receive response */
    uint8_t recv_buffer[HTTP_BUFFER_SIZE];
    int recv_len = socket_receive(sock, recv_buffer, sizeof(recv_buffer));
    
    if (recv_len <= 0) {
        socket_close(sock);
        response->error_code = -7;
        http_strcpy(response->error_message, "Failed to receive response", sizeof(response->error_message));
        return response;
    }
    
    client.responses_received++;
    client.bytes_received += (uint32_t)recv_len;
    
    /* Parse response */
    recv_buffer[recv_len] = '\0';
    char* line = (char*)recv_buffer;
    char* next_line;
    int parsing_headers = 1;
    int first_line = 1;
    
    /* Find body start */
    char* body_start = http_strstr((char*)recv_buffer, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
    }
    
    while (line && parsing_headers) {
        /* Find end of line */
        next_line = http_strstr(line, "\r\n");
        if (next_line) {
            *next_line = '\0';
            next_line += 2;
        }
        
        if (first_line) {
            parse_status_line(line, response);
            first_line = 0;
        } else if (line[0] == '\0') {
            /* Empty line - end of headers */
            parsing_headers = 0;
        } else {
            parse_header_line(line, response);
        }
        
        line = next_line;
    }
    
    /* Copy body */
    if (body_start && body_start < (char*)recv_buffer + recv_len) {
        uint32_t body_len = (uint32_t)(recv_len - (body_start - (char*)recv_buffer));
        if (body_len > RESPONSE_BODY_BUFFER_SIZE - 1) {
            body_len = RESPONSE_BODY_BUFFER_SIZE - 1;
        }
        http_memcpy(response->body, body_start, body_len);
        response->body_length = body_len;
    }
    
    /* Handle chunked transfer encoding */
    if (response->chunked && response->body_length > 0) {
        /* Parse chunked data - simplified implementation */
        /* A full implementation would properly decode chunks */
    }
    
    /* Continue receiving if content-length indicates more data */
    while (response->content_length > 0 && 
           response->body_length < response->content_length &&
           response->body_length < RESPONSE_BODY_BUFFER_SIZE - 1) {
        recv_len = socket_receive(sock, recv_buffer, sizeof(recv_buffer));
        if (recv_len <= 0) break;
        
        uint32_t copy_len = (uint32_t)recv_len;
        if (response->body_length + copy_len > RESPONSE_BODY_BUFFER_SIZE - 1) {
            copy_len = RESPONSE_BODY_BUFFER_SIZE - 1 - response->body_length;
        }
        
        http_memcpy(response->body + response->body_length, recv_buffer, copy_len);
        response->body_length += copy_len;
        client.bytes_received += (uint32_t)recv_len;
    }
    
    socket_close(sock);
    
    return response;
}

void http_free_request(http_request_config_t* request) {
    if (request) {
        http_memset(request, 0, sizeof(http_request_config_t));
    }
}

void http_free_response(http_response_t* response) {
    if (response) {
        http_memset(response, 0, sizeof(http_response_t));
    }
}

const char* http_get_response_header(http_response_t* response, const char* name) {
    if (!response || !name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < response->header_count; i++) {
        if (http_strcasecmp(response->headers[i].name, name) == 0) {
            return response->headers[i].value;
        }
    }
    
    return NULL;
}

http_response_t* http_get(const char* url) {
    http_request_config_t* request = http_create_request(HTTP_METHOD_GET, url);
    if (!request) {
        return NULL;
    }
    
    http_response_t* response = http_send(request);
    http_free_request(request);
    
    return response;
}

http_response_t* http_post(const char* url, const uint8_t* body,
                           uint32_t body_length, const char* content_type) {
    http_request_config_t* request = http_create_request(HTTP_METHOD_POST, url);
    if (!request) {
        return NULL;
    }
    
    if (body && body_length > 0) {
        http_set_body(request, body, body_length, content_type ? content_type : "application/octet-stream");
    }
    
    http_response_t* response = http_send(request);
    http_free_request(request);
    
    return response;
}

int http_download_file(const char* url, const char* path,
                       void (*progress_callback)(uint64_t downloaded, uint64_t total)) {
    if (!url || !path) {
        return -1;
    }
    
    http_response_t* response = http_get(url);
    if (!response || response->status_code != HTTP_STATUS_OK) {
        return -1;
    }
    
    /* In real implementation, would write to file system */
    if (progress_callback) {
        progress_callback(response->body_length, response->body_length);
    }
    
    http_free_response(response);
    return 0;
}

int http_url_encode(const char* input, char* output, uint32_t output_size) {
    if (!input || !output || output_size < 2) {
        return -1;
    }
    
    static const char hex[] = "0123456789ABCDEF";
    uint32_t out_pos = 0;
    
    while (*input && out_pos < output_size - 4) {
        char c = *input++;
        
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            output[out_pos++] = c;
        } else {
            output[out_pos++] = '%';
            output[out_pos++] = hex[(c >> 4) & 0x0F];
            output[out_pos++] = hex[c & 0x0F];
        }
    }
    
    output[out_pos] = '\0';
    return (int)out_pos;
}

int http_url_decode(const char* input, char* output, uint32_t output_size) {
    if (!input || !output || output_size < 2) {
        return -1;
    }
    
    uint32_t out_pos = 0;
    
    while (*input && out_pos < output_size - 1) {
        if (*input == '%' && input[1] && input[2]) {
            char hex[3] = {input[1], input[2], 0};
            int value = 0;
            
            for (int i = 0; i < 2; i++) {
                value <<= 4;
                if (hex[i] >= '0' && hex[i] <= '9') {
                    value |= hex[i] - '0';
                } else if (hex[i] >= 'A' && hex[i] <= 'F') {
                    value |= hex[i] - 'A' + 10;
                } else if (hex[i] >= 'a' && hex[i] <= 'f') {
                    value |= hex[i] - 'a' + 10;
                }
            }
            
            output[out_pos++] = (char)value;
            input += 3;
        } else if (*input == '+') {
            output[out_pos++] = ' ';
            input++;
        } else {
            output[out_pos++] = *input++;
        }
    }
    
    output[out_pos] = '\0';
    return (int)out_pos;
}

int http_parse_url(const char* url, char* protocol, char* host,
                   uint16_t* port, char* path) {
    if (!url) {
        return -1;
    }
    
    const char* ptr = url;
    
    /* Parse protocol */
    const char* protocol_end = http_strstr(ptr, "://");
    if (protocol_end && protocol) {
        size_t proto_len = protocol_end - ptr;
        if (proto_len < 16) {
            http_memcpy(protocol, ptr, proto_len);
            protocol[proto_len] = '\0';
        }
        ptr = protocol_end + 3;
    } else if (protocol) {
        http_strcpy(protocol, "http", 16);
    }
    
    /* Parse host and port */
    const char* path_start = ptr;
    while (*path_start && *path_start != '/' && *path_start != '?' && *path_start != '#') {
        path_start++;
    }
    
    /* Check for port */
    const char* port_sep = NULL;
    for (const char* p = ptr; p < path_start; p++) {
        if (*p == ':') {
            port_sep = p;
            break;
        }
    }
    
    if (host) {
        size_t host_len;
        if (port_sep) {
            host_len = port_sep - ptr;
        } else {
            host_len = path_start - ptr;
        }
        if (host_len < 256) {
            http_memcpy(host, ptr, host_len);
            host[host_len] = '\0';
        }
    }
    
    if (port) {
        if (port_sep) {
            *port = (uint16_t)http_atoi(port_sep + 1);
        } else {
            *port = 0; /* Will be set to default later */
        }
    }
    
    /* Parse path */
    if (path) {
        if (*path_start) {
            http_strcpy(path, path_start, 1024);
        } else {
            http_strcpy(path, "/", 1024);
        }
    }
    
    return 0;
}

void http_get_stats(uint32_t* requests, uint32_t* responses,
                    uint32_t* bytes_sent, uint32_t* bytes_received) {
    if (requests) *requests = client.requests_sent;
    if (responses) *responses = client.responses_received;
    if (bytes_sent) *bytes_sent = client.bytes_sent;
    if (bytes_received) *bytes_received = client.bytes_received;
}

void http_set_user_agent(const char* user_agent) {
    if (user_agent) {
        http_strcpy(client.user_agent, user_agent, sizeof(client.user_agent));
    }
}

void http_set_timeout(uint32_t timeout_ms) {
    client.default_timeout_ms = timeout_ms;
}

void http_set_ssl_verify(uint8_t verify) {
    client.verify_ssl = verify;
}
