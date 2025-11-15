/**
 * Aurora OS - Keyboard Driver Implementation
 * 
 * PS/2 keyboard driver for input handling
 */

#include "keyboard.h"
#include <stddef.h>

/* Keyboard buffer */
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static size_t buffer_read_pos = 0;
static size_t buffer_write_pos = 0;

/* Keyboard state */
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int caps_lock = 0;

/* US QWERTY keyboard scancode to ASCII map */
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

/* Shifted characters */
static const char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

/**
 * Read byte from keyboard data port
 */
static inline uint8_t keyboard_read_data(void) {
    /* In a real implementation, this would use inb() */
    /* For now, return 0 as we don't have port I/O yet */
    return 0;
}

/**
 * Read byte from keyboard status port
 */
static inline uint8_t keyboard_read_status(void) {
    /* In a real implementation, this would use inb() */
    return 0;
}

/**
 * Write byte to keyboard command port
 */
static inline void keyboard_write_command(uint8_t command) {
    /* In a real implementation, this would use outb() */
    (void)command;
}

/**
 * Initialize keyboard driver
 */
void keyboard_init(void) {
    /* Clear buffer */
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    
    /* Reset state */
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    caps_lock = 0;
    
    /* Enable keyboard (send command 0xAE to command port) */
    keyboard_write_command(0xAE);
}

/**
 * Add character to keyboard buffer
 */
static void keyboard_buffer_add(char c) {
    size_t next_write = (buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
    
    /* Only add if buffer not full */
    if (next_write != buffer_read_pos) {
        keyboard_buffer[buffer_write_pos] = c;
        buffer_write_pos = next_write;
    }
}

/**
 * Convert scancode to ASCII character
 */
static char scancode_to_char(uint8_t scancode) {
    if (scancode >= sizeof(scancode_to_ascii)) {
        return 0;
    }
    
    char c;
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }
    
    /* Apply caps lock to letters */
    if (caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    return c;
}

/**
 * Keyboard interrupt handler
 */
void keyboard_handler(void) {
    uint8_t status = keyboard_read_status();
    
    /* Check if data is available */
    if (!(status & KEYBOARD_STATUS_OUTPUT_FULL)) {
        return;
    }
    
    uint8_t scancode = keyboard_read_data();
    
    /* Check if key released (high bit set) */
    if (scancode & 0x80) {
        /* Key released */
        scancode &= 0x7F;
        
        switch (scancode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = 0;
                break;
            case KEY_CTRL:
                ctrl_pressed = 0;
                break;
            case KEY_ALT:
                alt_pressed = 0;
                break;
        }
    } else {
        /* Key pressed */
        switch (scancode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = 1;
                break;
            case KEY_CTRL:
                ctrl_pressed = 1;
                break;
            case KEY_ALT:
                alt_pressed = 1;
                break;
            case KEY_CAPSLOCK:
                caps_lock = !caps_lock;
                break;
            default: {
                /* Convert to character and add to buffer */
                char c = scancode_to_char(scancode);
                if (c != 0) {
                    keyboard_buffer_add(c);
                }
                break;
            }
        }
    }
}

/**
 * Get character from keyboard buffer
 */
char keyboard_getchar(void) {
    /* Wait until character available */
    while (buffer_read_pos == buffer_write_pos) {
        /* In real implementation, would wait for interrupt */
    }
    
    char c = keyboard_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    
    return c;
}

/**
 * Check if keyboard has input available
 */
int keyboard_has_input(void) {
    return buffer_read_pos != buffer_write_pos;
}
