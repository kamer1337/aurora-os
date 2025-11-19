/**
 * Aurora OS - Calculator Application Implementation
 * 
 * Scientific calculator for Aurora OS
 */

#include "calculator.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include <stddef.h>

// Math utilities (simplified implementations)
static double calc_sqrt(double x) {
    // Newton's method for square root
    if (x < 0) return 0.0;
    if (x == 0) return 0.0;
    
    double guess = x / 2.0;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

static double calc_power(double base, double exp) {
    // Simple power function (integer exponents only)
    if (exp == 0.0) return 1.0;
    if (exp < 0.0) return 1.0 / calc_power(base, -exp);
    
    double result = 1.0;
    int int_exp = (int)exp;
    for (int i = 0; i < int_exp; i++) {
        result *= base;
    }
    return result;
}

// String utilities
static size_t calc_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void calc_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void calc_strcat(char* dest, const char* src, size_t max) {
    size_t dest_len = calc_strlen(dest);
    size_t i = 0;
    while (src && src[i] && (dest_len + i) < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

// Convert double to string (simplified)
static void double_to_string(double value, char* buffer, size_t max) {
    int int_part = (int)value;
    double frac_part = value - (double)int_part;
    
    // Handle negative
    size_t pos = 0;
    if (int_part < 0) {
        buffer[pos++] = '-';
        int_part = -int_part;
        frac_part = -frac_part;
    }
    
    // Convert integer part
    char temp[20];
    int temp_pos = 0;
    if (int_part == 0) {
        temp[temp_pos++] = '0';
    } else {
        while (int_part > 0) {
            temp[temp_pos++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    
    // Reverse and copy
    for (int i = temp_pos - 1; i >= 0 && pos < max - 1; i--) {
        buffer[pos++] = temp[i];
    }
    
    // Add decimal point and fraction
    if (frac_part > 0.0001 && pos < max - 8) {
        buffer[pos++] = '.';
        for (int i = 0; i < 6 && pos < max - 1; i++) {
            frac_part *= 10.0;
            int digit = (int)frac_part;
            buffer[pos++] = '0' + digit;
            frac_part -= (double)digit;
        }
    }
    
    buffer[pos] = '\0';
}

// Calculator state
static calculator_state_t calc_state;
static uint8_t calc_initialized = 0;

void calculator_init(void) {
    if (calc_initialized) return;
    
    calc_state.window = NULL;
    calc_state.current_value = 0.0;
    calc_state.stored_value = 0.0;
    calc_state.pending_op = CALC_OP_NONE;
    calc_state.mode = CALC_MODE_BASIC;
    calc_state.clear_on_next = 0;
    calc_strcpy(calc_state.display, "0", 32);
    
    calc_initialized = 1;
}

window_t* calculator_create(void) {
    if (!calc_initialized) {
        calculator_init();
    }
    
    window_t* window = gui_create_window("Calculator", 250, 200, 320, 480);
    if (!window) return NULL;
    
    calc_state.window = window;
    
    // Display
    gui_create_label(window, calc_state.display, 20, 20);
    
    // Number pad
    int button_width = 60;
    int button_height = 50;
    int button_spacing = 10;
    int start_x = 20;
    int start_y = 120;
    
    // Row 1: 7 8 9 /
    gui_create_button(window, "7", start_x, start_y, button_width, button_height);
    gui_create_button(window, "8", start_x + (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "9", start_x + 2 * (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "/", start_x + 3 * (button_width + button_spacing), start_y, button_width, button_height);
    
    // Row 2: 4 5 6 *
    start_y += button_height + button_spacing;
    gui_create_button(window, "4", start_x, start_y, button_width, button_height);
    gui_create_button(window, "5", start_x + (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "6", start_x + 2 * (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "*", start_x + 3 * (button_width + button_spacing), start_y, button_width, button_height);
    
    // Row 3: 1 2 3 -
    start_y += button_height + button_spacing;
    gui_create_button(window, "1", start_x, start_y, button_width, button_height);
    gui_create_button(window, "2", start_x + (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "3", start_x + 2 * (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "-", start_x + 3 * (button_width + button_spacing), start_y, button_width, button_height);
    
    // Row 4: 0 . = +
    start_y += button_height + button_spacing;
    gui_create_button(window, "0", start_x, start_y, button_width, button_height);
    gui_create_button(window, ".", start_x + (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "=", start_x + 2 * (button_width + button_spacing), start_y, button_width, button_height);
    gui_create_button(window, "+", start_x + 3 * (button_width + button_spacing), start_y, button_width, button_height);
    
    // Top row: C CE sqrt
    gui_create_button(window, "C", 20, 70, 60, 40);
    gui_create_button(window, "CE", 90, 70, 60, 40);
    gui_create_button(window, "sqrt", 160, 70, 60, 40);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return window;
}

void calculator_destroy(window_t* window) {
    if (window) {
        gui_close_window(window);
    }
    calc_state.window = NULL;
}

void calculator_process_input(char button) {
    if (button >= '0' && button <= '9') {
        calculator_add_digit(button - '0');
    } else if (button == '.') {
        calculator_add_decimal();
    } else if (button == '+') {
        calculator_set_operation(CALC_OP_ADD);
    } else if (button == '-') {
        calculator_set_operation(CALC_OP_SUBTRACT);
    } else if (button == '*') {
        calculator_set_operation(CALC_OP_MULTIPLY);
    } else if (button == '/') {
        calculator_set_operation(CALC_OP_DIVIDE);
    } else if (button == '=') {
        calculator_calculate();
    } else if (button == 'C' || button == 'c') {
        calculator_clear();
    }
    
    calculator_update_display();
}

void calculator_clear(void) {
    calc_state.current_value = 0.0;
    calc_state.stored_value = 0.0;
    calc_state.pending_op = CALC_OP_NONE;
    calc_state.clear_on_next = 0;
    calc_strcpy(calc_state.display, "0", 32);
}

void calculator_clear_entry(void) {
    calc_state.current_value = 0.0;
    calc_state.clear_on_next = 0;
    calc_strcpy(calc_state.display, "0", 32);
}

void calculator_add_digit(uint8_t digit) {
    if (calc_state.clear_on_next) {
        calc_strcpy(calc_state.display, "0", 32);
        calc_state.clear_on_next = 0;
    }
    
    // Check if display is "0"
    if (calc_state.display[0] == '0' && calc_state.display[1] == '\0') {
        calc_state.display[0] = '0' + digit;
    } else if (calc_strlen(calc_state.display) < 31) {
        char digit_str[2] = {'0' + digit, '\0'};
        calc_strcat(calc_state.display, digit_str, 32);
    }
}

void calculator_add_decimal(void) {
    // Check if decimal already exists
    for (size_t i = 0; i < calc_strlen(calc_state.display); i++) {
        if (calc_state.display[i] == '.') return;
    }
    
    if (calc_strlen(calc_state.display) < 30) {
        calc_strcat(calc_state.display, ".", 32);
    }
}

void calculator_set_operation(calc_operation_t op) {
    // Parse current display
    double value = 0.0;
    int decimal_places = 0;
    uint8_t after_decimal = 0;
    
    for (size_t i = 0; i < calc_strlen(calc_state.display); i++) {
        char c = calc_state.display[i];
        if (c >= '0' && c <= '9') {
            if (after_decimal) {
                decimal_places++;
                value = value + (double)(c - '0') / calc_power(10.0, (double)decimal_places);
            } else {
                value = value * 10.0 + (double)(c - '0');
            }
        } else if (c == '.') {
            after_decimal = 1;
        }
    }
    
    calc_state.current_value = value;
    
    // If there's a pending operation, calculate it first
    if (calc_state.pending_op != CALC_OP_NONE) {
        calculator_calculate();
    } else {
        calc_state.stored_value = calc_state.current_value;
    }
    
    calc_state.pending_op = op;
    calc_state.clear_on_next = 1;
}

void calculator_calculate(void) {
    // Parse current display
    double value = 0.0;
    int decimal_places = 0;
    uint8_t after_decimal = 0;
    
    for (size_t i = 0; i < calc_strlen(calc_state.display); i++) {
        char c = calc_state.display[i];
        if (c >= '0' && c <= '9') {
            if (after_decimal) {
                decimal_places++;
                value = value + (double)(c - '0') / calc_power(10.0, (double)decimal_places);
            } else {
                value = value * 10.0 + (double)(c - '0');
            }
        } else if (c == '.') {
            after_decimal = 1;
        }
    }
    
    calc_state.current_value = value;
    
    double result = 0.0;
    switch (calc_state.pending_op) {
        case CALC_OP_ADD:
            result = calc_state.stored_value + calc_state.current_value;
            break;
        case CALC_OP_SUBTRACT:
            result = calc_state.stored_value - calc_state.current_value;
            break;
        case CALC_OP_MULTIPLY:
            result = calc_state.stored_value * calc_state.current_value;
            break;
        case CALC_OP_DIVIDE:
            if (calc_state.current_value != 0.0) {
                result = calc_state.stored_value / calc_state.current_value;
            } else {
                calc_strcpy(calc_state.display, "Error", 32);
                calc_state.pending_op = CALC_OP_NONE;
                return;
            }
            break;
        case CALC_OP_SQRT:
            result = calc_sqrt(calc_state.current_value);
            break;
        default:
            result = calc_state.current_value;
            break;
    }
    
    calc_state.current_value = result;
    calc_state.stored_value = result;
    double_to_string(result, calc_state.display, 32);
    calc_state.pending_op = CALC_OP_NONE;
    calc_state.clear_on_next = 1;
}

void calculator_update_display(void) {
    if (!calc_state.window) return;
    
    // In a real implementation, this would update the display label
    // For now, it's a placeholder
}

void calculator_set_mode(calc_mode_t mode) {
    calc_state.mode = mode;
}
