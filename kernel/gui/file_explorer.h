/**
 * Aurora OS - File Explorer Header
 * 
 * Enhanced file explorer with extension visibility and hidden file support
 */

#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include <stdint.h>
#include "gui.h"

// File explorer configuration
#define EXPLORER_MAX_PATH 512
#define EXPLORER_MAX_FILES 256
#define EXPLORER_MAX_TABS 5

// File view modes
typedef enum {
    VIEW_MODE_LIST,
    VIEW_MODE_ICONS,
    VIEW_MODE_DETAILS
} view_mode_t;

// File entry structure
typedef struct {
    char name[128];
    char path[512];
    uint8_t is_directory;
    uint8_t is_hidden;
    uint32_t size;
    char extension[16];
} file_entry_t;

// File explorer tab
typedef struct explorer_tab {
    char path[EXPLORER_MAX_PATH];
    file_entry_t files[EXPLORER_MAX_FILES];
    int file_count;
    int selected_index;
    struct explorer_tab* next;
} explorer_tab_t;

// File explorer state
typedef struct {
    window_t* window;
    explorer_tab_t* tabs;
    explorer_tab_t* active_tab;
    int tab_count;
    view_mode_t view_mode;
    uint8_t show_extensions;
    uint8_t show_hidden;
    uint8_t show_system_files;
} explorer_state_t;

/**
 * Initialize file explorer
 */
void file_explorer_init(void);

/**
 * Create and show file explorer window
 * @param initial_path Initial directory path (NULL for current)
 * @return Pointer to explorer window or NULL on failure
 */
window_t* file_explorer_create(const char* initial_path);

/**
 * Destroy file explorer window
 * @param window Explorer window to destroy
 */
void file_explorer_destroy(window_t* window);

/**
 * Navigate to directory
 * @param path Directory path
 * @return 0 on success, -1 on error
 */
int file_explorer_navigate(const char* path);

/**
 * Refresh current directory listing
 */
void file_explorer_refresh(void);

/**
 * Go up to parent directory
 */
void file_explorer_go_up(void);

/**
 * Toggle file extension visibility
 */
void file_explorer_toggle_extensions(void);

/**
 * Toggle hidden file visibility
 */
void file_explorer_toggle_hidden(void);

/**
 * Toggle system file visibility
 */
void file_explorer_toggle_system(void);

/**
 * Set view mode
 * @param mode New view mode
 */
void file_explorer_set_view_mode(view_mode_t mode);

/**
 * Get current view mode
 * @return Current view mode
 */
view_mode_t file_explorer_get_view_mode(void);

/**
 * Create new folder
 * @param name Folder name
 * @return 0 on success, -1 on error
 */
int file_explorer_create_folder(const char* name);

/**
 * Delete selected file/folder
 * @return 0 on success, -1 on error
 */
int file_explorer_delete_selected(void);

/**
 * Rename selected file/folder
 * @param new_name New name
 * @return 0 on success, -1 on error
 */
int file_explorer_rename_selected(const char* new_name);

/**
 * Show properties of selected file/folder
 */
void file_explorer_show_properties(void);

/**
 * Copy selected file/folder
 * @return 0 on success, -1 on error
 */
int file_explorer_copy_selected(void);

/**
 * Paste copied file/folder
 * @return 0 on success, -1 on error
 */
int file_explorer_paste(void);

/**
 * Create new tab
 * @param path Initial path for new tab
 * @return 0 on success, -1 on error
 */
int file_explorer_new_tab(const char* path);

/**
 * Close current tab
 */
void file_explorer_close_tab(void);

/**
 * Switch to next tab
 */
void file_explorer_next_tab(void);

/**
 * Switch to previous tab
 */
void file_explorer_prev_tab(void);

/**
 * Update explorer display
 */
void file_explorer_update_display(void);

/**
 * Check if extensions are visible
 * @return 1 if visible, 0 otherwise
 */
int file_explorer_extensions_visible(void);

/**
 * Check if hidden files are visible
 * @return 1 if visible, 0 otherwise
 */
int file_explorer_hidden_visible(void);

#endif // FILE_EXPLORER_H
