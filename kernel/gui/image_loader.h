/**
 * Aurora OS - Image Loader
 * 
 * Support for loading PNG and BMP image files for wallpapers
 */

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <stdint.h>
#include "framebuffer.h"

/**
 * Image format types
 */
typedef enum {
    IMAGE_FORMAT_UNKNOWN,
    IMAGE_FORMAT_BMP,
    IMAGE_FORMAT_PNG
} image_format_t;

/**
 * Image data structure
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;           // Bits per pixel
    image_format_t format;
    uint8_t* pixels;        // RGBA pixel data
    uint32_t size;          // Size of pixel data in bytes
} image_data_t;

/**
 * Initialize image loader subsystem
 * @return 0 on success, -1 on failure
 */
int image_loader_init(void);

/**
 * Detect image format from file header
 * @param data File data
 * @param size Size of data
 * @return Image format
 */
image_format_t image_detect_format(const uint8_t* data, uint32_t size);

/**
 * Load BMP image from memory
 * @param data BMP file data
 * @param size Size of data
 * @param out_image Output image data
 * @return 0 on success, -1 on failure
 */
int image_load_bmp(const uint8_t* data, uint32_t size, image_data_t* out_image);

/**
 * Load PNG image from memory (simplified PNG support)
 * @param data PNG file data
 * @param size Size of data
 * @param out_image Output image data
 * @return 0 on success, -1 on failure
 */
int image_load_png(const uint8_t* data, uint32_t size, image_data_t* out_image);

/**
 * Load image from file path
 * @param path File path
 * @param out_image Output image data
 * @return 0 on success, -1 on failure
 */
int image_load_file(const char* path, image_data_t* out_image);

/**
 * Free image data
 * @param image Image to free
 */
void image_free(image_data_t* image);

/**
 * Draw image to framebuffer
 * @param image Image to draw
 * @param x X position
 * @param y Y position
 * @param width Draw width (scale if different from image width)
 * @param height Draw height (scale if different from image height)
 */
void image_draw(const image_data_t* image, int32_t x, int32_t y, uint32_t width, uint32_t height);

#endif // IMAGE_LOADER_H
