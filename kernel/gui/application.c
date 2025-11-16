/**
 * Aurora OS - Application Framework Implementation
 * 
 * Framework for launching and managing GUI applications
 */

#include "application.h"
#include "gui.h"
#include "framebuffer.h"
#include "desktop_config.h"
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
    
    /* Add terminal interface */
    gui_create_label(window, "Aurora OS Terminal v1.0", 10, 10);
    gui_create_label(window, "Copyright (c) 2025 Aurora OS Project", 10, 30);
    gui_create_label(window, "", 10, 50);
    gui_create_label(window, "Available commands:", 10, 70);
    gui_create_label(window, "  help      - Display this help", 10, 95);
    gui_create_label(window, "  clear     - Clear the screen", 10, 115);
    gui_create_label(window, "  version   - Show OS version", 10, 135);
    gui_create_label(window, "  sysinfo   - Display system information", 10, 155);
    gui_create_label(window, "  storage   - Show storage devices", 10, 175);
    gui_create_label(window, "  mem       - Display memory information", 10, 195);
    gui_create_label(window, "  exit      - Close terminal", 10, 215);
    gui_create_label(window, "", 10, 235);
    gui_create_label(window, "aurora@os:~$ _", 10, 255);
    
    /* Add status bar at bottom */
    gui_create_label(window, "Terminal ready - Type 'help' for commands", 10, 445);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_settings(void) {
    // Use the desktop configuration system instead
    desktop_config_show_settings();
    
    // Also create the traditional settings window
    window_t* window = gui_create_window("System Settings", 180, 100, 550, 500);
    if (!window) return -1;
    
    applications[APP_SETTINGS].window = window;
    
    /* Add settings categories */
    gui_create_label(window, "System Settings", 20, 20);
    gui_create_label(window, "Configure your Aurora OS system", 20, 45);
    
    /* Desktop Appearance Button */
    gui_create_button(window, "Desktop Appearance", 30, 80, 180, 35);
    
    /* Display Settings Section */
    gui_create_label(window, "Display Settings", 30, 130);
    gui_create_label(window, "  Resolution: 1920x1080x32", 50, 155);
    gui_create_label(window, "  Color Depth: 32-bit RGBA", 50, 175);
    
    /* Storage Settings Section */
    gui_create_label(window, "Storage Settings", 30, 210);
    storage_init();
    int device_count = storage_detect_devices();
    
    char storage_text[48];
    int pos = 0;
    const char* prefix = "  Devices Detected: ";
    while (*prefix) storage_text[pos++] = *prefix++;
    storage_text[pos++] = '0' + device_count;
    storage_text[pos] = '\0';
    gui_create_label(window, storage_text, 50, 235);
    
    if (device_count > 0) {
        storage_device_t* device = storage_get_device(0);
        if (device) {
            /* Show first device as primary */
            char primary_text[64];
            pos = 0;
            const char* prim_prefix = "  Primary Drive: ";
            while (*prim_prefix) primary_text[pos++] = *prim_prefix++;
            const char* type_str = storage_get_type_string(device->type);
            while (*type_str) primary_text[pos++] = *type_str++;
            primary_text[pos++] = ' ';
            uint32_t cap = (uint32_t)storage_get_capacity_gb(device);
            if (cap >= 100) primary_text[pos++] = '0' + (cap / 100);
            if (cap >= 10) primary_text[pos++] = '0' + ((cap / 10) % 10);
            primary_text[pos++] = '0' + (cap % 10);
            primary_text[pos++] = 'G';
            primary_text[pos++] = 'B';
            primary_text[pos] = '\0';
            gui_create_label(window, primary_text, 50, 255);
        }
    }
    
    gui_create_button(window, "Manage Storage", 50, 285, 140, 30);
    
    /* Network Settings Section */
    gui_create_label(window, "Network Settings", 30, 335);
    gui_create_label(window, "  Network Interface: Enabled", 50, 360);
    gui_create_label(window, "  Status: Not Connected", 50, 380);
    
    /* Power Settings Section */
    gui_create_label(window, "Power Settings", 30, 420);
    gui_create_label(window, "  Power Mode: Balanced", 50, 445);
    
    /* Action buttons */
    gui_create_button(window, "Apply", 30, 450, 80, 30);
    gui_create_button(window, "Close", 440, 450, 80, 30);
    
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
    
    /* Add text editor interface */
    gui_create_label(window, "Text Editor - Untitled", 20, 20);
    
    /* Menu bar simulation */
    gui_create_button(window, "File", 20, 50, 60, 25);
    gui_create_button(window, "Edit", 90, 50, 60, 25);
    gui_create_button(window, "View", 160, 50, 60, 25);
    gui_create_button(window, "Help", 230, 50, 60, 25);
    
    /* Editor area */
    gui_create_label(window, "1 |", 10, 90);
    gui_create_label(window, "2 |", 10, 110);
    gui_create_label(window, "3 |", 10, 130);
    gui_create_label(window, "4 |", 10, 150);
    gui_create_label(window, "5 |", 10, 170);
    
    /* Placeholder text */
    gui_create_label(window, "Welcome to Aurora OS Text Editor!", 40, 90);
    gui_create_label(window, "A simple text editing application", 40, 110);
    gui_create_label(window, "", 40, 130);
    gui_create_label(window, "Features:", 40, 150);
    gui_create_label(window, "- Line numbering", 40, 170);
    gui_create_label(window, "- Syntax highlighting (planned)", 40, 190);
    gui_create_label(window, "- Find and replace (planned)", 40, 210);
    gui_create_label(window, "- Multiple file tabs (planned)", 40, 230);
    
    /* Status bar */
    gui_create_label(window, "Line: 1  Col: 1  |  UTF-8  |  Ready", 20, 460);
    
    /* Action buttons */
    gui_create_button(window, "Save", 20, 420, 70, 30);
    gui_create_button(window, "Save As", 100, 420, 80, 30);
    gui_create_button(window, "Close", 600, 420, 70, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_calculator(void) {
    window_t* window = gui_create_window("Calculator", 250, 200, 320, 420);
    if (!window) return -1;
    
    applications[APP_CALCULATOR].window = window;
    
    /* Add calculator interface */
    gui_create_label(window, "Calculator", 20, 20);
    
    /* Display area */
    gui_create_label(window, "0", 240, 60);
    
    /* Button grid - Row 1 */
    gui_create_button(window, "7", 20, 100, 60, 45);
    gui_create_button(window, "8", 90, 100, 60, 45);
    gui_create_button(window, "9", 160, 100, 60, 45);
    gui_create_button(window, "/", 230, 100, 60, 45);
    
    /* Button grid - Row 2 */
    gui_create_button(window, "4", 20, 155, 60, 45);
    gui_create_button(window, "5", 90, 155, 60, 45);
    gui_create_button(window, "6", 160, 155, 60, 45);
    gui_create_button(window, "*", 230, 155, 60, 45);
    
    /* Button grid - Row 3 */
    gui_create_button(window, "1", 20, 210, 60, 45);
    gui_create_button(window, "2", 90, 210, 60, 45);
    gui_create_button(window, "3", 160, 210, 60, 45);
    gui_create_button(window, "-", 230, 210, 60, 45);
    
    /* Button grid - Row 4 */
    gui_create_button(window, "0", 20, 265, 60, 45);
    gui_create_button(window, ".", 90, 265, 60, 45);
    gui_create_button(window, "=", 160, 265, 60, 45);
    gui_create_button(window, "+", 230, 265, 60, 45);
    
    /* Function buttons - Row 5 */
    gui_create_button(window, "C", 20, 320, 60, 45);
    gui_create_button(window, "CE", 90, 320, 60, 45);
    gui_create_button(window, "<-", 160, 320, 60, 45);
    gui_create_button(window, "+/-", 230, 320, 60, 45);
    
    /* Status */
    gui_create_label(window, "Standard Calculator", 20, 380);
    
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
