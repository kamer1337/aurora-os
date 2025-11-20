/**
 * Aurora OS - Virtual Desktop Manager
 * 
 * Multiple workspace/virtual desktop support
 */

#ifndef VIRTUAL_DESKTOP_H
#define VIRTUAL_DESKTOP_H

#include <stdint.h>
#include "gui.h"

#define MAX_WORKSPACES 4
#define MAX_WINDOWS_PER_WORKSPACE 32

// Workspace OS type
typedef enum {
    WORKSPACE_OS_AURORA = 0,    // Native Aurora OS workspace
    WORKSPACE_OS_LINUX = 1,     // Linux VM workspace
} workspace_os_type_t;

// Workspace structure
typedef struct {
    uint8_t id;
    char name[32];
    uint8_t active;
    workspace_os_type_t os_type;    // OS type for this workspace
    window_t* windows[MAX_WINDOWS_PER_WORKSPACE];
    uint8_t window_count;
} workspace_t;

/**
 * Initialize virtual desktop system
 * @return 0 on success, -1 on failure
 */
int vdesktop_init(void);

/**
 * Create a new workspace
 * @param name Workspace name
 * @return Workspace ID or -1 on failure
 */
int vdesktop_create_workspace(const char* name);

/**
 * Switch to a workspace
 * @param workspace_id Workspace ID to switch to
 * @return 0 on success, -1 on failure
 */
int vdesktop_switch_to(uint8_t workspace_id);

/**
 * Get current active workspace
 * @return Current workspace ID
 */
uint8_t vdesktop_get_current(void);

/**
 * Move window to a workspace
 * @param window Window to move
 * @param workspace_id Target workspace ID
 * @return 0 on success, -1 on failure
 */
int vdesktop_move_window(window_t* window, uint8_t workspace_id);

/**
 * Get workspace by ID
 * @param workspace_id Workspace ID
 * @return Pointer to workspace or NULL
 */
workspace_t* vdesktop_get_workspace(uint8_t workspace_id);

/**
 * Show workspace switcher UI
 */
void vdesktop_show_switcher(void);

/**
 * Handle keyboard shortcut for workspace switching
 * @param key Key code
 * @return 1 if handled, 0 otherwise
 */
int vdesktop_handle_shortcut(uint32_t key);

/**
 * Set workspace OS type
 * @param workspace_id Workspace ID
 * @param os_type OS type to set
 * @return 0 on success, -1 on failure
 */
int vdesktop_set_os_type(uint8_t workspace_id, workspace_os_type_t os_type);

/**
 * Get workspace OS type
 * @param workspace_id Workspace ID
 * @return OS type or -1 on failure
 */
int vdesktop_get_os_type(uint8_t workspace_id);

/**
 * Show OS type selector for workspace
 * @param workspace_id Workspace ID
 */
void vdesktop_show_os_selector(uint8_t workspace_id);

#endif // VIRTUAL_DESKTOP_H
