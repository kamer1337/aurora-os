/**
 * Aurora OS - Quick Launch Toolbar Implementation
 */

#include "quick_launch.h"
#include "framebuffer.h"
#include "gui_effects.h"
#include <stddef.h>

#define ICON_PADDING 8
#define LABEL_HEIGHT 20

// Quick launch state
static struct {
    int32_t x;
    int32_t y;
    uint32_t icon_size;
    uint32_t total_width;
    uint32_t total_height;
    quick_launch_item_t items[MAX_QUICK_LAUNCH_ITEMS];
    int item_count;
    int hover_item;
    uint8_t visible;
} ql_state = {0};

int quick_launch_init(int32_t x, int32_t y, uint32_t icon_size) {
    ql_state.x = x;
    ql_state.y = y;
    ql_state.icon_size = icon_size;
    ql_state.total_height = icon_size + ICON_PADDING * 2 + LABEL_HEIGHT;
    ql_state.item_count = 0;
    ql_state.hover_item = -1;
    ql_state.visible = 1;
    
    return 0;
}

int quick_launch_add_item(app_type_t app_type, sprite_t* icon, const char* label, void (*on_click)(void)) {
    if (ql_state.item_count >= MAX_QUICK_LAUNCH_ITEMS) {
        return -1;
    }
    
    int index = ql_state.item_count++;
    ql_state.items[index].app_type = app_type;
    ql_state.items[index].icon = icon;
    ql_state.items[index].label = (char*)label;
    ql_state.items[index].visible = 1;
    ql_state.items[index].on_click = on_click;
    
    // Update total width
    ql_state.total_width = ql_state.item_count * (ql_state.icon_size + ICON_PADDING) + ICON_PADDING;
    
    return index;
}

void quick_launch_remove_item(int index) {
    if (index < 0 || index >= ql_state.item_count) {
        return;
    }
    
    // Shift remaining items
    for (int i = index; i < ql_state.item_count - 1; i++) {
        ql_state.items[i] = ql_state.items[i + 1];
    }
    ql_state.item_count--;
    
    // Update total width
    ql_state.total_width = ql_state.item_count * (ql_state.icon_size + ICON_PADDING) + ICON_PADDING;
}

void quick_launch_update(int32_t mouse_x, int32_t mouse_y, uint8_t mouse_clicked) {
    if (!ql_state.visible) {
        return;
    }
    
    ql_state.hover_item = -1;
    
    // Check if mouse is over toolbar area
    if (mouse_y < ql_state.y || mouse_y >= ql_state.y + (int32_t)ql_state.total_height) {
        return;
    }
    
    // Check each item
    for (int i = 0; i < ql_state.item_count; i++) {
        if (!ql_state.items[i].visible) {
            continue;
        }
        
        int32_t item_x = ql_state.x + ICON_PADDING + i * (ql_state.icon_size + ICON_PADDING);
        
        if (mouse_x >= item_x && mouse_x < item_x + (int32_t)ql_state.icon_size) {
            ql_state.hover_item = i;
            
            if (mouse_clicked && ql_state.items[i].on_click) {
                ql_state.items[i].on_click();
            }
            break;
        }
    }
}

void quick_launch_draw(void) {
    if (!ql_state.visible || ql_state.item_count == 0) {
        return;
    }
    
    // Draw toolbar background with glass effect
    color_t bg = {50, 50, 60, 200};
    gui_draw_rounded_rect(ql_state.x, ql_state.y, ql_state.total_width, ql_state.total_height, 8, bg);
    
    // Draw subtle border
    color_t border = {100, 100, 120, 220};
    framebuffer_draw_rect_outline(ql_state.x, ql_state.y, ql_state.total_width, ql_state.total_height, border);
    
    // Draw items
    for (int i = 0; i < ql_state.item_count; i++) {
        if (!ql_state.items[i].visible) {
            continue;
        }
        
        int32_t item_x = ql_state.x + ICON_PADDING + i * (ql_state.icon_size + ICON_PADDING);
        int32_t item_y = ql_state.y + ICON_PADDING;
        
        // Highlight on hover with glow effect
        if (ql_state.hover_item == i) {
            color_t highlight = {120, 160, 255, 100};
            gui_draw_rounded_rect(item_x - 4, item_y - 4, 
                                ql_state.icon_size + 8, ql_state.icon_size + 8, 6, highlight);
            
            // Add subtle glow
            gui_draw_glow(item_x - 4, item_y - 4,
                         ql_state.icon_size + 8, ql_state.icon_size + 8,
                         (color_t){120, 160, 255, 255}, 2);
        }
        
        // Draw icon or default placeholder
        if (ql_state.items[i].icon) {
            gui_draw_sprite(ql_state.items[i].icon, item_x, item_y);
        } else {
            // Draw default icon (colored square)
            color_t default_icon = {100, 150, 200, 255};
            framebuffer_draw_rect(item_x, item_y, ql_state.icon_size, ql_state.icon_size, default_icon);
            framebuffer_draw_rect_outline(item_x, item_y, ql_state.icon_size, ql_state.icon_size, COLOR_WHITE);
        }
        
        // Draw label if available and item is hovered
        if (ql_state.hover_item == i && ql_state.items[i].label) {
            int32_t label_y = item_y + ql_state.icon_size + 4;
            
            // Calculate label width
            int label_len = 0;
            const char* l = ql_state.items[i].label;
            while (*l++) label_len++;
            
            int32_t label_x = item_x + (ql_state.icon_size - label_len * 6) / 2;
            
            // Draw label with shadow
            framebuffer_draw_string_5x7(label_x + 1, label_y + 1, ql_state.items[i].label,
                                       (color_t){0, 0, 0, 180}, (color_t){0, 0, 0, 0});
            framebuffer_draw_string_5x7(label_x, label_y, ql_state.items[i].label,
                                       COLOR_WHITE, (color_t){0, 0, 0, 0});
        }
    }
}

quick_launch_item_t* quick_launch_get_item(int index) {
    if (index < 0 || index >= ql_state.item_count) {
        return NULL;
    }
    return &ql_state.items[index];
}

void quick_launch_set_visible(uint8_t visible) {
    ql_state.visible = visible;
}

int quick_launch_is_visible(void) {
    return ql_state.visible;
}
