/**
 * Aurora OS - Text Editor Application Header
 * 
 * Full-featured text editor for Aurora OS
 */

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <stdint.h>
#include "gui.h"

// Text editor configuration
#define EDITOR_MAX_LINES 1000
#define EDITOR_MAX_LINE_LENGTH 512
#define EDITOR_VISIBLE_LINES 20
#define EDITOR_TAB_SIZE 4

// Editor mode
typedef enum {
    EDITOR_MODE_NORMAL,
    EDITOR_MODE_INSERT,
    EDITOR_MODE_VISUAL
} editor_mode_t;

// File information
typedef struct {
    char filename[256];
    uint8_t modified;
    uint32_t file_size;
} file_info_t;

// Text buffer
typedef struct {
    char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LENGTH];
    uint32_t line_count;
    uint32_t cursor_line;
    uint32_t cursor_col;
    uint32_t scroll_offset;
    editor_mode_t mode;
    file_info_t file_info;
} text_buffer_t;

// Text editor state
typedef struct {
    window_t* window;
    text_buffer_t buffer;
    uint8_t show_line_numbers;
    uint8_t syntax_highlighting;
    uint8_t word_wrap;
} text_editor_state_t;

/**
 * Initialize text editor system
 */
void text_editor_init(void);

/**
 * Create and show text editor window
 * @return Pointer to editor window or NULL on failure
 */
window_t* text_editor_create(void);

/**
 * Create text editor with file loaded
 * @param filename File to open
 * @return Pointer to editor window or NULL on failure
 */
window_t* text_editor_create_with_file(const char* filename);

/**
 * Destroy text editor window
 * @param window Editor window to destroy
 */
void text_editor_destroy(window_t* window);

/**
 * Process editor input
 * @param key Key code
 */
void text_editor_process_input(uint32_t key);

/**
 * Open a file in the editor
 * @param filename File to open
 * @return 0 on success, -1 on error
 */
int text_editor_open_file(const char* filename);

/**
 * Save current file
 * @return 0 on success, -1 on error
 */
int text_editor_save_file(void);

/**
 * Save file with new name
 * @param filename New filename
 * @return 0 on success, -1 on error
 */
int text_editor_save_file_as(const char* filename);

/**
 * Insert character at cursor position
 * @param c Character to insert
 */
void text_editor_insert_char(char c);

/**
 * Delete character at cursor position
 */
void text_editor_delete_char(void);

/**
 * Insert new line at cursor position
 */
void text_editor_insert_newline(void);

/**
 * Move cursor up
 */
void text_editor_cursor_up(void);

/**
 * Move cursor down
 */
void text_editor_cursor_down(void);

/**
 * Move cursor left
 */
void text_editor_cursor_left(void);

/**
 * Move cursor right
 */
void text_editor_cursor_right(void);

/**
 * Move cursor to beginning of line
 */
void text_editor_cursor_home(void);

/**
 * Move cursor to end of line
 */
void text_editor_cursor_end(void);

/**
 * Update editor display
 */
void text_editor_update_display(void);

/**
 * Toggle line numbers display
 */
void text_editor_toggle_line_numbers(void);

/**
 * Clear editor buffer
 */
void text_editor_clear_buffer(void);

/**
 * Get line count
 * @return Number of lines in buffer
 */
uint32_t text_editor_get_line_count(void);

/**
 * Get cursor position
 * @param line Pointer to store line number
 * @param col Pointer to store column number
 */
void text_editor_get_cursor_pos(uint32_t* line, uint32_t* col);

#endif // TEXT_EDITOR_H
