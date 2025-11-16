/**
 * Aurora OS - Application Framework Implementation
 * 
 * Framework for launching and managing GUI applications
 */

#include "application.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../drivers/storage.h"
#include <stddef.h>

// String functions
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Application registry
static application_t applications[APP_MAX];
static uint8_t app_framework_initialized = 0;

// Forward declarations for application launchers
static int launch_file_manager(void);
static int launch_terminal(void);
static int launch_settings(void);
static int launch_system_info(void);
static int launch_text_editor(void);
static int launch_calculator(void);
static int launch_disk_manager(void);

void app_init(void) {
    if (app_framework_initialized) return;
    
    // Initialize application registry
    applications[APP_FILE_MANAGER] = (application_t){
        .type = APP_FILE_MANAGER,
        .name = "File Manager",
        .description = "Browse and manage files",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_TERMINAL] = (application_t){
        .type = APP_TERMINAL,
        .name = "Terminal",
        .description = "Command line interface",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_SETTINGS] = (application_t){
        .type = APP_SETTINGS,
        .name = "System Settings",
        .description = "Configure system settings",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_SYSTEM_INFO] = (application_t){
        .type = APP_SYSTEM_INFO,
        .name = "System Information",
        .description = "View system information",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_TEXT_EDITOR] = (application_t){
        .type = APP_TEXT_EDITOR,
        .name = "Text Editor",
        .description = "Edit text files",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_CALCULATOR] = (application_t){
        .type = APP_CALCULATOR,
        .name = "Calculator",
        .description = "Simple calculator",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_DISK_MANAGER] = (application_t){
        .type = APP_DISK_MANAGER,
        .name = "Disk Manager",
        .description = "Manage storage devices",
        .window = NULL,
        .running = 0
    };
    
    app_framework_initialized = 1;
}

int app_launch(app_type_t type) {
    if (!app_framework_initialized || type >= APP_MAX) return -1;
    
    // Don't launch if already running
    if (applications[type].running) {
        // Just focus the window if already running
        if (applications[type].window) {
            gui_focus_window(applications[type].window);
        }
        return 0;
    }
    
    // Launch the appropriate application
    int result = -1;
    switch (type) {
        case APP_FILE_MANAGER:
            result = launch_file_manager();
            break;
        case APP_TERMINAL:
            result = launch_terminal();
            break;
        case APP_SETTINGS:
            result = launch_settings();
            break;
        case APP_SYSTEM_INFO:
            result = launch_system_info();
            break;
        case APP_TEXT_EDITOR:
            result = launch_text_editor();
            break;
        case APP_CALCULATOR:
            result = launch_calculator();
            break;
        case APP_DISK_MANAGER:
            result = launch_disk_manager();
            break;
        default:
            return -1;
    }
    
    if (result == 0) {
        applications[type].running = 1;
    }
    
    return result;
}

void app_close(app_type_t type) {
    if (!app_framework_initialized || type >= APP_MAX) return;
    
    if (applications[type].running && applications[type].window) {
        gui_destroy_window(applications[type].window);
        applications[type].window = NULL;
        applications[type].running = 0;
    }
}

application_t* app_get(app_type_t type) {
    if (!app_framework_initialized || type >= APP_MAX) return NULL;
    return &applications[type];
}

int app_is_running(app_type_t type) {
    if (!app_framework_initialized || type >= APP_MAX) return 0;
    return applications[type].running;
}

// Application launchers

static int launch_file_manager(void) {
    window_t* window = gui_create_window("File Manager", 150, 120, 600, 400);
    if (!window) return -1;
    
    applications[APP_FILE_MANAGER].window = window;
    
    /* Add file manager interface */
    gui_create_label(window, "File Manager", 20, 20);
    gui_create_label(window, "Location: /", 20, 50);
    
    /* Add storage device information */
    gui_create_label(window, "Storage Devices:", 20, 90);
    
    /* Try to initialize and detect storage devices */
    storage_init();
    int device_count = storage_detect_devices();
    
    if (device_count > 0) {
        char info_text[128];
        int y_offset = 120;
        
        for (int i = 0; i < device_count && i < 4; i++) {
            storage_device_t* device = storage_get_device(i);
            if (device) {
                /* Format: "Drive 0: ATA, 10 GB, Online" */
                const char* type_str = storage_get_type_string(device->type);
                const char* status_str = storage_get_status_string(device->status);
                uint32_t capacity_gb = (uint32_t)storage_get_capacity_gb(device);
                
                /* Simple sprintf alternative for our needs */
                int pos = 0;
                const char* prefix = "Drive ";
                while (*prefix) info_text[pos++] = *prefix++;
                info_text[pos++] = '0' + i;
                info_text[pos++] = ':';
                info_text[pos++] = ' ';
                
                while (*type_str) info_text[pos++] = *type_str++;
                info_text[pos++] = ',';
                info_text[pos++] = ' ';
                
                /* Add capacity (simple conversion) */
                if (capacity_gb > 0) {
                    if (capacity_gb >= 1000) {
                        info_text[pos++] = '1';
                        info_text[pos++] = 'T';
                        info_text[pos++] = 'B';
                    } else {
                        /* Simple number to string */
                        if (capacity_gb >= 100) info_text[pos++] = '0' + (capacity_gb / 100);
                        if (capacity_gb >= 10) info_text[pos++] = '0' + ((capacity_gb / 10) % 10);
                        info_text[pos++] = '0' + (capacity_gb % 10);
                        info_text[pos++] = ' ';
                        info_text[pos++] = 'G';
                        info_text[pos++] = 'B';
                    }
                } else {
                    info_text[pos++] = 'N';
                    info_text[pos++] = '/';
                    info_text[pos++] = 'A';
                }
                
                info_text[pos++] = ',';
                info_text[pos++] = ' ';
                while (*status_str) info_text[pos++] = *status_str++;
                info_text[pos] = '\0';
                
                gui_create_label(window, info_text, 40, y_offset);
                y_offset += 25;
            }
        }
    } else {
        gui_create_label(window, "No storage devices detected", 40, 120);
    }
    
    /* Add file operation buttons */
    gui_create_button(window, "Refresh", 20, 350, 80, 30);
    gui_create_button(window, "Properties", 110, 350, 90, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_terminal(void) {
    window_t* window = gui_create_window("Terminal", 100, 150, 640, 480);
    if (!window) return -1;
    
    applications[APP_TERMINAL].window = window;
    
    // Add terminal interface
    gui_create_label(window, "Aurora OS Terminal v1.0", 10, 10);
    gui_create_label(window, "$ _", 10, 40);
    gui_create_label(window, "Terminal functionality coming soon", 10, 70);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_settings(void) {
    window_t* window = gui_create_window("System Settings", 180, 100, 500, 450);
    if (!window) return -1;
    
    applications[APP_SETTINGS].window = window;
    
    // Add settings categories
    gui_create_label(window, "System Settings", 20, 20);
    gui_create_label(window, "Display Settings", 40, 60);
    gui_create_label(window, "Network Settings", 40, 90);
    gui_create_label(window, "Power Settings", 40, 120);
    gui_create_label(window, "User Accounts", 40, 150);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_system_info(void) {
    window_t* window = gui_create_window("System Information", 200, 150, 500, 400);
    if (!window) return -1;
    
    applications[APP_SYSTEM_INFO].window = window;
    
    /* Add system information */
    gui_create_label(window, "Aurora OS v1.0", 20, 20);
    gui_create_label(window, "Kernel Version: 1.0.0", 20, 50);
    gui_create_label(window, "Build Date: Nov 2025", 20, 80);
    gui_create_label(window, "Memory: Available", 20, 110);
    gui_create_label(window, "CPU: x86 Compatible", 20, 140);
    gui_create_label(window, "GUI: Phase 4 Complete", 20, 170);
    
    /* Storage information */
    gui_create_label(window, "Storage Devices:", 20, 210);
    
    storage_init();
    int device_count = storage_detect_devices();
    
    if (device_count > 0) {
        char count_text[32];
        int pos = 0;
        const char* prefix = "  Detected: ";
        while (*prefix) count_text[pos++] = *prefix++;
        count_text[pos++] = '0' + device_count;
        const char* suffix = " device(s)";
        while (*suffix) count_text[pos++] = *suffix++;
        count_text[pos] = '\0';
        
        gui_create_label(window, count_text, 20, 235);
        
        /* Show first device details */
        storage_device_t* device = storage_get_device(0);
        if (device) {
            int temp = 0;
            storage_get_temperature(device, &temp);
            
            char temp_text[48];
            pos = 0;
            const char* temp_prefix = "  Temperature: ";
            while (*temp_prefix) temp_text[pos++] = *temp_prefix++;
            if (temp >= 100) temp_text[pos++] = '0' + (temp / 100);
            if (temp >= 10) temp_text[pos++] = '0' + ((temp / 10) % 10);
            temp_text[pos++] = '0' + (temp % 10);
            temp_text[pos++] = ' ';
            temp_text[pos++] = 'C';
            temp_text[pos] = '\0';
            
            gui_create_label(window, temp_text, 20, 260);
            
            /* SMART status */
            uint8_t smart_status = 0;
            storage_get_smart_status(device, &smart_status);
            const char* health = smart_status ? "  Health: Warning" : "  Health: Good";
            gui_create_label(window, health, 20, 285);
        }
    } else {
        gui_create_label(window, "  No devices detected", 20, 235);
    }
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_text_editor(void) {
    window_t* window = gui_create_window("Text Editor", 120, 80, 700, 500);
    if (!window) return -1;
    
    applications[APP_TEXT_EDITOR].window = window;
    
    // Add text editor interface
    gui_create_label(window, "Text Editor - Untitled", 20, 20);
    gui_create_label(window, "Edit functionality coming soon", 20, 60);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_calculator(void) {
    window_t* window = gui_create_window("Calculator", 250, 200, 300, 400);
    if (!window) return -1;
    
    applications[APP_CALCULATOR].window = window;
    
    // Add calculator interface
    gui_create_label(window, "Calculator", 20, 20);
    gui_create_label(window, "0", 200, 60);
    
    // Add button grid (placeholder)
    gui_create_button(window, "7", 20, 100, 50, 40);
    gui_create_button(window, "8", 80, 100, 50, 40);
    gui_create_button(window, "9", 140, 100, 50, 40);
    gui_create_button(window, "+", 200, 100, 50, 40);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_disk_manager(void) {
    window_t* window = gui_create_window("Disk Manager", 100, 100, 700, 500);
    if (!window) return -1;
    
    applications[APP_DISK_MANAGER].window = window;
    
    /* Add disk manager interface */
    gui_create_label(window, "Disk Manager - Storage Device Information", 20, 20);
    
    /* Initialize and detect storage devices */
    storage_init();
    int device_count = storage_detect_devices();
    
    gui_create_label(window, "Detected Devices:", 20, 60);
    
    if (device_count > 0) {
        int y_offset = 90;
        
        for (int i = 0; i < device_count && i < 8; i++) {
            storage_device_t* device = storage_get_device(i);
            if (!device) continue;
            
            /* Device header */
            char header[64];
            int pos = 0;
            const char* prefix = "Device ";
            while (*prefix) header[pos++] = *prefix++;
            header[pos++] = '0' + i;
            header[pos++] = ':';
            header[pos++] = ' ';
            const char* type_str = storage_get_type_string(device->type);
            while (*type_str) header[pos++] = *type_str++;
            header[pos] = '\0';
            gui_create_label(window, header, 40, y_offset);
            y_offset += 25;
            
            /* Model */
            char model_line[64];
            pos = 0;
            const char* model_prefix = "  Model: ";
            while (*model_prefix) model_line[pos++] = *model_prefix++;
            for (int j = 0; j < 40 && device->model[j] && pos < 60; j++) {
                model_line[pos++] = device->model[j];
            }
            model_line[pos] = '\0';
            gui_create_label(window, model_line, 40, y_offset);
            y_offset += 20;
            
            /* Capacity */
            char cap_line[64];
            pos = 0;
            const char* cap_prefix = "  Capacity: ";
            while (*cap_prefix) cap_line[pos++] = *cap_prefix++;
            uint32_t cap_gb = (uint32_t)storage_get_capacity_gb(device);
            if (cap_gb >= 1000) {
                uint32_t tb = cap_gb / 1000;
                if (tb >= 10) cap_line[pos++] = '0' + (tb / 10);
                cap_line[pos++] = '0' + (tb % 10);
                cap_line[pos++] = ' ';
                cap_line[pos++] = 'T';
                cap_line[pos++] = 'B';
            } else if (cap_gb > 0) {
                if (cap_gb >= 1000) cap_line[pos++] = '0' + (cap_gb / 1000);
                if (cap_gb >= 100) cap_line[pos++] = '0' + ((cap_gb / 100) % 10);
                if (cap_gb >= 10) cap_line[pos++] = '0' + ((cap_gb / 10) % 10);
                cap_line[pos++] = '0' + (cap_gb % 10);
                cap_line[pos++] = ' ';
                cap_line[pos++] = 'G';
                cap_line[pos++] = 'B';
            } else {
                const char* na = "N/A";
                while (*na) cap_line[pos++] = *na++;
            }
            cap_line[pos] = '\0';
            gui_create_label(window, cap_line, 40, y_offset);
            y_offset += 20;
            
            /* Status */
            char status_line[48];
            pos = 0;
            const char* status_prefix = "  Status: ";
            while (*status_prefix) status_line[pos++] = *status_prefix++;
            const char* status_str = storage_get_status_string(device->status);
            while (*status_str) status_line[pos++] = *status_str++;
            status_line[pos] = '\0';
            gui_create_label(window, status_line, 40, y_offset);
            y_offset += 20;
            
            /* Temperature */
            int temp = 0;
            if (storage_get_temperature(device, &temp) == 0) {
                char temp_line[48];
                pos = 0;
                const char* temp_prefix = "  Temperature: ";
                while (*temp_prefix) temp_line[pos++] = *temp_prefix++;
                if (temp >= 100) temp_line[pos++] = '0' + (temp / 100);
                if (temp >= 10) temp_line[pos++] = '0' + ((temp / 10) % 10);
                temp_line[pos++] = '0' + (temp % 10);
                temp_line[pos++] = ' ';
                temp_line[pos++] = 'C';
                temp_line[pos] = '\0';
                gui_create_label(window, temp_line, 40, y_offset);
                y_offset += 20;
            }
            
            /* Statistics */
            char stats_line[64];
            pos = 0;
            const char* stats_prefix = "  Operations: R=";
            while (*stats_prefix) stats_line[pos++] = *stats_prefix++;
            uint32_t reads = (uint32_t)(device->reads & 0xFFFFFFFF);
            if (reads >= 100) stats_line[pos++] = '0' + ((reads / 100) % 10);
            if (reads >= 10) stats_line[pos++] = '0' + ((reads / 10) % 10);
            stats_line[pos++] = '0' + (reads % 10);
            const char* write_part = ", W=";
            while (*write_part) stats_line[pos++] = *write_part++;
            uint32_t writes = (uint32_t)(device->writes & 0xFFFFFFFF);
            if (writes >= 100) stats_line[pos++] = '0' + ((writes / 100) % 10);
            if (writes >= 10) stats_line[pos++] = '0' + ((writes / 10) % 10);
            stats_line[pos++] = '0' + (writes % 10);
            stats_line[pos] = '\0';
            gui_create_label(window, stats_line, 40, y_offset);
            y_offset += 30;
        }
    } else {
        gui_create_label(window, "No storage devices detected", 40, 90);
        gui_create_label(window, "Storage devices may not be available", 40, 115);
        gui_create_label(window, "in this environment.", 40, 135);
    }
    
    /* Action buttons */
    gui_create_button(window, "Refresh", 20, 450, 90, 30);
    gui_create_button(window, "Close", 590, 450, 90, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}
