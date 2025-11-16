/**
 * Aurora OS - Window Switcher Implementation
 */

#include "window_switcher.h"
#include "gui.h"
#include "gui_effects.h"
#include "framebuffer.h"
#include <stddef.h>

#define MAX_WINDOWS_IN_SWITCHER 16
#define PREVIEW_WIDTH 200
#define PREVIEW_HEIGHT 150
#define PREVIEW_SPACING 20
#define ANIMATION_DURATION 200  // milliseconds

// Key codes (simplified)
#define KEY_TAB 0x09
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_ENTER 0x1C
#define KEY_ESC 0x01

// Window switcher state
static struct {
    uint8_t visible;
    uint8_t active;
    int selected_index;
    int window_count;
    window_t* windows[MAX_WINDOWS_IN_SWITCHER];
    float animation_progress;  // 0.0 to 1.0
    uint8_t animating_in;
} switcher_state = {0};

void window_switcher_init(void) {
    switcher_state.visible = 0;
    switcher_state.active = 0;
    switcher_state.selected_index = 0;
    switcher_state.window_count = 0;
    switcher_state.animation_progress = 0.0f;
}

void window_switcher_show(void) {
    if (switcher_state.visible) {
        return;
    }
    
    // Build list of windows
    switcher_state.window_count = 0;
    window_t* current = gui_get_window_list();
    
    while (current && switcher_state.window_count < MAX_WINDOWS_IN_SWITCHER) {
        if (current->visible && !current->minimized) {
            switcher_state.windows[switcher_state.window_count++] = current;
        }
        current = current->next;
    }
    
    if (switcher_state.window_count == 0) {
        return;  // No windows to switch
    }
    
    switcher_state.visible = 1;
    switcher_state.active = 1;
    switcher_state.selected_index = 0;
    switcher_state.animation_progress = 0.0f;
    switcher_state.animating_in = 1;
}

void window_switcher_hide(void) {
    switcher_state.visible = 0;
    switcher_state.active = 0;
}

int window_switcher_is_visible(void) {
    return switcher_state.visible;
}

void window_switcher_next(void) {
    if (!switcher_state.active || switcher_state.window_count == 0) {
        return;
    }
    
    switcher_state.selected_index = (switcher_state.selected_index + 1) % switcher_state.window_count;
}

void window_switcher_prev(void) {
    if (!switcher_state.active || switcher_state.window_count == 0) {
        return;
    }
    
    switcher_state.selected_index--;
    if (switcher_state.selected_index < 0) {
        switcher_state.selected_index = switcher_state.window_count - 1;
    }
}

void window_switcher_activate(void) {
    if (!switcher_state.active || switcher_state.window_count == 0) {
        return;
    }
    
    window_t* selected = switcher_state.windows[switcher_state.selected_index];
    if (selected) {
        gui_set_focused_window(selected);
        gui_bring_to_front(selected);
    }
    
    window_switcher_hide();
}

void window_switcher_update(uint32_t delta_time) {
    if (!switcher_state.visible) {
        return;
    }
    
    // Update animation
    if (switcher_state.animation_progress < 1.0f) {
        float increment = (float)delta_time / ANIMATION_DURATION;
        switcher_state.animation_progress += increment;
        if (switcher_state.animation_progress > 1.0f) {
            switcher_state.animation_progress = 1.0f;
        }
    }
}

void window_switcher_draw(void) {
    if (!switcher_state.visible || switcher_state.window_count == 0) {
        return;
    }
    
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info) return;
    
    // Calculate animation easing
    float ease = gui_ease(switcher_state.animation_progress, EASE_OUT_CUBIC);
    
    // Calculate total width and starting position
    int32_t total_width = switcher_state.window_count * (PREVIEW_WIDTH + PREVIEW_SPACING) - PREVIEW_SPACING;
    int32_t start_x = (fb_info->width - total_width) / 2;
    int32_t center_y = (fb_info->height - PREVIEW_HEIGHT) / 2;
    
    // Draw semi-transparent overlay
    color_t overlay = {0, 0, 0, (uint8_t)(150 * ease)};
    gui_draw_rect_alpha(0, 0, fb_info->width, fb_info->height, overlay);
    
    // Draw window previews
    for (int i = 0; i < switcher_state.window_count; i++) {
        window_t* win = switcher_state.windows[i];
        if (!win) continue;
        
        int32_t preview_x = start_x + i * (PREVIEW_WIDTH + PREVIEW_SPACING);
        int32_t preview_y = center_y;
        
        // Apply scale animation
        uint32_t preview_w = (uint32_t)(PREVIEW_WIDTH * ease);
        uint32_t preview_h = (uint32_t)(PREVIEW_HEIGHT * ease);
        int32_t offset_x = (PREVIEW_WIDTH - preview_w) / 2;
        int32_t offset_y = (PREVIEW_HEIGHT - preview_h) / 2;
        
        // Draw selection highlight
        if (i == switcher_state.selected_index) {
            color_t highlight = {100, 150, 255, (uint8_t)(100 * ease)};
            gui_draw_rounded_rect(preview_x + offset_x - 5, preview_y + offset_y - 5,
                                preview_w + 10, preview_h + 10, 5, highlight);
            
            // Draw glow effect
            gui_draw_glow(preview_x + offset_x - 5, preview_y + offset_y - 5,
                         preview_w + 10, preview_h + 10,
                         (color_t){100, 150, 255, 255}, 3);
        }
        
        // Draw preview background
        color_t preview_bg = {60, 60, 70, (uint8_t)(200 * ease)};
        gui_draw_rounded_rect(preview_x + offset_x, preview_y + offset_y,
                            preview_w, preview_h, 8, preview_bg);
        
        // Draw window title
        if (win->title) {
            color_t title_color = i == switcher_state.selected_index ? COLOR_WHITE : COLOR_LIGHT_GRAY;
            title_color.a = (uint8_t)(255 * ease);
            
            // Center title
            int title_len = 0;
            const char* t = win->title;
            while (*t++) title_len++;
            
            int32_t title_x = preview_x + offset_x + (preview_w - title_len * 6) / 2;
            int32_t title_y = preview_y + offset_y + preview_h + 10;
            
            framebuffer_draw_string_5x7(title_x, title_y, win->title, 
                                       title_color, (color_t){0, 0, 0, 0});
        }
        
        // Draw simplified window representation
        // (In a full implementation, this would be a thumbnail of the actual window)
        color_t win_rep = {80, 80, 100, (uint8_t)(255 * ease)};
        framebuffer_draw_rect(preview_x + offset_x + 20, preview_y + offset_y + 30,
                            preview_w - 40, preview_h - 60, win_rep);
    }
    
    // Draw instructions
    const char* instructions = "Tab/Arrows: Switch  Enter: Select  Esc: Cancel";
    color_t text_color = {200, 200, 200, (uint8_t)(255 * ease)};
    int32_t inst_x = (fb_info->width - (sizeof(instructions) - 1) * 6) / 2;
    framebuffer_draw_string_5x7(inst_x, fb_info->height - 40, instructions,
                               text_color, (color_t){0, 0, 0, 0});
}

int window_switcher_handle_key(uint32_t key, uint8_t pressed) {
    if (!switcher_state.visible || !pressed) {
        return 0;
    }
    
    switch (key) {
        case KEY_TAB:
        case KEY_RIGHT:
            window_switcher_next();
            return 1;
            
        case KEY_LEFT:
            window_switcher_prev();
            return 1;
            
        case KEY_ENTER:
            window_switcher_activate();
            return 1;
            
        case KEY_ESC:
            window_switcher_hide();
            return 1;
            
        default:
            return 0;
    }
}
