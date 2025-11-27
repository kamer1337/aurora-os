/**
 * @file gdb_server.c
 * @brief GDB Server Socket Implementation
 *
 * Completes the GDB remote debugging infrastructure with network support
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../include/platform/aurora_vm.h"
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * GDB PROTOCOL DEFINITIONS
 * ============================================================================ */

/* GDB packet markers */
#define GDB_START           '$'
#define GDB_END             '#'
#define GDB_ACK             '+'
#define GDB_NACK            '-'
#define GDB_INTERRUPT       '\x03'
#define GDB_ESCAPE          '}'
#define GDB_XOR_MASK        0x20

/* GDB Signal numbers */
#define GDB_SIGNAL_TRAP     5   /* SIGTRAP */
#define GDB_SIGNAL_INT      2   /* SIGINT */
#define GDB_SIGNAL_SEGV     11  /* SIGSEGV */
#define GDB_SIGNAL_ILL      4   /* SIGILL */

/* GDB packet buffer size */
#define GDB_PACKET_SIZE     4096

/* GDB register IDs for x86 */
#define GDB_REG_EAX         0
#define GDB_REG_ECX         1
#define GDB_REG_EDX         2
#define GDB_REG_EBX         3
#define GDB_REG_ESP         4
#define GDB_REG_EBP         5
#define GDB_REG_ESI         6
#define GDB_REG_EDI         7
#define GDB_REG_EIP         8
#define GDB_REG_EFLAGS      9
#define GDB_REG_CS          10
#define GDB_REG_SS          11
#define GDB_REG_DS          12
#define GDB_REG_ES          13
#define GDB_REG_FS          14
#define GDB_REG_GS          15

/* ============================================================================
 * GDB SERVER STRUCTURES
 * ============================================================================ */

/* Socket abstraction */
typedef struct {
    int fd;                     /* Socket file descriptor */
    int client_fd;              /* Client connection */
    uint16_t port;              /* Listening port */
    bool listening;             /* Server is listening */
    bool connected;             /* Client connected */
} gdb_socket_t;

/* GDB packet parser state */
typedef struct {
    char buffer[GDB_PACKET_SIZE];
    uint32_t length;
    uint32_t expected_checksum;
    bool in_packet;
    bool escaped;
} gdb_parser_t;

/* GDB Server state */
typedef struct {
    gdb_socket_t socket;
    gdb_parser_t parser;
    AuroraVM* vm;
    
    /* Execution state */
    bool running;
    bool stepping;
    bool stopped;
    int stop_signal;
    
    /* Breakpoints */
    uint32_t breakpoints[AURORA_VM_MAX_BREAKPOINTS];
    uint32_t num_breakpoints;
    
    /* Watchpoints */
    struct {
        uint32_t addr;
        uint32_t size;
        uint8_t type;   /* 0=write, 1=read, 2=access */
        bool active;
    } watchpoints[8];
    uint32_t num_watchpoints;
    
    /* Response buffer */
    char response[GDB_PACKET_SIZE];
    
    /* Features */
    bool no_ack_mode;
    bool extended_mode;
    bool multiprocess;
    
    bool initialized;
} gdb_server_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static gdb_server_t g_gdb_server;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Convert hex character to value
 */
static int hex_char_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/**
 * Convert value to hex character
 */
static char value_to_hex(int v) {
    static const char hex[] = "0123456789abcdef";
    return hex[v & 0xF];
}

/**
 * Parse hex string to value
 */
static uint32_t parse_hex(const char* str, uint32_t* consumed) {
    uint32_t value = 0;
    uint32_t count = 0;
    
    while (*str) {
        int v = hex_char_value(*str);
        if (v < 0) break;
        value = (value << 4) | v;
        str++;
        count++;
    }
    
    if (consumed) *consumed = count;
    return value;
}

/**
 * Write hex value to string
 */
static void write_hex(char* str, uint32_t value, int bytes) {
    for (int i = bytes - 1; i >= 0; i--) {
        uint8_t b = (value >> (i * 8)) & 0xFF;
        *str++ = value_to_hex(b >> 4);
        *str++ = value_to_hex(b);
    }
    *str = '\0';
}

/**
 * Calculate checksum
 */
static uint8_t calculate_checksum(const char* data, uint32_t length) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += (uint8_t)data[i];
    }
    return sum;
}

/* ============================================================================
 * SOCKET SIMULATION (would use real sockets in production)
 * ============================================================================ */

/* Simulated socket buffer */
static char g_socket_rx_buffer[GDB_PACKET_SIZE];
static uint32_t g_socket_rx_len = 0;
static char g_socket_tx_buffer[GDB_PACKET_SIZE];
static uint32_t g_socket_tx_len = 0;

/**
 * Initialize socket
 */
static int socket_init(gdb_socket_t* sock, uint16_t port) {
    sock->fd = 0;
    sock->client_fd = -1;
    sock->port = port;
    sock->listening = false;
    sock->connected = false;
    
    /* In a real implementation, would:
     * 1. Create socket (socket())
     * 2. Bind to port (bind())
     * 3. Start listening (listen())
     */
    
    sock->fd = 1;  /* Simulated fd */
    sock->listening = true;
    
    return 0;
}

/**
 * Accept connection
 */
static int socket_accept(gdb_socket_t* sock) {
    if (!sock->listening) {
        return -1;
    }
    
    /* In a real implementation, would call accept() */
    sock->client_fd = 2;  /* Simulated client fd */
    sock->connected = true;
    
    return 0;
}

/**
 * Receive data
 */
static int socket_recv(gdb_socket_t* sock, char* buffer, uint32_t max_len) {
    if (!sock->connected) {
        return -1;
    }
    
    /* In a real implementation, would call recv() */
    uint32_t to_read = (g_socket_rx_len < max_len) ? g_socket_rx_len : max_len;
    if (to_read > 0) {
        platform_memcpy(buffer, g_socket_rx_buffer, to_read);
        /* Shift remaining data */
        for (uint32_t i = to_read; i < g_socket_rx_len; i++) {
            g_socket_rx_buffer[i - to_read] = g_socket_rx_buffer[i];
        }
        g_socket_rx_len -= to_read;
    }
    
    return (int)to_read;
}

/**
 * Send data
 */
static int socket_send(gdb_socket_t* sock, const char* buffer, uint32_t length) {
    if (!sock->connected) {
        return -1;
    }
    
    /* In a real implementation, would call send() */
    if (g_socket_tx_len + length > GDB_PACKET_SIZE) {
        return -1;
    }
    
    platform_memcpy(g_socket_tx_buffer + g_socket_tx_len, buffer, length);
    g_socket_tx_len += length;
    
    return (int)length;
}

/**
 * Close socket
 */
static void socket_close(gdb_socket_t* sock) {
    sock->connected = false;
    sock->listening = false;
    sock->client_fd = -1;
    sock->fd = -1;
}

/* ============================================================================
 * GDB PACKET HANDLING
 * ============================================================================ */

/**
 * Send GDB packet
 */
static int gdb_send_packet(const char* data) {
    uint32_t len = platform_strlen(data);
    uint8_t checksum = calculate_checksum(data, len);
    
    char packet[GDB_PACKET_SIZE];
    packet[0] = GDB_START;
    platform_memcpy(packet + 1, data, len);
    packet[1 + len] = GDB_END;
    packet[2 + len] = value_to_hex(checksum >> 4);
    packet[3 + len] = value_to_hex(checksum);
    packet[4 + len] = '\0';
    
    return socket_send(&g_gdb_server.socket, packet, 4 + len);
}

/**
 * Send OK response
 */
static void gdb_send_ok(void) {
    gdb_send_packet("OK");
}

/**
 * Send error response
 */
static void gdb_send_error(int error) {
    char resp[8];
    resp[0] = 'E';
    resp[1] = value_to_hex(error >> 4);
    resp[2] = value_to_hex(error);
    resp[3] = '\0';
    gdb_send_packet(resp);
}

/**
 * Send stop reply
 */
static void gdb_send_stop_reply(int signal) {
    char resp[8];
    resp[0] = 'S';
    resp[1] = value_to_hex(signal >> 4);
    resp[2] = value_to_hex(signal);
    resp[3] = '\0';
    gdb_send_packet(resp);
}

/* ============================================================================
 * GDB COMMAND HANDLERS
 * ============================================================================ */

/**
 * Handle query packet (q)
 */
static void gdb_handle_query(const char* packet) {
    if (platform_strncmp(packet, "qSupported", 10) == 0) {
        /* Report supported features */
        gdb_send_packet("PacketSize=1000;qXfer:features:read+;swbreak+;hwbreak+");
    }
    else if (platform_strncmp(packet, "qAttached", 9) == 0) {
        gdb_send_packet("1");
    }
    else if (platform_strncmp(packet, "qC", 2) == 0) {
        /* Current thread ID */
        gdb_send_packet("QC1");
    }
    else if (platform_strncmp(packet, "qfThreadInfo", 12) == 0) {
        gdb_send_packet("m1");
    }
    else if (platform_strncmp(packet, "qsThreadInfo", 12) == 0) {
        gdb_send_packet("l");
    }
    else if (platform_strncmp(packet, "qOffsets", 8) == 0) {
        gdb_send_packet("Text=0;Data=0;Bss=0");
    }
    else {
        gdb_send_packet("");  /* Unsupported query */
    }
}

/**
 * Handle read registers (g)
 */
static void gdb_handle_read_registers(void) {
    if (!g_gdb_server.vm) {
        gdb_send_error(1);
        return;
    }
    
    char resp[256];
    char* p = resp;
    
    /* Write general purpose registers */
    for (int i = 0; i < 16; i++) {
        uint32_t reg = aurora_vm_get_register(g_gdb_server.vm, i);
        write_hex(p, reg, 4);
        p += 8;
    }
    
    /* Write PC */
    write_hex(p, g_gdb_server.vm->cpu.pc, 4);
    p += 8;
    
    /* Write flags */
    write_hex(p, g_gdb_server.vm->cpu.flags, 4);
    p += 8;
    
    *p = '\0';
    gdb_send_packet(resp);
}

/**
 * Handle write registers (G)
 */
static void gdb_handle_write_registers(const char* packet) {
    if (!g_gdb_server.vm) {
        gdb_send_error(1);
        return;
    }
    
    const char* p = packet + 1;  /* Skip 'G' */
    
    /* Read general purpose registers */
    for (int i = 0; i < 16; i++) {
        uint32_t consumed;
        uint32_t reg = parse_hex(p, &consumed);
        aurora_vm_set_register(g_gdb_server.vm, i, reg);
        p += 8;
    }
    
    gdb_send_ok();
}

/**
 * Handle read memory (m)
 */
static void gdb_handle_read_memory(const char* packet) {
    if (!g_gdb_server.vm) {
        gdb_send_error(1);
        return;
    }
    
    const char* p = packet + 1;  /* Skip 'm' */
    uint32_t consumed;
    
    /* Parse address */
    uint32_t addr = parse_hex(p, &consumed);
    p += consumed;
    
    if (*p != ',') {
        gdb_send_error(1);
        return;
    }
    p++;
    
    /* Parse length */
    uint32_t length = parse_hex(p, &consumed);
    
    if (length > 512) length = 512;  /* Limit size */
    
    char resp[1024];
    uint8_t buf[512];
    
    int bytes = aurora_vm_read_memory(g_gdb_server.vm, addr, length, buf);
    if (bytes < 0) {
        gdb_send_error(1);
        return;
    }
    
    /* Convert to hex */
    for (uint32_t i = 0; i < (uint32_t)bytes; i++) {
        resp[i*2] = value_to_hex(buf[i] >> 4);
        resp[i*2+1] = value_to_hex(buf[i]);
    }
    resp[bytes * 2] = '\0';
    
    gdb_send_packet(resp);
}

/**
 * Handle write memory (M)
 */
static void gdb_handle_write_memory(const char* packet) {
    if (!g_gdb_server.vm) {
        gdb_send_error(1);
        return;
    }
    
    const char* p = packet + 1;  /* Skip 'M' */
    uint32_t consumed;
    
    /* Parse address */
    uint32_t addr = parse_hex(p, &consumed);
    p += consumed;
    
    if (*p != ',') {
        gdb_send_error(1);
        return;
    }
    p++;
    
    /* Parse length */
    uint32_t length = parse_hex(p, &consumed);
    p += consumed;
    
    if (*p != ':') {
        gdb_send_error(1);
        return;
    }
    p++;
    
    /* Parse data */
    uint8_t buf[256];
    if (length > 256) length = 256;
    
    for (uint32_t i = 0; i < length; i++) {
        int hi = hex_char_value(*p++);
        int lo = hex_char_value(*p++);
        if (hi < 0 || lo < 0) {
            gdb_send_error(1);
            return;
        }
        buf[i] = (hi << 4) | lo;
    }
    
    if (aurora_vm_write_memory(g_gdb_server.vm, addr, length, buf) < 0) {
        gdb_send_error(1);
        return;
    }
    
    gdb_send_ok();
}

/**
 * Handle continue (c)
 */
static void gdb_handle_continue(const char* packet) {
    (void)packet;
    
    if (!g_gdb_server.vm) {
        gdb_send_error(1);
        return;
    }
    
    g_gdb_server.stopped = false;
    g_gdb_server.stepping = false;
    g_gdb_server.running = true;
    
    /* In a real implementation, would resume VM execution */
}

/**
 * Handle step (s)
 */
static void gdb_handle_step(const char* packet) {
    (void)packet;
    
    if (!g_gdb_server.vm) {
        gdb_send_error(1);
        return;
    }
    
    g_gdb_server.stepping = true;
    
    /* Execute single instruction */
    aurora_vm_step(g_gdb_server.vm);
    
    /* Report stop */
    gdb_send_stop_reply(GDB_SIGNAL_TRAP);
}

/**
 * Handle set breakpoint (Z)
 */
static void gdb_handle_set_breakpoint(const char* packet) {
    const char* p = packet + 1;  /* Skip 'Z' */
    
    int type = *p++ - '0';
    if (*p++ != ',') {
        gdb_send_error(1);
        return;
    }
    
    uint32_t consumed;
    uint32_t addr = parse_hex(p, &consumed);
    p += consumed;
    
    if (*p++ != ',') {
        gdb_send_error(1);
        return;
    }
    
    uint32_t kind = parse_hex(p, &consumed);
    (void)kind;
    
    switch (type) {
        case 0: /* Software breakpoint */
        case 1: /* Hardware breakpoint */
            if (g_gdb_server.vm) {
                if (aurora_vm_debugger_add_breakpoint(g_gdb_server.vm, addr) == 0) {
                    gdb_send_ok();
                } else {
                    gdb_send_error(1);
                }
            }
            break;
        default:
            gdb_send_error(1);
            break;
    }
}

/**
 * Handle remove breakpoint (z)
 */
static void gdb_handle_remove_breakpoint(const char* packet) {
    const char* p = packet + 1;  /* Skip 'z' */
    
    int type = *p++ - '0';
    if (*p++ != ',') {
        gdb_send_error(1);
        return;
    }
    
    uint32_t consumed;
    uint32_t addr = parse_hex(p, &consumed);
    
    switch (type) {
        case 0: /* Software breakpoint */
        case 1: /* Hardware breakpoint */
            if (g_gdb_server.vm) {
                aurora_vm_debugger_remove_breakpoint(g_gdb_server.vm, addr);
                gdb_send_ok();
            }
            break;
        default:
            gdb_send_error(1);
            break;
    }
}

/**
 * Process received packet
 */
static void gdb_process_packet(const char* packet) {
    char cmd = packet[0];
    
    switch (cmd) {
        case '?':  /* Query halt reason */
            gdb_send_stop_reply(g_gdb_server.stop_signal);
            break;
            
        case 'g':  /* Read registers */
            gdb_handle_read_registers();
            break;
            
        case 'G':  /* Write registers */
            gdb_handle_write_registers(packet);
            break;
            
        case 'm':  /* Read memory */
            gdb_handle_read_memory(packet);
            break;
            
        case 'M':  /* Write memory */
            gdb_handle_write_memory(packet);
            break;
            
        case 'c':  /* Continue */
            gdb_handle_continue(packet);
            break;
            
        case 's':  /* Step */
            gdb_handle_step(packet);
            break;
            
        case 'Z':  /* Set breakpoint */
            gdb_handle_set_breakpoint(packet);
            break;
            
        case 'z':  /* Remove breakpoint */
            gdb_handle_remove_breakpoint(packet);
            break;
            
        case 'q':  /* Query */
            gdb_handle_query(packet);
            break;
            
        case 'H':  /* Set thread */
            gdb_send_ok();
            break;
            
        case 'D':  /* Detach */
            gdb_send_ok();
            g_gdb_server.socket.connected = false;
            break;
            
        case 'k':  /* Kill */
            /* Stop VM */
            break;
            
        default:
            gdb_send_packet("");  /* Unsupported command */
            break;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Initialize GDB server
 */
int gdb_server_init(AuroraVM* vm, uint16_t port) {
    if (g_gdb_server.initialized) {
        return 0;
    }
    
    platform_memset(&g_gdb_server, 0, sizeof(gdb_server_t));
    
    g_gdb_server.vm = vm;
    g_gdb_server.stop_signal = GDB_SIGNAL_TRAP;
    
    if (socket_init(&g_gdb_server.socket, port) != 0) {
        return -1;
    }
    
    g_gdb_server.initialized = true;
    
    return 0;
}

/**
 * Start GDB server (wait for connection)
 */
int gdb_server_start(void) {
    if (!g_gdb_server.initialized) {
        return -1;
    }
    
    if (socket_accept(&g_gdb_server.socket) != 0) {
        return -1;
    }
    
    g_gdb_server.stopped = true;
    
    return 0;
}

/**
 * Stop GDB server
 */
void gdb_server_stop(void) {
    if (!g_gdb_server.initialized) {
        return;
    }
    
    socket_close(&g_gdb_server.socket);
    g_gdb_server.running = false;
}

/**
 * Handle GDB server events
 */
int gdb_server_poll(void) {
    if (!g_gdb_server.initialized || !g_gdb_server.socket.connected) {
        return -1;
    }
    
    char buffer[256];
    int len = socket_recv(&g_gdb_server.socket, buffer, sizeof(buffer));
    
    if (len <= 0) {
        return 0;
    }
    
    /* Parse received data */
    for (int i = 0; i < len; i++) {
        char c = buffer[i];
        
        if (c == GDB_INTERRUPT) {
            /* Ctrl+C - stop execution */
            g_gdb_server.stopped = true;
            gdb_send_stop_reply(GDB_SIGNAL_INT);
            continue;
        }
        
        if (c == GDB_ACK || c == GDB_NACK) {
            /* Acknowledgement */
            continue;
        }
        
        if (c == GDB_START) {
            g_gdb_server.parser.in_packet = true;
            g_gdb_server.parser.length = 0;
            continue;
        }
        
        if (c == GDB_END && g_gdb_server.parser.in_packet) {
            /* Packet complete - read checksum */
            continue;
        }
        
        if (g_gdb_server.parser.in_packet) {
            if (g_gdb_server.parser.length < GDB_PACKET_SIZE - 1) {
                g_gdb_server.parser.buffer[g_gdb_server.parser.length++] = c;
            }
        }
    }
    
    /* Process complete packet */
    if (g_gdb_server.parser.length > 0) {
        g_gdb_server.parser.buffer[g_gdb_server.parser.length] = '\0';
        
        /* Send ACK */
        if (!g_gdb_server.no_ack_mode) {
            char ack = GDB_ACK;
            socket_send(&g_gdb_server.socket, &ack, 1);
        }
        
        gdb_process_packet(g_gdb_server.parser.buffer);
        
        g_gdb_server.parser.in_packet = false;
        g_gdb_server.parser.length = 0;
    }
    
    return 0;
}

/**
 * Notify GDB of breakpoint hit
 */
void gdb_server_notify_breakpoint(uint32_t addr) {
    (void)addr;
    
    if (!g_gdb_server.initialized || !g_gdb_server.socket.connected) {
        return;
    }
    
    g_gdb_server.stopped = true;
    g_gdb_server.stop_signal = GDB_SIGNAL_TRAP;
    gdb_send_stop_reply(GDB_SIGNAL_TRAP);
}

/**
 * Check if stopped
 */
bool gdb_server_is_stopped(void) {
    return g_gdb_server.stopped;
}

/**
 * Inject data (for testing)
 */
void gdb_server_inject_data(const char* data, uint32_t length) {
    if (g_socket_rx_len + length <= GDB_PACKET_SIZE) {
        platform_memcpy(g_socket_rx_buffer + g_socket_rx_len, data, length);
        g_socket_rx_len += length;
    }
}

/**
 * Get sent data (for testing)
 */
uint32_t gdb_server_get_sent_data(char* buffer, uint32_t max_len) {
    uint32_t to_copy = (g_socket_tx_len < max_len) ? g_socket_tx_len : max_len;
    platform_memcpy(buffer, g_socket_tx_buffer, to_copy);
    g_socket_tx_len = 0;
    return to_copy;
}

/**
 * Get GDB server version
 */
const char* gdb_server_get_version(void) {
    return "1.0.0-aurora-gdb";
}
