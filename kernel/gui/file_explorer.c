/**
 * Aurora OS - File Explorer Implementation
 * 
 * Enhanced file explorer with extension visibility and hidden file support
 */

#include "file_explorer.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../../filesystem/vfs/vfs.h"
#include <stddef.h>

// String utilities
static size_t fe_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void fe_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int fe_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void fe_strcat(char* dest, const char* src, size_t max) {
    size_t dest_len = fe_strlen(dest);
    size_t i = 0;
    while (src && src[i] && (dest_len + i) < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

static void extract_extension(const char* filename, char* ext, size_t max) {
    const char* dot = NULL;
    const char* p = filename;
    
    // Find last dot
    while (*p) {
        if (*p == '.') dot = p;
        p++;
    }
    
    if (dot && *(dot + 1)) {
        fe_strcpy(ext, dot + 1, max);
    } else {
        ext[0] = '\0';
    }
}

static int is_hidden_file(const char* filename) {
    // Files starting with '.' are hidden on Unix-like systems
    return (filename && filename[0] == '.');
}

// File explorer state
static explorer_state_t explorer_state;
static uint8_t explorer_initialized = 0;

// Static storage for tabs
static explorer_tab_t tabs_storage[EXPLORER_MAX_TABS];

void file_explorer_init(void) {
    if (explorer_initialized) return;
    
    explorer_state.window = NULL;
    explorer_state.tabs = NULL;
    explorer_state.active_tab = NULL;
    explorer_state.tab_count = 0;
    explorer_state.view_mode = VIEW_MODE_DETAILS;
    explorer_state.show_extensions = 1;  // Show by default
    explorer_state.show_hidden = 0;      // Hide by default
    explorer_state.show_system_files = 0; // Hide by default
    
    explorer_initialized = 1;
}

static void populate_directory_listing(explorer_tab_t* tab) {
    // In a real implementation, this would read from VFS
    // For now, create sample entries
    tab->file_count = 0;
    
    // Add parent directory entry if not at root
    if (fe_strcmp(tab->path, "/") != 0) {
        file_entry_t* entry = &tab->files[tab->file_count++];
        fe_strcpy(entry->name, "..", sizeof(entry->name));
        fe_strcpy(entry->path, tab->path, sizeof(entry->path));
        entry->is_directory = 1;
        entry->is_hidden = 0;
        entry->size = 0;
        entry->extension[0] = '\0';
    }
    
    // Add sample directories
    const char* dirs[] = {"Documents", "Downloads", "Pictures", "Music", "Videos", "Desktop", ".config"};
    for (int i = 0; i < 7 && tab->file_count < EXPLORER_MAX_FILES; i++) {
        file_entry_t* entry = &tab->files[tab->file_count++];
        fe_strcpy(entry->name, dirs[i], sizeof(entry->name));
        fe_strcpy(entry->path, tab->path, sizeof(entry->path));
        fe_strcat(entry->path, dirs[i], sizeof(entry->path));
        entry->is_directory = 1;
        entry->is_hidden = is_hidden_file(dirs[i]);
        entry->size = 0;
        entry->extension[0] = '\0';
    }
    
    // Add sample files
    const char* files[] = {
        "readme.txt", "config.ini", "data.json", "image.png", 
        "document.pdf", "script.sh", ".bashrc", ".gitignore"
    };
    const uint32_t sizes[] = {1024, 512, 2048, 153600, 204800, 4096, 256, 128};
    
    for (int i = 0; i < 8 && tab->file_count < EXPLORER_MAX_FILES; i++) {
        file_entry_t* entry = &tab->files[tab->file_count++];
        fe_strcpy(entry->name, files[i], sizeof(entry->name));
        fe_strcpy(entry->path, tab->path, sizeof(entry->path));
        fe_strcat(entry->path, files[i], sizeof(entry->path));
        entry->is_directory = 0;
        entry->is_hidden = is_hidden_file(files[i]);
        entry->size = sizes[i];
        extract_extension(files[i], entry->extension, sizeof(entry->extension));
    }
}

window_t* file_explorer_create(const char* initial_path) {
    if (!explorer_initialized) {
        file_explorer_init();
    }
    
    window_t* window = gui_create_window("File Explorer", 120, 80, 760, 560);
    if (!window) return NULL;
    
    explorer_state.window = window;
    
    // Create navigation bar
    gui_create_button(window, "<", 10, 10, 40, 30);        // Back
    gui_create_button(window, "^", 55, 10, 40, 30);        // Up
    gui_create_button(window, "Home", 100, 10, 60, 30);    // Home
    gui_create_button(window, "Refresh", 165, 10, 70, 30); // Refresh
    
    // View options
    gui_create_button(window, "List", 240, 10, 50, 30);    // List view
    gui_create_button(window, "Icons", 295, 10, 60, 30);   // Icon view
    gui_create_button(window, "Details", 360, 10, 70, 30); // Details view
    
    // Path bar
    const char* display_path = initial_path ? initial_path : "/home/user";
    char path_label[256];
    fe_strcpy(path_label, "Path: ", sizeof(path_label));
    fe_strcat(path_label, display_path, sizeof(path_label));
    gui_create_label(window, path_label, 10, 50);
    
    // Tab bar
    gui_create_label(window, "Tab 1", 10, 75);
    gui_create_button(window, "+", 70, 73, 25, 20);        // New tab
    
    // Toolbar
    gui_create_button(window, "New Folder", 10, 100, 90, 25);
    gui_create_button(window, "Delete", 105, 100, 70, 25);
    gui_create_button(window, "Rename", 180, 100, 70, 25);
    gui_create_button(window, "Copy", 255, 100, 60, 25);
    gui_create_button(window, "Paste", 320, 100, 60, 25);
    gui_create_button(window, "Properties", 385, 100, 85, 25);
    
    // View options toggle
    gui_create_label(window, "View Options:", 480, 105);
    gui_create_button(window, "Ext: ON", 560, 100, 70, 25);   // Toggle extensions
    gui_create_button(window, "Hidden: OFF", 635, 100, 100, 25); // Toggle hidden
    
    // File list header
    gui_create_label(window, "Name", 15, 135);
    gui_create_label(window, "Type", 300, 135);
    gui_create_label(window, "Size", 450, 135);
    gui_create_label(window, "Ext", 570, 135);
    
    // Separator line (simulated with label)
    gui_create_label(window, "-------------------------------------------------------", 15, 155);
    
    // File listing area (sample entries)
    int y = 170;
    gui_create_label(window, "[DIR]  Documents", 15, y);
    gui_create_label(window, "Folder", 300, y);
    gui_create_label(window, "--", 450, y);
    gui_create_label(window, "--", 570, y);
    
    y += 20;
    gui_create_label(window, "[DIR]  Downloads", 15, y);
    gui_create_label(window, "Folder", 300, y);
    gui_create_label(window, "--", 450, y);
    gui_create_label(window, "--", 570, y);
    
    y += 20;
    gui_create_label(window, "[DIR]  Pictures", 15, y);
    gui_create_label(window, "Folder", 300, y);
    gui_create_label(window, "--", 450, y);
    gui_create_label(window, "--", 570, y);
    
    y += 20;
    gui_create_label(window, "[FILE] readme.txt", 15, y);
    gui_create_label(window, "Text", 300, y);
    gui_create_label(window, "1 KB", 450, y);
    gui_create_label(window, ".txt", 570, y);
    
    y += 20;
    gui_create_label(window, "[FILE] image.png", 15, y);
    gui_create_label(window, "Image", 300, y);
    gui_create_label(window, "150 KB", 450, y);
    gui_create_label(window, ".png", 570, y);
    
    y += 20;
    gui_create_label(window, "[FILE] document.pdf", 15, y);
    gui_create_label(window, "Document", 300, y);
    gui_create_label(window, "200 KB", 450, y);
    gui_create_label(window, ".pdf", 570, y);
    
    // Status bar
    gui_create_label(window, "6 items | Extensions: ON | Hidden files: OFF", 10, 520);
    
    // Create initial tab
    file_explorer_new_tab(initial_path);
    
    gui_show_window(window);
    gui_focus_window(window);
    
    return window;
}

void file_explorer_destroy(window_t* window) {
    if (window && explorer_state.window == window) {
        explorer_state.window = NULL;
        explorer_state.tabs = NULL;
        explorer_state.active_tab = NULL;
        explorer_state.tab_count = 0;
        
        gui_destroy_window(window);
    }
}

int file_explorer_navigate(const char* path) {
    if (!path || !explorer_state.active_tab) {
        return -1;
    }
    
    fe_strcpy(explorer_state.active_tab->path, path, EXPLORER_MAX_PATH);
    populate_directory_listing(explorer_state.active_tab);
    
    return 0;
}

void file_explorer_refresh(void) {
    if (explorer_state.active_tab) {
        populate_directory_listing(explorer_state.active_tab);
    }
}

void file_explorer_go_up(void) {
    if (!explorer_state.active_tab) return;
    
    char* path = explorer_state.active_tab->path;
    size_t len = fe_strlen(path);
    
    if (len > 1) {
        // Find last slash
        for (int i = len - 2; i >= 0; i--) {
            if (path[i] == '/') {
                path[i + 1] = '\0';
                break;
            }
        }
        populate_directory_listing(explorer_state.active_tab);
    }
}

void file_explorer_toggle_extensions(void) {
    explorer_state.show_extensions = !explorer_state.show_extensions;
}

void file_explorer_toggle_hidden(void) {
    explorer_state.show_hidden = !explorer_state.show_hidden;
}

void file_explorer_toggle_system(void) {
    explorer_state.show_system_files = !explorer_state.show_system_files;
}

void file_explorer_set_view_mode(view_mode_t mode) {
    explorer_state.view_mode = mode;
}

view_mode_t file_explorer_get_view_mode(void) {
    return explorer_state.view_mode;
}

int file_explorer_create_folder(const char* name) {
    if (!name || !explorer_state.active_tab) {
        return -1;
    }
    
    // In real implementation, would create via VFS
    return 0;
}

int file_explorer_delete_selected(void) {
    if (!explorer_state.active_tab) {
        return -1;
    }
    
    // In real implementation, would delete via VFS
    return 0;
}

int file_explorer_rename_selected(const char* new_name) {
    if (!new_name || !explorer_state.active_tab) {
        return -1;
    }
    
    // In real implementation, would rename via VFS
    return 0;
}

void file_explorer_show_properties(void) {
    if (!explorer_state.active_tab || !explorer_state.window) {
        return;
    }
    
    // Create a properties dialog window
    window_t* props = gui_create_window("Properties", 300, 200, 400, 350);
    if (!props) return;
    
    gui_create_label(props, "File Properties", 20, 20);
    gui_create_label(props, "", 20, 40);
    gui_create_label(props, "Name: readme.txt", 20, 60);
    gui_create_label(props, "Type: Text File", 20, 80);
    gui_create_label(props, "Size: 1,024 bytes", 20, 100);
    gui_create_label(props, "Extension: .txt", 20, 120);
    gui_create_label(props, "Hidden: No", 20, 140);
    gui_create_label(props, "Read-only: No", 20, 160);
    gui_create_label(props, "", 20, 180);
    gui_create_label(props, "Location: /home/user/readme.txt", 20, 200);
    gui_create_label(props, "Created: Nov 16, 2025", 20, 220);
    gui_create_label(props, "Modified: Nov 16, 2025", 20, 240);
    gui_create_label(props, "Accessed: Nov 16, 2025", 20, 260);
    
    gui_create_button(props, "OK", 160, 290, 80, 30);
    
    gui_show_window(props);
    gui_focus_window(props);
}

int file_explorer_copy_selected(void) {
    // In real implementation, would copy to clipboard
    return 0;
}

int file_explorer_paste(void) {
    // In real implementation, would paste from clipboard
    return 0;
}

int file_explorer_new_tab(const char* path) {
    if (explorer_state.tab_count >= EXPLORER_MAX_TABS) {
        return -1;
    }
    
    explorer_tab_t* new_tab = &tabs_storage[explorer_state.tab_count];
    
    if (path) {
        fe_strcpy(new_tab->path, path, EXPLORER_MAX_PATH);
    } else {
        fe_strcpy(new_tab->path, "/home/user", EXPLORER_MAX_PATH);
    }
    
    new_tab->file_count = 0;
    new_tab->selected_index = -1;
    new_tab->next = NULL;
    
    populate_directory_listing(new_tab);
    
    // Add to tab list
    if (!explorer_state.tabs) {
        explorer_state.tabs = new_tab;
    } else {
        explorer_tab_t* last = explorer_state.tabs;
        while (last->next) {
            last = last->next;
        }
        last->next = new_tab;
    }
    
    explorer_state.active_tab = new_tab;
    explorer_state.tab_count++;
    
    return 0;
}

void file_explorer_close_tab(void) {
    if (!explorer_state.active_tab || explorer_state.tab_count == 0) {
        return;
    }
    
    // Find and remove active tab
    if (explorer_state.tabs == explorer_state.active_tab) {
        explorer_state.tabs = explorer_state.active_tab->next;
    } else {
        explorer_tab_t* prev = explorer_state.tabs;
        while (prev && prev->next != explorer_state.active_tab) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = explorer_state.active_tab->next;
        }
    }
    
    explorer_state.tab_count--;
    
    // Switch to another tab if available
    if (explorer_state.tabs) {
        explorer_state.active_tab = explorer_state.tabs;
    } else {
        explorer_state.active_tab = NULL;
    }
}

void file_explorer_next_tab(void) {
    if (!explorer_state.active_tab || !explorer_state.active_tab->next) {
        return;
    }
    
    explorer_state.active_tab = explorer_state.active_tab->next;
}

void file_explorer_prev_tab(void) {
    if (!explorer_state.active_tab || explorer_state.active_tab == explorer_state.tabs) {
        return;
    }
    
    explorer_tab_t* prev = explorer_state.tabs;
    while (prev->next != explorer_state.active_tab) {
        prev = prev->next;
    }
    
    explorer_state.active_tab = prev;
}

void file_explorer_update_display(void) {
    // Display will be updated by the GUI system
}

int file_explorer_extensions_visible(void) {
    return explorer_state.show_extensions;
}

int file_explorer_hidden_visible(void) {
    return explorer_state.show_hidden;
}
