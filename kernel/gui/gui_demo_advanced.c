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

// Icon sprites for 5D demo
static sprite_t* icon_sprites[5] = {NULL, NULL, NULL, NULL, NULL};
static uint8_t icon_hover[5] = {0, 0, 0, 0, 0};
static uint8_t layered_demo_active = 0;

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

static void on_layered_windows_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Draw layered windows demonstration (book effect)
    int32_t base_x = fb->width / 2 - 200;
    int32_t base_y = fb->height / 2 - 150;
    
    // Draw 5 windows at different depths
    color_t window_colors[5] = {
        {200, 100, 100, 255},  // Red
        {100, 200, 100, 255},  // Green
        {100, 100, 200, 255},  // Blue
        {200, 200, 100, 255},  // Yellow
        {200, 100, 200, 255}   // Magenta
    };
    
    const char* titles[5] = {
        "Back Window",
        "Mid-Back",
        "Middle",
        "Mid-Front",
        "Front Window"
    };
    
    // Draw from back to front
    for (int i = 4; i >= 0; i--) {
        float depth = (float)i / 5.0f;
        gui_draw_window_with_depth(base_x, base_y, 400, 250, depth, 
                                   window_colors[i], titles[i]);
    }
    
    layered_demo_active = 1;
}

static void on_icons_5d_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Create icon sprites if not already created
    if (icon_sprites[0] == NULL) {
        color_t icon_colors[5] = {
            {255, 100, 100, 255},  // Red
            {100, 255, 100, 255},  // Green
            {100, 100, 255, 255},  // Blue
            {255, 200, 50, 255},   // Gold
            {200, 50, 255, 255}    // Purple
        };
        
        for (int i = 0; i < 5; i++) {
            icon_sprites[i] = gui_create_icon(48, icon_colors[i], i);
        }
    }
    
    // Draw icons at different depths
    int32_t start_x = fb->width / 2 - 250;
    int32_t start_y = fb->height / 2 - 24;
    
    icon_depth_level_t depths[5] = {
        ICON_DEPTH_FAR,
        ICON_DEPTH_BACKGROUND,
        ICON_DEPTH_NORMAL,
        ICON_DEPTH_NORMAL,
        ICON_DEPTH_FOREGROUND
    };
    
    for (int i = 0; i < 5; i++) {
        if (icon_sprites[i]) {
            gui_draw_icon_5d(icon_sprites[i], 
                           start_x + i * 100, start_y,
                           depths[i], icon_hover[i]);
        }
    }
}

static void on_horizontal_gradient_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Draw horizontal gradient showcase
    int32_t gx = fb->width / 2 - 200;
    int32_t gy = fb->height / 2 - 100;
    
    color_t color1 = {255, 50, 50, 255};    // Red
    color_t color2 = {50, 50, 255, 255};    // Blue
    
    gui_draw_gradient_horizontal(gx, gy, 400, 80, color1, color2);
    
    // Add label
    framebuffer_draw_string(gx + 120, gy + 35, "Horizontal Gradient", 
                          COLOR_WHITE, color1);
    
    // Draw radial gradient below
    gui_draw_gradient_radial(fb->width / 2, gy + 150, 60,
                            (color_t){255, 255, 100, 255},
                            (color_t){255, 100, 100, 255});
    
    framebuffer_draw_string(fb->width / 2 - 50, gy + 140, "Radial", 
                          COLOR_WHITE, COLOR_BLACK);
}

void gui_demo_advanced_init(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Create main demo window with 3D effects
    demo_window = gui_create_window("Aurora OS - Enhanced 5D Rendering Demo", 
                                   50, 50, 700, 450);
    if (!demo_window) return;
    
    // Set gradient background (simulated with color)
    demo_window->bg_color = (color_t){240, 248, 255, 255};  // Alice blue
    
    // Add title label
    gui_create_label(demo_window, "Enhanced Visual Effects Showcase", 20, 20);
    
    // Add description
    gui_create_label(demo_window, "Click buttons to see enhanced 5D effects:", 20, 45);
    
    // First row of buttons
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
    
    // Second row - NEW enhanced features
    widget_t* layered_btn = gui_create_button(demo_window, "Layered Windows", 
                                             20, 140, 150, 40);
    gui_set_widget_click_handler(layered_btn, on_layered_windows_click);
    layered_btn->bg_color = (color_t){150, 100, 200, 255};  // Purple
    
    widget_t* icons_btn = gui_create_button(demo_window, "5D Icons", 
                                           180, 140, 150, 40);
    gui_set_widget_click_handler(icons_btn, on_icons_5d_click);
    icons_btn->bg_color = (color_t){255, 150, 100, 255};  // Orange
    
    widget_t* h_gradient_btn = gui_create_button(demo_window, "H/R Gradients", 
                                                 340, 140, 150, 40);
    gui_set_widget_click_handler(h_gradient_btn, on_horizontal_gradient_click);
    h_gradient_btn->bg_color = (color_t){100, 200, 200, 255};  // Cyan
    
    widget_t* anim_btn = gui_create_button(demo_window, "Animation (4D)", 
                                          500, 140, 150, 40);
    gui_set_widget_click_handler(anim_btn, on_animation_click);
    anim_btn->bg_color = (color_t){255, 150, 150, 255};  // Pink
    
    // Add information panel with shadow
    gui_create_label(demo_window, "Enhanced Features:", 20, 200);
    gui_create_label(demo_window, "- 3D Depth: Multi-layer shadows, enhanced gradients", 20, 225);
    gui_create_label(demo_window, "- Transparency: Alpha-blended rounded corners", 20, 245);
    gui_create_label(demo_window, "- Layered Windows: Book-style depth rendering", 20, 265);
    gui_create_label(demo_window, "- 5D Icons: Depth-based scaling & hover effects", 20, 285);
    gui_create_label(demo_window, "- Advanced Gradients: Horizontal & radial gradients", 20, 305);
    gui_create_label(demo_window, "- Interactive Effects: Particle emissions on hover", 20, 325);
    
    // Create effects info window
    effects_window = gui_create_window("5D Rendering Features",
                                      fb->width - 350, 50, 300, 350);
    if (effects_window) {
        effects_window->bg_color = (color_t){255, 250, 240, 255};  // Floral white
        
        gui_create_label(effects_window, "Enhanced 3D Depth:", 10, 10);
        gui_create_label(effects_window, "* Multi-layer shadows", 20, 30);
        gui_create_label(effects_window, "* Horizontal gradients", 20, 45);
        gui_create_label(effects_window, "* Radial gradients", 20, 60);
        gui_create_label(effects_window, "* Rounded alpha corners", 20, 75);
        
        gui_create_label(effects_window, "4D Animation:", 10, 105);
        gui_create_label(effects_window, "* Smooth easing (9 types)", 20, 125);
        gui_create_label(effects_window, "* Color transitions", 20, 140);
        gui_create_label(effects_window, "* Scale interpolation", 20, 155);
        
        gui_create_label(effects_window, "5D Interactive:", 10, 185);
        gui_create_label(effects_window, "* Layered window depth", 20, 205);
        gui_create_label(effects_window, "* Icon depth system", 20, 220);
        gui_create_label(effects_window, "* Hover glow effects", 20, 235);
        gui_create_label(effects_window, "* Particle on interact", 20, 250);
        gui_create_label(effects_window, "* Glass/blur effects", 20, 265);
        gui_create_label(effects_window, "* Perspective scaling", 20, 280);
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
    
    // Update layered windows demo if active
    if (layered_demo_active && animation_tick % 120 < 60) {
        framebuffer_info_t* fb = framebuffer_get_info();
        if (fb) {
            // Animate the layered windows slightly
            int32_t base_x = fb->width / 2 - 200;
            int32_t base_y = fb->height / 2 - 150;
            
            float anim_t = (float)(animation_tick % 120) / 60.0f;
            if (anim_t > 1.0f) anim_t = 2.0f - anim_t;
            
            // Subtle wave effect on depth
            for (int i = 4; i >= 0; i--) {
                float base_depth = (float)i / 5.0f;
                float depth = base_depth + (anim_t - 0.5f) * 0.1f;
                if (depth < 0.0f) depth = 0.0f;
                if (depth > 1.0f) depth = 1.0f;
                
                color_t window_colors[5] = {
                    {200, 100, 100, 255},
                    {100, 200, 100, 255},
                    {100, 100, 200, 255},
                    {200, 200, 100, 255},
                    {200, 100, 200, 255}
                };
                
                const char* titles[5] = {
                    "Back", "Mid-Back", "Middle", "Mid-Front", "Front"
                };
                
                gui_draw_window_with_depth(base_x, base_y, 400, 250, depth,
                                          window_colors[i], titles[i]);
            }
        }
    }
    
    // Simulate icon hover for demo (cycle through icons)
    if (icon_sprites[0] != NULL) {
        static uint32_t icon_cycle = 0;
        icon_cycle++;
        
        for (int i = 0; i < 5; i++) {
            icon_hover[i] = ((icon_cycle / 30) % 5) == (uint32_t)i;
        }
    }
}
