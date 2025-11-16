/**
 * Aurora OS - Font Manager
 * 
 * Manages multiple font types and allows runtime font switching
 */

#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <stdint.h>
#include "framebuffer.h"

/**
 * Font types supported by the system
 */
typedef enum {
    FONT_8X8_STANDARD,      // Standard 8x8 bitmap font
    FONT_8X8_CRYSTALLINE,   // Crystalline/futuristic 8x8 font
    FONT_5X7_CRYSTALLINE,   // Compact crystalline 5x7 font
    FONT_6X8_MONO,          // Monospace 6x8 font
    FONT_COUNT              // Total number of fonts
} font_type_t;

/**
 * Font information structure
 */
typedef struct {
    font_type_t type;
    const char* name;
    uint8_t width;
    uint8_t height;
    uint8_t spacing;        // Additional spacing between characters
    const uint8_t* data;    // Pointer to font data
} font_info_t;

/**
 * Initialize the font manager
 * @return 0 on success, -1 on failure
 */
int font_manager_init(void);

/**
 * Get current font type
 * @return Current font type
 */
font_type_t font_manager_get_current(void);

/**
 * Set current font type
 * @param type Font type to set
 * @return 0 on success, -1 on failure
 */
int font_manager_set_current(font_type_t type);

/**
 * Get font information
 * @param type Font type
 * @return Pointer to font info or NULL if invalid
 */
const font_info_t* font_manager_get_info(font_type_t type);

/**
 * Draw a character using current font
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 * @param fg_color Foreground color
 * @param bg_color Background color
 */
void font_manager_draw_char(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color);

/**
 * Draw a string using current font
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param fg_color Foreground color
 * @param bg_color Background color
 */
void font_manager_draw_string(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color);

/**
 * Get character width for current font
 * @return Character width in pixels
 */
uint8_t font_manager_get_char_width(void);

/**
 * Get character height for current font
 * @return Character height in pixels
 */
uint8_t font_manager_get_char_height(void);

/**
 * Get total character advance (width + spacing) for current font
 * @return Character advance in pixels
 */
uint8_t font_manager_get_char_advance(void);

#endif // FONT_MANAGER_H
