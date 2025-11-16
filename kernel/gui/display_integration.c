/**
 * Aurora OS - Display Manager Integration Example
 * 
 * This example demonstrates how to integrate the display manager
 * with the existing framebuffer system.
 */

#include "display_manager.h"
#include "framebuffer.h"

/**
 * Initialize display system with display manager
 * 
 * This function should be called during system initialization
 * to set up the display manager and configure the framebuffer.
 */
int display_system_init(void) {
    // Initialize the display manager
    if (display_manager_init() != 0) {
        return -1;
    }
    
    // Get primary display
    display_device_t* primary = display_manager_get_primary_display();
    if (!primary) {
        return -1;
    }
    
    // Get preferred resolution from EDID
    video_mode_t preferred_mode;
    if (display_get_preferred_mode(primary->display_id, &preferred_mode) == 0) {
        // Set the preferred mode
        display_set_mode(primary->display_id, &preferred_mode);
    } else {
        // Fall back to 1920x1080@60 if EDID not available
        display_set_mode(primary->display_id, &VIDEO_MODE_1920x1080_60);
    }
    
    // Map the framebuffer with write-combining for optimal performance
    uint32_t fb_size = primary->current_mode.width * 
                       primary->current_mode.height * 
                       (primary->current_mode.bits_per_pixel / 8);
    
    uint32_t fb_phys_addr = 0xE0000000; // Typical framebuffer address
    uint32_t fb_virt_addr = display_map_framebuffer(primary->display_id, 
                                                     fb_phys_addr, 
                                                     fb_size, 
                                                     FB_CACHE_WRITE_COMBINE);
    
    if (fb_virt_addr == 0) {
        return -1;
    }
    
    // Initialize the framebuffer driver with the current mode
    framebuffer_init(primary->current_mode.width,
                    primary->current_mode.height,
                    primary->current_mode.bits_per_pixel);
    
    return 0;
}

/**
 * Switch display resolution at runtime
 * 
 * @param width Desired width
 * @param height Desired height
 * @param refresh_rate Desired refresh rate
 * @return 0 on success, -1 on failure
 */
int display_system_set_resolution(uint32_t width, uint32_t height, uint32_t refresh_rate) {
    display_device_t* primary = display_manager_get_primary_display();
    if (!primary) {
        return -1;
    }
    
    // Find matching video mode
    video_mode_t modes[MAX_VIDEO_MODES];
    uint8_t mode_count = 0;
    
    if (display_get_supported_modes(primary->display_id, modes, MAX_VIDEO_MODES, &mode_count) != 0) {
        return -1;
    }
    
    // Search for matching mode
    video_mode_t* target_mode = NULL;
    for (uint8_t i = 0; i < mode_count; i++) {
        if (modes[i].width == width && 
            modes[i].height == height && 
            modes[i].refresh_rate == refresh_rate) {
            target_mode = &modes[i];
            break;
        }
    }
    
    if (!target_mode) {
        return -1; // Mode not supported
    }
    
    // Unmap old framebuffer
    display_unmap_framebuffer(primary->display_id);
    
    // Set new mode
    if (display_set_mode(primary->display_id, target_mode) != 0) {
        return -1;
    }
    
    // Remap framebuffer with new size
    uint32_t fb_size = target_mode->width * target_mode->height * 
                       (target_mode->bits_per_pixel / 8);
    uint32_t fb_phys_addr = 0xE0000000;
    uint32_t fb_virt_addr = display_map_framebuffer(primary->display_id,
                                                     fb_phys_addr,
                                                     fb_size,
                                                     FB_CACHE_WRITE_COMBINE);
    
    if (fb_virt_addr == 0) {
        return -1;
    }
    
    // Reinitialize framebuffer with new resolution
    framebuffer_init(target_mode->width, target_mode->height, target_mode->bits_per_pixel);
    
    return 0;
}

/**
 * Get current display information
 */
int display_system_get_info(uint32_t* width, uint32_t* height, uint32_t* bpp) {
    display_device_t* primary = display_manager_get_primary_display();
    if (!primary) {
        return -1;
    }
    
    if (width) *width = primary->current_mode.width;
    if (height) *height = primary->current_mode.height;
    if (bpp) *bpp = primary->current_mode.bits_per_pixel;
    
    return 0;
}

/**
 * Enable hardware acceleration if available
 */
int display_system_enable_hw_accel(void) {
    display_device_t* primary = display_manager_get_primary_display();
    if (!primary) {
        return -1;
    }
    
    // Check if hardware acceleration is available
    hw_accel_caps_t caps;
    if (display_get_hw_caps(primary->display_id, &caps) != 0) {
        return -1;
    }
    
    if (!caps.has_2d_accel) {
        return -1; // No 2D acceleration available
    }
    
    // Hardware acceleration would be initialized here
    // by registering a hw_accel_interface_t with the display manager
    
    return 0;
}

/**
 * Example: Configure multi-monitor setup
 */
int display_system_setup_dual_monitors(void) {
    display_manager_t* dm = display_manager_get();
    if (!dm || dm->display_count < 2) {
        return -1; // Need at least 2 displays
    }
    
    // Configure first display (primary) at position (0, 0)
    display_set_position(0, 0, 0);
    display_enable(0);
    
    // Configure second display to the right of the first
    video_mode_t mode0, mode1;
    display_get_current_mode(0, &mode0);
    
    // Position second display to the right
    display_set_position(1, mode0.width, 0);
    
    // Set matching resolution for second display
    display_set_mode(1, &mode0);
    display_enable(1);
    
    return 0;
}

/**
 * Example: List all available resolutions
 */
void display_system_list_resolutions(void) {
    display_device_t* primary = display_manager_get_primary_display();
    if (!primary) {
        return;
    }
    
    video_mode_t modes[MAX_VIDEO_MODES];
    uint8_t mode_count = 0;
    
    if (display_get_supported_modes(primary->display_id, modes, MAX_VIDEO_MODES, &mode_count) != 0) {
        return;
    }
    
    // In a real implementation, this would print or display the modes
    // For now, just iterate through them
    for (uint8_t i = 0; i < mode_count; i++) {
        // Process mode: modes[i].width x modes[i].height @ modes[i].refresh_rate Hz
        (void)modes; // Suppress warning
    }
}
