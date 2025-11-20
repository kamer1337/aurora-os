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
        g_workspaces[i].os_type = WORKSPACE_OS_AURORA;  // Default to Aurora OS
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
    int switcher_h = 250;
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
    int btn_h = 100;
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
        framebuffer_draw_string(btn_x + 10, btn_y + 20, g_workspaces[i].name, title_color, (color_t){0, 0, 0, 0});
        
        // OS type indicator
        const char* os_label = (g_workspaces[i].os_type == WORKSPACE_OS_LINUX) ? "Linux VM" : "Aurora OS";
        color_t os_color = (g_workspaces[i].os_type == WORKSPACE_OS_LINUX) ? 
            (color_t){255, 200, 100, 255} : (color_t){100, 200, 255, 255};
        framebuffer_draw_string(btn_x + 10, btn_y + 40, os_label, os_color, (color_t){0, 0, 0, 0});
        
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
        framebuffer_draw_string(btn_x + 10, btn_y + 70, count_text, count_color, (color_t){0, 0, 0, 0});
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

int vdesktop_set_os_type(uint8_t workspace_id, workspace_os_type_t os_type) {
    if (!g_initialized) {
        vdesktop_init();
    }
    
    if (workspace_id >= MAX_WORKSPACES) {
        return -1;
    }
    
    g_workspaces[workspace_id].os_type = os_type;
    
    // Update workspace name to reflect OS type
    if (os_type == WORKSPACE_OS_LINUX) {
        // Update name to indicate Linux workspace
        const char* linux_prefix = "Linux ";
        for (int i = 0; i < 6; i++) {
            g_workspaces[workspace_id].name[i] = linux_prefix[i];
        }
        g_workspaces[workspace_id].name[6] = '1' + workspace_id;
        g_workspaces[workspace_id].name[7] = '\0';
    } else {
        // Reset to default Aurora OS name
        const char* aurora_prefix = "Workspace ";
        for (int i = 0; i < 10; i++) {
            g_workspaces[workspace_id].name[i] = aurora_prefix[i];
        }
        g_workspaces[workspace_id].name[10] = '1' + workspace_id;
        g_workspaces[workspace_id].name[11] = '\0';
    }
    
    return 0;
}

int vdesktop_get_os_type(uint8_t workspace_id) {
    if (!g_initialized) {
        vdesktop_init();
    }
    
    if (workspace_id >= MAX_WORKSPACES) {
        return -1;
    }
    
    return (int)g_workspaces[workspace_id].os_type;
}

void vdesktop_show_os_selector(uint8_t workspace_id) {
    if (!g_initialized) {
        vdesktop_init();
    }
    
    if (workspace_id >= MAX_WORKSPACES) {
        return;
    }
    
    // Draw OS selector overlay
    int screen_w = framebuffer_get_info()->width;
    int screen_h = framebuffer_get_info()->height;
    
    int selector_w = 500;
    int selector_h = 300;
    int selector_x = (screen_w - selector_w) / 2;
    int selector_y = (screen_h - selector_h) / 2;
    
    // Semi-transparent background
    color_t bg = {20, 20, 30, 230};
    framebuffer_draw_rect(selector_x, selector_y, selector_w, selector_h, bg);
    
    // Draw border
    color_t border = {100, 150, 255, 255};
    framebuffer_draw_rect_outline(selector_x, selector_y, selector_w, selector_h, border);
    
    // Title
    color_t title_color = {255, 255, 255, 255};
    framebuffer_draw_string(selector_x + 20, selector_y + 20, "Select Workspace OS Type", title_color, (color_t){0, 0, 0, 0});
    
    // Workspace info
    char ws_info[64];
    ws_info[0] = 'W'; ws_info[1] = 'o'; ws_info[2] = 'r'; ws_info[3] = 'k'; 
    ws_info[4] = 's'; ws_info[5] = 'p'; ws_info[6] = 'a'; ws_info[7] = 'c'; 
    ws_info[8] = 'e'; ws_info[9] = ' ';
    ws_info[10] = '1' + workspace_id;
    ws_info[11] = '\0';
    
    color_t info_color = {200, 200, 200, 255};
    framebuffer_draw_string(selector_x + 20, selector_y + 50, ws_info, info_color, (color_t){0, 0, 0, 0});
    
    // Aurora OS option button
    int btn_w = 200;
    int btn_h = 60;
    int btn_y = selector_y + 100;
    int aurora_btn_x = selector_x + 50;
    
    color_t aurora_btn_color = (g_workspaces[workspace_id].os_type == WORKSPACE_OS_AURORA) ?
        (color_t){80, 150, 220, 255} : (color_t){50, 50, 70, 255};
    framebuffer_draw_rect(aurora_btn_x, btn_y, btn_w, btn_h, aurora_btn_color);
    framebuffer_draw_rect_outline(aurora_btn_x, btn_y, btn_w, btn_h, border);
    framebuffer_draw_string(aurora_btn_x + 40, btn_y + 20, "Aurora OS", title_color, (color_t){0, 0, 0, 0});
    
    // Linux VM option button
    int linux_btn_x = selector_x + 250;
    
    color_t linux_btn_color = (g_workspaces[workspace_id].os_type == WORKSPACE_OS_LINUX) ?
        (color_t){220, 150, 80, 255} : (color_t){50, 50, 70, 255};
    framebuffer_draw_rect(linux_btn_x, btn_y, btn_w, btn_h, linux_btn_color);
    framebuffer_draw_rect_outline(linux_btn_x, btn_y, btn_w, btn_h, border);
    framebuffer_draw_string(linux_btn_x + 40, btn_y + 20, "Linux VM", title_color, (color_t){0, 0, 0, 0});
    
    // Description text
    const char* desc = "Choose the operating system type for this workspace";
    framebuffer_draw_string(selector_x + 30, selector_y + 200, desc, info_color, (color_t){0, 0, 0, 0});
    
    // Instructions
    const char* instr = "Press 1 for Aurora OS, 2 for Linux VM, ESC to cancel";
    framebuffer_draw_string(selector_x + 20, selector_y + 250, instr, (color_t){150, 150, 150, 255}, (color_t){0, 0, 0, 0});
}
