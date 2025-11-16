/**
 * Aurora OS - Multiboot Specification Header
 * 
 * Defines structures and constants for the Multiboot specification
 * Used to parse boot information from GRUB bootloader
 */

#ifndef AURORA_MULTIBOOT_H
#define AURORA_MULTIBOOT_H

#include <stdint.h>

/* Multiboot magic number */
#define MULTIBOOT_MAGIC 0x2BADB002

/* Multiboot flags */
#define MULTIBOOT_FLAG_MEM     0x00000001  /* Memory info available */
#define MULTIBOOT_FLAG_DEVICE  0x00000002  /* Boot device info available */
#define MULTIBOOT_FLAG_CMDLINE 0x00000004  /* Command line available */
#define MULTIBOOT_FLAG_MODS    0x00000008  /* Modules available */
#define MULTIBOOT_FLAG_AOUT    0x00000010  /* a.out symbol table */
#define MULTIBOOT_FLAG_ELF     0x00000020  /* ELF section header table */
#define MULTIBOOT_FLAG_MMAP    0x00000040  /* Memory map available */
#define MULTIBOOT_FLAG_DRIVES  0x00000080  /* Drive info available */
#define MULTIBOOT_FLAG_CONFIG  0x00000100  /* Config table available */
#define MULTIBOOT_FLAG_LOADER  0x00000200  /* Boot loader name available */
#define MULTIBOOT_FLAG_APM     0x00000400  /* APM table available */
#define MULTIBOOT_FLAG_VBE     0x00000800  /* Video info available */
#define MULTIBOOT_FLAG_FB      0x00001000  /* Framebuffer info available */

/* Multiboot framebuffer types */
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED  0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB      1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2

/**
 * Multiboot information structure
 * Passed by the bootloader to the kernel
 */
typedef struct {
    uint32_t flags;              /* Feature flags */
    
    /* Available if flags[0] is set */
    uint32_t mem_lower;          /* Amount of lower memory in KB */
    uint32_t mem_upper;          /* Amount of upper memory in KB */
    
    /* Available if flags[1] is set */
    uint32_t boot_device;        /* Boot device identifier */
    
    /* Available if flags[2] is set */
    uint32_t cmdline;            /* Physical address of command line */
    
    /* Available if flags[3] is set */
    uint32_t mods_count;         /* Number of modules loaded */
    uint32_t mods_addr;          /* Physical address of module structures */
    
    /* Available if flags[4] or flags[5] is set */
    uint32_t syms[4];            /* Symbol table info */
    
    /* Available if flags[6] is set */
    uint32_t mmap_length;        /* Memory map length */
    uint32_t mmap_addr;          /* Memory map address */
    
    /* Available if flags[7] is set */
    uint32_t drives_length;      /* Drive info length */
    uint32_t drives_addr;        /* Drive info address */
    
    /* Available if flags[8] is set */
    uint32_t config_table;       /* ROM configuration table */
    
    /* Available if flags[9] is set */
    uint32_t boot_loader_name;   /* Boot loader name */
    
    /* Available if flags[10] is set */
    uint32_t apm_table;          /* APM table */
    
    /* Available if flags[11] is set - VBE info */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    
    /* Available if flags[12] is set - Framebuffer info */
    uint64_t framebuffer_addr;   /* Physical address of framebuffer */
    uint32_t framebuffer_pitch;  /* Bytes per scanline */
    uint32_t framebuffer_width;  /* Width in pixels */
    uint32_t framebuffer_height; /* Height in pixels */
    uint8_t  framebuffer_bpp;    /* Bits per pixel */
    uint8_t  framebuffer_type;   /* Framebuffer type */
    
    /* Color info (only for RGB type) */
    union {
        struct {
            uint32_t framebuffer_palette_addr;
            uint16_t framebuffer_palette_num_colors;
        };
        struct {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;
            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;
            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;
        };
    };
} __attribute__((packed)) multiboot_info_t;

/**
 * Memory map entry structure
 */
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

/**
 * Module structure
 */
typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t pad;
} __attribute__((packed)) multiboot_module_t;

#endif /* AURORA_MULTIBOOT_H */
