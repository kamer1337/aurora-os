/**
 * Aurora OS - Goals Manager Application Implementation
 * 
 * Manages and tracks long-term development goals from TODO.md and ROADMAP.md
 */

#include "goals_manager.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include <stddef.h>

// String utilities
static size_t gm_strlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

static void gm_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src && src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Goals manager state
static goals_manager_state_t gm_state;
static uint8_t gm_initialized = 0;

// Initialize with long-term goals from TODO.md and ROADMAP.md
void goals_manager_init(void) {
    if (gm_initialized) return;
    
    gm_state.window = NULL;
    gm_state.goal_count = 0;
    gm_state.scroll_offset = 0;
    gm_state.show_completed = 1;  // Show completed by default
    
    // Initialize all goals as invisible
    for (uint32_t i = 0; i < MAX_GOALS; i++) {
        gm_state.goals[i].visible = 0;
    }
    
    // Load long-term goals from TODO.md - Phase 2
    goals_manager_add_goal("Basic memory management (heap allocator)", "Phase 2", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Process management with scheduler", "Phase 2", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Interrupt handling and system calls", "Phase 2", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Multi-core support", "Phase 2", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Advanced memory management (demand paging)", "Phase 2", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Network stack", "Phase 2", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("USB support", "Phase 2", GOAL_STATUS_COMPLETED);
    
    // Phase 3: File Systems
    goals_manager_add_goal("VFS implementation", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Ramdisk support with read/write", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Journaling layer", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("File operations (create, read, write, delete)", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Directory operations", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Ext2/3/4 support", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("FAT32 support", "Phase 3", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Custom Aurora FS", "Phase 3", GOAL_STATUS_COMPLETED);
    
    // Phase 4: User Interface (all completed)
    goals_manager_add_goal("Framebuffer driver", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("GUI framework (basic)", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Window management", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Widget system (buttons, labels, panels)", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("3D depth effects", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("4D animation system", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("5D interactive effects", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Desktop environment with wallpaper", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Start menu / Application launcher", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Login/guest screen with authentication", "Phase 4", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("User session management", "Phase 4", GOAL_STATUS_COMPLETED);
    
    // Phase 5: Testing & Debugging (all completed)
    goals_manager_add_goal("Comprehensive test framework", "Phase 5", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("End-to-end system testing", "Phase 5", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Memory leak detection", "Phase 5", GOAL_STATUS_COMPLETED);
    goals_manager_add_goal("Performance benchmarking", "Phase 5", GOAL_STATUS_COMPLETED);
    
    // Long-term goals from ROADMAP.md (Q3-Q4 2026)
    goals_manager_add_goal("SSE/AVX instruction support", "Advanced", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Scientific computing library", "Advanced", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Complete JIT code generation backend", "Advanced", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Ext2/Ext3/Ext4 file system driver", "File System", GOAL_STATUS_PENDING);
    goals_manager_add_goal("NTFS read support", "File System", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Network file system (NFS/CIFS) client", "File System", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Multiple workspace/virtual desktop support", "Desktop", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Window snapping and tiling features", "Desktop", GOAL_STATUS_PENDING);
    goals_manager_add_goal("System settings application", "Desktop", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Web browser (WebKit or Chromium)", "Desktop", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Application store/marketplace", "Ecosystem", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Third-party application SDK", "Ecosystem", GOAL_STATUS_PENDING);
    goals_manager_add_goal("64-bit mode support", "Kernel", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Live kernel patching", "Kernel", GOAL_STATUS_PENDING);
    goals_manager_add_goal("Hardware-accelerated rendering (GPU)", "Graphics", GOAL_STATUS_PENDING);
    goals_manager_add_goal("4K/8K display support", "Graphics", GOAL_STATUS_PENDING);
    goals_manager_add_goal("OpenGL/Vulkan graphics API", "Graphics", GOAL_STATUS_PENDING);
    
    gm_initialized = 1;
}

int goals_manager_add_goal(const char* name, const char* category, goal_status_t status) {
    if (gm_state.goal_count >= MAX_GOALS) return -1;
    if (!name || !category) return -1;
    
    goal_t* goal = &gm_state.goals[gm_state.goal_count];
    gm_strcpy(goal->name, name, GOAL_NAME_MAX);
    gm_strcpy(goal->category, category, GOAL_CATEGORY_MAX);
    goal->status = status;
    goal->visible = 1;
    
    gm_state.goal_count++;
    return 0;
}

void goals_manager_toggle_goal(uint32_t index) {
    if (index >= gm_state.goal_count) return;
    
    goal_t* goal = &gm_state.goals[index];
    goal->status = (goal->status == GOAL_STATUS_COMPLETED) ? 
                   GOAL_STATUS_PENDING : GOAL_STATUS_COMPLETED;
    
    goals_manager_update_display();
}

goal_t* goals_manager_get_goal(uint32_t index) {
    if (index >= gm_state.goal_count) return NULL;
    return &gm_state.goals[index];
}

void goals_manager_toggle_filter(void) {
    gm_state.show_completed = !gm_state.show_completed;
    goals_manager_update_display();
}

void goals_manager_scroll(int32_t delta) {
    int32_t new_offset = (int32_t)gm_state.scroll_offset + delta;
    if (new_offset < 0) new_offset = 0;
    if (new_offset > (int32_t)gm_state.goal_count - 10) {
        new_offset = (int32_t)gm_state.goal_count - 10;
    }
    if (new_offset < 0) new_offset = 0;
    
    gm_state.scroll_offset = (uint32_t)new_offset;
    goals_manager_update_display();
}

uint32_t goals_manager_get_count(void) {
    return gm_state.goal_count;
}

uint32_t goals_manager_get_completed_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < gm_state.goal_count; i++) {
        if (gm_state.goals[i].status == GOAL_STATUS_COMPLETED) {
            count++;
        }
    }
    return count;
}

window_t* goals_manager_create(void) {
    if (!gm_initialized) {
        goals_manager_init();
    }
    
    if (gm_state.window) {
        return gm_state.window;  // Already created
    }
    
    // Create window
    gm_state.window = gui_create_window("Long-Term Goals Manager", 100, 50, 720, 500);
    if (!gm_state.window) return NULL;
    
    goals_manager_update_display();
    
    return gm_state.window;
}

void goals_manager_update_display(void) {
    if (!gm_state.window) return;
    
    window_t* win = gm_state.window;
    
    // Create header label showing progress
    char header[128];
    uint32_t completed = goals_manager_get_completed_count();
    uint32_t total = gm_state.goal_count;
    
    // Simple number to string conversion
    char completed_str[16];
    char total_str[16];
    int pos = 0;
    uint32_t temp = completed;
    if (temp == 0) {
        completed_str[pos++] = '0';
    } else {
        char rev[16];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = rev_pos - 1; i >= 0; i--) {
            completed_str[pos++] = rev[i];
        }
    }
    completed_str[pos] = '\0';
    
    pos = 0;
    temp = total;
    if (temp == 0) {
        total_str[pos++] = '0';
    } else {
        char rev[16];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = rev_pos - 1; i >= 0; i--) {
            total_str[pos++] = rev[i];
        }
    }
    total_str[pos] = '\0';
    
    // Build header string
    gm_strcpy(header, "Progress: ", 128);
    size_t hlen = gm_strlen(header);
    gm_strcpy(header + hlen, completed_str, 128 - hlen);
    hlen = gm_strlen(header);
    gm_strcpy(header + hlen, " / ", 128 - hlen);
    hlen = gm_strlen(header);
    gm_strcpy(header + hlen, total_str, 128 - hlen);
    hlen = gm_strlen(header);
    gm_strcpy(header + hlen, " goals completed", 128 - hlen);
    
    // Create main header label
    gui_create_label(win, header, 10, 10);
    
    // Create info labels
    gui_create_label(win, "Long-Term Goals Manager", 10, 35);
    gui_create_label(win, "Click on goals to toggle completion status", 10, 55);
    
    // Display goals as labels
    uint32_t y_offset = 80;
    uint32_t visible_count = 0;
    const uint32_t max_visible = 12;
    
    for (uint32_t i = gm_state.scroll_offset; 
         i < gm_state.goal_count && visible_count < max_visible; 
         i++) {
        goal_t* goal = &gm_state.goals[i];
        
        // Apply filter
        if (!gm_state.show_completed && goal->status == GOAL_STATUS_COMPLETED) {
            continue;
        }
        
        // Create goal display string
        char goal_display[256];
        
        // Add status indicator
        if (goal->status == GOAL_STATUS_COMPLETED) {
            gm_strcpy(goal_display, "[X] ", 256);
        } else {
            gm_strcpy(goal_display, "[ ] ", 256);
        }
        
        // Add category tag
        size_t dlen = gm_strlen(goal_display);
        gm_strcpy(goal_display + dlen, "[", 256 - dlen);
        dlen = gm_strlen(goal_display);
        gm_strcpy(goal_display + dlen, goal->category, 256 - dlen);
        dlen = gm_strlen(goal_display);
        gm_strcpy(goal_display + dlen, "] ", 256 - dlen);
        
        // Add goal name
        dlen = gm_strlen(goal_display);
        gm_strcpy(goal_display + dlen, goal->name, 256 - dlen);
        
        // Create label for this goal
        gui_create_label(win, goal_display, 15, y_offset);
        
        y_offset += 25;
        visible_count++;
    }
    
    // Add instructions at bottom
    gui_create_label(win, "Use Up/Down arrows to scroll | F to toggle filter", 10, 450);
    
    const char* filter_status = gm_state.show_completed ? 
        "Filter: All Goals" : "Filter: Pending Only";
    gui_create_label(win, filter_status, 10, 470);
}

void goals_manager_destroy(window_t* window) {
    if (window == gm_state.window) {
        gm_state.window = NULL;
    }
    gui_close_window(window);
}
