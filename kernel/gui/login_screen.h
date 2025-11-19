/**
 * Aurora OS - Login Screen
 * 
 * User authentication and guest mode login screen
 */

#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include <stdint.h>
#include "gui.h"

// Login result codes
#define LOGIN_SUCCESS 0
#define LOGIN_FAILED -1
#define LOGIN_GUEST -2

// User session structure
typedef struct {
    char username[64];
    uint8_t is_guest;
    uint8_t is_admin;
    uint32_t session_id;
} user_session_t;

/**
 * Initialize the login screen system
 * @return 0 on success, -1 on failure
 */
int login_screen_init(void);

/**
 * Show the login screen
 * @return User session on successful login, NULL on failure
 */
user_session_t* login_screen_show(void);

/**
 * Process login attempt
 * @param username Username to authenticate
 * @param password Password to verify
 * @return LOGIN_SUCCESS, LOGIN_FAILED, or error code
 */
int login_screen_authenticate(const char* username, const char* password);

/**
 * Start a guest session
 * @return User session for guest, NULL on failure
 */
user_session_t* login_screen_guest_login(void);

/**
 * End current user session
 * @param session Session to terminate
 */
void login_screen_logout(user_session_t* session);

/**
 * Get current user session
 * @return Current session or NULL if not logged in
 */
user_session_t* login_screen_get_current_session(void);

#endif /* LOGIN_SCREEN_H */
