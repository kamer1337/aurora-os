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
 * Load a DLL into memory from VFS or create stub
 * @param dll_name Name of the DLL to load
 * @return Handle to loaded DLL or NULL on failure
 */
HMODULE dll_load(const char* dll_name);

/**
 * Load a DLL from memory buffer
 * @param dll_name Name to assign to the DLL
 * @param data Pointer to PE file data
 * @param size Size of PE file data
 * @return Handle to loaded DLL or NULL on failure
 */
HMODULE dll_load_from_memory(const char* dll_name, const void* data, uint32_t size);

/**
 * Get procedure address from DLL (uses export table)
 * @param module Handle to DLL
 * @param proc_name Name of procedure to find
 * @return Pointer to procedure or NULL if not found
 */
void* dll_get_proc_address(HMODULE module, const char* proc_name);

/**
 * Get DLL image base address
 * @param module Handle to DLL
 * @return Image base address or NULL
 */
void* dll_get_image_base(HMODULE module);

/**
 * Get DLL entry point
 * @param module Handle to DLL
 * @return Entry point address or NULL
 */
void* dll_get_entry_point(HMODULE module);

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
