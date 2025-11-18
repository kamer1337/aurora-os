/**
 * Aurora OS - GUI Rendering Optimization Header
 */

#ifndef GUI_OPTIMIZATION_H
#define GUI_OPTIMIZATION_H

#include <stdint.h>

typedef struct {
    int enabled;
    uint32_t current_fps;
    uint32_t target_fps;
    int vsync_enabled;
    int dirty_region_tracking;
    int batch_rendering;
    int gpu_acceleration;
} gui_optimization_state_t;

// Function prototypes
int gui_optimization_init(void);
void gui_enable_dirty_region_tracking(void);
void gui_enable_batch_rendering(void);
int gui_enable_gpu_acceleration(void);
uint32_t gui_get_current_fps(void);
void gui_set_target_fps(uint32_t fps);

#endif // GUI_OPTIMIZATION_H
