/**
 * Aurora OS - Framebuffer Driver
 * 
 * This module provides framebuffer (graphics mode) support for Aurora OS.
 * It manages VESA/VBE framebuffer for graphical output.
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

// Framebuffer information structure
typedef struct {
    uint32_t* address;        // Physical address of framebuffer
    uint32_t width;           // Width in pixels
    uint32_t height;          // Height in pixels
    uint32_t pitch;           // Bytes per scanline
    uint8_t bpp;              // Bits per pixel (typically 32)
    uint8_t red_position;     // Red field position
    uint8_t red_mask_size;    // Red field size
    uint8_t green_position;   // Green field position
    uint8_t green_mask_size;  // Green field size
    uint8_t blue_position;    // Blue field position
    uint8_t blue_mask_size;   // Blue field size
} framebuffer_info_t;

// Color structure (RGBA)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} color_t;

// Predefined colors
#define COLOR_BLACK       (color_t){0, 0, 0, 255}
#define COLOR_WHITE       (color_t){255, 255, 255, 255}
#define COLOR_RED         (color_t){255, 0, 0, 255}
#define COLOR_GREEN       (color_t){0, 255, 0, 255}
#define COLOR_BLUE        (color_t){0, 0, 255, 255}
#define COLOR_YELLOW      (color_t){255, 255, 0, 255}
#define COLOR_CYAN        (color_t){0, 255, 255, 255}
#define COLOR_MAGENTA     (color_t){255, 0, 255, 255}
#define COLOR_GRAY        (color_t){128, 128, 128, 255}
#define COLOR_LIGHT_GRAY  (color_t){192, 192, 192, 255}
#define COLOR_DARK_GRAY   (color_t){64, 64, 64, 255}

/**
 * Initialize the framebuffer driver
 * @param width Desired width (default 1024 if 0)
 * @param height Desired height (default 768 if 0)
 * @param bpp Bits per pixel (default 32 if 0)
 * @return 0 on success, -1 on failure
 */
int framebuffer_init(uint32_t width, uint32_t height, uint8_t bpp);

/**
 * Get framebuffer information
 * @return Pointer to framebuffer info structure
 */
framebuffer_info_t* framebuffer_get_info(void);

/**
 * Check if framebuffer is available
 * @return 1 if available, 0 otherwise
 */
int framebuffer_is_available(void);

/**
 * Draw a pixel at the specified coordinates
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color to draw
 */
void framebuffer_draw_pixel(uint32_t x, uint32_t y, color_t color);

/**
 * Draw a filled rectangle
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Color to fill with
 */
void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);

/**
 * Draw a rectangle outline
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Color to draw with
 */
void framebuffer_draw_rect_outline(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);

/**
 * Draw a horizontal line
 * @param x1 Starting X coordinate
 * @param x2 Ending X coordinate
 * @param y Y coordinate
 * @param color Color to draw with
 */
void framebuffer_draw_hline(uint32_t x1, uint32_t x2, uint32_t y, color_t color);

/**
 * Draw a vertical line
 * @param x X coordinate
 * @param y1 Starting Y coordinate
 * @param y2 Ending Y coordinate
 * @param color Color to draw with
 */
void framebuffer_draw_vline(uint32_t x, uint32_t y1, uint32_t y2, color_t color);

/**
 * Clear the entire screen with a color
 * @param color Color to clear with
 */
void framebuffer_clear(color_t color);

/**
 * Draw a character at the specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 * @param fg_color Foreground color
 * @param bg_color Background color
 */
void framebuffer_draw_char(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color);

/**
 * Draw a string at the specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param fg_color Foreground color
 * @param bg_color Background color
 */
void framebuffer_draw_string(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color);

/**
 * Scroll the framebuffer content up by specified lines
 * @param lines Number of lines to scroll
 * @param bg_color Background color for cleared area
 */
void framebuffer_scroll_up(uint32_t lines, color_t bg_color);

/**
 * Draw a character at the specified position using 5x7 font
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 * @param fg_color Foreground color
 * @param bg_color Background color
 */
void framebuffer_draw_char_5x7(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color);

/**
 * Draw a string at the specified position using 5x7 font
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param fg_color Foreground color
 * @param bg_color Background color
 */
void framebuffer_draw_string_5x7(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color);

/**
 * Convert color_t to framebuffer pixel format
 * @param color Color to convert
 * @return Pixel value in framebuffer format
 */
uint32_t framebuffer_color_to_pixel(color_t color);

#endif // FRAMEBUFFER_H
