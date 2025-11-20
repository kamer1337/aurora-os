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
 * Find DLL by name
 */
static dll_info_t* dll_find_by_name(const char* name) {
    for (int i = 0; i < MAX_LOADED_DLLS; i++) {
        if (dll_table[i].handle != NULL && 
            dll_strcmp(dll_table[i].name, name) == 0) {
            return &dll_table[i];
        }
    }
    return NULL;
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
    
    /* NOTE: In a real implementation, we would:
     * 1. Search for DLL file in system paths
     * 2. Load DLL file from disk
     * 3. Parse PE format
     * 4. Load into memory
     * 5. Resolve imports
     * 
     * For now, we create a stub DLL entry
     */
    
    /* Copy DLL name */
    dll_strcpy(dll_info->name, dll_name);
    
    /* Create a handle (using the dll_info pointer) */
    dll_info->handle = (HMODULE)dll_info;
    dll_info->ref_count = 1;
    
    vga_write("DLL Loader: Loaded ");
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
    
    /* NOTE: In a real implementation, we would:
     * 1. Parse PE export table
     * 2. Search for function by name or ordinal
     * 3. Return function pointer
     * 
     * For now, we return NULL to indicate function not found
     */
    
    vga_write("DLL Loader: Looking up ");
    vga_write(proc_name);
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
