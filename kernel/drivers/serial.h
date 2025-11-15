/**
 * Aurora OS - Serial Port Driver Header
 * 
 * Serial port driver for debugging output
 */

#ifndef AURORA_SERIAL_H
#define AURORA_SERIAL_H

#include <stdint.h>

/* Serial ports */
#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8

/* Serial port registers */
#define SERIAL_DATA_REG       0
#define SERIAL_INT_ENABLE_REG 1
#define SERIAL_FIFO_CTRL_REG  2
#define SERIAL_LINE_CTRL_REG  3
#define SERIAL_MODEM_CTRL_REG 4
#define SERIAL_LINE_STATUS_REG 5

/* Serial driver functions */
void serial_init(uint16_t port);
void serial_putchar(uint16_t port, char c);
char serial_getchar(uint16_t port);
void serial_write(uint16_t port, const char* str);
int serial_can_read(uint16_t port);

#endif /* AURORA_SERIAL_H */
