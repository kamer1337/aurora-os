/**
 * Aurora OS - Virtual Desktop Manager
 * 
 * Multiple workspace/virtual desktop support
 */

#include "virtual_desktop.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"

// Global workspace state
static workspace_t g_workspaces[MAX_WORKSPACES];
static uint8_t g_workspace_count = 0;
static uint8_t g_current_workspace = 0;
static uint8_t g_initialized = 0;

int vdesktop_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    // Initialize workspaces
    for (int i = 0; i < MAX_WORKSPACES; i++) {
        g_workspaces[i].id = i;
        g_workspaces[i].active = 0;
        g_workspaces[i].window_count = 0;
        
        // Default names
        g_workspaces[i].name[0] = 'W';
        g_workspaces[i].name[1] = 'o';
        g_workspaces[i].name[2] = 'r';
        g_workspaces[i].name[3] = 'k';
        g_workspaces[i].name[4] = 's';
        g_workspaces[i].name[5] = 'p';
        g_workspaces[i].name[6] = 'a';
        g_workspaces[i].name[7] = 'c';
        g_workspaces[i].name[8] = 'e';
        g_workspaces[i].name[9] = ' ';
        g_workspaces[i].name[10] = '1' + i;
        g_workspaces[i].name[11] = '\0';
        
        for (int j = 0; j < MAX_WINDOWS_PER_WORKSPACE; j++) {
            g_workspaces[i].windows[j] = NULL;
        }
    }
    
    // Create default workspace
    g_workspace_count = 4;
    g_workspaces[0].active = 1;
    g_current_workspace = 0;
    
    g_initialized = 1;
    return 0;
}

int vdesktop_create_workspace(const char* name) {
    if (g_workspace_count >= MAX_WORKSPACES) {
        return -1;
    }
    
    uint8_t id = g_workspace_count;
    
    // Set name
    if (name) {
        for (int i = 0; i < 31 && name[i]; i++) {
            g_workspaces[id].name[i] = name[i];
            g_workspaces[id].name[i + 1] = '\0';
        }
    }
    
    g_workspace_count++;
    return id;
}

int vdesktop_switch_to(uint8_t workspace_id) {
    if (!g_initialized) {
        vdesktop_init();
    }
    
    if (workspace_id >= MAX_WORKSPACES) {
        return -1;
    }
    
    if (workspace_id == g_current_workspace) {
        return 0; // Already on this workspace
    }
    
    // Deactivate current workspace
    g_workspaces[g_current_workspace].active = 0;
    
    // Hide windows from current workspace
    workspace_t* current = &g_workspaces[g_current_workspace];
    for (int i = 0; i < current->window_count; i++) {
        if (current->windows[i]) {
            current->windows[i]->visible = 0;
        }
    }
    
    // Activate new workspace
    g_current_workspace = workspace_id;
    g_workspaces[workspace_id].active = 1;
    
    // Show windows in new workspace
    workspace_t* new_ws = &g_workspaces[workspace_id];
    for (int i = 0; i < new_ws->window_count; i++) {
        if (new_ws->windows[i]) {
            new_ws->windows[i]->visible = 1;
        }
    }
    
    // Request screen refresh
    gui_update();
    
    return 0;
}

uint8_t vdesktop_get_current(void) {
    if (!g_initialized) {
        vdesktop_init();
    }
    return g_current_workspace;
}

int vdesktop_move_window(window_t* window, uint8_t workspace_id) {
    if (!g_initialized || !window || workspace_id >= MAX_WORKSPACES) {
        return -1;
    }
    
    // Remove from current workspace
    for (int ws = 0; ws < MAX_WORKSPACES; ws++) {
        workspace_t* workspace = &g_workspaces[ws];
        for (int i = 0; i < workspace->window_count; i++) {
            if (workspace->windows[i] == window) {
                // Shift remaining windows
                for (int j = i; j < workspace->window_count - 1; j++) {
                    workspace->windows[j] = workspace->windows[j + 1];
                }
                workspace->windows[workspace->window_count - 1] = NULL;
                workspace->window_count--;
                break;
            }
        }
    }
    
    // Add to target workspace
    workspace_t* target = &g_workspaces[workspace_id];
    if (target->window_count < MAX_WINDOWS_PER_WORKSPACE) {
        target->windows[target->window_count] = window;
        target->window_count++;
        
        // Hide window if not in current workspace
        if (workspace_id != g_current_workspace) {
            window->visible = 0;
        }
        
        return 0;
    }
    
    return -1;
}

workspace_t* vdesktop_get_workspace(uint8_t workspace_id) {
    if (!g_initialized || workspace_id >= MAX_WORKSPACES) {
        return NULL;
    }
    return &g_workspaces[workspace_id];
}

void vdesktop_show_switcher(void) {
    if (!g_initialized) {
        vdesktop_init();
    }
    
    // Draw workspace switcher overlay
    int screen_w = framebuffer_get_info()->width;
    int screen_h = framebuffer_get_info()->height;
    
    int switcher_w = 600;
    int switcher_h = 200;
    int switcher_x = (screen_w - switcher_w) / 2;
    int switcher_y = (screen_h - switcher_h) / 2;
    
    // Semi-transparent background
    color_t bg = {20, 20, 30, 220};
    framebuffer_draw_rect(switcher_x, switcher_y, switcher_w, switcher_h, bg);
    
    // Draw border
    color_t border = {100, 150, 255, 255};
    framebuffer_draw_rect_outline(switcher_x, switcher_y, switcher_w, switcher_h, border);
    
    // Title
    color_t title_color = {255, 255, 255, 255};
    framebuffer_draw_string(switcher_x + 20, switcher_y + 20, "Switch Workspace", title_color, (color_t){0, 0, 0, 0});
    
    // Draw workspace buttons
    int btn_w = 120;
    int btn_h = 80;
    int spacing = 20;
    int start_x = switcher_x + 40;
    int btn_y = switcher_y + 70;
    
    for (int i = 0; i < MAX_WORKSPACES; i++) {
        int btn_x = start_x + (i * (btn_w + spacing));
        
        // Highlight current workspace
        color_t btn_color;
        if (i == g_current_workspace) {
            btn_color = (color_t){80, 120, 200, 255};
        } else {
            btn_color = (color_t){50, 50, 60, 255};
        }
        
        framebuffer_draw_rect(btn_x, btn_y, btn_w, btn_h, btn_color);
        framebuffer_draw_rect_outline(btn_x, btn_y, btn_w, btn_h, border);
        
        // Workspace name
        framebuffer_draw_string(btn_x + 10, btn_y + 30, g_workspaces[i].name, title_color, (color_t){0, 0, 0, 0});
        
        // Window count
        char count_text[16];
        count_text[0] = '0' + (g_workspaces[i].window_count / 10);
        count_text[1] = '0' + (g_workspaces[i].window_count % 10);
        count_text[2] = ' ';
        count_text[3] = 'w';
        count_text[4] = 'i';
        count_text[5] = 'n';
        count_text[6] = 's';
        count_text[7] = '\0';
        
        color_t count_color = {180, 180, 180, 255};
        framebuffer_draw_string(btn_x + 10, btn_y + 50, count_text, count_color, (color_t){0, 0, 0, 0});
    }
}

int vdesktop_handle_shortcut(uint32_t key) {
    if (!g_initialized) {
        vdesktop_init();
    }
    
    // Handle Ctrl+Alt+[1-4] for workspace switching
    // Key codes would need to be defined properly
    // This is a simplified version
    switch (key) {
        case 0x02: // '1'
            return vdesktop_switch_to(0);
        case 0x03: // '2'
            return vdesktop_switch_to(1);
        case 0x04: // '3'
            return vdesktop_switch_to(2);
        case 0x05: // '4'
            return vdesktop_switch_to(3);
        default:
            return 0;
    }
}
