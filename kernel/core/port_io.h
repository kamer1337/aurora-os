/**
 * Aurora OS - Port I/O Operations
 * 
 * Inline assembly functions for x86 port I/O
 */

#ifndef PORT_IO_H
#define PORT_IO_H

#include <stdint.h>

/**
 * Write byte to I/O port
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read byte from I/O port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write word to I/O port
 */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read word from I/O port
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write double word to I/O port
 */
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read double word from I/O port
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Wait for I/O operation to complete (I/O delay)
 */
static inline void io_wait(void) {
    /* Port 0x80 is used for POST codes and is safe for delays */
    outb(0x80, 0);
}

#endif /* PORT_IO_H */
