/**
 * Aurora OS - DLL Loader
 * 
 * Support for loading Windows DLL (Dynamic Link Library) files
 */

#ifndef DLL_LOADER_H
#define DLL_LOADER_H

#include <stdint.h>
#include "pe_loader.h"

/* Maximum number of loaded DLLs */
#define MAX_LOADED_DLLS 64

/* Maximum number of exported functions per DLL */
#define MAX_DLL_EXPORTS 256

/* DLL Handle */
typedef void* HMODULE;

/* Export entry for built-in DLLs */
typedef struct {
    const char* name;
    void* address;
} dll_export_t;

/* DLL Information */
typedef struct {
    char name[256];
    HMODULE handle;
    pe_image_t image;
    int ref_count;
    int is_builtin;              /* Flag for built-in DLLs */
    dll_export_t* exports;       /* Export table for built-in DLLs */
    uint32_t export_count;       /* Number of exports */
} dll_info_t;

/**
 * Initialize DLL loader subsystem
 */
void dll_loader_init(void);

/**
 * Load a DLL into memory
 * @param dll_name Name of the DLL to load
 * @return Handle to loaded DLL or NULL on failure
 */
HMODULE dll_load(const char* dll_name);

/**
 * Get procedure address from DLL
 * @param module Handle to DLL
 * @param proc_name Name of procedure to find
 * @return Pointer to procedure or NULL if not found
 */
void* dll_get_proc_address(HMODULE module, const char* proc_name);

/**
 * Get procedure address from DLL by ordinal
 * @param module Handle to DLL
 * @param ordinal Ordinal number of procedure
 * @return Pointer to procedure or NULL if not found
 */
void* dll_get_proc_address_ordinal(HMODULE module, uint16_t ordinal);

/**
 * Free a loaded DLL
 * @param module Handle to DLL to free
 * @return 0 on success, -1 on error
 */
int dll_free(HMODULE module);

/**
 * Get DLL handle by name
 * @param dll_name Name of DLL
 * @return Handle to DLL or NULL if not loaded
 */
HMODULE dll_get_module_handle(const char* dll_name);

/**
 * Check if DLL is loaded
 * @param dll_name Name of DLL
 * @return 1 if loaded, 0 otherwise
 */
int dll_is_loaded(const char* dll_name);

/**
 * Register a built-in DLL with exports
 * @param dll_name Name of the DLL
 * @param exports Array of export entries
 * @param count Number of exports
 * @return Handle to DLL or NULL on failure
 */
HMODULE dll_register_builtin(const char* dll_name, dll_export_t* exports, uint32_t count);

#endif /* DLL_LOADER_H */
