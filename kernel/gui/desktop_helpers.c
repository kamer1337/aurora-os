/**
 * Aurora OS - Desktop Helper Functions Implementation
 * 
 * Utility functions for desktop operations and UI helpers
 */

#include "desktop_helpers.h"
#include "application.h"
#include "../drivers/storage.h"

static uint8_t desktop_helpers_initialized = 0;

void desktop_helpers_init(void) {
    if (desktop_helpers_initialized) return;
    
    /* Initialize helper subsystems */
    desktop_helpers_initialized = 1;
}

int desktop_perform_action(desktop_action_t action) {
    if (!desktop_helpers_initialized) {
        desktop_helpers_init();
    }
    
    switch (action) {
        case DESKTOP_ACTION_NEW_FOLDER:
            // Create a new folder in the current directory
            {
                // This would interact with the VFS to create a folder
                // For now, create a placeholder dialog
                window_t* dialog = gui_create_window("New Folder", 300, 200, 350, 150);
                if (dialog) {
                    dialog->bg_color = COLOR_WHITE;
                    gui_create_label(dialog, "Create new folder:", 20, 20);
                    gui_create_label(dialog, "Name: New Folder", 20, 50);
                    gui_create_button(dialog, "Create", 100, 90, 70, 30);
                    gui_create_button(dialog, "Cancel", 180, 90, 70, 30);
                    gui_show_window(dialog);
                    gui_focus_window(dialog);
                }
            }
            return 0;
            
        case DESKTOP_ACTION_NEW_FILE:
            // Create a new file
            {
                window_t* dialog = gui_create_window("New File", 300, 200, 350, 150);
                if (dialog) {
                    dialog->bg_color = COLOR_WHITE;
                    gui_create_label(dialog, "Create new file:", 20, 20);
                    gui_create_label(dialog, "Name: New File.txt", 20, 50);
                    gui_create_button(dialog, "Create", 100, 90, 70, 30);
                    gui_create_button(dialog, "Cancel", 180, 90, 70, 30);
                    gui_show_window(dialog);
                    gui_focus_window(dialog);
                }
            }
            return 0;
            
        case DESKTOP_ACTION_REFRESH:
            // Refresh the desktop icons and windows
            {
                // Clear and redraw the desktop
                framebuffer_info_t* fb = framebuffer_get_info();
                if (fb) {
                    // Trigger a full GUI update
                    gui_update();
                }
            }
            return 0;
            
        case DESKTOP_ACTION_PROPERTIES:
            // Show desktop properties dialog
            {
                window_t* props = gui_create_window("Desktop Properties", 250, 150, 400, 300);
                if (props) {
                    props->bg_color = (color_t){240, 240, 245, 255};
                    
                    framebuffer_info_t* fb = framebuffer_get_info();
                    if (fb) {
                        char res_buffer[64];
                        desktop_format_number(fb->width, res_buffer, sizeof(res_buffer));
                        gui_create_label(props, "Resolution:", 20, 20);
                        gui_create_label(props, res_buffer, 150, 20);
                        
                        desktop_format_number(fb->height, res_buffer, sizeof(res_buffer));
                        gui_create_label(props, "x", 230, 20);
                        gui_create_label(props, res_buffer, 250, 20);
                        
                        desktop_format_number(fb->bpp, res_buffer, sizeof(res_buffer));
                        gui_create_label(props, "Color Depth:", 20, 50);
                        gui_create_label(props, res_buffer, 150, 50);
                        gui_create_label(props, "bits", 180, 50);
                        
                        int icon_count = desktop_get_icon_count();
                        desktop_format_number(icon_count, res_buffer, sizeof(res_buffer));
                        gui_create_label(props, "Desktop Icons:", 20, 80);
                        gui_create_label(props, res_buffer, 150, 80);
                    }
                    
                    gui_create_button(props, "Settings", 80, 240, 100, 30);
                    gui_create_button(props, "Close", 220, 240, 100, 30);
                    gui_show_window(props);
                    gui_focus_window(props);
                }
            }
            return 0;
            
        case DESKTOP_ACTION_OPEN_TERMINAL:
            return app_launch(APP_TERMINAL);
            
        case DESKTOP_ACTION_OPEN_FILE_MANAGER:
            return app_launch(APP_FILE_MANAGER);
            
        case DESKTOP_ACTION_OPEN_DISK_MANAGER:
            return app_launch(APP_DISK_MANAGER);
            
        default:
            return -1;
    }
}

const char* desktop_format_size(uint64_t bytes, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 16) {
        return "0 B";
    }
    
    int pos = 0;
    
    /* Use bit shifting to avoid 64-bit division */
    if (bytes >= (1ULL << 40)) {
        /* TB range */
        uint32_t tb = (uint32_t)(bytes >> 40);
        if (tb >= 100) buffer[pos++] = '0' + (tb / 100);
        if (tb >= 10) buffer[pos++] = '0' + ((tb / 10) % 10);
        buffer[pos++] = '0' + (tb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'T';
        buffer[pos++] = 'B';
    } else if (bytes >= (1ULL << 30)) {
        /* GB range */
        uint32_t gb = (uint32_t)(bytes >> 30);
        if (gb >= 100) buffer[pos++] = '0' + (gb / 100);
        if (gb >= 10) buffer[pos++] = '0' + ((gb / 10) % 10);
        buffer[pos++] = '0' + (gb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'G';
        buffer[pos++] = 'B';
    } else if (bytes >= (1ULL << 20)) {
        /* MB range */
        uint32_t mb = (uint32_t)(bytes >> 20);
        if (mb >= 100) buffer[pos++] = '0' + (mb / 100);
        if (mb >= 10) buffer[pos++] = '0' + ((mb / 10) % 10);
        buffer[pos++] = '0' + (mb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'M';
        buffer[pos++] = 'B';
    } else if (bytes >= (1ULL << 10)) {
        /* KB range */
        uint32_t kb = (uint32_t)(bytes >> 10);
        if (kb >= 100) buffer[pos++] = '0' + (kb / 100);
        if (kb >= 10) buffer[pos++] = '0' + ((kb / 10) % 10);
        buffer[pos++] = '0' + (kb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'K';
        buffer[pos++] = 'B';
    } else {
        /* Bytes */
        uint32_t b = (uint32_t)bytes;
        if (b >= 100) buffer[pos++] = '0' + (b / 100);
        if (b >= 10) buffer[pos++] = '0' + ((b / 10) % 10);
        buffer[pos++] = '0' + (b % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'B';
    }
    
    buffer[pos] = '\0';
    return buffer;
}

const char* desktop_format_number(uint32_t num, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 12) {
        return "0";
    }
    
    int pos = 0;
    
    /* Format number with commas for readability */
    if (num >= 1000000000) {
        buffer[pos++] = '0' + (num / 1000000000);
        buffer[pos++] = ',';
        num %= 1000000000;
        buffer[pos++] = '0' + (num / 100000000);
        buffer[pos++] = '0' + ((num / 10000000) % 10);
        buffer[pos++] = '0' + ((num / 1000000) % 10);
        buffer[pos++] = ',';
        num %= 1000000;
        buffer[pos++] = '0' + (num / 100000);
        buffer[pos++] = '0' + ((num / 10000) % 10);
        buffer[pos++] = '0' + ((num / 1000) % 10);
        buffer[pos++] = ',';
        num %= 1000;
        buffer[pos++] = '0' + (num / 100);
        buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    } else if (num >= 1000000) {
        buffer[pos++] = '0' + (num / 1000000);
        buffer[pos++] = ',';
        num %= 1000000;
        buffer[pos++] = '0' + (num / 100000);
        buffer[pos++] = '0' + ((num / 10000) % 10);
        buffer[pos++] = '0' + ((num / 1000) % 10);
        buffer[pos++] = ',';
        num %= 1000;
        buffer[pos++] = '0' + (num / 100);
        buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    } else if (num >= 1000) {
        buffer[pos++] = '0' + (num / 1000);
        buffer[pos++] = ',';
        num %= 1000;
        buffer[pos++] = '0' + (num / 100);
        buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    } else {
        if (num >= 100) buffer[pos++] = '0' + (num / 100);
        if (num >= 10) buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    }
    
    buffer[pos] = '\0';
    return buffer;
}

int desktop_get_icon_count(void) {
    // Return actual count of desktop icons
    return 4;  // File Manager, Terminal, Settings, System Info
}

int desktop_launch_app(uint8_t app_type) {
    if (app_type >= APP_MAX) {
        return -1;
    }
    
    return app_launch((app_type_t)app_type);
}
