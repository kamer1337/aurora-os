/**
 * Aurora OS - DLL Loader Implementation
 * 
 * Support for loading Windows DLL (Dynamic Link Library) files
 */

#include "dll_loader.h"
#include "pe_loader.h"
#include "../memory/memory.h"
#include "../memory/paging.h"
#include "../drivers/vga.h"
#include "../../filesystem/vfs/vfs.h"
#include <stddef.h>

/* DLL table */
static dll_info_t dll_table[MAX_LOADED_DLLS];
static int dll_table_initialized = 0;

/* DLL search paths */
#define MAX_SEARCH_PATHS 8
static const char* dll_search_paths[MAX_SEARCH_PATHS] = {
    "/",
    "/system/",
    "/windows/system32/",
    "/lib/",
    NULL
};

/* String helper functions */
static int dll_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int dll_stricmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

static void dll_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void dll_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    dll_strcpy(dest, src);
}

static int dll_strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

static void dll_memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
}

static void dll_memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
}

/**
 * Initialize DLL loader subsystem
 */
void dll_loader_init(void) {
    if (dll_table_initialized) {
        return;
    }
    
    /* Clear DLL table */
    dll_memset(dll_table, 0, sizeof(dll_table));
    
    dll_table_initialized = 1;
    vga_write("DLL Loader: Initialized with VFS integration\n");
}

/**
 * Find free DLL slot
 */
static dll_info_t* dll_find_free_slot(void) {
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (dll_table[i].handle == NULL) {
            return &dll_table[i];
        }
    }
    return NULL;
}

/**
 * Find DLL by name (case insensitive)
 */
static dll_info_t* dll_find_by_name(const char* name) {
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (dll_table[i].handle != NULL && 
            dll_stricmp(dll_table[i].name, name) == 0) {
            return &dll_table[i];
        }
    }
    return NULL;
}

/**
 * Try to load PE file from VFS
 * Returns pointer to file data or NULL
 */
static void* dll_load_from_vfs(const char* dll_name, uint32_t* file_size) {
    char full_path[512];
    int fd;
    
    /* Try each search path */
    for (int i = 0; dll_search_paths[i] != NULL; i++) {
        dll_strcpy(full_path, dll_search_paths[i]);
        dll_strcat(full_path, dll_name);
        
        fd = vfs_open(full_path, O_RDONLY);
        if (fd >= 0) {
            /* Get file size */
            inode_t stat;
            if (vfs_stat(full_path, &stat) == 0) {
                *file_size = stat.size;
                
                /* Allocate buffer for file data */
                void* data = kmalloc(*file_size);
                if (data) {
                    int bytes_read = vfs_read(fd, data, *file_size);
                    vfs_close(fd);
                    
                    if (bytes_read == (int)*file_size) {
                        vga_write("DLL Loader: Loaded from VFS: ");
                        vga_write(full_path);
                        vga_write("\n");
                        return data;
                    }
                    kfree(data);
                }
            }
            vfs_close(fd);
        }
    }
    
    return NULL;
}

/**
 * Get export address from PE export table
 */
static void* dll_get_export_address(pe_image_t* image, const char* func_name) {
    if (!image || !image->image_base || !image->data_directories || !func_name) {
        return NULL;
    }
    
    /* Get export directory */
    pe_data_directory_t* export_dir = &image->data_directories[PE_DIRECTORY_EXPORT];
    if (export_dir->VirtualAddress == 0 || export_dir->Size == 0) {
        return NULL;
    }
    
    /* Get export directory structure */
    pe_export_directory_t* exports = (pe_export_directory_t*)
        ((uint8_t*)image->image_base + export_dir->VirtualAddress);
    
    /* Get address tables */
    uint32_t* functions = (uint32_t*)
        ((uint8_t*)image->image_base + exports->AddressOfFunctions);
    uint32_t* names = (uint32_t*)
        ((uint8_t*)image->image_base + exports->AddressOfNames);
    uint16_t* ordinals = (uint16_t*)
        ((uint8_t*)image->image_base + exports->AddressOfNameOrdinals);
    
    /* Search for function by name */
    for (uint32_t i = 0; i < exports->NumberOfNames; i++) {
        const char* name = (const char*)
            ((uint8_t*)image->image_base + names[i]);
        
        if (dll_strcmp(name, func_name) == 0) {
            uint16_t ordinal = ordinals[i];
            uint32_t func_rva = functions[ordinal];
            
            /* Check for forwarder (function RVA within export section) */
            if (func_rva >= export_dir->VirtualAddress &&
                func_rva < export_dir->VirtualAddress + export_dir->Size) {
                /* Forwarder - not supported yet */
                vga_write("DLL Loader: Forwarder not supported: ");
                vga_write(func_name);
                vga_write("\n");
                return NULL;
            }
            
            return (uint8_t*)image->image_base + func_rva;
        }
    }
    
    /* Try by ordinal if func_name is a number */
    if ((uintptr_t)func_name <= 0xFFFF) {
        uint16_t ordinal = (uint16_t)(uintptr_t)func_name - (uint16_t)exports->Base;
        if (ordinal < exports->NumberOfFunctions) {
            uint32_t func_rva = functions[ordinal];
            return (uint8_t*)image->image_base + func_rva;
        }
    }
    
    return NULL;
}

/**
 * Resolve imports for a PE image
 */
static int dll_resolve_pe_imports(pe_image_t* image) {
    if (!image || !image->image_base || !image->data_directories) {
        return -1;
    }
    
    /* Get import directory */
    pe_data_directory_t* import_dir = &image->data_directories[PE_DIRECTORY_IMPORT];
    if (import_dir->VirtualAddress == 0 || import_dir->Size == 0) {
        return 0; /* No imports */
    }
    
    /* Get import descriptors */
    pe_import_descriptor_t* import_desc = (pe_import_descriptor_t*)
        ((uint8_t*)image->image_base + import_dir->VirtualAddress);
    
    /* Iterate through import descriptors */
    while (import_desc->NameRVA != 0) {
        const char* dll_name = (const char*)
            ((uint8_t*)image->image_base + import_desc->NameRVA);
        
        vga_write("DLL Loader: Resolving imports from: ");
        vga_write(dll_name);
        vga_write("\n");
        
        /* Load the required DLL */
        HMODULE dep_module = dll_load(dll_name);
        if (!dep_module) {
            vga_write("DLL Loader: Failed to load dependency: ");
            vga_write(dll_name);
            vga_write("\n");
            return -1;
        }
        
        /* Get import lookup table and import address table */
        uint32_t* ilt = (uint32_t*)((uint8_t*)image->image_base + 
            (import_desc->ImportLookupTableRVA ? import_desc->ImportLookupTableRVA : import_desc->ImportAddressTableRVA));
        uint32_t* iat = (uint32_t*)((uint8_t*)image->image_base + import_desc->ImportAddressTableRVA);
        
        /* Resolve each import */
        while (*ilt != 0) {
            void* func_addr = NULL;
            
            if (*ilt & 0x80000000) {
                /* Import by ordinal */
                uint16_t ordinal = (uint16_t)(*ilt & 0xFFFF);
                func_addr = dll_get_proc_address(dep_module, (const char*)(uintptr_t)ordinal);
            } else {
                /* Import by name */
                const char* func_name = (const char*)
                    ((uint8_t*)image->image_base + (*ilt & 0x7FFFFFFF) + 2); /* Skip hint */
                func_addr = dll_get_proc_address(dep_module, func_name);
                
                if (!func_addr) {
                    vga_write("DLL Loader: Import not found: ");
                    vga_write(func_name);
                    vga_write("\n");
                }
            }
            
            /* Write address to IAT */
            *iat = (uint32_t)(uintptr_t)func_addr;
            
            ilt++;
            iat++;
        }
        
        import_desc++;
    }
    
    return 0;
}

/**
 * Apply memory protection to PE sections
 */
static int dll_apply_section_protection(pe_image_t* image) {
    if (!image || !image->image_base || !image->coff_header || !image->section_headers) {
        return -1;
    }
    
    page_directory_t* dir = paging_get_current_directory();
    if (!dir) {
        return -1;
    }
    
    for (uint16_t i = 0; i < image->coff_header->NumberOfSections; i++) {
        pe_section_header_t* section = &image->section_headers[i];
        
        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        
        /* Check section characteristics */
        if (section->Characteristics & PE_SCN_MEM_WRITE) {
            flags |= PAGE_WRITE;
        }
        
        /* Calculate page range for this section */
        uint32_t section_start = (uint32_t)(uintptr_t)image->image_base + section->VirtualAddress;
        uint32_t section_end = section_start + section->VirtualSize;
        
        /* Apply protection to each page in section */
        for (uint32_t addr = section_start & ~(PAGE_SIZE - 1); 
             addr < section_end; 
             addr += PAGE_SIZE) {
            uint32_t phys = paging_get_physical_address(dir, addr);
            if (phys) {
                paging_map_page(dir, addr, phys, flags);
            }
        }
    }
    
    vga_write("DLL Loader: Applied section memory protection\n");
    return 0;
}

/**
 * Load a DLL into memory
 */
HMODULE dll_load(const char* dll_name) {
    if (!dll_name) {
        return NULL;
    }
    
    if (!dll_table_initialized) {
        dll_loader_init();
    }
    
    /* Check if DLL is already loaded */
    dll_info_t* existing = dll_find_by_name(dll_name);
    if (existing) {
        existing->ref_count++;
        vga_write("DLL Loader: ");
        vga_write(dll_name);
        vga_write(" already loaded (refcount: ");
        vga_write_dec(existing->ref_count);
        vga_write(")\n");
        return existing->handle;
    }
    
    /* Find free slot */
    dll_info_t* dll_info = dll_find_free_slot();
    if (!dll_info) {
        vga_write("DLL Loader: No free slots for ");
        vga_write(dll_name);
        vga_write("\n");
        return NULL;
    }
    
    /* Try to load from VFS */
    uint32_t file_size = 0;
    void* file_data = dll_load_from_vfs(dll_name, &file_size);
    
    if (file_data && file_size > 0) {
        /* Parse and load PE image */
        if (pe_load(file_data, file_size, &dll_info->image) == 0) {
            /* Apply section protection */
            dll_apply_section_protection(&dll_info->image);
            
            /* Resolve imports */
            if (dll_resolve_pe_imports(&dll_info->image) != 0) {
                vga_write("DLL Loader: Failed to resolve imports for ");
                vga_write(dll_name);
                vga_write("\n");
                pe_unload(&dll_info->image);
                kfree(file_data);
                return NULL;
            }
            
            kfree(file_data);
            
            /* Copy DLL name */
            dll_strcpy(dll_info->name, dll_name);
            dll_info->handle = (HMODULE)dll_info;
            dll_info->ref_count = 1;
            
            vga_write("DLL Loader: Successfully loaded PE: ");
            vga_write(dll_name);
            vga_write("\n");
            
            return dll_info->handle;
        }
        kfree(file_data);
    }
    
    /* Fall back to stub DLL entry for built-in DLLs */
    vga_write("DLL Loader: Creating stub for: ");
    vga_write(dll_name);
    vga_write("\n");
    
    /* Copy DLL name */
    dll_strcpy(dll_info->name, dll_name);
    
    /* Create a handle (using the dll_info pointer) */
    dll_info->handle = (HMODULE)dll_info;
    dll_info->ref_count = 1;
    dll_memset(&dll_info->image, 0, sizeof(pe_image_t));
    
    return dll_info->handle;
}

/**
 * Load DLL from memory buffer
 */
HMODULE dll_load_from_memory(const char* dll_name, const void* data, uint32_t size) {
    if (!dll_name || !data || size == 0) {
        return NULL;
    }
    
    if (!dll_table_initialized) {
        dll_loader_init();
    }
    
    /* Check if already loaded */
    dll_info_t* existing = dll_find_by_name(dll_name);
    if (existing) {
        existing->ref_count++;
        return existing->handle;
    }
    
    /* Find free slot */
    dll_info_t* dll_info = dll_find_free_slot();
    if (!dll_info) {
        return NULL;
    }
    
    /* Load PE image from memory */
    if (pe_load(data, size, &dll_info->image) != 0) {
        vga_write("DLL Loader: Failed to parse PE from memory\n");
        return NULL;
    }
    
    /* Apply section protection */
    dll_apply_section_protection(&dll_info->image);
    
    /* Resolve imports */
    if (dll_resolve_pe_imports(&dll_info->image) != 0) {
        pe_unload(&dll_info->image);
        return NULL;
    }
    
    /* Copy DLL name */
    dll_strcpy(dll_info->name, dll_name);
    dll_info->handle = (HMODULE)dll_info;
    dll_info->ref_count = 1;
    
    vga_write("DLL Loader: Loaded from memory: ");
    vga_write(dll_name);
    vga_write("\n");
    
    return dll_info->handle;
}

/**
 * Get procedure address from DLL
 */
void* dll_get_proc_address(HMODULE module, const char* proc_name) {
    if (!module || !proc_name) {
        return NULL;
    }
    
    dll_info_t* dll_info = (dll_info_t*)module;
    
    /* Verify this is a valid DLL handle */
    int valid = 0;
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (&dll_table[i] == dll_info && dll_info->handle != NULL) {
            valid = 1;
            break;
        }
    }
    
    if (!valid) {
        return NULL;
    }
    
    /* If PE image is loaded, search export table */
    if (dll_info->image.image_base) {
        void* addr = dll_get_export_address(&dll_info->image, proc_name);
        if (addr) {
            return addr;
        }
    }
    
    /* Log lookup for debugging */
    vga_write("DLL Loader: Looking up ");
    vga_write(proc_name);
    vga_write(" in ");
    vga_write(dll_info->name);
    vga_write("\n");
    
    return NULL;
}

/**
 * Get DLL image base address
 */
void* dll_get_image_base(HMODULE module) {
    if (!module) {
        return NULL;
    }
    
    dll_info_t* dll_info = (dll_info_t*)module;
    
    /* Verify valid handle */
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (&dll_table[i] == dll_info && dll_info->handle != NULL) {
            return dll_info->image.image_base;
        }
    }
    
    return NULL;
}

/**
 * Get DLL entry point
 */
void* dll_get_entry_point(HMODULE module) {
    if (!module) {
        return NULL;
    }
    
    dll_info_t* dll_info = (dll_info_t*)module;
    
    /* Verify valid handle */
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (&dll_table[i] == dll_info && dll_info->handle != NULL) {
            return pe_get_entry_point(&dll_info->image);
        }
    }
    
    return NULL;
}

/**
 * Free a loaded DLL
 */
int dll_free(HMODULE module) {
    if (!module) {
        return -1;
    }
    
    dll_info_t* dll_info = (dll_info_t*)module;
    
    /* Verify this is a valid DLL handle */
    int valid = 0;
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (&dll_table[i] == dll_info && dll_info->handle != NULL) {
            valid = 1;
            break;
        }
    }
    
    if (!valid) {
        return -1;
    }
    
    /* Decrement reference count */
    dll_info->ref_count--;
    
    if (dll_info->ref_count <= 0) {
        vga_write("DLL Loader: Unloading ");
        vga_write(dll_info->name);
        vga_write("\n");
        
        /* Unload PE image if loaded */
        if (dll_info->image.image_base) {
            pe_unload(&dll_info->image);
        }
        
        /* Clear DLL info */
        dll_memset(dll_info, 0, sizeof(dll_info_t));
    }
    
    return 0;
}

/**
 * Get DLL handle by name
 */
HMODULE dll_get_module_handle(const char* dll_name) {
    if (!dll_name) {
        return NULL;
    }
    
    dll_info_t* dll_info = dll_find_by_name(dll_name);
    return dll_info ? dll_info->handle : NULL;
}

/**
 * Check if DLL is loaded
 */
int dll_is_loaded(const char* dll_name) {
    return dll_get_module_handle(dll_name) != NULL;
}
