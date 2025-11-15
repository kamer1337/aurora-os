/**
 * Aurora OS - Mouse Driver Header
 * 
 * PS/2 mouse driver for input handling
 */

#ifndef AURORA_MOUSE_H
#define AURORA_MOUSE_H

#include <stdint.h>

/* Mouse data structure */
typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
    int8_t delta_x;
    int8_t delta_y;
} mouse_state_t;

/* Mouse button flags */
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04

/* Mouse driver functions */
void mouse_init(void);
void mouse_handler(void);
mouse_state_t* mouse_get_state(void);
void mouse_set_bounds(uint32_t width, uint32_t height);

#endif /* AURORA_MOUSE_H */
