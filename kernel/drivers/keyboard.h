/**
 * Aurora OS - Keyboard Driver Header
 * 
 * PS/2 keyboard driver for input handling
 */

#ifndef AURORA_KEYBOARD_H
#define AURORA_KEYBOARD_H

#include <stdint.h>

/* Keyboard ports */
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

/* Keyboard status flags */
#define KEYBOARD_STATUS_OUTPUT_FULL 0x01
#define KEYBOARD_STATUS_INPUT_FULL  0x02

/* Special keys */
#define KEY_ESC       0x01
#define KEY_BACKSPACE 0x0E
#define KEY_TAB       0x0F
#define KEY_ENTER     0x1C
#define KEY_CTRL      0x1D
#define KEY_LSHIFT    0x2A
#define KEY_RSHIFT    0x36
#define KEY_ALT       0x38
#define KEY_CAPSLOCK  0x3A

/* Keyboard driver functions */
void keyboard_init(void);
void keyboard_handler(void);
char keyboard_getchar(void);
int keyboard_has_input(void);

#endif /* AURORA_KEYBOARD_H */
