/**
 * Aurora OS - Display Manager
 * 
 * Comprehensive display management system with:
 * - Multiple resolution switching at runtime
 * - EDID parsing for optimal resolution detection
 * - Direct framebuffer mapping through paging
 * - Support for multiple displays
 * - Hardware acceleration hooks
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

/* Maximum number of supported displays */
#define MAX_DISPLAYS 8

/* Maximum number of video modes per display */
#define MAX_VIDEO_MODES 32

/* EDID constants */
#define EDID_BLOCK_SIZE 128
#define EDID_EXTENSION_BLOCKS_MAX 4

/* Display connection status */
typedef enum {
    DISPLAY_STATUS_DISCONNECTED = 0,
    DISPLAY_STATUS_CONNECTED = 1,
    DISPLAY_STATUS_ACTIVE = 2,
    DISPLAY_STATUS_ERROR = 3
} display_status_t;

/* Video mode structure */
typedef struct {
    uint32_t width;           // Width in pixels
    uint32_t height;          // Height in pixels
    uint32_t refresh_rate;    // Refresh rate in Hz
    uint8_t bits_per_pixel;   // Color depth
    uint32_t pitch;           // Bytes per scanline
    uint8_t interlaced;       // Interlaced mode flag
    uint32_t pixel_clock;     // Pixel clock in kHz
} video_mode_t;

/* EDID detailed timing descriptor */
typedef struct {
    uint32_t pixel_clock;     // Pixel clock in 10kHz units
    uint16_t h_active;        // Horizontal active pixels
    uint16_t h_blanking;      // Horizontal blanking pixels
    uint16_t v_active;        // Vertical active lines
    uint16_t v_blanking;      // Vertical blanking lines
    uint16_t h_sync_offset;   // Horizontal sync offset
    uint16_t h_sync_width;    // Horizontal sync pulse width
    uint16_t v_sync_offset;   // Vertical sync offset
    uint16_t v_sync_width;    // Vertical sync pulse width
    uint8_t interlaced;       // Interlaced flag
} edid_timing_t;

/* EDID information structure */
typedef struct {
    // Header and basic info
    uint8_t manufacturer_id[2];
    uint16_t product_code;
    uint32_t serial_number;
    uint8_t manufacture_week;
    uint16_t manufacture_year;
    uint8_t edid_version;
    uint8_t edid_revision;
    
    // Display characteristics
    uint8_t video_input_definition;
    uint8_t max_horizontal_size_cm;
    uint8_t max_vertical_size_cm;
    uint8_t display_gamma;
    uint8_t feature_support;
    
    // Supported timings
    uint8_t established_timings[3];
    uint16_t standard_timings[8];
    
    // Detailed timings
    edid_timing_t detailed_timings[4];
    
    // Preferred mode (typically first detailed timing)
    video_mode_t preferred_mode;
    
    // Supported modes derived from EDID
    uint8_t mode_count;
    video_mode_t supported_modes[MAX_VIDEO_MODES];
    
    // Raw EDID data
    uint8_t raw_data[EDID_BLOCK_SIZE];
    uint8_t extension_count;
    uint8_t extensions[EDID_EXTENSION_BLOCKS_MAX][EDID_BLOCK_SIZE];
} edid_info_t;

/* Framebuffer mapping information */
typedef struct {
    uint32_t physical_address;    // Physical framebuffer address
    uint32_t virtual_address;     // Virtual mapped address
    uint32_t size;                // Framebuffer size in bytes
    uint8_t cache_type;           // Cache policy (write-through, write-combine, etc.)
    uint8_t mapped;               // Mapping status
} fb_mapping_t;

/* Hardware acceleration capabilities */
typedef struct {
    uint8_t has_2d_accel;         // 2D acceleration available
    uint8_t has_3d_accel;         // 3D acceleration available
    uint8_t has_video_accel;      // Video decode/encode acceleration
    uint8_t has_dma;              // DMA support
    uint32_t vram_size;           // Video RAM size in bytes
    uint32_t max_texture_size;    // Maximum texture dimension
    uint8_t hw_cursor;            // Hardware cursor support
    uint8_t hw_overlay;           // Hardware overlay support
} hw_accel_caps_t;

/* Hardware acceleration interface */
typedef struct {
    // Function pointers for hardware acceleration
    int (*init)(void);
    void (*cleanup)(void);
    
    // 2D operations
    int (*fill_rect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    int (*blit)(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, 
                uint32_t w, uint32_t h);
    int (*stretch_blit)(uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h,
                        uint32_t dst_x, uint32_t dst_y, uint32_t dst_w, uint32_t dst_h);
    
    // DMA operations
    int (*dma_transfer)(void* src, void* dst, uint32_t size);
    int (*wait_for_dma)(void);
    
    // Command buffer
    void* (*alloc_command_buffer)(uint32_t size);
    int (*submit_command_buffer)(void* buffer, uint32_t size);
    void (*free_command_buffer)(void* buffer);
    
    // Synchronization
    int (*wait_for_idle)(void);
    int (*wait_for_vblank)(void);
} hw_accel_interface_t;

/* Display device structure */
typedef struct {
    uint8_t display_id;           // Display identifier
    display_status_t status;      // Connection status
    char name[32];                // Display name
    
    // EDID information
    edid_info_t edid;
    uint8_t edid_valid;           // EDID parsing status
    
    // Current configuration
    video_mode_t current_mode;
    uint8_t mode_active;
    
    // Framebuffer
    fb_mapping_t framebuffer;
    
    // Hardware capabilities
    hw_accel_caps_t hw_caps;
    hw_accel_interface_t* hw_interface;
    
    // Display properties
    uint8_t is_primary;           // Primary display flag
    int32_t position_x;           // Position in virtual desktop
    int32_t position_y;           // Position in virtual desktop
    uint16_t rotation;            // Display rotation (0, 90, 180, 270)
    uint8_t enabled;              // Display enabled flag
} display_device_t;

/* Display manager context */
typedef struct {
    uint8_t display_count;        // Number of detected displays
    display_device_t displays[MAX_DISPLAYS];
    uint8_t primary_display_id;   // Primary display identifier
    uint8_t initialized;          // Manager initialization status
} display_manager_t;

/**
 * Initialize the display manager
 * @return 0 on success, -1 on failure
 */
int display_manager_init(void);

/**
 * Shutdown the display manager
 */
void display_manager_shutdown(void);

/**
 * Get the display manager instance
 * @return Pointer to display manager context
 */
display_manager_t* display_manager_get(void);

/**
 * Detect and enumerate connected displays
 * @return Number of displays detected, or -1 on error
 */
int display_manager_detect_displays(void);

/**
 * Get display by ID
 * @param display_id Display identifier
 * @return Pointer to display device, or NULL if not found
 */
display_device_t* display_manager_get_display(uint8_t display_id);

/**
 * Get primary display
 * @return Pointer to primary display device, or NULL if none
 */
display_device_t* display_manager_get_primary_display(void);

/**
 * Set primary display
 * @param display_id Display identifier to set as primary
 * @return 0 on success, -1 on failure
 */
int display_manager_set_primary_display(uint8_t display_id);

/**
 * Read and parse EDID from display
 * @param display_id Display identifier
 * @return 0 on success, -1 on failure
 */
int display_read_edid(uint8_t display_id);

/**
 * Parse raw EDID data
 * @param edid Pointer to EDID structure to populate
 * @param raw_data Raw EDID data buffer
 * @param size Size of raw data
 * @return 0 on success, -1 on failure
 */
int display_parse_edid(edid_info_t* edid, const uint8_t* raw_data, uint32_t size);

/**
 * Get supported video modes for a display
 * @param display_id Display identifier
 * @param modes Array to store video modes
 * @param max_modes Maximum number of modes to return
 * @param count Output parameter for actual number of modes
 * @return 0 on success, -1 on failure
 */
int display_get_supported_modes(uint8_t display_id, video_mode_t* modes, 
                                uint8_t max_modes, uint8_t* count);

/**
 * Get preferred video mode from EDID
 * @param display_id Display identifier
 * @param mode Output parameter for preferred mode
 * @return 0 on success, -1 on failure
 */
int display_get_preferred_mode(uint8_t display_id, video_mode_t* mode);

/**
 * Set video mode for a display (runtime resolution switching)
 * @param display_id Display identifier
 * @param mode Video mode to set
 * @return 0 on success, -1 on failure
 */
int display_set_mode(uint8_t display_id, const video_mode_t* mode);

/**
 * Get current video mode for a display
 * @param display_id Display identifier
 * @param mode Output parameter for current mode
 * @return 0 on success, -1 on failure
 */
int display_get_current_mode(uint8_t display_id, video_mode_t* mode);

/**
 * Map framebuffer to virtual memory with proper caching
 * @param display_id Display identifier
 * @param physical_addr Physical framebuffer address
 * @param size Framebuffer size in bytes
 * @param cache_type Cache policy
 * @return Virtual address of mapped framebuffer, or 0 on failure
 */
uint32_t display_map_framebuffer(uint8_t display_id, uint32_t physical_addr, 
                                 uint32_t size, uint8_t cache_type);

/**
 * Unmap framebuffer from virtual memory
 * @param display_id Display identifier
 * @return 0 on success, -1 on failure
 */
int display_unmap_framebuffer(uint8_t display_id);

/**
 * Get framebuffer mapping information
 * @param display_id Display identifier
 * @param mapping Output parameter for mapping info
 * @return 0 on success, -1 on failure
 */
int display_get_framebuffer_mapping(uint8_t display_id, fb_mapping_t* mapping);

/**
 * Enable display
 * @param display_id Display identifier
 * @return 0 on success, -1 on failure
 */
int display_enable(uint8_t display_id);

/**
 * Disable display
 * @param display_id Display identifier
 * @return 0 on success, -1 on failure
 */
int display_disable(uint8_t display_id);

/**
 * Set display position in virtual desktop
 * @param display_id Display identifier
 * @param x X position
 * @param y Y position
 * @return 0 on success, -1 on failure
 */
int display_set_position(uint8_t display_id, int32_t x, int32_t y);

/**
 * Set display rotation
 * @param display_id Display identifier
 * @param rotation Rotation angle (0, 90, 180, 270)
 * @return 0 on success, -1 on failure
 */
int display_set_rotation(uint8_t display_id, uint16_t rotation);

/**
 * Register hardware acceleration interface
 * @param display_id Display identifier
 * @param interface Pointer to hardware acceleration interface
 * @return 0 on success, -1 on failure
 */
int display_register_hw_accel(uint8_t display_id, hw_accel_interface_t* interface);

/**
 * Get hardware acceleration capabilities
 * @param display_id Display identifier
 * @param caps Output parameter for capabilities
 * @return 0 on success, -1 on failure
 */
int display_get_hw_caps(uint8_t display_id, hw_accel_caps_t* caps);

/**
 * Perform hardware-accelerated fill rectangle
 * @param display_id Display identifier
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Width
 * @param height Height
 * @param color Color value
 * @return 0 on success, -1 on failure (falls back to software)
 */
int display_hw_fill_rect(uint8_t display_id, uint32_t x, uint32_t y, 
                         uint32_t width, uint32_t height, uint32_t color);

/**
 * Perform hardware-accelerated blit operation
 * @param display_id Display identifier
 * @param src_x Source X coordinate
 * @param src_y Source Y coordinate
 * @param dst_x Destination X coordinate
 * @param dst_y Destination Y coordinate
 * @param width Width
 * @param height Height
 * @return 0 on success, -1 on failure (falls back to software)
 */
int display_hw_blit(uint8_t display_id, uint32_t src_x, uint32_t src_y,
                    uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height);

/**
 * Wait for vertical blanking interval
 * @param display_id Display identifier
 * @return 0 on success, -1 on failure
 */
int display_wait_vblank(uint8_t display_id);

/**
 * Handle display hotplug event
 * @param display_id Display identifier
 * @param connected 1 if connected, 0 if disconnected
 */
void display_hotplug_handler(uint8_t display_id, uint8_t connected);

/* Cache types for framebuffer mapping */
#define FB_CACHE_DISABLED       0  // No caching (UC)
#define FB_CACHE_WRITE_THROUGH  1  // Write-through (WT)
#define FB_CACHE_WRITE_BACK     2  // Write-back (WB)
#define FB_CACHE_WRITE_COMBINE  3  // Write-combining (WC) - optimal for framebuffers

/* Common video mode presets */
extern const video_mode_t VIDEO_MODE_640x480_60;
extern const video_mode_t VIDEO_MODE_800x600_60;
extern const video_mode_t VIDEO_MODE_1024x768_60;
extern const video_mode_t VIDEO_MODE_1280x720_60;
extern const video_mode_t VIDEO_MODE_1280x1024_60;
extern const video_mode_t VIDEO_MODE_1920x1080_60;
extern const video_mode_t VIDEO_MODE_2560x1440_60;
extern const video_mode_t VIDEO_MODE_3840x2160_60;

#endif // DISPLAY_MANAGER_H
