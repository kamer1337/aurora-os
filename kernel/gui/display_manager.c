/**
 * Aurora OS - Display Manager Implementation
 * 
 * Comprehensive display management system implementation
 */

#include "display_manager.h"
#include "../memory/paging.h"
#include "../core/port_io.h"
#include <stddef.h>

/* Global display manager instance */
static display_manager_t display_manager = {0};

/* Common video mode presets */
const video_mode_t VIDEO_MODE_640x480_60 = {
    .width = 640, .height = 480, .refresh_rate = 60, 
    .bits_per_pixel = 32, .pitch = 640 * 4, .interlaced = 0, .pixel_clock = 25175
};

const video_mode_t VIDEO_MODE_800x600_60 = {
    .width = 800, .height = 600, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 800 * 4, .interlaced = 0, .pixel_clock = 40000
};

const video_mode_t VIDEO_MODE_1024x768_60 = {
    .width = 1024, .height = 768, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 1024 * 4, .interlaced = 0, .pixel_clock = 65000
};

const video_mode_t VIDEO_MODE_1280x720_60 = {
    .width = 1280, .height = 720, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 1280 * 4, .interlaced = 0, .pixel_clock = 74250
};

const video_mode_t VIDEO_MODE_1280x1024_60 = {
    .width = 1280, .height = 1024, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 1280 * 4, .interlaced = 0, .pixel_clock = 108000
};

const video_mode_t VIDEO_MODE_1920x1080_60 = {
    .width = 1920, .height = 1080, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 1920 * 4, .interlaced = 0, .pixel_clock = 148500
};

const video_mode_t VIDEO_MODE_2560x1440_60 = {
    .width = 2560, .height = 1440, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 2560 * 4, .interlaced = 0, .pixel_clock = 241500
};

const video_mode_t VIDEO_MODE_3840x2160_60 = {
    .width = 3840, .height = 2160, .refresh_rate = 60,
    .bits_per_pixel = 32, .pitch = 3840 * 4, .interlaced = 0, .pixel_clock = 533280
};

/* Helper function to copy memory */
static void memcpy_internal(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* Helper function to set memory */
static void memset_internal(void* dest, uint8_t val, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = val;
    }
}

/**
 * Initialize the display manager
 */
int display_manager_init(void) {
    if (display_manager.initialized) {
        return 0; // Already initialized
    }
    
    // Clear display manager structure
    memset_internal(&display_manager, 0, sizeof(display_manager_t));
    
    // Detect connected displays
    int detected = display_manager_detect_displays();
    if (detected < 0) {
        return -1;
    }
    
    display_manager.initialized = 1;
    return 0;
}

/**
 * Shutdown the display manager
 */
void display_manager_shutdown(void) {
    if (!display_manager.initialized) {
        return;
    }
    
    // Unmap all framebuffers
    for (uint8_t i = 0; i < display_manager.display_count; i++) {
        display_unmap_framebuffer(i);
    }
    
    display_manager.initialized = 0;
}

/**
 * Get the display manager instance
 */
display_manager_t* display_manager_get(void) {
    return &display_manager;
}

/**
 * Detect and enumerate connected displays
 */
int display_manager_detect_displays(void) {
    // Start with at least one display (primary)
    display_manager.display_count = 1;
    display_manager.primary_display_id = 0;
    
    // Initialize primary display
    display_device_t* primary = &display_manager.displays[0];
    primary->display_id = 0;
    primary->status = DISPLAY_STATUS_CONNECTED;
    primary->is_primary = 1;
    primary->enabled = 1;
    primary->position_x = 0;
    primary->position_y = 0;
    primary->rotation = 0;
    
    // Set default mode to 1920x1080@60
    memcpy_internal(&primary->current_mode, &VIDEO_MODE_1920x1080_60, sizeof(video_mode_t));
    primary->mode_active = 1;
    
    // Try to read EDID for primary display
    display_read_edid(0);
    
    // Detect additional displays (stub for now - can be extended with PCI enumeration)
    // In a real implementation, this would scan PCI bus for additional graphics adapters
    
    return display_manager.display_count;
}

/**
 * Get display by ID
 */
display_device_t* display_manager_get_display(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return NULL;
    }
    
    return &display_manager.displays[display_id];
}

/**
 * Get primary display
 */
display_device_t* display_manager_get_primary_display(void) {
    return display_manager_get_display(display_manager.primary_display_id);
}

/**
 * Set primary display
 */
int display_manager_set_primary_display(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    // Unmark old primary
    display_manager.displays[display_manager.primary_display_id].is_primary = 0;
    
    // Set new primary
    display_manager.primary_display_id = display_id;
    display_manager.displays[display_id].is_primary = 1;
    
    return 0;
}

/**
 * Read and parse EDID from display
 */
int display_read_edid(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    // In a real implementation, this would:
    // 1. Access DDC/I2C bus to read EDID
    // 2. Use VESA VBE DDC functions or direct I2C access
    // 3. Read 128 bytes of EDID data
    
    // For now, we'll populate some default EDID data
    uint8_t default_edid[EDID_BLOCK_SIZE] = {
        0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, // Header
        0x10, 0xAC,                                     // Manufacturer ID (Dell)
        0x01, 0x00,                                     // Product code
        0x00, 0x00, 0x00, 0x00,                        // Serial number
        0x01, 0x1E,                                     // Week 1, Year 2020
        0x01, 0x04,                                     // EDID version 1.4
        // Rest would be filled with timing info, etc.
    };
    
    return display_parse_edid(&display->edid, default_edid, EDID_BLOCK_SIZE);
}

/**
 * Parse raw EDID data
 */
int display_parse_edid(edid_info_t* edid, const uint8_t* raw_data, uint32_t size) {
    if (!edid || !raw_data || size < EDID_BLOCK_SIZE) {
        return -1;
    }
    
    // Copy raw data
    memcpy_internal(edid->raw_data, raw_data, EDID_BLOCK_SIZE);
    
    // Parse header (should be 00 FF FF FF FF FF FF 00)
    if (raw_data[0] != 0x00 || raw_data[1] != 0xFF) {
        return -1; // Invalid EDID
    }
    
    // Parse manufacturer ID
    edid->manufacturer_id[0] = raw_data[8];
    edid->manufacturer_id[1] = raw_data[9];
    
    // Parse product code
    edid->product_code = (raw_data[11] << 8) | raw_data[10];
    
    // Parse serial number
    edid->serial_number = (raw_data[15] << 24) | (raw_data[14] << 16) |
                          (raw_data[13] << 8) | raw_data[12];
    
    // Parse manufacture date
    edid->manufacture_week = raw_data[16];
    edid->manufacture_year = 1990 + raw_data[17];
    
    // Parse EDID version
    edid->edid_version = raw_data[18];
    edid->edid_revision = raw_data[19];
    
    // Parse video input definition
    edid->video_input_definition = raw_data[20];
    
    // Parse screen size
    edid->max_horizontal_size_cm = raw_data[21];
    edid->max_vertical_size_cm = raw_data[22];
    
    // Parse gamma
    edid->display_gamma = raw_data[23];
    
    // Parse feature support
    edid->feature_support = raw_data[24];
    
    // Parse established timings
    edid->established_timings[0] = raw_data[35];
    edid->established_timings[1] = raw_data[36];
    edid->established_timings[2] = raw_data[37];
    
    // Parse standard timings
    for (int i = 0; i < 8; i++) {
        edid->standard_timings[i] = (raw_data[39 + i*2] << 8) | raw_data[38 + i*2];
    }
    
    // Parse detailed timing descriptors (4 descriptors starting at byte 54)
    for (int i = 0; i < 4; i++) {
        uint8_t* desc = (uint8_t*)&raw_data[54 + i * 18];
        
        if (desc[0] == 0 && desc[1] == 0) {
            continue; // Not a timing descriptor
        }
        
        edid_timing_t* timing = &edid->detailed_timings[i];
        
        // Parse pixel clock (in 10kHz units)
        timing->pixel_clock = (desc[1] << 8) | desc[0];
        
        // Parse horizontal active
        timing->h_active = desc[2] | ((desc[4] & 0xF0) << 4);
        
        // Parse horizontal blanking
        timing->h_blanking = desc[3] | ((desc[4] & 0x0F) << 8);
        
        // Parse vertical active
        timing->v_active = desc[5] | ((desc[7] & 0xF0) << 4);
        
        // Parse vertical blanking
        timing->v_blanking = desc[6] | ((desc[7] & 0x0F) << 8);
        
        // Parse sync info
        timing->h_sync_offset = desc[8] | ((desc[11] & 0xC0) << 2);
        timing->h_sync_width = desc[9] | ((desc[11] & 0x30) << 4);
        timing->v_sync_offset = ((desc[10] & 0xF0) >> 4) | ((desc[11] & 0x0C) << 2);
        timing->v_sync_width = (desc[10] & 0x0F) | ((desc[11] & 0x03) << 4);
        
        // Check interlaced flag
        timing->interlaced = (desc[17] & 0x80) ? 1 : 0;
    }
    
    // Build supported modes list from EDID
    edid->mode_count = 0;
    
    // Add modes from established timings
    if (edid->established_timings[0] & 0x80) { // 800x600@60
        memcpy_internal(&edid->supported_modes[edid->mode_count++], 
                       &VIDEO_MODE_800x600_60, sizeof(video_mode_t));
    }
    if (edid->established_timings[0] & 0x08) { // 1024x768@60
        memcpy_internal(&edid->supported_modes[edid->mode_count++],
                       &VIDEO_MODE_1024x768_60, sizeof(video_mode_t));
    }
    
    // Add common modes
    if (edid->mode_count < MAX_VIDEO_MODES) {
        memcpy_internal(&edid->supported_modes[edid->mode_count++],
                       &VIDEO_MODE_1920x1080_60, sizeof(video_mode_t));
    }
    if (edid->mode_count < MAX_VIDEO_MODES) {
        memcpy_internal(&edid->supported_modes[edid->mode_count++],
                       &VIDEO_MODE_1280x720_60, sizeof(video_mode_t));
    }
    
    // Set preferred mode (typically first detailed timing)
    if (edid->detailed_timings[0].pixel_clock > 0) {
        edid->preferred_mode.width = edid->detailed_timings[0].h_active;
        edid->preferred_mode.height = edid->detailed_timings[0].v_active;
        edid->preferred_mode.refresh_rate = 60; // Default
        edid->preferred_mode.bits_per_pixel = 32;
        edid->preferred_mode.pitch = edid->detailed_timings[0].h_active * 4;
        edid->preferred_mode.interlaced = edid->detailed_timings[0].interlaced;
        edid->preferred_mode.pixel_clock = edid->detailed_timings[0].pixel_clock * 10;
    } else {
        // Default to 1920x1080@60
        memcpy_internal(&edid->preferred_mode, &VIDEO_MODE_1920x1080_60, sizeof(video_mode_t));
    }
    
    return 0;
}

/**
 * Get supported video modes for a display
 */
int display_get_supported_modes(uint8_t display_id, video_mode_t* modes,
                                uint8_t max_modes, uint8_t* count) {
    if (display_id >= display_manager.display_count || !modes || !count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    if (display->edid_valid) {
        // Return modes from EDID
        *count = display->edid.mode_count < max_modes ? 
                 display->edid.mode_count : max_modes;
        
        for (uint8_t i = 0; i < *count; i++) {
            memcpy_internal(&modes[i], &display->edid.supported_modes[i], 
                          sizeof(video_mode_t));
        }
    } else {
        // Return common modes
        *count = 0;
        if (*count < max_modes) {
            memcpy_internal(&modes[(*count)++], &VIDEO_MODE_640x480_60, sizeof(video_mode_t));
        }
        if (*count < max_modes) {
            memcpy_internal(&modes[(*count)++], &VIDEO_MODE_800x600_60, sizeof(video_mode_t));
        }
        if (*count < max_modes) {
            memcpy_internal(&modes[(*count)++], &VIDEO_MODE_1024x768_60, sizeof(video_mode_t));
        }
        if (*count < max_modes) {
            memcpy_internal(&modes[(*count)++], &VIDEO_MODE_1280x720_60, sizeof(video_mode_t));
        }
        if (*count < max_modes) {
            memcpy_internal(&modes[(*count)++], &VIDEO_MODE_1920x1080_60, sizeof(video_mode_t));
        }
    }
    
    return 0;
}

/**
 * Get preferred video mode from EDID
 */
int display_get_preferred_mode(uint8_t display_id, video_mode_t* mode) {
    if (display_id >= display_manager.display_count || !mode) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    if (display->edid_valid) {
        memcpy_internal(mode, &display->edid.preferred_mode, sizeof(video_mode_t));
    } else {
        // Default to 1920x1080@60
        memcpy_internal(mode, &VIDEO_MODE_1920x1080_60, sizeof(video_mode_t));
    }
    
    return 0;
}

/**
 * Set video mode for a display (runtime resolution switching)
 */
int display_set_mode(uint8_t display_id, const video_mode_t* mode) {
    if (display_id >= display_manager.display_count || !mode) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    // In a real implementation, this would:
    // 1. Use VESA VBE to switch modes
    // 2. Or use native GPU driver mode-setting
    // 3. Update CRTC registers
    // 4. Reconfigure display timings
    
    // For now, just update the mode structure
    memcpy_internal(&display->current_mode, mode, sizeof(video_mode_t));
    display->mode_active = 1;
    
    // Update framebuffer size if mapped
    if (display->framebuffer.mapped) {
        uint32_t new_size = mode->pitch * mode->height;
        if (new_size != display->framebuffer.size) {
            // Remap framebuffer with new size
            display_unmap_framebuffer(display_id);
            display_map_framebuffer(display_id, display->framebuffer.physical_address,
                                   new_size, display->framebuffer.cache_type);
        }
    }
    
    return 0;
}

/**
 * Get current video mode for a display
 */
int display_get_current_mode(uint8_t display_id, video_mode_t* mode) {
    if (display_id >= display_manager.display_count || !mode) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    memcpy_internal(mode, &display->current_mode, sizeof(video_mode_t));
    
    return 0;
}

/**
 * Map framebuffer to virtual memory with proper caching
 */
uint32_t display_map_framebuffer(uint8_t display_id, uint32_t physical_addr,
                                 uint32_t size, uint8_t cache_type) {
    if (display_id >= display_manager.display_count) {
        return 0;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    // In a real implementation, this would:
    // 1. Allocate virtual address space
    // 2. Create page table entries with appropriate caching attributes
    // 3. Map physical framebuffer to virtual address
    // 4. Set PAT (Page Attribute Table) for write-combining if requested
    
    // For now, use identity mapping (physical == virtual for framebuffer region)
    uint32_t virtual_addr = physical_addr;
    
    display->framebuffer.physical_address = physical_addr;
    display->framebuffer.virtual_address = virtual_addr;
    display->framebuffer.size = size;
    display->framebuffer.cache_type = cache_type;
    display->framebuffer.mapped = 1;
    
    return virtual_addr;
}

/**
 * Unmap framebuffer from virtual memory
 */
int display_unmap_framebuffer(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    if (!display->framebuffer.mapped) {
        return 0; // Already unmapped
    }
    
    // In a real implementation, this would:
    // 1. Remove page table entries
    // 2. Free virtual address space
    // 3. Flush TLB
    
    display->framebuffer.mapped = 0;
    display->framebuffer.virtual_address = 0;
    
    return 0;
}

/**
 * Get framebuffer mapping information
 */
int display_get_framebuffer_mapping(uint8_t display_id, fb_mapping_t* mapping) {
    if (display_id >= display_manager.display_count || !mapping) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    memcpy_internal(mapping, &display->framebuffer, sizeof(fb_mapping_t));
    
    return 0;
}

/**
 * Enable display
 */
int display_enable(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    display->enabled = 1;
    display->status = DISPLAY_STATUS_ACTIVE;
    
    return 0;
}

/**
 * Disable display
 */
int display_disable(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    display->enabled = 0;
    display->status = DISPLAY_STATUS_CONNECTED;
    
    return 0;
}

/**
 * Set display position in virtual desktop
 */
int display_set_position(uint8_t display_id, int32_t x, int32_t y) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    display->position_x = x;
    display->position_y = y;
    
    return 0;
}

/**
 * Set display rotation
 */
int display_set_rotation(uint8_t display_id, uint8_t rotation) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270) {
        return -1; // Invalid rotation
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    display->rotation = rotation;
    
    return 0;
}

/**
 * Register hardware acceleration interface
 */
int display_register_hw_accel(uint8_t display_id, hw_accel_interface_t* interface) {
    if (display_id >= display_manager.display_count || !interface) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    display->hw_interface = interface;
    
    // Initialize hardware acceleration if init function provided
    if (interface->init) {
        return interface->init();
    }
    
    return 0;
}

/**
 * Get hardware acceleration capabilities
 */
int display_get_hw_caps(uint8_t display_id, hw_accel_caps_t* caps) {
    if (display_id >= display_manager.display_count || !caps) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    memcpy_internal(caps, &display->hw_caps, sizeof(hw_accel_caps_t));
    
    return 0;
}

/**
 * Perform hardware-accelerated fill rectangle
 */
int display_hw_fill_rect(uint8_t display_id, uint32_t x, uint32_t y,
                         uint32_t width, uint32_t height, uint32_t color) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    if (display->hw_interface && display->hw_interface->fill_rect) {
        return display->hw_interface->fill_rect(x, y, width, height, color);
    }
    
    return -1; // No hardware acceleration available
}

/**
 * Perform hardware-accelerated blit operation
 */
int display_hw_blit(uint8_t display_id, uint32_t src_x, uint32_t src_y,
                    uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    if (display->hw_interface && display->hw_interface->blit) {
        return display->hw_interface->blit(src_x, src_y, dst_x, dst_y, width, height);
    }
    
    return -1; // No hardware acceleration available
}

/**
 * Wait for vertical blanking interval
 */
int display_wait_vblank(uint8_t display_id) {
    if (display_id >= display_manager.display_count) {
        return -1;
    }
    
    display_device_t* display = &display_manager.displays[display_id];
    
    if (display->hw_interface && display->hw_interface->wait_for_vblank) {
        return display->hw_interface->wait_for_vblank();
    }
    
    // Fallback: basic delay
    // In a real implementation, this would wait for VGA/display port vblank signal
    for (volatile int i = 0; i < 10000; i++) {
        // Wait
    }
    
    return 0;
}

/**
 * Handle display hotplug event
 */
void display_hotplug_handler(uint8_t display_id, uint8_t connected) {
    if (display_id >= MAX_DISPLAYS) {
        return;
    }
    
    if (connected) {
        // Display connected
        if (display_id >= display_manager.display_count) {
            // New display
            display_manager.display_count = display_id + 1;
        }
        
        display_device_t* display = &display_manager.displays[display_id];
        display->display_id = display_id;
        display->status = DISPLAY_STATUS_CONNECTED;
        display->enabled = 1;
        
        // Try to read EDID
        display_read_edid(display_id);
        
        // Set default mode
        display_get_preferred_mode(display_id, &display->current_mode);
        display->mode_active = 1;
    } else {
        // Display disconnected
        if (display_id < display_manager.display_count) {
            display_device_t* display = &display_manager.displays[display_id];
            display->status = DISPLAY_STATUS_DISCONNECTED;
            display->enabled = 0;
            
            // Unmap framebuffer
            display_unmap_framebuffer(display_id);
        }
    }
}
