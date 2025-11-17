/**
 * Aurora OS - Web Browser Application Header
 * 
 * Basic web browser with HTML rendering support
 */

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <stdint.h>
#include "gui.h"

// Browser configuration
#define BROWSER_MAX_URL_LENGTH 512
#define BROWSER_MAX_HISTORY 50
#define BROWSER_MAX_BOOKMARKS 100
#define BROWSER_MAX_TABS 10

// Browser tab structure
typedef struct browser_tab {
    char url[BROWSER_MAX_URL_LENGTH];
    char title[128];
    uint8_t loading;
    uint8_t active;
    struct browser_tab* next;
} browser_tab_t;

// Browser state
typedef struct {
    window_t* window;
    browser_tab_t* tabs;
    int tab_count;
    browser_tab_t* active_tab;
    char history[BROWSER_MAX_HISTORY][BROWSER_MAX_URL_LENGTH];
    int history_count;
    int history_index;
    char bookmarks[BROWSER_MAX_BOOKMARKS][BROWSER_MAX_URL_LENGTH];
    int bookmark_count;
} browser_state_t;

/**
 * Initialize browser system
 */
void browser_init(void);

/**
 * Create and show browser window
 * @return Pointer to browser window or NULL on failure
 */
window_t* browser_create(void);

/**
 * Destroy browser window
 * @param window Browser window to destroy
 */
void browser_destroy(window_t* window);

/**
 * Navigate to URL
 * @param url URL to navigate to
 * @return 0 on success, -1 on error
 */
int browser_navigate(const char* url);

/**
 * Go back in history
 */
void browser_back(void);

/**
 * Go forward in history
 */
void browser_forward(void);

/**
 * Refresh current page
 */
void browser_refresh(void);

/**
 * Stop loading current page
 */
void browser_stop(void);

/**
 * Create new tab
 * @param url Initial URL for new tab (can be NULL)
 * @return 0 on success, -1 on error
 */
int browser_new_tab(const char* url);

/**
 * Close current tab
 */
void browser_close_tab(void);

/**
 * Switch to next tab
 */
void browser_next_tab(void);

/**
 * Switch to previous tab
 */
void browser_prev_tab(void);

/**
 * Add bookmark
 * @param url URL to bookmark
 * @return 0 on success, -1 on error
 */
int browser_add_bookmark(const char* url);

/**
 * Update browser display
 */
void browser_update_display(void);

/**
 * Render HTML content (basic)
 * @param html HTML content to render
 */
void browser_render_html(const char* html);

#endif // WEBVIEW_H
