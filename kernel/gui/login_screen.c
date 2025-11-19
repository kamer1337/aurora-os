/**
 * Aurora OS - Login Screen Implementation
 * 
 * User authentication and guest mode login screen
 */

#include "login_screen.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../security/quantum_crypto.h"
#include <stddef.h>

// Current user session
static user_session_t* current_session = NULL;
static uint32_t next_session_id = 1;

// Login screen UI elements
static window_t* login_window = NULL;
static widget_t* username_input = NULL;
static widget_t* password_input = NULL;
static widget_t* login_button = NULL;
static widget_t* guest_button = NULL;
static widget_t* status_label = NULL;

// Simple string utilities
static size_t login_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void login_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int login_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Default users (in production, this would be from a secure database)
typedef struct {
    char username[64];
    uint8_t password_hash[32];  // Quantum hash (16 bytes salt + 16 bytes hash)
    uint8_t is_admin;
} user_account_t;

// Pre-computed quantum hashes for default passwords
// In production, these would be generated during user setup
static user_account_t default_users[3];
static uint8_t users_initialized = 0;

int login_screen_init(void) {
    // Initialize login system
    current_session = NULL;
    
    // Initialize default users with hashed passwords (only once)
    if (!users_initialized) {
        // User 0: admin with password "admin123"
        login_strcpy(default_users[0].username, "admin", 64);
        default_users[0].is_admin = 1;
        quantum_hash_password("admin123", default_users[0].password_hash, 32);
        
        // User 1: user with password "user123"
        login_strcpy(default_users[1].username, "user", 64);
        default_users[1].is_admin = 0;
        quantum_hash_password("user123", default_users[1].password_hash, 32);
        
        // User 2: sentinel (empty username)
        default_users[2].username[0] = '\0';
        default_users[2].is_admin = 0;
        
        users_initialized = 1;
    }
    
    return 0;
}

int login_screen_authenticate(const char* username, const char* password) {
    if (!username || !password) {
        return LOGIN_FAILED;
    }

    // Check against default users using quantum password verification
    for (int i = 0; i < 2; i++) {  // Only check actual users, not sentinel
        if (default_users[i].username[0] == '\0') {
            break;
        }
        
        if (login_strcmp(username, default_users[i].username) == 0) {
            // Verify password using quantum cryptography
            int verify_result = quantum_verify_password(password, 
                                                       default_users[i].password_hash, 
                                                       32);
            
            if (verify_result == QCRYPTO_SUCCESS) {
                // Create session
                if (current_session) {
                    kfree(current_session);
                }
                
                current_session = (user_session_t*)kmalloc(sizeof(user_session_t));
                if (!current_session) {
                    return LOGIN_FAILED;
                }
                
                login_strcpy(current_session->username, username, 64);
                current_session->is_guest = 0;
                current_session->is_admin = default_users[i].is_admin;
                current_session->session_id = next_session_id++;
                
                return LOGIN_SUCCESS;
            }
        }
    }
    
    return LOGIN_FAILED;
}

user_session_t* login_screen_guest_login(void) {
    // Create guest session
    if (current_session) {
        kfree(current_session);
    }
    
    current_session = (user_session_t*)kmalloc(sizeof(user_session_t));
    if (!current_session) {
        return NULL;
    }
    
    login_strcpy(current_session->username, "Guest", 64);
    current_session->is_guest = 1;
    current_session->is_admin = 0;
    current_session->session_id = next_session_id++;
    
    return current_session;
}

void login_screen_logout(user_session_t* session) {
    if (session) {
        if (session == current_session) {
            current_session = NULL;
        }
        kfree(session);
    }
}

user_session_t* login_screen_get_current_session(void) {
    return current_session;
}

// Button click handlers
static void on_login_button_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    // In a real implementation, get text from input widgets
    // For now, use default credentials
    const char* username = "user";
    const char* password = "user123";
    
    int result = login_screen_authenticate(username, password);
    
    if (result == LOGIN_SUCCESS) {
        // Close login window and proceed to desktop
        if (login_window) {
            gui_close_window(login_window);
            login_window = NULL;
        }
    }
}

static void on_guest_button_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    user_session_t* session = login_screen_guest_login();
    
    if (session) {
        // Close login window and proceed to desktop
        if (login_window) {
            gui_close_window(login_window);
            login_window = NULL;
        }
    }
}

user_session_t* login_screen_show(void) {
    // Get screen dimensions
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info) {
        return NULL;
    }
    
    uint32_t screen_width = fb_info->width;
    uint32_t screen_height = fb_info->height;
    
    // Create login window (centered)
    uint32_t window_width = 400;
    uint32_t window_height = 300;
    int32_t window_x = (screen_width - window_width) / 2;
    int32_t window_y = (screen_height - window_height) / 2;
    
    login_window = gui_create_window("Aurora OS - Login", window_x, window_y, 
                                     window_width, window_height);
    
    if (!login_window) {
        return NULL;
    }
    
    // Set window background color
    login_window->bg_color = (color_t){240, 240, 245, 255};
    
    // Create title label
    widget_t* title_label = gui_create_label(login_window, "Welcome to Aurora OS", 
                                             50, 30);
    if (title_label) {
        title_label->fg_color = (color_t){40, 40, 60, 255};
    }
    
    // Create username label
    widget_t* username_label = gui_create_label(login_window, "Username:", 
                                                50, 80);
    if (username_label) {
        username_label->fg_color = (color_t){60, 60, 80, 255};
    }
    
    // Create username input (placeholder)
    username_input = gui_create_button(login_window, "user", 
                                       160, 80, 180, 30);
    if (username_input) {
        username_input->bg_color = (color_t){255, 255, 255, 255};
        username_input->fg_color = (color_t){40, 40, 60, 255};
    }
    
    // Create password label
    widget_t* password_label = gui_create_label(login_window, "Password:", 
                                                50, 130);
    if (password_label) {
        password_label->fg_color = (color_t){60, 60, 80, 255};
    }
    
    // Create password input (placeholder)
    password_input = gui_create_button(login_window, "********", 
                                       160, 130, 180, 30);
    if (password_input) {
        password_input->bg_color = (color_t){255, 255, 255, 255};
        password_input->fg_color = (color_t){40, 40, 60, 255};
    }
    
    // Create login button
    login_button = gui_create_button(login_window, "Login", 
                                     50, 190, 140, 35);
    if (login_button) {
        login_button->bg_color = (color_t){70, 130, 220, 255};
        login_button->fg_color = (color_t){255, 255, 255, 255};
        login_button->on_click = on_login_button_click;
    }
    
    // Create guest button
    guest_button = gui_create_button(login_window, "Guest Login", 
                                     210, 190, 140, 35);
    if (guest_button) {
        guest_button->bg_color = (color_t){100, 180, 100, 255};
        guest_button->fg_color = (color_t){255, 255, 255, 255};
        guest_button->on_click = on_guest_button_click;
    }
    
    // Create status label
    status_label = gui_create_label(login_window, "Enter credentials or login as guest", 
                                    50, 240);
    if (status_label) {
        status_label->fg_color = (color_t){100, 100, 120, 255};
    }
    
    // Show the window
    gui_show_window(login_window);
    gui_focus_window(login_window);
    
    // In a real implementation, this would run an event loop
    // and return after successful login
    // For now, return the current session
    return current_session;
}
