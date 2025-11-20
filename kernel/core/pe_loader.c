/**
 * Aurora OS - PE/COFF Binary Loader Implementation
 * 
 * Support for loading Windows PE (Portable Executable) format binaries
 */

#include "pe_loader.h"
#include "../memory/memory.h"
#include "../drivers/vga.h"
#include <stddef.h>

/* String functions for PE loader */
static int pe_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void pe_memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
}

static void pe_memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
}

/**
 * Initialize PE loader subsystem
 */
void pe_loader_init(void) {
    vga_write("PE Loader: Initialized\n");
}

/**
 * Validate PE file format
 */
int pe_validate(const void* data, uint32_t size) {
    if (!data || size < sizeof(pe_dos_header_t)) {
        return 0;
    }
    
    /* Check DOS header */
    pe_dos_header_t* dos_header = (pe_dos_header_t*)data;
    if (dos_header->e_magic != PE_DOS_SIGNATURE) {
        return 0;
    }
    
    /* Check PE signature offset */
    if (dos_header->e_lfanew >= size - sizeof(uint32_t)) {
        return 0;
    }
    
    /* Check PE signature */
    uint32_t* pe_sig = (uint32_t*)((uint8_t*)data + dos_header->e_lfanew);
    if (*pe_sig != PE_NT_SIGNATURE) {
        return 0;
    }
    
    return 1;
}

/**
 * Load PE executable into memory
 */
int pe_load(const void* data, uint32_t size, pe_image_t* image) {
    if (!pe_validate(data, size) || !image) {
        return -1;
    }
    
    /* Initialize image structure */
    pe_memset(image, 0, sizeof(pe_image_t));
    
    /* Get DOS header */
    image->dos_header = (pe_dos_header_t*)data;
    
    /* Get COFF header */
    uint8_t* pe_header = (uint8_t*)data + image->dos_header->e_lfanew;
    pe_header += sizeof(uint32_t); /* Skip PE signature */
    image->coff_header = (pe_coff_header_t*)pe_header;
    
    /* Verify machine type */
    if (image->coff_header->Machine != PE_MACHINE_I386 &&
        image->coff_header->Machine != PE_MACHINE_AMD64) {
        vga_write("PE Loader: Unsupported machine type\n");
        return -1;
    }
    
    /* Get optional header */
    pe_header += sizeof(pe_coff_header_t);
    image->optional_header = (pe_optional_header32_t*)pe_header;
    
    /* Verify optional header magic */
    if (image->optional_header->Magic != PE_OPTIONAL_HDR_MAGIC_PE32) {
        vga_write("PE Loader: Only PE32 format supported\n");
        return -1;
    }
    
    /* Get data directories */
    pe_header += sizeof(pe_optional_header32_t);
    image->data_directories = (pe_data_directory_t*)pe_header;
    
    /* Get section headers */
    pe_header = (uint8_t*)image->coff_header + sizeof(pe_coff_header_t) +
                image->coff_header->SizeOfOptionalHeader;
    image->section_headers = (pe_section_header_t*)pe_header;
    
    /* Calculate image size */
    image->image_size = image->optional_header->SizeOfImage;
    
    /* Allocate memory for image */
    image->image_base = kmalloc(image->image_size);
    if (!image->image_base) {
        vga_write("PE Loader: Failed to allocate memory for image\n");
        return -1;
    }
    
    /* Zero out image memory */
    pe_memset(image->image_base, 0, image->image_size);
    
    /* Load sections into memory */
    for (uint16_t i = 0; i < image->coff_header->NumberOfSections; i++) {
        pe_section_header_t* section = &image->section_headers[i];
        
        /* Calculate destination address */
        void* dest = (uint8_t*)image->image_base + section->VirtualAddress;
        
        /* Calculate source address */
        const void* src = (uint8_t*)data + section->PointerToRawData;
        
        /* Copy section data if it exists */
        if (section->SizeOfRawData > 0) {
            uint32_t copy_size = section->SizeOfRawData;
            if (copy_size > section->VirtualSize) {
                copy_size = section->VirtualSize;
            }
            pe_memcpy(dest, src, copy_size);
        }
    }
    
    /* Set entry point */
    image->entry_point = image->optional_header->AddressOfEntryPoint;
    
    vga_write("PE Loader: Image loaded successfully\n");
    return 0;
}

/**
 * Unload PE image from memory
 */
void pe_unload(pe_image_t* image) {
    if (!image) {
        return;
    }
    
    if (image->image_base) {
        kfree(image->image_base);
        image->image_base = NULL;
    }
    
    pe_memset(image, 0, sizeof(pe_image_t));
}

/**
 * Get PE entry point address
 */
void* pe_get_entry_point(pe_image_t* image) {
    if (!image || !image->image_base) {
        return NULL;
    }
    
    return (uint8_t*)image->image_base + image->entry_point;
}

/**
 * Resolve imports in PE image
 */
int pe_resolve_imports(pe_image_t* image) {
    if (!image || !image->image_base || !image->data_directories) {
        return -1;
    }
    
    /* Get import directory */
    pe_data_directory_t* import_dir = &image->data_directories[PE_DIRECTORY_IMPORT];
    if (import_dir->VirtualAddress == 0 || import_dir->Size == 0) {
        /* No imports */
        return 0;
    }
    
    /* Get import descriptors */
    pe_import_descriptor_t* import_desc = (pe_import_descriptor_t*)
        ((uint8_t*)image->image_base + import_dir->VirtualAddress);
    
    /* Iterate through import descriptors */
    while (import_desc->NameRVA != 0) {
        /* Get DLL name */
        const char* dll_name = (const char*)
            ((uint8_t*)image->image_base + import_desc->NameRVA);
        
        /* Note: Actual DLL loading and symbol resolution would happen here */
        /* For now, we just log the DLL name */
        vga_write("PE Loader: Import from DLL: ");
        vga_write(dll_name);
        vga_write("\n");
        
        import_desc++;
    }
    
    return 0;
}
