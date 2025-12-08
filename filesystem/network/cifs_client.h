/**
 * Aurora OS - CIFS/SMB Client Implementation
 * 
 * Common Internet File System (SMB) client for Windows network shares
 */

#ifndef AURORA_CIFS_CLIENT_H
#define AURORA_CIFS_CLIENT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../vfs/vfs.h"

/* SMB protocol versions */
#define SMB_VERSION_1       0x0001
#define SMB_VERSION_2       0x0002
#define SMB_VERSION_3       0x0003

/* SMB port */
#define SMB_PORT            445
#define SMB_NETBIOS_PORT    139

/* SMB commands (SMB1) */
#define SMB_COM_NEGOTIATE   0x72
#define SMB_COM_SESSION_SETUP 0x73
#define SMB_COM_TREE_CONNECT 0x75
#define SMB_COM_OPEN        0x2D
#define SMB_COM_CLOSE       0x04
#define SMB_COM_READ        0x2E
#define SMB_COM_WRITE       0x2F
#define SMB_COM_CREATE_DIR  0x00
#define SMB_COM_DELETE_DIR  0x01
#define SMB_COM_DELETE      0x06

/* SMB2 commands */
#define SMB2_NEGOTIATE      0x0000
#define SMB2_SESSION_SETUP  0x0001
#define SMB2_LOGOFF         0x0002
#define SMB2_TREE_CONNECT   0x0003
#define SMB2_TREE_DISCONNECT 0x0004
#define SMB2_CREATE         0x0005
#define SMB2_CLOSE          0x0006
#define SMB2_FLUSH          0x0007
#define SMB2_READ           0x0008
#define SMB2_WRITE          0x0009
#define SMB2_QUERY_DIRECTORY 0x000E
#define SMB2_CHANGE_NOTIFY  0x000F
#define SMB2_QUERY_INFO     0x0010
#define SMB2_SET_INFO       0x0011

/* SMB status codes */
#define SMB_STATUS_SUCCESS              0x00000000
#define SMB_STATUS_MORE_PROCESSING      0xC0000016
#define SMB_STATUS_ACCESS_DENIED        0xC0000022
#define SMB_STATUS_OBJECT_NAME_NOT_FOUND 0xC0000034
#define SMB_STATUS_LOGON_FAILURE        0xC000006D

/* SMB header */
typedef struct __attribute__((packed)) {
    uint32_t protocol;      /* 0xFF 'SMB' */
    uint8_t  command;
    uint32_t status;
    uint8_t  flags;
    uint16_t flags2;
    uint16_t pid_high;
    uint64_t security;
    uint16_t reserved;
    uint16_t tid;
    uint16_t pid;
    uint16_t uid;
    uint16_t mid;
} smb1_header_t;

/* SMB2 header */
typedef struct __attribute__((packed)) {
    uint32_t protocol;      /* 0xFE 'SMB' */
    uint16_t structure_size;
    uint16_t credit_charge;
    uint32_t status;
    uint16_t command;
    uint16_t credit_request;
    uint32_t flags;
    uint32_t next_command;
    uint64_t message_id;
    uint32_t reserved;
    uint32_t tree_id;
    uint64_t session_id;
    uint8_t  signature[16];
} smb2_header_t;

/* File attributes */
#define SMB_ATTR_READONLY       0x0001
#define SMB_ATTR_HIDDEN         0x0002
#define SMB_ATTR_SYSTEM         0x0004
#define SMB_ATTR_DIRECTORY      0x0010
#define SMB_ATTR_ARCHIVE        0x0020
#define SMB_ATTR_NORMAL         0x0080

/* CIFS connection info */
typedef struct {
    char server[256];
    char share[256];
    char username[256];
    char password[256];
    uint32_t server_ip;
    uint16_t port;
    uint32_t version;
    uint32_t session_id;
    uint32_t tree_id;
    uint64_t message_id;
    int socket;
    bool connected;
    bool authenticated;
} cifs_conn_t;

/* File handle */
typedef struct {
    uint64_t file_id_persistent;
    uint64_t file_id_volatile;
} cifs_fhandle_t;

/* CIFS initialization */
void cifs_client_init(void);
int cifs_connect(const char* server, const char* share, 
                 const char* username, const char* password);
int cifs_disconnect(void);

/* Get CIFS file system operations */
fs_ops_t* cifs_get_ops(void);

/* CIFS protocol functions */
int cifs_negotiate(cifs_conn_t* conn);
int cifs_session_setup(cifs_conn_t* conn, const char* username, const char* password);
int cifs_tree_connect(cifs_conn_t* conn, const char* share);

/* CIFS operations */
int cifs_open_file(cifs_conn_t* conn, const char* path, uint32_t access, 
                   cifs_fhandle_t* fh);
int cifs_close_file(cifs_conn_t* conn, const cifs_fhandle_t* fh);
int cifs_read_file(cifs_conn_t* conn, const cifs_fhandle_t* fh,
                   uint64_t offset, uint32_t length, void* buffer);
int cifs_write_file(cifs_conn_t* conn, const cifs_fhandle_t* fh,
                    uint64_t offset, uint32_t length, const void* buffer);
int cifs_create_file(cifs_conn_t* conn, const char* path, uint32_t attributes);
int cifs_delete_file(cifs_conn_t* conn, const char* path);
int cifs_query_directory(cifs_conn_t* conn, const cifs_fhandle_t* fh,
                         void* entries, uint32_t count);

#endif /* AURORA_CIFS_CLIENT_H */
