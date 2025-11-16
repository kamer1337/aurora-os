/**
 * Aurora OS - Keyboard Shortcuts System
 * 
 * Provides global keyboard shortcuts for application launching and window management
 */

#ifndef KEYBOARD_SHORTCUTS_H
#define KEYBOARD_SHORTCUTS_H

#include <stdint.h>
#include "application.h"

// Modifier key masks
#define MOD_CTRL  (1 << 0)
#define MOD_ALT   (1 << 1)
#define MOD_SHIFT (1 << 2)
#define MOD_SUPER (1 << 3)  // Windows key / Command key

// Keyboard shortcut structure
typedef struct {
    uint8_t modifiers;      // Combination of MOD_* flags
    uint32_t key_code;      // Key code
    app_type_t app_type;    // Application to launch (if applicable)
    void (*callback)(void); // Callback function for custom actions
    const char* description; // Description of shortcut
    uint8_t enabled;        // Whether shortcut is enabled
} keyboard_shortcut_t;

/**
 * Initialize keyboard shortcuts system
 */
void keyboard_shortcuts_init(void);

/**
 * Process a key press event
 * @param key_code Key code that was pressed
 * @param modifiers Current modifier key state (combination of MOD_* flags)
 * @return 1 if shortcut was handled, 0 otherwise
 */
int keyboard_shortcuts_process(uint32_t key_code, uint8_t modifiers);

/**
 * Register a new keyboard shortcut
 * @param modifiers Modifier keys required
 * @param key_code Key code
 * @param app_type Application to launch (APP_MAX for none)
 * @param callback Callback function (NULL if using app_type)
 * @param description Description of shortcut
 * @return 0 on success, -1 on failure
 */
int keyboard_shortcuts_register(uint8_t modifiers, uint32_t key_code, 
                                app_type_t app_type, void (*callback)(void),
                                const char* description);

/**
 * Unregister a keyboard shortcut
 * @param modifiers Modifier keys
 * @param key_code Key code
 */
void keyboard_shortcuts_unregister(uint8_t modifiers, uint32_t key_code);

/**
 * Enable or disable a keyboard shortcut
 * @param modifiers Modifier keys
 * @param key_code Key code
 * @param enabled 1 to enable, 0 to disable
 */
void keyboard_shortcuts_set_enabled(uint8_t modifiers, uint32_t key_code, uint8_t enabled);

/**
 * Get list of all registered shortcuts (for display in help/settings)
 * @param out_shortcuts Output array of shortcuts
 * @param max_shortcuts Maximum number of shortcuts to return
 * @return Number of shortcuts returned
 */
int keyboard_shortcuts_get_list(keyboard_shortcut_t* out_shortcuts, int max_shortcuts);

#endif // KEYBOARD_SHORTCUTS_H
