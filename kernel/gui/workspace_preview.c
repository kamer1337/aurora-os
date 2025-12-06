/**
 * @file workspace_preview.c
 * @brief Workspace Preview Implementation
 */

#include "workspace_preview.h"
#include "framebuffer.h"
#include "virtual_desktop.h"
#include "../memory/memory.h"
#include "../core/kernel.h"

/* Global workspace preview manager */
static workspace_preview_manager_t g_preview_mgr = {0};

/* Helper functions */
static void preview_capture_workspace(uint32_t workspace_id);
static void preview_render_thumbnail(workspace_preview_t* preview);
static void preview_scale_framebuffer(const uint32_t* source, uint32_t src_width, uint32_t src_height,
                                     uint32_t* dest, uint32_t dest_width, uint32_t dest_height);
static void wp_memset(void* ptr, int value, size_t size);
static void wp_strcpy(char* dest, const char* src, size_t max_len);

/* Memory management for freestanding */
static void wp_memset(void* ptr, int value, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
}

static void wp_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int workspace_preview_init(void) {
    wp_memset(&g_preview_mgr, 0, sizeof(workspace_preview_manager_t));
    
    /* Configuration */
    g_preview_mgr.panel_x = 400;
    g_preview_mgr.panel_y = 300;
    g_preview_mgr.panel_width = 1120;  /* 4 previews wide + margins */
    g_preview_mgr.panel_height = 380;  /* 2 rows + margins */
    g_preview_mgr.update_interval_ms = 500;  /* Update every 500ms */
    g_preview_mgr.visible = false;
    
    /* Initialize 4 default workspaces */
    g_preview_mgr.preview_count = 4;
    for (uint32_t i = 0; i < g_preview_mgr.preview_count; i++) {
        workspace_preview_t* preview = &g_preview_mgr.previews[i];
        preview->workspace_id = i;
        preview->dirty = true;
        preview->active = (i == 0);
        preview->hovered = false;
        preview->width = PREVIEW_WIDTH;
        preview->height = PREVIEW_HEIGHT;
        preview->window_count = 0;
        
        /* Set default names */
        wp_strcpy(preview->name, "Workspace", sizeof(preview->name));
        preview->name[9] = ' ';
        preview->name[10] = '1' + i;
        preview->name[11] = '\0';
        
        wp_strcpy(preview->os_type, "Aurora OS", sizeof(preview->os_type));
        
        /* Calculate position (2x2 grid) */
        uint32_t col = i % 2;
        uint32_t row = i / 2;
        preview->x = g_preview_mgr.panel_x + PREVIEW_MARGIN + 
                    (col * (PREVIEW_WIDTH + PREVIEW_MARGIN * 2));
        preview->y = g_preview_mgr.panel_y + PREVIEW_MARGIN +
                    (row * (PREVIEW_HEIGHT + PREVIEW_MARGIN * 2));
    }
    
    return 0;
}

void workspace_preview_shutdown(void) {
    wp_memset(&g_preview_mgr, 0, sizeof(workspace_preview_manager_t));
}

void workspace_preview_update_all(void) {
    for (uint32_t i = 0; i < g_preview_mgr.preview_count; i++) {
        if (g_preview_mgr.previews[i].dirty) {
            preview_capture_workspace(i);
            g_preview_mgr.previews[i].dirty = false;
        }
    }
    g_preview_mgr.last_update = 0;  /* Would use get_system_ticks() */
}

void workspace_preview_update(uint32_t workspace_id) {
    if (workspace_id >= g_preview_mgr.preview_count) return;
    preview_capture_workspace(workspace_id);
    g_preview_mgr.previews[workspace_id].dirty = false;
}

void workspace_preview_render(void) {
    if (!g_preview_mgr.visible) return;
    
    /* Draw panel background with slight transparency */
    color_t panel_bg = {0x20, 0x20, 0x20, 230};
    framebuffer_draw_rect(g_preview_mgr.panel_x, g_preview_mgr.panel_y,
                         g_preview_mgr.panel_width, g_preview_mgr.panel_height,
                         panel_bg);
    
    /* Draw title */
    color_t title_color = {0xFF, 0xFF, 0xFF, 255};
    color_t title_bg = {0x20, 0x20, 0x20, 230};
    framebuffer_draw_string(g_preview_mgr.panel_x + 20, 
                           g_preview_mgr.panel_y + 15,
                           "Workspace Switcher", title_color, title_bg);
    
    /* Render all workspace previews */
    for (uint32_t i = 0; i < g_preview_mgr.preview_count; i++) {
        preview_render_thumbnail(&g_preview_mgr.previews[i]);
    }
}

void workspace_preview_show(void) {
    g_preview_mgr.visible = true;
    workspace_preview_update_all();
}

void workspace_preview_hide(void) {
    g_preview_mgr.visible = false;
}

void workspace_preview_toggle(void) {
    g_preview_mgr.visible = !g_preview_mgr.visible;
    if (g_preview_mgr.visible) {
        workspace_preview_update_all();
    }
}

int workspace_preview_handle_click(int32_t x, int32_t y) {
    if (!g_preview_mgr.visible) return -1;
    
    for (uint32_t i = 0; i < g_preview_mgr.preview_count; i++) {
        workspace_preview_t* preview = &g_preview_mgr.previews[i];
        
        if (x >= preview->x && x < preview->x + (int32_t)preview->width &&
            y >= preview->y && y < preview->y + (int32_t)preview->height) {
            return (int)i;
        }
    }
    
    return -1;
}

void workspace_preview_handle_hover(int32_t x, int32_t y) {
    if (!g_preview_mgr.visible) return;
    
    for (uint32_t i = 0; i < g_preview_mgr.preview_count; i++) {
        workspace_preview_t* preview = &g_preview_mgr.previews[i];
        
        bool was_hovered = preview->hovered;
        preview->hovered = (x >= preview->x && x < preview->x + (int32_t)preview->width &&
                           y >= preview->y && y < preview->y + (int32_t)preview->height);
        
        /* Mark dirty if hover state changed */
        if (was_hovered != preview->hovered) {
            preview->dirty = true;
        }
    }
}

void workspace_preview_set_name(uint32_t workspace_id, const char* name) {
    if (workspace_id >= g_preview_mgr.preview_count) return;
    wp_strcpy(g_preview_mgr.previews[workspace_id].name, name, 
             sizeof(g_preview_mgr.previews[workspace_id].name));
}

void workspace_preview_set_os_type(uint32_t workspace_id, const char* os_type) {
    if (workspace_id >= g_preview_mgr.preview_count) return;
    wp_strcpy(g_preview_mgr.previews[workspace_id].os_type, os_type,
             sizeof(g_preview_mgr.previews[workspace_id].os_type));
}

void workspace_preview_mark_dirty(uint32_t workspace_id) {
    if (workspace_id >= g_preview_mgr.preview_count) return;
    g_preview_mgr.previews[workspace_id].dirty = true;
}

void workspace_preview_set_active(uint32_t workspace_id) {
    if (workspace_id >= g_preview_mgr.preview_count) return;
    
    /* Clear old active */
    for (uint32_t i = 0; i < g_preview_mgr.preview_count; i++) {
        g_preview_mgr.previews[i].active = false;
    }
    
    /* Set new active */
    g_preview_mgr.previews[workspace_id].active = true;
    g_preview_mgr.active_workspace = workspace_id;
}

/* Helper functions */

static void preview_capture_workspace(uint32_t workspace_id) {
    if (workspace_id >= g_preview_mgr.preview_count) return;
    
    workspace_preview_t* preview = &g_preview_mgr.previews[workspace_id];
    framebuffer_info_t* fb_info = framebuffer_get_info();
    
    if (!fb_info || !fb_info->address) {
        /* If framebuffer not available, just fill with default color */
        for (uint32_t i = 0; i < PREVIEW_WIDTH * PREVIEW_HEIGHT; i++) {
            preview->thumbnail[i] = 0xFF303030;  /* Dark gray */
        }
        return;
    }
    
    /* Scale down framebuffer to thumbnail size */
    preview_scale_framebuffer(fb_info->address, fb_info->width, fb_info->height,
                             preview->thumbnail, PREVIEW_WIDTH, PREVIEW_HEIGHT);
}

static void preview_render_thumbnail(workspace_preview_t* preview) {
    /* Draw border (thicker for active, highlight for hover) */
    color_t border_color;
    uint32_t border_width = PREVIEW_BORDER;
    
    if (preview->active) {
        border_color = (color_t){0x4A, 0x90, 0xE2, 255};  /* Blue for active */
        border_width = 4;
    } else if (preview->hovered) {
        border_color = (color_t){0x7E, 0xD3, 0x21, 255};  /* Green for hover */
        border_width = 3;
    } else {
        border_color = (color_t){0x60, 0x60, 0x60, 255};  /* Gray for inactive */
    }
    
    /* Draw border */
    framebuffer_draw_rect_outline(preview->x - border_width, 
                                  preview->y - border_width,
                                  preview->width + border_width * 2,
                                  preview->height + border_width * 2,
                                  border_color);
    
    /* Draw thumbnail content (simplified - would copy actual pixels) */
    color_t thumb_bg = {0x30, 0x30, 0x30, 255};
    framebuffer_draw_rect(preview->x, preview->y, preview->width, preview->height, thumb_bg);
    
    /* Draw workspace name */
    color_t text_color = {0xFF, 0xFF, 0xFF, 255};
    color_t text_bg = {0x30, 0x30, 0x30, 255};
    framebuffer_draw_string(preview->x + 5, preview->y + 5, 
                           preview->name, text_color, text_bg);
    
    /* Draw OS type */
    color_t os_color = {0xCC, 0xCC, 0xCC, 255};
    framebuffer_draw_string(preview->x + 5, preview->y + 20,
                           preview->os_type, os_color, text_bg);
}

static void preview_scale_framebuffer(const uint32_t* source, uint32_t src_width, uint32_t src_height,
                                     uint32_t* dest, uint32_t dest_width, uint32_t dest_height) {
    /* Simple nearest-neighbor scaling */
    for (uint32_t dy = 0; dy < dest_height; dy++) {
        for (uint32_t dx = 0; dx < dest_width; dx++) {
            uint32_t sx = (dx * src_width) / dest_width;
            uint32_t sy = (dy * src_height) / dest_height;
            
            if (sx < src_width && sy < src_height) {
                dest[dy * dest_width + dx] = source[sy * src_width + sx];
            }
        }
    }
}
