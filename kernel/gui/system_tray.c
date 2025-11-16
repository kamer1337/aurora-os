/**
 * Aurora OS - System Tray Implementation
 */

#include "system_tray.h"
#include "framebuffer.h"
#include "gui_effects.h"
#include <stddef.h>

#define MAX_TRAY_ICONS 16
#define MAX_NOTIFICATIONS 8
#define ICON_SIZE 16
#define ICON_PADDING 4
#define DEFAULT_NOTIFICATION_DURATION 3000

// Forward declarations
static void draw_battery_icon(int32_t x, int32_t y, uint8_t level, uint8_t charging);
static void draw_volume_icon(int32_t x, int32_t y, uint8_t level, uint8_t muted);
static void draw_network_icon(int32_t x, int32_t y, uint8_t connected, uint8_t strength);

// System tray state
static struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    tray_icon_t icons[MAX_TRAY_ICONS];
    int icon_count;
    uint8_t hours;
    uint8_t minutes;
    uint8_t network_connected;
    uint8_t network_strength;
    uint8_t volume_level;
    uint8_t volume_muted;
    uint8_t battery_level;
    uint8_t battery_charging;
    int hover_icon;
} tray_state = {0};

// Notification state
static notification_t notifications[MAX_NOTIFICATIONS] = {0};

int system_tray_init(int32_t x, int32_t y, uint32_t width) {
    tray_state.x = x;
    tray_state.y = y;
    tray_state.width = width;
    tray_state.height = ICON_SIZE + ICON_PADDING * 2;
    tray_state.icon_count = 0;
    tray_state.hours = 12;
    tray_state.minutes = 0;
    tray_state.hover_icon = -1;
    
    return 0;
}

int system_tray_add_icon(tray_icon_type_t type, sprite_t* icon, const char* tooltip, void (*on_click)(void)) {
    if (tray_state.icon_count >= MAX_TRAY_ICONS) {
        return -1;
    }
    
    int index = tray_state.icon_count++;
    tray_state.icons[index].type = type;
    tray_state.icons[index].icon = icon;
    tray_state.icons[index].tooltip = (char*)tooltip;
    tray_state.icons[index].visible = 1;
    tray_state.icons[index].on_click = on_click;
    
    return index;
}

void system_tray_remove_icon(int index) {
    if (index < 0 || index >= tray_state.icon_count) {
        return;
    }
    
    // Shift remaining icons
    for (int i = index; i < tray_state.icon_count - 1; i++) {
        tray_state.icons[i] = tray_state.icons[i + 1];
    }
    tray_state.icon_count--;
}

void system_tray_update(int32_t mouse_x, int32_t mouse_y, uint8_t mouse_clicked) {
    tray_state.hover_icon = -1;
    
    // Check if mouse is over tray area
    if (mouse_y < tray_state.y || mouse_y >= tray_state.y + (int32_t)tray_state.height) {
        return;
    }
    
    // Calculate icon positions and check for hover/click
    int32_t icon_x = tray_state.x + tray_state.width - ICON_PADDING;
    
    for (int i = tray_state.icon_count - 1; i >= 0; i--) {
        if (!tray_state.icons[i].visible) {
            continue;
        }
        
        icon_x -= ICON_SIZE + ICON_PADDING;
        
        if (mouse_x >= icon_x && mouse_x < icon_x + ICON_SIZE) {
            tray_state.hover_icon = i;
            
            if (mouse_clicked && tray_state.icons[i].on_click) {
                tray_state.icons[i].on_click();
            }
            break;
        }
    }
}

void system_tray_draw(void) {
    // Draw tray background with glass effect
    color_t bg = {40, 40, 50, 220};
    gui_draw_rect_alpha(tray_state.x, tray_state.y, tray_state.width, tray_state.height, bg);
    
    // Draw subtle top border
    color_t border = {80, 80, 100, 200};
    framebuffer_draw_hline(tray_state.x, tray_state.x + tray_state.width, tray_state.y, border);
    
    // Draw icons from right to left
    int32_t icon_x = tray_state.x + tray_state.width - ICON_PADDING;
    
    // First draw time (rightmost)
    icon_x -= 50;  // Space for time display
    char time_str[6];
    time_str[0] = '0' + (tray_state.hours / 10);
    time_str[1] = '0' + (tray_state.hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (tray_state.minutes / 10);
    time_str[4] = '0' + (tray_state.minutes % 10);
    time_str[5] = '\0';
    
    framebuffer_draw_string_5x7(icon_x, tray_state.y + ICON_PADDING + 4, time_str, 
                                COLOR_WHITE, (color_t){0, 0, 0, 0});
    
    icon_x -= ICON_PADDING;
    
    // Draw system status icons (battery, volume, network)
    // Battery
    if (tray_state.battery_level <= 100) {
        icon_x -= ICON_SIZE + ICON_PADDING;
        draw_battery_icon(icon_x, tray_state.y + ICON_PADDING, 
                         tray_state.battery_level, tray_state.battery_charging);
    }
    
    // Volume
    icon_x -= ICON_SIZE + ICON_PADDING;
    draw_volume_icon(icon_x, tray_state.y + ICON_PADDING, 
                     tray_state.volume_level, tray_state.volume_muted);
    
    // Network
    icon_x -= ICON_SIZE + ICON_PADDING;
    draw_network_icon(icon_x, tray_state.y + ICON_PADDING, 
                      tray_state.network_connected, tray_state.network_strength);
    
    // Draw custom icons
    for (int i = 0; i < tray_state.icon_count; i++) {
        if (!tray_state.icons[i].visible || tray_state.icons[i].type != TRAY_ICON_CUSTOM) {
            continue;
        }
        
        icon_x -= ICON_SIZE + ICON_PADDING;
        
        // Highlight on hover
        if (tray_state.hover_icon == i) {
            color_t highlight = {255, 255, 255, 50};
            gui_draw_rounded_rect(icon_x - 2, tray_state.y + ICON_PADDING - 2, 
                                ICON_SIZE + 4, ICON_SIZE + 4, 3, highlight);
        }
        
        // Draw icon
        if (tray_state.icons[i].icon) {
            gui_draw_sprite(tray_state.icons[i].icon, icon_x, tray_state.y + ICON_PADDING);
        }
    }
}

// Helper functions to draw system icons
static void draw_battery_icon(int32_t x, int32_t y, uint8_t level, uint8_t charging) {
    // Draw battery outline
    color_t outline = COLOR_WHITE;
    framebuffer_draw_rect_outline(x + 2, y + 3, 10, 8, outline);
    framebuffer_draw_rect(x + 12, y + 5, 2, 4, outline);
    
    // Draw battery fill based on level
    uint8_t fill_width = (level * 8) / 100;
    color_t fill = level > 20 ? COLOR_GREEN : COLOR_RED;
    if (fill_width > 0) {
        framebuffer_draw_rect(x + 3, y + 4, fill_width, 6, fill);
    }
    
    // Draw charging indicator
    if (charging) {
        color_t bolt = COLOR_YELLOW;
        framebuffer_draw_vline(x + 7, y + 5, y + 9, bolt);
        framebuffer_draw_pixel(x + 6, y + 6, bolt);
        framebuffer_draw_pixel(x + 8, y + 8, bolt);
    }
}

static void draw_volume_icon(int32_t x, int32_t y, uint8_t level, uint8_t muted) {
    color_t icon_color = COLOR_WHITE;
    
    // Draw speaker cone
    framebuffer_draw_rect(x + 3, y + 6, 3, 4, icon_color);
    framebuffer_draw_pixel(x + 6, y + 5, icon_color);
    framebuffer_draw_pixel(x + 6, y + 10, icon_color);
    
    if (muted) {
        // Draw X over icon
        color_t red = COLOR_RED;
        framebuffer_draw_pixel(x + 9, y + 5, red);
        framebuffer_draw_pixel(x + 10, y + 6, red);
        framebuffer_draw_pixel(x + 11, y + 7, red);
        framebuffer_draw_pixel(x + 10, y + 8, red);
        framebuffer_draw_pixel(x + 9, y + 9, red);
    } else {
        // Draw sound waves based on level
        if (level > 33) {
            framebuffer_draw_pixel(x + 9, y + 6, icon_color);
            framebuffer_draw_pixel(x + 9, y + 9, icon_color);
        }
        if (level > 66) {
            framebuffer_draw_pixel(x + 11, y + 5, icon_color);
            framebuffer_draw_pixel(x + 11, y + 10, icon_color);
        }
    }
}

static void draw_network_icon(int32_t x, int32_t y, uint8_t connected, uint8_t strength) {
    color_t icon_color = connected ? COLOR_WHITE : COLOR_GRAY;
    
    // Draw signal bars
    int bars = (strength / 25) + 1;  // 1-4 bars
    if (bars > 4) bars = 4;
    
    for (int i = 0; i < 4; i++) {
        uint8_t bar_height = (i + 1) * 3;
        color_t bar_color = (i < bars) ? icon_color : (color_t){60, 60, 60, 255};
        framebuffer_draw_rect(x + i * 3 + 2, y + 12 - bar_height, 2, bar_height, bar_color);
    }
}

int system_tray_notify(const char* title, const char* message, uint32_t duration, color_t color) {
    // Find available notification slot
    int slot = -1;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;  // No slots available
    }
    
    // Set up notification
    notifications[slot].title = (char*)title;
    notifications[slot].message = (char*)message;
    notifications[slot].duration = duration ? duration : DEFAULT_NOTIFICATION_DURATION;
    notifications[slot].start_time = 0;  // Will be set in update
    notifications[slot].active = 1;
    notifications[slot].color = color;
    
    return slot;
}

void system_tray_update_notifications(uint32_t current_time) {
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].active) {
            continue;
        }
        
        // Initialize start time on first update
        if (notifications[i].start_time == 0) {
            notifications[i].start_time = current_time;
        }
        
        // Check if notification expired
        if (current_time - notifications[i].start_time >= notifications[i].duration) {
            notifications[i].active = 0;
        }
    }
}

void system_tray_draw_notifications(void) {
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info) return;
    
    int32_t notify_x = fb_info->width - 320;
    int32_t notify_y = 50;
    
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].active) {
            continue;
        }
        
        // Draw notification box with shadow
        gui_draw_shadow(notify_x, notify_y, 300, 80, 4, 3);
        
        // Draw background with gradient
        color_t top = notifications[i].color;
        color_t bottom = notifications[i].color;
        bottom.r = (uint8_t)(bottom.r * 0.7f);
        bottom.g = (uint8_t)(bottom.g * 0.7f);
        bottom.b = (uint8_t)(bottom.b * 0.7f);
        gui_draw_gradient(notify_x, notify_y, 300, 80, top, bottom);
        
        // Draw border
        framebuffer_draw_rect_outline(notify_x, notify_y, 300, 80, COLOR_WHITE);
        
        // Draw title
        if (notifications[i].title) {
            framebuffer_draw_string_5x7(notify_x + 10, notify_y + 10, 
                                        notifications[i].title, COLOR_WHITE, (color_t){0, 0, 0, 0});
        }
        
        // Draw message
        if (notifications[i].message) {
            framebuffer_draw_string_5x7(notify_x + 10, notify_y + 30, 
                                        notifications[i].message, COLOR_LIGHT_GRAY, (color_t){0, 0, 0, 0});
        }
        
        notify_y += 90;  // Stack notifications vertically
    }
}

void system_tray_set_time(uint8_t hours, uint8_t minutes) {
    tray_state.hours = hours % 24;
    tray_state.minutes = minutes % 60;
}

void system_tray_set_network_status(uint8_t connected, uint8_t strength) {
    tray_state.network_connected = connected;
    tray_state.network_strength = strength > 100 ? 100 : strength;
}

void system_tray_set_volume(uint8_t level, uint8_t muted) {
    tray_state.volume_level = level > 100 ? 100 : level;
    tray_state.volume_muted = muted;
}

void system_tray_set_battery(uint8_t level, uint8_t charging) {
    tray_state.battery_level = level > 100 ? 100 : level;
    tray_state.battery_charging = charging;
}
