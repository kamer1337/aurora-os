/**
 * Aurora OS - Framebuffer Driver Implementation
 * 
 * Provides framebuffer (graphics mode) support for graphical output.
 */

#include "framebuffer.h"
#include "../../include/multiboot.h"
#include <stdint.h>
#include <stddef.h>

// Default framebuffer settings (can be overridden by multiboot)
// Full HD resolution support
#define DEFAULT_FB_WIDTH  1920
#define DEFAULT_FB_HEIGHT 1080
#define DEFAULT_FB_BPP    32

// 8x8 bitmap font (simplified ASCII font)
const uint8_t font8x8[128][8] = {
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

// 5x7 bitmap font with modified patterns for alphanumeric characters
// Each character is encoded in 7 bytes, with 5 bits used per byte (bits 0-4)
// Note: Characters 0-31 and 127 (control characters) are intentionally left zero-initialized
// as they are not typically rendered. Access to these will render as blank characters.
const uint8_t font5x7[128][7] = {
    // Space (32)
    [32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33) - Sharp exclamation
    [33] = {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00},
    // " (34) - Angular quotes
    [34] = {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # (35) - Grid hash
    [35] = {0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00, 0x00},
    // $ (36) - Currency symbol
    [36] = {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04},
    // % (37) - Diagonal split
    [37] = {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},
    // & (38) - Angular ampersand
    [38] = {0x08, 0x14, 0x14, 0x08, 0x15, 0x12, 0x0D},
    // ' (39) - Sharp tick
    [39] = {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ( (40) - Angular bracket
    [40] = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},
    // ) (41) - Angular bracket
    [41] = {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},
    // * (42) - Star burst
    [42] = {0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00},
    // + (43) - Cross
    [43] = {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00},
    // , (44) - Sharp comma
    [44] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08},
    // - (45) - Sharp line
    [45] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
    // . (46) - Crystal dot
    [46] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00},
    // / (47) - Diagonal blade
    [47] = {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10},
    // 0 (48) - Diamond/crystal shape with inner details
    [48] = {0x0E, 0x15, 0x15, 0x15, 0x15, 0x15, 0x0E},
    // 1 (49) - Angular ascending line
    [49] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x1F},
    // 2 (50) - Sharp angular path
    [50] = {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    // 3 (51) - Dual curves with crystal points
    [51] = {0x1F, 0x01, 0x02, 0x06, 0x01, 0x11, 0x0E},
    // 4 (52) - Sharp intersection
    [52] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    // 5 (53) - Angular transformation
    [53] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    // 6 (54) - Spiral crystalline
    [54] = {0x07, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    // 7 (55) - Diagonal blade
    [55] = {0x1F, 0x11, 0x02, 0x04, 0x04, 0x08, 0x08},
    // 8 (56) - Double diamond stack
    [56] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    // 9 (57) - Inverted spiral
    [57] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x1C},
    // : (58)
    [58] = {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00},
    // ; (59)
    [59] = {0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08},
    // < (60)
    [60] = {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},
    // = (61)
    [61] = {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00},
    // > (62)
    [62] = {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},
    // ? (63)
    [63] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04},
    // @ (64)
    [64] = {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E},
    // A (65) - Sharp crystalline triangle with cross-beam
    [65] = {0x04, 0x0A, 0x11, 0x11, 0x1F, 0x11, 0x11},
    // B (66) - Angular blocky with sharp edges
    [66] = {0x1E, 0x09, 0x09, 0x1E, 0x09, 0x09, 0x1E},
    // C (67) - Sharp arc opening
    [67] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    // D (68) - Angled pillar
    [68] = {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    // E (69) - Tiered blocks
    [69] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    // F (70) - Rising tier blocks
    [70] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    // G (71) - Arc with inner structure
    [71] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
    // H (72) - Pillar bridge
    [72] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    // I (73) - Crystal column
    [73] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // J (74) - Curved blade
    [74] = {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    // K (75) - Angular splitter
    [75] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    // L (76) - Right angle base
    [76] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    // M (77) - Twin peaks
    [77] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    // N (78) - Diagonal beam
    [78] = {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
    // O (79) - Perfect diamond
    [79] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // P (80) - Half arc closure
    [80] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    // Q (81) - Diamond with tail
    [81] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    // R (82) - Arc with diagonal leg
    [82] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    // S (83) - Angular snake path
    [83] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    // T (84) - Crystal tower
    [84] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    // U (85) - Cup form
    [85] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // V (86) - Converging lines
    [86] = {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    // W (87) - Twin valleys
    [87] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A},
    // X (88) - Crossed blades
    [88] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    // Y (89) - Forked convergence
    [89] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    // Z (90) - Angular lightning
    [90] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
    // [ (91)
    [91] = {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E},
    // \ (92)
    [92] = {0x10, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01},
    // ] (93)
    [93] = {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E},
    // ^ (94)
    [94] = {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00},
    // _ (95)
    [95] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F},
    // ` (96)
    [96] = {0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    // a (97) - Compact angular form
    [97] = {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F},
    // b (98) - Vertical with curve
    [98] = {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E},
    // c (99) - Angular arc
    [99] = {0x00, 0x00, 0x0E, 0x11, 0x10, 0x11, 0x0E},
    // d (100) - Mirror curve
    [100] = {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F},
    // e (101) - Cut diamond
    [101] = {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E},
    // f (102) - Hooked column
    [102] = {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08},
    // g (103) - Curved with tail
    [103] = {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01},
    // h (104) - Pillar hook
    [104] = {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11},
    // i (105) - Dotted column
    [105] = {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E},
    // j (106) - Dotted hook
    [106] = {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C},
    // k (107) - Angular split
    [107] = {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12},
    // l (108) - Simple column
    [108] = {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // m (109) - Twin arcs
    [109] = {0x00, 0x00, 0x1A, 0x15, 0x15, 0x15, 0x11},
    // n (110) - Single arc
    [110] = {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11},
    // o (111) - Round crystal
    [111] = {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E},
    // p (112) - Arc with base
    [112] = {0x00, 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10},
    // q (113) - Arc with tail
    [113] = {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01},
    // r (114) - Angled stub
    [114] = {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10},
    // s (115) - Compact snake
    [115] = {0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E},
    // t (116) - Crossed pole
    [116] = {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06},
    // u (117) - Curved bottom
    [117] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D},
    // v (118) - Converging lines
    [118] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04},
    // w (119) - Twin converge
    [119] = {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A},
    // x (120) - Crossed paths
    [120] = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11},
    // y (121) - Forked tail
    [121] = {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    // z (122) - Angular strike
    [122] = {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F},
    // { (123)
    [123] = {0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02},
    // | (124)
    [124] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    // } (125)
    [125] = {0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08},
    // ~ (126)
    [126] = {0x00, 0x00, 0x08, 0x15, 0x02, 0x00, 0x00},
};

// Global framebuffer info
static framebuffer_info_t fb_info = {0};
static int fb_available = 0;

// Double buffering support
static uint32_t* back_buffer = 0;
static uint32_t* front_buffer = 0;
static int double_buffering_enabled = 0;
static uint32_t* current_draw_buffer = 0;

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
    front_buffer = fb_info.address;
    current_draw_buffer = fb_info.address;
    
    // Clear the screen to black
    framebuffer_clear(COLOR_BLACK);
    
    return 0;
}

int framebuffer_init_from_multiboot(void* mbi) {
    if (!mbi) {
        // Fall back to default initialization
        return framebuffer_init(0, 0, 0);
    }
    
    multiboot_info_t* mb_info = (multiboot_info_t*)mbi;
    
    // Check if framebuffer info is available
    if (!(mb_info->flags & MULTIBOOT_FLAG_FB)) {
        // No framebuffer info, use defaults
        return framebuffer_init(0, 0, 0);
    }
    
    // Validate framebuffer type (we only support RGB framebuffer)
    if (mb_info->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        // Unsupported framebuffer type, use defaults
        return framebuffer_init(0, 0, 0);
    }
    
    // Set up framebuffer info from multiboot
    fb_info.address = (uint32_t*)(uintptr_t)mb_info->framebuffer_addr;
    fb_info.width = mb_info->framebuffer_width;
    fb_info.height = mb_info->framebuffer_height;
    fb_info.pitch = mb_info->framebuffer_pitch;
    fb_info.bpp = mb_info->framebuffer_bpp;
    
    // Set color field information
    fb_info.red_position = mb_info->framebuffer_red_field_position;
    fb_info.red_mask_size = mb_info->framebuffer_red_mask_size;
    fb_info.green_position = mb_info->framebuffer_green_field_position;
    fb_info.green_mask_size = mb_info->framebuffer_green_mask_size;
    fb_info.blue_position = mb_info->framebuffer_blue_field_position;
    fb_info.blue_mask_size = mb_info->framebuffer_blue_mask_size;
    
    fb_available = 1;
    front_buffer = fb_info.address;
    current_draw_buffer = fb_info.address;
    
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

void framebuffer_draw_char_5x7(uint32_t x, uint32_t y, int c, color_t fg_color, color_t bg_color) {
    if (!fb_available || c < 0 || c >= 128) return;
    
    const uint8_t* glyph = font5x7[c];
    
    for (int dy = 0; dy < 7; dy++) {
        uint8_t row = glyph[dy];
        for (int dx = 0; dx < 5; dx++) {
            if (row & (1 << dx)) {
                framebuffer_draw_pixel(x + dx, y + dy, fg_color);
            } else {
                framebuffer_draw_pixel(x + dx, y + dy, bg_color);
            }
        }
    }
}

void framebuffer_draw_string_5x7(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color) {
    if (!fb_available || !str) return;
    
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;
    
    while (*str) {
        if (*str == '\n') {
            cursor_x = x;
            cursor_y += 7;
        } else if (*str == '\t') {
            cursor_x += 6 * 4;  // Tab = 4 spaces (6 pixels per character including spacing)
        } else {
            // Check if we have space to render this character
            if (cursor_y + 7 <= fb_info.height) {
                framebuffer_draw_char_5x7(cursor_x, cursor_y, *str, fg_color, bg_color);
            }
            cursor_x += 6;  // 5 pixels for char + 1 pixel spacing
            
            // Wrap to next line if needed
            if (cursor_x + 6 > fb_info.width) {
                cursor_x = x;
                cursor_y += 7;
            }
        }
        
        str++;
    }
}

/**
 * Enable or disable double buffering
 */
int framebuffer_set_double_buffering(int enable) {
    if (!fb_available) {
        return -1;
    }
    
    if (enable && !double_buffering_enabled) {
        // Allocate back buffer
        size_t buffer_size = fb_info.width * fb_info.height * sizeof(uint32_t);
        
        // Use a fixed memory region for back buffer (in kernel heap)
        // In a real implementation, this would use proper memory allocation
        back_buffer = (uint32_t*)0xE1000000;  // 16MB offset from front buffer
        front_buffer = fb_info.address;
        current_draw_buffer = back_buffer;
        double_buffering_enabled = 1;
        
        // Clear back buffer
        for (uint32_t i = 0; i < fb_info.width * fb_info.height; i++) {
            back_buffer[i] = 0;
        }
    } else if (!enable && double_buffering_enabled) {
        // Disable double buffering
        current_draw_buffer = front_buffer;
        double_buffering_enabled = 0;
        back_buffer = 0;
    }
    
    return 0;
}

/**
 * Swap buffers (present back buffer to front)
 */
void framebuffer_swap_buffers(void) {
    if (!double_buffering_enabled || !back_buffer || !front_buffer) {
        return;
    }
    
    // Copy back buffer to front buffer
    uint32_t total_pixels = fb_info.width * fb_info.height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        front_buffer[i] = back_buffer[i];
    }
}

/**
 * Get the current drawing buffer address
 */
uint32_t* framebuffer_get_draw_buffer(void) {
    if (double_buffering_enabled && back_buffer) {
        return back_buffer;
    }
    return fb_info.address;
}

/**
 * Draw a character with anti-aliasing using 5x7 font
 */
void framebuffer_draw_char_antialiased(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color) {
    if (!fb_available || c < 0 || c >= 128) {
        return;
    }
    
    // Draw character with 2x2 supersampling for anti-aliasing
    for (uint32_t row = 0; row < 7; row++) {
        uint8_t bitmap = font5x7[(int)c][row];
        
        for (uint32_t col = 0; col < 5; col++) {
            uint8_t pixel = (bitmap >> col) & 1;
            
            if (pixel) {
                // Draw with edge smoothing
                // Check neighboring pixels for edge detection
                uint8_t left = (col > 0) ? ((bitmap >> (col - 1)) & 1) : 0;
                uint8_t right = (col < 4) ? ((bitmap >> (col + 1)) & 1) : 0;
                uint8_t top = (row > 0) ? ((font5x7[(int)c][row - 1] >> col) & 1) : 0;
                uint8_t bottom = (row < 6) ? ((font5x7[(int)c][row + 1] >> col) & 1) : 0;
                
                // Calculate edge factor (how many neighbors are filled)
                uint8_t neighbors = left + right + top + bottom;
                
                // Full pixel
                framebuffer_draw_pixel(x + col, y + row, fg_color);
                
                // Anti-alias edges by drawing semi-transparent pixels at borders
                if (neighbors < 4) {
                    uint8_t alpha = 128;  // Semi-transparent
                    
                    // Add edge smoothing pixels
                    if (!left && col > 0) {
                        color_t edge = fg_color;
                        edge.a = alpha;
                        // Would need alpha blending here - simplified for now
                    }
                    if (!right && col < 4) {
                        color_t edge = fg_color;
                        edge.a = alpha;
                        // Would need alpha blending here - simplified for now
                    }
                }
            } else {
                // Background pixel
                framebuffer_draw_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

/**
 * Draw a string with anti-aliasing
 */
void framebuffer_draw_string_antialiased(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color) {
    if (!fb_available || !str) {
        return;
    }
    
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;
    
    while (*str) {
        if (*str == '\n') {
            cursor_x = x;
            cursor_y += 7;
        } else if (*str == '\t') {
            cursor_x += 6 * 4;  // Tab = 4 spaces
        } else {
            if (cursor_y + 7 <= fb_info.height) {
                framebuffer_draw_char_antialiased(cursor_x, cursor_y, *str, fg_color, bg_color);
            }
            cursor_x += 6;  // 5 pixels + 1 spacing
            
            if (cursor_x + 6 > fb_info.width) {
                cursor_x = x;
                cursor_y += 7;
            }
        }
        
        str++;
    }
}
