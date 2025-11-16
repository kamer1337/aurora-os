/**
 * Aurora OS - Mouse Driver Implementation
 * 
 * PS/2 mouse driver for input handling
 */

#include "mouse.h"
#include "../core/port_io.h"
#include <stddef.h>

/* PS/2 controller ports */
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

/* PS/2 controller commands */
#define PS2_CMD_ENABLE_MOUSE  0xA8
#define PS2_CMD_WRITE_MOUSE   0xD4

/* Mouse commands */
#define MOUSE_CMD_ENABLE      0xF4
#define MOUSE_CMD_SET_DEFAULTS 0xF6

/* Mouse packet state */
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

/* Mouse state */
static mouse_state_t mouse_state = {0};
static uint32_t screen_width = 1024;
static uint32_t screen_height = 768;

/**
 * Wait for PS/2 controller to be ready for writing
 */
static void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (!(inb(PS2_STATUS_PORT) & 0x02)) {
            return;
        }
    }
}

/**
 * Wait for PS/2 controller to have data ready for reading
 */
static void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            return;
        }
    }
}

/**
 * Write byte to mouse
 */
static void mouse_write(uint8_t data) {
    mouse_wait_write();
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_MOUSE);
    mouse_wait_write();
    outb(PS2_DATA_PORT, data);
}

/**
 * Read byte from mouse
 */
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(PS2_DATA_PORT);
}

/**
 * Initialize mouse driver
 */
void mouse_init(void) {
    /* Enable auxiliary PS/2 device (mouse) */
    mouse_wait_write();
    outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_MOUSE);
    
    /* Enable interrupts and set defaults */
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    mouse_read(); // Acknowledge
    
    mouse_write(MOUSE_CMD_ENABLE);
    mouse_read(); // Acknowledge
    
    /* Initialize mouse state */
    mouse_state.x = screen_width / 2;
    mouse_state.y = screen_height / 2;
    mouse_state.buttons = 0;
    mouse_state.delta_x = 0;
    mouse_state.delta_y = 0;
    mouse_cycle = 0;
}

/**
 * Mouse interrupt handler
 */
void mouse_handler(void) {
    uint8_t status = inb(PS2_STATUS_PORT);
    
    /* Check if data is from mouse */
    if (!(status & 0x20)) {
        return;
    }
    
    uint8_t data = inb(PS2_DATA_PORT);
    
    /* Build mouse packet (3 bytes) */
    switch (mouse_cycle) {
        case 0:
            /* First byte - buttons and flags */
            if (!(data & 0x08)) {
                /* Invalid packet, wait for sync */
                return;
            }
            mouse_packet[0] = data;
            mouse_cycle++;
            break;
            
        case 1:
            /* Second byte - X movement */
            mouse_packet[1] = data;
            mouse_cycle++;
            break;
            
        case 2:
            /* Third byte - Y movement */
            mouse_packet[2] = data;
            mouse_cycle = 0;
            
            /* Parse packet */
            mouse_state.buttons = mouse_packet[0] & 0x07;
            
            /* Get X and Y deltas */
            int16_t delta_x = mouse_packet[1];
            int16_t delta_y = mouse_packet[2];
            
            /* Apply sign extension if negative */
            if (mouse_packet[0] & 0x10) {
                delta_x |= 0xFF00;
            }
            if (mouse_packet[0] & 0x20) {
                delta_y |= 0xFF00;
            }
            
            /* Update position */
            mouse_state.x += delta_x;
            mouse_state.y -= delta_y; // Y is inverted
            
            /* Clamp to screen bounds */
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.x >= (int32_t)screen_width) mouse_state.x = screen_width - 1;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.y >= (int32_t)screen_height) mouse_state.y = screen_height - 1;
            
            /* Store deltas */
            mouse_state.delta_x = delta_x;
            mouse_state.delta_y = -delta_y; // Y is inverted
            break;
    }
}

/**
 * Get current mouse state
 */
mouse_state_t* mouse_get_state(void) {
    return &mouse_state;
}

/**
 * Set screen bounds for mouse movement
 */
void mouse_set_bounds(uint32_t width, uint32_t height) {
    screen_width = width;
    screen_height = height;
}
