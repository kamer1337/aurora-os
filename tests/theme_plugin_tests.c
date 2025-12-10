/**
 * Aurora OS - Theme Plugin System Tests Implementation
 */

#include "theme_plugin_tests.h"
#include "../kernel/gui/theme_plugin.h"
#include "../kernel/gui/theme_plugins_examples.h"
#include "../kernel/gui/gui_5d_renderer.h"
#include "../kernel/gui/theme_manager.h"
#include <stdio.h>

// Test helper
static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("[FAIL] %s: %s\n", __FUNCTION__, message); \
            test_failed++; \
            return -1; \
        } \
        test_passed++; \
    } while(0)

int test_plugin_registration(void) {
    printf("[TEST] Plugin Registration\n");
    
    // Initialize plugin system
    int result = theme_plugin_system_init();
    TEST_ASSERT(result == 0, "Plugin system init failed");
    
    // Create a test plugin
    static theme_plugin_t test_plugin = {
        .name = "TestPlugin",
        .author = "Test",
        .version = "1.0",
        .description = "Test plugin",
        .api_version = THEME_PLUGIN_API_VERSION,
        .type = PLUGIN_TYPE_THEME,
        .status = PLUGIN_STATUS_UNLOADED,
        .init = NULL,
        .shutdown = NULL,
        .get_theme = NULL,
        .private_data = NULL,
        .effects = NULL,
        .next = NULL
    };
    
    // Register plugin
    result = theme_plugin_register(&test_plugin);
    TEST_ASSERT(result == 0, "Plugin registration failed");
    
    // Try to register again (should fail)
    result = theme_plugin_register(&test_plugin);
    TEST_ASSERT(result == -1, "Duplicate registration should fail");
    
    // Get plugin by name
    theme_plugin_t* found = theme_plugin_get_by_name("TestPlugin");
    TEST_ASSERT(found == &test_plugin, "Plugin not found by name");
    
    // Unregister plugin
    result = theme_plugin_unregister(&test_plugin);
    TEST_ASSERT(result == 0, "Plugin unregistration failed");
    
    // Verify plugin removed
    found = theme_plugin_get_by_name("TestPlugin");
    TEST_ASSERT(found == NULL, "Plugin should be removed");
    
    printf("[PASS] Plugin Registration\n");
    return 0;
}

int test_plugin_loading(void) {
    printf("[TEST] Plugin Loading\n");
    
    // Initialize plugin system
    theme_plugin_system_init();
    
    // Flag to track init/shutdown calls
    static int init_called = 0;
    static int shutdown_called = 0;
    
    // Init callback
    static int test_init(theme_plugin_t* plugin) {
        (void)plugin;
        init_called = 1;
        return 0;
    }
    
    // Shutdown callback
    static void test_shutdown(theme_plugin_t* plugin) {
        (void)plugin;
        shutdown_called = 1;
    }
    
    // Create test plugin with callbacks
    static theme_plugin_t test_plugin = {
        .name = "LoadTestPlugin",
        .author = "Test",
        .version = "1.0",
        .description = "Test plugin",
        .api_version = THEME_PLUGIN_API_VERSION,
        .type = PLUGIN_TYPE_THEME,
        .status = PLUGIN_STATUS_UNLOADED,
        .init = test_init,
        .shutdown = test_shutdown,
        .get_theme = NULL,
        .private_data = NULL,
        .effects = NULL,
        .next = NULL
    };
    
    // Register plugin
    int result = theme_plugin_register(&test_plugin);
    TEST_ASSERT(result == 0, "Plugin registration failed");
    
    // Load plugin
    init_called = 0;
    result = theme_plugin_load(&test_plugin);
    TEST_ASSERT(result == 0, "Plugin load failed");
    TEST_ASSERT(init_called == 1, "Init callback not called");
    TEST_ASSERT(test_plugin.status == PLUGIN_STATUS_LOADED, "Plugin status incorrect");
    
    // Unload plugin
    shutdown_called = 0;
    result = theme_plugin_unload(&test_plugin);
    TEST_ASSERT(result == 0, "Plugin unload failed");
    TEST_ASSERT(shutdown_called == 1, "Shutdown callback not called");
    TEST_ASSERT(test_plugin.status == PLUGIN_STATUS_UNLOADED, "Plugin status incorrect");
    
    // Cleanup
    theme_plugin_unregister(&test_plugin);
    
    printf("[PASS] Plugin Loading\n");
    return 0;
}

int test_plugin_activation(void) {
    printf("[TEST] Plugin Activation\n");
    
    // Initialize systems
    theme_plugin_system_init();
    theme_manager_init();
    
    // Initialize example plugins
    int result = theme_plugins_examples_init();
    TEST_ASSERT(result == 0, "Example plugins init failed");
    
    // Get neon plugin
    theme_plugin_t* neon = theme_plugin_get_neon();
    TEST_ASSERT(neon != NULL, "Neon plugin not found");
    
    // Activate neon plugin
    result = theme_plugin_activate(neon);
    TEST_ASSERT(result == 0, "Plugin activation failed");
    TEST_ASSERT(neon->status == PLUGIN_STATUS_ACTIVE, "Plugin status not active");
    
    // Verify it's the active plugin
    theme_plugin_t* active = theme_plugin_get_active();
    TEST_ASSERT(active == neon, "Active plugin incorrect");
    
    // Deactivate by activating another
    theme_plugin_t* retro = theme_plugin_get_retro();
    result = theme_plugin_activate(retro);
    TEST_ASSERT(result == 0, "Retro activation failed");
    TEST_ASSERT(neon->status == PLUGIN_STATUS_LOADED, "Previous plugin should be loaded");
    TEST_ASSERT(retro->status == PLUGIN_STATUS_ACTIVE, "Retro should be active");
    
    // Cleanup
    theme_plugins_examples_shutdown();
    
    printf("[PASS] Plugin Activation\n");
    return 0;
}

int test_plugin_effects(void) {
    printf("[TEST] Plugin Effects\n");
    
    // Initialize plugin system
    theme_plugin_system_init();
    
    // Flag to track effect render call
    static int effect_rendered = 0;
    
    // Test effect renderer
    static void test_effect_render(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                   void* params, void* user_data) {
        (void)x; (void)y; (void)width; (void)height; (void)params; (void)user_data;
        effect_rendered = 1;
    }
    
    // Create test plugin
    static theme_plugin_t test_plugin = {
        .name = "EffectTestPlugin",
        .author = "Test",
        .version = "1.0",
        .description = "Effect test",
        .api_version = THEME_PLUGIN_API_VERSION,
        .type = PLUGIN_TYPE_EFFECT,
        .status = PLUGIN_STATUS_UNLOADED,
        .init = NULL,
        .shutdown = NULL,
        .get_theme = NULL,
        .private_data = NULL,
        .effects = NULL,
        .next = NULL
    };
    
    // Create test effect
    static plugin_effect_t test_effect = {
        .name = "test_effect",
        .description = "Test effect",
        .render = test_effect_render,
        .user_data = NULL,
        .next = NULL
    };
    
    // Register plugin
    int result = theme_plugin_register(&test_plugin);
    TEST_ASSERT(result == 0, "Plugin registration failed");
    
    // Register effect
    result = theme_plugin_register_effect(&test_plugin, &test_effect);
    TEST_ASSERT(result == 0, "Effect registration failed");
    
    // Get effect
    plugin_effect_t* found = theme_plugin_get_effect(&test_plugin, "test_effect");
    TEST_ASSERT(found == &test_effect, "Effect not found");
    
    // Render effect
    effect_rendered = 0;
    result = theme_plugin_render_effect(&test_plugin, "test_effect", 0, 0, 100, 100, NULL);
    TEST_ASSERT(result == 0, "Effect render failed");
    TEST_ASSERT(effect_rendered == 1, "Effect render callback not called");
    
    // Unregister effect
    result = theme_plugin_unregister_effect(&test_plugin, "test_effect");
    TEST_ASSERT(result == 0, "Effect unregistration failed");
    
    // Verify removed
    found = theme_plugin_get_effect(&test_plugin, "test_effect");
    TEST_ASSERT(found == NULL, "Effect should be removed");
    
    // Cleanup
    theme_plugin_unregister(&test_plugin);
    
    printf("[PASS] Plugin Effects\n");
    return 0;
}

int test_example_plugins(void) {
    printf("[TEST] Example Plugins\n");
    
    // Initialize systems
    theme_plugin_system_init();
    
    // Initialize example plugins
    int result = theme_plugins_examples_init();
    TEST_ASSERT(result == 0, "Example plugins init failed");
    
    // Test Neon plugin
    theme_plugin_t* neon = theme_plugin_get_neon();
    TEST_ASSERT(neon != NULL, "Neon plugin not found");
    TEST_ASSERT(theme_plugin_strcmp(neon->name, "Neon") == 0, "Neon name incorrect");
    
    // Test Retro plugin
    theme_plugin_t* retro = theme_plugin_get_retro();
    TEST_ASSERT(retro != NULL, "Retro plugin not found");
    TEST_ASSERT(theme_plugin_strcmp(retro->name, "Retro") == 0, "Retro name incorrect");
    
    // Test Matrix plugin
    theme_plugin_t* matrix = theme_plugin_get_matrix();
    TEST_ASSERT(matrix != NULL, "Matrix plugin not found");
    TEST_ASSERT(theme_plugin_strcmp(matrix->name, "Matrix") == 0, "Matrix name incorrect");
    
    // Load and get themes
    result = theme_plugin_load(neon);
    TEST_ASSERT(result == 0, "Neon load failed");
    
    const theme_t* neon_theme = neon->get_theme(neon);
    TEST_ASSERT(neon_theme != NULL, "Neon theme not provided");
    
    // Cleanup
    theme_plugins_examples_shutdown();
    
    printf("[PASS] Example Plugins\n");
    return 0;
}

int test_5d_renderer_init(void) {
    printf("[TEST] 5D Renderer Initialization\n");
    
    // Initialize renderer
    int result = gui_5d_renderer_init();
    TEST_ASSERT(result == 0, "5D renderer init failed");
    
    // Get context
    renderer_5d_context_t* ctx = gui_5d_renderer_get_context();
    TEST_ASSERT(ctx != NULL, "Context not available");
    TEST_ASSERT(ctx->mode == RENDER_MODE_STANDARD, "Default mode incorrect");
    TEST_ASSERT(ctx->enable_depth_fog == 1, "Depth fog should be enabled by default");
    
    // Test mode setting
    result = gui_5d_renderer_set_mode(RENDER_MODE_ENHANCED);
    TEST_ASSERT(result == 0, "Mode setting failed");
    TEST_ASSERT(ctx->mode == RENDER_MODE_ENHANCED, "Mode not updated");
    
    // Test feature toggles
    gui_5d_renderer_enable_depth_fog(0);
    TEST_ASSERT(ctx->enable_depth_fog == 0, "Depth fog not disabled");
    
    gui_5d_renderer_enable_volumetric_lighting(1);
    TEST_ASSERT(ctx->enable_volumetric_lighting == 1, "Volumetric lighting not enabled");
    
    // Test depth of field
    gui_5d_renderer_set_depth_of_field(0.5f);
    TEST_ASSERT(ctx->depth_of_field == 0.5f, "Depth of field not set");
    
    // Shutdown
    gui_5d_renderer_shutdown();
    
    printf("[PASS] 5D Renderer Initialization\n");
    return 0;
}

int test_5d_renderer_layers(void) {
    printf("[TEST] 5D Renderer Layers\n");
    
    // Initialize renderer
    gui_5d_renderer_init();
    
    // Create test layer
    render_layer_5d_t layer = {
        .x = 100,
        .y = 100,
        .width = 200,
        .height = 150,
        .depth = 0.5f,
        .opacity = 0.8f,
        .tint = {255, 255, 255, 255},
        .content = NULL
    };
    
    // Render layer
    int result = gui_5d_renderer_render_layer(&layer);
    TEST_ASSERT(result == 0, "Layer rendering failed");
    
    // Test with different depth values
    layer.depth = 0.0f;
    result = gui_5d_renderer_render_layer(&layer);
    TEST_ASSERT(result == 0, "Front layer rendering failed");
    
    layer.depth = 1.0f;
    result = gui_5d_renderer_render_layer(&layer);
    TEST_ASSERT(result == 0, "Back layer rendering failed");
    
    // Shutdown
    gui_5d_renderer_shutdown();
    
    printf("[PASS] 5D Renderer Layers\n");
    return 0;
}

int test_5d_renderer_plugin_integration(void) {
    printf("[TEST] 5D Renderer Plugin Integration\n");
    
    // Initialize systems
    theme_plugin_system_init();
    gui_5d_renderer_init();
    theme_plugins_examples_init();
    
    // Activate a plugin
    theme_plugin_t* neon = theme_plugin_get_neon();
    int result = theme_plugin_activate(neon);
    TEST_ASSERT(result == 0, "Plugin activation failed");
    
    // Set renderer to enhanced mode
    result = gui_5d_renderer_set_mode(RENDER_MODE_ENHANCED);
    TEST_ASSERT(result == 0, "Mode setting failed");
    
    // Render plugin effects
    result = gui_5d_render_plugin_effects(100, 100, 200, 150, NULL);
    TEST_ASSERT(result == 0, "Plugin effect rendering failed");
    
    // Cleanup
    theme_plugins_examples_shutdown();
    gui_5d_renderer_shutdown();
    
    printf("[PASS] 5D Renderer Plugin Integration\n");
    return 0;
}

int theme_plugin_tests_run_all(void) {
    printf("\n=== Theme Plugin System Tests ===\n\n");
    
    test_passed = 0;
    test_failed = 0;
    
    // Run all tests
    test_plugin_registration();
    test_plugin_loading();
    test_plugin_activation();
    test_plugin_effects();
    test_example_plugins();
    test_5d_renderer_init();
    test_5d_renderer_layers();
    test_5d_renderer_plugin_integration();
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Total:  %d\n", test_passed + test_failed);
    
    return test_failed;
}
