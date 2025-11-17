/**
 * Aurora OS - Theme and Wallpaper System Tests
 * 
 * Tests for the theme manager and wallpaper manager
 */

#include "theme_wallpaper_tests.h"
#include "../kernel/gui/theme_manager.h"
#include "../kernel/gui/wallpaper_manager.h"
#include "../kernel/gui/desktop_config.h"
#include <stddef.h>

// Simple test result counter
static int tests_passed = 0;
static int tests_failed = 0;

// Test assertion helper
static void test_assert(int condition, const char* test_name) {
    if (condition) {
        tests_passed++;
    } else {
        tests_failed++;
    }
}

void run_theme_manager_tests(void) {
    // Test 1: Theme manager initialization
    int result = theme_manager_init();
    test_assert(result == 0, "Theme manager initialization");
    
    // Test 2: Get default theme
    const theme_t* default_theme = theme_manager_get_current();
    test_assert(default_theme != NULL, "Get current theme");
    test_assert(default_theme->type == THEME_DEFAULT, "Default theme is correct");
    
    // Test 3: Get theme by type
    const theme_t* dark_theme = theme_manager_get_theme(THEME_DARK);
    test_assert(dark_theme != NULL, "Get dark theme");
    test_assert(dark_theme->type == THEME_DARK, "Dark theme type is correct");
    
    // Test 4: Get theme by index
    for (uint32_t i = 0; i < theme_manager_get_count(); i++) {
        const theme_t* theme = theme_manager_get_by_index(i);
        test_assert(theme != NULL, "Get theme by index");
    }
    
    // Test 5: Set theme
    result = theme_manager_set_theme(THEME_OCEAN);
    test_assert(result == 0, "Set ocean theme");
    
    const theme_t* current = theme_manager_get_current();
    test_assert(current->type == THEME_OCEAN, "Current theme changed to ocean");
    
    // Test 6: Invalid theme type
    result = theme_manager_set_theme(THEME_COUNT + 10);
    test_assert(result == -1, "Invalid theme type rejected");
    
    // Test 7: Theme has valid colors
    const theme_t* light_theme = theme_manager_get_theme(THEME_LIGHT);
    test_assert(light_theme->desktop_bg_start.a == 255, "Theme colors have valid alpha");
    
    // Test 8: All themes have names
    for (int i = 0; i < THEME_COUNT; i++) {
        const theme_t* theme = theme_manager_get_theme((theme_type_t)i);
        test_assert(theme != NULL && theme->name != NULL, "Theme has name");
    }
    
    // Cleanup
    theme_manager_shutdown();
}

void run_wallpaper_manager_tests(void) {
    // Test 1: Wallpaper manager initialization
    int result = wallpaper_manager_init();
    test_assert(result == 0, "Wallpaper manager initialization");
    
    // Test 2: Get configuration
    wallpaper_config_t* config = wallpaper_manager_get_config();
    test_assert(config != NULL, "Get wallpaper configuration");
    
    // Test 3: Set solid color wallpaper
    color_t test_color = {100, 150, 200, 255};
    result = wallpaper_manager_set_solid_color(test_color);
    test_assert(result == 0, "Set solid color wallpaper");
    
    config = wallpaper_manager_get_config();
    test_assert(config->mode == WALLPAPER_MODE_SOLID, "Wallpaper mode is solid");
    test_assert(config->solid_color.r == 100, "Solid color R is correct");
    test_assert(config->solid_color.g == 150, "Solid color G is correct");
    test_assert(config->solid_color.b == 200, "Solid color B is correct");
    
    // Test 4: Set gradient wallpaper
    result = wallpaper_manager_set_gradient(GRADIENT_STYLE_OCEAN, WALLPAPER_MODE_GRADIENT_V);
    test_assert(result == 0, "Set gradient wallpaper");
    
    config = wallpaper_manager_get_config();
    test_assert(config->mode == WALLPAPER_MODE_GRADIENT_V, "Wallpaper mode is vertical gradient");
    test_assert(config->gradient_style == GRADIENT_STYLE_OCEAN, "Gradient style is ocean");
    
    // Test 5: Set custom gradient
    color_t grad_start = {50, 100, 150, 255};
    color_t grad_end = {150, 200, 250, 255};
    result = wallpaper_manager_set_custom_gradient(grad_start, grad_end, WALLPAPER_MODE_GRADIENT_H);
    test_assert(result == 0, "Set custom gradient");
    
    config = wallpaper_manager_get_config();
    test_assert(config->mode == WALLPAPER_MODE_GRADIENT_H, "Wallpaper mode is horizontal gradient");
    test_assert(config->gradient_style == GRADIENT_STYLE_CUSTOM, "Gradient style is custom");
    
    // Test 6: Set live wallpaper
    result = wallpaper_manager_set_live(WALLPAPER_NATURE_FOREST);
    test_assert(result == 0, "Set live wallpaper");
    
    config = wallpaper_manager_get_config();
    test_assert(config->mode == WALLPAPER_MODE_LIVE, "Wallpaper mode is live");
    test_assert(config->live_type == WALLPAPER_NATURE_FOREST, "Live wallpaper type is forest");
    
    // Test 7: Get gradient names
    for (int i = 0; i < GRADIENT_STYLE_CUSTOM; i++) {
        const char* name = wallpaper_manager_get_gradient_name((gradient_style_t)i);
        test_assert(name != NULL, "Gradient style has name");
    }
    
    // Test 8: Get gradient colors
    color_t start, end;
    wallpaper_manager_get_gradient_colors(GRADIENT_STYLE_SUNSET, &start, &end);
    test_assert(start.a == 255 && end.a == 255, "Gradient colors have valid alpha");
    
    // Cleanup
    wallpaper_manager_shutdown();
}

void run_integration_tests(void) {
    // Test 1: Desktop config initialization
    int result = desktop_config_init();
    test_assert(result == 0, "Desktop config initialization");
    
    // Test 2: Theme manager integration
    result = theme_manager_set_theme(THEME_FOREST);
    test_assert(result == 0, "Set forest theme via theme manager");
    
    desktop_config_t* config = desktop_config_get();
    test_assert(config != NULL, "Get desktop configuration");
    
    // Verify that theme colors were applied to desktop config
    const theme_t* forest_theme = theme_manager_get_theme(THEME_FOREST);
    test_assert(config->desktop_bg_start.r == forest_theme->desktop_bg_start.r, 
                "Desktop config updated with theme colors");
    
    // Test 3: Wallpaper manager integration
    result = wallpaper_manager_set_gradient(GRADIENT_STYLE_PURPLE_DREAM, WALLPAPER_MODE_GRADIENT_V);
    test_assert(result == 0, "Set purple dream gradient");
    
    wallpaper_config_t* wp_config = wallpaper_manager_get_config();
    test_assert(wp_config->mode == WALLPAPER_MODE_GRADIENT_V, "Wallpaper configuration applied");
    
    // Test 4: Switch between multiple themes
    for (int i = 0; i < THEME_COUNT - 1; i++) {  // Exclude THEME_CUSTOM
        result = theme_manager_set_theme((theme_type_t)i);
        test_assert(result == 0, "Switch to theme");
    }
    
    // Test 5: Switch between multiple wallpapers
    result = wallpaper_manager_set_solid_color((color_t){50, 50, 50, 255});
    test_assert(result == 0, "Switch to solid wallpaper");
    
    result = wallpaper_manager_set_gradient(GRADIENT_STYLE_NIGHT, WALLPAPER_MODE_GRADIENT_RADIAL);
    test_assert(result == 0, "Switch to radial gradient");
    
    result = wallpaper_manager_set_live(WALLPAPER_NATURE_OCEAN);
    test_assert(result == 0, "Switch to live wallpaper");
}

int run_all_theme_wallpaper_tests(void) {
    tests_passed = 0;
    tests_failed = 0;
    
    run_theme_manager_tests();
    run_wallpaper_manager_tests();
    run_integration_tests();
    
    return tests_failed;
}

int get_theme_wallpaper_tests_passed(void) {
    return tests_passed;
}

int get_theme_wallpaper_tests_failed(void) {
    return tests_failed;
}
