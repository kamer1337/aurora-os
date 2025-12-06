/**
 * @file workspace_preview.h
 * @brief Workspace Preview Thumbnails for Virtual Desktop Switcher
 * 
 * Provides live thumbnail previews of workspace contents
 */

#ifndef WORKSPACE_PREVIEW_H
#define WORKSPACE_PREVIEW_H

#include <stdint.h>
#include <stdbool.h>
#include "gui.h"

/* Preview configuration */
#define PREVIEW_WIDTH       200
#define PREVIEW_HEIGHT      150
#define PREVIEW_MARGIN      10
#define PREVIEW_BORDER      2
#define MAX_WORKSPACE_PREVIEWS  8

/* Workspace preview structure */
typedef struct {
    uint32_t workspace_id;
    uint32_t thumbnail[PREVIEW_WIDTH * PREVIEW_HEIGHT];  /* Scaled framebuffer */
    bool dirty;                    /* Needs update */
    bool active;                   /* Currently active workspace */
    bool hovered;                  /* Mouse hovering */
    
    /* Display info */
    char name[64];
    char os_type[32];              /* "Aurora OS", "Linux VM", "Android VM" */
    uint32_t window_count;
    
    /* Position */
    int32_t x, y;
    uint32_t width, height;
} workspace_preview_t;

/* Workspace preview manager */
typedef struct {
    workspace_preview_t previews[MAX_WORKSPACE_PREVIEWS];
    uint32_t preview_count;
    uint32_t active_workspace;
    bool visible;
    
    /* Layout */
    int32_t panel_x, panel_y;
    uint32_t panel_width, panel_height;
    
    /* Timing */
    uint32_t update_interval_ms;
    uint64_t last_update;
} workspace_preview_manager_t;

/**
 * Initialize workspace preview system
 * @return 0 on success, -1 on failure
 */
int workspace_preview_init(void);

/**
 * Shutdown workspace preview system
 */
void workspace_preview_shutdown(void);

/**
 * Update all workspace thumbnails
 */
void workspace_preview_update_all(void);

/**
 * Update a specific workspace thumbnail
 * @param workspace_id Workspace ID to update
 */
void workspace_preview_update(uint32_t workspace_id);

/**
 * Render workspace preview panel
 */
void workspace_preview_render(void);

/**
 * Show workspace preview panel
 */
void workspace_preview_show(void);

/**
 * Hide workspace preview panel
 */
void workspace_preview_hide(void);

/**
 * Toggle workspace preview panel visibility
 */
void workspace_preview_toggle(void);

/**
 * Handle mouse click on preview panel
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 * @return Workspace ID if clicked, -1 otherwise
 */
int workspace_preview_handle_click(int32_t x, int32_t y);

/**
 * Handle mouse movement over preview panel
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 */
void workspace_preview_handle_hover(int32_t x, int32_t y);

/**
 * Set workspace preview name
 * @param workspace_id Workspace ID
 * @param name New name
 */
void workspace_preview_set_name(uint32_t workspace_id, const char* name);

/**
 * Set workspace OS type
 * @param workspace_id Workspace ID
 * @param os_type OS type string
 */
void workspace_preview_set_os_type(uint32_t workspace_id, const char* os_type);

/**
 * Mark workspace as dirty (needs update)
 * @param workspace_id Workspace ID
 */
void workspace_preview_mark_dirty(uint32_t workspace_id);

/**
 * Set active workspace
 * @param workspace_id Workspace ID
 */
void workspace_preview_set_active(uint32_t workspace_id);

#endif /* WORKSPACE_PREVIEW_H */
