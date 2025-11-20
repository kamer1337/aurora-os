/**
 * @file surfaceflinger.c
 * @brief SurfaceFlinger Display Compositor Implementation
 */

#include "../../include/platform/surfaceflinger.h"

/* Simple memory functions for freestanding environment */
static void* simple_malloc(uint32_t size) {
    /* Stub - in real implementation would use kernel allocator */
    (void)size;
    return (void*)0;
}

static void simple_free(void* ptr) {
    /* Stub - in real implementation would use kernel allocator */
    (void)ptr;
}

static void simple_memset(void* ptr, int value, uint32_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
}

static void simple_memcpy(void* dest, const void* src, uint32_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static void simple_strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    if (i < n) {
        dest[i] = '\0';
    }
}

/* Global SurfaceFlinger state */
static surfaceflinger_t g_surfaceflinger;
static composition_t g_composition;
static display_device_t g_display;

#define SURFACEFLINGER_VERSION "1.0.0-aurora-sf"

/* Helper functions */
static uint32_t get_bytes_per_pixel(pixel_format_t format) {
    switch (format) {
        case PIXEL_FORMAT_RGBA_8888:
        case PIXEL_FORMAT_RGBX_8888:
        case PIXEL_FORMAT_BGRA_8888:
            return 4;
        case PIXEL_FORMAT_RGB_888:
            return 3;
        case PIXEL_FORMAT_RGB_565:
        case PIXEL_FORMAT_RGBA_5551:
        case PIXEL_FORMAT_RGBA_4444:
            return 2;
        default:
            return 4;
    }
}

static void blend_pixel(uint32_t* dest, uint32_t src, uint8_t alpha, blend_mode_t mode) {
    if (mode == BLEND_MODE_NONE || alpha == 255) {
        *dest = src;
        return;
    }
    
    if (alpha == 0) {
        return; /* Fully transparent */
    }
    
    /* Extract components */
    uint8_t src_r = (src >> 16) & 0xFF;
    uint8_t src_g = (src >> 8) & 0xFF;
    uint8_t src_b = src & 0xFF;
    uint8_t src_a = (src >> 24) & 0xFF;
    
    uint8_t dest_r = (*dest >> 16) & 0xFF;
    uint8_t dest_g = (*dest >> 8) & 0xFF;
    uint8_t dest_b = *dest & 0xFF;
    
    /* Apply global alpha */
    src_a = (src_a * alpha) / 255;
    
    /* Alpha blending */
    uint8_t out_r = ((src_r * src_a) + (dest_r * (255 - src_a))) / 255;
    uint8_t out_g = ((src_g * src_a) + (dest_g * (255 - src_a))) / 255;
    uint8_t out_b = ((src_b * src_a) + (dest_b * (255 - src_a))) / 255;
    
    *dest = (255 << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

static bool rect_intersect(rect_t* r1, rect_t* r2, rect_t* result) {
    result->left = (r1->left > r2->left) ? r1->left : r2->left;
    result->top = (r1->top > r2->top) ? r1->top : r2->top;
    result->right = (r1->right < r2->right) ? r1->right : r2->right;
    result->bottom = (r1->bottom < r2->bottom) ? r1->bottom : r2->bottom;
    
    return (result->left < result->right) && (result->top < result->bottom);
}

int surfaceflinger_init(void) {
    if (g_surfaceflinger.initialized) {
        return 0;
    }
    
    /* Initialize globals */
    simple_memset(&g_surfaceflinger, 0, sizeof(surfaceflinger_t));
    simple_memset(&g_composition, 0, sizeof(composition_t));
    simple_memset(&g_display, 0, sizeof(display_device_t));
    
    g_composition.layers = (layer_t*)0;
    g_composition.layer_count = 0;
    g_composition.next_layer_id = 1;
    g_composition.display = &g_display;
    g_composition.needs_redraw = true;
    
    g_surfaceflinger.composition = &g_composition;
    g_surfaceflinger.initialized = true;
    g_surfaceflinger.running = false;
    g_surfaceflinger.frame_count = 0;
    g_surfaceflinger.fps = 60;
    
    return 0;
}

void surfaceflinger_shutdown(void) {
    if (!g_surfaceflinger.initialized) {
        return;
    }
    
    /* Destroy all layers */
    layer_t* layer = g_composition.layers;
    while (layer) {
        layer_t* next = layer->next;
        
        /* Free buffer queue */
        if (layer->buffer_queue) {
            for (uint32_t i = 0; i < layer->buffer_queue->num_buffers; i++) {
                if (layer->buffer_queue->buffers[i]) {
                    surfaceflinger_free_buffer(layer->buffer_queue->buffers[i]);
                }
            }
            simple_free(layer->buffer_queue);
        }
        
        simple_free(layer);
        layer = next;
    }
    
    g_surfaceflinger.initialized = false;
}

int surfaceflinger_set_display(uint32_t width, uint32_t height,
                                void* framebuffer, uint32_t pitch) {
    if (!g_surfaceflinger.initialized) {
        surfaceflinger_init();
    }
    
    g_display.id = 0;
    g_display.width = width;
    g_display.height = height;
    g_display.format = PIXEL_FORMAT_RGBA_8888;
    g_display.framebuffer = framebuffer;
    g_display.framebuffer_size = pitch * height;
    g_display.pitch = pitch;
    g_display.vsync_enabled = true;
    g_display.refresh_rate = 60;
    
    return 0;
}

uint32_t surfaceflinger_create_layer(const char* name, surface_type_t type) {
    if (!g_surfaceflinger.initialized) {
        return 0;
    }
    
    /* Allocate layer */
    layer_t* layer = (layer_t*)simple_malloc(sizeof(layer_t));
    if (!layer) {
        return 0;
    }
    
    /* Initialize layer */
    simple_memset(layer, 0, sizeof(layer_t));
    layer->id = g_composition.next_layer_id++;
    layer->type = type;
    
    if (name) {
        simple_strncpy(layer->name, name, sizeof(layer->name));
    }
    
    /* Initialize layer state */
    layer->state.frame.left = 0;
    layer->state.frame.top = 0;
    layer->state.frame.right = 0;
    layer->state.frame.bottom = 0;
    layer->state.z_order = 0;
    layer->state.transform = 0;
    layer->state.alpha = 255;
    layer->state.blend_mode = BLEND_MODE_PREMULTIPLIED;
    layer->state.visible = true;
    
    /* Allocate buffer queue */
    layer->buffer_queue = (buffer_queue_t*)simple_malloc(sizeof(buffer_queue_t));
    if (!layer->buffer_queue) {
        simple_free(layer);
        return 0;
    }
    
    simple_memset(layer->buffer_queue, 0, sizeof(buffer_queue_t));
    layer->buffer_queue->num_buffers = 0;
    layer->buffer_queue->queue_head = -1;
    layer->buffer_queue->queue_tail = -1;
    layer->buffer_queue->acquired_buffer = -1;
    layer->buffer_queue->consumer_connected = true;
    
    /* Add to layer list */
    layer->next = g_composition.layers;
    g_composition.layers = layer;
    g_composition.layer_count++;
    g_composition.needs_redraw = true;
    
    return layer->id;
}

int surfaceflinger_destroy_layer(uint32_t layer_id) {
    if (!g_surfaceflinger.initialized) {
        return -1;
    }
    
    layer_t* prev = (layer_t*)0;
    layer_t* layer = g_composition.layers;
    
    /* Find layer */
    while (layer) {
        if (layer->id == layer_id) {
            /* Remove from list */
            if (prev) {
                prev->next = layer->next;
            } else {
                g_composition.layers = layer->next;
            }
            
            /* Free buffer queue */
            if (layer->buffer_queue) {
                for (uint32_t i = 0; i < layer->buffer_queue->num_buffers; i++) {
                    if (layer->buffer_queue->buffers[i]) {
                        surfaceflinger_free_buffer(layer->buffer_queue->buffers[i]);
                    }
                }
                simple_free(layer->buffer_queue);
            }
            
            simple_free(layer);
            g_composition.layer_count--;
            g_composition.needs_redraw = true;
            
            return 0;
        }
        
        prev = layer;
        layer = layer->next;
    }
    
    return -1; /* Not found */
}

layer_t* surfaceflinger_get_layer(uint32_t layer_id) {
    layer_t* layer = g_composition.layers;
    while (layer) {
        if (layer->id == layer_id) {
            return layer;
        }
        layer = layer->next;
    }
    return (layer_t*)0;
}

int surfaceflinger_set_layer_position(uint32_t layer_id, int32_t x, int32_t y) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer) {
        return -1;
    }
    
    int32_t width = layer->state.frame.right - layer->state.frame.left;
    int32_t height = layer->state.frame.bottom - layer->state.frame.top;
    
    layer->state.frame.left = x;
    layer->state.frame.top = y;
    layer->state.frame.right = x + width;
    layer->state.frame.bottom = y + height;
    
    g_composition.needs_redraw = true;
    
    return 0;
}

int surfaceflinger_set_layer_size(uint32_t layer_id, uint32_t width, uint32_t height) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer) {
        return -1;
    }
    
    layer->state.frame.right = layer->state.frame.left + (int32_t)width;
    layer->state.frame.bottom = layer->state.frame.top + (int32_t)height;
    
    g_composition.needs_redraw = true;
    
    return 0;
}

int surfaceflinger_set_layer_z_order(uint32_t layer_id, uint32_t z_order) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer) {
        return -1;
    }
    
    layer->state.z_order = z_order;
    g_composition.needs_redraw = true;
    
    return 0;
}

int surfaceflinger_set_layer_alpha(uint32_t layer_id, uint8_t alpha) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer) {
        return -1;
    }
    
    layer->state.alpha = alpha;
    g_composition.needs_redraw = true;
    
    return 0;
}

int surfaceflinger_set_layer_visible(uint32_t layer_id, bool visible) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer) {
        return -1;
    }
    
    layer->state.visible = visible;
    g_composition.needs_redraw = true;
    
    return 0;
}

int surfaceflinger_set_layer_transform(uint32_t layer_id, uint32_t transform) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer) {
        return -1;
    }
    
    layer->state.transform = transform;
    g_composition.needs_redraw = true;
    
    return 0;
}

graphics_buffer_t* surfaceflinger_alloc_buffer(uint32_t width, uint32_t height,
                                                pixel_format_t format) {
    graphics_buffer_t* buffer = (graphics_buffer_t*)simple_malloc(sizeof(graphics_buffer_t));
    if (!buffer) {
        return (graphics_buffer_t*)0;
    }
    
    uint32_t bpp = get_bytes_per_pixel(format);
    uint32_t stride = width;
    uint32_t size = stride * height * bpp;
    
    buffer->width = width;
    buffer->height = height;
    buffer->stride = stride;
    buffer->format = format;
    buffer->size = size;
    buffer->locked = false;
    
    /* Allocate buffer data */
    buffer->data = simple_malloc(size);
    if (!buffer->data) {
        simple_free(buffer);
        return (graphics_buffer_t*)0;
    }
    
    simple_memset(buffer->data, 0, size);
    
    return buffer;
}

void surfaceflinger_free_buffer(graphics_buffer_t* buffer) {
    if (!buffer) {
        return;
    }
    
    if (buffer->data) {
        simple_free(buffer->data);
    }
    
    simple_free(buffer);
}

int surfaceflinger_lock_buffer(graphics_buffer_t* buffer) {
    if (!buffer || buffer->locked) {
        return -1;
    }
    
    buffer->locked = true;
    return 0;
}

int surfaceflinger_unlock_buffer(graphics_buffer_t* buffer) {
    if (!buffer || !buffer->locked) {
        return -1;
    }
    
    buffer->locked = false;
    return 0;
}

int surfaceflinger_queue_buffer(uint32_t layer_id, graphics_buffer_t* buffer) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer || !buffer || !layer->buffer_queue) {
        return -1;
    }
    
    buffer_queue_t* queue = layer->buffer_queue;
    
    /* Add buffer to queue if not already present */
    bool found = false;
    for (uint32_t i = 0; i < queue->num_buffers; i++) {
        if (queue->buffers[i] == buffer) {
            found = true;
            break;
        }
    }
    
    if (!found && queue->num_buffers < MAX_BUFFER_SLOTS) {
        queue->buffers[queue->num_buffers++] = buffer;
    }
    
    /* Set as active buffer */
    layer->active_buffer = buffer;
    g_composition.needs_redraw = true;
    
    return 0;
}

graphics_buffer_t* surfaceflinger_dequeue_buffer(uint32_t layer_id) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer || !layer->buffer_queue) {
        return (graphics_buffer_t*)0;
    }
    
    /* Return first available buffer */
    buffer_queue_t* queue = layer->buffer_queue;
    if (queue->num_buffers > 0) {
        return queue->buffers[0];
    }
    
    return (graphics_buffer_t*)0;
}

int surfaceflinger_mark_damage(uint32_t layer_id, rect_t* rect) {
    layer_t* layer = surfaceflinger_get_layer(layer_id);
    if (!layer || !rect) {
        return -1;
    }
    
    /* Add to damage region */
    if (layer->damage_region.count < MAX_REGION_RECTS) {
        layer->damage_region.rects[layer->damage_region.count++] = *rect;
    }
    
    g_composition.needs_redraw = true;
    
    return 0;
}

int surfaceflinger_compose(void) {
    if (!g_surfaceflinger.initialized || !g_composition.display) {
        return -1;
    }
    
    if (!g_composition.needs_redraw) {
        return 0; /* Nothing to do */
    }
    
    display_device_t* display = g_composition.display;
    if (!display->framebuffer) {
        return -1; /* No framebuffer */
    }
    
    /* Composite layers in z-order */
    /* This is a simplified version - real implementation would:
     * 1. Sort layers by z-order
     * 2. Calculate visible regions
     * 3. Optimize composition (dirty rectangles, etc.)
     * 4. Apply transforms
     * 5. Perform hardware composition if available
     */
    
    layer_t* layer = g_composition.layers;
    while (layer) {
        if (layer->state.visible && layer->active_buffer) {
            graphics_buffer_t* buffer = layer->active_buffer;
            
            /* Simple blit with alpha blending */
            rect_t src_rect = {0, 0, (int32_t)buffer->width, (int32_t)buffer->height};
            rect_t clip_rect;
            
            if (rect_intersect(&layer->state.frame, &src_rect, &clip_rect)) {
                uint32_t* fb = (uint32_t*)display->framebuffer;
                uint32_t* src = (uint32_t*)buffer->data;
                
                for (int32_t y = clip_rect.top; y < clip_rect.bottom; y++) {
                    for (int32_t x = clip_rect.left; x < clip_rect.right; x++) {
                        uint32_t fb_x = (uint32_t)(x + layer->state.frame.left);
                        uint32_t fb_y = (uint32_t)(y + layer->state.frame.top);
                        
                        if (fb_x < display->width && fb_y < display->height) {
                            uint32_t fb_idx = fb_y * (display->pitch / 4) + fb_x;
                            uint32_t src_idx = (uint32_t)y * buffer->stride + (uint32_t)x;
                            
                            blend_pixel(&fb[fb_idx], src[src_idx], 
                                       layer->state.alpha, layer->state.blend_mode);
                        }
                    }
                }
            }
        }
        
        layer = layer->next;
    }
    
    g_composition.needs_redraw = false;
    g_surfaceflinger.frame_count++;
    
    return 0;
}

int surfaceflinger_enable_vsync(bool enable) {
    if (!g_surfaceflinger.initialized || !g_composition.display) {
        return -1;
    }
    
    g_composition.display->vsync_enabled = enable;
    return 0;
}

uint32_t surfaceflinger_get_fps(void) {
    return g_surfaceflinger.fps;
}

uint32_t surfaceflinger_get_frame_count(void) {
    return g_surfaceflinger.frame_count;
}

surfaceflinger_t* surfaceflinger_get_instance(void) {
    return &g_surfaceflinger;
}

const char* surfaceflinger_get_version(void) {
    return SURFACEFLINGER_VERSION;
}
