/**
 * Aurora OS - Modern Web Browser (Chrome WebView-based)
 * 
 * Full-featured web browser with:
 * - Chrome-compatible rendering engine
 * - Extension/addon support
 * - DevTools integration
 * - Multi-process architecture
 * - Hardware acceleration
 * - Modern web standards (HTML5, CSS3, ES6+)
 */

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <stdint.h>
#include "gui.h"

/* ========== Browser Configuration ========== */
#define BROWSER_MAX_URL_LENGTH      2048
#define BROWSER_MAX_HISTORY         1000
#define BROWSER_MAX_BOOKMARKS       10000
#define BROWSER_MAX_TABS            100
#define BROWSER_MAX_EXTENSIONS      50
#define BROWSER_MAX_DOWNLOADS       20
#define BROWSER_MAX_COOKIES         5000
#define BROWSER_CACHE_SIZE_MB       512
#define BROWSER_USER_AGENT          "Aurora/1.0 Chrome/120.0 WebView"

/* ========== Browser Security Levels ========== */
#define SECURITY_NONE               0
#define SECURITY_LOW                1
#define SECURITY_MEDIUM             2
#define SECURITY_HIGH               3
#define SECURITY_STRICT             4

/* ========== Content Types ========== */
#define CONTENT_HTML                0x01
#define CONTENT_CSS                 0x02
#define CONTENT_JAVASCRIPT          0x04
#define CONTENT_IMAGE               0x08
#define CONTENT_VIDEO               0x10
#define CONTENT_AUDIO               0x20
#define CONTENT_FONT                0x40
#define CONTENT_JSON                0x80

/* ========== HTTP Methods ========== */
#define HTTP_GET                    0
#define HTTP_POST                   1
#define HTTP_PUT                    2
#define HTTP_DELETE                 3
#define HTTP_HEAD                   4
#define HTTP_OPTIONS                5
#define HTTP_PATCH                  6

/* ========== SSL/TLS Versions ========== */
#define TLS_1_0                     0x0301
#define TLS_1_1                     0x0302
#define TLS_1_2                     0x0303
#define TLS_1_3                     0x0304

/* ========== Cookie Structure ========== */
typedef struct browser_cookie {
    char name[64];
    char value[256];
    char domain[256];
    char path[128];
    uint64_t expires;
    uint8_t secure;
    uint8_t http_only;
    uint8_t same_site;      /* 0=None, 1=Lax, 2=Strict */
    struct browser_cookie* next;
} browser_cookie_t;

/* ========== Download Item ========== */
typedef struct browser_download {
    uint32_t id;
    char url[BROWSER_MAX_URL_LENGTH];
    char filename[256];
    char save_path[512];
    uint64_t total_size;
    uint64_t downloaded;
    uint8_t status;         /* 0=pending, 1=downloading, 2=paused, 3=complete, 4=error */
    uint8_t progress;       /* 0-100 */
    struct browser_download* next;
} browser_download_t;

/* ========== Extension/Addon Structure ========== */
typedef struct browser_extension {
    char id[64];
    char name[128];
    char version[32];
    char description[512];
    char author[128];
    uint8_t enabled;
    uint8_t permissions;    /* Bit flags for permissions */
    void* manifest;         /* Extension manifest data */
    void (*on_load)(void);
    void (*on_unload)(void);
    void (*on_message)(const char* message);
    struct browser_extension* next;
} browser_extension_t;

/* ========== DOM Element (simplified) ========== */
typedef struct dom_element {
    char tag[32];
    char id[64];
    char class_name[128];
    char* inner_html;
    char* inner_text;
    uint32_t child_count;
    struct dom_element** children;
    struct dom_element* parent;
    void* style;            /* CSS computed style */
    void* attributes;       /* Element attributes */
} dom_element_t;

/* ========== Render Tree Node ========== */
typedef struct render_node {
    dom_element_t* element;
    int32_t x, y;
    uint32_t width, height;
    uint32_t bg_color;
    uint32_t fg_color;
    uint8_t visible;
    struct render_node* next;
    struct render_node* children;
} render_node_t;

/* ========== JavaScript Context ========== */
typedef struct js_context {
    void* global_object;
    void* execution_stack;
    uint32_t stack_depth;
    uint8_t strict_mode;
    void* event_queue;
} js_context_t;

/* ========== SSL/TLS Session ========== */
typedef struct ssl_session {
    uint16_t version;
    uint8_t cipher_suite[2];
    uint8_t session_id[32];
    uint8_t master_secret[48];
    uint8_t client_random[32];
    uint8_t server_random[32];
    uint8_t verified;
    char* certificate_chain;
} ssl_session_t;

/* ========== HTTP Request ========== */
typedef struct http_request {
    uint8_t method;
    char url[BROWSER_MAX_URL_LENGTH];
    char host[256];
    uint16_t port;
    char path[1024];
    char* headers;
    uint8_t* body;
    uint32_t body_length;
    uint8_t use_ssl;
    ssl_session_t* ssl;
} http_request_t;

/* ========== HTTP Response ========== */
typedef struct http_response {
    uint16_t status_code;
    char status_text[64];
    char* headers;
    uint8_t* body;
    uint32_t body_length;
    uint32_t content_length;
    char content_type[128];
    char encoding[32];
    browser_cookie_t* cookies;
} http_response_t;

/* ========== Browser Tab (Enhanced) ========== */
typedef struct browser_tab {
    uint32_t id;
    char url[BROWSER_MAX_URL_LENGTH];
    char title[256];
    char favicon[128];
    uint8_t loading;
    uint8_t active;
    uint8_t secure;         /* HTTPS connection */
    uint8_t private_mode;   /* Incognito */
    uint8_t muted;          /* Audio muted */
    uint8_t playing_audio;
    uint8_t progress;       /* Load progress 0-100 */
    dom_element_t* document;
    render_node_t* render_tree;
    js_context_t* js_ctx;
    ssl_session_t* ssl;
    char* page_source;
    uint32_t scroll_x;
    uint32_t scroll_y;
    uint64_t load_start_time;
    uint64_t load_end_time;
    struct browser_tab* prev;
    struct browser_tab* next;
} browser_tab_t;

/* ========== DevTools State ========== */
typedef struct devtools_state {
    uint8_t visible;
    uint8_t docked;         /* 0=undocked, 1=bottom, 2=right, 3=left */
    uint8_t active_panel;   /* 0=elements, 1=console, 2=network, 3=sources, 4=performance */
    char console_output[8192];
    uint32_t console_lines;
    void* network_log;
    void* performance_data;
} devtools_state_t;

/* ========== Browser Settings ========== */
typedef struct browser_settings {
    uint8_t javascript_enabled;
    uint8_t cookies_enabled;
    uint8_t popup_blocker;
    uint8_t do_not_track;
    uint8_t hardware_accel;
    uint8_t dark_mode;
    uint8_t security_level;
    uint8_t auto_update;
    char home_page[BROWSER_MAX_URL_LENGTH];
    char search_engine[BROWSER_MAX_URL_LENGTH];
    char download_path[512];
    uint32_t cache_size_mb;
    char proxy_host[256];
    uint16_t proxy_port;
    uint8_t proxy_enabled;
} browser_settings_t;

/* ========== Browser State (Enhanced) ========== */
typedef struct {
    window_t* window;
    browser_tab_t* tabs;
    uint32_t tab_count;
    browser_tab_t* active_tab;
    char history[BROWSER_MAX_HISTORY][BROWSER_MAX_URL_LENGTH];
    char history_titles[BROWSER_MAX_HISTORY][256];
    uint64_t history_timestamps[BROWSER_MAX_HISTORY];
    uint32_t history_count;
    int32_t history_index;
    char bookmarks[BROWSER_MAX_BOOKMARKS][BROWSER_MAX_URL_LENGTH];
    char bookmark_titles[BROWSER_MAX_BOOKMARKS][256];
    uint32_t bookmark_count;
    browser_cookie_t* cookies;
    browser_download_t* downloads;
    browser_extension_t* extensions;
    devtools_state_t devtools;
    browser_settings_t settings;
    uint8_t initialized;
} browser_state_t;

/* ========== Core Browser Functions ========== */

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

/* ========== Tab Management ========== */

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
 * Close specific tab
 * @param tab_id Tab ID to close
 */
void browser_close_tab_by_id(uint32_t tab_id);

/**
 * Switch to next tab
 */
void browser_next_tab(void);

/**
 * Switch to previous tab
 */
void browser_prev_tab(void);

/**
 * Switch to specific tab
 * @param tab_id Tab ID to activate
 */
void browser_switch_to_tab(uint32_t tab_id);

/**
 * Duplicate current tab
 * @return New tab ID or 0 on failure
 */
uint32_t browser_duplicate_tab(void);

/**
 * Pin/unpin tab
 * @param tab_id Tab ID
 * @param pinned Pin state
 */
void browser_set_tab_pinned(uint32_t tab_id, uint8_t pinned);

/**
 * Mute/unmute tab audio
 * @param tab_id Tab ID
 * @param muted Mute state
 */
void browser_set_tab_muted(uint32_t tab_id, uint8_t muted);

/* ========== Bookmark Management ========== */

/**
 * Add bookmark
 * @param url URL to bookmark
 * @return 0 on success, -1 on error
 */
int browser_add_bookmark(const char* url);

/**
 * Add bookmark with title
 * @param url URL to bookmark
 * @param title Bookmark title
 * @return 0 on success, -1 on error
 */
int browser_add_bookmark_with_title(const char* url, const char* title);

/**
 * Remove bookmark
 * @param url URL to remove from bookmarks
 * @return 0 on success, -1 if not found
 */
int browser_remove_bookmark(const char* url);

/**
 * Check if URL is bookmarked
 * @param url URL to check
 * @return 1 if bookmarked, 0 if not
 */
int browser_is_bookmarked(const char* url);

/**
 * Get bookmark count
 * @return Number of bookmarks
 */
uint32_t browser_get_bookmark_count(void);

/* ========== History Management ========== */

/**
 * Clear browsing history
 */
void browser_clear_history(void);

/**
 * Get history count
 * @return Number of history entries
 */
uint32_t browser_get_history_count(void);

/**
 * Search history
 * @param query Search query
 * @param results Output array for matching URLs
 * @param max_results Maximum results to return
 * @return Number of results found
 */
uint32_t browser_search_history(const char* query, char** results, uint32_t max_results);

/* ========== Cookie Management ========== */

/**
 * Set cookie
 * @param domain Cookie domain
 * @param name Cookie name
 * @param value Cookie value
 * @param expires Expiration timestamp (0 for session cookie)
 * @param secure HTTPS only flag
 * @param http_only HTTP only flag
 * @return 0 on success, -1 on error
 */
int browser_set_cookie(const char* domain, const char* name, const char* value,
                       uint64_t expires, uint8_t secure, uint8_t http_only);

/**
 * Get cookie value
 * @param domain Cookie domain
 * @param name Cookie name
 * @return Cookie value or NULL if not found
 */
const char* browser_get_cookie(const char* domain, const char* name);

/**
 * Delete cookie
 * @param domain Cookie domain
 * @param name Cookie name
 * @return 0 on success, -1 if not found
 */
int browser_delete_cookie(const char* domain, const char* name);

/**
 * Clear all cookies
 */
void browser_clear_cookies(void);

/**
 * Clear cookies for domain
 * @param domain Domain to clear cookies for
 */
void browser_clear_domain_cookies(const char* domain);

/* ========== Download Management ========== */

/**
 * Start download
 * @param url URL to download
 * @param save_path Path to save file (NULL for default)
 * @return Download ID or 0 on failure
 */
uint32_t browser_start_download(const char* url, const char* save_path);

/**
 * Pause download
 * @param download_id Download ID
 * @return 0 on success, -1 on error
 */
int browser_pause_download(uint32_t download_id);

/**
 * Resume download
 * @param download_id Download ID
 * @return 0 on success, -1 on error
 */
int browser_resume_download(uint32_t download_id);

/**
 * Cancel download
 * @param download_id Download ID
 * @return 0 on success, -1 on error
 */
int browser_cancel_download(uint32_t download_id);

/**
 * Get download progress
 * @param download_id Download ID
 * @return Progress 0-100 or -1 if not found
 */
int browser_get_download_progress(uint32_t download_id);

/* ========== Extension Management ========== */

/**
 * Install extension
 * @param extension_path Path to extension package
 * @return 0 on success, -1 on error
 */
int browser_install_extension(const char* extension_path);

/**
 * Uninstall extension
 * @param extension_id Extension ID
 * @return 0 on success, -1 on error
 */
int browser_uninstall_extension(const char* extension_id);

/**
 * Enable extension
 * @param extension_id Extension ID
 * @return 0 on success, -1 on error
 */
int browser_enable_extension(const char* extension_id);

/**
 * Disable extension
 * @param extension_id Extension ID
 * @return 0 on success, -1 on error
 */
int browser_disable_extension(const char* extension_id);

/**
 * Get extension count
 * @return Number of installed extensions
 */
uint32_t browser_get_extension_count(void);

/**
 * Send message to extension
 * @param extension_id Extension ID
 * @param message Message to send
 * @return 0 on success, -1 on error
 */
int browser_send_extension_message(const char* extension_id, const char* message);

/* ========== DevTools ========== */

/**
 * Toggle DevTools visibility
 */
void browser_toggle_devtools(void);

/**
 * Open DevTools for current tab
 */
void browser_open_devtools(void);

/**
 * Close DevTools
 */
void browser_close_devtools(void);

/**
 * Execute JavaScript in page context
 * @param script JavaScript code to execute
 * @return Result string or NULL on error
 */
char* browser_execute_script(const char* script);

/**
 * Log to DevTools console
 * @param level Log level (0=log, 1=warn, 2=error)
 * @param message Message to log
 */
void browser_console_log(uint8_t level, const char* message);

/* ========== Settings ========== */

/**
 * Get browser settings
 * @return Pointer to settings structure
 */
browser_settings_t* browser_get_settings(void);

/**
 * Set home page
 * @param url Home page URL
 */
void browser_set_home_page(const char* url);

/**
 * Set search engine
 * @param url Search engine URL (use %s for query placeholder)
 */
void browser_set_search_engine(const char* url);

/**
 * Enable/disable JavaScript
 * @param enabled JavaScript state
 */
void browser_set_javascript_enabled(uint8_t enabled);

/**
 * Enable/disable cookies
 * @param enabled Cookies state
 */
void browser_set_cookies_enabled(uint8_t enabled);

/**
 * Set proxy configuration
 * @param host Proxy host (NULL to disable)
 * @param port Proxy port
 */
void browser_set_proxy(const char* host, uint16_t port);

/**
 * Clear browser cache
 */
void browser_clear_cache(void);

/**
 * Clear all browsing data
 * @param history Clear history
 * @param cookies Clear cookies
 * @param cache Clear cache
 * @param downloads Clear downloads
 */
void browser_clear_data(uint8_t history, uint8_t cookies, uint8_t cache, uint8_t downloads);

/* ========== Rendering ========== */

/**
 * Update browser display
 */
void browser_update_display(void);

/**
 * Render HTML content (basic)
 * @param html HTML content to render
 */
void browser_render_html(const char* html);

/**
 * Parse HTML document
 * @param html HTML source
 * @return DOM tree root or NULL on error
 */
dom_element_t* browser_parse_html(const char* html);

/**
 * Parse CSS stylesheet
 * @param css CSS source
 * @return Style object or NULL on error
 */
void* browser_parse_css(const char* css);

/**
 * Build render tree from DOM
 * @param document DOM document root
 * @return Render tree root or NULL on error
 */
render_node_t* browser_build_render_tree(dom_element_t* document);

/**
 * Layout render tree
 * @param root Render tree root
 * @param width Viewport width
 * @param height Viewport height
 */
void browser_layout(render_node_t* root, uint32_t width, uint32_t height);

/**
 * Paint render tree to framebuffer
 * @param root Render tree root
 * @param x Viewport X offset
 * @param y Viewport Y offset
 */
void browser_paint(render_node_t* root, int32_t x, int32_t y);

/* ========== Network Requests ========== */

/**
 * Create HTTP request
 * @param method HTTP method
 * @param url URL to request
 * @return Request object or NULL on error
 */
http_request_t* browser_create_request(uint8_t method, const char* url);

/**
 * Send HTTP request
 * @param request Request to send
 * @return Response object or NULL on error
 */
http_response_t* browser_send_request(http_request_t* request);

/**
 * Free HTTP request
 * @param request Request to free
 */
void browser_free_request(http_request_t* request);

/**
 * Free HTTP response
 * @param response Response to free
 */
void browser_free_response(http_response_t* response);

/* ========== SSL/TLS ========== */

/**
 * Create SSL session
 * @param host Hostname
 * @param port Port number
 * @return SSL session or NULL on error
 */
ssl_session_t* browser_ssl_connect(const char* host, uint16_t port);

/**
 * Close SSL session
 * @param ssl SSL session to close
 */
void browser_ssl_close(ssl_session_t* ssl);

/**
 * Verify SSL certificate
 * @param ssl SSL session
 * @return 1 if valid, 0 if invalid
 */
int browser_ssl_verify(ssl_session_t* ssl);

/**
 * Get certificate info
 * @param ssl SSL session
 * @return Certificate info string or NULL
 */
const char* browser_ssl_get_cert_info(ssl_session_t* ssl);

/* ========== Find in Page ========== */

/**
 * Start find in page
 * @param query Search query
 * @return Number of matches found
 */
uint32_t browser_find_in_page(const char* query);

/**
 * Find next match
 */
void browser_find_next(void);

/**
 * Find previous match
 */
void browser_find_previous(void);

/**
 * Close find bar
 */
void browser_find_close(void);

/* ========== Print/Save ========== */

/**
 * Print current page
 */
void browser_print(void);

/**
 * Save page as HTML
 * @param path Path to save
 * @return 0 on success, -1 on error
 */
int browser_save_page(const char* path);

/**
 * Save page as PDF
 * @param path Path to save
 * @return 0 on success, -1 on error
 */
int browser_save_as_pdf(const char* path);

/* ========== Zoom ========== */

/**
 * Set zoom level
 * @param percent Zoom percentage (25-500)
 */
void browser_set_zoom(uint16_t percent);

/**
 * Get current zoom level
 * @return Zoom percentage
 */
uint16_t browser_get_zoom(void);

/**
 * Zoom in
 */
void browser_zoom_in(void);

/**
 * Zoom out
 */
void browser_zoom_out(void);

/**
 * Reset zoom to 100%
 */
void browser_zoom_reset(void);

#endif // WEBVIEW_H
