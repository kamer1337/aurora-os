/**
 * Aurora OS - DLL Loader Implementation
 * 
 * Support for loading Windows DLL (Dynamic Link Library) files
 */

#include "dll_loader.h"
#include "pe_loader.h"
#include "../memory/memory.h"
#include "../drivers/vga.h"
#include <stddef.h>

/* DLL table */
static dll_info_t dll_table[MAX_LOADED_DLLS];
static int dll_table_initialized = 0;

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

static void dll_memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
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
    vga_write("DLL Loader: Initialized\n");
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
 * Find DLL by name (case-insensitive for Windows compatibility)
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
 * Verify DLL handle is valid
 */
static dll_info_t* dll_validate_handle(HMODULE module) {
    if (!module) return NULL;
    
    dll_info_t* dll_info = (dll_info_t*)module;
    
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (&dll_table[i] == dll_info && dll_info->handle != NULL) {
            return dll_info;
        }
    }
    return NULL;
}

/**
 * Register a built-in DLL with exports
 */
HMODULE dll_register_builtin(const char* dll_name, dll_export_t* exports, uint32_t count) {
    if (!dll_name) {
        return NULL;
    }
    
    if (!dll_table_initialized) {
        dll_loader_init();
    }
    
    /* Check if DLL is already registered */
    dll_info_t* existing = dll_find_by_name(dll_name);
    if (existing) {
        /* Update exports if provided */
        if (exports && count > 0) {
            existing->exports = exports;
            existing->export_count = count;
        }
        existing->ref_count++;
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
    
    /* Initialize built-in DLL entry */
    dll_memset(dll_info, 0, sizeof(dll_info_t));
    dll_strcpy(dll_info->name, dll_name);
    dll_info->handle = (HMODULE)dll_info;
    dll_info->ref_count = 1;
    dll_info->is_builtin = 1;
    dll_info->exports = exports;
    dll_info->export_count = count;
    
    vga_write("DLL Loader: Registered built-in DLL ");
    vga_write(dll_name);
    vga_write(" with ");
    vga_write_dec((int)count);
    vga_write(" exports\n");
    
    return dll_info->handle;
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
    
    /* Initialize DLL entry */
    dll_memset(dll_info, 0, sizeof(dll_info_t));
    dll_strcpy(dll_info->name, dll_name);
    dll_info->handle = (HMODULE)dll_info;
    dll_info->ref_count = 1;
    dll_info->is_builtin = 0;
    
    /* TODO: In a real implementation:
     * 1. Search for DLL file in system paths
     * 2. Load DLL file from disk using VFS
     * 3. Parse PE format
     * 4. Load into memory
     * 5. Apply base relocations
     * 6. Resolve imports
     */
    
    vga_write("DLL Loader: Loaded ");
    vga_write(dll_name);
    vga_write("\n");
    
    return dll_info->handle;
}

/**
 * Get procedure address from DLL by name
 */
void* dll_get_proc_address(HMODULE module, const char* proc_name) {
    if (!module || !proc_name) {
        return NULL;
    }
    
    dll_info_t* dll_info = dll_validate_handle(module);
    if (!dll_info) {
        return NULL;
    }
    
    /* Check built-in exports first */
    if (dll_info->is_builtin && dll_info->exports) {
        for (uint32_t i = 0; i < dll_info->export_count; i++) {
            if (dll_info->exports[i].name && 
                dll_strcmp(dll_info->exports[i].name, proc_name) == 0) {
                return dll_info->exports[i].address;
            }
        }
    }
    
    /* Check PE exports if image is loaded */
    if (dll_info->image.image_base) {
        void* addr = pe_get_export_by_name(&dll_info->image, proc_name);
        if (addr) {
            return addr;
        }
    }
    
    vga_write("DLL Loader: Function not found: ");
    vga_write(proc_name);
    vga_write(" in ");
    vga_write(dll_info->name);
    vga_write("\n");
    
    return NULL;
}

/**
 * Get procedure address from DLL by ordinal
 */
void* dll_get_proc_address_ordinal(HMODULE module, uint16_t ordinal) {
    if (!module) {
        return NULL;
    }
    
    dll_info_t* dll_info = dll_validate_handle(module);
    if (!dll_info) {
        return NULL;
    }
    
    /* Check PE exports if image is loaded */
    if (dll_info->image.image_base) {
        void* addr = pe_get_export_by_ordinal(&dll_info->image, ordinal);
        if (addr) {
            return addr;
        }
    }
    
    vga_write("DLL Loader: Ordinal not found: ");
    vga_write_dec(ordinal);
    vga_write(" in ");
    vga_write(dll_info->name);
    vga_write("\n");
    
    return NULL;
}

/**
 * Free a loaded DLL
 */
int dll_free(HMODULE module) {
    if (!module) {
        return -1;
    }
    
    dll_info_t* dll_info = dll_validate_handle(module);
    if (!dll_info) {
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
