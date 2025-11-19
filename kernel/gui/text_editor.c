/**
 * Aurora OS - Text Editor Application Implementation
 * 
 * Full-featured text editor for Aurora OS
 */

#include "text_editor.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../../filesystem/vfs/vfs.h"
#include <stddef.h>

// String utilities
static size_t te_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void te_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int te_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void te_strcat(char* dest, const char* src, size_t max) {
    size_t dest_len = te_strlen(dest);
    size_t i = 0;
    while (src && src[i] && (dest_len + i) < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

// Text editor state
static text_editor_state_t editor_state;
static uint8_t editor_initialized = 0;

void text_editor_init(void) {
    if (editor_initialized) return;
    
    editor_state.window = NULL;
    editor_state.buffer.line_count = 1;
    editor_state.buffer.cursor_line = 0;
    editor_state.buffer.cursor_col = 0;
    editor_state.buffer.scroll_offset = 0;
    editor_state.buffer.mode = EDITOR_MODE_INSERT;
    editor_state.show_line_numbers = 1;
    editor_state.syntax_highlighting = 0;
    editor_state.word_wrap = 0;
    
    // Initialize with empty line
    editor_state.buffer.lines[0][0] = '\0';
    
    // Initialize file info
    te_strcpy(editor_state.buffer.file_info.filename, "Untitled.txt", 256);
    editor_state.buffer.file_info.modified = 0;
    editor_state.buffer.file_info.file_size = 0;
    
    editor_initialized = 1;
}

window_t* text_editor_create(void) {
    if (!editor_initialized) {
        text_editor_init();
    }
    
    window_t* window = gui_create_window("Text Editor - Untitled.txt", 120, 80, 800, 600);
    if (!window) return NULL;
    
    editor_state.window = window;
    
    // Create menu bar
    gui_create_button(window, "File", 10, 10, 60, 25);
    gui_create_button(window, "Edit", 75, 10, 60, 25);
    gui_create_button(window, "View", 140, 10, 60, 25);
    gui_create_button(window, "Help", 205, 10, 60, 25);
    
    // Create toolbar
    gui_create_button(window, "New", 10, 40, 50, 25);
    gui_create_button(window, "Open", 65, 40, 50, 25);
    gui_create_button(window, "Save", 120, 40, 50, 25);
    gui_create_button(window, "Undo", 185, 40, 50, 25);
    gui_create_button(window, "Redo", 240, 40, 50, 25);
    
    // Status bar information
    gui_create_label(window, "Line: 1  Col: 1  |  INSERT  |  UTF-8  |  Ready", 10, 560);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    text_editor_update_display();
    
    return window;
}

window_t* text_editor_create_with_file(const char* filename) {
    window_t* window = text_editor_create();
    if (!window) return NULL;
    
    if (text_editor_open_file(filename) == 0) {
        // Update window title
        char title[300];
        te_strcpy(title, "Text Editor - ", 300);
        te_strcat(title, filename, 300);
        // Note: Would need gui_set_window_title() function
    }
    
    return window;
}

void text_editor_destroy(window_t* window) {
    if (window) {
        gui_close_window(window);
    }
    editor_state.window = NULL;
}

void text_editor_process_input(uint32_t key) {
    // Handle special keys
    if (key == 0x1C) {  // Enter
        text_editor_insert_newline();
    } else if (key == 0x0E) {  // Backspace
        text_editor_delete_char();
    } else if (key == 0x48) {  // Up arrow
        text_editor_cursor_up();
    } else if (key == 0x50) {  // Down arrow
        text_editor_cursor_down();
    } else if (key == 0x4B) {  // Left arrow
        text_editor_cursor_left();
    } else if (key == 0x4D) {  // Right arrow
        text_editor_cursor_right();
    } else if (key == 0x47) {  // Home
        text_editor_cursor_home();
    } else if (key == 0x4F) {  // End
        text_editor_cursor_end();
    } else if (key >= 32 && key < 127) {  // Printable characters
        text_editor_insert_char((char)key);
    }
    
    text_editor_update_display();
}

int text_editor_open_file(const char* filename) {
    // In a real implementation, this would use VFS to read the file
    // For now, load some sample content
    
    te_strcpy(editor_state.buffer.file_info.filename, filename, 256);
    editor_state.buffer.file_info.modified = 0;
    
    // Clear buffer and add sample content
    text_editor_clear_buffer();
    
    te_strcpy(editor_state.buffer.lines[0], "// Aurora OS Text Editor", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[1], "// File opened: ", EDITOR_MAX_LINE_LENGTH);
    te_strcat(editor_state.buffer.lines[1], filename, EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[2], "", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[3], "This is a sample file content.", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[4], "The text editor supports:", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[5], "- Line editing", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[6], "- Cursor movement", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[7], "- Insert and delete operations", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[8], "- Line numbers", EDITOR_MAX_LINE_LENGTH);
    te_strcpy(editor_state.buffer.lines[9], "- File operations (save, load)", EDITOR_MAX_LINE_LENGTH);
    
    editor_state.buffer.line_count = 10;
    editor_state.buffer.cursor_line = 0;
    editor_state.buffer.cursor_col = 0;
    editor_state.buffer.scroll_offset = 0;
    
    return 0;
}

int text_editor_save_file(void) {
    // In a real implementation, this would use VFS to write the file
    editor_state.buffer.file_info.modified = 0;
    return 0;
}

int text_editor_save_file_as(const char* filename) {
    te_strcpy(editor_state.buffer.file_info.filename, filename, 256);
    return text_editor_save_file();
}

void text_editor_insert_char(char c) {
    text_buffer_t* buf = &editor_state.buffer;
    
    if (buf->cursor_line >= EDITOR_MAX_LINES) return;
    if (buf->cursor_col >= EDITOR_MAX_LINE_LENGTH - 1) return;
    
    char* line = buf->lines[buf->cursor_line];
    size_t line_len = te_strlen(line);
    
    // Shift characters right
    for (size_t i = line_len; i > buf->cursor_col; i--) {
        line[i] = line[i - 1];
    }
    
    // Insert character
    line[buf->cursor_col] = c;
    if (buf->cursor_col == line_len) {
        line[line_len + 1] = '\0';
    }
    
    buf->cursor_col++;
    buf->file_info.modified = 1;
}

void text_editor_delete_char(void) {
    text_buffer_t* buf = &editor_state.buffer;
    
    if (buf->cursor_col == 0) {
        // Delete at beginning of line - merge with previous line
        if (buf->cursor_line > 0) {
            size_t prev_len = te_strlen(buf->lines[buf->cursor_line - 1]);
            if (prev_len + te_strlen(buf->lines[buf->cursor_line]) < EDITOR_MAX_LINE_LENGTH) {
                te_strcat(buf->lines[buf->cursor_line - 1], 
                         buf->lines[buf->cursor_line], 
                         EDITOR_MAX_LINE_LENGTH);
                
                // Shift lines up
                for (uint32_t i = buf->cursor_line; i < buf->line_count - 1; i++) {
                    te_strcpy(buf->lines[i], buf->lines[i + 1], EDITOR_MAX_LINE_LENGTH);
                }
                
                buf->line_count--;
                buf->cursor_line--;
                buf->cursor_col = (uint32_t)prev_len;
                buf->file_info.modified = 1;
            }
        }
        return;
    }
    
    char* line = buf->lines[buf->cursor_line];
    size_t line_len = te_strlen(line);
    
    if (buf->cursor_col > 0 && buf->cursor_col <= line_len) {
        // Shift characters left
        for (size_t i = buf->cursor_col - 1; i < line_len; i++) {
            line[i] = line[i + 1];
        }
        buf->cursor_col--;
        buf->file_info.modified = 1;
    }
}

void text_editor_insert_newline(void) {
    text_buffer_t* buf = &editor_state.buffer;
    
    if (buf->line_count >= EDITOR_MAX_LINES) return;
    
    // Shift lines down
    for (uint32_t i = buf->line_count; i > buf->cursor_line + 1; i--) {
        te_strcpy(buf->lines[i], buf->lines[i - 1], EDITOR_MAX_LINE_LENGTH);
    }
    
    // Split current line
    char* current_line = buf->lines[buf->cursor_line];
    char* new_line = buf->lines[buf->cursor_line + 1];
    
    // Copy text after cursor to new line
    te_strcpy(new_line, &current_line[buf->cursor_col], EDITOR_MAX_LINE_LENGTH);
    
    // Terminate current line at cursor
    current_line[buf->cursor_col] = '\0';
    
    buf->line_count++;
    buf->cursor_line++;
    buf->cursor_col = 0;
    buf->file_info.modified = 1;
}

void text_editor_cursor_up(void) {
    if (editor_state.buffer.cursor_line > 0) {
        editor_state.buffer.cursor_line--;
        
        // Adjust column if line is shorter
        size_t line_len = te_strlen(editor_state.buffer.lines[editor_state.buffer.cursor_line]);
        if (editor_state.buffer.cursor_col > line_len) {
            editor_state.buffer.cursor_col = (uint32_t)line_len;
        }
        
        // Adjust scroll if needed
        if (editor_state.buffer.cursor_line < editor_state.buffer.scroll_offset) {
            editor_state.buffer.scroll_offset = editor_state.buffer.cursor_line;
        }
    }
}

void text_editor_cursor_down(void) {
    if (editor_state.buffer.cursor_line < editor_state.buffer.line_count - 1) {
        editor_state.buffer.cursor_line++;
        
        // Adjust column if line is shorter
        size_t line_len = te_strlen(editor_state.buffer.lines[editor_state.buffer.cursor_line]);
        if (editor_state.buffer.cursor_col > line_len) {
            editor_state.buffer.cursor_col = (uint32_t)line_len;
        }
        
        // Adjust scroll if needed
        if (editor_state.buffer.cursor_line >= editor_state.buffer.scroll_offset + EDITOR_VISIBLE_LINES) {
            editor_state.buffer.scroll_offset = editor_state.buffer.cursor_line - EDITOR_VISIBLE_LINES + 1;
        }
    }
}

void text_editor_cursor_left(void) {
    if (editor_state.buffer.cursor_col > 0) {
        editor_state.buffer.cursor_col--;
    } else if (editor_state.buffer.cursor_line > 0) {
        // Move to end of previous line
        editor_state.buffer.cursor_line--;
        editor_state.buffer.cursor_col = (uint32_t)te_strlen(
            editor_state.buffer.lines[editor_state.buffer.cursor_line]);
    }
}

void text_editor_cursor_right(void) {
    size_t line_len = te_strlen(editor_state.buffer.lines[editor_state.buffer.cursor_line]);
    if (editor_state.buffer.cursor_col < line_len) {
        editor_state.buffer.cursor_col++;
    } else if (editor_state.buffer.cursor_line < editor_state.buffer.line_count - 1) {
        // Move to beginning of next line
        editor_state.buffer.cursor_line++;
        editor_state.buffer.cursor_col = 0;
    }
}

void text_editor_cursor_home(void) {
    editor_state.buffer.cursor_col = 0;
}

void text_editor_cursor_end(void) {
    editor_state.buffer.cursor_col = (uint32_t)te_strlen(
        editor_state.buffer.lines[editor_state.buffer.cursor_line]);
}

void text_editor_update_display(void) {
    if (!editor_state.window) return;
    
    // This would redraw the editor content in a real implementation
    // For now, it's a placeholder for the GUI update logic
    
    // The actual rendering would:
    // 1. Clear the text area
    // 2. Draw line numbers if enabled
    // 3. Draw visible lines with syntax highlighting
    // 4. Draw cursor
    // 5. Update status bar
}

void text_editor_toggle_line_numbers(void) {
    editor_state.show_line_numbers = !editor_state.show_line_numbers;
    text_editor_update_display();
}

void text_editor_clear_buffer(void) {
    editor_state.buffer.line_count = 1;
    editor_state.buffer.cursor_line = 0;
    editor_state.buffer.cursor_col = 0;
    editor_state.buffer.scroll_offset = 0;
    editor_state.buffer.lines[0][0] = '\0';
    editor_state.buffer.file_info.modified = 0;
}

uint32_t text_editor_get_line_count(void) {
    return editor_state.buffer.line_count;
}

void text_editor_get_cursor_pos(uint32_t* line, uint32_t* col) {
    if (line) *line = editor_state.buffer.cursor_line;
    if (col) *col = editor_state.buffer.cursor_col;
}
