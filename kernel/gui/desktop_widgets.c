/**
 * Aurora OS - Desktop Widgets and Gadgets Implementation
 * 
 * Desktop widget system for placing interactive widgets on the desktop
 */

#include "desktop_widgets.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../../filesystem/vfs/vfs.h"

// Widget storage
static desktop_widget_t* widgets[MAX_DESKTOP_WIDGETS];
static uint32_t widget_count = 0;
static uint8_t initialized = 0;

// Clock widget data
typedef struct {
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
} clock_widget_data_t;

// System monitor widget data
typedef struct {
    uint32_t cpu_usage;
    uint32_t memory_usage;
    uint32_t disk_usage;
} system_monitor_data_t;

// Forward declarations
static void clock_widget_update(desktop_widget_t* widget);
static void clock_widget_render(desktop_widget_t* widget);
static void system_monitor_update(desktop_widget_t* widget);
static void system_monitor_render(desktop_widget_t* widget);
static void calendar_widget_render(desktop_widget_t* widget);

int desktop_widgets_init(void) {
    if (initialized) {
        return 0;
    }
    
    // Initialize widget array
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS; i++) {
        widgets[i] = NULL;
    }
    widget_count = 0;
    
    // Try to load saved configuration
    desktop_widgets_load_config();
    
    initialized = 1;
    return 0;
}

void desktop_widgets_shutdown(void) {
    if (!initialized) {
        return;
    }
    
    // Save configuration
    desktop_widgets_save_config();
    
    // Free all widgets
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS; i++) {
        if (widgets[i]) {
            if (widgets[i]->data) {
                kfree(widgets[i]->data);
            }
            kfree(widgets[i]);
            widgets[i] = NULL;
        }
    }
    
    widget_count = 0;
    initialized = 0;
}

desktop_widget_t* desktop_widget_create(desktop_widget_type_t type, int32_t x, int32_t y, 
                                       uint32_t width, uint32_t height) {
    if (widget_count >= MAX_DESKTOP_WIDGETS) {
        return NULL;
    }
    
    // Find free slot
    uint32_t slot = 0;
    for (slot = 0; slot < MAX_DESKTOP_WIDGETS; slot++) {
        if (widgets[slot] == NULL) {
            break;
        }
    }
    
    if (slot >= MAX_DESKTOP_WIDGETS) {
        return NULL;
    }
    
    // Allocate widget
    desktop_widget_t* widget = (desktop_widget_t*)kmalloc(sizeof(desktop_widget_t));
    if (!widget) {
        return NULL;
    }
    
    // Initialize common properties
    widget->type = type;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = width;
    widget->bounds.height = height;
    widget->visible = 1;
    widget->enabled = 1;
    widget->draggable = 1;
    widget->bg_color = (color_t){40, 40, 50, 220};
    widget->data = NULL;
    widget->update = NULL;
    widget->render = NULL;
    widget->on_click = NULL;
    
    // Configure based on type
    switch (type) {
        case DESKTOP_WIDGET_CLOCK:
            for (int i = 0; i < 5; i++) widget->title[i] = "Clock"[i];
            widget->title[5] = '\0';
            widget->data = kmalloc(sizeof(clock_widget_data_t));
            if (widget->data) {
                clock_widget_data_t* data = (clock_widget_data_t*)widget->data;
                data->hours = 12;
                data->minutes = 0;
                data->seconds = 0;
            }
            widget->update = clock_widget_update;
            widget->render = clock_widget_render;
            break;
            
        case DESKTOP_WIDGET_SYSTEM_MONITOR:
            for (int i = 0; i < 14; i++) widget->title[i] = "System Monitor"[i];
            widget->title[14] = '\0';
            widget->data = kmalloc(sizeof(system_monitor_data_t));
            if (widget->data) {
                system_monitor_data_t* data = (system_monitor_data_t*)widget->data;
                data->cpu_usage = 0;
                data->memory_usage = 0;
                data->disk_usage = 0;
            }
            widget->update = system_monitor_update;
            widget->render = system_monitor_render;
            break;
            
        case DESKTOP_WIDGET_CALENDAR:
            for (int i = 0; i < 8; i++) widget->title[i] = "Calendar"[i];
            widget->title[8] = '\0';
            widget->render = calendar_widget_render;
            break;
            
        default:
            for (int i = 0; i < 6; i++) widget->title[i] = "Widget"[i];
            widget->title[6] = '\0';
            break;
    }
    
    widgets[slot] = widget;
    widget_count++;
    
    return widget;
}

void desktop_widget_destroy(desktop_widget_t* widget) {
    if (!widget) {
        return;
    }
    
    // Find and remove from array
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS; i++) {
        if (widgets[i] == widget) {
            if (widget->data) {
                kfree(widget->data);
            }
            kfree(widget);
            widgets[i] = NULL;
            widget_count--;
            return;
        }
    }
}

void desktop_widgets_update(void) {
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS; i++) {
        if (widgets[i] && widgets[i]->enabled && widgets[i]->update) {
            widgets[i]->update(widgets[i]);
        }
    }
}

void desktop_widgets_render(void) {
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS; i++) {
        desktop_widget_t* widget = widgets[i];
        if (widget && widget->visible) {
            // Draw widget background with rounded corners
            framebuffer_draw_rect(widget->bounds.x, widget->bounds.y, 
                                widget->bounds.width, widget->bounds.height, 
                                widget->bg_color);
            
            // Draw border
            color_t border_color = {100, 100, 120, 255};
            framebuffer_draw_rect_outline(widget->bounds.x, widget->bounds.y,
                                        widget->bounds.width, widget->bounds.height,
                                        border_color);
            
            // Draw title bar
            color_t title_bg = {50, 50, 70, 255};
            framebuffer_draw_rect(widget->bounds.x, widget->bounds.y, 
                                widget->bounds.width, 25, title_bg);
            
            // Draw title text
            color_t title_color = {200, 200, 200, 255};
            framebuffer_draw_string(widget->bounds.x + 5, widget->bounds.y + 5,
                                  widget->title, title_color, (color_t){0, 0, 0, 0});
            
            // Draw widget content
            if (widget->render) {
                widget->render(widget);
            }
        }
    }
}

int desktop_widgets_handle_click(int32_t x, int32_t y) {
    // Check clicks in reverse order (top to bottom)
    for (int i = MAX_DESKTOP_WIDGETS - 1; i >= 0; i--) {
        desktop_widget_t* widget = widgets[i];
        if (widget && widget->visible && widget->enabled) {
            if (x >= widget->bounds.x && x < widget->bounds.x + (int32_t)widget->bounds.width &&
                y >= widget->bounds.y && y < widget->bounds.y + (int32_t)widget->bounds.height) {
                if (widget->on_click) {
                    widget->on_click(widget, x - widget->bounds.x, y - widget->bounds.y);
                }
                return 1;
            }
        }
    }
    return 0;
}

void desktop_widget_show(desktop_widget_t* widget) {
    if (widget) {
        widget->visible = 1;
    }
}

void desktop_widget_hide(desktop_widget_t* widget) {
    if (widget) {
        widget->visible = 0;
    }
}

int desktop_widgets_get_all(desktop_widget_t** out_widgets, uint32_t max_widgets) {
    if (!out_widgets || max_widgets == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS && count < max_widgets; i++) {
        if (widgets[i]) {
            out_widgets[count++] = widgets[i];
        }
    }
    
    return count;
}

int desktop_widgets_save_config(void) {
    // Save widget configuration to file
    int fd = vfs_create("/home/.widgets_config");
    if (fd < 0) {
        fd = vfs_open("/home/.widgets_config", O_WRONLY | O_TRUNC);
        if (fd < 0) {
            return -1;
        }
    }
    
    // Write widget count
    vfs_write(fd, &widget_count, sizeof(widget_count));
    
    // Write each widget configuration (excluding data pointer)
    for (uint32_t i = 0; i < MAX_DESKTOP_WIDGETS; i++) {
        if (widgets[i]) {
            // Write basic widget info
            vfs_write(fd, &widgets[i]->type, sizeof(desktop_widget_type_t));
            vfs_write(fd, &widgets[i]->bounds, sizeof(rect_t));
            vfs_write(fd, &widgets[i]->visible, sizeof(uint8_t));
        }
    }
    
    vfs_close(fd);
    return 0;
}

int desktop_widgets_load_config(void) {
    int fd = vfs_open("/home/.widgets_config", O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Read widget count
    uint32_t saved_count;
    if (vfs_read(fd, &saved_count, sizeof(saved_count)) != sizeof(saved_count)) {
        vfs_close(fd);
        return -1;
    }
    
    // Read widget configurations
    for (uint32_t i = 0; i < saved_count && i < MAX_DESKTOP_WIDGETS; i++) {
        desktop_widget_type_t type;
        rect_t bounds;
        uint8_t visible;
        
        if (vfs_read(fd, &type, sizeof(type)) == sizeof(type) &&
            vfs_read(fd, &bounds, sizeof(bounds)) == sizeof(bounds) &&
            vfs_read(fd, &visible, sizeof(visible)) == sizeof(visible)) {
            
            // Recreate widget
            desktop_widget_t* widget = desktop_widget_create(type, bounds.x, bounds.y,
                                                            bounds.width, bounds.height);
            if (widget) {
                widget->visible = visible;
            }
        }
    }
    
    vfs_close(fd);
    return 0;
}

// Widget implementations

static void clock_widget_update(desktop_widget_t* widget) {
    if (!widget || !widget->data) {
        return;
    }
    
    clock_widget_data_t* data = (clock_widget_data_t*)widget->data;
    // Update time (simplified - would use system timer)
    data->seconds++;
    if (data->seconds >= 60) {
        data->seconds = 0;
        data->minutes++;
        if (data->minutes >= 60) {
            data->minutes = 0;
            data->hours++;
            if (data->hours >= 24) {
                data->hours = 0;
            }
        }
    }
}

static void clock_widget_render(desktop_widget_t* widget) {
    if (!widget || !widget->data) {
        return;
    }
    
    clock_widget_data_t* data = (clock_widget_data_t*)widget->data;
    
    // Draw time
    int content_x = widget->bounds.x + 10;
    int content_y = widget->bounds.y + 35;
    
    color_t time_color = {150, 200, 255, 255};
    char time_str[16];
    
    // Format time string HH:MM:SS
    time_str[0] = '0' + (data->hours / 10);
    time_str[1] = '0' + (data->hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (data->minutes / 10);
    time_str[4] = '0' + (data->minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (data->seconds / 10);
    time_str[7] = '0' + (data->seconds % 10);
    time_str[8] = '\0';
    
    framebuffer_draw_string(content_x, content_y, time_str, time_color, (color_t){0, 0, 0, 0});
}

static void system_monitor_update(desktop_widget_t* widget) {
    if (!widget || !widget->data) {
        return;
    }
    
    system_monitor_data_t* data = (system_monitor_data_t*)widget->data;
    // Update system stats (simplified)
    data->cpu_usage = (data->cpu_usage + 5) % 100;
    data->memory_usage = 45;
    data->disk_usage = 60;
}

static void system_monitor_render(desktop_widget_t* widget) {
    if (!widget || !widget->data) {
        return;
    }
    
    system_monitor_data_t* data = (system_monitor_data_t*)widget->data;
    
    int content_x = widget->bounds.x + 10;
    int content_y = widget->bounds.y + 35;
    
    color_t label_color = {180, 180, 180, 255};
    color_t value_color = {150, 200, 255, 255};
    
    // Draw CPU usage
    framebuffer_draw_string(content_x, content_y, "CPU:", label_color, (color_t){0, 0, 0, 0});
    char cpu_str[8];
    cpu_str[0] = '0' + (data->cpu_usage / 10);
    cpu_str[1] = '0' + (data->cpu_usage % 10);
    cpu_str[2] = '%';
    cpu_str[3] = '\0';
    framebuffer_draw_string(content_x + 50, content_y, cpu_str, value_color, (color_t){0, 0, 0, 0});
    
    // Draw memory usage
    content_y += 20;
    framebuffer_draw_string(content_x, content_y, "RAM:", label_color, (color_t){0, 0, 0, 0});
    char mem_str[8];
    mem_str[0] = '0' + (data->memory_usage / 10);
    mem_str[1] = '0' + (data->memory_usage % 10);
    mem_str[2] = '%';
    mem_str[3] = '\0';
    framebuffer_draw_string(content_x + 50, content_y, mem_str, value_color, (color_t){0, 0, 0, 0});
    
    // Draw disk usage
    content_y += 20;
    framebuffer_draw_string(content_x, content_y, "Disk:", label_color, (color_t){0, 0, 0, 0});
    char disk_str[8];
    disk_str[0] = '0' + (data->disk_usage / 10);
    disk_str[1] = '0' + (data->disk_usage % 10);
    disk_str[2] = '%';
    disk_str[3] = '\0';
    framebuffer_draw_string(content_x + 50, content_y, disk_str, value_color, (color_t){0, 0, 0, 0});
}

static void calendar_widget_render(desktop_widget_t* widget) {
    if (!widget) {
        return;
    }
    
    int content_x = widget->bounds.x + 10;
    int content_y = widget->bounds.y + 35;
    
    color_t label_color = {180, 180, 180, 255};
    
    // Draw current date (simplified)
    framebuffer_draw_string(content_x, content_y, "December 2025", label_color, (color_t){0, 0, 0, 0});
    content_y += 25;
    framebuffer_draw_string(content_x, content_y, "Sunday, 8th", label_color, (color_t){0, 0, 0, 0});
}
