/**
 * Aurora OS - Advanced GUI Demo Implementation
 * 
 * Showcases 3D depth, 4D (animation), and 5D (interactive) effects
 */

#include "gui_demo_advanced.h"
#include "gui.h"
#include "gui_effects.h"
#include "framebuffer.h"

// Animation state
static uint32_t animation_tick = 0;
static window_t* demo_window = NULL;
static window_t* effects_window = NULL;

// Button click handlers
static void on_gradient_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    // Show gradient effect in a new window
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Draw gradient showcase in center
    int32_t gx = fb->width / 2 - 200;
    int32_t gy = fb->height / 2 - 100;
    
    color_t color1 = {255, 100, 100, 255};  // Red
    color_t color2 = {100, 100, 255, 255};  // Blue
    
    gui_draw_gradient(gx, gy, 400, 200, color1, color2);
    
    // Add text overlay
    framebuffer_draw_string(gx + 150, gy + 95, "Gradient Effect", 
                          COLOR_WHITE, color1);
}

static void on_particles_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Emit particles at center of screen
    color_t particle_color = {255, 200, 50, 255};  // Golden
    gui_emit_particles(fb->width / 2, fb->height / 2, 50, particle_color);
}

static void on_glow_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Draw glowing rectangle in center
    int32_t gx = fb->width / 2 - 100;
    int32_t gy = fb->height / 2 - 50;
    
    color_t glow_color = {100, 255, 100, 255};  // Green
    gui_draw_glow(gx, gy, 200, 100, glow_color, 8);
    
    // Draw the actual rectangle
    gui_draw_rounded_rect(gx, gy, 200, 100, 10, glow_color);
    
    framebuffer_draw_string(gx + 60, gy + 45, "Glowing!", 
                          COLOR_WHITE, glow_color);
}

static void on_glass_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Draw glass effect panel
    int32_t gx = fb->width / 2 - 150;
    int32_t gy = fb->height / 2 - 75;
    
    gui_draw_glass_effect(gx, gy, 300, 150, 120);
    
    framebuffer_draw_string(gx + 80, gy + 70, "Glass Effect", 
                          COLOR_BLACK, COLOR_WHITE);
}

static void on_animation_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    // Trigger animation by resetting tick
    animation_tick = 0;
}

void gui_demo_advanced_init(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Create main demo window with 3D effects
    demo_window = gui_create_window("Aurora OS - 3D/4D/5D GUI Demo", 
                                   50, 50, 600, 400);
    if (!demo_window) return;
    
    // Set gradient background (simulated with color)
    demo_window->bg_color = (color_t){240, 248, 255, 255};  // Alice blue
    
    // Add title label
    gui_create_label(demo_window, "Advanced Visual Effects Showcase", 20, 20);
    
    // Add description
    gui_create_label(demo_window, "Click buttons to see different effects:", 20, 45);
    
    // Create effect buttons with 3D styling
    widget_t* gradient_btn = gui_create_button(demo_window, "Gradient (3D)", 
                                              20, 80, 130, 40);
    gui_set_widget_click_handler(gradient_btn, on_gradient_click);
    gradient_btn->bg_color = (color_t){100, 150, 255, 255};  // Blue
    
    widget_t* particles_btn = gui_create_button(demo_window, "Particles (5D)", 
                                               160, 80, 130, 40);
    gui_set_widget_click_handler(particles_btn, on_particles_click);
    particles_btn->bg_color = (color_t){255, 200, 50, 255};  // Gold
    
    widget_t* glow_btn = gui_create_button(demo_window, "Glow Effect", 
                                          300, 80, 130, 40);
    gui_set_widget_click_handler(glow_btn, on_glow_click);
    glow_btn->bg_color = (color_t){100, 255, 100, 255};  // Green
    
    widget_t* glass_btn = gui_create_button(demo_window, "Glass Effect", 
                                           440, 80, 130, 40);
    gui_set_widget_click_handler(glass_btn, on_glass_click);
    glass_btn->bg_color = (color_t){200, 200, 255, 255};  // Light blue
    
    widget_t* anim_btn = gui_create_button(demo_window, "Animation (4D)", 
                                          20, 140, 130, 40);
    gui_set_widget_click_handler(anim_btn, on_animation_click);
    anim_btn->bg_color = (color_t){255, 150, 150, 255};  // Pink
    
    // Add information panel with shadow
    gui_create_label(demo_window, "Features Demonstrated:", 20, 200);
    gui_create_label(demo_window, "- 3D Depth: Shadows, gradients, rounded corners", 20, 225);
    gui_create_label(demo_window, "- 4D Time: Smooth animations with easing", 20, 245);
    gui_create_label(demo_window, "- 5D Interactive: Particles, glow, glass effects", 20, 265);
    gui_create_label(demo_window, "- Advanced rendering with alpha blending", 20, 285);
    
    // Create effects info window
    effects_window = gui_create_window("Visual Effects Information",
                                      fb->width - 350, 50, 300, 300);
    if (effects_window) {
        effects_window->bg_color = (color_t){255, 250, 240, 255};  // Floral white
        
        gui_create_label(effects_window, "3D Depth Effects:", 10, 10);
        gui_create_label(effects_window, "* Window shadows", 20, 30);
        gui_create_label(effects_window, "* Gradient titlebars", 20, 45);
        gui_create_label(effects_window, "* 3D buttons", 20, 60);
        gui_create_label(effects_window, "* Rounded corners", 20, 75);
        
        gui_create_label(effects_window, "4D Animation:", 10, 105);
        gui_create_label(effects_window, "* Smooth easing", 20, 125);
        gui_create_label(effects_window, "* Color transitions", 20, 140);
        gui_create_label(effects_window, "* Value interpolation", 20, 155);
        
        gui_create_label(effects_window, "5D Interactive:", 10, 185);
        gui_create_label(effects_window, "* Particle systems", 20, 205);
        gui_create_label(effects_window, "* Dynamic glow", 20, 220);
        gui_create_label(effects_window, "* Glass/blur effects", 20, 235);
    }
    
    // Show windows
    gui_show_window(demo_window);
    if (effects_window) {
        gui_show_window(effects_window);
    }
    gui_focus_window(demo_window);
}

void gui_demo_advanced_update(void) {
    // Update animations
    animation_tick++;
    
    // Update particle system
    gui_update_particles(1);
    
    // Example: Animate demo window position slightly (breathing effect)
    if (demo_window && animation_tick > 0 && animation_tick < 200) {
        float t = (float)animation_tick / 200.0f;
        float eased = gui_ease(t, EASE_IN_OUT_QUAD);
        
        // Subtle breathing animation (scale would be complex, so we just do color)
        // In a full implementation, we could animate window borders, colors, etc.
        (void)eased;  // Placeholder for now
    }
    
    // Draw particles if any are active
    gui_draw_particles();
    
    // Draw animated elements
    if (animation_tick < 100) {
        // Show an animated circle growing at center of screen
        framebuffer_info_t* fb = framebuffer_get_info();
        if (fb) {
            float t = (float)animation_tick / 100.0f;
            float eased = gui_ease(t, EASE_OUT_CUBIC);
            
            int32_t radius = (int32_t)(eased * 50.0f);
            int32_t cx = fb->width / 2;
            int32_t cy = fb->height / 2;
            
            // Draw expanding circle with glow
            color_t anim_color = {255, 150, 255, (uint8_t)(200 * (1.0f - t))};
            
            for (int32_t dy = -radius; dy <= radius; dy++) {
                for (int32_t dx = -radius; dx <= radius; dx++) {
                    if (dx * dx + dy * dy <= radius * radius) {
                        gui_draw_pixel_alpha(cx + dx, cy + dy, anim_color);
                    }
                }
            }
        }
    }
}
