/**
 * Aurora OS - LDAP/Active Directory Integration Header
 * 
 * Provides enterprise authentication and directory services integration
 */

#ifndef LDAP_AD_H
#define LDAP_AD_H

#include <stdint.h>
#include <stddef.h>

/* LDAP protocol version */
#define LDAP_VERSION_2  2
#define LDAP_VERSION_3  3

/* LDAP default ports */
#define LDAP_PORT       389
#define LDAPS_PORT      636   /* LDAP over SSL/TLS */
#define LDAP_GC_PORT    3268  /* Active Directory Global Catalog */
#define LDAPS_GC_PORT   3269  /* Global Catalog over SSL */

/* Authentication methods */
typedef enum {
    LDAP_AUTH_SIMPLE = 0,      /* Simple bind (username/password) */
    LDAP_AUTH_SASL = 1,        /* SASL authentication */
    LDAP_AUTH_KERBEROS = 2,    /* Kerberos (for AD) */
    LDAP_AUTH_GSSAPI = 3,      /* GSSAPI */
    LDAP_AUTH_NTLM = 4         /* NTLM (for AD) */
} ldap_auth_method_t;

/* LDAP operation result codes */
#define LDAP_SUCCESS                    0
#define LDAP_OPERATIONS_ERROR           1
#define LDAP_PROTOCOL_ERROR             2
#define LDAP_TIME_LIMIT_EXCEEDED        3
#define LDAP_SIZE_LIMIT_EXCEEDED        4
#define LDAP_AUTH_METHOD_NOT_SUPPORTED  7
#define LDAP_STRONG_AUTH_REQUIRED       8
#define LDAP_NO_SUCH_ATTRIBUTE          16
#define LDAP_UNDEFINED_TYPE             17
#define LDAP_INAPPROPRIATE_MATCHING     18
#define LDAP_CONSTRAINT_VIOLATION       19
#define LDAP_TYPE_OR_VALUE_EXISTS       20
#define LDAP_INVALID_SYNTAX             21
#define LDAP_NO_SUCH_OBJECT             32
#define LDAP_ALIAS_PROBLEM              33
#define LDAP_INVALID_DN_SYNTAX          34
#define LDAP_ALIAS_DEREFERENCING_PROBLEM 36
#define LDAP_INAPPROPRIATE_AUTH         48
#define LDAP_INVALID_CREDENTIALS        49
#define LDAP_INSUFFICIENT_ACCESS        50
#define LDAP_BUSY                       51
#define LDAP_UNAVAILABLE                52
#define LDAP_UNWILLING_TO_PERFORM       53

/* LDAP search scope */
typedef enum {
    LDAP_SCOPE_BASE = 0,       /* Search only the base DN */
    LDAP_SCOPE_ONELEVEL = 1,   /* Search immediate children */
    LDAP_SCOPE_SUBTREE = 2     /* Search entire subtree */
} ldap_search_scope_t;

/* LDAP connection structure */
typedef struct {
    char server[256];          /* LDAP server hostname or IP */
    uint16_t port;             /* Server port */
    int use_ssl;               /* Use SSL/TLS */
    int protocol_version;      /* LDAP protocol version */
    int connected;             /* Connection status */
    int authenticated;         /* Authentication status */
    char bind_dn[512];         /* Bind DN for authentication */
    char base_dn[512];         /* Base DN for searches */
    uint32_t timeout_ms;       /* Operation timeout */
    int socket_fd;             /* Network socket */
} ldap_connection_t;

/* LDAP attribute structure */
typedef struct {
    char name[128];
    char** values;
    uint32_t value_count;
} ldap_attribute_t;

/* LDAP entry structure */
typedef struct {
    char dn[512];              /* Distinguished Name */
    ldap_attribute_t* attributes;
    uint32_t attribute_count;
} ldap_entry_t;

/* Active Directory specific structures */
typedef struct {
    char domain[256];          /* AD domain name */
    char forest[256];          /* AD forest name */
    char site[256];            /* AD site */
    char dc_name[256];         /* Domain Controller name */
    uint32_t domain_functional_level;
    uint32_t forest_functional_level;
} ad_domain_info_t;

/* User account structure */
typedef struct {
    char username[256];
    char display_name[256];
    char email[256];
    char home_directory[512];
    char profile_path[512];
    uint32_t uid;              /* User ID */
    uint32_t primary_gid;      /* Primary group ID */
    char** groups;             /* Group memberships */
    uint32_t group_count;
    int account_enabled;
    int password_expired;
    uint64_t last_logon;
    uint64_t password_last_set;
} ldap_user_info_t;

/* Group structure */
typedef struct {
    char name[256];
    char description[512];
    uint32_t gid;              /* Group ID */
    char** members;            /* Group member DNs */
    uint32_t member_count;
    int is_security_group;
} ldap_group_info_t;

/* Core LDAP functions */
int ldap_init(void);
int ldap_connect(ldap_connection_t* conn, const char* server, uint16_t port, int use_ssl);
int ldap_disconnect(ldap_connection_t* conn);
int ldap_bind(ldap_connection_t* conn, const char* bind_dn, const char* password, 
              ldap_auth_method_t auth_method);
int ldap_unbind(ldap_connection_t* conn);

/* Search operations */
int ldap_search(ldap_connection_t* conn, const char* base_dn, ldap_search_scope_t scope,
                const char* filter, char** attributes, ldap_entry_t** results, uint32_t* result_count);
int ldap_free_results(ldap_entry_t* results, uint32_t count);

/* Authentication */
int ldap_authenticate_user(ldap_connection_t* conn, const char* username, const char* password);
int ldap_get_user_info(ldap_connection_t* conn, const char* username, ldap_user_info_t* user_info);
int ldap_get_group_info(ldap_connection_t* conn, const char* group_name, ldap_group_info_t* group_info);
int ldap_check_group_membership(ldap_connection_t* conn, const char* username, const char* group_name);

/* Active Directory specific functions */
int ad_connect(ldap_connection_t* conn, const char* domain);
int ad_get_domain_info(ldap_connection_t* conn, ad_domain_info_t* domain_info);
int ad_locate_domain_controller(const char* domain, char* dc_name, size_t dc_name_len);
int ad_authenticate_kerberos(ldap_connection_t* conn, const char* principal);
int ad_get_user_groups(ldap_connection_t* conn, const char* username, 
                       char*** groups, uint32_t* group_count);

/* Configuration */
int ldap_set_timeout(ldap_connection_t* conn, uint32_t timeout_ms);
int ldap_enable_referral_chasing(ldap_connection_t* conn, int enable);
int ldap_set_size_limit(ldap_connection_t* conn, uint32_t limit);

#endif /* LDAP_AD_H */
