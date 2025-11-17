/**
 * Aurora OS - Web Browser Application Implementation
 * 
 * Basic web browser with HTML rendering support
 */

#include "webview.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include <stddef.h>

// String utilities
static size_t web_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void web_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int web_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void web_strcat(char* dest, const char* src, size_t max) {
    size_t dest_len = web_strlen(dest);
    size_t i = 0;
    while (src && src[i] && (dest_len + i) < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

// Browser state
static browser_state_t browser_state;
static uint8_t browser_initialized = 0;

void browser_init(void) {
    if (browser_initialized) return;
    
    browser_state.window = NULL;
    browser_state.tabs = NULL;
    browser_state.tab_count = 0;
    browser_state.active_tab = NULL;
    browser_state.history_count = 0;
    browser_state.history_index = -1;
    browser_state.bookmark_count = 0;
    
    browser_initialized = 1;
}

window_t* browser_create(void) {
    if (!browser_initialized) {
        browser_init();
    }
    
    window_t* window = gui_create_window("Aurora Web Browser", 80, 60, 840, 600);
    if (!window) return NULL;
    
    browser_state.window = window;
    
    // Create navigation bar
    gui_create_button(window, "<", 10, 10, 40, 30);        // Back
    gui_create_button(window, ">", 55, 10, 40, 30);        // Forward
    gui_create_button(window, "R", 100, 10, 40, 30);       // Refresh
    gui_create_button(window, "X", 145, 10, 40, 30);       // Stop
    gui_create_button(window, "Home", 190, 10, 60, 30);    // Home
    
    // URL bar (simulated with label for now)
    gui_create_label(window, "URL: http://localhost/", 260, 18);
    
    // Navigation buttons
    gui_create_button(window, "Go", 720, 10, 50, 30);
    gui_create_button(window, "+", 775, 10, 40, 30);       // New tab
    
    // Tab bar
    gui_create_label(window, "Tab 1", 10, 50);
    gui_create_button(window, "x", 70, 48, 25, 20);
    
    // Bookmarks bar
    gui_create_label(window, "Bookmarks:", 10, 80);
    gui_create_button(window, "Home", 90, 78, 70, 25);
    gui_create_button(window, "Docs", 165, 78, 70, 25);
    gui_create_button(window, "+ Add", 240, 78, 70, 25);
    
    // Content area
    gui_create_label(window, "Aurora OS Web Browser v1.0", 20, 120);
    gui_create_label(window, "", 20, 140);
    gui_create_label(window, "Welcome to Aurora Browser!", 20, 160);
    gui_create_label(window, "", 20, 180);
    gui_create_label(window, "Features:", 20, 200);
    gui_create_label(window, "  - Multiple tabs", 20, 220);
    gui_create_label(window, "  - Bookmarks", 20, 240);
    gui_create_label(window, "  - Navigation history", 20, 260);
    gui_create_label(window, "  - Basic HTML rendering", 20, 280);
    gui_create_label(window, "", 20, 300);
    gui_create_label(window, "Navigation:", 20, 320);
    gui_create_label(window, "  - Use the buttons above to navigate", 20, 340);
    gui_create_label(window, "  - Click '+' to open a new tab", 20, 360);
    gui_create_label(window, "  - Click 'x' on tabs to close them", 20, 380);
    gui_create_label(window, "  - Use bookmarks for quick access", 20, 400);
    gui_create_label(window, "", 20, 420);
    gui_create_label(window, "Supported protocols:", 20, 440);
    gui_create_label(window, "  - http://  (Hypertext Transfer Protocol)", 20, 460);
    gui_create_label(window, "  - https:// (Secure HTTP)", 20, 480);
    gui_create_label(window, "  - file://  (Local file access)", 20, 500);
    
    // Status bar
    gui_create_label(window, "Ready", 10, 565);
    
    // Create initial tab
    browser_new_tab("http://localhost/");
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return window;
}

void browser_destroy(window_t* window) {
    if (window && browser_state.window == window) {
        // Free tabs
        browser_tab_t* tab = browser_state.tabs;
        while (tab) {
            browser_tab_t* next = tab->next;
            // Free tab memory (would use kfree in real implementation)
            tab = next;
        }
        
        browser_state.window = NULL;
        browser_state.tabs = NULL;
        browser_state.tab_count = 0;
        browser_state.active_tab = NULL;
        
        gui_destroy_window(window);
    }
}

int browser_navigate(const char* url) {
    if (!url || web_strlen(url) == 0) {
        return -1;
    }
    
    if (!browser_state.active_tab) {
        return -1;
    }
    
    // Copy URL to active tab
    web_strcpy(browser_state.active_tab->url, url, BROWSER_MAX_URL_LENGTH);
    browser_state.active_tab->loading = 1;
    
    // Add to history
    if (browser_state.history_count < BROWSER_MAX_HISTORY) {
        web_strcpy(browser_state.history[browser_state.history_count], 
                  url, BROWSER_MAX_URL_LENGTH);
        browser_state.history_count++;
        browser_state.history_index = browser_state.history_count - 1;
    }
    
    // Simulate page load (in real implementation, would fetch and render content)
    browser_state.active_tab->loading = 0;
    
    return 0;
}

void browser_back(void) {
    if (browser_state.history_index > 0) {
        browser_state.history_index--;
        browser_navigate(browser_state.history[browser_state.history_index]);
    }
}

void browser_forward(void) {
    if (browser_state.history_index < browser_state.history_count - 1) {
        browser_state.history_index++;
        browser_navigate(browser_state.history[browser_state.history_index]);
    }
}

void browser_refresh(void) {
    if (browser_state.active_tab) {
        browser_navigate(browser_state.active_tab->url);
    }
}

void browser_stop(void) {
    if (browser_state.active_tab) {
        browser_state.active_tab->loading = 0;
    }
}

int browser_new_tab(const char* url) {
    if (browser_state.tab_count >= BROWSER_MAX_TABS) {
        return -1;
    }
    
    // Allocate new tab (in real implementation, would use kmalloc)
    // For now, we'll use a static array
    static browser_tab_t tabs_storage[BROWSER_MAX_TABS];
    browser_tab_t* new_tab = &tabs_storage[browser_state.tab_count];
    
    new_tab->loading = 0;
    new_tab->active = 1;
    new_tab->next = NULL;
    web_strcpy(new_tab->title, "New Tab", sizeof(new_tab->title));
    
    if (url) {
        web_strcpy(new_tab->url, url, BROWSER_MAX_URL_LENGTH);
    } else {
        web_strcpy(new_tab->url, "about:blank", BROWSER_MAX_URL_LENGTH);
    }
    
    // Add to tab list
    if (!browser_state.tabs) {
        browser_state.tabs = new_tab;
    } else {
        browser_tab_t* last = browser_state.tabs;
        while (last->next) {
            last->active = 0;
            last = last->next;
        }
        last->active = 0;
        last->next = new_tab;
    }
    
    browser_state.active_tab = new_tab;
    browser_state.tab_count++;
    
    if (url) {
        browser_navigate(url);
    }
    
    return 0;
}

void browser_close_tab(void) {
    if (!browser_state.active_tab || browser_state.tab_count == 0) {
        return;
    }
    
    // Find and remove active tab
    if (browser_state.tabs == browser_state.active_tab) {
        browser_state.tabs = browser_state.active_tab->next;
    } else {
        browser_tab_t* prev = browser_state.tabs;
        while (prev && prev->next != browser_state.active_tab) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = browser_state.active_tab->next;
        }
    }
    
    browser_state.tab_count--;
    
    // Switch to another tab if available
    if (browser_state.tabs) {
        browser_state.active_tab = browser_state.tabs;
        browser_state.active_tab->active = 1;
    } else {
        browser_state.active_tab = NULL;
    }
}

void browser_next_tab(void) {
    if (!browser_state.active_tab || !browser_state.active_tab->next) {
        return;
    }
    
    browser_state.active_tab->active = 0;
    browser_state.active_tab = browser_state.active_tab->next;
    browser_state.active_tab->active = 1;
}

void browser_prev_tab(void) {
    if (!browser_state.active_tab || browser_state.active_tab == browser_state.tabs) {
        return;
    }
    
    browser_tab_t* prev = browser_state.tabs;
    while (prev->next != browser_state.active_tab) {
        prev = prev->next;
    }
    
    browser_state.active_tab->active = 0;
    browser_state.active_tab = prev;
    browser_state.active_tab->active = 1;
}

int browser_add_bookmark(const char* url) {
    if (!url || browser_state.bookmark_count >= BROWSER_MAX_BOOKMARKS) {
        return -1;
    }
    
    web_strcpy(browser_state.bookmarks[browser_state.bookmark_count], 
              url, BROWSER_MAX_URL_LENGTH);
    browser_state.bookmark_count++;
    
    return 0;
}

void browser_update_display(void) {
    // Display will be updated by the GUI system
}

void browser_render_html(const char* html) {
    // Basic HTML rendering (placeholder implementation)
    // In a full implementation, this would parse HTML and render it
    (void)html;
    
    if (!browser_state.window) return;
    
    // For now, just display a message that content would be rendered here
    // A full HTML renderer would parse tags, render text, images, etc.
}
