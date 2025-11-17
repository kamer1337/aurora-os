/**
 * Aurora OS - Application Framework Implementation
 * 
 * Framework for launching and managing GUI applications
 */

#include "application.h"
#include "gui.h"
#include "framebuffer.h"
#include "desktop_config.h"
#include "terminal.h"
#include "webview.h"
#include "file_explorer.h"
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
static int launch_paint_editor(void);
static int launch_image_viewer(void);
static int launch_notebook(void);
static int launch_my_pc(void);
static int launch_recycle_bin(void);
static int launch_uninstaller(void);
static int launch_task_manager(void);
static int launch_control_panel(void);
static int launch_help(void);
static int launch_web_browser(void);
static int launch_file_explorer(void);

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
    
    applications[APP_PAINT_EDITOR] = (application_t){
        .type = APP_PAINT_EDITOR,
        .name = "Paint Editor",
        .description = "Create and edit images",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_IMAGE_VIEWER] = (application_t){
        .type = APP_IMAGE_VIEWER,
        .name = "Image Viewer",
        .description = "View image files",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_NOTEBOOK] = (application_t){
        .type = APP_NOTEBOOK,
        .name = "Notebook",
        .description = "Take notes and organize tasks",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_MY_PC] = (application_t){
        .type = APP_MY_PC,
        .name = "My PC",
        .description = "View system resources and storage",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_RECYCLE_BIN] = (application_t){
        .type = APP_RECYCLE_BIN,
        .name = "Recycle Bin",
        .description = "Manage deleted files",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_UNINSTALLER] = (application_t){
        .type = APP_UNINSTALLER,
        .name = "Apps Uninstaller",
        .description = "Uninstall applications",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_TASK_MANAGER] = (application_t){
        .type = APP_TASK_MANAGER,
        .name = "Task Manager",
        .description = "Monitor system resources and processes",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_CONTROL_PANEL] = (application_t){
        .type = APP_CONTROL_PANEL,
        .name = "Control Panel",
        .description = "System configuration and settings",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_HELP] = (application_t){
        .type = APP_HELP,
        .name = "Help & Support",
        .description = "Aurora OS help and documentation",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_WEB_BROWSER] = (application_t){
        .type = APP_WEB_BROWSER,
        .name = "Web Browser",
        .description = "Browse the web",
        .window = NULL,
        .running = 0
    };
    
    applications[APP_FILE_EXPLORER] = (application_t){
        .type = APP_FILE_EXPLORER,
        .name = "File Explorer",
        .description = "Advanced file browser with extensions toggle",
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
        case APP_PAINT_EDITOR:
            result = launch_paint_editor();
            break;
        case APP_IMAGE_VIEWER:
            result = launch_image_viewer();
            break;
        case APP_NOTEBOOK:
            result = launch_notebook();
            break;
        case APP_MY_PC:
            result = launch_my_pc();
            break;
        case APP_RECYCLE_BIN:
            result = launch_recycle_bin();
            break;
        case APP_UNINSTALLER:
            result = launch_uninstaller();
            break;
        case APP_TASK_MANAGER:
            result = launch_task_manager();
            break;
        case APP_CONTROL_PANEL:
            result = launch_control_panel();
            break;
        case APP_HELP:
            result = launch_help();
            break;
        case APP_WEB_BROWSER:
            result = launch_web_browser();
            break;
        case APP_FILE_EXPLORER:
            result = launch_file_explorer();
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
    
    /* Add file manager interface with tabs support */
    gui_create_label(window, "File Manager", 20, 20);
    
    /* Tab bar for folder navigation */
    gui_create_label(window, "Tabs:", 20, 50);
    gui_create_button(window, "/ (Root)", 70, 45, 80, 30);
    gui_create_button(window, "Home", 155, 45, 70, 30);
    gui_create_button(window, "+ New Tab", 230, 45, 90, 30);
    
    /* Location bar */
    gui_create_label(window, "Location: /", 20, 85);
    
    /* Add storage device information */
    gui_create_label(window, "Storage Devices:", 20, 115);
    
    /* Try to initialize and detect storage devices */
    storage_init();
    int device_count = storage_detect_devices();
    
    if (device_count > 0) {
        char info_text[128];
        int y_offset = 145;
        
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
        gui_create_label(window, "No storage devices detected", 40, 145);
    }
    
    /* File operations toolbar */
    gui_create_label(window, "File Operations:", 20, 245);
    gui_create_button(window, "Copy", 20, 270, 70, 30);
    gui_create_button(window, "Move", 100, 270, 70, 30);
    gui_create_button(window, "Delete", 180, 270, 70, 30);
    gui_create_button(window, "Rename", 260, 270, 70, 30);
    gui_create_button(window, "New Folder", 340, 270, 100, 30);
    
    /* Drive management */
    gui_create_label(window, "Drive Management:", 20, 315);
    gui_create_button(window, "Mount", 20, 340, 70, 30);
    gui_create_button(window, "Unmount", 100, 340, 80, 30);
    
    /* Add file operation buttons */
    gui_create_button(window, "Refresh", 20, 350, 80, 30);
    gui_create_button(window, "Properties", 110, 350, 90, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_terminal(void) {
    // Initialize terminal system
    terminal_init();
    
    // Create terminal window
    window_t* window = terminal_create();
    if (!window) return -1;
    
    applications[APP_TERMINAL].window = window;
    
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

static int launch_paint_editor(void) {
    window_t* window = gui_create_window("Paint Editor", 120, 100, 700, 550);
    if (!window) return -1;
    
    applications[APP_PAINT_EDITOR].window = window;
    
    /* Add paint editor interface */
    gui_create_label(window, "Paint Editor - Drawing Canvas", 20, 20);
    
    /* Tool palette */
    gui_create_label(window, "Tools:", 20, 60);
    gui_create_button(window, "Brush", 20, 85, 80, 30);
    gui_create_button(window, "Pencil", 20, 120, 80, 30);
    gui_create_button(window, "Eraser", 20, 155, 80, 30);
    gui_create_button(window, "Fill", 20, 190, 80, 30);
    gui_create_button(window, "Line", 20, 225, 80, 30);
    gui_create_button(window, "Rectangle", 20, 260, 80, 30);
    gui_create_button(window, "Circle", 20, 295, 80, 30);
    gui_create_button(window, "Text", 20, 330, 80, 30);
    
    /* Color palette */
    gui_create_label(window, "Colors:", 20, 375);
    gui_create_button(window, "Black", 20, 400, 40, 25);
    gui_create_button(window, "White", 65, 400, 40, 25);
    gui_create_button(window, "Red", 20, 430, 40, 25);
    gui_create_button(window, "Green", 65, 430, 40, 25);
    gui_create_button(window, "Blue", 20, 460, 40, 25);
    gui_create_button(window, "Yellow", 65, 460, 40, 25);
    
    /* Canvas area indicator */
    gui_create_label(window, "Canvas Area (Click and drag to draw)", 120, 60);
    
    /* File operations */
    gui_create_button(window, "New", 120, 495, 70, 30);
    gui_create_button(window, "Open", 200, 495, 70, 30);
    gui_create_button(window, "Save", 280, 495, 70, 30);
    gui_create_button(window, "Clear", 360, 495, 70, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_image_viewer(void) {
    window_t* window = gui_create_window("Image Viewer", 140, 120, 680, 520);
    if (!window) return -1;
    
    applications[APP_IMAGE_VIEWER].window = window;
    
    /* Add image viewer interface */
    gui_create_label(window, "Image Viewer", 20, 20);
    gui_create_label(window, "Supported formats: BMP, PNG, JPG, GIF", 20, 45);
    
    /* Image display area indicator */
    gui_create_label(window, "Image Display Area", 250, 200);
    gui_create_label(window, "(No image loaded)", 260, 230);
    
    /* Image controls */
    gui_create_label(window, "Zoom:", 20, 80);
    gui_create_button(window, "Fit", 70, 75, 60, 30);
    gui_create_button(window, "100%", 140, 75, 60, 30);
    gui_create_button(window, "Zoom In", 210, 75, 80, 30);
    gui_create_button(window, "Zoom Out", 300, 75, 80, 30);
    
    /* Navigation controls */
    gui_create_button(window, "Previous", 20, 465, 90, 30);
    gui_create_button(window, "Next", 120, 465, 90, 30);
    gui_create_button(window, "Rotate Left", 220, 465, 100, 30);
    gui_create_button(window, "Rotate Right", 330, 465, 100, 30);
    
    /* File operations */
    gui_create_button(window, "Open Image", 540, 465, 110, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_notebook(void) {
    window_t* window = gui_create_window("Notebook", 160, 140, 650, 500);
    if (!window) return -1;
    
    applications[APP_NOTEBOOK].window = window;
    
    /* Add notebook interface with tabs support */
    gui_create_label(window, "Notebook - Organize Your Notes", 20, 20);
    
    /* Tab bar */
    gui_create_label(window, "Tabs:", 20, 50);
    gui_create_button(window, "Note 1", 70, 45, 80, 30);
    gui_create_button(window, "Note 2", 155, 45, 80, 30);
    gui_create_button(window, "Note 3", 240, 45, 80, 30);
    gui_create_button(window, "+ New Tab", 325, 45, 90, 30);
    
    /* Editor area */
    gui_create_label(window, "Title: Untitled Note", 20, 90);
    gui_create_label(window, "Content:", 20, 120);
    gui_create_label(window, "(Type your notes here)", 20, 150);
    
    /* Formatting toolbar */
    gui_create_label(window, "Format:", 20, 380);
    gui_create_button(window, "Bold", 80, 375, 60, 30);
    gui_create_button(window, "Italic", 145, 375, 60, 30);
    gui_create_button(window, "List", 210, 375, 60, 30);
    gui_create_button(window, "Heading", 275, 375, 75, 30);
    
    /* Note management buttons */
    gui_create_button(window, "Save", 20, 430, 80, 30);
    gui_create_button(window, "Save As", 110, 430, 80, 30);
    gui_create_button(window, "Delete Note", 200, 430, 100, 30);
    gui_create_button(window, "Export", 310, 430, 80, 30);
    
    /* Battery indicator placeholder */
    gui_create_label(window, "Battery: 85%", 520, 430);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_my_pc(void) {
    window_t* window = gui_create_window("My PC", 100, 80, 720, 560);
    if (!window) return -1;
    
    applications[APP_MY_PC].window = window;
    
    /* Header */
    gui_create_label(window, "My PC - Computer Resources", 20, 20);
    
    /* System Overview */
    gui_create_label(window, "System Information:", 20, 60);
    gui_create_label(window, "Computer Name: AURORA-PC", 40, 85);
    gui_create_label(window, "Operating System: Aurora OS", 40, 110);
    gui_create_label(window, "Processor: Intel Core i7", 40, 135);
    gui_create_label(window, "Installed RAM: 8.00 GB", 40, 160);
    
    /* Storage Devices */
    gui_create_label(window, "Storage Devices:", 20, 200);
    
    /* Drive C: */
    gui_create_label(window, "[C:] System Drive", 40, 230);
    gui_create_label(window, "Total: 256 GB | Used: 128 GB | Free: 128 GB", 60, 255);
    gui_create_label(window, "Capacity: 50%", 60, 280);
    gui_create_button(window, "Open", 580, 250, 100, 30);
    
    /* Drive D: */
    gui_create_label(window, "[D:] Data Drive", 40, 310);
    gui_create_label(window, "Total: 512 GB | Used: 200 GB | Free: 312 GB", 60, 335);
    gui_create_label(window, "Capacity: 39%", 60, 360);
    gui_create_button(window, "Open", 580, 330, 100, 30);
    
    /* Network Locations */
    gui_create_label(window, "Network Locations:", 20, 400);
    gui_create_label(window, "No network locations configured", 40, 425);
    
    /* Action buttons */
    gui_create_button(window, "System Properties", 20, 480, 150, 30);
    gui_create_button(window, "Device Manager", 180, 480, 140, 30);
    gui_create_button(window, "Disk Cleanup", 330, 480, 120, 30);
    gui_create_button(window, "Refresh", 580, 480, 100, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_recycle_bin(void) {
    window_t* window = gui_create_window("Recycle Bin", 120, 100, 700, 540);
    if (!window) return -1;
    
    applications[APP_RECYCLE_BIN].window = window;
    
    /* Header */
    gui_create_label(window, "Recycle Bin - Deleted Items", 20, 20);
    gui_create_label(window, "Items in Recycle Bin: 3", 20, 45);
    
    /* Toolbar */
    gui_create_button(window, "Empty Bin", 20, 75, 100, 30);
    gui_create_button(window, "Restore All", 130, 75, 100, 30);
    gui_create_button(window, "Refresh", 240, 75, 80, 30);
    
    /* Deleted files list header */
    gui_create_label(window, "Name", 40, 120);
    gui_create_label(window, "Original Location", 250, 120);
    gui_create_label(window, "Date Deleted", 480, 120);
    
    /* Sample deleted files */
    gui_create_label(window, "document.txt", 40, 155);
    gui_create_label(window, "/home/user/documents/", 250, 155);
    gui_create_label(window, "Nov 17, 2025", 480, 155);
    gui_create_button(window, "Restore", 600, 150, 70, 30);
    
    gui_create_label(window, "photo.jpg", 40, 200);
    gui_create_label(window, "/home/user/pictures/", 250, 200);
    gui_create_label(window, "Nov 16, 2025", 480, 200);
    gui_create_button(window, "Restore", 600, 195, 70, 30);
    
    gui_create_label(window, "oldapp.exe", 40, 245);
    gui_create_label(window, "/home/user/programs/", 250, 245);
    gui_create_label(window, "Nov 15, 2025", 480, 245);
    gui_create_button(window, "Restore", 600, 240, 70, 30);
    
    /* Info section */
    gui_create_label(window, "Total size: 15.2 MB", 40, 290);
    gui_create_label(window, "Space available after empty: 15.2 MB", 40, 315);
    
    /* Recycle Bin options */
    gui_create_label(window, "Recycle Bin Settings:", 20, 360);
    gui_create_label(window, "[ ] Don't move files to Recycle Bin", 40, 385);
    gui_create_label(window, "[ ] Display deletion confirmation", 40, 410);
    gui_create_label(window, "Maximum size: 10% of each drive", 40, 435);
    
    /* Action buttons */
    gui_create_button(window, "Properties", 20, 480, 100, 30);
    gui_create_button(window, "Close", 580, 480, 90, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_uninstaller(void) {
    window_t* window = gui_create_window("Apps Uninstaller", 140, 110, 680, 550);
    if (!window) return -1;
    
    applications[APP_UNINSTALLER].window = window;
    
    /* Header */
    gui_create_label(window, "Applications Manager - Uninstall Programs", 20, 20);
    gui_create_label(window, "Total installed applications: 10", 20, 45);
    
    /* Toolbar */
    gui_create_button(window, "Uninstall", 20, 75, 100, 30);
    gui_create_button(window, "Modify", 130, 75, 80, 30);
    gui_create_button(window, "Refresh", 220, 75, 80, 30);
    gui_create_label(window, "Sort by:", 340, 80);
    gui_create_button(window, "Name", 400, 75, 60, 30);
    gui_create_button(window, "Size", 465, 75, 60, 30);
    gui_create_button(window, "Date", 530, 75, 60, 30);
    
    /* Applications list header */
    gui_create_label(window, "Application", 40, 120);
    gui_create_label(window, "Publisher", 280, 120);
    gui_create_label(window, "Size", 450, 120);
    gui_create_label(window, "Install Date", 530, 120);
    
    /* List of installed applications */
    gui_create_label(window, "File Manager", 40, 155);
    gui_create_label(window, "Aurora OS", 280, 155);
    gui_create_label(window, "2.5 MB", 450, 155);
    gui_create_label(window, "Nov 10, 2025", 530, 155);
    
    gui_create_label(window, "Text Editor", 40, 190);
    gui_create_label(window, "Aurora OS", 280, 190);
    gui_create_label(window, "1.8 MB", 450, 190);
    gui_create_label(window, "Nov 10, 2025", 530, 190);
    
    gui_create_label(window, "Calculator", 40, 225);
    gui_create_label(window, "Aurora OS", 280, 225);
    gui_create_label(window, "0.5 MB", 450, 225);
    gui_create_label(window, "Nov 10, 2025", 530, 225);
    
    gui_create_label(window, "Paint Editor", 40, 260);
    gui_create_label(window, "Aurora OS", 280, 260);
    gui_create_label(window, "3.2 MB", 450, 260);
    gui_create_label(window, "Nov 10, 2025", 530, 260);
    
    gui_create_label(window, "Image Viewer", 40, 295);
    gui_create_label(window, "Aurora OS", 280, 295);
    gui_create_label(window, "1.2 MB", 450, 295);
    gui_create_label(window, "Nov 10, 2025", 530, 295);
    
    gui_create_label(window, "Terminal", 40, 330);
    gui_create_label(window, "Aurora OS", 280, 330);
    gui_create_label(window, "1.0 MB", 450, 330);
    gui_create_label(window, "Nov 10, 2025", 530, 330);
    
    gui_create_label(window, "Notebook", 40, 365);
    gui_create_label(window, "Aurora OS", 280, 365);
    gui_create_label(window, "2.0 MB", 450, 365);
    gui_create_label(window, "Nov 10, 2025", 530, 365);
    
    gui_create_label(window, "Disk Manager", 40, 400);
    gui_create_label(window, "Aurora OS", 280, 400);
    gui_create_label(window, "1.5 MB", 450, 400);
    gui_create_label(window, "Nov 10, 2025", 530, 400);
    
    gui_create_label(window, "System Settings", 40, 435);
    gui_create_label(window, "Aurora OS", 280, 435);
    gui_create_label(window, "2.8 MB", 450, 435);
    gui_create_label(window, "Nov 10, 2025", 530, 435);
    
    gui_create_label(window, "System Info", 40, 470);
    gui_create_label(window, "Aurora OS", 280, 470);
    gui_create_label(window, "0.8 MB", 450, 470);
    gui_create_label(window, "Nov 10, 2025", 530, 470);
    
    /* Status bar */
    gui_create_label(window, "Total size: 17.3 MB", 20, 505);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_task_manager(void) {
    window_t* window = gui_create_window("Task Manager", 100, 80, 750, 580);
    if (!window) return -1;
    
    applications[APP_TASK_MANAGER].window = window;
    
    /* Header */
    gui_create_label(window, "Task Manager - System Monitor", 20, 20);
    
    /* Tabs */
    gui_create_button(window, "Processes", 20, 55, 100, 30);
    gui_create_button(window, "Performance", 125, 55, 110, 30);
    gui_create_button(window, "Services", 240, 55, 90, 30);
    gui_create_button(window, "Startup", 335, 55, 80, 30);
    
    /* Process list header */
    gui_create_label(window, "Name", 30, 100);
    gui_create_label(window, "PID", 280, 100);
    gui_create_label(window, "CPU %", 360, 100);
    gui_create_label(window, "Memory", 450, 100);
    gui_create_label(window, "Status", 570, 100);
    
    /* Sample processes */
    gui_create_label(window, "kernel.bin", 30, 135);
    gui_create_label(window, "1", 280, 135);
    gui_create_label(window, "5%", 360, 135);
    gui_create_label(window, "128 MB", 450, 135);
    gui_create_label(window, "Running", 570, 135);
    
    gui_create_label(window, "gui_manager", 30, 170);
    gui_create_label(window, "2", 280, 170);
    gui_create_label(window, "15%", 360, 170);
    gui_create_label(window, "64 MB", 450, 170);
    gui_create_label(window, "Running", 570, 170);
    
    gui_create_label(window, "file_manager", 30, 205);
    gui_create_label(window, "3", 280, 205);
    gui_create_label(window, "3%", 360, 205);
    gui_create_label(window, "32 MB", 450, 205);
    gui_create_label(window, "Running", 570, 205);
    
    gui_create_label(window, "terminal", 30, 240);
    gui_create_label(window, "4", 280, 240);
    gui_create_label(window, "2%", 360, 240);
    gui_create_label(window, "16 MB", 450, 240);
    gui_create_label(window, "Running", 570, 240);
    
    gui_create_label(window, "paint_editor", 30, 275);
    gui_create_label(window, "5", 280, 275);
    gui_create_label(window, "8%", 360, 275);
    gui_create_label(window, "48 MB", 450, 275);
    gui_create_label(window, "Running", 570, 275);
    
    /* System resources summary */
    gui_create_label(window, "System Resources:", 20, 330);
    gui_create_label(window, "CPU Usage: 33%", 40, 355);
    gui_create_label(window, "Memory: 288 MB / 8 GB (3.5%)", 40, 380);
    gui_create_label(window, "Disk: 128 GB / 256 GB (50%)", 40, 405);
    gui_create_label(window, "Network: 2.5 MB/s", 40, 430);
    gui_create_label(window, "Uptime: 2 hours 15 minutes", 40, 455);
    
    /* Action buttons */
    gui_create_button(window, "End Task", 20, 510, 100, 30);
    gui_create_button(window, "New Task", 130, 510, 100, 30);
    gui_create_button(window, "Refresh", 240, 510, 100, 30);
    gui_create_button(window, "Details", 620, 510, 100, 30);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_control_panel(void) {
    window_t* window = gui_create_window("Control Panel", 120, 90, 720, 600);
    if (!window) return -1;
    
    applications[APP_CONTROL_PANEL].window = window;
    
    /* Header */
    gui_create_label(window, "Control Panel - System Settings", 20, 20);
    gui_create_label(window, "Adjust settings and configure your system", 20, 45);
    
    /* System and Security */
    gui_create_label(window, "System and Security", 20, 85);
    gui_create_button(window, "System Information", 40, 110, 180, 35);
    gui_create_button(window, "Security Settings", 230, 110, 180, 35);
    gui_create_button(window, "Firewall", 420, 110, 180, 35);
    
    /* Hardware and Sound */
    gui_create_label(window, "Hardware and Sound", 20, 165);
    gui_create_button(window, "Device Manager", 40, 190, 180, 35);
    gui_create_button(window, "Sound Settings", 230, 190, 180, 35);
    gui_create_button(window, "Display Settings", 420, 190, 180, 35);
    
    /* Network and Internet */
    gui_create_label(window, "Network and Internet", 20, 245);
    gui_create_button(window, "Network Status", 40, 270, 180, 35);
    gui_create_button(window, "Network Settings", 230, 270, 180, 35);
    gui_create_button(window, "Proxy Settings", 420, 270, 180, 35);
    
    /* Appearance and Personalization */
    gui_create_label(window, "Appearance and Personalization", 20, 325);
    gui_create_button(window, "Themes", 40, 350, 180, 35);
    gui_create_button(window, "Wallpaper", 230, 350, 180, 35);
    gui_create_button(window, "Colors", 420, 350, 180, 35);
    
    /* User Accounts */
    gui_create_label(window, "User Accounts", 20, 405);
    gui_create_button(window, "Manage Accounts", 40, 430, 180, 35);
    gui_create_button(window, "Change Password", 230, 430, 180, 35);
    gui_create_button(window, "User Permissions", 420, 430, 180, 35);
    
    /* Programs */
    gui_create_label(window, "Programs", 20, 485);
    gui_create_button(window, "Uninstall Programs", 40, 510, 180, 35);
    gui_create_button(window, "Default Programs", 230, 510, 180, 35);
    gui_create_button(window, "Startup Programs", 420, 510, 180, 35);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_help(void) {
    window_t* window = gui_create_window("Help & Support", 140, 100, 680, 550);
    if (!window) return -1;
    
    applications[APP_HELP].window = window;
    
    /* Header */
    gui_create_label(window, "Aurora OS Help & Support", 20, 20);
    gui_create_label(window, "Find answers and learn about Aurora OS", 20, 45);
    
    /* Search */
    gui_create_label(window, "Search for help:", 20, 85);
    gui_create_button(window, "Search", 540, 80, 110, 30);
    
    /* Quick Help Topics */
    gui_create_label(window, "Quick Help Topics:", 20, 130);
    
    gui_create_button(window, "Getting Started", 40, 160, 280, 40);
    gui_create_label(window, "Learn the basics of Aurora OS", 60, 210);
    
    gui_create_button(window, "Desktop Environment", 360, 160, 280, 40);
    gui_create_label(window, "Using the desktop and windows", 380, 210);
    
    gui_create_button(window, "File Management", 40, 240, 280, 40);
    gui_create_label(window, "Working with files and folders", 60, 290);
    
    gui_create_button(window, "System Settings", 360, 240, 280, 40);
    gui_create_label(window, "Configuring your system", 380, 290);
    
    gui_create_button(window, "Applications", 40, 320, 280, 40);
    gui_create_label(window, "Using built-in applications", 60, 370);
    
    gui_create_button(window, "Troubleshooting", 360, 320, 280, 40);
    gui_create_label(window, "Solving common problems", 380, 370);
    
    /* Support Resources */
    gui_create_label(window, "Support Resources:", 20, 415);
    gui_create_label(window, "• Online Documentation: docs.aurora-os.org", 40, 440);
    gui_create_label(window, "• Community Forums: forums.aurora-os.org", 40, 465);
    gui_create_label(window, "• Version: 1.0.0 (Release Candidate)", 40, 490);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return 0;
}

static int launch_web_browser(void) {
    // Initialize browser system
    browser_init();
    
    // Create browser window
    window_t* window = browser_create();
    if (!window) return -1;
    
    applications[APP_WEB_BROWSER].window = window;
    
    return 0;
}

static int launch_file_explorer(void) {
    // Initialize file explorer
    file_explorer_init();
    
    // Create file explorer window
    window_t* window = file_explorer_create(NULL);
    if (!window) return -1;
    
    applications[APP_FILE_EXPLORER].window = window;
    
    return 0;
}
