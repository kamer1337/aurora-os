/**
 * Aurora OS - GUI Demo
 * 
 * Demonstrates the GUI framework capabilities
 */

#include "gui_demo.h"
#include "../gui/gui.h"
#include "../gui/framebuffer.h"

// Window background color for font demo
#define FONT_WINDOW_BG_COLOR ((color_t){40, 40, 60, 255})

// Button click handlers
static void button1_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    // In a real system, this would perform an action
    // For now, we just acknowledge the click happened
}

static void button2_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    // Another button handler
}

void gui_demo_init(void) {
    // Initialize GUI system
    if (gui_init() != 0) {
        return;
    }
    
    // Initialize input handling (mouse and keyboard)
    gui_init_input();
    
    // Create a welcome window
    window_t* welcome_win = gui_create_window("Welcome to Aurora OS", 150, 100, 500, 350);
    if (welcome_win) {
        // Add welcome message labels
        gui_create_label(welcome_win, "Aurora OS - Version 0.1", 40, 20);
        gui_create_label(welcome_win, "A Modern Operating System", 40, 40);
        gui_create_label(welcome_win, "", 40, 60);  // Spacer
        gui_create_label(welcome_win, "Features:", 40, 80);
        gui_create_label(welcome_win, "* Quantum Encryption", 60, 100);
        gui_create_label(welcome_win, "* Virtual File System", 60, 120);
        gui_create_label(welcome_win, "* Process Management", 60, 140);
        gui_create_label(welcome_win, "* GUI Framework", 60, 160);
        
        // Add buttons
        widget_t* btn1 = gui_create_button(welcome_win, "OK", 40, 200, 100, 30);
        widget_t* btn2 = gui_create_button(welcome_win, "About", 160, 200, 100, 30);
        
        gui_set_widget_click_handler(btn1, button1_click);
        gui_set_widget_click_handler(btn2, button2_click);
        
        // Add panel for additional info
        widget_t* panel = gui_create_panel(welcome_win, 40, 250, 420, 60);
        (void)panel;  // Unused for now
    }
    
    // Create a system info window
    window_t* info_win = gui_create_window("System Information", 200, 150, 400, 250);
    if (info_win) {
        gui_create_label(info_win, "System Status:", 20, 20);
        gui_create_label(info_win, "* Memory: Initialized", 40, 50);
        gui_create_label(info_win, "* Scheduler: Running", 40, 70);
        gui_create_label(info_win, "* VFS: Mounted", 40, 90);
        gui_create_label(info_win, "* Display: 1024x768x32", 40, 110);
        
        widget_t* close_btn = gui_create_button(info_win, "Close", 20, 150, 100, 30);
        gui_set_widget_click_handler(close_btn, button1_click);
    }
    
    // Create a font demo window
    window_t* font_win = gui_create_window("Font Demo - 5x7 vs 8x8", 250, 200, 520, 300);
    if (font_win) {
        // Draw comparison using framebuffer directly
        framebuffer_info_t* fb_info = framebuffer_get_info();
        if (fb_info && fb_info->address) {
            // Demo text for both fonts
            const char* demo_text_1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            const char* demo_text_2 = "abcdefghijklmnopqrstuvwxyz";
            const char* demo_text_3 = "0123456789 !@#$%^&*()";
            
            uint32_t base_x = font_win->bounds.x + 20;
            uint32_t base_y = font_win->bounds.y + 50;
            
            // Draw 8x8 font section
            framebuffer_draw_string(base_x, base_y, "8x8 Font:", COLOR_WHITE, 
                                  FONT_WINDOW_BG_COLOR);
            framebuffer_draw_string(base_x, base_y + 15, demo_text_1, COLOR_CYAN, 
                                  FONT_WINDOW_BG_COLOR);
            framebuffer_draw_string(base_x, base_y + 25, demo_text_2, COLOR_CYAN, 
                                  FONT_WINDOW_BG_COLOR);
            framebuffer_draw_string(base_x, base_y + 35, demo_text_3, COLOR_CYAN, 
                                  FONT_WINDOW_BG_COLOR);
            
            // Draw 5x7 font section
            framebuffer_draw_string_5x7(base_x, base_y + 100, "5x7 Font:", COLOR_WHITE, 
                                       FONT_WINDOW_BG_COLOR);
            framebuffer_draw_string_5x7(base_x, base_y + 115, demo_text_1, COLOR_GREEN, 
                                       FONT_WINDOW_BG_COLOR);
            framebuffer_draw_string_5x7(base_x, base_y + 125, demo_text_2, COLOR_GREEN, 
                                       FONT_WINDOW_BG_COLOR);
            framebuffer_draw_string_5x7(base_x, base_y + 135, demo_text_3, COLOR_GREEN, 
                                       FONT_WINDOW_BG_COLOR);
            
            // Add info text
            framebuffer_draw_string_5x7(base_x, base_y + 180, "5x7 font is more compact!", 
                                       COLOR_YELLOW, FONT_WINDOW_BG_COLOR);
        }
        
        widget_t* font_btn = gui_create_button(font_win, "OK", 20, 220, 100, 30);
        gui_set_widget_click_handler(font_btn, button1_click);
    }
    
    // Update the display to show windows
    gui_update();
}

void gui_demo_show(void) {
    // Trigger a repaint
    gui_update();
}
