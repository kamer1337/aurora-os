/**
 * Aurora OS - User Management System Implementation
 * 
 * Multi-user support with user accounts, permissions, and home directories
 */

#include "user_manager.h"
#include "quantum_crypto.h"
#include "../memory/memory.h"
#include "../../filesystem/vfs/vfs.h"
#include <stddef.h>

// User database
static user_account_t user_database[MAX_USERS];
static uint32_t user_count = 0;
static uint32_t next_uid = 1000;  // Start UIDs at 1000
static uint8_t initialized = 0;

// Database file path
#define USER_DB_PATH "/etc/users.db"
#define USER_DB_VERSION 1

// Helper functions
static size_t user_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void user_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int user_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void user_memset(void* dest, uint8_t val, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = val;
    }
}

static void user_memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

int user_manager_init(void) {
    if (initialized) {
        return 0;
    }
    
    // Initialize user database
    for (uint32_t i = 0; i < MAX_USERS; i++) {
        user_database[i].uid = 0;
        user_database[i].username[0] = '\0';
        user_database[i].home_directory[0] = '\0';
        user_database[i].permissions = USER_PERM_NONE;
        user_database[i].is_active = 0;
        user_database[i].is_admin = 0;
        user_database[i].last_login = 0;
        user_database[i].created_time = 0;
    }
    
    user_count = 0;
    
    // Try to load existing database
    if (user_load_database() != 0) {
        // Create default admin user if no database exists
        uint32_t admin_uid;
        if (user_create("admin", "admin123", 1, &admin_uid) == 0) {
            // Set all permissions for admin
            user_set_permissions(admin_uid, USER_PERM_ALL);
        }
        
        // Create default user account
        uint32_t user_uid;
        if (user_create("user", "user123", 0, &user_uid) == 0) {
            // Set basic permissions for regular user
            user_set_permissions(user_uid, USER_PERM_READ | USER_PERM_WRITE | USER_PERM_EXECUTE);
        }
        
        // Save initial database
        user_save_database();
    }
    
    initialized = 1;
    return 0;
}

void user_manager_shutdown(void) {
    if (initialized) {
        user_save_database();
        initialized = 0;
    }
}

int user_create(const char* username, const char* password, uint8_t is_admin, uint32_t* out_uid) {
    if (!username || !password || user_count >= MAX_USERS) {
        return -1;
    }
    
    // Check if username already exists
    if (user_get_by_username(username) != NULL) {
        return -1;  // Username already taken
    }
    
    // Find free slot
    uint32_t slot = 0;
    for (slot = 0; slot < MAX_USERS; slot++) {
        if (user_database[slot].uid == 0) {
            break;
        }
    }
    
    if (slot >= MAX_USERS) {
        return -1;
    }
    
    // Create new user
    user_account_t* user = &user_database[slot];
    user->uid = next_uid++;
    user_strcpy(user->username, username, MAX_USERNAME_LENGTH);
    
    // Hash password
    quantum_hash_password(password, user->password_hash, MAX_PASSWORD_HASH_LENGTH);
    
    // Set home directory
    char home_path[MAX_HOME_PATH_LENGTH];
    home_path[0] = '/';
    home_path[1] = 'h';
    home_path[2] = 'o';
    home_path[3] = 'm';
    home_path[4] = 'e';
    home_path[5] = '/';
    user_strcpy(home_path + 6, username, MAX_HOME_PATH_LENGTH - 6);
    user_strcpy(user->home_directory, home_path, MAX_HOME_PATH_LENGTH);
    
    // Set flags
    user->is_admin = is_admin;
    user->is_active = 1;
    user->permissions = is_admin ? USER_PERM_ALL : (USER_PERM_READ | USER_PERM_WRITE | USER_PERM_EXECUTE);
    user->created_time = 0;  // Would be set to current time in real implementation
    user->last_login = 0;
    
    user_count++;
    
    // Create home directory
    user_create_home_directory(user->uid);
    
    if (out_uid) {
        *out_uid = user->uid;
    }
    
    return 0;
}

int user_delete(uint32_t uid) {
    user_account_t* user = user_get_by_uid(uid);
    if (!user) {
        return -1;
    }
    
    // Clear user data
    user->uid = 0;
    user->username[0] = '\0';
    user->home_directory[0] = '\0';
    user->is_active = 0;
    user->permissions = USER_PERM_NONE;
    user_memset(user->password_hash, 0, MAX_PASSWORD_HASH_LENGTH);
    
    user_count--;
    return 0;
}

int user_authenticate(const char* username, const char* password, uint32_t* out_uid) {
    if (!username || !password) {
        return -1;
    }
    
    user_account_t* user = user_get_by_username(username);
    if (!user || !user->is_active) {
        return -1;
    }
    
    // Verify password using quantum cryptography
    if (quantum_verify_password(password, user->password_hash, MAX_PASSWORD_HASH_LENGTH) != 0) {
        return -1;
    }
    
    // Update last login time
    user->last_login = 0;  // Would be set to current time in real implementation
    
    if (out_uid) {
        *out_uid = user->uid;
    }
    
    return 0;
}

user_account_t* user_get_by_uid(uint32_t uid) {
    for (uint32_t i = 0; i < MAX_USERS; i++) {
        if (user_database[i].uid == uid && user_database[i].is_active) {
            return &user_database[i];
        }
    }
    return NULL;
}

user_account_t* user_get_by_username(const char* username) {
    if (!username) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < MAX_USERS; i++) {
        if (user_database[i].uid != 0 && 
            user_database[i].is_active &&
            user_strcmp(user_database[i].username, username) == 0) {
            return &user_database[i];
        }
    }
    return NULL;
}

int user_has_permission(uint32_t uid, user_permission_t permission) {
    user_account_t* user = user_get_by_uid(uid);
    if (!user) {
        return 0;
    }
    
    // Admins have all permissions
    if (user->is_admin) {
        return 1;
    }
    
    return (user->permissions & permission) != 0;
}

int user_set_permissions(uint32_t uid, uint32_t permissions) {
    user_account_t* user = user_get_by_uid(uid);
    if (!user) {
        return -1;
    }
    
    user->permissions = permissions;
    return 0;
}

const char* user_get_home_directory(uint32_t uid) {
    user_account_t* user = user_get_by_uid(uid);
    if (!user) {
        return NULL;
    }
    
    return user->home_directory;
}

int user_create_home_directory(uint32_t uid) {
    user_account_t* user = user_get_by_uid(uid);
    if (!user) {
        return -1;
    }
    
    // Create home directory using VFS
    if (vfs_mkdir(user->home_directory) != 0) {
        // Directory might already exist, which is OK
    }
    
    return 0;
}

int user_change_password(uint32_t uid, const char* old_password, const char* new_password) {
    if (!old_password || !new_password) {
        return -1;
    }
    
    user_account_t* user = user_get_by_uid(uid);
    if (!user) {
        return -1;
    }
    
    // Verify old password
    if (quantum_verify_password(old_password, user->password_hash, MAX_PASSWORD_HASH_LENGTH) != 0) {
        return -1;  // Old password incorrect
    }
    
    // Hash new password
    quantum_hash_password(new_password, user->password_hash, MAX_PASSWORD_HASH_LENGTH);
    
    return 0;
}

int user_list_all(user_account_t** out_users, uint32_t max_users) {
    if (!out_users || max_users == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_USERS && count < max_users; i++) {
        if (user_database[i].uid != 0 && user_database[i].is_active) {
            out_users[count++] = &user_database[i];
        }
    }
    
    return count;
}

int user_save_database(void) {
    // Create database file
    int fd = vfs_create(USER_DB_PATH);
    if (fd < 0) {
        // Try to open if it already exists
        fd = vfs_open(USER_DB_PATH, O_WRONLY | O_TRUNC);
        if (fd < 0) {
            return -1;
        }
    }
    
    // Write version
    uint32_t version = USER_DB_VERSION;
    if (vfs_write(fd, &version, sizeof(version)) != sizeof(version)) {
        vfs_close(fd);
        return -1;
    }
    
    // Write user count
    if (vfs_write(fd, &user_count, sizeof(user_count)) != sizeof(user_count)) {
        vfs_close(fd);
        return -1;
    }
    
    // Write each active user
    for (uint32_t i = 0; i < MAX_USERS; i++) {
        if (user_database[i].uid != 0 && user_database[i].is_active) {
            if (vfs_write(fd, &user_database[i], sizeof(user_account_t)) != sizeof(user_account_t)) {
                vfs_close(fd);
                return -1;
            }
        }
    }
    
    vfs_close(fd);
    return 0;
}

int user_load_database(void) {
    // Open database file
    int fd = vfs_open(USER_DB_PATH, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Read version
    uint32_t version;
    if (vfs_read(fd, &version, sizeof(version)) != sizeof(version)) {
        vfs_close(fd);
        return -1;
    }
    
    if (version != USER_DB_VERSION) {
        vfs_close(fd);
        return -1;  // Unsupported version
    }
    
    // Read user count
    uint32_t loaded_count;
    if (vfs_read(fd, &loaded_count, sizeof(loaded_count)) != sizeof(loaded_count)) {
        vfs_close(fd);
        return -1;
    }
    
    // Read users
    user_count = 0;
    uint32_t max_uid = 1000;
    for (uint32_t i = 0; i < loaded_count && i < MAX_USERS; i++) {
        uint32_t slot = 0;
        for (slot = 0; slot < MAX_USERS; slot++) {
            if (user_database[slot].uid == 0) {
                break;
            }
        }
        
        if (slot >= MAX_USERS) {
            break;
        }
        
        if (vfs_read(fd, &user_database[slot], sizeof(user_account_t)) == sizeof(user_account_t)) {
            user_count++;
            if (user_database[slot].uid >= max_uid) {
                max_uid = user_database[slot].uid + 1;
            }
        }
    }
    
    next_uid = max_uid;
    vfs_close(fd);
    return 0;
}
