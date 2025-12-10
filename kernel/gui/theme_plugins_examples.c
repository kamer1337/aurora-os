/**
 * Aurora OS - Example Theme Plugins Implementation
 * 
 * Demonstrates plugin API with three example themes:
 * - Neon: Bright neon colors with glow effects
 * - Retro: Classic 80s/90s computing aesthetic
 * - Matrix: Green-on-black Matrix-inspired theme
 */

#include "theme_plugins_examples.h"
#include "theme_manager.h"
#include "gui_effects.h"
#include "framebuffer.h"

// ============================================================================
// NEON THEME PLUGIN
// ============================================================================

static theme_t neon_theme;
static plugin_effect_t neon_glow_effect;

// Neon glow effect renderer
static void neon_glow_render(int32_t x, int32_t y, uint32_t width, uint32_t height,
                             void* params, void* user_data) {
    (void)params;
    (void)user_data;
    
    // Create intense neon glow
    color_t neon_pink = {255, 20, 147, 255};
    gui_draw_glow(x, y, width, height, neon_pink, 8);
    
    // Add cyan accent glow
    color_t neon_cyan = {0, 255, 255, 200};
    gui_draw_glow(x + 2, y + 2, width - 4, height - 4, neon_cyan, 5);
}

// Neon plugin init
static int neon_plugin_init(theme_plugin_t* plugin) {
    (void)plugin;
    
    // Initialize neon theme colors
    neon_theme.type = THEME_CUSTOM;
    neon_theme.name = "Neon";
    
    // Desktop background - dark with neon gradient
    neon_theme.desktop_bg_start = (color_t){10, 0, 20, 255};
    neon_theme.desktop_bg_end = (color_t){40, 0, 60, 255};
    
    // Taskbar - neon pink
    neon_theme.taskbar_bg = (color_t){200, 0, 100, 255};
    neon_theme.taskbar_fg = (color_t){255, 255, 255, 255};
    neon_theme.taskbar_highlight = (color_t){255, 20, 147, 255};
    
    // Window colors - dark with neon accents
    neon_theme.window_title_active = (color_t){255, 0, 128, 255};
    neon_theme.window_title_inactive = (color_t){100, 0, 50, 255};
    neon_theme.window_border = (color_t){255, 20, 147, 255};
    neon_theme.window_bg = (color_t){20, 0, 30, 255};
    
    // Buttons - cyan glow
    neon_theme.button_bg = (color_t){0, 180, 200, 255};
    neon_theme.button_fg = (color_t){255, 255, 255, 255};
    neon_theme.button_hover = (color_t){0, 255, 255, 255};
    neon_theme.button_pressed = (color_t){0, 140, 160, 255};
    
    // Text - bright on dark
    neon_theme.text_primary = (color_t){255, 255, 255, 255};
    neon_theme.text_secondary = (color_t){200, 200, 255, 255};
    neon_theme.text_disabled = (color_t){100, 100, 120, 255};
    
    return 0;
}

// Neon plugin shutdown
static void neon_plugin_shutdown(theme_plugin_t* plugin) {
    (void)plugin;
    // Nothing to cleanup
}

// Neon plugin theme provider
static const theme_t* neon_plugin_get_theme(theme_plugin_t* plugin) {
    (void)plugin;
    return &neon_theme;
}

// Neon plugin definition
static theme_plugin_t neon_plugin = {
    .name = "Neon",
    .author = "Aurora OS Team",
    .version = "1.0.0",
    .description = "Bright neon colors with intense glow effects",
    .api_version = THEME_PLUGIN_API_VERSION,
    .type = PLUGIN_TYPE_COMBINED,
    .status = PLUGIN_STATUS_UNLOADED,
    .init = neon_plugin_init,
    .shutdown = neon_plugin_shutdown,
    .get_theme = neon_plugin_get_theme,
    .private_data = NULL,
    .effects = NULL,
    .next = NULL
};

// ============================================================================
// RETRO THEME PLUGIN
// ============================================================================

static theme_t retro_theme;
static plugin_effect_t retro_scanline_effect;

// Retro scanline effect renderer
static void retro_scanline_render(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                  void* params, void* user_data) {
    (void)params;
    (void)user_data;
    
    // Draw horizontal scanlines
    color_t scanline_color = {0, 0, 0, 60};
    for (uint32_t i = 0; i < height; i += 2) {
        framebuffer_draw_rect(x, y + i, width, 1, scanline_color);
    }
}

// Retro plugin init
static int retro_plugin_init(theme_plugin_t* plugin) {
    (void)plugin;
    
    // Initialize retro theme colors (classic beige/brown computing)
    retro_theme.type = THEME_CUSTOM;
    retro_theme.name = "Retro";
    
    // Desktop background - warm beige gradient
    retro_theme.desktop_bg_start = (color_t){200, 180, 140, 255};
    retro_theme.desktop_bg_end = (color_t){160, 140, 100, 255};
    
    // Taskbar - classic gray
    retro_theme.taskbar_bg = (color_t){192, 192, 192, 255};
    retro_theme.taskbar_fg = (color_t){0, 0, 0, 255};
    retro_theme.taskbar_highlight = (color_t){0, 0, 128, 255};
    
    // Window colors - classic 3D window look
    retro_theme.window_title_active = (color_t){0, 0, 128, 255};
    retro_theme.window_title_inactive = (color_t){128, 128, 128, 255};
    retro_theme.window_border = (color_t){192, 192, 192, 255};
    retro_theme.window_bg = (color_t){192, 192, 192, 255};
    
    // Buttons - classic raised look
    retro_theme.button_bg = (color_t){192, 192, 192, 255};
    retro_theme.button_fg = (color_t){0, 0, 0, 255};
    retro_theme.button_hover = (color_t){210, 210, 210, 255};
    retro_theme.button_pressed = (color_t){160, 160, 160, 255};
    
    // Text - black on gray
    retro_theme.text_primary = (color_t){0, 0, 0, 255};
    retro_theme.text_secondary = (color_t){64, 64, 64, 255};
    retro_theme.text_disabled = (color_t){128, 128, 128, 255};
    
    return 0;
}

// Retro plugin shutdown
static void retro_plugin_shutdown(theme_plugin_t* plugin) {
    (void)plugin;
    // Nothing to cleanup
}

// Retro plugin theme provider
static const theme_t* retro_plugin_get_theme(theme_plugin_t* plugin) {
    (void)plugin;
    return &retro_theme;
}

// Retro plugin definition
static theme_plugin_t retro_plugin = {
    .name = "Retro",
    .author = "Aurora OS Team",
    .version = "1.0.0",
    .description = "Classic 80s/90s computing aesthetic with CRT scanlines",
    .api_version = THEME_PLUGIN_API_VERSION,
    .type = PLUGIN_TYPE_COMBINED,
    .status = PLUGIN_STATUS_UNLOADED,
    .init = retro_plugin_init,
    .shutdown = retro_plugin_shutdown,
    .get_theme = retro_plugin_get_theme,
    .private_data = NULL,
    .effects = NULL,
    .next = NULL
};

// ============================================================================
// MATRIX THEME PLUGIN
// ============================================================================

static theme_t matrix_theme;
static plugin_effect_t matrix_rain_effect;

// Matrix rain effect renderer (simple version)
static void matrix_rain_render(int32_t x, int32_t y, uint32_t width, uint32_t height,
                               void* params, void* user_data) {
    (void)params;
    (void)user_data;
    
    // Draw random "digital rain" effect
    color_t matrix_green = {0, 255, 0, 100};
    color_t matrix_dim = {0, 150, 0, 50};
    
    // Create vertical "rain" streaks
    for (uint32_t i = 0; i < width; i += 12) {
        uint32_t streak_height = (i * 7) % (height / 2) + 20;
        framebuffer_draw_rect(x + i, y, 2, streak_height, matrix_dim);
        framebuffer_draw_rect(x + i, y + streak_height - 10, 2, 10, matrix_green);
    }
}

// Matrix plugin init
static int matrix_plugin_init(theme_plugin_t* plugin) {
    (void)plugin;
    
    // Initialize matrix theme colors (green on black)
    matrix_theme.type = THEME_CUSTOM;
    matrix_theme.name = "Matrix";
    
    // Desktop background - pure black
    matrix_theme.desktop_bg_start = (color_t){0, 0, 0, 255};
    matrix_theme.desktop_bg_end = (color_t){0, 20, 0, 255};
    
    // Taskbar - dark with green accents
    matrix_theme.taskbar_bg = (color_t){0, 20, 0, 255};
    matrix_theme.taskbar_fg = (color_t){0, 255, 0, 255};
    matrix_theme.taskbar_highlight = (color_t){0, 180, 0, 255};
    
    // Window colors - terminal style
    matrix_theme.window_title_active = (color_t){0, 180, 0, 255};
    matrix_theme.window_title_inactive = (color_t){0, 80, 0, 255};
    matrix_theme.window_border = (color_t){0, 255, 0, 255};
    matrix_theme.window_bg = (color_t){0, 10, 0, 255};
    
    // Buttons - green phosphor look
    matrix_theme.button_bg = (color_t){0, 100, 0, 255};
    matrix_theme.button_fg = (color_t){0, 255, 0, 255};
    matrix_theme.button_hover = (color_t){0, 150, 0, 255};
    matrix_theme.button_pressed = (color_t){0, 60, 0, 255};
    
    // Text - terminal green
    matrix_theme.text_primary = (color_t){0, 255, 0, 255};
    matrix_theme.text_secondary = (color_t){0, 180, 0, 255};
    matrix_theme.text_disabled = (color_t){0, 80, 0, 255};
    
    return 0;
}

// Matrix plugin shutdown
static void matrix_plugin_shutdown(theme_plugin_t* plugin) {
    (void)plugin;
    // Nothing to cleanup
}

// Matrix plugin theme provider
static const theme_t* matrix_plugin_get_theme(theme_plugin_t* plugin) {
    (void)plugin;
    return &matrix_theme;
}

// Matrix plugin definition
static theme_plugin_t matrix_plugin = {
    .name = "Matrix",
    .author = "Aurora OS Team",
    .version = "1.0.0",
    .description = "Green-on-black Matrix-inspired terminal theme with digital rain",
    .api_version = THEME_PLUGIN_API_VERSION,
    .type = PLUGIN_TYPE_COMBINED,
    .status = PLUGIN_STATUS_UNLOADED,
    .init = matrix_plugin_init,
    .shutdown = matrix_plugin_shutdown,
    .get_theme = matrix_plugin_get_theme,
    .private_data = NULL,
    .effects = NULL,
    .next = NULL
};

// ============================================================================
// EXAMPLE PLUGINS INITIALIZATION
// ============================================================================

int theme_plugins_examples_init(void) {
    int result = 0;
    
    // Initialize plugin system if needed
    theme_plugin_system_init();
    
    // Setup neon effect
    neon_glow_effect.name = "neon_glow";
    neon_glow_effect.description = "Intense neon glow effect";
    neon_glow_effect.render = neon_glow_render;
    neon_glow_effect.user_data = NULL;
    neon_glow_effect.next = NULL;
    
    // Setup retro effect
    retro_scanline_effect.name = "scanlines";
    retro_scanline_effect.description = "CRT scanline effect";
    retro_scanline_effect.render = retro_scanline_render;
    retro_scanline_effect.user_data = NULL;
    retro_scanline_effect.next = NULL;
    
    // Setup matrix effect
    matrix_rain_effect.name = "matrix_rain";
    matrix_rain_effect.description = "Matrix digital rain effect";
    matrix_rain_effect.render = matrix_rain_render;
    matrix_rain_effect.user_data = NULL;
    matrix_rain_effect.next = NULL;
    
    // Register plugins
    result |= theme_plugin_register(&neon_plugin);
    result |= theme_plugin_register(&retro_plugin);
    result |= theme_plugin_register(&matrix_plugin);
    
    // Register effects with plugins
    theme_plugin_register_effect(&neon_plugin, &neon_glow_effect);
    theme_plugin_register_effect(&retro_plugin, &retro_scanline_effect);
    theme_plugin_register_effect(&matrix_plugin, &matrix_rain_effect);
    
    return result;
}

void theme_plugins_examples_shutdown(void) {
    // Unregister plugins
    theme_plugin_unregister(&neon_plugin);
    theme_plugin_unregister(&retro_plugin);
    theme_plugin_unregister(&matrix_plugin);
}

theme_plugin_t* theme_plugin_get_neon(void) {
    return &neon_plugin;
}

theme_plugin_t* theme_plugin_get_retro(void) {
    return &retro_plugin;
}

theme_plugin_t* theme_plugin_get_matrix(void) {
    return &matrix_plugin;
}
