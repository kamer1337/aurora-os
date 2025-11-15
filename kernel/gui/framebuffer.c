/**
 * Aurora OS - Framebuffer Driver Implementation
 * 
 * Provides framebuffer (graphics mode) support for graphical output.
 */

#include "framebuffer.h"
#include <stdint.h>
#include <stddef.h>

// Default framebuffer settings (can be overridden by multiboot)
#define DEFAULT_FB_WIDTH  1024
#define DEFAULT_FB_HEIGHT 768
#define DEFAULT_FB_BPP    32

// 8x8 bitmap font (simplified ASCII font)
static const uint8_t font8x8[128][8] = {
    // Space (32)
    [32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    [33] = {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},
    // " (34)
    [34] = {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # (35)
    [35] = {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},
    // A (65)
    [65] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00},
    // B (66)
    [66] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
    // C (67)
    [67] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    // D (68)
    [68] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    // E (69)
    [69] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00},
    // F (70)
    [70] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00},
    // ... (Add more as needed)
};

// Global framebuffer info
static framebuffer_info_t fb_info = {0};
static int fb_available = 0;

int framebuffer_init(uint32_t width, uint32_t height, uint8_t bpp) {
    // Set default values if not specified
    if (width == 0) width = DEFAULT_FB_WIDTH;
    if (height == 0) height = DEFAULT_FB_HEIGHT;
    if (bpp == 0) bpp = DEFAULT_FB_BPP;
    
    // For now, we'll use a simulated framebuffer at a fixed address
    // In a real implementation, this would come from multiboot info or VESA
    fb_info.address = (uint32_t*)0xE0000000;  // Typical framebuffer address
    fb_info.width = width;
    fb_info.height = height;
    fb_info.pitch = width * (bpp / 8);
    fb_info.bpp = bpp;
    
    // Assume standard RGB format for 32bpp
    fb_info.red_position = 16;
    fb_info.red_mask_size = 8;
    fb_info.green_position = 8;
    fb_info.green_mask_size = 8;
    fb_info.blue_position = 0;
    fb_info.blue_mask_size = 8;
    
    fb_available = 1;
    
    // Clear the screen to black
    framebuffer_clear(COLOR_BLACK);
    
    return 0;
}

framebuffer_info_t* framebuffer_get_info(void) {
    return &fb_info;
}

int framebuffer_is_available(void) {
    return fb_available;
}

uint32_t framebuffer_color_to_pixel(color_t color) {
    uint32_t pixel = 0;
    
    pixel |= ((uint32_t)color.r << fb_info.red_position);
    pixel |= ((uint32_t)color.g << fb_info.green_position);
    pixel |= ((uint32_t)color.b << fb_info.blue_position);
    
    return pixel;
}

void framebuffer_draw_pixel(uint32_t x, uint32_t y, color_t color) {
    if (!fb_available || x >= fb_info.width || y >= fb_info.height) {
        return;
    }
    
    uint32_t pixel = framebuffer_color_to_pixel(color);
    uint32_t offset = y * (fb_info.pitch / 4) + x;
    fb_info.address[offset] = pixel;
}

void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color) {
    if (!fb_available) return;
    
    for (uint32_t dy = 0; dy < height; dy++) {
        for (uint32_t dx = 0; dx < width; dx++) {
            framebuffer_draw_pixel(x + dx, y + dy, color);
        }
    }
}

void framebuffer_draw_rect_outline(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color) {
    if (!fb_available) return;
    
    // Top and bottom horizontal lines
    framebuffer_draw_hline(x, x + width - 1, y, color);
    framebuffer_draw_hline(x, x + width - 1, y + height - 1, color);
    
    // Left and right vertical lines
    framebuffer_draw_vline(x, y, y + height - 1, color);
    framebuffer_draw_vline(x + width - 1, y, y + height - 1, color);
}

void framebuffer_draw_hline(uint32_t x1, uint32_t x2, uint32_t y, color_t color) {
    if (!fb_available) return;
    
    if (x1 > x2) {
        uint32_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    for (uint32_t x = x1; x <= x2; x++) {
        framebuffer_draw_pixel(x, y, color);
    }
}

void framebuffer_draw_vline(uint32_t x, uint32_t y1, uint32_t y2, color_t color) {
    if (!fb_available) return;
    
    if (y1 > y2) {
        uint32_t temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    for (uint32_t y = y1; y <= y2; y++) {
        framebuffer_draw_pixel(x, y, color);
    }
}

void framebuffer_clear(color_t color) {
    if (!fb_available) return;
    
    framebuffer_draw_rect(0, 0, fb_info.width, fb_info.height, color);
}

void framebuffer_draw_char(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color) {
    if (!fb_available || c < 0 || c >= 128) return;
    
    const uint8_t* glyph = font8x8[(int)c];
    
    for (int dy = 0; dy < 8; dy++) {
        uint8_t row = glyph[dy];
        for (int dx = 0; dx < 8; dx++) {
            if (row & (1 << (7 - dx))) {
                framebuffer_draw_pixel(x + dx, y + dy, fg_color);
            } else {
                framebuffer_draw_pixel(x + dx, y + dy, bg_color);
            }
        }
    }
}

void framebuffer_draw_string(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color) {
    if (!fb_available || !str) return;
    
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;
    
    while (*str) {
        if (*str == '\n') {
            cursor_x = x;
            cursor_y += 8;
        } else if (*str == '\t') {
            cursor_x += 8 * 4;  // Tab = 4 spaces
        } else {
            framebuffer_draw_char(cursor_x, cursor_y, *str, fg_color, bg_color);
            cursor_x += 8;
            
            // Wrap to next line if needed
            if (cursor_x + 8 > fb_info.width) {
                cursor_x = x;
                cursor_y += 8;
            }
        }
        
        str++;
    }
}

void framebuffer_scroll_up(uint32_t lines, color_t bg_color) {
    if (!fb_available) return;
    
    uint32_t scroll_height = lines * 8;  // Assuming 8-pixel font height
    
    // Move framebuffer content up
    for (uint32_t y = scroll_height; y < fb_info.height; y++) {
        for (uint32_t x = 0; x < fb_info.width; x++) {
            uint32_t src_offset = y * (fb_info.pitch / 4) + x;
            uint32_t dst_offset = (y - scroll_height) * (fb_info.pitch / 4) + x;
            fb_info.address[dst_offset] = fb_info.address[src_offset];
        }
    }
    
    // Clear the bottom area
    framebuffer_draw_rect(0, fb_info.height - scroll_height, fb_info.width, scroll_height, bg_color);
}
