/**
 * @file notification_system.c
 * @brief Modern Notification System Implementation
 */

#include "notification_system.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../core/kernel.h"

/* Global notification system */
static notification_system_t g_notif_system = {0};

/* Helper functions */
static void notification_free(notification_t* notif);
static void notification_remove(uint32_t id);
static notification_t* notification_find(uint32_t id);
static void notification_reposition_all(void);
static uint32_t notification_get_color(notification_type_t type);
static void notification_draw(notification_t* notif);

/* Simple memory management for freestanding environment */
static void* notif_malloc(size_t size) {
    return kmalloc(size);
}

static void notif_free(void* ptr) {
    kfree(ptr);
}

static void notif_memset(void* ptr, int value, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
}

static void notif_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int notification_system_init(void) {
    notif_memset(&g_notif_system, 0, sizeof(notification_system_t));
    
    /* Configuration */
    g_notif_system.notification_x = 1700;           /* Right side of 1920px screen */
    g_notif_system.notification_y = 50;             /* Top margin */
    g_notif_system.notification_width = 350;
    g_notif_system.notification_spacing = 10;
    g_notif_system.max_visible = 5;
    g_notif_system.default_timeout_ms = 5000;       /* 5 seconds */
    g_notif_system.animation_duration_ms = 300;     /* 300ms animation */
    g_notif_system.next_id = 1;
    
    return 0;
}

void notification_system_shutdown(void) {
    notification_t* current = g_notif_system.notifications;
    while (current) {
        notification_t* next = current->next;
        notification_free(current);
        current = next;
    }
    g_notif_system.notifications = NULL;
    g_notif_system.notification_count = 0;
}

uint32_t notification_show(
    const char* title,
    const char* message,
    notification_type_t type,
    notification_priority_t priority,
    uint32_t timeout_ms
) {
    /* Allocate notification */
    notification_t* notif = (notification_t*)notif_malloc(sizeof(notification_t));
    if (!notif) return 0;
    
    notif_memset(notif, 0, sizeof(notification_t));
    
    /* Set properties */
    notif->id = g_notif_system.next_id++;
    notif_strcpy(notif->title, title, sizeof(notif->title));
    notif_strcpy(notif->message, message, sizeof(notif->message));
    notif->type = type;
    notif->priority = priority;
    notif->timeout_ms = timeout_ms;
    notif->icon = notification_get_color(type);
    
    /* Get current time (stub - would use real timer) */
    notif->created_time = 0;  /* Would use get_system_ticks() */
    notif->expire_time = timeout_ms > 0 ? notif->created_time + timeout_ms : 0;
    
    /* Visual properties */
    notif->width = g_notif_system.notification_width;
    notif->height = 80;
    notif->opacity = 0.0f;
    notif->slide_offset = notif->width;  /* Start off-screen to the right */
    notif->visible = true;
    notif->animating_in = true;
    notif->animating_out = false;
    
    /* Add to list */
    notif->next = g_notif_system.notifications;
    g_notif_system.notifications = notif;
    g_notif_system.notification_count++;
    
    /* Reposition all notifications */
    notification_reposition_all();
    
    return notif->id;
}

uint32_t notification_show_with_action(
    const char* title,
    const char* message,
    notification_type_t type,
    const char* action_text,
    notification_action_t action_callback,
    void* user_data
) {
    uint32_t id = notification_show(title, message, type, 
                                    NOTIFICATION_PRIORITY_NORMAL, 0);
    if (id == 0) return 0;
    
    notification_t* notif = notification_find(id);
    if (notif) {
        notif_strcpy(notif->action_text, action_text, sizeof(notif->action_text));
        notif->action_callback = action_callback;
        notif->action_user_data = user_data;
    }
    
    return id;
}

void notification_dismiss(uint32_t id) {
    notification_t* notif = notification_find(id);
    if (notif && !notif->animating_out) {
        notif->animating_out = true;
        notif->animating_in = false;
    }
}

void notification_dismiss_all(void) {
    notification_t* current = g_notif_system.notifications;
    while (current) {
        if (!current->animating_out) {
            current->animating_out = true;
            current->animating_in = false;
        }
        current = current->next;
    }
}

void notification_system_update(uint32_t delta_ms) {
    uint64_t current_time = 0;  /* Would use get_system_ticks() */
    
    notification_t** prev = &g_notif_system.notifications;
    notification_t* current = g_notif_system.notifications;
    
    while (current) {
        notification_t* next = current->next;
        
        /* Check timeout */
        if (current->timeout_ms > 0 && current->expire_time > 0) {
            if (current_time >= current->expire_time && !current->animating_out) {
                current->animating_out = true;
                current->animating_in = false;
            }
        }
        
        /* Update animation */
        float animation_progress = (float)delta_ms / (float)g_notif_system.animation_duration_ms;
        
        if (current->animating_in) {
            current->opacity += animation_progress * 2.0f;
            current->slide_offset -= animation_progress * current->width;
            
            if (current->opacity >= 1.0f) {
                current->opacity = 1.0f;
                current->slide_offset = 0.0f;
                current->animating_in = false;
            }
        } else if (current->animating_out) {
            current->opacity -= animation_progress * 2.0f;
            current->slide_offset += animation_progress * current->width;
            
            if (current->opacity <= 0.0f) {
                /* Remove notification */
                *prev = next;
                notification_free(current);
                g_notif_system.notification_count--;
                current = next;
                continue;
            }
        }
        
        prev = &current->next;
        current = next;
    }
    
    /* Reposition after removals */
    notification_reposition_all();
}

void notification_system_render(void) {
    notification_t* current = g_notif_system.notifications;
    int32_t y_offset = g_notif_system.notification_y;
    uint32_t count = 0;
    
    while (current && count < g_notif_system.max_visible) {
        if (current->visible) {
            current->x = g_notif_system.notification_x + (int32_t)current->slide_offset;
            current->y = y_offset;
            notification_draw(current);
            y_offset += current->height + g_notif_system.notification_spacing;
            count++;
        }
        current = current->next;
    }
}

bool notification_system_handle_click(int32_t x, int32_t y) {
    notification_t* current = g_notif_system.notifications;
    
    while (current) {
        if (current->visible && !current->animating_out) {
            /* Check if click is within notification bounds */
            if (x >= current->x && x < current->x + (int32_t)current->width &&
                y >= current->y && y < current->y + (int32_t)current->height) {
                
                /* Check if action button was clicked */
                if (current->action_callback && current->action_text[0]) {
                    int32_t button_x = current->x + current->width - 80;
                    int32_t button_y = current->y + current->height - 30;
                    
                    if (x >= button_x && x < button_x + 70 &&
                        y >= button_y && y < button_y + 25) {
                        /* Action button clicked */
                        current->action_callback(current->action_user_data);
                        notification_dismiss(current->id);
                        return true;
                    }
                }
                
                /* Dismiss on click */
                notification_dismiss(current->id);
                return true;
            }
        }
        current = current->next;
    }
    
    return false;
}

/* Helper functions */

static void notification_free(notification_t* notif) {
    if (notif) {
        notif_free(notif);
    }
}

static notification_t* notification_find(uint32_t id) {
    notification_t* current = g_notif_system.notifications;
    while (current) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void notification_reposition_all(void) {
    /* Notifications are positioned dynamically during render */
    /* This could be enhanced to smoothly animate position changes */
}

static uint32_t notification_get_color(notification_type_t type) {
    switch (type) {
        case NOTIFICATION_INFO:    return 0xFF4A90E2;  /* Blue */
        case NOTIFICATION_WARNING: return 0xFFF5A623;  /* Orange */
        case NOTIFICATION_ERROR:   return 0xFFD0021B;  /* Red */
        case NOTIFICATION_SUCCESS: return 0xFF7ED321;  /* Green */
        default:                   return 0xFF888888;  /* Gray */
    }
}

static void notification_draw(notification_t* notif) {
    if (!notif->visible || notif->opacity <= 0.0f) return;
    
    /* Calculate opacity for rendering */
    uint8_t alpha = (uint8_t)(notif->opacity * 255.0f);
    if (alpha < 10) return;  /* Too transparent to render */
    
    /* Background color with transparency */
    color_t bg_color = {0x2C, 0x2C, 0x2C, alpha};  /* Dark gray background */
    color_t border_color = {
        (uint8_t)((notif->icon >> 16) & 0xFF),
        (uint8_t)((notif->icon >> 8) & 0xFF),
        (uint8_t)(notif->icon & 0xFF),
        alpha
    };
    
    /* Draw notification background */
    framebuffer_draw_rect(notif->x, notif->y, notif->width, notif->height, bg_color);
    
    /* Draw colored left border (4px) */
    framebuffer_draw_rect(notif->x, notif->y, 4, notif->height, border_color);
    
    /* Draw title */
    color_t text_color = {0xFF, 0xFF, 0xFF, alpha};
    framebuffer_draw_string(notif->x + 12, notif->y + 10, notif->title, text_color, bg_color);
    
    /* Draw message (wrapped if needed) */
    color_t msg_color = {0xCC, 0xCC, 0xCC, alpha};
    framebuffer_draw_string(notif->x + 12, notif->y + 30, notif->message, msg_color, bg_color);
    
    /* Draw action button if present */
    if (notif->action_callback && notif->action_text[0]) {
        int32_t button_x = notif->x + notif->width - 80;
        int32_t button_y = notif->y + notif->height - 30;
        framebuffer_draw_rect(button_x, button_y, 70, 25, border_color);
        framebuffer_draw_string(button_x + 10, button_y + 7, notif->action_text, text_color, border_color);
    }
    
    /* Draw close button (X) in top right */
    int32_t close_x = notif->x + notif->width - 25;
    int32_t close_y = notif->y + 5;
    color_t close_color = {0xAA, 0xAA, 0xAA, alpha};
    framebuffer_draw_string(close_x, close_y, "X", close_color, bg_color);
}
