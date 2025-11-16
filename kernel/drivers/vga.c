/**
 * Aurora OS - VGA Text Mode Driver Implementation
 * 
 * VGA text mode driver for display output
 */

#include "vga.h"

/* VGA state */
static uint16_t* vga_buffer;
static uint8_t vga_row;
static uint8_t vga_column;
static uint8_t vga_color;

/**
 * Create VGA entry with character and color
 */
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

/**
 * Create VGA color attribute
 */
static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

/**
 * Initialize VGA driver
 */
void vga_init(void) {
    vga_buffer = (uint16_t*) VGA_MEMORY;
    vga_row = 0;
    vga_column = 0;
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

/**
 * Clear screen
 */
void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_column = 0;
}

/**
 * Scroll screen up by one line
 */
static void vga_scroll(void) {
    /* Move all rows up */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear last row */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
    }
    
    vga_row = VGA_HEIGHT - 1;
}

/**
 * Put character at position
 */
static void vga_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

/**
 * Put character on screen
 */
void vga_putchar(char c) {
    /* Handle special characters */
    if (c == '\n') {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
        return;
    } else if (c == '\r') {
        vga_column = 0;
        return;
    } else if (c == '\t') {
        vga_column = (vga_column + 8) & ~7;
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            if (++vga_row == VGA_HEIGHT) {
                vga_scroll();
            }
        }
        return;
    } else if (c == '\b') {
        if (vga_column > 0) {
            vga_column--;
            vga_putentryat(' ', vga_color, vga_column, vga_row);
        }
        return;
    }
    
    /* Put character */
    vga_putentryat(c, vga_color, vga_column, vga_row);
    
    /* Advance cursor */
    if (++vga_column == VGA_WIDTH) {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
    }
}

/**
 * Write string to screen
 */
void vga_write(const char* str) {
    if (!str) {
        return;
    }
    
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

/**
 * Write string with specific color
 */
void vga_write_color(const char* str, vga_color_t fg, vga_color_t bg) {
    uint8_t old_color = vga_color;
    vga_color = vga_entry_color(fg, bg);
    vga_write(str);
    vga_color = old_color;
}

/**
 * Set text color
 */
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = vga_entry_color(fg, bg);
}

/**
 * Set cursor position
 */
void vga_set_cursor(uint8_t x, uint8_t y) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        vga_column = x;
        vga_row = y;
    }
}

/**
 * Write hexadecimal value
 */
void vga_write_hex(uint32_t value) {
    char hex_str[11]; /* "0x" + 8 digits + null */
    const char hex_chars[] = "0123456789ABCDEF";
    
    hex_str[0] = '0';
    hex_str[1] = 'x';
    
    for (int i = 7; i >= 0; i--) {
        hex_str[9 - i] = hex_chars[(value >> (i * 4)) & 0xF];
    }
    hex_str[10] = '\0';
    
    vga_write(hex_str);
}

/**
 * Write decimal value
 */
void vga_write_dec(int value) {
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    
    if (value < 0) {
        vga_putchar('-');
        value = -value;
    }
    
    char buffer[12]; /* Max 10 digits + sign + null */
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    /* Reverse and print */
    while (i > 0) {
        vga_putchar(buffer[--i]);
    }
}

