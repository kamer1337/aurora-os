/**
 * Aurora OS - User Management System
 * 
 * Multi-user support with user accounts, permissions, and home directories
 */

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <stdint.h>

#define MAX_USERNAME_LENGTH 64
#define MAX_PASSWORD_HASH_LENGTH 32
#define MAX_HOME_PATH_LENGTH 256
#define MAX_USERS 32

/**
 * User permissions flags
 */
typedef enum {
    USER_PERM_NONE = 0x00,
    USER_PERM_READ = 0x01,
    USER_PERM_WRITE = 0x02,
    USER_PERM_EXECUTE = 0x04,
    USER_PERM_ADMIN = 0x08,
    USER_PERM_NETWORK = 0x10,
    USER_PERM_USB = 0x20,
    USER_PERM_SYSTEM = 0x40,
    USER_PERM_ALL = 0xFF
} user_permission_t;

/**
 * User account structure
 */
typedef struct {
    uint32_t uid;                               // User ID
    char username[MAX_USERNAME_LENGTH];         // Username
    uint8_t password_hash[MAX_PASSWORD_HASH_LENGTH]; // Hashed password
    char home_directory[MAX_HOME_PATH_LENGTH];  // Home directory path
    uint32_t permissions;                       // Permission flags
    uint8_t is_active;                          // Account active flag
    uint8_t is_admin;                           // Administrator flag
    uint64_t last_login;                        // Last login timestamp
    uint64_t created_time;                      // Account creation timestamp
} user_account_t;

/**
 * Initialize user management system
 * @return 0 on success, -1 on failure
 */
int user_manager_init(void);

/**
 * Shutdown user management system
 */
void user_manager_shutdown(void);

/**
 * Create a new user account
 * @param username Username
 * @param password Plain text password (will be hashed)
 * @param is_admin Administrator flag
 * @param out_uid Output user ID
 * @return 0 on success, -1 on failure
 */
int user_create(const char* username, const char* password, uint8_t is_admin, uint32_t* out_uid);

/**
 * Delete a user account
 * @param uid User ID
 * @return 0 on success, -1 on failure
 */
int user_delete(uint32_t uid);

/**
 * Authenticate a user
 * @param username Username
 * @param password Plain text password
 * @param out_uid Output user ID on success
 * @return 0 on success, -1 on failure
 */
int user_authenticate(const char* username, const char* password, uint32_t* out_uid);

/**
 * Get user account by UID
 * @param uid User ID
 * @return Pointer to user account or NULL if not found
 */
user_account_t* user_get_by_uid(uint32_t uid);

/**
 * Get user account by username
 * @param username Username
 * @return Pointer to user account or NULL if not found
 */
user_account_t* user_get_by_username(const char* username);

/**
 * Check if user has permission
 * @param uid User ID
 * @param permission Permission to check
 * @return 1 if user has permission, 0 otherwise
 */
int user_has_permission(uint32_t uid, user_permission_t permission);

/**
 * Set user permissions
 * @param uid User ID
 * @param permissions Permission flags
 * @return 0 on success, -1 on failure
 */
int user_set_permissions(uint32_t uid, uint32_t permissions);

/**
 * Get user home directory
 * @param uid User ID
 * @return Home directory path or NULL if user not found
 */
const char* user_get_home_directory(uint32_t uid);

/**
 * Create user home directory
 * @param uid User ID
 * @return 0 on success, -1 on failure
 */
int user_create_home_directory(uint32_t uid);

/**
 * Change user password
 * @param uid User ID
 * @param old_password Old password for verification
 * @param new_password New password
 * @return 0 on success, -1 on failure
 */
int user_change_password(uint32_t uid, const char* old_password, const char* new_password);

/**
 * List all users
 * @param out_users Array to store user pointers
 * @param max_users Maximum number of users to return
 * @return Number of users returned
 */
int user_list_all(user_account_t** out_users, uint32_t max_users);

/**
 * Save user database to persistent storage
 * @return 0 on success, -1 on failure
 */
int user_save_database(void);

/**
 * Load user database from persistent storage
 * @return 0 on success, -1 on failure
 */
int user_load_database(void);

#endif // USER_MANAGER_H
