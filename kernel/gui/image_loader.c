/**
 * Aurora OS - Image Loader Implementation
 * 
 * Support for loading PNG and BMP image files for wallpapers
 */

#include "image_loader.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../../filesystem/vfs/vfs.h"
#include <stddef.h>

// BMP file header structures
#pragma pack(push, 1)
typedef struct {
    uint16_t type;          // "BM" = 0x4D42
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;        // Offset to pixel data
} bmp_file_header_t;

typedef struct {
    uint32_t size;          // Size of this header
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;           // Bits per pixel
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_info_header_t;
#pragma pack(pop)

// PNG signature
static const uint8_t PNG_SIGNATURE[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

// Helper functions
static void* img_memset(void* dest, uint8_t val, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = val;
    }
    return dest;
}

static void* img_memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static int img_memcmp(const void* s1, const void* s2, uint32_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (uint32_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

int image_loader_init(void) {
    return 0;
}

image_format_t image_detect_format(const uint8_t* data, uint32_t size) {
    if (!data || size < 8) {
        return IMAGE_FORMAT_UNKNOWN;
    }
    
    // Check for BMP signature "BM"
    if (size >= 2 && data[0] == 'B' && data[1] == 'M') {
        return IMAGE_FORMAT_BMP;
    }
    
    // Check for PNG signature
    if (size >= 8 && img_memcmp(data, PNG_SIGNATURE, 8) == 0) {
        return IMAGE_FORMAT_PNG;
    }
    
    return IMAGE_FORMAT_UNKNOWN;
}

int image_load_bmp(const uint8_t* data, uint32_t size, image_data_t* out_image) {
    if (!data || !out_image || size < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t)) {
        return -1;
    }
    
    // Read file header
    bmp_file_header_t file_header;
    img_memcpy(&file_header, data, sizeof(file_header));
    
    // Verify BMP signature
    if (file_header.type != 0x4D42) {  // "BM"
        return -1;
    }
    
    // Read info header
    bmp_info_header_t info_header;
    img_memcpy(&info_header, data + sizeof(file_header), sizeof(info_header));
    
    // Only support uncompressed BMP (BI_RGB = 0)
    if (info_header.compression != 0) {
        return -1;
    }
    
    // Only support 24 or 32 bit BMP
    if (info_header.bpp != 24 && info_header.bpp != 32) {
        return -1;
    }
    
    // Handle negative height (top-down BMP)
    int32_t height = info_header.height;
    uint8_t flip_vertical = 0;
    if (height < 0) {
        height = -height;
        flip_vertical = 1;
    }
    
    // Validate dimensions
    if (info_header.width <= 0 || height <= 0 || info_header.width > 4096 || height > 4096) {
        return -1;
    }
    
    // Allocate pixel buffer (RGBA format)
    uint32_t pixel_count = info_header.width * height;
    uint32_t pixel_size = pixel_count * 4;  // 4 bytes per pixel (RGBA)
    uint8_t* pixels = (uint8_t*)kmalloc(pixel_size);
    if (!pixels) {
        return -1;
    }
    
    // Read pixel data
    const uint8_t* src = data + file_header.offset;
    uint32_t bytes_per_pixel = info_header.bpp / 8;
    uint32_t row_size = ((info_header.width * info_header.bpp + 31) / 32) * 4;  // Rows are 4-byte aligned
    
    for (int32_t y = 0; y < height; y++) {
        int32_t dest_y = flip_vertical ? y : (height - 1 - y);
        uint8_t* dest_row = pixels + (dest_y * info_header.width * 4);
        const uint8_t* src_row = src + (y * row_size);
        
        for (int32_t x = 0; x < info_header.width; x++) {
            const uint8_t* src_pixel = src_row + (x * bytes_per_pixel);
            uint8_t* dest_pixel = dest_row + (x * 4);
            
            // BMP is BGR(A), convert to RGBA
            dest_pixel[0] = src_pixel[2];  // R
            dest_pixel[1] = src_pixel[1];  // G
            dest_pixel[2] = src_pixel[0];  // B
            dest_pixel[3] = (info_header.bpp == 32) ? src_pixel[3] : 255;  // A
        }
    }
    
    // Fill output structure
    out_image->width = info_header.width;
    out_image->height = height;
    out_image->bpp = 32;  // Always output as 32-bit RGBA
    out_image->format = IMAGE_FORMAT_BMP;
    out_image->pixels = pixels;
    out_image->size = pixel_size;
    
    return 0;
}

int image_load_png(const uint8_t* data, uint32_t size, image_data_t* out_image) {
    if (!data || !out_image || size < 8) {
        return -1;
    }
    
    // Verify PNG signature
    if (img_memcmp(data, PNG_SIGNATURE, 8) != 0) {
        return -1;
    }
    
    // Simplified PNG loader - only supports basic uncompressed PNG
    // In a full implementation, this would use zlib for decompression
    // For now, return error to indicate PNG is not fully supported
    // Future enhancement: integrate with a lightweight PNG library
    
    return -1;  // PNG not yet fully implemented
}

int image_load_file(const char* path, image_data_t* out_image) {
    if (!path || !out_image) {
        return -1;
    }
    
    // Open file
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Get file size using stat
    inode_t stat;
    if (vfs_stat(path, &stat) != 0) {
        vfs_close(fd);
        return -1;
    }
    
    uint32_t file_size = stat.size;
    if (file_size == 0 || file_size > 10 * 1024 * 1024) {  // Max 10MB
        vfs_close(fd);
        return -1;
    }
    
    // Allocate buffer for file data
    uint8_t* buffer = (uint8_t*)kmalloc(file_size);
    if (!buffer) {
        vfs_close(fd);
        return -1;
    }
    
    // Read file
    int bytes_read = vfs_read(fd, buffer, file_size);
    vfs_close(fd);
    
    if (bytes_read != (int)file_size) {
        kfree(buffer);
        return -1;
    }
    
    // Detect format
    image_format_t format = image_detect_format(buffer, file_size);
    
    int result = -1;
    switch (format) {
        case IMAGE_FORMAT_BMP:
            result = image_load_bmp(buffer, file_size, out_image);
            break;
        
        case IMAGE_FORMAT_PNG:
            result = image_load_png(buffer, file_size, out_image);
            break;
        
        default:
            break;
    }
    
    kfree(buffer);
    return result;
}

void image_free(image_data_t* image) {
    if (!image) {
        return;
    }
    
    if (image->pixels) {
        kfree(image->pixels);
        image->pixels = NULL;
    }
    
    image->width = 0;
    image->height = 0;
    image->size = 0;
}

void image_draw(const image_data_t* image, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!image || !image->pixels) {
        return;
    }
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb || !fb->address) {
        return;
    }
    
    // Simple nearest-neighbor scaling
    for (uint32_t dy = 0; dy < height; dy++) {
        for (uint32_t dx = 0; dx < width; dx++) {
            // Map destination pixel to source pixel
            uint32_t sx = (dx * image->width) / width;
            uint32_t sy = (dy * image->height) / height;
            
            if (sx >= image->width || sy >= image->height) {
                continue;
            }
            
            // Get source pixel (RGBA)
            uint8_t* src_pixel = image->pixels + ((sy * image->width + sx) * 4);
            
            // Create color
            color_t color;
            color.r = src_pixel[0];
            color.g = src_pixel[1];
            color.b = src_pixel[2];
            color.a = src_pixel[3];
            
            // Draw pixel with alpha blending
            int32_t dest_x = x + dx;
            int32_t dest_y = y + dy;
            
            if (dest_x >= 0 && dest_x < (int32_t)fb->width && 
                dest_y >= 0 && dest_y < (int32_t)fb->height) {
                
                if (color.a == 255) {
                    // Fully opaque - direct draw
                    framebuffer_draw_pixel(dest_x, dest_y, color);
                } else if (color.a > 0) {
                    // Alpha blending
                    // Get destination pixel
                    uint32_t dest_offset = dest_y * fb->pitch + dest_x * (fb->bpp / 8);
                    uint8_t* dest_ptr = (uint8_t*)fb->address + dest_offset;
                    
                    // Blend
                    uint8_t alpha = color.a;
                    uint8_t inv_alpha = 255 - alpha;
                    
                    uint8_t dest_r = dest_ptr[2];
                    uint8_t dest_g = dest_ptr[1];
                    uint8_t dest_b = dest_ptr[0];
                    
                    color.r = (color.r * alpha + dest_r * inv_alpha) / 255;
                    color.g = (color.g * alpha + dest_g * inv_alpha) / 255;
                    color.b = (color.b * alpha + dest_b * inv_alpha) / 255;
                    
                    framebuffer_draw_pixel(dest_x, dest_y, color);
                }
            }
        }
    }
}
