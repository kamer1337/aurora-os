/**
 * Aurora OS - Calculator Application Header
 * 
 * Scientific calculator for Aurora OS
 */

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdint.h>
#include "gui.h"

// Calculator mode
typedef enum {
    CALC_MODE_BASIC,
    CALC_MODE_SCIENTIFIC,
    CALC_MODE_PROGRAMMER
} calc_mode_t;

// Calculator operation
typedef enum {
    CALC_OP_NONE,
    CALC_OP_ADD,
    CALC_OP_SUBTRACT,
    CALC_OP_MULTIPLY,
    CALC_OP_DIVIDE,
    CALC_OP_MODULO,
    CALC_OP_POWER,
    CALC_OP_SQRT,
    CALC_OP_SIN,
    CALC_OP_COS,
    CALC_OP_TAN
} calc_operation_t;

// Calculator state
typedef struct {
    window_t* window;
    double current_value;
    double stored_value;
    calc_operation_t pending_op;
    calc_mode_t mode;
    uint8_t clear_on_next;
    char display[32];
} calculator_state_t;

/**
 * Initialize calculator system
 */
void calculator_init(void);

/**
 * Create and show calculator window
 * @return Pointer to calculator window or NULL on failure
 */
window_t* calculator_create(void);

/**
 * Destroy calculator window
 * @param window Calculator window to destroy
 */
void calculator_destroy(window_t* window);

/**
 * Process calculator input
 * @param button Button pressed (0-9, +, -, *, /, =, C, etc.)
 */
void calculator_process_input(char button);

/**
 * Clear calculator
 */
void calculator_clear(void);

/**
 * Clear current entry
 */
void calculator_clear_entry(void);

/**
 * Add digit to display
 * @param digit Digit to add (0-9)
 */
void calculator_add_digit(uint8_t digit);

/**
 * Add decimal point
 */
void calculator_add_decimal(void);

/**
 * Set operation
 * @param op Operation to set
 */
void calculator_set_operation(calc_operation_t op);

/**
 * Calculate result
 */
void calculator_calculate(void);

/**
 * Update calculator display
 */
void calculator_update_display(void);

/**
 * Switch calculator mode
 * @param mode Mode to switch to
 */
void calculator_set_mode(calc_mode_t mode);

#endif // CALCULATOR_H
