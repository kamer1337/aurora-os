/**
 * Aurora OS - Serial Port Driver Implementation
 * 
 * Serial port driver for debugging output
 */

#include "serial.h"

/**
 * Write byte to port
 */
static inline void outb(uint16_t port, uint8_t value) {
    /* In a real implementation, this would use inline assembly */
    /* For now, this is a stub */
    (void)port;
    (void)value;
}

/**
 * Read byte from port
 */
static inline uint8_t inb(uint16_t port) {
    /* In a real implementation, this would use inline assembly */
    /* For now, return 0 */
    (void)port;
    return 0;
}

/**
 * Initialize serial port
 */
void serial_init(uint16_t port) {
    /* Disable all interrupts */
    outb(port + SERIAL_INT_ENABLE_REG, 0x00);
    
    /* Enable DLAB (set baud rate divisor) */
    outb(port + SERIAL_LINE_CTRL_REG, 0x80);
    
    /* Set divisor to 3 (38400 baud) */
    outb(port + SERIAL_DATA_REG, 0x03);
    outb(port + SERIAL_INT_ENABLE_REG, 0x00);
    
    /* 8 bits, no parity, one stop bit */
    outb(port + SERIAL_LINE_CTRL_REG, 0x03);
    
    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(port + SERIAL_FIFO_CTRL_REG, 0xC7);
    
    /* IRQs enabled, RTS/DSR set */
    outb(port + SERIAL_MODEM_CTRL_REG, 0x0B);
}

/**
 * Check if transmit buffer is empty
 */
static int serial_can_transmit(uint16_t port) {
    return inb(port + SERIAL_LINE_STATUS_REG) & 0x20;
}

/**
 * Write character to serial port
 */
void serial_putchar(uint16_t port, char c) {
    /* Wait for transmit buffer to be empty */
    while (!serial_can_transmit(port)) {
        /* Wait */
    }
    
    /* Send character */
    outb(port + SERIAL_DATA_REG, (uint8_t)c);
}

/**
 * Check if data is available to read
 */
int serial_can_read(uint16_t port) {
    return inb(port + SERIAL_LINE_STATUS_REG) & 0x01;
}

/**
 * Read character from serial port
 */
char serial_getchar(uint16_t port) {
    /* Wait for data to be available */
    while (!serial_can_read(port)) {
        /* Wait */
    }
    
    /* Read and return character */
    return (char)inb(port + SERIAL_DATA_REG);
}

/**
 * Write string to serial port
 */
void serial_write(uint16_t port, const char* str) {
    if (!str) {
        return;
    }
    
    while (*str) {
        serial_putchar(port, *str);
        str++;
    }
}
