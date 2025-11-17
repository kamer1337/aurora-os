/**
 * Aurora OS - Terminal Application Header
 * 
 * Full-featured terminal with bash and PowerShell-style commands
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include "gui.h"

// Terminal configuration
#define TERMINAL_MAX_LINES 50
#define TERMINAL_MAX_LINE_LENGTH 256
#define TERMINAL_MAX_HISTORY 50
#define TERMINAL_MAX_ARGS 32

// Terminal shell types
typedef enum {
    SHELL_BASH,
    SHELL_POWERSHELL,
    SHELL_CMD
} shell_type_t;

// Command structure
typedef struct {
    const char* name;
    const char* alias;
    const char* description;
    int (*handler)(int argc, char** argv);
    shell_type_t shell_type;
} terminal_command_t;

// Terminal state
typedef struct {
    window_t* window;
    char lines[TERMINAL_MAX_LINES][TERMINAL_MAX_LINE_LENGTH];
    int line_count;
    int scroll_offset;
    char current_input[TERMINAL_MAX_LINE_LENGTH];
    int input_pos;
    char history[TERMINAL_MAX_HISTORY][TERMINAL_MAX_LINE_LENGTH];
    int history_count;
    int history_index;
    shell_type_t current_shell;
    char current_dir[256];
} terminal_state_t;

/**
 * Initialize terminal system
 */
void terminal_init(void);

/**
 * Create and show terminal window
 * @return Pointer to terminal window or NULL on failure
 */
window_t* terminal_create(void);

/**
 * Destroy terminal window
 * @param window Terminal window to destroy
 */
void terminal_destroy(window_t* window);

/**
 * Process terminal input
 * @param key Key code
 */
void terminal_process_input(uint32_t key);

/**
 * Execute a command
 * @param command Command string to execute
 * @return 0 on success, -1 on error
 */
int terminal_execute_command(const char* command);

/**
 * Print text to terminal
 * @param text Text to print
 */
void terminal_print(const char* text);

/**
 * Clear terminal screen
 */
void terminal_clear(void);

/**
 * Get current working directory
 * @return Current directory path
 */
const char* terminal_get_cwd(void);

/**
 * Change current working directory
 * @param path New directory path
 * @return 0 on success, -1 on error
 */
int terminal_chdir(const char* path);

/**
 * Switch shell type
 * @param shell_type New shell type
 */
void terminal_set_shell(shell_type_t shell_type);

/**
 * Get current shell type
 * @return Current shell type
 */
shell_type_t terminal_get_shell(void);

/**
 * Update terminal display
 */
void terminal_update_display(void);

#endif // TERMINAL_H
