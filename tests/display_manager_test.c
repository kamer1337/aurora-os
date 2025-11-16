/**
 * Aurora OS - Display Manager Test
 * 
 * Tests for the display manager functionality including:
 * - Resolution switching
 * - EDID parsing
 * - Multiple display support
 * - Framebuffer mapping
 * - Hardware acceleration hooks
 */

#include "../kernel/gui/display_manager.h"
#include <stddef.h>

// Simple test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    if (condition) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
    }

/**
 * Test display manager initialization
 */
static void test_display_manager_init(void) {
    // Initialize display manager
    int result = display_manager_init();
    TEST_ASSERT(result == 0, "Display manager initialization");
    
    // Get display manager instance
    display_manager_t* dm = display_manager_get();
    TEST_ASSERT(dm != NULL, "Display manager instance retrieval");
    TEST_ASSERT(dm->initialized == 1, "Display manager initialized flag");
    TEST_ASSERT(dm->display_count > 0, "At least one display detected");
}

/**
 * Test EDID parsing
 */
static void test_edid_parsing(void) {
    // Get primary display
    display_device_t* display = display_manager_get_primary_display();
    TEST_ASSERT(display != NULL, "Primary display retrieval");
    
    // Check if EDID was read
    int result = display_read_edid(0);
    TEST_ASSERT(result == 0, "EDID reading");
    
    // Get preferred mode from EDID
    video_mode_t preferred_mode;
    result = display_get_preferred_mode(0, &preferred_mode);
    TEST_ASSERT(result == 0, "Preferred mode retrieval");
    TEST_ASSERT(preferred_mode.width > 0, "Preferred mode has valid width");
    TEST_ASSERT(preferred_mode.height > 0, "Preferred mode has valid height");
}

/**
 * Test video mode enumeration and switching
 */
static void test_video_modes(void) {
    video_mode_t modes[MAX_VIDEO_MODES];
    uint8_t mode_count = 0;
    
    // Get supported modes
    int result = display_get_supported_modes(0, modes, MAX_VIDEO_MODES, &mode_count);
    TEST_ASSERT(result == 0, "Supported modes retrieval");
    TEST_ASSERT(mode_count > 0, "At least one mode available");
    
    // Get current mode
    video_mode_t current_mode;
    result = display_get_current_mode(0, &current_mode);
    TEST_ASSERT(result == 0, "Current mode retrieval");
    
    // Test mode switching to 1280x720
    result = display_set_mode(0, &VIDEO_MODE_1280x720_60);
    TEST_ASSERT(result == 0, "Mode switching to 1280x720@60");
    
    // Verify mode was set
    result = display_get_current_mode(0, &current_mode);
    TEST_ASSERT(result == 0, "Mode retrieval after switch");
    TEST_ASSERT(current_mode.width == 1280, "Mode width is 1280");
    TEST_ASSERT(current_mode.height == 720, "Mode height is 720");
    
    // Switch back to 1920x1080
    result = display_set_mode(0, &VIDEO_MODE_1920x1080_60);
    TEST_ASSERT(result == 0, "Mode switching to 1920x1080@60");
}

/**
 * Test framebuffer mapping
 */
static void test_framebuffer_mapping(void) {
    // Map framebuffer with write-combining cache
    uint32_t phys_addr = 0xE0000000;
    uint32_t size = 1920 * 1080 * 4;
    
    uint32_t virt_addr = display_map_framebuffer(0, phys_addr, size, FB_CACHE_WRITE_COMBINE);
    TEST_ASSERT(virt_addr != 0, "Framebuffer mapping");
    
    // Get mapping info
    fb_mapping_t mapping;
    int result = display_get_framebuffer_mapping(0, &mapping);
    TEST_ASSERT(result == 0, "Framebuffer mapping info retrieval");
    TEST_ASSERT(mapping.mapped == 1, "Framebuffer is mapped");
    TEST_ASSERT(mapping.physical_address == phys_addr, "Physical address matches");
    TEST_ASSERT(mapping.size == size, "Size matches");
    TEST_ASSERT(mapping.cache_type == FB_CACHE_WRITE_COMBINE, "Cache type is write-combine");
    
    // Unmap framebuffer
    result = display_unmap_framebuffer(0);
    TEST_ASSERT(result == 0, "Framebuffer unmapping");
    
    result = display_get_framebuffer_mapping(0, &mapping);
    TEST_ASSERT(result == 0, "Framebuffer mapping info after unmap");
    TEST_ASSERT(mapping.mapped == 0, "Framebuffer is unmapped");
}

/**
 * Test multiple display support
 */
static void test_multiple_displays(void) {
    display_manager_t* dm = display_manager_get();
    TEST_ASSERT(dm != NULL, "Display manager instance");
    
    // Get primary display
    display_device_t* primary = display_manager_get_primary_display();
    TEST_ASSERT(primary != NULL, "Primary display exists");
    TEST_ASSERT(primary->is_primary == 1, "Primary flag is set");
    
    // Test display positioning (for virtual desktop)
    int result = display_set_position(0, 0, 0);
    TEST_ASSERT(result == 0, "Set display position");
    
    // Test display rotation
    result = display_set_rotation(0, 0);
    TEST_ASSERT(result == 0, "Set display rotation to 0");
    
    result = display_set_rotation(0, 90);
    TEST_ASSERT(result == 0, "Set display rotation to 90");
    
    result = display_set_rotation(0, 180);
    TEST_ASSERT(result == 0, "Set display rotation to 180");
    
    result = display_set_rotation(0, 270);
    TEST_ASSERT(result == 0, "Set display rotation to 270");
    
    // Invalid rotation should fail
    result = display_set_rotation(0, 45);
    TEST_ASSERT(result == -1, "Invalid rotation angle rejected");
}

/**
 * Test hardware acceleration interface
 */
static void test_hardware_acceleration(void) {
    // Try hardware accelerated operations (will fail without actual hw accel)
    int result = display_hw_fill_rect(0, 0, 0, 100, 100, 0xFFFFFF);
    TEST_ASSERT(result == -1, "Hardware fill rect returns error without hw accel");
    
    result = display_hw_blit(0, 0, 0, 100, 100, 200, 200);
    TEST_ASSERT(result == -1, "Hardware blit returns error without hw accel");
    
    // Test wait for vblank (software fallback)
    result = display_wait_vblank(0);
    TEST_ASSERT(result == 0, "Wait for vblank (software fallback)");
}

/**
 * Test display enable/disable
 */
static void test_display_control(void) {
    display_device_t* display = display_manager_get_display(0);
    TEST_ASSERT(display != NULL, "Display retrieval");
    
    // Disable display
    int result = display_disable(0);
    TEST_ASSERT(result == 0, "Display disable");
    TEST_ASSERT(display->enabled == 0, "Display enabled flag is 0");
    TEST_ASSERT(display->status == DISPLAY_STATUS_CONNECTED, "Display status is connected");
    
    // Enable display
    result = display_enable(0);
    TEST_ASSERT(result == 0, "Display enable");
    TEST_ASSERT(display->enabled == 1, "Display enabled flag is 1");
    TEST_ASSERT(display->status == DISPLAY_STATUS_ACTIVE, "Display status is active");
}

/**
 * Test hotplug handling
 */
static void test_hotplug(void) {
    // Simulate hotplug disconnect
    display_hotplug_handler(0, 0);
    
    display_device_t* display = display_manager_get_display(0);
    TEST_ASSERT(display != NULL, "Display still exists after disconnect");
    TEST_ASSERT(display->status == DISPLAY_STATUS_DISCONNECTED, "Display status is disconnected");
    TEST_ASSERT(display->enabled == 0, "Display is disabled");
    
    // Simulate hotplug connect
    display_hotplug_handler(0, 1);
    TEST_ASSERT(display->status == DISPLAY_STATUS_CONNECTED, "Display status is connected");
    TEST_ASSERT(display->enabled == 1, "Display is enabled");
}

/**
 * Run all display manager tests
 */
void run_display_manager_tests(void) {
    tests_passed = 0;
    tests_failed = 0;
    
    test_display_manager_init();
    test_edid_parsing();
    test_video_modes();
    test_framebuffer_mapping();
    test_multiple_displays();
    test_hardware_acceleration();
    test_display_control();
    test_hotplug();
    
    // Cleanup
    display_manager_shutdown();
}

/**
 * Get test results
 */
void get_display_manager_test_results(int* passed, int* failed) {
    if (passed) *passed = tests_passed;
    if (failed) *failed = tests_failed;
}
