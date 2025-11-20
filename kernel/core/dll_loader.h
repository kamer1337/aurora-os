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

/* DLL Handle */
typedef void* HMODULE;

/* DLL Information */
typedef struct {
    char name[256];
    HMODULE handle;
    pe_image_t image;
    int ref_count;
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

#endif /* DLL_LOADER_H */
