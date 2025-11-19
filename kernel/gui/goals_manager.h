/**
 * Aurora OS - Goals Manager Application Header
 * 
 * Manages and tracks long-term development goals
 */

#ifndef GOALS_MANAGER_H
#define GOALS_MANAGER_H

#include <stdint.h>
#include "gui.h"

#define MAX_GOALS 50
#define GOAL_NAME_MAX 128
#define GOAL_CATEGORY_MAX 64

// Goal status
typedef enum {
    GOAL_STATUS_PENDING,
    GOAL_STATUS_COMPLETED
} goal_status_t;

// Goal structure
typedef struct {
    char name[GOAL_NAME_MAX];
    char category[GOAL_CATEGORY_MAX];
    goal_status_t status;
    uint8_t visible;  // For filtering
} goal_t;

// Goals manager state
typedef struct {
    window_t* window;
    goal_t goals[MAX_GOALS];
    uint32_t goal_count;
    uint32_t scroll_offset;
    uint8_t show_completed;  // Filter: show completed goals
} goals_manager_state_t;

/**
 * Initialize goals manager system
 */
void goals_manager_init(void);

/**
 * Create and show goals manager window
 * @return Pointer to goals manager window or NULL on failure
 */
window_t* goals_manager_create(void);

/**
 * Destroy goals manager window
 * @param window Goals manager window to destroy
 */
void goals_manager_destroy(window_t* window);

/**
 * Add a goal to the manager
 * @param name Goal name/description
 * @param category Goal category (e.g., "Phase 2", "Phase 3")
 * @param status Initial status
 * @return 0 on success, -1 on failure
 */
int goals_manager_add_goal(const char* name, const char* category, goal_status_t status);

/**
 * Toggle goal completion status
 * @param index Goal index
 */
void goals_manager_toggle_goal(uint32_t index);

/**
 * Get goal by index
 * @param index Goal index
 * @return Pointer to goal or NULL if invalid index
 */
goal_t* goals_manager_get_goal(uint32_t index);

/**
 * Update goals manager display
 */
void goals_manager_update_display(void);

/**
 * Toggle showing completed goals
 */
void goals_manager_toggle_filter(void);

/**
 * Scroll goals list
 * @param delta Scroll delta (positive = down, negative = up)
 */
void goals_manager_scroll(int32_t delta);

/**
 * Get total goal count
 * @return Total number of goals
 */
uint32_t goals_manager_get_count(void);

/**
 * Get completed goal count
 * @return Number of completed goals
 */
uint32_t goals_manager_get_completed_count(void);

#endif // GOALS_MANAGER_H
