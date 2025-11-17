/**
 * Aurora OS - Keyboard Shortcuts System Implementation
 */

#include "keyboard_shortcuts.h"
#include "application.h"
#include "../memory/memory.h"
#include <stddef.h>

#define MAX_SHORTCUTS 32

// Registered shortcuts
static keyboard_shortcut_t shortcuts[MAX_SHORTCUTS];
static int shortcut_count = 0;
static uint8_t shortcuts_initialized = 0;

// Default keyboard shortcut callbacks
static void shortcut_toggle_start_menu(void);
static void shortcut_close_window(void);
static void shortcut_minimize_window(void);
static void shortcut_maximize_window(void);

void keyboard_shortcuts_init(void) {
    if (shortcuts_initialized) return;
    
    shortcut_count = 0;
    
    // Register default application shortcuts (Super/Windows key + letter)
    // Super+E = File Manager
    keyboard_shortcuts_register(MOD_SUPER, 'E', APP_FILE_MANAGER, NULL, "Open File Manager");
    
    // Super+T = Terminal
    keyboard_shortcuts_register(MOD_SUPER, 'T', APP_TERMINAL, NULL, "Open Terminal");
    
    // Super+S = Settings
    keyboard_shortcuts_register(MOD_SUPER, 'S', APP_SETTINGS, NULL, "Open Settings");
    
    // Super+I = System Info
    keyboard_shortcuts_register(MOD_SUPER, 'I', APP_SYSTEM_INFO, NULL, "Open System Info");
    
    // Super+N = Notebook
    keyboard_shortcuts_register(MOD_SUPER, 'N', APP_NOTEBOOK, NULL, "Open Notebook");
    
    // Super+P = Paint Editor
    keyboard_shortcuts_register(MOD_SUPER, 'P', APP_PAINT_EDITOR, NULL, "Open Paint Editor");
    
    // Super+V = Image Viewer
    keyboard_shortcuts_register(MOD_SUPER, 'V', APP_IMAGE_VIEWER, NULL, "Open Image Viewer");
    
    // Register window management shortcuts
    // Alt+F4 = Close window (standard)
    keyboard_shortcuts_register(MOD_ALT, 0xF4, APP_MAX, shortcut_close_window, "Close Window");
    
    // Super+D = Show desktop (minimize all)
    keyboard_shortcuts_register(MOD_SUPER, 'D', APP_MAX, shortcut_minimize_window, "Minimize Window");
    
    // Super+Up = Maximize window
    keyboard_shortcuts_register(MOD_SUPER, 0x26, APP_MAX, shortcut_maximize_window, "Maximize Window");
    
    // Super = Toggle start menu
    keyboard_shortcuts_register(MOD_SUPER, 0, APP_MAX, shortcut_toggle_start_menu, "Toggle Start Menu");
    
    shortcuts_initialized = 1;
}

int keyboard_shortcuts_process(uint32_t key_code, uint8_t modifiers) {
    if (!shortcuts_initialized) return 0;
    
    // Search for matching shortcut
    for (int i = 0; i < shortcut_count; i++) {
        if (!shortcuts[i].enabled) continue;
        
        // Check if modifiers and key match
        if (shortcuts[i].modifiers == modifiers && shortcuts[i].key_code == key_code) {
            // Execute the shortcut
            if (shortcuts[i].callback) {
                shortcuts[i].callback();
                return 1;
            } else if (shortcuts[i].app_type < APP_MAX) {
                app_launch(shortcuts[i].app_type);
                return 1;
            }
        }
    }
    
    return 0;
}

int keyboard_shortcuts_register(uint8_t modifiers, uint32_t key_code, 
                                app_type_t app_type, void (*callback)(void),
                                const char* description) {
    if (shortcut_count >= MAX_SHORTCUTS) return -1;
    
    // Check for duplicate
    for (int i = 0; i < shortcut_count; i++) {
        if (shortcuts[i].modifiers == modifiers && shortcuts[i].key_code == key_code) {
            return -1;  // Already registered
        }
    }
    
    shortcuts[shortcut_count].modifiers = modifiers;
    shortcuts[shortcut_count].key_code = key_code;
    shortcuts[shortcut_count].app_type = app_type;
    shortcuts[shortcut_count].callback = callback;
    shortcuts[shortcut_count].description = description;
    shortcuts[shortcut_count].enabled = 1;
    
    shortcut_count++;
    return 0;
}

void keyboard_shortcuts_unregister(uint8_t modifiers, uint32_t key_code) {
    for (int i = 0; i < shortcut_count; i++) {
        if (shortcuts[i].modifiers == modifiers && shortcuts[i].key_code == key_code) {
            // Shift remaining shortcuts down
            for (int j = i; j < shortcut_count - 1; j++) {
                shortcuts[j] = shortcuts[j + 1];
            }
            shortcut_count--;
            return;
        }
    }
}

void keyboard_shortcuts_set_enabled(uint8_t modifiers, uint32_t key_code, uint8_t enabled) {
    for (int i = 0; i < shortcut_count; i++) {
        if (shortcuts[i].modifiers == modifiers && shortcuts[i].key_code == key_code) {
            shortcuts[i].enabled = enabled ? 1 : 0;
            return;
        }
    }
}

int keyboard_shortcuts_get_list(keyboard_shortcut_t* out_shortcuts, int max_shortcuts) {
    if (!out_shortcuts) return 0;
    
    int count = (shortcut_count < max_shortcuts) ? shortcut_count : max_shortcuts;
    for (int i = 0; i < count; i++) {
        out_shortcuts[i] = shortcuts[i];
    }
    
    return count;
}

// Default shortcut callbacks

static void shortcut_toggle_start_menu(void) {
    // This would call the GUI function to toggle start menu
    // Declared as external since it's implemented in gui.c
    extern void gui_toggle_start_menu(void);
    gui_toggle_start_menu();
}

static void shortcut_close_window(void) {
    // Close the focused window
    // This would be implemented by getting the focused window and closing it
    extern window_t* gui_get_window_list(void);
    window_t* windows = gui_get_window_list();
    if (windows && windows->focused) {
        extern void gui_destroy_window(window_t* window);
        gui_destroy_window(windows);
    }
}

static void shortcut_minimize_window(void) {
    // Minimize the focused window
    extern window_t* gui_get_window_list(void);
    extern void gui_minimize_window(window_t* window);
    
    window_t* windows = gui_get_window_list();
    if (windows && windows->focused) {
        gui_minimize_window(windows);
    }
}

static void shortcut_maximize_window(void) {
    // Maximize the focused window
    extern window_t* gui_get_window_list(void);
    extern void gui_maximize_window(window_t* window);
    
    window_t* windows = gui_get_window_list();
    if (windows && windows->focused) {
        gui_maximize_window(windows);
    }
}
