/**
 * Aurora OS - Theme Plugin System Tests
 * 
 * Tests for the plugin theme system and 5D renderer enhancements
 */

#ifndef THEME_PLUGIN_TESTS_H
#define THEME_PLUGIN_TESTS_H

#include <stdint.h>

/**
 * Run all theme plugin system tests
 * @return Number of tests failed (0 = all passed)
 */
int theme_plugin_tests_run_all(void);

/**
 * Test plugin registration and unregistration
 * @return 0 on success, -1 on failure
 */
int test_plugin_registration(void);

/**
 * Test plugin loading and unloading
 * @return 0 on success, -1 on failure
 */
int test_plugin_loading(void);

/**
 * Test plugin activation and theme application
 * @return 0 on success, -1 on failure
 */
int test_plugin_activation(void);

/**
 * Test plugin effect registration and rendering
 * @return 0 on success, -1 on failure
 */
int test_plugin_effects(void);

/**
 * Test example plugins (Neon, Retro, Matrix)
 * @return 0 on success, -1 on failure
 */
int test_example_plugins(void);

/**
 * Test 5D renderer initialization and configuration
 * @return 0 on success, -1 on failure
 */
int test_5d_renderer_init(void);

/**
 * Test 5D renderer layer rendering
 * @return 0 on success, -1 on failure
 */
int test_5d_renderer_layers(void);

/**
 * Test 5D renderer plugin integration
 * @return 0 on success, -1 on failure
 */
int test_5d_renderer_plugin_integration(void);

#endif // THEME_PLUGIN_TESTS_H
