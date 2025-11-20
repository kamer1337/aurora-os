/**
 * @file surfaceflinger.h
 * @brief SurfaceFlinger Display Compositor for Aurora OS
 * 
 * Provides Android SurfaceFlinger functionality for compositing
 * application surfaces and rendering to display
 */

#ifndef SURFACEFLINGER_H
#define SURFACEFLINGER_H

#include <stdint.h>
#include <stdbool.h>

/* Surface Types */
typedef enum {
    SURFACE_TYPE_NORMAL = 0,
    SURFACE_TYPE_PANEL,
    SURFACE_TYPE_WALLPAPER,
    SURFACE_TYPE_CURSOR
} surface_type_t;

/* Buffer Formats */
typedef enum {
    PIXEL_FORMAT_RGBA_8888 = 1,
    PIXEL_FORMAT_RGBX_8888 = 2,
    PIXEL_FORMAT_RGB_888 = 3,
    PIXEL_FORMAT_RGB_565 = 4,
    PIXEL_FORMAT_BGRA_8888 = 5,
    PIXEL_FORMAT_RGBA_5551 = 6,
    PIXEL_FORMAT_RGBA_4444 = 7
} pixel_format_t;

/* Blend Modes */
typedef enum {
    BLEND_MODE_NONE = 0,
    BLEND_MODE_PREMULTIPLIED = 1,
    BLEND_MODE_COVERAGE = 2
} blend_mode_t;

/* Transform Modes */
#define TRANSFORM_FLIP_H 0x01
#define TRANSFORM_FLIP_V 0x02
#define TRANSFORM_ROT_90 0x04
#define TRANSFORM_ROT_180 (TRANSFORM_FLIP_H | TRANSFORM_FLIP_V)
#define TRANSFORM_ROT_270 (TRANSFORM_ROT_90 | TRANSFORM_ROT_180)

/* Rectangle */
typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} rect_t;

/* Region (collection of rectangles) */
#define MAX_REGION_RECTS 16

typedef struct {
    rect_t rects[MAX_REGION_RECTS];
    uint32_t count;
} region_t;

/* Graphics Buffer */
typedef struct {
    uint32_t width;             /* Buffer width */
    uint32_t height;            /* Buffer height */
    uint32_t stride;            /* Row stride in pixels */
    pixel_format_t format;      /* Pixel format */
    void* data;                 /* Buffer data */
    uint32_t size;              /* Buffer size in bytes */
    bool locked;                /* Buffer is locked */
} graphics_buffer_t;

/* Buffer Queue - Manages surface buffers */
#define MAX_BUFFER_SLOTS 3

typedef struct {
    graphics_buffer_t* buffers[MAX_BUFFER_SLOTS]; /* Buffer slots */
    uint32_t num_buffers;       /* Number of allocated buffers */
    int32_t queue_head;         /* Queue head index */
    int32_t queue_tail;         /* Queue tail index */
    int32_t acquired_buffer;    /* Currently acquired buffer */
    bool consumer_connected;    /* Consumer is connected */
} buffer_queue_t;

/* Layer State */
typedef struct {
    rect_t frame;               /* Frame rectangle */
    rect_t crop;                /* Crop rectangle */
    uint32_t z_order;           /* Z-order (layer depth) */
    uint32_t transform;         /* Transform flags */
    uint8_t alpha;              /* Global alpha (0-255) */
    blend_mode_t blend_mode;    /* Blend mode */
    bool visible;               /* Layer is visible */
} layer_state_t;

/* Layer - Represents a surface layer */
typedef struct layer {
    uint32_t id;                /* Layer ID */
    char name[64];              /* Layer name */
    surface_type_t type;        /* Surface type */
    buffer_queue_t* buffer_queue; /* Buffer queue */
    layer_state_t state;        /* Current layer state */
    graphics_buffer_t* active_buffer; /* Currently displayed buffer */
    region_t visible_region;    /* Visible region */
    region_t damage_region;     /* Damaged region needing update */
    struct layer* next;         /* Next layer in list */
} layer_t;

/* Display Device */
typedef struct {
    uint32_t id;                /* Display ID */
    uint32_t width;             /* Display width */
    uint32_t height;            /* Display height */
    pixel_format_t format;      /* Display format */
    void* framebuffer;          /* Framebuffer base address */
    uint32_t framebuffer_size;  /* Framebuffer size */
    uint32_t pitch;             /* Framebuffer pitch */
    bool vsync_enabled;         /* VSync enabled */
    uint32_t refresh_rate;      /* Refresh rate in Hz */
} display_device_t;

/* Composition */
typedef struct {
    layer_t* layers;            /* List of layers */
    uint32_t layer_count;       /* Number of layers */
    uint32_t next_layer_id;     /* Next layer ID */
    display_device_t* display;  /* Target display */
    bool needs_redraw;          /* Composition needs redraw */
    rect_t dirty_rect;          /* Dirty rectangle */
} composition_t;

/* SurfaceFlinger Instance */
typedef struct surfaceflinger {
    composition_t* composition; /* Composition state */
    bool initialized;           /* SurfaceFlinger initialized */
    bool running;               /* Compositor thread running */
    uint32_t frame_count;       /* Total frames rendered */
    uint32_t fps;               /* Current FPS */
} surfaceflinger_t;

/**
 * Initialize SurfaceFlinger subsystem
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_init(void);

/**
 * Shutdown SurfaceFlinger subsystem
 */
void surfaceflinger_shutdown(void);

/**
 * Set display device
 * @param width Display width
 * @param height Display height
 * @param framebuffer Framebuffer address
 * @param pitch Framebuffer pitch
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_display(uint32_t width, uint32_t height, 
                                void* framebuffer, uint32_t pitch);

/**
 * Create new layer
 * @param name Layer name
 * @param type Surface type
 * @return Layer ID or 0 on failure
 */
uint32_t surfaceflinger_create_layer(const char* name, surface_type_t type);

/**
 * Destroy layer
 * @param layer_id Layer ID
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_destroy_layer(uint32_t layer_id);

/**
 * Get layer by ID
 * @param layer_id Layer ID
 * @return Layer or NULL if not found
 */
layer_t* surfaceflinger_get_layer(uint32_t layer_id);

/**
 * Set layer position
 * @param layer_id Layer ID
 * @param x X coordinate
 * @param y Y coordinate
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_layer_position(uint32_t layer_id, int32_t x, int32_t y);

/**
 * Set layer size
 * @param layer_id Layer ID
 * @param width Width
 * @param height Height
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_layer_size(uint32_t layer_id, uint32_t width, uint32_t height);

/**
 * Set layer z-order
 * @param layer_id Layer ID
 * @param z_order Z-order value
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_layer_z_order(uint32_t layer_id, uint32_t z_order);

/**
 * Set layer alpha
 * @param layer_id Layer ID
 * @param alpha Alpha value (0-255)
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_layer_alpha(uint32_t layer_id, uint8_t alpha);

/**
 * Set layer visibility
 * @param layer_id Layer ID
 * @param visible Visibility flag
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_layer_visible(uint32_t layer_id, bool visible);

/**
 * Set layer transform
 * @param layer_id Layer ID
 * @param transform Transform flags
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_set_layer_transform(uint32_t layer_id, uint32_t transform);

/**
 * Allocate graphics buffer
 * @param width Buffer width
 * @param height Buffer height
 * @param format Pixel format
 * @return Buffer or NULL on failure
 */
graphics_buffer_t* surfaceflinger_alloc_buffer(uint32_t width, uint32_t height, 
                                                pixel_format_t format);

/**
 * Free graphics buffer
 * @param buffer Buffer to free
 */
void surfaceflinger_free_buffer(graphics_buffer_t* buffer);

/**
 * Lock buffer for CPU access
 * @param buffer Buffer to lock
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_lock_buffer(graphics_buffer_t* buffer);

/**
 * Unlock buffer
 * @param buffer Buffer to unlock
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_unlock_buffer(graphics_buffer_t* buffer);

/**
 * Queue buffer to layer
 * @param layer_id Layer ID
 * @param buffer Buffer to queue
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_queue_buffer(uint32_t layer_id, graphics_buffer_t* buffer);

/**
 * Dequeue buffer from layer
 * @param layer_id Layer ID
 * @return Buffer or NULL if none available
 */
graphics_buffer_t* surfaceflinger_dequeue_buffer(uint32_t layer_id);

/**
 * Mark region as damaged
 * @param layer_id Layer ID
 * @param rect Damaged rectangle
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_mark_damage(uint32_t layer_id, rect_t* rect);

/**
 * Compose all layers to display
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_compose(void);

/**
 * Enable/disable VSync
 * @param enable Enable flag
 * @return 0 on success, -1 on failure
 */
int surfaceflinger_enable_vsync(bool enable);

/**
 * Get current FPS
 * @return FPS value
 */
uint32_t surfaceflinger_get_fps(void);

/**
 * Get frame count
 * @return Total frames rendered
 */
uint32_t surfaceflinger_get_frame_count(void);

/**
 * Get SurfaceFlinger instance
 * @return SurfaceFlinger instance
 */
surfaceflinger_t* surfaceflinger_get_instance(void);

/**
 * Get SurfaceFlinger version string
 * @return Version string
 */
const char* surfaceflinger_get_version(void);

#endif /* SURFACEFLINGER_H */
