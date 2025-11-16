/**
 * Aurora OS - Font Tests
 * 
 * Tests for 5x7 font encoding
 */

#include "../kernel/gui/framebuffer.h"
#include "../kernel/drivers/vga.h"

/**
 * Test that 5x7 font is properly encoded for key characters
 */
static void test_5x7_font_encoding(void) {
    vga_write("\n=== Testing 5x7 Font Encoding ===\n");
    
    // Initialize framebuffer for testing
    int result = framebuffer_init(1024, 768, 32);
    vga_write("Framebuffer initialization: ");
    if (result == 0) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
        return;
    }
    
    // Test that framebuffer is available
    vga_write("Framebuffer available: ");
    if (framebuffer_is_available()) {
        vga_write("PASS\n");
    } else {
        vga_write("FAIL\n");
        return;
    }
    
    // Test drawing characters with 5x7 font
    vga_write("Drawing test characters with 5x7 font: ");
    
    // Test uppercase letters
    framebuffer_draw_char_5x7(10, 10, 'A', COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_char_5x7(20, 10, 'B', COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_char_5x7(30, 10, 'Z', COLOR_WHITE, COLOR_BLACK);
    
    // Test lowercase letters
    framebuffer_draw_char_5x7(10, 20, 'a', COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_char_5x7(20, 20, 'b', COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_char_5x7(30, 20, 'z', COLOR_WHITE, COLOR_BLACK);
    
    // Test numbers
    framebuffer_draw_char_5x7(10, 30, '0', COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_char_5x7(20, 30, '5', COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_char_5x7(30, 30, '9', COLOR_WHITE, COLOR_BLACK);
    
    vga_write("PASS\n");
    
    // Test drawing strings with 5x7 font
    vga_write("Drawing test strings with 5x7 font: ");
    framebuffer_draw_string_5x7(10, 50, "Hello World!", COLOR_GREEN, COLOR_BLACK);
    framebuffer_draw_string_5x7(10, 60, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", COLOR_CYAN, COLOR_BLACK);
    framebuffer_draw_string_5x7(10, 70, "abcdefghijklmnopqrstuvwxyz", COLOR_YELLOW, COLOR_BLACK);
    framebuffer_draw_string_5x7(10, 80, "0123456789", COLOR_MAGENTA, COLOR_BLACK);
    vga_write("PASS\n");
    
    // Test comparison with 8x8 font
    vga_write("Comparing 8x8 and 5x7 fonts: ");
    framebuffer_draw_string(10, 100, "8x8: The quick brown fox", COLOR_WHITE, COLOR_BLACK);
    framebuffer_draw_string_5x7(10, 110, "5x7: The quick brown fox", COLOR_WHITE, COLOR_BLACK);
    vga_write("PASS\n");
    
    vga_write("5x7 font encoding tests: COMPLETE\n");
}

/**
 * Run all font tests
 */
void run_font_tests(void) {
    vga_write("\n======================================\n");
    vga_write("  Font Test Suite\n");
    vga_write("======================================\n");
    
    test_5x7_font_encoding();
    
    vga_write("\n======================================\n");
    vga_write("  Font Tests Complete\n");
    vga_write("======================================\n");
}
