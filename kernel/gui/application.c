/**
 * Aurora OS - Application Framework Implementation
 * 
 * Framework for launching and managing GUI applications
 */

#include "application.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
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
    
    // Add some placeholder content
    gui_create_label(window, "File Manager - Coming Soon", 20, 20);
    gui_create_label(window, "This will allow you to browse", 20, 50);
    gui_create_label(window, "and manage files on the system.", 20, 70);
    
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
    window_t* window = gui_create_window("System Information", 200, 150, 450, 350);
    if (!window) return -1;
    
    applications[APP_SYSTEM_INFO].window = window;
    
    // Add system information
    gui_create_label(window, "Aurora OS v1.0", 20, 20);
    gui_create_label(window, "Kernel Version: 1.0.0", 20, 50);
    gui_create_label(window, "Build Date: Nov 2025", 20, 80);
    gui_create_label(window, "Memory: Available", 20, 110);
    gui_create_label(window, "CPU: x86 Compatible", 20, 140);
    gui_create_label(window, "GUI: Phase 4 Complete", 20, 170);
    
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
